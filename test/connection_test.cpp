#include <connection.hpp>
#include <work.hpp>

#include <pqxx/pqxx>

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>

#define TEST_TABLE "tbl_test"
#define TEST_TABLE_INITIAL_ROWS "3"

#define CONN_STRING "host=127.0.0.1 user=postgres"

using namespace postgrespp;

using ioc_t = connection::io_context_t;

TEST(StandaloneConnectionTest, standalone_ioc_construct) {
  connection c{CONN_STRING};
}

TEST(ExternalIocConnectionTest, external_ioc_construct) {
  ioc_t ioc;

  connection c{ioc, CONN_STRING};

  ioc.run();
}

class ConnectionTest : public ::testing::Test {
protected:
  using connection_t = ::postgrespp::connection;

protected:
  void SetUp() override {
    pqxx::connection c{CONN_STRING};
    pqxx::work txn{c};

    txn.exec("DROP TABLE IF EXISTS " TEST_TABLE);
    txn.exec("CREATE TABLE " TEST_TABLE " (id serial NOT NULL, si smallint, i int, bi bigint, t text)");
    txn.exec("INSERT INTO " TEST_TABLE " (si, i, bi, t) VALUES (10, 20, 40, 'row 0')");
    txn.exec("INSERT INTO " TEST_TABLE " (si, i, bi, t) VALUES (11, 22, 44, 'row 1')");
    txn.exec("INSERT INTO " TEST_TABLE " (si, i, bi, t) VALUES (NULL, NULL, NULL, NULL)");

    txn.commit();
  }

  void TearDown() override {
    pqxx::connection c{CONN_STRING};
    pqxx::work txn{c};

    txn.exec("DROP TABLE " TEST_TABLE);

    txn.commit();
  }

  void run() {
    ioc_.run();
    c_.reset();
  }

  connection_t& connection() { return c_.value(); }

protected:
  ioc_t ioc_;
  std::optional<connection_t> c_ = std::make_optional<connection_t>(ioc_, CONN_STRING);
};

TEST_F(ConnectionTest, async_exec_select) {
  int called = 0;

  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "SELECT * FROM " TEST_TABLE,
            [&, shared_txn](auto result) {
              ++called;

              ASSERT_FALSE(result.done());
              ASSERT_EQ(result::status_t::TUPLES_OK, result.status()) << result.error_message();
              ASSERT_EQ(3, result.size());
            });
      });

  run();

  ASSERT_EQ(1, called) << connection().last_error();
}

TEST_F(ConnectionTest, async_exec_select_multi) {
  std::size_t num_calls = 0;
  bool empty_res_seen = false;

  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec_all(
            "SELECT * FROM " TEST_TABLE "; SELECT * FROM " TEST_TABLE,
            [&, shared_txn](auto result) {
              if (!result.done()) {
                ++num_calls;
                ASSERT_EQ(result::status_t::TUPLES_OK, result.status()) << result.error_message();
                ASSERT_EQ(3, result.size());
              } else {
                empty_res_seen = true;
              }
            });
      });

  run();

  ASSERT_EQ(2, num_calls);
  ASSERT_EQ(true, empty_res_seen);
}

TEST_F(ConnectionTest, async_exec_select_param) {
  int called = 0;

  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "SELECT * FROM " TEST_TABLE " WHERE id = $1",
            [&, shared_txn](const auto& result) {
              ++called;
              ASSERT_FALSE(result.done());
              ASSERT_EQ(result::status_t::TUPLES_OK, result.status()) << result.error_message();
              ASSERT_EQ(1, result.size());
            },
            1);
      });

  run();

  ASSERT_EQ(1, called) << connection().last_error();
}

TEST_F(ConnectionTest, async_exec_select_fields) {
  int called = 0;

  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "SELECT * FROM " TEST_TABLE,
            [&, shared_txn](auto result) {
              ++called;

              ASSERT_FALSE(result.done());
              ASSERT_EQ(result::status_t::TUPLES_OK, result.status()) << result.error_message();

              for (std::size_t row = 0; row < 2; ++row) {
                const auto& r = result.at(row);
                ASSERT_EQ(row + 1, r.at(0).template as<std::int32_t>());

                ASSERT_EQ((10 + row), r.at(1).template as<std::int16_t>());
                ASSERT_EQ((10 + row) * 2, r.at(2).template as<std::int32_t>());
                ASSERT_EQ((10 + row) * 4, r.at(3).template as<std::int64_t>());
              }

              EXPECT_THROW(result.at(0).at(1).template as<std::int32_t>(), std::length_error);

              ASSERT_TRUE(result.at(2).at(1).is_null());
              ASSERT_EQ(16, result.at(2).at(1).template as<std::int16_t>(16));

              EXPECT_THROW(result.at(2).at(1).template as<std::int16_t>(), std::length_error);
            });
      });

  run();

  ASSERT_EQ(1, called) << connection().last_error();
}

