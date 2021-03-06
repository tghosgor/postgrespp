#include "example_data_fixture.hpp"

#include <async_exec.hpp>
#include <use_future.hpp>

#include <gtest/gtest.h>

#include <functional>

class AsyncExec : public example_data_fixture {
protected:
  void run() {
    ioc_.run();
    c_.reset();
  }

  connection_t& connection() { return c_.value(); }

  template <class CallableT>
  auto wrap_handler(CallableT&& callable) {
    return [this, callable = std::move(callable)](auto&&... args) mutable {
      ++num_calls_;
      callable(std::forward<decltype(args)>(args)...);
    };
  }

protected:
  std::size_t num_calls_ = 0;
  ioc_t ioc_;
  std::optional<connection_t> c_ = std::make_optional<connection_t>(ioc_, CONN_STRING);
};

using namespace postgrespp;

TEST_F(AsyncExec, select) {
  async_exec(connection(), "SELECT * FROM " TEST_TABLE, wrap_handler([](auto&& result) {
        ASSERT_EQ(result::status_t::TUPLES_OK, result.status());
        ASSERT_EQ(3, result.size());
      }));

  run();

  ASSERT_EQ(1, num_calls_);
}

TEST_F(AsyncExec, select_param) {
  async_exec(connection(), "SELECT * FROM " TEST_TABLE " WHERE id = $1", wrap_handler([](auto&& result) {
        ASSERT_EQ(result::status_t::TUPLES_OK, result.status());
        ASSERT_EQ(1, result.size());
      }),
      1);

  run();

  ASSERT_EQ(1, num_calls_);
}

TEST_F(AsyncExec, select_param_future) {
  auto result_future = async_exec(connection(),
      "SELECT * FROM " TEST_TABLE " WHERE id = $1",
      use_future,
      1);

  run();

  const auto result = result_future.get();

  ASSERT_EQ(result::status_t::TUPLES_OK, result.status());
  ASSERT_EQ(1, result.size());
}

TEST_F(AsyncExec, insert_param) {
  async_exec(connection(), "INSERT INTO " TEST_TABLE " (si, i, bi) VALUES ($1, $2, $3)", wrap_handler([](auto&& result) {
        ASSERT_EQ(result::status_t::COMMAND_OK, result.status()) << result.error_message();
      }),
      static_cast<std::int16_t>(1), 2, static_cast<std::int64_t>(3));

  run();

  ASSERT_EQ(1, num_calls_);
}

TEST_F(AsyncExec, transaction_select) {
  connection().async_transaction<>([this](auto txn) {
        auto s_txn = std::make_shared<work>(std::move(txn));

        async_exec(*s_txn, "SELECT * FROM " TEST_TABLE, wrap_handler([s_txn](auto&& result) mutable {
              ASSERT_EQ(result::status_t::TUPLES_OK, result.status());
              ASSERT_EQ(3, result.size());
            }));
      });

  run();

  ASSERT_EQ(1, num_calls_);
}

TEST_F(AsyncExec, transaction_select_param) {
  connection().async_transaction<>([this](auto txn) {
        auto s_txn = std::make_shared<work>(std::move(txn));

        async_exec(*s_txn,
            "SELECT * FROM " TEST_TABLE " WHERE id = $1",
            wrap_handler([s_txn](auto&& result) mutable {
              ASSERT_EQ(result::status_t::TUPLES_OK, result.status());
              ASSERT_EQ(1, result.size());
            }),
            1);
      });

  run();

  ASSERT_EQ(1, num_calls_);
}
