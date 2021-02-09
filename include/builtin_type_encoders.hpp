#pragma once

#include <boost/endian/conversion.hpp>

#include <cstring>
#include <string>

namespace postgrespp {

template <>
class type_encoder<std::string, void> {
public:
  using encoder_t = type_encoder<std::string>;
  using value_t = const char*;

public:
  static std::size_t size(const std::string& t) {
    return t.size();
  }

  static constexpr int type(const std::string& t) {
    return 0;
  }

  value_t to_text_value(const std::string& t) {
    return t.c_str();
  }

  const char* c_str(const std::string& t) {
    return t.c_str();
  }
};

template <>
class type_encoder<const char*, void> {
public:
  using encoder_t = type_encoder<const char*>;
  using value_t = const char*;

public:
  static std::size_t size(const char* const& t) {
    return std::strlen(t);
  }

  static constexpr int type(const char* const& t) {
    return 0;
  }

  value_t to_text_value(const char* const& t) {
    return t;
  }

  const char* c_str(const value_t& t) {
    return t;
  }
};

template <class T>
class type_encoder<T, std::enable_if_t<std::is_integral_v<T>>> {
public:
  using value_t = T;

public:
  static std::size_t size(const T& t) {
    return sizeof(t);
  }

  static constexpr int type(const T& t) {
    return 1;
  }

  value_t to_text_value(const T& t) {
    return boost::endian::native_to_big(t);
  }

  const char* c_str(const T& t) {
    return reinterpret_cast<const char*>(&t);
  }
};

}
