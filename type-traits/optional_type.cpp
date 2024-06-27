/* Author: lipixun
 * Created Time : 2024-06-26 13:02:46
 *
 * File Name: optional_type.cpp
 * Description:
 *
 *  Optional type
 *
 */

#include <iostream>
#include <type_traits>

struct algorithm_implementation {
  struct implementation {
    static void run() { std::cout << "Run default implementation" << std::endl; }
  };
};

template <typename T>
struct algorithm_implementation_traits {
  algorithm_implementation_traits() = delete;
};

template <typename T>
concept is_algorithm_implementation_specialized = requires { algorithm_implementation_traits<T>(); };

template <>
struct algorithm_implementation_traits<int> {
  struct implementation {
    static void run() { std::cout << "Run a int specialized algorithm implementation" << std::endl; }
  };
};

template <typename T>
void run() {
  if constexpr (is_algorithm_implementation_specialized<T>) {
    return algorithm_implementation_traits<T>::implementation::run();
  } else {
    return algorithm_implementation::implementation::run();
  }
}

int main() {
  run<float>();
  run<int>();
  return 0;
}
