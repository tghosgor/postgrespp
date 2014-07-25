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

using namespace postgrespp;

int main(int argc, char* argv[]) {
  auto connection = Connection::create("host=127.0.0.1");

  auto onQueryExecuted = [connection](const boost::system::error_code& ec, Result result) {
    if (!ec) {
      while (result.next()) {
        char* str;

        str = result.get<char*>(0);
        std::cout << "STR0: " << str << std::endl;
        str = result.get<char*>(1);
        std::cout << "STR1: " << str << std::endl;
        str = result.get<char*>(2);
        std::cout << "STR2: " << str << std::endl;
        str = result.get<char*>(3);
        std::cout << "STR3: " << str << std::endl;
      }
    } else {
      std::cout << ec << std::endl;
    }

    // we are done, stop the service
    Connection::ioService().service().stop();
  };

  connection->queryParams("select * from test_table", std::move(onQueryExecuted),  Connection::ResultFormat::TEXT);

  Connection::ioService().thread().join();

  return 0;
}
