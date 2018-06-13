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

    LamportTime receive_event(LamportTime received_time) {
        RECEIVE_EVENT:

        auto cur = get_time();

        // If received time is old, do nothing.
        if (received_time < cur) {
            return cur;
        }

        // Ensure that local timer is at least one ahead.
        if (!time_.compare_exchange_strong(cur, received_time + 1)) {
            goto RECEIVE_EVENT;
        }

        return received_time + 1;
    }

private:
    std::atomic<LamportTime> time_;
};
