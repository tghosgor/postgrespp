#pragma once

#include "basic_transaction.hpp"
#include "query.hpp"
#include "socket_operations.hpp"
#include "utility.hpp"

#include <libpq-fe.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <stdexcept>
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

  template <class ExecutorT>
  basic_connection(ExecutorT& exc, const char* const& pgconninfo)
    : socket_{exc} {
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

  template <class CompletionTokenT>
  auto async_prepare(
      const statement_name_t& statement_name,
      const query_t& query,
      CompletionTokenT&& handler) {
    const auto res = PQsendPrepare(connection().underlying_handle(),
        statement_name.c_str(),
        query.c_str(),
        0,
        nullptr);

    if (res != 1) {
      throw std::runtime_error{
        "error preparing statement '" + statement_name + "': " + std::string{connection().last_error_message()}};
    }

    return handle_exec(std::forward<CompletionTokenT>(handler));
  }

  /**
   * Creates a read/write transaction. Make sure the created transaction
   * object lives until you are done with it.
   */
  template <
    class Unused_RWT = void,
    class Unused_IsolationT = void,
    class TransactionHandlerT>
  auto async_transaction(TransactionHandlerT&& handler) {
    using txn_t = basic_transaction<Unused_RWT, Unused_IsolationT>;

    auto initiation = [this](auto&& handler) {
      auto w = std::make_shared<txn_t>(*this);
      w->async_exec("BEGIN",
          [handler = std::move(handler), w](auto&& res) mutable { handler(std::move(*w)); } );
    };

    return boost::asio::async_initiate<
      TransactionHandlerT, void(txn_t)>(
          initiation, handler);
  }

  PGconn* underlying_handle() { return c_; }

  const PGconn* underlying_handle() const { return c_; }

  socket_t& socket() { return socket_; }

  const char* last_error_message() const { return PQerrorMessage(underlying_handle()); }

private:
  int status() const;

  basic_connection& connection() { return *this; }

  io_context_t& standalone_ioc();

private:
  socket_t socket_;

  PGconn* c_;
};

}
