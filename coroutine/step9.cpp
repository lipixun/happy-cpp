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

template <typename ReturnType, typename SendType, SendType DEFAULT_SEND_VALUE>
class Generator {
 public:
  //
  // Promise
  //
  class promise_type {
   public:
    Generator get_return_object() { return Generator(std::coroutine_handle<promise_type>::from_promise(*this)); }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { exception_ = std::current_exception(); }
    void return_void() {}

    template <std::convertible_to<ReturnType> From>  // C++20 concept
    auto yield_value(From&& value) {
      return_value_ = std::forward<From>(value);

      struct send_awaiter {
        std::coroutine_handle<promise_type> handle_;

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) {}
        const SendType& await_resume() const noexcept {
          // Return the send value
          return handle_.promise().send_value_;
        }
      };

      return send_awaiter{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    SendType send_value_;
    ReturnType return_value_;
    std::exception_ptr exception_;
  };

  //
  // Iterator
  //

  class sentinel {};

  class iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ReturnType;
    using reference = ReturnType&;
    using pointer = ReturnType*;

    template <typename T>
    iterator(Generator& gen, T&& send_value) noexcept : gen_(gen), send_value_(std::forward<T>(send_value)) {
      operator++();  // Initial read
    }
    iterator(const iterator&) = default;
    ~iterator() = default;

    friend bool operator==(const iterator& it, sentinel) noexcept {
      // The iterator stopped when generator is done
      return it.gen_.Done();
    }

    iterator& operator++() {
      gen_.Next(send_value_);
      return *this;
    }

    void operator++(int) { operator++(); }

    reference operator*() const { return gen_.Get(); }

   private:
    Generator& gen_;
    SendType send_value_;
  };

  //
  // Generator
  //

  Generator(const std::coroutine_handle<promise_type>& handle) : handle_(handle) {}

  ~Generator() { handle_.destroy(); }

  explicit operator bool() const noexcept { return !handle_.done() && !consumed_; }

  ReturnType& Get() {
    consumed_ = true;
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    return *value_;
  }

  bool Next() { return Next(DEFAULT_SEND_VALUE); }

  template <typename T>
  bool Next(T&& send_value) {
    if (!handle_.done()) {
      handle_.promise().send_value_ = std::forward<T>(send_value);
      handle_();
      if (exception_ = handle_.promise().exception_; exception_) {
        // Has exception, and should return true as if there's new value (To let the exception rethrow by Get)
        consumed_ = false;
        value_ = {};
        return true;
      } else if (handle_.done()) {
        // The final suspend, no more to read
        consumed_ = true;
        value_ = {};
        return false;
      } else {
        // Has new value
        consumed_ = false;
        value_ = std::move(handle_.promise().return_value_);  // Move the value out of promise
        return true;
      }
    } else {
      // Done, no more to read
      consumed_ = true;
      value_ = {};
      return false;
    }
  }

  bool Done() const { return handle_.done(); }

  iterator begin() { return begin(DEFAULT_SEND_VALUE); }

  iterator begin(SendType&& send_value) {
    // Create iterator by custom send value
    return iterator(*this, std::forward<SendType>(send_value));
  }

  sentinel end() noexcept { return {}; }

 private:
  std::coroutine_handle<promise_type> handle_;
  bool consumed_ = true;
  std::optional<ReturnType> value_;
  std::exception_ptr exception_;
};

template <size_t STEP = 1>
Generator<size_t, size_t, STEP> counter(size_t max) {
  for (size_t i = 0; i < max;) {
    auto step = co_yield i;
    i += step;
  }
}

int main() {
  //
  // Usage 1
  //
  std::cout << "[+] Usage1" << std::endl;
  auto gen1 = counter(5);
  while (gen1.Next(2)) {
    std::cout << "main:" << gen1.Get() << std::endl;
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
[+] Usage1
main:0
main:2
main:4
[+] Usage2
main:0
main:3
main:6
[+] Usage3
main:0
main:1
main:2
*/
