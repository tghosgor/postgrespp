#pragma once

#include "type_encoder.hpp"

#include <tuple>

namespace postgrespp { namespace utility {

template <class... Params>
std::tuple<typename type_encoder<Params>::encoder_t::value_t...>
create_value_holders(Params&&... params) {
  return std::make_tuple(typename type_encoder<Params>::encoder_t{}.to_text_value(params)...);
}

template <class... Params>
std::array<const char*, sizeof...(Params)> value_array(Params&&... params) {
  return {typename type_encoder<Params>::encoder_t{}.c_str(params)...};
}

template <class... Params>
std::array<int, sizeof...(Params)> size_array(Params&&... params) {
  return {static_cast<int>(typename type_encoder<Params>::encoder_t{}.size(params))...};
}

template <class... Params>
std::array<int, sizeof...(Params)> type_array(Params&&... params) {
  return {typename type_encoder<Params>::encoder_t{}.type(params)...};
}

}}
