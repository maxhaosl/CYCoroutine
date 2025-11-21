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

#ifndef __CY_BINARY_SEMAPHORE_HPP__
#define __CY_BINARY_SEMAPHORE_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"

#include <mutex>
#include <condition_variable>
#include <chrono>

#if defined(__APPLE__) && defined(__clang__)

#else
#include <semaphore>
#endif

CYCOROUTINE_NAMESPACE_BEGIN

class BinarySemaphore
{
public:
    explicit BinarySemaphore(ptrdiff_t desired)
        : flag(desired > 0 ? 1 : 0)
    {
    }

#undef max
    static constexpr ptrdiff_t max() noexcept
    {
        return 1;
    }

    void acquire()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return flag == 1; });
        flag = 0;
    }

    bool try_acquire()
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (flag == 1)
        {
            flag = 0;
            return true;
        }
        return false;
    }

    template<class Rep, class Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period>& dur)
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (!cv.wait_for(lock, dur, [&] { return flag == 1; }))
            return false;

        flag = 0;
        return true;
    }

    template<class Clock, class Duration>
    bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& tp)
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (!cv.wait_until(lock, tp, [&] { return flag == 1; }))
            return false;

        flag = 0;
        return true;
    }

    // release(update) —  C++20 std::binary_semaphore
    void release(ptrdiff_t update = 1)
    {
        if (update < 1)
            return;

        std::unique_lock<std::mutex> lock(mtx);

        const bool was_zero = (flag == 0);
        flag = 1;

        if (was_zero)
            cv.notify_one();
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    int flag = 0;  // 0 = unavailable, 1 = available
};

#if defined(__APPLE__)
using cy_binary_semaphore = BinarySemaphore;
#else
using cy_binary_semaphore = std::binary_semaphore;
#endif

CYCOROUTINE_NAMESPACE_END

#endif //__CY_BINARY_SEMAPHORE_HPP__