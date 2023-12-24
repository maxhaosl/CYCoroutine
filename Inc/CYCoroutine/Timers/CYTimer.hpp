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

#ifndef __CY_TIMER_CORO_HPP__
#define __CY_TIMER_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"

#include <atomic>
#include <chrono>
#include <memory>

CYCOROUTINE_NAMESPACE_BEGIN

using ClockType = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<ClockType>;
using milliseconds = std::chrono::milliseconds;

class CYExecutor;
class CYTimerQueue;
class CYCOROUTINE_API CYTimerStateBase : public std::enable_shared_from_this<CYTimerStateBase>
{
public:
    CYTimerStateBase(size_t nDueTime, size_t nFrequency, SharePtr<CYExecutor> ptrExecutor, WeakPtr<CYTimerQueue> ptrTimerQueue, bool isOneShot) noexcept;
    virtual ~CYTimerStateBase() noexcept = default;

public:
    virtual void Execute() = 0;

    void Fire();

    inline bool Expired(const TimePoint now) const noexcept
    {
        return m_tpDeadLine <= now;
    }

    inline TimePoint GetDeadLine() const noexcept
    {
        return m_tpDeadLine;
    }

    inline size_t GetFrequency() const noexcept
    {
        return m_nFrequency.load(std::memory_order_relaxed);
    }

    inline size_t GetDueTime() const noexcept
    {
        return m_nDueTime;
    }

    inline bool IsOneShot() const noexcept
    {
        return m_IsOneShot;
    }

    inline SharePtr<CYExecutor> GetExecutor() const noexcept
    {
        return m_ptrExecutor;
    }

    inline WeakPtr<CYTimerQueue> GetTimerQueue() const noexcept
    {
        return m_ptrTimerQueue;
    }

    inline void SetNewFrequency(size_t new_frequency) noexcept
    {
        m_nFrequency.store(new_frequency, std::memory_order_relaxed);
    }

    inline void Cancel() noexcept
    {
        m_bCancelled.store(true, std::memory_order_relaxed);
    }

    inline bool Cancelled() const noexcept
    {
        return m_bCancelled.load(std::memory_order_relaxed);
    }

private:
    inline static TimePoint MakeDeadLine(milliseconds nDiff) noexcept
    {
        return ClockType::now() + nDiff;
    }

private:
    const size_t m_nDueTime;
    const bool m_IsOneShot;

    TimePoint m_tpDeadLine;
    std::atomic_bool m_bCancelled;
    std::atomic_size_t m_nFrequency;

    const SharePtr<CYExecutor> m_ptrExecutor;
    const WeakPtr<CYTimerQueue> m_ptrTimerQueue;
};

//////////////////////////////////////////////////////////////////////////
template<class CALLABLE_TYPE>
class CYCOROUTINE_API CYTimerState final : public CYTimerStateBase
{
public:
    template<class GIVEN_CALLABLE_TYPE>
    CYTimerState(size_t nDueTime, size_t nFrequency, SharePtr<CYExecutor> ptrExecutor, WeakPtr<CYTimerQueue> ptrTimerQueue, bool isOneShot, GIVEN_CALLABLE_TYPE&& callable)
        : CYTimerStateBase(nDueTime, nFrequency, std::move(ptrExecutor), std::move(ptrTimerQueue), isOneShot)
        , m_callable(std::forward<GIVEN_CALLABLE_TYPE>(callable))
    {
    }

public:
    void Execute() override
    {
        if (Cancelled())
        {
            return;
        }

        m_callable();
    }

private:
    CALLABLE_TYPE m_callable;
};

//////////////////////////////////////////////////////////////////////////
class CYCOROUTINE_API CYTimer
{
public:
    CYTimer() noexcept = default;
    ~CYTimer() noexcept;

    CYTimer(CYTimer&& rhs) noexcept = default;
    CYTimer& operator=(CYTimer&& rhs) noexcept;

    CYTimer(SharePtr<CYTimerStateBase> ptrTimerImpl) noexcept;

public:
    void Cancel();

    milliseconds GetDueTime() const;
    SharePtr<CYExecutor> GetExecutor() const;
    WeakPtr<CYTimerQueue> GetTimerQueue() const;

    milliseconds GetFrequency() const;
    void SetFrequency(milliseconds nFrequency);

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_ptrState);
    }

private:
    CYTimer(const CYTimer&) = delete;
    CYTimer& operator=(const CYTimer&) = delete;

    void IfEmptyThrow(const char* pszErrorMessage) const;

private:
    SharePtr<CYTimerStateBase> m_ptrState;
};

CYCOROUTINE_NAMESPACE_END

#endif  // __CY_TIMER_CORO_HPP__
