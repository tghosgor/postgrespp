/*
Copyright (c) 2013, Tolga HOŞGÖR
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

EscapedLiteral::EscapedLiteral(char* const& data) : data_(data)
{};

EscapedLiteral::~EscapedLiteral(){ PQfreemem(data_); }

inline char* const& EscapedLiteral::c_str() { return data_; }

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

int Result::rows()
{
	return nrows_;
}

bool Result::next()
{
	return (++row_ < rows());
}

char* Result::get(int const& column)
{
	return PQgetvalue(res_, row_, column);
}

void Result::reset()
{
	row_ = -1;
}

Connection::Connection(boost::asio::io_service& is) :
		is_(is),
		socket_(is)
{}

Connection::Connection(boost::asio::io_service& is, const char* const& pgconninfo) :
		Connection(is)
{
	connect(pgconninfo);
}

Connection::~Connection()
{
#ifndef NDEBUG
	std::cerr << "PG connection ended.\n";
#endif
	PQfinish(handle_);
}

ConnStatusType Connection::connect(const char* const& pgconninfo)
{
	handle_ = PQconnectdb(pgconninfo);
	bool success = !PQsetnonblocking(handle_, 1) && status() == CONNECTION_OK && PQsocket(handle_) != -1;
	assert(success);
	if(success)
		socket_.assign(boost::asio::ip::tcp::v4(), PQsocket(handle_));
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

PGresult* Connection::prepare(const char* const& stmtName, const char* const& query, int const& nParams,
			const Oid* const& paramTypes)
{
	return PQprepare(handle_, stmtName, query, nParams, paramTypes);
}

Pool::Pool(boost::asio::io_service& is, const char* const& pgconninfo, size_t n, SpawnFunctor sf):
		is_(is),
		pgconninfo_(pgconninfo),
		spawnFunctor_(std::move(sf))
{
	while(n)
	{
		pool_.emplace_back(is_, pgconninfo_.c_str());
		if(spawnFunctor_ != nullptr)
			spawnFunctor_(pool_.back());
		--n;
	}
}

Pool::Pool(boost::asio::io_service& is, const char* const& pgconninfo, size_t n) :
Pool(is, pgconninfo, n, nullptr)
{}

bool Pool::query(const char* const& query, Callback cb)
{
	//std::cout << "1(3)\n";

	std::unique_lock<std::mutex> lg(poolLock_);

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
#ifndef _NDEBUG
		std::cout << "Creating " << pool_.size() << ". db connection.\n";
#endif
		if(pool_.size() > 75)
			return false;
		pool_.emplace_back(is_, pgconninfo_.c_str());
		if(spawnFunctor_ != nullptr)
			spawnFunctor_(pool_.back());
		c = &pool_.back();
		c->status_ = Connection::Status::ACTIVE;
	}

	lg.unlock();

	PQsendQuery(c->handle_, query);
	/*boost::asio::async_read(c->socket_, boost::asio::null_buffers(), std::bind(&Connection::async_query_cb, this, std::placeholders::_1,
                                                    std::placeholders::_2, std::move(cb)));*/
	c->socket_.async_read_some(boost::asio::null_buffers(), std::bind(&Connection::async_query_cb, c, std::placeholders::_1,
			std::placeholders::_2, std::move(cb)));
	//c->socket_.async_read_some(boost::asio::null_buffers(), [](boost::system::error_code const& ec, size_t bt){});
	return true;
}

void Connection::async_query_cb(boost::system::error_code const& ec, size_t const& bt, Callback cb)
{
	PGresult* res = nullptr;
	if(!ec)
	{
		auto ret = PQconsumeInput(handle_);
		if(!ret)
			std::cerr << PQerrorMessage(handle_) << std::endl;
		//TODO: buraya PQisBusy ile tekrar kendine asnyc_read_some çağırılmalı mı?
		res = PQgetResult(handle_);
		//std::cout << res << std::endl;
		while(PQgetResult(handle_) != nullptr);
		//	std::cout << res << std::endl;
		status_ = Status::DEACTIVE;
		cb(ec, {{res}});
	}
	else
	{
		status_ = Status::DEACTIVE;
		cb(boost::system::error_code(1, boost::system::system_category()), {{res}});
	}
}

}
