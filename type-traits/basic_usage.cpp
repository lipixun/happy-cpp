/* Author: lipixun
 * Created Time : 2024-06-25 19:10:23
 *
 * File Name: basic_usage.cpp
 * Description:
 *
 *  Basic usage
 *
 */

#include <iostream>
#include <type_traits>

template <typename T>
struct is_void {
  static const bool value = false;
};

template <>
struct is_void<void> {
  static const bool value = true;
};

template <typename T>
struct is_pointer {
  static const bool value = false;
};

template <typename T>
struct is_pointer<T*> {
  static const bool value = true;
};

template <typename T>
struct is_void2 : std::integral_constant<bool, std::is_same<T, void>::value> {};

int main() {
  std::cout << "is_void<int>::value = " << is_void<int>::value << std::endl;
  std::cout << "is_void<void>::value = " << is_void<void>::value << std::endl;
  std::cout << "is_void2<int>::value = " << is_void2<int>::value << std::endl;
  std::cout << "is_void2<void>::value = " << is_void2<void>::value << std::endl;
  std::cout << "is_pointer<int>::value = " << is_pointer<int>::value << std::endl;
  std::cout << "is_pointer<int*>::value = " << is_pointer<int*>::value << std::endl;

  return 0;
}
