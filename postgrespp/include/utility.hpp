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

#include <thread>

namespace postgrespp {
namespace utility {

class StandaloneIoService {
public:
  StandaloneIoService()
    : m_emptyWork(m_ioService)
    , m_ioServiceThread([this](){ m_ioService.run(); })
  { }

  ~StandaloneIoService() {
    m_ioService.stop();
    if (m_ioServiceThread.joinable())
      m_ioServiceThread.join();
  }

  boost::asio::io_service& service() { return m_ioService; }
  std::thread& thread() { return m_ioServiceThread; }

private:
  boost::asio::io_service m_ioService;
  boost::asio::io_service::work m_emptyWork;
  std::thread m_ioServiceThread;
};

struct Account {
  std::string m_name;
  std::string m_password;
};

}// NS utility
}// NS postgrespp
