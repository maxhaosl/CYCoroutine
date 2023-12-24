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

#ifndef __CY_MANUE_EXECUTOR_CORO_HPP__
#define __CY_MANUE_EXECUTOR_CORO_HPP__

#include "CYCoroutine/Executors/CYDerivableExecutor.hpp"
#include "CYCoroutine/Threads/CYCacheLine.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>

CYCOROUTINE_NAMESPACE_BEGIN

class CYCOROUTINE_API alignas(CACHE_LINE_ALIGNMENT) CYManualExecutor final : public CYDerivableExecutor<CYManualExecutor>
{
public:
    CYManualExecutor();

    void Enqueue(CYTask task) override;
    void Enqueue(std::span<CYTask> tasks) override;

    int MaxConcurrencyLevel() const noexcept override;

    void ShutDown() override;
    bool ShutdownRequested() const override;

    size_t size() const;
    bool Empty() const;

    size_t Clear();

    //////////////////////////////////////////////////////////////////////////
    bool LoopOnce();
    bool LoopOnceFor(std::chrono::milliseconds nMaxWaitTime);

    template<class CLOCK_TYPE, class DURATION_TYPE>
    bool LoopOnceUntil(std::chrono::time_point<CLOCK_TYPE, DURATION_TYPE> timeoutTime)
    {
        return LoopUntilImpl(1, ToSystemTimePoint(timeoutTime));
    }

    size_t Loop(size_t nMaxCount);
    size_t LoopFor(size_t nMaxCount, std::chrono::milliseconds nMaxWaitTime);

    template<class CLOCK_TYPE, class DURATION_TYPE>
    size_t LoopUntil(size_t nMaxCount, std::chrono::time_point<CLOCK_TYPE, DURATION_TYPE> timeoutTime)
    {
        return LoopUntilImpl(nMaxCount, ToSystemTimePoint(timeoutTime));
    }

    //////////////////////////////////////////////////////////////////////////
    void WaitForTask();
    bool WaitForTaskFor(std::chrono::milliseconds nMaxWaitTime);

    template<class CLOCK_TYPE, class DURATION_TYPE>
    bool WaitForTaskUntil(std::chrono::time_point<CLOCK_TYPE, DURATION_TYPE> timeoutTime)
    {
        return WaitForTasksImpl(1, ToSystemTimePoint(timeoutTime)) == 1;
    }

    void WaitForTasks(size_t nCount);
    size_t WaitForTasksFor(size_t nCount, std::chrono::milliseconds nMaxWaitTime);

    template<class CLOCK_TYPE, class DURATION_TYPE>
    size_t WaitForTasksUntil(size_t nCount, std::chrono::time_point<CLOCK_TYPE, DURATION_TYPE> timeoutTime)
    {
        return WaitForTasksImpl(nCount, ToSystemTimePoint(timeoutTime));
    }

private:
    template<class CLOCK_TYPE, class DURATION_TYPE>
    static std::chrono::system_clock::time_point ToSystemTimePoint(std::chrono::time_point<CLOCK_TYPE, DURATION_TYPE> TimePoint) noexcept(noexcept(CLOCK_TYPE::now()))
    {
        const auto srcNow = CLOCK_TYPE::now();
        const auto dstNow = std::chrono::system_clock::now();
        return dstNow + std::chrono::duration_cast<std::chrono::milliseconds>(TimePoint - srcNow);
    }

    static std::chrono::system_clock::time_point TimePointFromNow(std::chrono::milliseconds ms) noexcept
    {
        return std::chrono::system_clock::now() + ms;
    }

    size_t LoopImpl(size_t nMaxCount);
    size_t LoopUntilImpl(size_t nMaxCount, std::chrono::time_point<std::chrono::system_clock> deadline);

    void WaitForTasksImpl(size_t nCount);
    size_t WaitForTasksImpl(size_t nCount, std::chrono::time_point<std::chrono::system_clock> deadline);

private:
    bool m_bAbort;
    mutable std::mutex m_lock;
    std::deque<CYTask> m_lstTasks;
    std::condition_variable m_condition;
    std::atomic_bool m_bAtomicAbort;

};
CYCOROUTINE_NAMESPACE_END

#endif //__CY_MANUE_EXECUTOR_CORO_HPP__
