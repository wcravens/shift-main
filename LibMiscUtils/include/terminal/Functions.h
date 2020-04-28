#pragma once

#include "Common.h"

#include <functional>
#include <future>
#include <thread>
#include <utility>

using namespace std::chrono_literals;

namespace shift::terminal {

#if __cplusplus >= 201103L

template <typename _AsyncTaskTy>
void dots_awaiter(_AsyncTaskTy&& task)
{
    std::promise<void> pms;
    std::thread ater([](std::promise<void>& pms) {
        auto fut = pms.get_future();
        int i = 0;
        while (fut.wait_for(1500ms) != std::future_status::ready) {
            if (++i % 4) {
                cout << '.' << flush;
            } else {
                cout << "\b\b\b   \b\b\b" << flush;
            }
        }
        // cout << endl;
    } // []{}
        ,
        std::ref(pms));

    std::forward<_AsyncTaskTy>(task)(); // do and wait task
    // task done
    pms.set_value();
    ater.join();
}

#else // __cplusplus >= 201103L

template <typename _AsyncTaskTy>
void dots_awaiter(_AsyncTaskTy task)
{
    cout << "..." << endl;
    task();
}

#endif // __cplusplus >= 201103L

} // shift::terminal
