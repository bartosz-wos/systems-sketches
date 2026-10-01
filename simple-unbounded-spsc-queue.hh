#pragma once

#include <atomic>
#include <thread>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <new>

template <typename T>
struct DataWrapper
{
    T data{ };
    bool is_last_chunk{ false };
};

template <typename T, typename Callback>
class SPSC{
public:
    explicit SPSC(Callback callback)
    : callback_{ std::move(callback) }
    , head_{ new Node() }
    , tail_{ head_.load(std::memory_order_relaxed) }
    , stop_requested_{ false }
    {
        consumer_thread_ = std::thread(&SPSC::Consume, this);
    }

    ~SPSC(){
        stop_requested_.store(true, std::memory_order_release);

        if(consumer_thread_.joinable()){
            consumer_thread_.join();
        }

        Node* current = head_.load(std::memory_order_relaxed);
        while(current != nullptr){
            Node* next = current->next.load(std::memory_order_relaxed);
            delete current;
            current = next;
        }
    }

    SPSC(const SPSC&) = delete;
    SPSC& operator=(const SPSC&) = delete;
    SPSC(SPSC&&) = delete;
    SPSC& operator=(SPSC&&) = delete;

    void PushWork(const DataWrapper<T>& wrapper){
        Node* new_node = new Node(wrapper);
        tail_->next.store(new_node, std::memory_order_release);
        tail_ = new_node;
    }

    void PushWork(DataWrapper<T>&& wrapper){
        Node* new_node = new Node(std::move(wrapper));
        tail_->next.store(new_node, std::memory_order_release);
        tail_ = new_node;
    }

private:
    struct Node{
        DataWrapper<T> data;
        std::atomic<Node*> next{ nullptr };

        Node() = default;
        explicit Node(const DataWrapper<T>& d) : data{ d }, next{ nullptr } {}
        explicit Node(DataWrapper<T>&& d) : data{ std::move(d) }, next{ nullptr } {}
    };

    static inline void cpu_pause(){
        std::this_thread::yield();
    }

    void Invoke(DataWrapper<T>& wrapper){
        if constexpr(std::is_invocable_v<Callback&, const DataWrapper<T>&>){
            (*callback_)(wrapper);
        }else if constexpr(std::is_invocable_v<Callback&, DataWrapper<T>&>){
            (*callback_)(wrapper);
        }else if constexpr(std::is_invocable_v<Callback&, DataWrapper<T>>){
            (*callback_)(wrapper);
        }else if constexpr(std::is_invocable_v<Callback&, const T&>){
            (*callback_)(wrapper.data);
        }else if constexpr(std::is_invocable_v<Callback&, T&>){
            (*callback_)(wrapper.data);
        }else if constexpr(std::is_invocable_v<Callback&, T>){
            (*callback_)(wrapper.data);
        }else if constexpr(std::is_invocable_v<Callback&>){
            (*callback_)();
        }
    }

    void Consume(){
        while(true){
            Node* current_head = head_.load(std::memory_order_relaxed);
            Node* next = current_head->next.load(std::memory_order_acquire);

            if(next != nullptr){
                bool is_last = next->data.is_last_chunk;
                Invoke(next->data);

                head_.store(next, std::memory_order_relaxed);
                delete current_head;

                if(is_last){
                    break;
                }
            }else if(stop_requested_.load(std::memory_order_acquire)){
                break;
            }else{
                cpu_pause();
            }
        }
    }

    std::optional<Callback> callback_;

    alignas(64) std::atomic<Node*> head_{ nullptr };
    alignas(64) Node* tail_{ nullptr };
    alignas(64) std::atomic<bool> stop_requested_{ false };

    std::thread consumer_thread_;
};
