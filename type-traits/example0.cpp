/* Author: lipixun
 * Created Time : 2024-06-25 19:47:12
 *
 * File Name: example0.cpp
 * Description:
 *
 *  Example0
 *
 */

#include <iostream>
#include <type_traits>

template <bool USE_OPTIMIZED>
struct algorithm_implementation {
  template <typename T>
  static void run(const T& obj) {
    std::cout << "Run not optimized algorithm" << std::endl;
  }
};

template <>
struct algorithm_implementation<true> {
  template <typename T>
  static void run(const T& obj) {
    std::cout << "Run optimized algorithm" << std::endl;
    obj.optimized_codes();
  }
};

template <typename T>
struct if_supports_optimised_algorithm : std::false_type {};

struct ObjectA {};

struct ObjectB {
  static void optimized_codes() { std::cout << "ObjectB: optimized_codes" << std::endl; }
};

template <>
struct if_supports_optimised_algorithm<ObjectB> : std::true_type {};

template <typename T>
void run_algorithm(const T& obj) {
  algorithm_implementation<if_supports_optimised_algorithm<T>::value>::run(obj);
}

int main() {
  ObjectA a;
  ObjectB b;
  run_algorithm(a);
  run_algorithm(b);
}
