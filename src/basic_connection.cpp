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

  socket_.assign(boost::asio::ip::tcp::v4(), PQsocket(c_));
}

basic_connection::~basic_connection() {
  PQfinish(c_);
}

void basic_connection::async_prepare(
    const statement_name_t& statement_name,
    const query_t& query,
    prepare_handler_t handler) {
  const auto res = PQsendPrepare(connection().underlying_handle(),
      statement_name.c_str(),
      query.c_str(),
      0,
      nullptr);

  if (res != 1) {
    throw std::runtime_error{"error preparing statement '" + statement_name + "': " + std::string{connection().last_error()}};
  }

  handle_exec(std::move(handler));
}

int basic_connection::status() const {
  return PQstatus(c_);
}

auto basic_connection::standalone_ioc() -> io_context_t& {
  return ioc;
}

}
