#include <atomic>

class LamportClock {
public:
    typedef unsigned int LamportTime;

    LamportClock() : time_(0) {}

    LamportTime get_time() const {
        return time_.load();
    }

    LamportTime send_event() {
        return time_.fetch_add(1);
    }

    void receive_event(LamportTime received_time) {
        auto cur = get_time();
        time_.store(std::max(cur, received_time) + 1);
    }

private:
    std::atomic<LamportTime> time_;
};
