
#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#include <queue>
#include <thread>

template<typename T>
class PacketQueue {
    typedef void (*ReleaseCallback)(T &);

private:
    std::mutex m_mutex;     // 互斥锁 防止多个线程同时修改锁
    std::condition_variable m_cond; // 条件变量 用于多线程的同步，通知队列中的变化
    std::queue<T> m_queue;   // 队列
    bool m_running; // 控制队列是否运行的标志位
    ReleaseCallback releaseCallback;  // 回调函数，用于在特定条件下释放资源

public:
    void push(T new_value) {
        std::lock_guard<std::mutex> lock(m_mutex); // 加锁，确保线程安全
        if (m_running) {   // 只有在队列处于运行状态时才允许添加数据
            m_queue.push(new_value); // 将新数据推入队列
            m_cond.notify_one();  // 通知一个等待中的线程，队列中有新的数据可处理
        }
    }

    int pop(T &value) {
        int ret = 0;  // 用于返回操作结果，0 表示失败，1 表示成功
        std::unique_lock<std::mutex> lock(m_mutex);  // 加锁，确保队列的线程安全
        if (!m_running) {
            return false;
        }
        if (!m_queue.empty()) {
            value = m_queue.front();  // 取出队列的第一个元素
            m_queue.pop(); // 弹出该元素
            return 1;
        }
        return ret;
    }


    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        int size = m_queue.size();
        for (int i = 0; i < size; ++i) {
            T value = m_queue.front();
            releaseCallback(value);
            m_queue.pop();
        }
    }

    void setRunning(bool run) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = run;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    int size() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<int>(m_queue.size());
    }

    void setReleaseCallback(ReleaseCallback callback) {
        releaseCallback = callback;
    }



};

#endif // PACKET_QUEUE_H
