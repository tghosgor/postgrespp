/*
  Copyright (c) 2014, Tolga HOŞGÖR
  All rights reserved.

  This file is part of postgrespp.

  postgrespp is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  postgrespp is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with postgrespp.  If not, see <http://www.gnu.org/licenses/>
*/

#pragma once

#include <endian.h>

#include <cassert>
#include <cstdint>
#include <libpq-fe.h>

namespace postgrespp {

class Result {
  friend class Connection;

public:
  using Status = int;

public:
  Result(Result&& other);
  ~Result();

  /**
   * @brief rows
   * @return The number of rows in the result set.
   */
  int rows() {
    return m_numRows;
  }

  /**
   * @brief next Increments the internal row counter to the next row.
   *
   * @return True if it the incremented row is a valid row, false if not and we are past the end row.
   */
  bool next() {
    assert(m_row + 1 < rows());
    return (++m_row < rows());
  }

  /**
   * @brief get
   * @return The value in 'coulmn'th coulmn in the current row.
   */
  template<typename T>
  T get(int const& column);

  /**
   * @brief getStatus
   * @return Result status.
   */
  Status getStatus() {
    assert(m_result != nullptr);
    return PQresultStatus(m_result);
  }

  /**
   * @brief getStatusText
   * @return Result status as text.
   */
  const char* getStatusText() {
    assert(m_result != nullptr);
    return PQresStatus(PQresultStatus(m_result));
  }

  /**
   * @brief getErrorMessage
   * @return Error message associated with the result.
   */
  const char* getErrorMessage() {
    assert(m_result != nullptr);
    return PQresultErrorMessage(m_result);
  }

  /**
   * @brief reset Resets the internal row counter to the first row.
   */
  void reset() {
    m_row = -1;
  }

private:
  enum class Format : int {
    TEXT = 0,
    BINARY = 1
  };

  PGresult* m_result;
  int m_row = -1;
  int m_numRows;

private:
  Result(PGresult* const& result);
};

template<>
int8_t Result::get<int8_t>(int const&);
template<>
int16_t Result::get<int16_t>(int const&);
template<>
int32_t Result::get<int32_t>(int const&);
template<>
int64_t Result::get<int64_t>(int const&);
template<>
char* Result::get<char*>(int const&);

}// NS postgrespp
