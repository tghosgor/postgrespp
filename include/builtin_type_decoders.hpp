#pragma once

#include <boost/endian/buffers.hpp>

#include <string>
#include <type_traits>

namespace postgrespp {

template <class T>
class type_decoder<T, std::enable_if_t<std::is_integral_v<T>>> {
public:
  T from_binary(const char* data) {
    using namespace boost::endian;

    return endian_load<T, sizeof(T), order::big>(reinterpret_cast<unsigned const char*>(data));
  }
};

}
