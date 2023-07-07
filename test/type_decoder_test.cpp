#include "example_data_fixture.hpp"

#include <connection.hpp>
#include <work.hpp>

#include <pqxx/pqxx>

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>

using namespace postgrespp;

using ioc_t = connection::io_context_t;

class TypeDecoderTest : public example_data_fixture {
protected:
  void SetUp() override {
    example_data_fixture::SetUp();

    pqxx::connection c{CONN_STRING};
    pqxx::work txn{c};

    txn.exec("INSERT INTO " TEST_TABLE " (si, i, bi, t) VALUES (NULL, NULL, NULL, '20-chars-long-string')");

    txn.commit();
  }

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

TEST_F(TypeDecoderTest, text_to_const_char_ptr) {
  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "SELECT * FROM " TEST_TABLE,
            wrap_handler([&, shared_txn](auto result) {
              ASSERT_STREQ("row 0", result.at(0).at(4).template as<const char*>());
              ASSERT_STREQ("20-chars-long-string", result.at(3).at(4).template as<const char*>());
              ASSERT_STREQ("", result.at(2).at(3).template as<const char*>());
            }));
      });

  run();

  ASSERT_EQ(1, num_calls_);
}

TEST_F(TypeDecoderTest, text_to_std_string) {
  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "SELECT * FROM " TEST_TABLE,
            wrap_handler([&, shared_txn](auto result) {
              ASSERT_EQ("row 0", result.at(0).at(4).template as<std::string>());
              ASSERT_EQ("20-chars-long-string", result.at(3).at(4).template as<std::string>());
              ASSERT_EQ("", result.at(2).at(3).template as<std::string>());
            }));
      });

  run();

  ASSERT_EQ(1, num_calls_);
}

TEST_F(TypeDecoderTest, std_optional_integral) {
  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "SELECT * FROM " TEST_TABLE,
            wrap_handler([&, shared_txn](auto result) {
              ASSERT_EQ(10, result.at(0).at(1).template as<std::optional<int16_t>>().value());
              ASSERT_FALSE(result.at(2).at(1).template as<std::optional<int16_t>>().has_value());
            }));
      });

  run();

  ASSERT_EQ(1, num_calls_);
}

TEST_F(TypeDecoderTest, std_optional_floating_point) {
  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "INSERT INTO " TEST_TABLE " (r, d) VALUES ($1, $2)",
            [&, shared_txn](auto&& result) {
              shared_txn->async_exec(
                  "SELECT * FROM " TEST_TABLE,
                  wrap_handler([&, shared_txn](auto result) {
                    ASSERT_FLOAT_EQ(1.5, result.at(0).at(5).template as<std::optional<float>>().value());
                    ASSERT_FLOAT_EQ(3.5, result.at(0).at(6).template as<std::optional<double>>().value());
                    ASSERT_FALSE(result.at(2).at(5).template as<std::optional<float>>().has_value());

                    const auto& last_row = result.size() - 1;
                    ASSERT_FLOAT_EQ(12.5, result.at(last_row).at(5).template as<std::optional<float>>().value());
                    ASSERT_FLOAT_EQ(32.5, result.at(last_row).at(6).template as<std::optional<double>>().value());
                  }));
            },
            12.5f,
            32.5);
      });

  run();

  ASSERT_EQ(1, num_calls_);
}
