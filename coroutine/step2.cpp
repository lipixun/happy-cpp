/* Author: lipixun
 * Created Time : 2024-06-18 10:30:13
 *
 * File Name: step2.cpp
 * Description:
 *
 *  Step 2: Transfer data by Promise type.
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

    size_t value_;
  };

  using promise_type = Promise;  // Required by c++ standard

  std::coroutine_handle<Promise> handle_;  // Use 'Promise' type argument
};

class GetPromiseAwaiter {
 public:
  constexpr bool await_ready() const noexcept { return false; }
  bool await_suspend(std::coroutine_handle<ReturnType::Promise> h) {
    promise_ = &h.promise();
    return false;  // Do not suspend
  }
  ReturnType::Promise* await_resume() const noexcept { return promise_; }

 private:
  ReturnType::Promise* promise_;
};

ReturnType counter() {
  std::cout << "counter: start" << std::endl;
  auto p_promise = co_await GetPromiseAwaiter{};
  std::cout << "counter: before for" << std::endl;
  for (size_t i = 0;; ++i) {
    p_promise->value_ = i;
    co_await std::suspend_always{};
  }
  std::cout << "counter: end" << std::endl;
}

int main() {
  auto return_object = counter();
  auto& promise = return_object.handle_.promise();
  for (size_t i = 0; i < 3; ++i) {
    std::cout << "main:" << promise.value_ << std::endl;
    return_object.handle_();
  }
  return_object.handle_.destroy();
  return 0;
}

/*
Outputs:
counter: start
counter: before for
main:0
main:1
main:2
*/
