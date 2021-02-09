#pragma once

namespace postgrespp {

template <class T, class Enable = void>
class type_decoder {
public:
  T from_binary(const char* data) {
    return data;
  }
};

}

#include "builtin_type_decoders.hpp"
