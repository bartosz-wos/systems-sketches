#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <future>
#include <type_traits>
#include <stdexcept>
#include <concepts>

class ThreadPool{
private:
  std::vector<std::thread> workers_{};
  std::queue<std::function<void()>> tasks_{};
  std::mutex mutex_{};
  std::condition_variable cv_{};
  bool stop_{ false };

public:
  explicit ThreadPool(std::size_t threads = std::thread::hardware_concurrency()){
    if(threads == 0)
      threads = 1;

    workers_.reserve(threads);
    for(std::size_t i = 0; i < threads; ++i){
      workers_.emplace_back([this]{
          while(true){
            std::function<void()> task;

            {
              std::unique_lock<std::mutex> lock{mutex_};
              cv_.wait(lock, [this]{
                  return stop_ || !tasks_.empty();
              });

              if(stop_ && tasks_.empty()){
                return;
              }

              task = std::move(tasks_.front());
              tasks_.pop();
            }

            task();
          }
      });
    }
  }

  ~ThreadPool(){
    {
      std::unique_lock<std::mutex> lock{mutex_};
      stop_ = true;
    }

    cv_.notify_all();
    for(std::thread& t : workers_){
      if(t.joinable()){
        t.join();
      }
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  template<typename F, typename... Args>
  requires std::invocable<std::decay_t<F>, std::decay_t<Args>...>
  auto push(F&& f, Args&&... args){
    using return_type = std::invoke_result_t<F, Args...>;

    auto promise = std::make_shared<std::promise<return_type>>();
    std::future<return_type> future = promise->get_future();

    auto task = [ promise
                , func = std::decay_t<F>(std::forward<F>(f))
                , args_list = std::make_tuple(std::forward<Args>(args)...)
                ]
                () mutable{
                  try{
                    if constexpr(std::is_void_v<return_type>){
                      std::apply(std::move(func), std::move(args_list));
                      promise->set_value();
                    }else{
                      promise->set_value(std::apply(std::move(func), std::move(args_list)));
                    }
                  }catch(...){
                    promise->set_exception(std::current_exception());
                  }
                };

    {
      std::unique_lock<std::mutex> lock{mutex_};
      if(stop_){
        throw std::runtime_error("Thread pool stopped, but there was an attempt to push a task");
      }

      tasks_.emplace(std::move(task));
    }

    cv_.notify_one();
    return future;
  }

};
