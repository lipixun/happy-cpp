/* Author: lipixun
 * Created Time : 2024-06-26 14:01:41
 *
 * File Name: type_constraits.cpp
 * Description:
 *
 *  Type contraints
 *
 */

#include <concepts>
#include <type_traits>

struct s0 {
  static void run() {}
};

struct s1 {};

template <typename T>
concept MustHasRun = requires(T t) {
  { t.run() } -> std::same_as<void>;
};

template <MustHasRun T>
void run() {
  T t;
  t::run();
}

int main() {
  run<s0>();
  run<s1>();
}
