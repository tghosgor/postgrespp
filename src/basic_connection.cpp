#include <basic_connection.hpp>
#include <work.hpp>

#include <boost/asio/executor_work_guard.hpp>

#include <memory>
#include <thread>

namespace postgrespp {

namespace {

boost::asio::io_context ioc;
std::shared_ptr<std::thread> ioc_thread{
  new std::thread{[] { const auto w = boost::asio::make_work_guard(ioc); ioc.run(); }},
  [](std::thread* thread) { ioc.stop(); thread->join(); delete thread; }
};

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
