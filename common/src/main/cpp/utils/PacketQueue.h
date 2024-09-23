#include <queue>
#include <pthread.h>
#include <libavcodec/avcodec.h>

class PacketQueue {
public:
    // 构造函数和析构函数
    PacketQueue() {
        pthread_mutex_init(&mutex, nullptr);
        pthread_cond_init(&cond, nullptr);
        flag = true;
    }

    ~PacketQueue() {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
        // 清理队列中的所有 AVPacket*
        while (!q.empty()) {
            AVPacket* pkt = q.front();
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            q.pop();
        }
    }

    /**
     * 出队
     */
    int pop(AVPacket* &t) {
        int ret = 0;
        pthread_mutex_lock(&mutex); // 锁上

        while (flag && q.empty()) {
            // 如果工作状态 并且 队列中没有数据，就等待
            pthread_cond_wait(&cond, &mutex);
        }

        if (!q.empty()) {
            AVPacket* pkt = q.front();
            q.pop();

            // 进行深拷贝
            t = av_packet_alloc();
            if (!t) {
                // 分配失败，处理错误
                ret = 0;
            } else {
                if (av_packet_ref(t, pkt) < 0) {
                    // 深拷贝失败，释放分配的包
                    av_packet_free(&t);
                    t = nullptr;
                    ret = 0;
                } else {
                    // 成功深拷贝，释放原始包
                    av_packet_unref(pkt);
                    av_packet_free(&pkt);
                    ret = 1;
                }
            }
        }

        pthread_mutex_unlock(&mutex); // 解锁

        return ret;
    }

    /**
     * 入队
     */
    void push(AVPacket* pkt) {
        pthread_mutex_lock(&mutex);
        q.push(pkt);
        pthread_cond_signal(&cond); // 通知等待的线程
        pthread_mutex_unlock(&mutex);
    }

    /**
     * 停止队列
     */
    void stop() {
        pthread_mutex_lock(&mutex);
        flag = false;
        pthread_cond_broadcast(&cond); // 通知所有等待的线程
        pthread_mutex_unlock(&mutex);
    }

private:
    std::queue<AVPacket*> q;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool flag; // 标识队列是否处于工作状态
};
