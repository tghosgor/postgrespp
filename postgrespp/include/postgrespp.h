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

#pragma once

#include <deque>
#include <functional>
#include <mutex>

#include <boost/asio.hpp>

#include <libpq-fe.h>

namespace postgrespp
{

class Connection;

class EscapedLiteral
{
	friend class Connection;

	enum class Status : bool
	{
		DEACTIVE = 0,
				ACTIVE = 1
	};

	char* data_;

	EscapedLiteral(char* const& data);
	EscapedLiteral(EscapedLiteral const&) = delete;
	EscapedLiteral& operator=(EscapedLiteral const&) = delete;

public:
	EscapedLiteral(EscapedLiteral&& o);
	~EscapedLiteral();

	char* const& c_str();

};

class Result
{
	friend class Connection;

public:
	enum class Format : int
	{
		TEXT = 0,
				BINARY = 1
	};

	PGresult* res_;
	int row_ = -1;
	int nrows_;

	bool moved_ = false;

private:
	Result(PGresult* const& res);

public:
	Result(Result&& o);

	~Result();

	int rows();

	bool next();

	char* get(int const& column);

	void reset();
};

class Pool;
class ConnectionInstance;

class Connection
{
	friend class Pool;
	friend class ConnectionInstance;

public:
	typedef std::function<void(boost::system::error_code const&, Result)> Callback;

	enum class Status : bool
	{
		DEACTIVE = 0,
				ACTIVE = 1
	};
	boost::asio::io_service& is_;
	boost::asio::ip::tcp::socket socket_;
	//boost::asio::local::stream_protocol::socket socket_;

	PGconn* handle_;
	Status status_ = Status::DEACTIVE;

public:
	Connection(boost::asio::io_service& is);

	Connection(boost::asio::io_service& is, const char* const& pgconninfo);

	~Connection();

	ConnStatusType connect(const char* const& pgconninfo);

	ConnStatusType status();

	EscapedLiteral escapeLiteral(const char* const& str, size_t const& len);

	EscapedLiteral escapeLiteral(const char* const& str);

	EscapedLiteral escapeLiteral(std::string const& str);

	PGresult* prepare(const char* const& statementName, const char* const& query, int const& nParams,
			const Oid* const& paramTypes);

private:
	Connection(Connection&&) = delete;
	Connection(Connection const&) = delete;
	Connection& operator=(Connection const&) = delete;

	void async_query_cb(boost::system::error_code const& ec, size_t const& bt, Callback cb);
};

class Pool
{
public:
	typedef std::function<void(Connection&)> SpawnFunctor;
	typedef std::function<void(boost::system::error_code const&, Result)> Callback;

private:
	boost::asio::io_service& is_;

	std::mutex poolLock_;
	std::string pgconninfo_;

	std::deque<Connection> pool_;

	SpawnFunctor spawnFunctor_ = nullptr;

public:
	Pool(boost::asio::io_service& is, const char* const& pgconninfo, size_t n, SpawnFunctor sf);

	Pool(boost::asio::io_service& is, const char* const& pgconninfo, size_t n);

	bool query(const char* const& query, Callback cb);
};
}
