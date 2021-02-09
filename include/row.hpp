#pragma once

#include "field.hpp"

#include <libpq-fe.h>

#include <cstdint>
#include <stdexcept>

namespace postgrespp {

class row {
public:
  using field_t = field;
  using size_type = std::size_t;

public:
  row(const PGresult* res, size_type row)
    : res_{res}
    , row_{row} {
  }

  const field_t operator[](size_type n) const {
    return {res_, row_, n};
  }

  const field_t at(size_type n) const {
    if (n >= PQnfields(res_)) throw std::out_of_range{"field n >= size()"};

    return {res_, row_, n};
  }

private:
  const PGresult* const res_;
  const size_type row_;
};

}
