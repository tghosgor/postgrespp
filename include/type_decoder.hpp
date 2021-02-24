#pragma once

#include <limits>
#include <optional>

namespace postgrespp {

template <class T, class Enable = void>
class type_decoder {
public:
  static constexpr std::size_t min_size = 0;
  static constexpr std::size_t max_size = std::numeric_limits<std::size_t>::max();
  static constexpr bool nullable = false;

public:
  T from_binary(const char* data, std::size_t length) {
    return data;
  }
};

template <class T>
class type_decoder<std::optional<T>, void> {
private:
  using underlying_decoder_t = type_decoder<T, void>;

public:
  static constexpr std::size_t min_size = underlying_decoder_t::min_size;
  static constexpr std::size_t max_size = underlying_decoder_t::max_size;

  static constexpr bool nullable = true;

public:
  std::optional<T> from_binary(const char* data, std::size_t length) {
    if (length == 0) {
      return {};
    } else {
      return underlying_decoder_t{}.from_binary(data, length);
    }
  }
};

}

#include "builtin_type_decoders.hpp"
