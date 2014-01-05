/*
Copyright (c) 2013, 2014, Tolga HOŞGÖR
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

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>

#include <boost/asio.hpp>
#include <endian.h>
#include <libpq-fe.h>

namespace postgrespp
{

class Connection;
class EscapedLiteral
{
	friend class Connection;

	char* data_;

	EscapedLiteral(char* const& data);
	EscapedLiteral(EscapedLiteral const&) = delete;
	EscapedLiteral& operator=(EscapedLiteral const&) = delete;

public:
	EscapedLiteral(EscapedLiteral&& o);
	~EscapedLiteral();

	char* const& c_str() { return data_; }
};

class Result
{
	friend class Pool;
public:
	typedef char* RawPtr;
	/*struct RawPtr
	{
	public:
		RawPtr(char* const& ptr) : ptr_(ptr) { };
		
		operator char*() const
		{
			return ptr_;
		}
	
	private:
		char* ptr_;
	};*/

public:
	Result(Result&& o);
	~Result();

	/*
	 * Returns: the number of rows in the result set.
	 */
	int rows()
	{
		return nrows_;
	}
	
	/*
	 * Increments the internal row counter to the next row.
	 * 
	 * Returns: true if it the incremented row is a valid row, false if not and we are past the end row.
	 */
	bool next()
	{
		return (++row_ < rows());
	}

	/*
	 * Returns: the value in 'coulmn'th coulmn in the current row.
	 */
	template<typename T>
	T get(int const& column);
	
	/*
	 * Returns result status.
	 */
	 ExecStatusType getStatus()
	 {
		 return PQresultStatus(res_);
	 }
	
	/*
	 * Returns result status as text.
	 */
	 const char* getStatusText()
	 {
		 return PQresStatus(PQresultStatus(res_));
	 }
	 
	/*
	 * Returns error message associated with the result.
	 */
	 const char* getErrorMessage()
	 {
		 return PQresultErrorMessage(res_);
	 }
	 
	/*
	 * Resets the internal row counter to the first row.
	 */
	void reset()
	{
		row_ = -1;
	}

private:
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
};

class Pool;
class Connection
{
	friend class Pool;

public:
	typedef std::function<void(boost::system::error_code const&, Result)> Callback;

	enum class Status : bool
	{
		DEACTIVE = 0,
		ACTIVE = 1
	};

private:
	boost::asio::io_service& is_;
	boost::asio::ip::tcp::socket socket_;
	//boost::asio::local::stream_protocol::socket socket_;

	PGconn* handle_;
	std::atomic<Status> status_;
	
public:
	/*
	 * You must not manually instantiate this class
	 */
	Connection(boost::asio::io_service& is);
	Connection(boost::asio::io_service& is, const char* const& pgconninfo);
	
	~Connection();

	ConnStatusType status();

private:	
	Connection(Connection&&) = delete;
	Connection(Connection const&) = delete;
	Connection& operator=(Connection const&) = delete;
	
	ConnStatusType connect(const char* const& pgconninfo);

	EscapedLiteral escapeLiteral(const char* const& str, size_t const& len);
	EscapedLiteral escapeLiteral(const char* const& str);
	EscapedLiteral escapeLiteral(std::string const& str);

	PGresult* prepare(const char* const& statementName, const char* const& query, int const& nParams,
			const Oid* const& paramTypes);
};

class Pool
{
public:
	typedef std::function<void(Connection&)> SpawnFunction;
	typedef std::function<void(boost::system::error_code const&, Result)> Callback;
	
	enum class ResultFormat : int
	{
		TEXT = 0,
		BINARY
	};
	
	struct Settings
	{
		size_t connUnusedTimeout;
		size_t minConnCount;
		size_t maxConnCount;
		SpawnFunction spawnFunction;
	} settings_;

public:
	Pool(boost::asio::io_service& is, const char* const& pgconninfo, Settings const& settings = {20, 1, 75, nullptr});

	/*
	 * Tries to create 'n' connections.
	 * Using this function, you can exceed the maximum number of connections defined in the pool settings.
	 * 
	 * Returns: Number of the connections that could not be created.
	 */
	size_t createConn(size_t n);
	
	/*
	 * Picks and available connection from the pool and sends an asynchronous query with it
	 * and calls the callback with the results
	 * 
	 * Returns: true if query is sent successfully, false if not.
	 */
	bool query(const char* const& query, Callback cb);
	
	/*
	 * Same as Pool::query but takes parameters like PQexecParams.
	 */
	bool queryParams(const char* const& query, int const& n_params, ResultFormat const& result_format, Callback cb, const char* const* const& param_values, const int* const& param_lengths, const int* const& param_formats, const Oid* const& param_types = nullptr);
  
