#pragma once

#include "type_decoder.hpp"

#include <libpq-fe.h>

#include <cstdint>
#include <stdexcept>

namespace postgrespp {

class field {
public:
  using size_type = std::size_t;

public:
  field(const PGresult* res, size_type row, size_type col)
    : res_{res}
    , row_{row}
    , col_{col} {
  }

  template <class T>
  T as() const {
    using decoder_t = type_decoder<T>;

    if (!decoder_t::nullable && is_null())
      throw std::length_error{"field is null"};

    const auto field_length = PQgetlength(res_, row_, col_);
    if (!(field_length == 0 && decoder_t::nullable) &&
        field_length < decoder_t::min_size || field_length > decoder_t::max_size)
      throw std::length_error{"field length " + std::to_string(field_length) + " not in range " +
        std::to_string(decoder_t::min_size) + "-" +
        std::to_string(decoder_t::max_size)};

    return unsafe_as<T>();
  }

  template <class T>
  T as(T&& default_value) const {
    if (is_null())
      return std::forward<T>(default_value);
    else
      return as<T>();
  }

  template <class T>
  T unsafe_as() const {
    type_decoder<T> decoder{};

    return decoder.from_binary(PQgetvalue(res_, row_, col_), PQgetlength(res_, row_, col_));
  }

  template <class T>
  T unsafe_as(T&& default_value) const {
    if (is_null())
      return std::forward<T>(default_value);
    else
      return unsafe_as<T>();
  }

  bool is_null() const {
    return PQgetisnull(res_, row_, col_);
  }

private:
  const PGresult* const res_;
  const size_type row_;
  const size_type col_;
};

}
