/*
 * CYCoroutine License
 * -----------
 *
 * CYCoroutine is licensed under the terms of the MIT license reproduced below.
 * This means that CYCoroutine is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 *
 *
 * ===============================================================================
 *
 * Copyright (C) 2023-2024 ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ===============================================================================
 */
 /*
  * AUTHORS:  ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
  * VERSION:  1.0.0
  * PURPOSE:  A cross-platform efficient and stable Coroutine library.
  * CREATION: 2023.04.15
  * LCHANGE:  2023.04.15
  * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
  */

#ifndef __CY_CYCountingSemaphore_HPP__
#define __CY_CYCountingSemaphore_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"

#include <condition_variable>
#include <cstddef>
#include <limits>
#include <mutex>
#include <chrono>
#include <algorithm>

#if defined(__APPLE__) && defined(__clang__)
#elif defined(CYCOROUTINE_UNIX_OS)

#else
#include <semaphore>
#endif

CYCOROUTINE_NAMESPACE_BEGIN

// A portable CYCountingSemaphore compatible with C++20 std::CYCountingSemaphore
// Template parameter Max defines the maximum count (default: max(ptrdiff_t))
template<ptrdiff_t Max = std::numeric_limits<ptrdiff_t>::max()>
class CYCountingSemaphore
{
public:
    static_assert(Max > 0, "Max must be positive");

    using ptrdiff_type = ptrdiff_t;

#undef min
    // Construct with initial count (must be in [0, Max])
    explicit CYCountingSemaphore(ptrdiff_type desired = 0)
        : count_(std::max<ptrdiff_type>(0, std::min(desired, Max)))
    {
    }

    CYCountingSemaphore(const CYCountingSemaphore&) = delete;
    CYCountingSemaphore& operator=(const CYCountingSemaphore&) = delete;

    // maximum value
    static constexpr ptrdiff_type max() noexcept { return Max; }

    // acquire (blocking)
    void acquire()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&] { return count_ > 0; });
        --count_;
    }

    // try acquire (non-blocking)
    bool try_acquire() noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (count_ > 0)
        {
            --count_;
            return true;
        }
        return false;
    }

    // try acquire for duration
    template<class Rep, class Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!cv_.wait_for(lock, rel_time, [&] { return count_ > 0; }))
            return false;
        --count_;
        return true;
    }

    // try acquire until time_point
    template<class Clock, class Duration>
    bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!cv_.wait_until(lock, abs_time, [&] { return count_ > 0; }))
            return false;
        --count_;
        return true;
    }

    // release n permits (default 1)
    // increases count by at most (Max - count_), and notifies up to that many waiters
    void release(ptrdiff_type update = 1)
    {
        if (update <= 0) return; // ignore non-positive updates

        ptrdiff_type to_notify = 0;

        {
            std::lock_guard<std::mutex> lock(mtx_);
            // clamp the new count to [0, Max]
            ptrdiff_type old = count_;
            // compute new count in safe way
            ptrdiff_type available_space = Max - old;
            ptrdiff_type inc = (update <= available_space) ? update : available_space;
            count_ = old + inc;
            to_notify = inc;
        }

        // If there are more waiters than inc, we need to wake up inc of them.
        // We notify in a loop to allow up to 'to_notify' waiting threads to wake.
        // Using notify_one repeatedly is fine; spurious wakeups or race
        // will be handled by the waiting predicate.
        for (ptrdiff_type i = 0; i < to_notify; ++i)
        {
            cv_.notify_one();
        }
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    ptrdiff_type count_ = 0;
};

#if defined(__APPLE__)
template <ptrdiff_t Max = std::numeric_limits<ptrdiff_t>::max()>
using cy_counting_semaphore = CYCountingSemaphore<Max>;
#elif defined(CYCOROUTINE_UNIX_OS)
template <ptrdiff_t Max = std::numeric_limits<ptrdiff_t>::max()>
using cy_counting_semaphore = CYCountingSemaphore<Max>;
#else
#include <semaphore>
constexpr ptrdiff_t cy_native_semaphore_max = static_cast<ptrdiff_t>(std::numeric_limits<int>::max());

template <ptrdiff_t Max = std::numeric_limits<ptrdiff_t>::max()>
using cy_counting_semaphore = std::counting_semaphore<(Max < cy_native_semaphore_max) ? Max : cy_native_semaphore_max>;
#endif

CYCOROUTINE_NAMESPACE_END

#endif // __CY_CYCountingSemaphore_HPP__