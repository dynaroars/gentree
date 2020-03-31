#ifndef IGEN4_WORKQUEUE_H
#define IGEN4_WORKQUEUE_H

#include "common.h"

#include <algorithm>
#include <deque>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <boost/any.hpp>

namespace igen {

class WorkQueue {
private:
    struct Item;
    using Fn = std::function<void(int, int)>; // [](int thread_id, int item_id) {}

    struct WorkCtl : public intrusive_ref_base_mt<WorkCtl> {
        Fn fn;
        int n_finished = 0;
        int n_total;
        std::mutex mtx;
        std::condition_variable cv;

        WorkCtl(Fn fn, int tot) : fn(std::move(fn)), n_total(tot) {}

        bool done() { return n_finished == n_total; }
    };

    using PCtl = ptr<WorkCtl>;
    struct Item {
        int id;
        PCtl ctl;
    };

    using UniqueLock = std::unique_lock<std::mutex>;
public:
    WorkQueue() = default;

    WorkQueue(const WorkQueue &) = delete;

    WorkQueue(WorkQueue &&) = delete;

    ~WorkQueue() { stop(); }

    void init(int n_threads);

    void start();

    void stop();

    void run_batch_job(const Fn &fn, int num);

protected:
    void _add_nolock(const Item &item);

    void _process(int tid, const Item &item);

    void _run(int tid);

protected:
    int n_threads_ = 0;
    int n_idle_ = 0;
    bool running_ = false;
    std::deque<Item> deque_;

    std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<std::thread> threads;
};

}

#endif //IGEN4_WORKQUEUE_H
