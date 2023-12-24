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

#ifndef __CY_SHARED_RESULT_CORO_HPP__
#define __CY_SHARED_RESULT_CORO_HPP__

#include "CYCoroutine/Results/CYResult.hpp"
#include "CYCoroutine/Results/CYSharedResultAwaitable.hpp"
#include "CYCoroutine/Results/Impl/CYSharedResultState.hpp"

CYCOROUTINE_NAMESPACE_BEGIN
template<class TYPE>
class CYSharedResult
{
public:
    CYSharedResult() noexcept = default;
    virtual ~CYSharedResult() noexcept = default;

    CYSharedResult(const CYSharedResult& rhs) noexcept = default;
    CYSharedResult(CYSharedResult&& rhs) noexcept = default;

public:
    CYSharedResult(SharePtr<CYSharedResultState<TYPE>> state) noexcept
        : m_state(std::move(state))
    {
    }

    CYSharedResult(CYResult<TYPE> rhs)
    {
        if (!static_cast<bool>(rhs))
        {
            return;
        }

        auto CYResultState = CYSharedResultHelper::GetState(rhs);
        m_state = MakeShared<CYSharedResultState<TYPE>>(std::move(CYResultState));
        m_state->Share(std::static_pointer_cast<CYSharedResultStateBase>(m_state));
    }

    CYSharedResult& operator=(const CYSharedResult& rhs) noexcept
    {
        if (this != &rhs && m_state != rhs.m_state)
        {
            m_state = rhs.m_state;
        }

        return *this;
    }

    CYSharedResult& operator=(CYSharedResult&& rhs) noexcept
    {
        if (this != &rhs && m_state != rhs.m_state)
        {
            m_state = std::move(rhs.m_state);
        }

        return *this;
    }

    operator bool() const noexcept
    {
        return static_cast<bool>(m_state.get());
    }

public:
    EResultStatus Status() const
    {
        IfEmptyThrow("CYSharedResult::Status() - result is empty.");
        return m_state->Status();
    }

    void Wait()
    {
        IfEmptyThrow("CYSharedResult::wait() - result is empty.");
        m_state->Wait();
    }

    template<class duration_type, class ratio_type>
    EResultStatus WaitFor(std::chrono::duration<duration_type, ratio_type> duration)
    {
        IfEmptyThrow("CYSharedResult::WaitFor() - result is empty.");
        return m_state->WaitFor(duration);
    }

    template<class ClockType, class duration_type>
    EResultStatus WaitUntil(std::chrono::time_point<ClockType, duration_type> timeoutTime)
    {
        IfEmptyThrow("CYSharedResult::WaitUntil() - result is empty.");
        return m_state->WaitUntil(timeoutTime);
    }

    std::add_lvalue_reference_t<TYPE> Get()
    {
        IfEmptyThrow("CYSharedResult::get() - result is empty.");
        m_state->Wait();
        return m_state->Get();
    }

    auto operator co_await()
    {
        IfEmptyThrow("CYSharedResult::operator co_await() - result is empty.");
        return CYSharedAwaitable<TYPE> {m_state};
    }

    auto Resolve()
    {
        IfEmptyThrow("CYSharedResult::Resolve() - result is empty.");
        return CYSharedResolveAwaitable<TYPE> {m_state};
    }

private:
    SharePtr<CYSharedResultState<TYPE>> m_state;

    void IfEmptyThrow(const char* message) const
    {
        IfTrueThrow(!static_cast<bool>(m_state), AtoT(message));
    }
};
CYCOROUTINE_NAMESPACE_END

#endif //__CY_SHARED_RESULT_CORO_HPP__
