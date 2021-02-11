#pragma once

#include "basic_transaction.hpp"
#include "query.hpp"
#include "socket_operations.hpp"
#include "utility.hpp"

#include <libpq-fe.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <string>
#include <utility>

namespace postgrespp {

template <class, class>
class basic_transaction;

class result;

class basic_connection : public socket_operations<basic_connection> {
  friend class socket_operations<basic_connection>;
public:
  using io_context_t = boost::asio::io_context;
  using result_t = result;
  using socket_t = boost::asio::ip::tcp::socket;
  using query_t = query;
  using statement_name_t = std::string;

public:
  basic_connection(const char* const& pgconninfo)
    : basic_connection{standalone_ioc(), pgconninfo} {
  }

  basic_connection(io_context_t& ioc, const char* const& pgconninfo);

  ~basic_connection();

  basic_connection(basic_connection const&) = delete;

  basic_connection(basic_connection&& rhs) noexcept
    : socket_{std::move(rhs.socket_)}
    , c_{std::move(rhs.c_)} {
    rhs.c_ = nullptr;
  }

  basic_connection& operator=(basic_connection const&) = delete;

  basic_connection& operator=(basic_connection&& rhs) noexcept {
    using std::swap;

    swap(socket_, rhs.socket_);
    swap(c_, rhs.c_);

    return *this;
  }

  template <class ResultCallableT>
  void async_prepare(
      const statement_name_t& statement_name,
      const query_t& query,
      ResultCallableT&& handler) {
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

  /**
   * Creates a read/write transaction. Make sure the created transaction
   * object lives until you are done with it.
   */
  template <
    class Unused_RWT = void,
    class Unused_IsolationT = void,
    class TransactionHandlerT>
  void async_transaction(TransactionHandlerT&& handler) {
    auto w = std::make_shared<basic_transaction<Unused_RWT, Unused_IsolationT>>(*this);
    w->async_exec("BEGIN",
        [handler = std::move(handler), w](auto&& res) mutable { handler(std::move(*w)); } );
  }

  PGconn* underlying_handle() { return c_; }

  const PGconn* underlying_handle() const { return c_; }

  socket_t& socket() { return socket_; }

  const char* last_error() const { return PQerrorMessage(underlying_handle()); }

private:
  int status() const;

  basic_connection& connection() { return *this; }

  io_context_t& standalone_ioc();

private:
  socket_t socket_;

  PGconn* c_;
};

}
