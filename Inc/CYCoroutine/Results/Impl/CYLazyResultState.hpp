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

#ifndef __CY_LAZY_RESULT_STATE_CORO_HPP__
#define __CY_LAZY_RESULT_STATE_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/Impl/CYProducerContext.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

struct CYLazyFinalAwaiter : public suspend_always
{
    template<class promise_type>
    coroutine_handle<void> await_suspend(coroutine_handle<promise_type> handle) noexcept
    {
        return handle.promise().ResumeCaller();
    }
};

//////////////////////////////////////////////////////////////////////////
class CYLazyResultStateBase
{
public:
    coroutine_handle<void> ResumeCaller() const noexcept
    {
        return m_handleCaller;
    }

    coroutine_handle<void> await(coroutine_handle<void> handleCaller) noexcept
    {
        m_handleCaller = handleCaller;
        return coroutine_handle<CYLazyResultStateBase>::from_promise(*this);
    }

protected:
    coroutine_handle<void> m_handleCaller;
};

//////////////////////////////////////////////////////////////////////////
template<class TYPE>
class CYLazyResult;

template<class type>
class CYLazyResultState : public CYLazyResultStateBase
{
public:
    EResultStatus Status() const noexcept
    {
        return m_objProducer.Status();
    }

    CYLazyResult<type> get_return_object() noexcept
    {
        const auto self_handle = coroutine_handle<CYLazyResultState>::from_promise(*this);
        return CYLazyResult<type>(self_handle);
    }

    void unhandled_exception() noexcept
    {
        m_objProducer.BuildException(std::current_exception());
    }

    suspend_always initial_suspend() const noexcept
    {
        return {};
    }

    CYLazyFinalAwaiter final_suspend() const noexcept
    {
        return {};
    }

    template<class... ARGS_TYPES>
    void SetResult(ARGS_TYPES&&... args) noexcept(noexcept(type(std::forward<ARGS_TYPES>(args)...)))
    {
        m_objProducer.BuildResult(std::forward<ARGS_TYPES>(args)...);
    }

    type Get()
    {
        return m_objProducer.Get();
    }

private:
    CYProducerContext<type> m_objProducer;

};

CYCOROUTINE_NAMESPACE_END

#endif //__CY_LAZY_RESULT_STATE_CORO_HPP__