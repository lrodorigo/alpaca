
/// Start of Library

#include <cstdint>
#include <boost/pfr.hpp>
#include <type_traits>

namespace detail {

  enum class type : uint8_t
  {
   boolean,
   uint8,
   uint16_as_uint8,
   uint16,   
   uint32_as_uint8,
   uint32_as_uint16,
   uint32,
   uint64_as_uint8,
   uint64_as_uint16,
   uint64_as_uint32,   
   uint64,       
   int8,
   int16_as_int8,
   int16,
   int32_as_int8,
   int32_as_int16,
   int32,
   int64_as_int8,
   int64_as_int16,
   int64_as_int32,
   int64,        
   float32,      
   float64,      
   string,       
   vector
  };

  type get_repr_type(const uint64_t& input) {
    if (input <= std::numeric_limits<uint8_t>::max()
        && input >= std::numeric_limits<uint8_t>::min())
    {
      return type::uint64_as_uint8;
    }
    else if (input <= std::numeric_limits<uint16_t>::max()
        && input >= std::numeric_limits<uint16_t>::min())
    {
      return type::uint64_as_uint16;
    }
    else if (input <= std::numeric_limits<uint32_t>::max()
        && input >= std::numeric_limits<uint32_t>::min())
    {
      return type::uint64_as_uint32;
    }
    else {
      return type::uint64;
    }    
  }

  type get_repr_type(const uint32_t& input) {
    if (input <= std::numeric_limits<uint8_t>::max()
        && input >= std::numeric_limits<uint8_t>::min())
    {
      return type::uint32_as_uint8;
    }
    else if (input <= std::numeric_limits<uint16_t>::max()
        && input >= std::numeric_limits<uint16_t>::min())
    {
      return type::uint32_as_uint16;
    }
    else {
      return type::uint32;
    }    
  }

  type get_repr_type(const uint16_t& input) {
    if (input <= std::numeric_limits<uint8_t>::max()
        && input >= std::numeric_limits<uint8_t>::min())
    {
      return type::uint16_as_uint8;
    }
    else {
      return type::uint16;
    }    
  }

  type get_repr_type(const int64_t& input) {
    if (input <= std::numeric_limits<int8_t>::max()
        && input >= std::numeric_limits<int8_t>::min())
    {
      return type::int64_as_int8;
    }
    else if (input <= std::numeric_limits<int16_t>::max()
        && input >= std::numeric_limits<int16_t>::min())
    {
      return type::int64_as_int16;
    }
    else if (input <= std::numeric_limits<int32_t>::max()
        && input >= std::numeric_limits<int32_t>::min())
    {
      return type::int64_as_int32;
    }
    else {
      return type::int64;
    }    
  }

  type get_repr_type(const int32_t& input) {
    if (input <= std::numeric_limits<int8_t>::max()
        && input >= std::numeric_limits<int8_t>::min())
    {
      return type::int32_as_int8;
    }
    else if (input <= std::numeric_limits<int16_t>::max()
        && input >= std::numeric_limits<int16_t>::min())
    {
      return type::int32_as_int16;
    }
    else {
      return type::int32;
    }    
  }

  type get_repr_type(const int16_t& input) {
    if (input <= std::numeric_limits<int8_t>::max()
        && input >= std::numeric_limits<int8_t>::min())
    {
      return type::int16_as_int8;
    }
    else {
      return type::int16;
    }    
  }  

  template <typename T, typename U>
  void append(const T& value, U& bytes) {
    std::copy(static_cast<const char*>(static_cast<const void*>(&value)),
	      static_cast<const char*>(static_cast<const void*>(&value)) + sizeof value,
	      std::back_inserter(bytes));
  }

  template <typename U>
  void append(type type, U& bytes) {
    append(static_cast<uint8_t>(type), bytes);
  }

  template<typename T> struct is_vector : public std::false_type {};

  template<typename T, typename A>
  struct is_vector<std::vector<T, A>> : public std::true_type {};

  template<class T>struct tag_t{};
  template<class T>constexpr tag_t<T> tag{};
  namespace is_string {
    template<class T, class...Ts>
    constexpr bool is_stringlike(tag_t<T>, Ts&&...){ return false; }
    template<class T, class A>
    constexpr bool is_stringlike( tag_t<std::basic_string<T,A>> ){ return true; }
    template<class T>
    constexpr bool detect = is_stringlike(tag<T>); // enable ADL extension
  }

