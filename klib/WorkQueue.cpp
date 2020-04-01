//
// Created by kh on 3/25/20.
//

#include "WorkQueue.h"

#include <csignal>

namespace igen {


void WorkQueue::init(int n_threads) {
    n_threads_ = (n_threads == 0 ? (int) std::thread::hardware_concurrency() : n_threads);
}

void WorkQueue::start() {
    CHECK(n_threads_ > 0 && threads.empty());
    running_ = true;
    for (int tid = 0; tid < n_threads_; ++tid) {
        threads.emplace_back(&WorkQueue::_run, this, tid);
    }
}

void WorkQueue::stop() {
    if (threads.empty()) return;

    {
        UniqueLock lck(mtx_);
        running_ = false;
        cv_.notify_all();
    }

    // wait for the thread(s) to be finished
    std::for_each(threads.begin(), threads.end(), [](std::thread &t) { t.join(); });
    threads.clear();
}

static void mask_sig() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGRTMIN + 1);
    sigaddset(&mask, SIGRTMIN + 2);
    sigaddset(&mask, SIGRTMIN + 3);

    pthread_sigmask(SIG_BLOCK, &mask, nullptr);
}

void WorkQueue::_run(int tid) {
    mask_sig();
    UniqueLock lck(mtx_);
    for (;;) {
        while (deque_.empty() && running_) {
            ++n_idle_;
            cv_.wait(lck);
            --n_idle_;
        }

        if (!deque_.empty()) {
            const Item item = deque_.front();
            deque_.pop_front();
            lck.unlock();
            _process(tid, item);
            lck.lock();
        }

        if (deque_.empty() && !running_) {
            break;
        }
    }
}

void WorkQueue::_add_nolock(const WorkQueue::Item &item) {
    // UniqueLock lck(mtx_);
    deque_.push_back(item);
    if (n_idle_ > 0) cv_.notify_one();
}

void WorkQueue::_process(int tid, const Item &item) {
    item.ctl->fn(tid, item.id);
    const auto &ctl = item.ctl;
    UniqueLock lock(ctl->mtx);
    ctl->n_finished++;
    if (ctl->done())
        ctl->cv.notify_one();
}

void WorkQueue::run_batch_job(const Fn &fn, int num) {
    CHECK(!threads.empty());
    PCtl ctl = new WorkCtl(fn, num);
    {
        UniqueLock lck(mtx_);
        for (int i = 0; i < num; ++i) _add_nolock({i, ctl});
    }

    UniqueLock lock(ctl->mtx);
    while (!ctl->done()) ctl->cv.wait(lock);
}

}