#pragma once

#include "result_iterator.hpp"
#include "row.hpp"

#include <libpq-fe.h>

#include <cassert>
#include <cstdint>
#include <string>

namespace postgrespp {

class result {
public:
  using size_type = std::size_t;
  using iterator = result_iterator;
  using const_iterator = iterator;
  using row_t = row;

  enum class status_t : int {
    EMPTY_QUERY = PGRES_EMPTY_QUERY,
    COMMAND_OK = PGRES_COMMAND_OK,
    TUPLES_OK = PGRES_TUPLES_OK,
    BAD_RESPONSE = PGRES_BAD_RESPONSE,
    FATAL_ERROR = PGRES_FATAL_ERROR,
  };

public:
  result(PGresult* const& result) noexcept
    : res_{result} {
  }

  result(const result& other) = delete;

  result(result&& other) noexcept
    : res_{other.res_} {
    other.res_ = nullptr;
  }

  result& operator=(const result& other) = delete;

  result& operator=(result&& other) noexcept {
    using std::swap;
    swap(res_, other.res_);

    return *this;
  }

  ~result() {
    if(res_ != nullptr) {
      PQclear(res_);
    }
  }

  /**
   * If true, indicates that we are done and this result is empty. An empty
   * result is typically used to mark the end of a series of result objects
   * (e.g. \ref basic_transaction::async_exec_all).
   *
   * The result object is empty when this returns true, therefore,
   * the object must not be used, calling any other member function is invalid.
   */
  bool done() const { return res_ == nullptr; }

  bool ok() const { return status() == status_t::TUPLES_OK || status() == status_t::COMMAND_OK; }

  status_t status() const { return static_cast<status_t>(PQresultStatus(res_)); }

  const_iterator begin() const { return {res_}; }
  const_iterator cbegin() const { return begin(); }

  const_iterator end() const {
    auto rows = PQntuples(res_);
    assert(rows >= 0);
    if (rows < 0) rows = 0;
    return {res_, static_cast<size_type>(rows)};
  }
  const_iterator cend() const { return end(); }

  const row_t operator[](size_type n) const {
    return *(begin() + n);
  }

  const row_t at(size_type n) const {
    if (n >= size()) throw std::out_of_range{"row n >= size()"};

    return (*this)[n];
  }

  size_type size() const { return PQntuples(res_); }

  size_type affected_rows() const {
    const auto s = PQcmdTuples(res_);

    if (!std::strcmp(s, ""))
      throw std::runtime_error{"invalid query type for affected rows"};

    return std::stoull(s);
  }

  const char* error_message() const { return PQresultErrorMessage(res_); }

private:
  PGresult* res_;
};

}