  // append the type of value to bytes
  template <typename T, typename U>
  void append_value_type(U& bytes) {
    if constexpr (std::is_same<T, bool>::value) {
      append(type::boolean, bytes);
    }
    else if constexpr (std::is_same<T, char>::value) {
      append(type::uint8, bytes);
    }    
    else if constexpr (std::is_same<T, uint8_t>::value) {
      append(type::uint8, bytes);
    }
    else if constexpr (std::is_same<T, uint16_t>::value) {
      append(type::uint16, bytes);
    }
    else if constexpr (std::is_same<T, uint32_t>::value) {
      append(type::uint32, bytes);
    }
    else if constexpr (std::is_same<T, uint64_t>::value) {
      append(type::uint64, bytes);
    }
    else if constexpr (std::is_same<T, int8_t>::value) {
      append(type::int8, bytes);
    }
    else if constexpr (std::is_same<T, int16_t>::value) {
      append(type::int16, bytes);
    }
    else if constexpr (std::is_same<T, int32_t>::value) {
      append(type::int32, bytes);
    }
    else if constexpr (std::is_same<T, int64_t>::value) {
      append(type::int64, bytes);
    }
    else if constexpr (std::is_same<T, float>::value) {
      append(type::float32, bytes);
    }
    else if constexpr (std::is_same<T, double>::value) {
      append(type::float64, bytes);
    }
    else if constexpr (is_string::detect<T>) {
      append(type::string, bytes);
    }
    else if constexpr (is_vector<T>::value) {
      append(type::vector, bytes);
      append_value_type<typename std::decay<typename T::value_type>::type>(bytes);
    }
  }  

  template <bool save_type_info = true>
  void to_bytes(uint8_t input, std::vector<uint8_t>& bytes) {    
    // type of the value
    if constexpr (save_type_info) {
      append(type::uint8, bytes);
    }
    // value
    append(input, bytes);
  }

  template <bool save_type_info = true>  
  void to_bytes(uint16_t input, std::vector<uint8_t>& bytes) {
    // type of the value
    if constexpr (save_type_info) {
      append(get_repr_type(input), bytes);
    }
    
    if (input <= std::numeric_limits<uint8_t>::max()
        && input >= std::numeric_limits<uint8_t>::min())
    {
      // value can fit in an uint8_t
      to_bytes<false>(static_cast<uint8_t>(input), bytes);
    }
    else {
      // value
      append(input, bytes);
    }
  }  

  template <bool save_type_info = true>  
  void to_bytes(uint32_t input, std::vector<uint8_t>& bytes) {
    // type of the value
    if constexpr (save_type_info) {
      append(get_repr_type(input), bytes);
    }
    
    if (input <= std::numeric_limits<uint16_t>::max()
        && input >= std::numeric_limits<uint16_t>::min())
    {
      // value can fit in an uint16_t
      to_bytes<false>(static_cast<uint16_t>(input), bytes);
    }
    else {
      // value
      append(input, bytes);
    }
  }

  template <bool save_type_info = true>  
  void to_bytes(uint64_t input, std::vector<uint8_t>& bytes) {
    // type of the value
    if constexpr (save_type_info) {      
      append(get_repr_type(input), bytes);
    }
    
    if (input <= std::numeric_limits<uint32_t>::max()
        && input >= std::numeric_limits<uint32_t>::min())
    {
      // value can fit in an uint32_t
      to_bytes<false>(static_cast<uint32_t>(input), bytes);
    }
    else {
      // value
      append(input, bytes);
    }
  }

  template <bool save_type_info = true>  
  void to_bytes(bool input, std::vector<uint8_t>& bytes) {
    // type of the value
    if constexpr (save_type_info) {    
      append(type::boolean, bytes);
    }
    // value
    append(input, bytes);
  }  

  template <bool save_type_info = true>  
  void to_bytes(char input, std::vector<uint8_t>& bytes) {
    to_bytes<save_type_info>(static_cast<uint8_t>(input), bytes);
  }  

  template <bool save_type_info = true>  
  void to_bytes(int8_t input, std::vector<uint8_t>& bytes) {
    // type of the value
    if constexpr (save_type_info) {
      append(type::int8, bytes);
    }
    // value
    append(input, bytes);
  }    

  template <bool save_type_info = true>
  void to_bytes(int16_t input, std::vector<uint8_t>& bytes) {
    // type of the value
    if constexpr (save_type_info) {
      append(get_repr_type(input), bytes);	
    }
    
    if (input <= std::numeric_limits<int8_t>::max()
        && input >= std::numeric_limits<int8_t>::min())
    {
      // value can find in an int8_t
      to_bytes<false>(static_cast<int8_t>(input), bytes);
    }
    else {
      // value
      append(input, bytes);
    }
  }  

  template <bool save_type_info = true, bool originally_int32 = true>  
  void to_bytes(int32_t input, std::vector<uint8_t>& bytes) {
    // type of the value
    if constexpr (save_type_info) {
      append(get_repr_type(input), bytes);	
    }    
    
    if (input <= std::numeric_limits<int16_t>::max()
        && input >= std::numeric_limits<int16_t>::min())
    {
      // value can find in an int16_t
      to_bytes<false>(static_cast<int16_t>(input), bytes);
    }
    else {
      // value
      append(input, bytes);
    }
  }

