#ifndef _MEDIA_Q
#define _MEDIA_Q

#include <queue>
#include <mutex>
#include <condition_variable>

template<class T>
class mediaQueue {
    std::queue<T*> q;
    std::mutex m;
    std::condition_variable cv;
public:

    mediaQueue() {}

    int enq(T* elem) {
        if (nullptr == elem) {
            return -1;
        }
        std::unique_lock<std::mutex> lock(m);
        q.push(elem);
        cv.notify_one();
        return q.size();
    }

    bool deq(T** elem, bool fWait) {
        std::unique_lock<std::mutex> lock(m);
        if (fWait) {
            cv.wait(lock, [this]{return !(q.empty());} );
        }
        else if (q.empty()) {
            return false;
        }
        elem[0] = q.front();
        q.pop();
        return true;
    }

    int getQSize() {
        return q.size();
    }
};

#endif