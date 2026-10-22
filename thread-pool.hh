#pragma once

class ThreadPool{
private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
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

};
