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

#include "include/connection.hpp"

#include "include/utility.hpp"

namespace postgrespp
{

Connection::Connection(boost::asio::io_service& ioService, const char* const& pgconninfo)
  : m_socket(ioService)
{
  m_handle = PQconnectdb(pgconninfo);
  bool success = !PQsetnonblocking(m_handle, 1) && (status() == CONNECTION_OK);
  if(success)
    m_socket.assign(boost::asio::ip::tcp::v4(), PQsocket(m_handle));
  else {
#ifndef NDEBUG
    std::cerr << "postgres++: connection error: " << PQerrorMessage(m_handle) << std::endl;
#endif
    m_handle = nullptr;
  }
}

std::shared_ptr<Connection> Connection::create(const char* const& pgConnInfo) {
  return std::shared_ptr<Connection>(new Connection(Connection::ioService().service(), pgConnInfo));
}

std::shared_ptr<Connection> Connection::create(io_service& ioService, const char* const& pgConnInfo) {
  return std::shared_ptr<Connection>(new Connection(ioService, pgConnInfo));
}

utility::StandaloneIoService& Connection::ioService() {
  static utility::StandaloneIoService ioService;
  return ioService;
}

Connection::~Connection() {
#ifndef NDEBUG
  std::cerr << "postgres++: connection ended.\n";
#endif
  PQfinish(m_handle);
}

Connection::Status Connection::status() const {
  return PQstatus(m_handle);
}

bool Connection::queryParams(const char* const& query, int const& n_params, ResultFormat const& result_format,
                       Callback callback, const char* const* const& param_values, const int* const& param_lengths,
                       const int* const& param_formats, const Oid* const& param_types) {
  PQsendQueryParams(m_handle, query, n_params, param_types, param_values, param_lengths, param_formats, static_cast<int>(result_format));
  m_socket.async_read_some(boost::asio::null_buffers(), std::bind(&Connection::asyncQueryCb, this, std::placeholders::_1,
                                                                    std::placeholders::_2, std::move(callback)));
  return true;
}

PGresult* Connection::prepare(const char* const& stmt_name, const char* const& query, int const& n_params,
                              const Oid* const& param_types) {
  return PQprepare(m_handle, stmt_name, query, n_params, param_types);
}

}// NS postgrespp
