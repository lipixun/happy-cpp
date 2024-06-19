/* Author: lipixun
 * Created Time : 2024-06-18 13:54:08
 *
 * File Name: step10.cpp
 * Description:
 *
 *  Step10: A simple async queue
 *
 */

#include <chrono>
#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>
#include <thread>

using namespace std::chrono_literals;

std::vector<std::coroutine_handle<>> waiting_handles;

template <typename T>
class awaitable {
 public:
  //
  // Promise
  //
  class promise_type {
   public:
    awaitable get_return_object() {
      // Create a new awaitable object. It's awaitable's responsible to destroy handle
      return awaitable(std::coroutine_handle<promise_type>::from_promise(*this));
    }

    std::suspend_never initial_suspend() noexcept { return {}; }

    std::suspend_always final_suspend() noexcept {
      if (caller_handle_) {
        // The callee is completed. And we should schedule the await_resume of caller. (by calling caller_handle())
        waiting_handles.emplace_back(caller_handle_);
      }
      return {};
    }

    void unhandled_exception() { exception_ = std::current_exception(); }

    template <std::convertible_to<T> U>
    void return_value(U&& value) {
      value_ = std::forward<U>(value);
    }

    void set_caller(std::coroutine_handle<> handle) { caller_handle_ = handle; }

   private:
    friend awaitable;
    // Store the return value & exception
    std::optional<T> value_;
    std::exception_ptr exception_;
    // The caller coroutine handle
    std::coroutine_handle<> caller_handle_;
  };

  ~awaitable() noexcept { handle_.destroy(); }

  awaitable(const awaitable&) = delete;  // Cannot copy awaitable

  awaitable(awaitable&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}

  //
  // Awaiter
  //
  constexpr bool await_ready() const noexcept { return false; }

  void await_suspend(std::coroutine_handle<> h) {
    // The caller coroutine handle
    handle_.promise().set_caller(h);
  }

  T& await_resume() noexcept { return value(); }

  //
  // For normal caller
  //

  bool done() noexcept { return handle_.done(); }

  T& value() noexcept {
    auto& promise = handle_.promise();
    if (promise.exception_) {
      std::rethrow_exception(promise.exception_);
    }
    return *promise.value_;
  }

 private:
  friend promise_type;

  explicit awaitable(std::coroutine_handle<promise_type> handle) noexcept : handle_(handle) {}

  // The callee corouting handle
  std::coroutine_handle<promise_type> handle_;
};

template <typename T>
class await_callback {
 public:
  await_callback() {}

  constexpr bool await_ready() const noexcept { return false; }

  bool await_suspend(std::coroutine_handle<typename awaitable<T>::promise_type> h) {
    handle_ = h;
    return false;
  }

  auto await_resume() noexcept {
    return [h = this->handle_] { h(); };
  }

 private:
  std::coroutine_handle<typename awaitable<T>::promise_type> handle_;
};

awaitable<int> mock_heavy_func(int x) {
  std::cout << "[mock_heavy_func] Run\n";
  auto callback = co_await await_callback<int>();  // Will not suspend
  // Schedule a thread and sleep for sometime.
  std::thread thread([x, callback] {
    std::cout << "[mock_heavy_func] Wait in thread\n";
    std::this_thread::sleep_for(x * 1ms);
    std::cout << "[mock_heavy_func] Awake in thread\n";
    callback();  // Tell current coroutine to continue
  });
  thread.detach();
  co_await std::suspend_always{};
  // Will reach here after callback is called
  co_return x;
}

awaitable<int> simple_func(int x) {
  std::cout << "[simple_func] Run\n";
  auto value = co_await mock_heavy_func(x);
  std::cout << "[simple_func] Complete\n";
  co_return value + 1;
}

awaitable<int> complex_func() {
  std::cout << "[complex_func] Run\n";
  auto await1 = simple_func(100);
  auto await2 = simple_func(500);
  auto await3 = simple_func(1000);
  auto await4 = simple_func(2000);
  std::cout << "[complex_func] Wait\n";
  co_return co_await await1 + co_await await2 + co_await await3 + co_await await4;
}

int main() {
  std::cout << "[main] Run\n";
  auto await = complex_func();
  std::cout << "[main] After complex_func\n";

  while (true) {
    std::this_thread::sleep_for(10ms);
    if (await.done()) {
      // Complete. Result of complex_func should be 3604
      std::cout << "[main] complex_func done: " << await.value() << std::endl;
      break;
    }
    // A prototype of 'async executor' / 'task scheduler' / ....
    // All handles stored in the waiting_handles are waiting for being executed.
    // This simple implementation doesn't need lock since all coroutines are actually run in the main thread.
    while (waiting_handles.size() > 0) {
      auto handle = waiting_handles.back();
      waiting_handles.pop_back();
      handle();
    }
  }

  return 0;
}

/*
Outputs:
[main] Run
[complex_func] Run
[simple_func] Run
[mock_heavy_func] Run
[simple_func] Run
[mock_heavy_func] Run
[mock_heavy_func] Wait in thread
[simple_func] Run
[mock_heavy_func] Run
[mock_heavy_func] Wait in thread
[mock_heavy_func] Wait in thread
[simple_func] Run
[mock_heavy_func] Run
[complex_func] Wait
[main] After complex_func
[mock_heavy_func] Wait in thread
[mock_heavy_func] Awake in thread
[simple_func] Complete
[mock_heavy_func] Awake in thread
[simple_func] Complete
[mock_heavy_func] Awake in thread
[simple_func] Complete
[mock_heavy_func] Awake in thread
[simple_func] Complete
[main] complex_func done: 3604
*/
