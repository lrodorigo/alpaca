#pragma once
#include <alpaca/detail/aggregate_arity.h>
#include <alpaca/detail/crc32.h>
#include <alpaca/detail/endian.h>
#include <alpaca/detail/from_bytes.h>
#include <alpaca/detail/is_specialization.h>
#include <alpaca/detail/options.h>
#include <alpaca/detail/print_bytes.h>
#include <alpaca/detail/struct_nth_field.h>
#include <alpaca/detail/to_bytes.h>
#include <alpaca/detail/type_info.h>
#include <alpaca/detail/types/array.h>
#include <alpaca/detail/types/map.h>
#include <alpaca/detail/types/optional.h>
#include <alpaca/detail/types/pair.h>
#include <alpaca/detail/types/set.h>
#include <alpaca/detail/types/string.h>
#include <alpaca/detail/types/tuple.h>
#include <alpaca/detail/types/unique_ptr.h>
#include <alpaca/detail/types/variant.h>
#include <alpaca/detail/types/vector.h>
#include <alpaca/detail/variable_length_encoding.h>
#include <cassert>
#include <system_error>

namespace alpaca {

#if defined(_MSC_VER)
#define ALPACA_FUNCTION_SIGNATURE __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__)
#define ALPACA_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#else
#error unsupported compiler
#endif

namespace detail {

template <typename T, std::size_t N, std::size_t I>
void type_info_helper(
    std::vector<uint8_t> &typeids,
    std::unordered_map<std::string_view, std::size_t> &struct_visitor_map);

// for aggregates
template <typename T,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size()>
typename std::enable_if<std::is_aggregate_v<T> && !is_array_type<T>::value,
                        void>::type
type_info(
    std::vector<uint8_t> &typeids,
    std::unordered_map<std::string_view, std::size_t> &struct_visitor_map) {

  // store num fields in struct
  // store size of struct
  // if already visited before, store index in struct_visitor_map
  // else, visit the struct and store its field types
  std::string_view name = ALPACA_FUNCTION_SIGNATURE;
  auto it = struct_visitor_map.find(name);
  if (it != struct_visitor_map.end()) {
    // struct was previously visited

    // store index in struct_visitor_map
    typeids.push_back(it->second);
  } else {
    // struct visited for first time

    // save number of fields
    uint16_t num_fields = N;
    to_bytes<options::none>(typeids, num_fields);

    // save size of struct
    uint16_t size = sizeof(T);
    to_bytes<options::none>(typeids, size);

    struct_visitor_map[name] = struct_visitor_map.size() + 1;
    type_info_helper<T, N, 0>(typeids, struct_visitor_map);
  }
}

template <typename T, std::size_t N, std::size_t I>
void type_info_helper(
    std::vector<uint8_t> &typeids,
    std::unordered_map<std::string_view, std::size_t> &struct_visitor_map) {
  if constexpr (I < N) {
    T ref{};
    decltype(auto) field = detail::get<I, T, N>(ref);
    using decayed_field_type = typename std::decay<decltype(field)>::type;

    // save type of field in struct
    type_info<decayed_field_type>(typeids, struct_visitor_map);

    // go to next field
    type_info_helper<T, N, I + 1>(typeids, struct_visitor_map);
  }
}

} // namespace detail

// Forward declares
template <options O, typename T, typename Container, std::size_t N, std::size_t I>
void serialize_helper(const T &s, Container &bytes);

namespace detail {

// Start of serialization functions

// version for nested struct/class types
// incidentally, also works for std::pair
template <options O, typename T, typename U>
typename std::enable_if<std::is_aggregate_v<U>, void>::type
to_bytes(T &bytes, const U &input) {
  serialize_helper<O, U, T, detail::aggregate_arity<std::remove_cv_t<U>>::size(),
                   0>(input, bytes);
}

template <options O, typename T, typename U>
typename std::enable_if<!std::is_aggregate_v<U> && std::is_class_v<U>,
                        void>::type
to_bytes(T &bytes, const U &input);

template <options O, typename T, typename Container>
void to_bytes_router(const T &input, Container &bytes) {
  to_bytes<O>(bytes, input);
}

} // namespace detail

/// N -> number of fields in struct
/// I -> field to start from
template <options O, typename T, typename Container,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size(),
          std::size_t I = 0>
void serialize_helper(const T &s, Container &bytes) {
  if constexpr (I < N) {
    const auto &ref = s;
    decltype(auto) field = detail::get<I, decltype(ref), N>(ref);
    using decayed_field_type = typename std::decay<decltype(field)>::type;

    // serialize field
    detail::to_bytes_router<O, decayed_field_type, Container>(field, bytes);

    // go to next field
    serialize_helper<O, T, Container, N, I + 1>(s, bytes);
  }
}

template <typename T,
          typename Container = std::vector<uint8_t>,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size()>
void serialize(const T &s, Container &bytes) {
  serialize_helper<options::none, T, Container, N, 0>(s, bytes);
}

template <typename T,
          typename Container = std::vector<uint8_t>,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size()>
Container serialize(const T &s) {
  Container bytes{};
  serialize<T, Container, N>(s, bytes);
  return bytes;
}

// overloads taking options template parameter

template <typename T, typename Container, options O,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size()>
void serialize(const T &s, Container &bytes) {
  if constexpr (N > 0 && enum_has_flag<options, O, options::with_version>()) {
    // calculate typeid hash and save it to the bytearray
    std::vector<uint8_t> typeids;
    std::unordered_map<std::string_view, std::size_t> struct_visitor_map;
    detail::type_info<T, N>(typeids, struct_visitor_map);
    uint32_t version = crc32_fast(typeids.data(), typeids.size());
    detail::to_bytes_crc32<O>(bytes, version);
  }

  serialize_helper<O, T, Container, N, 0>(s, bytes);

  if constexpr (N > 0 && enum_has_flag<options, O, options::with_checksum>()) {
    // calculate crc32 for byte array and
    // pack uint32_t to the end
    uint32_t crc = crc32_fast(bytes.data(), bytes.size());
    detail::to_bytes_crc32<O>(bytes, crc);
  }
}

template <typename T, typename Container, options O,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size()>
Container serialize(const T &s) {
  Container bytes{};
  serialize<T, Container, O, N>(s, bytes);
  return bytes;
}

// Forward declares
template <options O, typename T, std::size_t N, std::size_t index>
void deserialize_helper(T &s, const std::vector<uint8_t> &bytes,
                        std::size_t &byte_index, std::size_t &end_index,
                        std::error_code &error_code);

namespace detail {

// Start of deserialization functions

// version for nested struct/class types
template <options O, typename T>
typename std::enable_if<std::is_aggregate_v<T> && !is_array_type<T>::value,
                        bool>::type
from_bytes(T &value, const std::vector<uint8_t> &bytes, std::size_t &byte_index,
           std::size_t &end_index, std::error_code &error_code) {
  deserialize_helper<O, T, detail::aggregate_arity<std::remove_cv_t<T>>::size(),
                     0>(value, bytes, byte_index, end_index, error_code);
  return true;
}

template <options O, typename T>
void from_bytes_router(T &output, const std::vector<uint8_t> &bytes,
                       std::size_t &byte_index, std::size_t &end_index,
                       std::error_code &error_code) {
  detail::from_bytes<O>(output, bytes, byte_index, end_index, error_code);
}

} // namespace detail

/// N -> number of fields in struct
/// I -> field to start from
template <options O, typename T,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size(),
          std::size_t I = 0>
void deserialize_helper(T &s, const std::vector<uint8_t> &bytes,
                        std::size_t &byte_index, std::size_t &end_index,
                        std::error_code &error_code) {
  if constexpr (I < N) {
    decltype(auto) field = detail::get<I, T, N>(s);
    using decayed_field_type = typename std::decay<decltype(field)>::type;

    // load current field
    detail::from_bytes_router<O, decayed_field_type>(field, bytes, byte_index,
                                                     end_index, error_code);

    if (error_code) {
      // stop here
      return;
    } else {
      // go to next field
      deserialize_helper<O, T, N, I + 1>(s, bytes, byte_index, end_index,
                                         error_code);
    }
  }
}

template <typename T,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size()>
void deserialize(T &s, const std::vector<uint8_t> &bytes,
                 std::size_t &byte_index, std::size_t &end_index,
                 std::error_code &error_code) {
  deserialize_helper<options::none, T, N, 0>(s, bytes, byte_index, end_index,
                                             error_code);
}

template <typename T,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size()>
T deserialize(const std::vector<uint8_t> &bytes, std::error_code &error_code) {
  T object{};

  if (bytes.empty()) {
    error_code = std::make_error_code(std::errc::message_size);
    return object;
  }

  std::size_t byte_index = 0;
  std::size_t end_index = bytes.size();
  deserialize<T, N>(object, bytes, byte_index, end_index, error_code);
  return object;
}

// Overloads to check crc in bytes
template <typename T, options O,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size()>
void deserialize(T &s, const std::vector<uint8_t> &bytes,
                 std::size_t &byte_index, std::size_t &end_index,
                 std::error_code &error_code) {

  if constexpr (N > 0 && enum_has_flag<options, O, options::with_version>()) {

    // calculate typeid hash and save it to the bytearray
    std::vector<uint8_t> typeids;
    std::unordered_map<std::string_view, std::size_t> struct_visitor_map;
    detail::type_info<T, N>(typeids, struct_visitor_map);
    uint32_t computed_version = crc32_fast(typeids.data(), typeids.size());

    // check computed version with version in input
    // there should be at least 4 bytes in input
    if (end_index < 4) {
      error_code = std::make_error_code(std::errc::invalid_argument);
      return;
    } else {
      std::vector<uint8_t> version_bytes{};
      for (std::size_t i = 0; i < 4; ++i) {
        version_bytes.push_back(bytes[byte_index++]);
      }
      uint32_t version = 0;
      std::size_t index = 0;
      detail::from_bytes_crc32<O>(version, bytes, index, end_index,
                                  error_code); // first 4 bytes

      if (version != computed_version) {
        error_code = std::make_error_code(std::errc::invalid_argument);
        return;
      }
    }
  }

  if constexpr (enum_has_flag<options, O, options::with_checksum>()) {
    // bytes must be at least 4 bytes long
    if (end_index < 4) {
      error_code = std::make_error_code(std::errc::invalid_argument);
      return;
    } else {
      // check crc bytes
      uint32_t trailing_crc;
      std::size_t index = end_index - 4;
      detail::from_bytes_crc32<O>(trailing_crc, bytes, index, end_index,
                                  error_code); // last 4 bytes

      auto computed_crc = crc32_fast(bytes.data(), end_index - 4);

      if (trailing_crc == computed_crc) {
        // message is good!
        end_index -= 4;
        deserialize_helper<O, T, N, 0>(s, bytes, byte_index, end_index,
                                       error_code);
      } else {
        // message is bad
        error_code = std::make_error_code(std::errc::bad_message);
        return;
      }
    }
  } else {
    // bytes does not have any CRC
    // just deserialize everything into type T
    deserialize_helper<O, T, N, 0>(s, bytes, byte_index, end_index, error_code);
  }
}

template <typename T, options O,
          std::size_t N = detail::aggregate_arity<std::remove_cv_t<T>>::size()>
T deserialize(const std::vector<uint8_t> &bytes, std::error_code &error_code) {
  T object{};

  if (bytes.empty()) {
    error_code = std::make_error_code(std::errc::message_size);
    return object;
  }

  std::size_t byte_index = 0;
  std::size_t end_index = bytes.size();
  deserialize<T, O, N>(object, bytes, byte_index, end_index, error_code);
  return object;
}

} // namespace alpaca