  template <bool save_type_info = true>  
  void to_bytes(int64_t input, std::vector<uint8_t>& bytes) {
    // type of the value
    if constexpr (save_type_info) {
      append(get_repr_type(input), bytes);	
    }    
    
    if (input <= std::numeric_limits<int32_t>::max()
        && input >= std::numeric_limits<int32_t>::min())
    {
      // value can find in an int32_t
      to_bytes<false>(static_cast<int32_t>(input), bytes);
    }
    else {
      // value
      append(input, bytes);
    }
  }

  template <bool save_type_info = true>  
  void to_bytes(const float input, std::vector<uint8_t>& bytes) {
    if constexpr (save_type_info) {
      append(type::float32, bytes);
    }
    append(input, bytes);
  }

  template <bool save_type_info = true>
  void to_bytes(const double input, std::vector<uint8_t>& bytes) {
    if constexpr (save_type_info) {
      append(type::float64, bytes);
    }
    append(input, bytes);
  } 

  template <bool save_type_info = true>  
  void to_bytes(const std::string& input, std::vector<uint8_t>& bytes) {
    if constexpr (save_type_info) {
      append(type::string, bytes);
    }
    
    to_bytes<true>(input.size(), bytes);

    for (auto& c: input) {
      append(c, bytes);
    }
  }

  template <bool save_type_info, typename T>
  void to_bytes_from_list_type(const T& input, std::vector<uint8_t>& bytes) {
    // type of the value
    if constexpr (save_type_info) {
      append(type::vector, bytes);
      // save type of list::value_type
      append_value_type<typename std::decay<typename T::value_type>::type>(bytes);      
    }
    
    to_bytes<true>(input.size(), bytes);

    // value of each element in list
    for (auto& v: input) {
      // check if the value_type is a nested list type
      using decayed_value_type = typename std::decay<decltype(v)>::type;
      if constexpr (is_vector<decayed_value_type>::value) {
	to_bytes_from_list_type<false, decayed_value_type>(v, bytes);
      }
      else {
	// dump all the values
	// note: no attempted compression for integer types
	append(v, bytes);

	// note:
	// if integer compression is requested
	// call:
	//    to_bytes(v, bytes);
	// instead
      }
    }
  }
  
}

template<typename T, std::size_t index = 0>
void serialize(T& s, std::vector<uint8_t>& bytes) {
  constexpr static auto max_index = boost::pfr::tuple_size<T>::value;
  if constexpr (index < max_index) {
    using decayed_field_type = typename std::decay<decltype(boost::pfr::get<index>(s))>::type;
    if constexpr (detail::is_vector<decayed_field_type>::value) {
      detail::to_bytes_from_list_type<true, decayed_field_type>(boost::pfr::get<index>(s), bytes);
    }
    else {      
      detail::to_bytes<true>(boost::pfr::get<index>(s), bytes);
    }
    serialize<T, index + 1>(s, bytes);
  }
}

/// Start of Test Code

#include <iostream>

struct my_struct {
  bool flag;
  std::string s;
  int i;
  float f;
  std::vector<int> list;
  std::vector<std::vector<char>> list_of_lists;
  std::vector<std::vector<int>> list_of_lists_int;
};


int main() {
  std::ios_base::fmtflags f(std::cout.flags());
  
  // Test 1
  {
    my_struct s{true, {"Hello world!"}, 5, 3.14, {1, 2, 3, 4, 5},
		{{'a', 'b', 'c'}, {'d', 'e', 'f'}},
		{{123, 456, 789}, {101112, 131415, 161718}}};
    std::cout << "Original struct size : " << sizeof(s) << " bytes\n";

    std::vector<uint8_t> bytes{};
    serialize<my_struct>(s, bytes);

    bytes.shrink_to_fit();
    std::cout << "Serialized to        : " << bytes.size() << " bytes\n";
    std::cout << "Compression ratio    : " << (float(sizeof(s)) / float(bytes.size()) * 100.0f) << "%\n";
    std::cout << "Space savings        : " << (1 - float(bytes.size()) / float(sizeof(s))) * 100.0f << "%\n";  

    for (auto& b: bytes) {
      std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
    }
    std::cout << "\n";
    std::cout.flags(f);
  }

  std::cout << "\n---\n\n";

  // Test 2
  {
    struct list {
      std::vector<int> values;
    };
    list s;
    for (int i = 0; i < 10E6; ++i) {
      s.values.push_back(i);
    }
    
    std::vector<uint8_t> bytes{};
    serialize<list>(s, bytes);
    bytes.shrink_to_fit();
    std::cout << "Original struct size : " << s.values.size() * sizeof(s.values[0]) << " bytes\n";
    std::cout << "Serialized to        : " << bytes.size() << " bytes\n";
    std::cout << "Compression ratio    : " << (float(s.values.size() * sizeof(s.values[0])) / float(bytes.size()) * 100.0f) << "%\n";
    std::cout << "Space savings        : " << (1 - float(bytes.size()) / float(sizeof(s))) * 100.0f << "%\n";
    std::cout.flags(f);
  }
  
}