/* Author: lipixun
 * Created Time : 2024-06-18 13:54:08
 *
 * File Name: step10.cpp
 * Description:
 *
 *  Step10: A simple async scheduler
 *
 */

#include <chrono>
#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>
#include <queue>
#include <semaphore>
#include <thread>

using namespace std::chrono_literals;

using spawn_function = std::function<void(std::coroutine_handle<>)>;

template <typename T>
class awaitable {
 public:
  //
  // Promise type
  //

  class promise_type {
   public:
    awaitable get_return_object() {
      // Create a new awaitable object. It's awaitable's responsible to destroy handle
      return awaitable(std::coroutine_handle<promise_type>::from_promise(*this));
    }

    std::suspend_always initial_suspend() noexcept { return {}; }

    std::suspend_always final_suspend() noexcept {
      if (spawn_ && caller_handle_) {
        // The callee is completed, and we should schedule the await_resume of caller. (by calling caller_handle())
        spawn_(caller_handle_);
      }
      return {};
    }

    void unhandled_exception() {
      // Store exception
      exception_ = std::current_exception();
    }

    template <std::convertible_to<T> U>
    void return_value(U&& value) {
      // Store return value
      value_ = std::forward<U>(value);
    }

    void set_caller(std::coroutine_handle<> handle) {
      // Store the caller of current coroutine.
      // This function may be called multiple times (one time per co_await from caller)
      caller_handle_ = handle;
    }

    //
    // Get & set spawn function. The handle only by ran when spawn function is set.
    // The spawn function may be changed at any time current coroutine is suspended.
    // That means the current coroutine or the caller's coroutine may resume at different thread.
    //
    spawn_function get_spawn() { return spawn_; }

    void set_spawn(spawn_function f) {
      spawn_ = f;
      if (f && !init_spawned_) {
        init_spawned_ = true;
        // Schedule current coroutine to continue from initial_suspend
        f(std::coroutine_handle<promise_type>::from_promise(*this));
      }
    }

   private:
    friend awaitable;

    // Check if current coroutine has been resumed after initial suspend.
    bool init_spawned_ = false;
    // The spawn function
    spawn_function spawn_;
    // Store the return value & exception
    std::optional<T> value_;
    std::exception_ptr exception_;
    // The caller coroutine handle
    std::coroutine_handle<> caller_handle_;
  };

  //
  // Awaitable
  //

  ~awaitable() noexcept {
    // Destroy the handle
    handle_.destroy();
  }

  awaitable(const awaitable&) = delete;  // Cannot copy awaitable

  awaitable(awaitable&& other) noexcept : handle_(std::exchange(other.handle_, {})) {
    // NOTE: This is tricky. A moved awaitable is not available any more but I didn't handle the case.
  }

  constexpr bool await_ready() const noexcept { return false; }

  void await_suspend(std::coroutine_handle<promise_type> h) {
    // Progragate spawn function from caller to callee and set caller. We can then call spawn_(caller_handle) to resume
    // the caller later.
    // NOTE:
    //  [handle_] is the [callee]'s coroutine_handle
    //  [h] is the [caller]'s coroutin_handle
    auto& promise = handle_.promise();
    promise.set_caller(h);
    promise.set_spawn(h.promise().get_spawn());
  }

  T& await_resume() noexcept { return value(); }

  bool done() noexcept { return handle_.done(); }

  T& value() noexcept {
    auto& promise = handle_.promise();
    if (promise.exception_) {
      std::rethrow_exception(promise.exception_);
    }
    return *promise.value_;
  }

  spawn_function get_spawn() { return handle_.promise().get_spawn(); }

  void set_spawn(spawn_function f) { return handle_.promise().set_spawn(f); }

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
    return [h = this->handle_] {
      // Add the handle to scheduler to continue the coroutine.
      h.promise().get_spawn()(h);
    };
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
  auto value = co_await await1 + co_await await2 + co_await await3 + co_await await4;
  std::cout << "[complex_func] Done\n";
  co_return value;
}

//
// A simple scheduler
//

template <typename T>
T spawn(awaitable<T>&& task) {
  //
  // Handle queue and spawn function
  //
  std::mutex m;
  std::counting_semaphore queue_size{0};
  std::queue<std::coroutine_handle<>> h_queue;
  spawn_function spawn = [&m, &queue_size, &h_queue](std::coroutine_handle<> h) {
    {
      std::lock_guard lock(m);
      h_queue.emplace(h);
    }
    queue_size.release();
  };

  // Set spawn function (And will actually run the function)
  task.set_spawn(spawn);

  //
  // Run handles (Yes, we can implement it in a multi-thread way, it's quite easy to do that)
  //
  //  For an industrial implementation, we must consider the following things:
  //
  //    1. Cancellation
  //    2. Effective way to enqueue / dequeue
  //    3. Dead lock detection
  //    4. ...
  //
  //  Remember: this is just a very simple prototype.
  //
  while (!task.done()) {
    // When all coroutines of current scheduler are suspend. The scheduler should wait for any coroutine becoming ready
    // to continue (e.g. data received from a socket, user input a string, ...).
    queue_size.acquire();
    // Run handles
    std::coroutine_handle<> handle;
    {
      std::lock_guard lock(m);
      handle = h_queue.front();
      h_queue.pop();
    }
    handle();
  }

  return task.value();
}

int main() {
  // Spawn the complex function and wait for it
  auto result = spawn(complex_func());
  std::cout << "[main] result:" << result << std::endl;
  return 0;
}

/*
Outputs:
[complex_func] Run
[complex_func] Wait
[simple_func] Run
[mock_heavy_func] Run
[mock_heavy_func] Wait in thread
[mock_heavy_func] Awake in thread
[simple_func] Complete
[simple_func] Run
[mock_heavy_func] Run
[mock_heavy_func] Wait in thread
[mock_heavy_func] Awake in thread
[simple_func] Complete
[simple_func] Run
[mock_heavy_func] Run
[mock_heavy_func] Wait in thread
[mock_heavy_func] Awake in thread
[simple_func] Complete
[simple_func] Run
[mock_heavy_func] Run
[mock_heavy_func] Wait in thread
[mock_heavy_func] Awake in thread
[simple_func] Complete
[complex_func] Done
[main] result:3604
*/
