/* Author: lipixun
 * Created Time : 2024-06-18 10:06:49
 *
 * File Name: step0.cpp
 * Description:
 *
 *  Step 0: A very simple example.
 *
 */

#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>

class ReturnType {
 public:
  // Define the promise type
  class Promise {
   public:
    ReturnType get_return_object() { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
  };

  using promise_type = Promise;  // Required by c++ standard
};

class Awaiter {
 public:
  std::coroutine_handle<> *p_handle_;
  constexpr bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) { *p_handle_ = h; }
  constexpr void await_resume() const noexcept {}
};

ReturnType counter(std::coroutine_handle<> *out_p_handle) {
  std::cout << "counter: start" << std::endl;
  Awaiter awaiter{.p_handle_ = out_p_handle};
  for (size_t i = 0;; ++i) {
    std::cout << "counter: before co_await" << i << std::endl;
    co_await awaiter;
    std::cout << "counter: after co_await" << i << std::endl;
  }
  std::cout << "counter: end" << std::endl;
}

int main() {
  std::coroutine_handle<> handle;
  counter(&handle);
  for (size_t i = 0; i < 3; ++i) {
    std::cout << "main" << std::endl;
    handle();
  }
  handle.destroy();
  return 0;
}

/*
Outputs:
counter: start
counter: before co_await0
main:0
counter: after co_await0
counter: before co_await1
main:1
counter: after co_await1
counter: before co_await2
main:2
counter: after co_await2
counter: before co_await3
*/
