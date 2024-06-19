/* Author: lipixun
 * Created Time : 2024-06-18 12:11:38
 *
 * File Name: step9.cpp
 * Description:
 *
 *  Step 9: Add more fun to step 8
 *  - Some thing like 'send to generator in python'
 *  - Iterator interface
 *
 */

#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>

template <typename ReturnType, typename SendType, SendType defaultSendValue>
class Generator {
 public:
  template <typename PromiseType>
  class SendAwaiter {
   public:
    constexpr bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {}
    const SendType& await_resume() const noexcept { return handle_.promise().send_value_; }
    std::coroutine_handle<PromiseType> handle_;
  };

  class Promise {
   public:
    Generator get_return_object() { return Generator(std::coroutine_handle<Promise>::from_promise(*this)); }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { exception_ = std::current_exception(); }
    void return_void() {}

    template <std::convertible_to<ReturnType> From>  // C++20 concept
    SendAwaiter<Promise> yield_value(From&& value) {
      return_value_ = std::forward<From>(value);
      return {
          .handle_ = std::coroutine_handle<Promise>::from_promise(*this),
      };
    }

    SendType send_value_;
    ReturnType return_value_;
    std::exception_ptr exception_;
  };

  class Sentinel {};

  class Iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ReturnType;
    using reference = ReturnType&;
    using pointer = ReturnType*;

    template <typename T>
    Iterator(Generator& gen, T&& default_send_value) noexcept
        : gen_(gen), default_send_value_(std::forward<T>(default_send_value)) {}
    Iterator(const Iterator&) = default;
    ~Iterator() = default;

    friend bool operator==(const Iterator& it, Sentinel) noexcept { return it.gen_.Done(); }

    Iterator& operator++() {
      gen_.Next(default_send_value_);
      return *this;
    }

    void operator++(int) { operator++(); }

    reference operator*() const { return *gen_.Get(); }

    void Next(SendType&& send_value) { gen_.Next(std::forward<SendType>(send_value)); }

   private:
    friend Generator;
    Generator& gen_;
    SendType default_send_value_;
  };

  using promise_type = Promise;

  Generator(const std::coroutine_handle<Promise>& handle) : handle_(handle) {}

  ~Generator() { handle_.destroy(); }

  explicit operator bool() const noexcept { return !handle_.done() && !consumed_; }

  std::optional<ReturnType>& operator()() { return operator()(defaultSendValue); }
  std::optional<ReturnType>& operator()(SendType&& send_value) {
    Next(std::forward<SendType>(send_value));
    return Get();
  }

  std::optional<ReturnType>& Get() {
    consumed_ = true;
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    return value_;
  }

  void Next() { Next(defaultSendValue); }

  template <typename T>
  void Next(T&& send_value) {
    if (!handle_.done()) {
      handle_.promise().send_value_ = std::forward<T>(send_value);
      handle_();
      if (exception_ = handle_.promise().exception_; exception_) {
        value_ = {};
      } else if (handle_.done()) {
        // The final suspend
        value_ = {};
      } else {
        consumed_ = false;
        value_ = std::move(handle_.promise().return_value_);  // Move the value out of promise
      }
    } else {
      value_ = {};
    }
  }

  bool Done() { return handle_.done(); }

  Iterator begin() { return begin(defaultSendValue); }

  Iterator begin(SendType&& send_value) {
    Next(send_value); // Read the first value
    return Iterator(*this, std::forward<SendType>(send_value));
  }

  Sentinel end() noexcept { return {}; }

 private:
  std::coroutine_handle<Promise> handle_;
  bool consumed_ = true;
  std::optional<ReturnType> value_;
  std::exception_ptr exception_;
};

Generator<size_t, size_t, 1> counter(size_t max) {
  for (size_t i = 0; i < max;) {
    auto step = co_yield i;
    i += step;
  }
}

int main() {
  //
  // Usage 0
  //
  std::cout << "[+] Usage0" << std::endl;
  auto gen0 = counter(3);
  gen0.Next();
  while (gen0) {
    std::cout << "main:" << *gen0.Get() << std::endl;
    gen0.Next();
  }
  //
  // Usage 1
  //
  std::cout << "[+] Usage1" << std::endl;
  auto gen1 = counter(5);
  auto value1 = gen1(2);
  while (value1) {
    std::cout << "main:" << *value1 << std::endl;
    value1 = gen1(2);
  }
  //
  // Usage 2
  //
  std::cout << "[+] Usage2" << std::endl;
  auto gen2 = counter(9);
  for (auto it = gen2.begin(3); it != gen2.end(); ++it) {
    std::cout << "main:" << *it << std::endl;
  }
  //
  // Usage 3
  //
  std::cout << "[+] Usage3" << std::endl;
  auto gen3 = counter(3);
  for (const auto value : gen3) {
    std::cout << "main:" << value << std::endl;
  }
  return 0;
}

/*
Outputs:
main:0
main:1
main:2
*/
