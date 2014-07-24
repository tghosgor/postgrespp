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

#include "include/result.hpp"

namespace postgrespp
{

Result::Result(PGresult* const& result) :
  m_result(result),
  m_numRows(PQntuples(m_result))
{}

Result::Result(Result&& other) :
  m_result(other.m_result),
  m_numRows(other.m_numRows)
{
  other.m_result = nullptr;
}

Result::~Result() {
  if(m_result != nullptr)
    PQclear(m_result);
}

template<>
int8_t Result::get<int8_t>(int const& column) {
  assert(m_result != nullptr);
  return *reinterpret_cast<int8_t*>(PQgetvalue(m_result, m_row, column));
}

template<>
int16_t Result::get<int16_t>(int const& column) {
  assert(m_result != nullptr);
  return be16toh(*reinterpret_cast<int16_t*>(PQgetvalue(m_result, m_row, column)));
}

template<>
int32_t Result::get<int32_t>(int const& column) {
  assert(m_result != nullptr);
  return be32toh(*reinterpret_cast<int32_t*>(PQgetvalue(m_result, m_row, column)));
}

template<>
int64_t Result::get<int64_t>(int const& column) {
  assert(m_result != nullptr);
  return be64toh(*reinterpret_cast<int64_t*>(PQgetvalue(m_result, m_row, column)));
}

template<>
char* Result::get<char*>(int const& column) {
  assert(m_result != nullptr);
  return PQgetvalue(m_result, m_row, column);
}

}
