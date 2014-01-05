/*
Copyright (c) 2013, 2014 Tolga HOŞGÖR
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies, 
either expressed or implied, of the FreeBSD Project.
*/

#include "postgrespp.h"

#include <iostream>

namespace postgrespp
{

/*
 * EscapedLiteral
 */
EscapedLiteral::EscapedLiteral(char* const& data) : data_(data)
{};

EscapedLiteral::EscapedLiteral(EscapedLiteral&& o)
{
	o.data_ = nullptr;
}

EscapedLiteral::~EscapedLiteral(){ if(data_ != nullptr) PQfreemem(data_); }

/*
 * Result
 */
Result::Result(PGresult* const& res) :
   res_(res),
   nrows_(PQntuples(res_))
{}

Result::Result(Result&& o) :
	res_(o.res_),
	nrows_(o.nrows_)
{
	o.moved_ = true;
}

Result::~Result()
{
	if(!moved_ && res_ != nullptr)
		PQclear(res_);
}

/*
 * Connection
 */
Connection::Connection(boost::asio::io_service& is) :
	is_(is),
	socket_(is),
	status_(Status::DEACTIVE)
{}

Connection::Connection(boost::asio::io_service& is, const char* const& pgconninfo) :
	Connection(is)
{
	connect(pgconninfo);
}

Connection::~Connection()
{
#ifndef NDEBUG
	std::cerr << "postgres++: connection ended.\n";
#endif
	PQfinish(handle_);
}

ConnStatusType Connection::connect(const char* const& pgconninfo)
{
	handle_ = PQconnectdb(pgconninfo);
	bool success = !PQsetnonblocking(handle_, 1) && status() == CONNECTION_OK;
	if(success)
		socket_.assign(boost::asio::ip::tcp::v4(), PQsocket(handle_));
	else
	{
#ifndef NDEBUG
		std::cerr << "postgres++: connection error: " << PQerrorMessage(handle_) << std::endl;
#endif
		handle_ = nullptr;
	}
	return status();
}

ConnStatusType Connection::status()
{
	return PQstatus(handle_);
}

EscapedLiteral Connection::escapeLiteral(const char* const& str)
{
	return escapeLiteral(str, strlen(str));
}

EscapedLiteral Connection::escapeLiteral(std::string const& str)
{
	return escapeLiteral(str.data(), str.length());
}

EscapedLiteral Connection::escapeLiteral(const char* const& str, size_t const& len)
{
	return EscapedLiteral(PQescapeLiteral(handle_, str, len));
}

PGresult* Connection::prepare(const char* const& stmt_name, const char* const& query, int const& n_params,
			const Oid* const& param_types)
{
	return PQprepare(handle_, stmt_name, query, n_params, param_types);
}

/*
 * Pool
 */
Pool::Pool(boost::asio::io_service& is, const char* const& pgconninfo, Pool::Settings const& settings) :
	settings_(settings),
	is_(is),
	pgconninfo_(pgconninfo),
	dtimer_(is)
{
	createConn(settings.min_conn_count);
}

size_t Pool::createConn(size_t n)
{
#ifndef NDEBUG
	std::cout << "postgres++: Creating " << pool_.size() << ". db connection.\n";
#endif
	while(n)
	{
		pool_.emplace_back(is_, pgconninfo_.c_str());
		if(pool_.back().status() != CONNECTION_OK)
		{
			pool_.pop_back();
			return n;
		}
		if(settings_.spawn_func != nullptr)
			settings_.spawn_func(pool_.back());
		--n;
	}
	
	return n;
}

Connection* Pool::getFreeConnection()
{
	std::lock_guard<std::mutex> lg(poolLock_);

	Connection* c = nullptr;
	for(auto& it : pool_)
	{
		if(it.status_ == Connection::Status::DEACTIVE)
		{
			it.status_ = Connection::Status::ACTIVE;
			c = &it;
			break;
		}
	}

	if(c == nullptr)
	{
		if(pool_.size() >= settings_.max_conn_count)
			return nullptr;
		if(createConn(1) == 1)
			return nullptr;
		c = &pool_.back();
		c->status_ = Connection::Status::ACTIVE;
	}
	
	return c;
}

bool Pool::query(const char* const& query, Callback cb)
{
	auto c = getFreeConnection();
	if(c == nullptr)
		return false;

	PQsendQuery(c->handle_, query);
	c->socket_.async_read_some(boost::asio::null_buffers(), std::bind(&Pool::asyncQueryCb, this, std::placeholders::_1,
			std::placeholders::_2, std::ref(*c), std::move(cb)));
			
	return true;
}

bool Pool::queryParams(const char* const& query, int const& n_params, ResultFormat const& result_format, Callback cb, const char* const* const& param_values, const int* const& param_lengths, const int* const& param_formats, const Oid* const& param_types)
{
	auto c = getFreeConnection();
	if(c == nullptr)
		return false;

	PQsendQueryParams(c->handle_, query, n_params, param_types, param_values, param_lengths, param_formats, static_cast<int>(result_format));
	c->socket_.async_read_some(boost::asio::null_buffers(), std::bind(&Pool::asyncQueryCb, this, std::placeholders::_1,
			std::placeholders::_2, std::ref(*c), std::move(cb)));
	return true;
}

void Pool::asyncQueryCb(boost::system::error_code const& ec, size_t const& bt, Connection& conn, Callback cb)
{
	PGresult* res = nullptr;
	if(!ec)
	{
		auto ret = PQconsumeInput(conn.handle_);
		if(!ret)
			std::cerr << PQerrorMessage(conn.handle_) << std::endl;
		//TODO: need a self call with PQisBusy query here?
		res = PQgetResult(conn.handle_);
		while(PQgetResult(conn.handle_) != nullptr);
		conn.status_ = Connection::Status::DEACTIVE;
		cb(ec, {{res}});
	}
	else
	{
		conn.status_ = Connection::Status::DEACTIVE;
		cb(boost::system::error_code(1, boost::system::system_category()), {{res}});
	}
	
	dtimer_.expires_from_now(boost::posix_time::seconds(settings_.conn_unused_timeout));
	dtimer_.async_wait([this](boost::system::error_code const& ec){
			if(!ec)
			{
				std::lock_guard<std::mutex> lg(poolLock_);
				if(pool_.size() > settings_.min_conn_count)
				{
					if(pool_.back().status_ == Connection::Status::DEACTIVE)
					{
						pool_.pop_back();
					}
				}
			}
		});
}

}
