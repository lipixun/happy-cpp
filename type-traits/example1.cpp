/* Author: lipixun
 * Created Time : 2024-06-25 20:03:37
 *
 * File Name: example1.cpp
 * Description:
 *
 *  Concept version of example0
 *
 */

#include <iostream>
#include <type_traits>

template <typename T>
concept HasOptimizedCodes = requires(T t) {
  { t.optimized_codes() } -> std::same_as<void>;
};

struct algorithm_implementation {
  template <typename T>
  static void run(const T& obj) {
    if constexpr (HasOptimizedCodes<T>) {
      std::cout << "Run optimized algorithm" << std::endl;
      obj.optimized_codes();
    } else {
      std::cout << "Run not optimized algorithm" << std::endl;
    }
  }
};

struct ObjectA {};

struct ObjectB {
  static void optimized_codes() { std::cout << "ObjectB: optimized_codes" << std::endl; }
};

template <typename T>
void run_algorithm(const T& obj) {
  algorithm_implementation::run<T>(obj);
}

int main() {
  ObjectA a;
  ObjectB b;
  run_algorithm(a);
  run_algorithm(b);
}
