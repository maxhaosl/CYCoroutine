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

#ifndef __CY_ASYNC_CONDITION_CORO_HPP__
#define __CY_ASYNC_CONDITION_CORO_HPP__

#include "CYCoroutine/Common/CYList.hpp"
#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/CYLazyResult.hpp"
#include "CYCoroutine/Threads/CYAsyncLock.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
class CYAsyncCondition;
class CYCOROUTINE_API CYAWaiter
{
public:
    CYAWaiter(CYAsyncCondition& parent, CScopedAsyncLock& lock) noexcept;
    virtual ~CYAWaiter() noexcept = default;

public:
    constexpr bool  await_ready() const noexcept;
    void            await_suspend(coroutine_handle<void> handleCaller);
    void            await_resume() const noexcept;
    void            resume() noexcept;

public:
    CYAWaiter*        next = nullptr;

private:
    CYAsyncCondition& m_parent;
    CScopedAsyncLock& m_lock;
    coroutine_handle<void> m_handleCaller;
};

//////////////////////////////////////////////////////////////////////////
class CYCOROUTINE_API CYAsyncCondition
{
    friend CYAWaiter;
public:
    CYAsyncCondition() noexcept = default;
    ~CYAsyncCondition() noexcept;

public:
    CYLazyResult<void> await(SharePtr<CYExecutor> ptrResumeExecutor, CScopedAsyncLock& lock);

    template<class PREDICATE_TYPE>
    CYLazyResult<void> await(SharePtr<CYExecutor> ptrResumeExecutor, CScopedAsyncLock& lock, PREDICATE_TYPE pred)
    {
        static_assert(std::is_invocable_r_v<bool, PREDICATE_TYPE>, "Given predicate isn't invocable with no args, or does not return a type which is or convertible to bool.");

        VerifyAwaitParams(ptrResumeExecutor, lock);
        return AWaitImpl(std::move(ptrResumeExecutor), lock, pred);
    }

    void NotifyOne();
    void NotifyALL();

private:
    CYAsyncCondition(const CYAsyncCondition&) noexcept = delete;
    CYAsyncCondition(CYAsyncCondition&&) noexcept = delete;

    CYLazyResult<void> AWaitImpl(SharePtr<CYExecutor> ptrResumeExecutor, CScopedAsyncLock& lock);

    template<class PREDICATE_TYPE>
    CYLazyResult<void> AWaitImpl(SharePtr<CYExecutor> ptrResumeExecutor, CScopedAsyncLock& lock, PREDICATE_TYPE pred)
    {
        while (true)
        {
            assert(lock.OwnsLock());
            if (pred())
            {
                break;
            }

            co_await AWaitImpl(ptrResumeExecutor, lock);
        }
    }

    static void VerifyAwaitParams(const SharePtr<CYExecutor>& ptrResumeExecutor, const CScopedAsyncLock& lock);

private:
    std::mutex          m_lock;
    CYList<CYAWaiter>   m_awaiters;

};
CYCOROUTINE_NAMESPACE_END

#endif //__CY_ASYNC_CONDITION_CORO_HPP__