/* Author: lipixun
 * Created Time : 2024-06-18 10:22:00
 *
 * File Name: step1.cpp
 * Description:
 *
 *  Step 1: Without using coroutine handle explicitly.
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
    ReturnType get_return_object() {
      return {
          .handle_ = std::coroutine_handle<Promise>::from_promise(*this),
      };
    }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
  };

  using promise_type = Promise;  // Required by c++ standard

  std::coroutine_handle<> handle_;
};

ReturnType counter() {
  std::cout << "counter: start" << std::endl;
  for (size_t i = 0;; ++i) {
    std::cout << "counter: before co_await" << i << std::endl;
    co_await std::suspend_always{};
    std::cout << "counter: after co_await" << i << std::endl;
  }
  std::cout << "counter: end" << std::endl;
}

int main() {
  auto return_object = counter();
  for (size_t i = 0; i < 3; ++i) {
    std::cout << "main" << std::endl;
    return_object.handle_();
  }
  return_object.handle_.destroy();
  return 0;
}

/*
Outputs:
counter: start
counter: before co_await0
main
counter: after co_await0
counter: before co_await1
main
counter: after co_await1
counter: before co_await2
main
counter: after co_await2
counter: before co_await3
*/
