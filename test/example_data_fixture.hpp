#pragma once

#include <connection.hpp>

#include <gtest/gtest.h>

#include <pqxx/pqxx>

#define CONN_STRING "host=127.0.0.1 user=postgres"
#define TEST_TABLE "tbl_test"
#define TEST_TABLE_INITIAL_ROWS "3"

class example_data_fixture : public ::testing::Test {
protected:
  using connection_t = ::postgrespp::connection;
  using ioc_t = connection_t::io_context_t;

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
};
