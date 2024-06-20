/* Author: lipixun
 * Created Time : 2024-06-18 11:40:23
 *
 * File Name: step8.cpp
 * Description:
 *
 *  Step 8: A generic generator
 *
 */

#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>

template <typename T>
class Generator {
 public:
  class promise_type {
   public:
    Generator get_return_object() { return Generator(std::coroutine_handle<promise_type>::from_promise(*this)); }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { exception_ = std::current_exception(); }
    void return_void() {}

    template <std::convertible_to<T> From>  // C++20 concept
    std::suspend_always yield_value(From&& value) {
      value_ = std::forward<From>(value);
      return {};
    }

    T value_;
    std::exception_ptr exception_;
  };

  Generator(const std::coroutine_handle<promise_type>& handle) : handle_(handle) {}

  ~Generator() { handle_.destroy(); }

  explicit operator bool() {
    Next();
    return !handle_.done();
  }

  const T& operator()() {
    Next();
    consumed_ = true;
    return handle_.promise().value_;
  }

 private:
  void Next() {
    if (consumed_) {
      handle_();
      if (handle_.promise().exception_) {
        std::rethrow_exception(handle_.promise().exception_);
      }
      consumed_ = false;
    }
  }

  bool consumed_ = true;
  std::coroutine_handle<promise_type> handle_;
};

/*

The real logic implementation, looks great even we've wrote a lot of clunky code above...

*/

Generator<size_t> counter(size_t num) {
  for (size_t i = 0; i < num; ++i) {
    co_yield i;
  }
}

int main() {
  auto gen = counter(3);
  while (gen) {
    std::cout << "main:" << gen() << std::endl;
  }
  return 0;
}

/*
Outputs:
main:0
main:1
main:2
*/