TEST_F(ConnectionTest, async_exec_select_param_fields) {
  int called = 0;

  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "SELECT * FROM " TEST_TABLE " WHERE id = 1",
            [&, shared_txn](auto result) {
              ++called;

              ASSERT_FALSE(result.done());
              ASSERT_EQ(result::status_t::TUPLES_OK, result.status()) << result.error_message();

              ASSERT_EQ(1, result.size());

              const auto& r = result.at(0);
              ASSERT_EQ(1, r.at(0).template as<std::int32_t>());
              ASSERT_EQ(10, r.at(1).template as<std::int16_t>());
              ASSERT_EQ(20, r.at(2).template as<std::int32_t>());
              ASSERT_EQ(40, r.at(3).template as<std::int64_t>());
            });
      });

  run();

  ASSERT_EQ(1, called) << connection().last_error();
}

TEST_F(ConnectionTest, async_exec_prepared_select) {
  int called = 0;

  connection().async_prepare("stmt", "SELECT * FROM " TEST_TABLE,
      [&](auto&& result) {
        ASSERT_TRUE(result.ok());

        connection().async_transaction<>([&](auto txn) {
          auto shared_txn = std::make_shared<work>(std::move(txn));

          shared_txn->async_exec_prepared(
              "stmt",
              [&, shared_txn](auto result) {
                ++called;

                ASSERT_FALSE(result.done());
                ASSERT_EQ(result::status_t::TUPLES_OK, result.status()) << result.error_message();

                for (std::size_t row = 0; row < 2; ++row) {
                  const auto& r = result.at(row);
                  ASSERT_EQ(row + 1, r.at(0).template as<std::int32_t>());

                  ASSERT_EQ((10 + row), r.at(1).template as<std::int16_t>());
                  ASSERT_EQ((10 + row) * 2, r.at(2).template as<std::int32_t>());
                  ASSERT_EQ((10 + row) * 4, r.at(3).template as<std::int64_t>());
                }

                EXPECT_THROW(result.at(0).at(1).template as<std::int32_t>(), std::length_error);

                ASSERT_TRUE(result.at(2).at(1).is_null());
                ASSERT_EQ(16, result.at(2).at(1).template as<std::int16_t>(16));

                EXPECT_THROW(result.at(2).at(1).template as<std::int16_t>(), std::length_error);
              });
          });
      });

  run();

  ASSERT_EQ(1, called) << connection().last_error();
}

TEST_F(ConnectionTest, async_exec_insert_100000) {
  std::size_t insertions = 0;

  std::shared_ptr<work> shared_txn;

  std::function<void()> insert;

  insert = [&]() {
    shared_txn->async_exec(
          "INSERT INTO " TEST_TABLE " (si, i, bi, t) VALUES ($1, $2, $3, $4)",
          [&](auto result) {
            ++insertions;

            ASSERT_EQ(result::status_t::COMMAND_OK, result.status()) << result.error_message();

            if (insertions < 100000) {
              insert();
            } else {
              shared_txn->commit([shared_txn]() { ; });
            }
          },
          static_cast<std::int16_t>(1), 2, static_cast<std::int64_t>(insertions), "my row text"
        );
      };

  connection().async_transaction<>([&](auto txn) {
        shared_txn = std::make_shared<work>(std::move(txn));

        insert();
      });

  run();

  pqxx::connection c{CONN_STRING};
  pqxx::work txn{c};
  const auto result = txn.exec("SELECT * FROM " TEST_TABLE " WHERE id > " TEST_TABLE_INITIAL_ROWS);

  ASSERT_EQ(100000, result.size());

  for (std::size_t i = 0; i < result.size(); ++i) {
    ASSERT_FALSE(result[i][3].is_null());
    ASSERT_EQ(i, result[i][3].as<std::int64_t>());
  }
  txn.commit();
}

TEST_F(ConnectionTest, async_exec_prepared_insert_10000) {
  std::size_t insertions = 0;

  std::shared_ptr<work> shared_txn;

  std::function<void()> insert;

  connection().async_prepare(
      "stmt",
      "INSERT INTO " TEST_TABLE " (si, i, bi, t) VALUES ($1, $2, $3, $4)",
      [&](auto&& result) {
          ASSERT_FALSE(result.done());
          ASSERT_TRUE(result.ok());

          insert = [&]() {
            shared_txn->async_exec_prepared(
                  "stmt",
                  [&](auto result) {
                    ++insertions;

                    ASSERT_EQ(result::status_t::COMMAND_OK, result.status()) << result.error_message();

                    if (insertions < 10000) {
                      insert();
                    } else {
                      shared_txn->commit([shared_txn]() { ; });
                    }
                  },
                  static_cast<std::int16_t>(1), 2, static_cast<std::int64_t>(insertions), "my row text"
                );
              };

          connection().async_transaction<>([&](auto txn) {
                shared_txn = std::make_shared<work>(std::move(txn));

                insert();
              });
        });

  run();

  pqxx::connection c{CONN_STRING};
  pqxx::work txn{c};
  const auto result = txn.exec("SELECT * FROM " TEST_TABLE " WHERE id > " TEST_TABLE_INITIAL_ROWS);

  ASSERT_EQ(10000, result.size());

  for (std::size_t i = 0; i < result.size(); ++i) {
    ASSERT_FALSE(result[i][3].is_null());
    ASSERT_EQ(i, result[i][3].as<std::int64_t>());
  }
  txn.commit();
}

