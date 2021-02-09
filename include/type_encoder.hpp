#pragma once

#include <string>
#include <type_traits>

namespace postgrespp {

template <class T, class Enable = void>
class type_encoder {
private:
  using no_ref_t = std::remove_const_t<std::remove_reference_t<T>>;

public:
  using encoder_t = std::conditional_t<
    std::is_array_v<no_ref_t>,
    type_encoder<std::remove_extent_t<std::add_const_t<no_ref_t>>*>,
    type_encoder<no_ref_t>>;
  using value_t = std::string;

public:
  static constexpr std::size_t size(const T& t) {
    return sizeof(t);
  }

  static constexpr int type(const T& t) {
    return 0;
  }

  value_t to_text_value(const T& t) {
    return std::to_string(t);
  }

  const char* c_str(const value_t& t) {
    return t.c_str();
  }
};

}

#include "builtin_type_encoders.hpp"
