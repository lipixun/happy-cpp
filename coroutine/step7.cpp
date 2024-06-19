/* Author: lipixun
 * Created Time : 2024-06-18 11:35:14
 *
 * File Name: step7.cpp
 * Description:
 *
 *  Step 7: The final correct version of step 5
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
    ~Promise() { std::cout << "Promise destroyed" << std::endl; }
    ReturnType get_return_object() {
      return {
          .handle_ = std::coroutine_handle<Promise>::from_promise(*this),
      };
    }
    // Look at here: each call of 'co_yield i' equals to 'co_await promise.yield_value(i)'
    std::suspend_always yield_value(size_t value) {
      value_ = value;
      return {};
    }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }  // Haha, here is the issue.
    void unhandled_exception() {}
    void return_void() {}  // Without this function, the behaviour is undefined...

    size_t value_;
  };

  using promise_type = Promise;  // Required by c++ standard

  std::coroutine_handle<Promise> handle_;  // Use 'Promise' type argument
};

ReturnType counter(size_t num) {
  std::cout << "counter: start" << std::endl;
  for (size_t i = 0; i < num; ++i) {
    co_yield i;
  }
  std::cout << "counter: end" << std::endl;
}

int main() {
  auto return_object = counter(3);
  auto& promise = return_object.handle_.promise();
  while (!return_object.handle_.done()) {
    std::cout << "main:" << promise.value_ << std::endl;
    return_object.handle_();
  }
  return_object.handle_.destroy();  // NOTE: Please try to comment out this line and run again. You will find it's
                                    // necessary to destroy the handle explicitly
  return 0;
}

/*
Outputs:
counter: start
main:0
main:1
main:2
counter: end
Promise destroyed
*/