TEST_F(ConnectionTest, transaction_dtor_with_no_action) {
  std::size_t insertions = 0;

  std::shared_ptr<work> shared_txn;

  std::function<void()> insert;

  insert = [&]() {
    shared_txn->async_exec(
          "INSERT INTO " TEST_TABLE " (si, i, bi, t) VALUES ($1, $2, $3, $4)",
          [&](auto result) {
            ++insertions;

            ASSERT_EQ(result::status_t::COMMAND_OK, result.status()) << result.error_message();

            if (insertions < 100) {
              insert();
            } else {
              shared_txn = nullptr;
            }
          },
          static_cast<std::int16_t>(1), 2, static_cast<std::int64_t>(insertions), "my row text"
        );
      };

  connection().async_transaction<>([&](auto txn) {
        shared_txn = std::make_shared<work>(std::move(txn));

        insert();
      });

  run();

  pqxx::connection c{CONN_STRING};
  pqxx::work txn{c};
  const auto result = txn.exec("SELECT * FROM " TEST_TABLE " WHERE id > " TEST_TABLE_INITIAL_ROWS);

  ASSERT_EQ(0, result.size());

  txn.commit();
}

TEST_F(ConnectionTest, async_exec_insert_40M_sized_field) {
  std::string data;

  constexpr auto data_size = 40 * 1024 * 1024;

  data.reserve(data_size);
  for (std::size_t i = 0; i < data_size; ++i) {
    data.push_back('a' + (i % ('z' + 1 - 'a')));
  }

  connection().async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "INSERT INTO " TEST_TABLE " (t) VALUES ($1)",
            [&, shared_txn](auto result) {
              shared_txn->commit([shared_txn]{});
            },
            data);
      });

  run();

  pqxx::connection c{CONN_STRING};
  pqxx::work txn{c};
  const auto result = txn.exec("SELECT * FROM " TEST_TABLE " WHERE id > " TEST_TABLE_INITIAL_ROWS);

  ASSERT_EQ(1, result.size());
  ASSERT_LT(4, result[0].size());
  ASSERT_FALSE(result[0][4].is_null());
  ASSERT_EQ(data, result[0][4].as<const char*>());
  txn.commit();
}

class LargeDataTest : public ::testing::Test {
protected:
  void SetUp() override {
    pqxx::connection c{CONN_STRING};
    pqxx::work txn{c};

    txn.exec("DROP TABLE IF EXISTS " TEST_TABLE);
    txn.exec("CREATE TABLE " TEST_TABLE " (id serial NOT NULL, si smallint, i int, bi bigint, t text)");

    for (std::size_t i = 0; i < num_rows_; ++i) {
      txn.exec("INSERT INTO " TEST_TABLE " (si, i, bi, t) VALUES (10, 20, " + std::to_string(i) + ", 'row 0')");
    }

    txn.commit();
  }

  void TearDown() override {
    pqxx::connection c{CONN_STRING};
    pqxx::work txn{c};

    txn.exec("DROP TABLE " TEST_TABLE);

    txn.commit();
  }

protected:
  std::size_t num_rows_ = 100000;
};

TEST_F(LargeDataTest, select_100000_rows) {
  ioc_t ioc;

  std::size_t num_batches = 0;
  std::size_t rows_seen = 0;

  connection c{ioc, CONN_STRING};

  c.async_transaction<>([&](auto txn) {
        auto shared_txn = std::make_shared<work>(std::move(txn));

        shared_txn->async_exec(
            "SELECT * FROM " TEST_TABLE,
            [&, shared_txn](auto result) {
              ASSERT_FALSE(result.done());
              ASSERT_EQ(result::status_t::TUPLES_OK, result.status()) << result.error_message();

              for (std::size_t i = 0; i < num_rows_; ++i) {
                ASSERT_EQ(i, result.at(i).at(3).template as<std::int64_t>());
              }

              rows_seen += result.size();
              ++num_batches;
            });
      });

  ioc.run();

  ASSERT_EQ(num_rows_, rows_seen);
  ASSERT_EQ(1, num_batches);
}
