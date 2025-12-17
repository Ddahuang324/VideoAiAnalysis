#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class ThreadSafetyQueue {
public:
    // 构造与析构
    explicit ThreadSafetyQueue(size_t maxSize = 0);
    ~ThreadSafetyQueue() = default;

    // 禁止拷贝与赋值,允许移动
    ThreadSafetyQueue(const ThreadSafetyQueue&) = delete;
    ThreadSafetyQueue& operator=(const ThreadSafetyQueue&) = delete;

    ThreadSafetyQueue(ThreadSafetyQueue&&) noexcept = default;
    ThreadSafetyQueue& operator=(ThreadSafetyQueue&&) noexcept = default;

    // 核心操作
    bool push(T value, std::chrono::milliseconds timeout);
    bool pop(T& value, std::chrono::milliseconds timeout);
    std::optional<T> try_pop();

    size_t size() const;
    bool empty() const;

    // 控制操作
    void clear();
    void stop();
    void reset();

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_notEmpty;
    std::condition_variable m_notFull;
    size_t m_maxSize;
    bool m_stopped{false};
};

// ============================================================================
// 实现部分 (模板类必须在头文件中实现)
// ============================================================================

template <typename T>
ThreadSafetyQueue<T>::ThreadSafetyQueue(size_t maxSize) : m_maxSize(maxSize), m_stopped(false) {}

template <typename T>
bool ThreadSafetyQueue<T>::push(T value, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_maxSize > 0) {  // 是否有最大容量限制
        if (!m_notFull.wait_for(lock, timeout, [this]() {
                return m_queue.size() < m_maxSize || m_stopped;
            })) {          // 有空间,则唤醒,继续推送,如果队列停止,则唤醒,允许线程退出等待
                           // 避免死锁,如果两者都为false(队列满且没有停止)则继续等待
            return false;  // 超时
        }
    }

    if (m_stopped) {
        return false;  // 队列已停止,拒绝推送
    }

    m_queue.push(std::move(value));
    m_notEmpty.notify_one();  // 通知有新数据可用
    return true;
}

template <typename T>
bool ThreadSafetyQueue<T>::pop(T& value, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (!m_notEmpty.wait_for(lock, timeout, [this]() {
            return !m_queue.empty() || m_stopped;
        })) {          // 有数据可用,则唤醒,继续弹出,如果队列停止,则唤醒,允许线程退出等待
                       // 避免死锁,如果两者都为false(队列空且没有停止)则继续等待
        return false;  // 超时
    }

    if (m_stopped && m_queue.empty()) {
        return false;  // 队列已停止且为空,拒绝弹出
    }

    value = std::move(m_queue.front());
    m_queue.pop();
    if (m_maxSize > 0) {
        m_notFull.notify_one();  // 通知有空间可用
    }
    return true;
}

template <typename T>
std::optional<T> ThreadSafetyQueue<T>::try_pop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
        return std::nullopt;
    }
    T value = std::move(m_queue.front());
    m_queue.pop();
    if (m_maxSize > 0) {
        m_notFull.notify_one();
    }
    return value;
}

template <typename T>
size_t ThreadSafetyQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

template <typename T>
bool ThreadSafetyQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

template <typename T>
void ThreadSafetyQueue<T>::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<T> empty;
    std::swap(m_queue, empty);
}

template <typename T>
void ThreadSafetyQueue<T>::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stopped = true;
    m_notEmpty.notify_all();
    m_notFull.notify_all();
}

template <typename T>
void ThreadSafetyQueue<T>::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stopped = false;
    std::queue<T> empty;
    std::swap(m_queue, empty);
}
