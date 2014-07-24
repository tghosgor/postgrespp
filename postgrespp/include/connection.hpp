/*
  Copyright (c) 2014, Tolga HOŞGÖR
  All rights reserved.

  This file is part of postgrespp.

  postgrespp is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  postgrespp is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with postgrespp.  If not, see <http://www.gnu.org/licenses/>
*/

#pragma once

#include <boost/asio.hpp>
#include <libpq-fe.h>

#include <functional>

namespace postgrespp {

class Result;
namespace utility {
class StandaloneIoService;
}

class Connection {
  friend class Pool;

  using io_service = boost::asio::io_service;

public:
  using Callback = std::function<void(boost::system::error_code const&, Result)>;
  using Status = int;
  enum class ResultFormat : int {
    TEXT = 0,
    BINARY
  };

public:
  ~Connection();

  Connection(Connection const&) = delete;
  Connection& operator=(Connection const&) = delete;

  Connection(Connection&&) = delete;
  Connection& operator=(Connection&&) = delete;

  /**
   * @brief create
   * @return A connection that uses library's internal io service.
   */
  static std::shared_ptr<Connection> create(const char* const& pgConnInfo);
  /**
   * @brief create
   * @return A connection that uses the given io service.
   */
  static std::shared_ptr<Connection> create(io_service& ioService, const char* const& pgConnInfo);

  /**
   * @brief ioService
   * @return A static StandaloneIoService object.
   *
   * Creates and returns a static StandaloneIoService object. Connection::create(const char* const&) calls this method to create
   * its io_service.
   */
  static utility::StandaloneIoService& ioService();

  /**
   * @brief queryParams Takes parameters like PQexecParams, also makes use of C++11 variadic template.
   * @param query Parametered query to send.
   * @param resultFormat The result format we want from the postgres server.
   * @param callback Callback that will be called when the operation completes.
   * @param args Arguments to insert to parametered query.
   *
   * This function automatically creates the structure to call the queryParams overload below.
   */
  template<typename... Args>
  bool queryParams(const char* const& query, ResultFormat const& resultFormat, Callback callback, Args... args);

  /**
    @brief queryParams Takes parameters like PQexecParams.
   */
  bool queryParams(const char* const& query, int const& n_params, ResultFormat const& result_format, Callback callback,
                   const char* const* const& param_values, const int* const& param_lengths,
                   const int* const& param_formats, const Oid* const& param_types = nullptr);

  Status status() const;

private:
  Connection(boost::asio::io_service& ioService, const char* const& pgconninfo);

  PGresult* prepare(const char* const& stmt_name, const char* const& query, int const& n_params,
                    const Oid* const& param_types);

private:
  void asyncQueryCb(boost::system::error_code const& ec, size_t const& bt, Callback cb);

  template<typename... Args>
  void fillParamArray(char** valptr_array, int* len_array, int* format_array, int64_t const& value, Args... args);
  template<typename... Args>
  void fillParamArray(char** valptr_array, int* len_array, int* format_array, int32_t const& value, Args... args);
  template<typename... Args>
  void fillParamArray(char** valptr_array, int* len_array, int* format_array, int16_t const& value, Args... args);
  template<typename... Args>
  void fillParamArray(char** valptr_array, int* len_array, int* format_array, int8_t const& value, Args... args);
  template<typename... Args>
  void fillParamArray(char** valptr_array, int* len_array, int* format_array, const char* const& value, Args... args);
  void fillParamArray(char** valptr_array, int* len_array, int* format_array);

private:
  boost::asio::ip::tcp::socket m_socket;

  std::string m_pgConnInfo;

  PGconn* m_handle;
};

}// NS postgrespp

#include "connection.impl"