  /*
   * Same as Pool::query but takes parameters like PQexecParams, also makes use of C++11 variadic template.
   */
  template<typename... Args>
  bool queryParams(const char* const& query, ResultFormat const& result_format, Callback cb, Args... args)
  {
    static char* valptr_array[sizeof...(Args)];
    static int len_array[sizeof...(Args)];
    static int format_array[sizeof...(Args)];
    
    fill_param_arrays(valptr_array, len_array, format_array, args...);
    
    //for(size_t i = 0; i < sizeof...(Args); ++i)
    //{
		//	std::cout << len_array[i] << " - " << format_array[i] << std::endl;
		//}
    
    auto ret = queryParams(query, sizeof...(Args), result_format, std::move(cb), valptr_array, len_array, format_array);
    
    for(size_t i = 0; i < sizeof...(Args); ++i)
			if(format_array[i] == 1)
				delete valptr_array[i];
      
    return ret;
  }

private:
	boost::asio::io_service& is_;

	std::mutex poolLock_;
	std::string pgconninfo_;

	std::deque<Connection> pool_;
	
	boost::asio::deadline_timer dtimer_;
	
private:
	Connection* getFreeConnection();
	void asyncQueryCb(boost::system::error_code const& ec, size_t const& bt, Connection& conn, Callback cb);
      
  void fill_param_arrays(char** valptr_array, int* len_array, int* format_array) { }

  template<typename... Args>
  void fill_param_arrays(char** valptr_array, int* len_array, int* format_array, int64_t const& value, Args... args)
  {
    *valptr_array = new char[sizeof(value)];
    **reinterpret_cast<int64_t**>(valptr_array) = htobe64(value);
    *len_array = sizeof(int64_t);
    *format_array = 1;
    fill_param_arrays(++valptr_array, ++len_array, ++format_array, args...);
  }

  template<typename... Args>
  void fill_param_arrays(char** valptr_array, int* len_array, int* format_array, int32_t const& value, Args... args)
  {
    *valptr_array = new char[sizeof(value)];
    **reinterpret_cast<int32_t**>(valptr_array) = htobe32(value);
    *len_array = sizeof(int32_t);
    *format_array = 1;
    fill_param_arrays(++valptr_array, ++len_array, ++format_array, args...);
  }

  template<typename... Args>
  void fill_param_arrays(char** valptr_array, int* len_array, int* format_array, int16_t const& value, Args... args)
  {
    *valptr_array = new char[sizeof(value)];
    **reinterpret_cast<int16_t**>(valptr_array) = htobe16(value);
    *len_array = sizeof(int16_t);
    *format_array = 1;
    fill_param_arrays(++valptr_array, ++len_array, ++format_array, args...);
  }

  template<typename... Args>
  void fill_param_arrays(char** valptr_array, int* len_array, int* format_array, int8_t const& value, Args... args)
  {
    *valptr_array = new char[sizeof(value)];
    **reinterpret_cast<int8_t**>(valptr_array) = value;
    *len_array = sizeof(int8_t);
    *format_array = 1;
    fill_param_arrays(++valptr_array, ++len_array, ++format_array, args...);
  }

  template<typename... Args>
  void fill_param_arrays(char** valptr_array, int* len_array, int* format_array, const char* const& value, Args... args)
  {
    *valptr_array = new char[sizeof(value)];
    *const_cast<const char **>(reinterpret_cast<char**>(valptr_array)) = value;
    *len_array = 0;
    *format_array = 0;
    fill_param_arrays(++valptr_array, ++len_array, ++format_array, args...);
  }
};

template<> inline
int8_t Result::get<int8_t>(int const& column)
{
	return *reinterpret_cast<int8_t*>(PQgetvalue(res_, row_, column));
}

template<> inline
int16_t Result::get<int16_t>(int const& column)
{
	return be16toh(*reinterpret_cast<int16_t*>(PQgetvalue(res_, row_, column)));
}
	
template<> inline
int32_t Result::get<int32_t>(int const& column)
{
	return be32toh(*reinterpret_cast<int32_t*>(PQgetvalue(res_, row_, column)));
}

template<> inline
int64_t Result::get<int64_t>(int const& column)
{
	return be64toh(*reinterpret_cast<int64_t*>(PQgetvalue(res_, row_, column)));
}

template<> inline
Result::RawPtr Result::get<Result::RawPtr>(int const& column)
{
	return PQgetvalue(res_, row_, column);
}

}
