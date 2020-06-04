#include <mutex>
#include <optional>
#include <vector>

//! Cross-thread queues

template <typename T>
class MutexQueue {
    std::mutex mutex;
    std::vector<T> vector;
public:
    void push(T t) {
        std::scoped_lock lock(this->mutex);
        this->vector.push_back(t);
    }

    std::optional<T> pop() {
        std::scoped_lock lock(this->mutex);
        if (this->vector.empty()) return std::nullopt;
        T t = this->vector.front();
        this->vector.erase(this->vector.begin());
        return t;
    }

    template <typename F>
    std::optional<T> remove(F f) {
        std::scoped_lock lock(this->mutex);
        for (auto i = this->vector.begin(); i != this->vector.end(); ++i) {
            if (f(*i)) {
                T t = *i;
                this->vector.erase(i);
                return t;
            }
        }
        return std::nullopt;
    }
};

struct Request {
    pthread_t thread_id;
    int uid;
    client_packet packet;
};

struct Response {
    pthread_t thread_id;
    server_packet packet;
};
