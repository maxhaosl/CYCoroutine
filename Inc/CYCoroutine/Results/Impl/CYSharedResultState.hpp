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

#ifndef __CY_SHARED_RESULT_STATE_CORO_HPP__
#define __CY_SHARED_RESULT_STATE_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/Impl/CYResultState.hpp"
#include "CYCoroutine/Results/Impl/CYCountingSemaphore.hpp"

#include <atomic>
#include <semaphore>
#include <chrono>
#include <cassert>

CYCOROUTINE_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
template<class TYPE>
class CYResult;

struct CYSharedResultHelper
{
    template<class TYPE>
    static CYConsumerResultStatePtr<TYPE> GetState(CYResult<TYPE>& result) noexcept
    {
        return std::move(result.m_ptrState);
    }
};

//////////////////////////////////////////////////////////////////////////
struct CYCOROUTINE_API CYSharedAwaitContext
{
    CYSharedAwaitContext* next = nullptr;
    coroutine_handle<void> handleCaller;
};

class CYCOROUTINE_API CYSharedResultStateBase
{
public:
    virtual ~CYSharedResultStateBase() noexcept = default;

    virtual void OnResultFinished() noexcept = 0;

    EResultStatus Status() const noexcept;

    void Wait() noexcept;

    bool await(CYSharedAwaitContext& awaiter) noexcept;

    template<class duration_unit, class ratio>
    EResultStatus WaitFor(std::chrono::duration<duration_unit, ratio> duration)
    {
        const auto TimePoint = std::chrono::system_clock::now() + duration;
        return WaitUntil(TimePoint);
    }

    template<class clock, class duration>
    EResultStatus WaitUntil(const std::chrono::time_point<clock, duration>& timeoutTime)
    {
        while ((Status() == EResultStatus::STATUS_RESULT_IDLE) && (clock::now() < timeoutTime))
        {
            const auto res = m_semaphore.try_acquire_until(timeoutTime);
            (void)res;
        }

        return Status();
    }

protected:
    cy_atomic<EResultStatus> m_status{ EResultStatus::STATUS_RESULT_IDLE };
    std::atomic<CYSharedAwaitContext*> m_awaiters{ nullptr };
    cy_counting_semaphore<> m_semaphore{ 0 };

    static CYSharedAwaitContext* ResultReadyConstant() noexcept;
};

template<class TYPE>
class CYSharedResultState final : public CYSharedResultStateBase
{
public:
    CYSharedResultState(CYConsumerResultStatePtr<TYPE> CYResultState) noexcept
        : m_result_state(std::move(CYResultState))
    {
        assert(static_cast<bool>(m_result_state));
    }

    ~CYSharedResultState() noexcept
    {
        assert(static_cast<bool>(m_result_state));
        m_result_state->TryRewindConsumer();
        m_result_state.reset();
    }

    void Share(const SharePtr<CYSharedResultStateBase>& self) noexcept
    {
        assert(static_cast<bool>(m_result_state));
        m_result_state->Share(self);
    }

    std::add_lvalue_reference_t<TYPE> Get()
    {
        assert(static_cast<bool>(m_result_state));
        return m_result_state->GetRef();
    }

    void OnResultFinished() noexcept override
    {
        m_status.store(m_result_state->Status(), std::memory_order_release);
        m_status.notify_all();

        /* theoretically buggish, practically there's no way
           that we'll have more than max(ptrdiff_t) / 2 waiters.
           on 64 bits, that's 2^62 waiters, on 32 bits thats 2^30 waiters.
           memory will run out before enough tasks could be created to wait this synchronously
        */
        m_semaphore.release(m_semaphore.max() / 2);

        auto k_result_ready = ResultReadyConstant();
        auto awaiters = m_awaiters.exchange(k_result_ready, std::memory_order_acq_rel);

        CYSharedAwaitContext* current = awaiters;
        CYSharedAwaitContext* prev = nullptr, * next = nullptr;

        while (current != nullptr)
        {
            next = current->next;
            current->next = prev;
            prev = current;
            current = next;
        }

        awaiters = prev;

        while (awaiters != nullptr)
        {
            assert(static_cast<bool>(awaiters->handleCaller));
            auto handleCaller = awaiters->handleCaller;
            awaiters = awaiters->next;
            handleCaller();
        }
    }

private:
    CYConsumerResultStatePtr<TYPE> m_result_state;
};

CYCOROUTINE_NAMESPACE_END

#endif //__CY_SHARED_RESULT_STATE_CORO_HPP__