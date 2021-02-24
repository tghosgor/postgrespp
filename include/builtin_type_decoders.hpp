#pragma once

#include <boost/endian/buffers.hpp>

#include <string>
#include <type_traits>

namespace postgrespp {

template <class T>
class type_decoder<T, std::enable_if_t<std::is_integral_v<T>>> {
public:
  static constexpr std::size_t min_size = sizeof(T);
  static constexpr std::size_t max_size = sizeof(T);
  static constexpr bool nullable = false;

public:
  T from_binary(const char* data, std::size_t length) {
    using namespace boost::endian;

    return endian_load<T, sizeof(T), order::big>(reinterpret_cast<unsigned const char*>(data));
  }
};

template <>
class type_decoder<const char*, void> {
public:
  static constexpr std::size_t min_size = 0;
  static constexpr std::size_t max_size = std::numeric_limits<std::size_t>::max();
  static constexpr bool nullable = true;

public:
  const char* from_binary(const char* data, std::size_t length) {
    return data;
  }
};

template <>
class type_decoder<std::string, void> {
public:
  static constexpr std::size_t min_size = 0;
  static constexpr std::size_t max_size = std::numeric_limits<std::size_t>::max();
  static constexpr bool nullable = true;

public:
  std::string from_binary(const char* data, std::size_t length) {
    return {data, length};
  }
};

}
