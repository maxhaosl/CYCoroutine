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

#ifndef __CY_LAZY_RESULT_AWAITTABLE_CORO_HPP__
#define __CY_LAZY_RESULT_AWAITTABLE_CORO_HPP__

#include "CYCoroutine/Results/Impl/CYLazyResultState.hpp"

#include <utility>

CYCOROUTINE_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
template<class TYPE>
class CYLazyAwaitable
{
public:
    CYLazyAwaitable(coroutine_handle<CYLazyResultState<TYPE>> handleState) noexcept
        : m_handleState(handleState)
    {
        assert(static_cast<bool>(handleState));
    }

    virtual ~CYLazyAwaitable() noexcept
    {
        auto handleState = m_handleState;
        handleState.destroy();
    }

    bool await_ready() const noexcept
    {
        return m_handleState.done();
    }

    coroutine_handle<void> await_suspend(coroutine_handle<void> handleCaller) noexcept
    {
        return m_handleState.promise().await(handleCaller);
    }

    TYPE await_resume()
    {
        return m_handleState.promise().Get();
    }

private:
    CYLazyAwaitable(const CYLazyAwaitable&) = delete;
    CYLazyAwaitable(CYLazyAwaitable&&) = delete;

private:
    const coroutine_handle<CYLazyResultState<TYPE>> m_handleState;
};

//////////////////////////////////////////////////////////////////////////
template<class TYPE>
class CYLazyResolveAwaitable
{
public:
    CYLazyResolveAwaitable(coroutine_handle<CYLazyResultState<TYPE>> handleState) noexcept
        : m_handleState(handleState)
    {
        assert(static_cast<bool>(handleState));
    }

    virtual ~CYLazyResolveAwaitable() noexcept
    {
        if (static_cast<bool>(m_handleState))
        {
            m_handleState.destroy();
        }
    }

    bool await_ready() const noexcept
    {
        return m_handleState.done();
    }

    coroutine_handle<void> await_suspend(coroutine_handle<void> handleCaller) noexcept
    {
        return m_handleState.promise().await(handleCaller);
    }

    CYLazyResult<TYPE> await_resume() noexcept
    {
        return { std::exchange(m_handleState, {}) };
    }

private:
    CYLazyResolveAwaitable(CYLazyResolveAwaitable&&) = delete;
    CYLazyResolveAwaitable(const CYLazyResolveAwaitable&) = delete;

private:
    coroutine_handle<CYLazyResultState<TYPE>> m_handleState;
};

CYCOROUTINE_NAMESPACE_END

#endif //__CY_LAZY_RESULT_AWAITTABLE_CORO_HPP__