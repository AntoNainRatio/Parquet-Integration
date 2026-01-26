#include <mutex>
#include <queue>


struct RowGroupJob {
    int start;
    int end;
    int job_id;
};

class JobQueue {
public:
    void push(const RowGroupJob& job) {
        std::unique_lock<std::mutex> lock(m_);
        q_.push(job);
        cv_.notify_one();
    }

    bool pop(RowGroupJob& job) {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty() || finished_; });
        if (q_.empty()) return false;
        job = q_.front();
        q_.pop();
        return true;
    }

    void set_finished() {
        std::unique_lock<std::mutex> lock(m_);
        finished_ = true;
        cv_.notify_all();
    }

private:
    std::queue<RowGroupJob> q_;
    std::mutex m_;
    std::condition_variable cv_;
    bool finished_ = false;
};