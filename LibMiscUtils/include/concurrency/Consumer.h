#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <utility>

using namespace std::chrono_literals;

namespace shift::concurrency {

template <typename _FutRes, typename _PredFn>
auto quitOrContinueConsumerThread(std::future<_FutRes>& quitFlagFut, std::condition_variable& cv, std::unique_lock<std::mutex>& lockForCV, _PredFn&& pred) -> bool
{
    cv.wait(lockForCV, [&quitFlagFut, &pred] {
        if (quitFlagFut.wait_for(0ms) == std::future_status::ready) {
            return true; // let current thread unblock from the condition variable and moves forward...
        }
        return std::forward<_PredFn>(pred)();
    });

    return quitFlagFut.wait_for(0ms) == std::future_status::ready; // quit?
}

template <typename _PmsVal>
void notifyConsumerThreadToQuit(std::promise<_PmsVal>& quitFlagPms, std::condition_variable& cv, std::thread& th)
{
    quitFlagPms.set_value(); // set the flag to prepare for terminating the thread
    cv.notify_all(); // notify so that the thread unblocks and moves forward
    if (th.joinable()) {
        th.join(); // wait for the thread returns/quits
    }
}

} // shift::concurrency
