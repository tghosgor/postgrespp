#include <basic_connection.hpp>
#include <work.hpp>

#include <boost/asio/executor_work_guard.hpp>

#include <memory>
#include <stdexcept>
#include <thread>

namespace postgrespp {

namespace {

basic_connection::io_context_t ioc;
std::shared_ptr<std::thread> ioc_thread{
  new std::thread{[] { const auto w = boost::asio::make_work_guard(ioc); ioc.run(); }},
  [](std::thread* thread) { ioc.stop(); thread->join(); delete thread; }
};

}

basic_connection::basic_connection(io_context_t& ioc, const char* const& pgconninfo)
  : socket_{ioc} {
  c_ = PQconnectdb(pgconninfo);

  if (status() != CONNECTION_OK)
    throw std::runtime_error{"could not connect: " + std::string{PQerrorMessage(c_)}};

  if (PQsetnonblocking(c_, 1) != 0)
    throw std::runtime_error{"could not set non-blocking: " + std::string{PQerrorMessage(c_)}};

  const auto socket = PQsocket(c_);

  if (socket < 0)
    throw std::runtime_error{"could not get a valid descriptor"};

  socket_.assign(boost::asio::ip::tcp::v4(), socket);
}

basic_connection::~basic_connection() {
  if (c_)
    PQfinish(c_);
}

int basic_connection::status() const {
  return PQstatus(c_);
}

auto basic_connection::standalone_ioc() -> io_context_t& {
  return ioc;
}

}
