#pragma once
#ifndef ALPACA_EXCLUDE_SUPPORT_STD_TEMPERATURE_PTR
#include <alpaca/detail/to_bytes.h>
#include <alpaca/detail/type_info.h>
#include <memory>
#include <system_error>
#include <vector>
#include "model/utils.h"

namespace alpaca {

namespace detail {

template <typename T>
typename std::enable_if<std::is_same_v<T, Temperature>, void>::type
type_info(
    std::vector<uint8_t> &typeids,
    std::unordered_map<std::string_view, std::size_t> &struct_visitor_map) {

  typeids.push_back(to_byte<field_type::temperature>());
}

template <options O, typename T, typename Container>
void to_bytes_router(const T &input, Container &bytes, std::size_t &byte_index);

template <options O, typename Container>
void to_bytes(Container &bytes, std::size_t &byte_index,
              const Temperature &input) {

  to_bytes_router<O>((float) input, bytes, byte_index);

}

template <options O, typename T, typename Container>
void from_bytes_router(T &output, Container &bytes, std::size_t &byte_index,
                       std::size_t &end_index, std::error_code &error_code);

template <options O, typename Container>
bool
from_bytes(Temperature &output, Container &bytes,
           std::size_t &byte_index, std::size_t &end_index,
           std::error_code &error_code) {

  if (byte_index >= end_index) {
    // end of input
    // return true for forward compatibility
    return true;
  }

  float value{};
  from_bytes_router<O>(value, bytes, byte_index, end_index, error_code);

  output = value;

  return true;
}

} // namespace detail

} // namespace alpaca
#endif