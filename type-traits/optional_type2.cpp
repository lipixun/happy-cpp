/* Author: lipixun
 * Created Time : 2024-06-27 13:28:34
 *
 * File Name: optional_type2.cpp
 * Description:
 *
 *  Optional type 2
 *
 */

#include <iostream>
#include <type_traits>

template <typename T>
struct algorithm_implementation {
  struct implementation {
    static void run(T value) { std::cout << "Run default implementation" << std::endl; }
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
    static void run(int value) { std::cout << "Run a int specialized algorithm implementation" << std::endl; }
  };
};

template <typename T>
using algorithm_implementation_selector =
    typename std::conditional_t<is_algorithm_implementation_specialized<T>, algorithm_implementation_traits<T>,
                                algorithm_implementation<T>>;

int main() {
  algorithm_implementation_selector<float>::implementation::run(1.0);
  algorithm_implementation_selector<int>::implementation::run(1);
  return 0;
}
