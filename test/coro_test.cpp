#include "example_data_fixture.hpp"

#include <async_exec.hpp>
#include <async_exec_prepared.hpp>
#include <connection.hpp>
#include <work.hpp>

#include <gtest/gtest.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/streambuf.hpp>

using namespace postgrespp;

using ioc_t = connection::io_context_t;

using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;

namespace this_coro = boost::asio::this_coro;

awaitable<void> run() {
  auto exc = co_await this_coro::executor;

  connection c{exc, CONN_STRING};

  auto txn = co_await c.async_transaction(use_awaitable);
  const auto res = co_await txn.async_exec("SELECT 1", use_awaitable);
  const auto res_param = co_await txn.async_exec("SELECT $1", use_awaitable, 1);
  const auto res_prep = co_await txn.async_exec_prepared("select_1", use_awaitable);
  const auto res_prep_param = co_await txn.async_exec_prepared("select_arg", use_awaitable, 1);
  const auto res_commit = co_await txn.commit(use_awaitable);
  const auto res_rollback = co_await txn.rollback(use_awaitable);
  const auto async_exec_conn_res = co_await async_exec(c, "SELECT $1", use_awaitable, 2);
  const auto async_exec_txn_res = co_await async_exec(txn, "SELECT $1", use_awaitable, 2);
  const auto async_exec_prepared_conn_res = co_await async_exec_prepared(c, "select_1", use_awaitable);
  const auto async_exec_prepared_txn_res = co_await async_exec_prepared(txn, "select_arg", use_awaitable, 2);
}

int main(int, char**) {
  ioc_t ioc;

  co_spawn(ioc, run(), detached);

  ioc.run();
}
