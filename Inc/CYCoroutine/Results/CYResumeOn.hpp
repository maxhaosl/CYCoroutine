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

#ifndef __CY_RESUME_ON_CORO_HPP__
#define __CY_RESUME_ON_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYExecutor.hpp"
#include "CYCoroutine/Results/Impl/CYConsumerContext.hpp"

#include <type_traits>

CYCOROUTINE_NAMESPACE_BEGIN

template<class EXECUTOR_TYPE>
class CYResumeOnAwaitable : public suspend_always
{
public:
    CYResumeOnAwaitable(EXECUTOR_TYPE& executor) noexcept
        : m_ptrExecutor(executor)
    {
    }

    void await_suspend(coroutine_handle<void> handle)
    {
        try
        {
            m_ptrExecutor.Post(CYAwaitViaFunctor{ handle, &m_bInterrupted });
        }
        catch (...)
        {
            // the exception caused the enqeueud CYTask to be broken and resumed with an interrupt, no need to do anything here.
        }
    }

    void await_resume() const
    {
        IfTrueThrow(m_bInterrupted, TEXT("CYResult - associated task was interrupted abnormally"));
    }

private:
    CYResumeOnAwaitable(CYResumeOnAwaitable&&) = delete;
    CYResumeOnAwaitable(const CYResumeOnAwaitable&) = delete;
    CYResumeOnAwaitable& operator=(CYResumeOnAwaitable&&) = delete;
    CYResumeOnAwaitable& operator=(const CYResumeOnAwaitable&) = delete;

private:
    EXECUTOR_TYPE& m_ptrExecutor;
    bool m_bInterrupted = false;
};

template<class EXECUTOR_TYPE>
auto ResumeOn(SharePtr<EXECUTOR_TYPE> ptrExecutor)
{
    static_assert(std::is_base_of_v<CYExecutor, EXECUTOR_TYPE>, "ResumeOn() - Given executor does not derive from executor");

    if (!static_cast<bool>(ptrExecutor))
    {
        throw std::invalid_argument("ResumeOn - Given executor is null.");
    }

    return CYResumeOnAwaitable<EXECUTOR_TYPE>(*ptrExecutor);
}

template<class EXECUTOR_TYPE>
auto ResumeOn(EXECUTOR_TYPE& ptrExecutor) noexcept
{
    return CYResumeOnAwaitable<EXECUTOR_TYPE>(ptrExecutor);
}
CYCOROUTINE_NAMESPACE_END

#endif //__CY_RESUME_ON_CORO_HPP__
