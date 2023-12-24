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

#ifndef __CY_ASYNC_LOCK_CORO_HPP__
#define __CY_ASYNC_LOCK_CORO_HPP__

#include "CYCoroutine/Common/CYList.hpp"
#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYExecutor.hpp"
#include "CYCoroutine/Results/CYLazyResult.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

class CAsyncLock;
class CYCOROUTINE_API CAsyncLockAwaiter
{
    friend class CAsyncLock;
public:
    inline CAsyncLockAwaiter(CAsyncLock& parent, UniqueLock& lock) noexcept;

    constexpr bool  await_ready() const noexcept;
    void            await_suspend(coroutine_handle<void> hHandle);
    constexpr void  await_resume() const noexcept;
    void            retry() noexcept;

public:
    CAsyncLockAwaiter*      next = nullptr;

private:
    CAsyncLock&             m_parent;
    UniqueLock              m_lock;
    coroutine_handle<void>  m_handleResume;
};

//////////////////////////////////////////////////////////////////////////
class CScopedAsyncLock;
class CYCOROUTINE_API CAsyncLock
{
    friend class CScopedAsyncLock;
    friend class CAsyncLockAwaiter;
public:
    virtual ~CAsyncLock() noexcept;

    CYLazyResult<CScopedAsyncLock>  Lock(SharePtr<CYExecutor> ptrResumeExecutor);
    CYLazyResult<bool>              TryLock();
    void                            UnLock();

private:
    CYLazyResult<CScopedAsyncLock>  LockImpl(SharePtr<CYExecutor> ptrResumeExecutor, bool bWithRAIIGuard);

private:
    bool                        m_bLocked = false;
    std::mutex                  m_mutexAwaiter;
    CYList<CAsyncLockAwaiter>   m_lstAwaiters;

#ifdef CYCOROUTINE_DEBUG_MODE
    std::atomic_intptr_t        m_nThreadCountInCriticalSection{ 0 };
#endif
};

//////////////////////////////////////////////////////////////////////////
class CYCOROUTINE_API CScopedAsyncLock
{
public:
    inline CScopedAsyncLock() noexcept = default;
    inline CScopedAsyncLock(CScopedAsyncLock&& rhs) noexcept;

    inline CScopedAsyncLock(CAsyncLock& lock, std::defer_lock_t) noexcept;
    inline CScopedAsyncLock(CAsyncLock& lock, std::adopt_lock_t) noexcept;

    virtual ~CScopedAsyncLock() noexcept;

public:
    CYLazyResult<void>  Lock(SharePtr<CYExecutor> ptrResumeExecutor);
    CYLazyResult<bool>  TryLock();
    void                UnLock();

    bool                OwnsLock() const noexcept;
    explicit operator bool() const noexcept;

    void                Swap(CScopedAsyncLock& rhs) noexcept;
    CAsyncLock* Release() noexcept;
    CAsyncLock* Mutex() const noexcept;

private:
    CAsyncLock* m_pLock = nullptr;
    bool                m_bOwns = false;

};
CYCOROUTINE_NAMESPACE_END

#endif //__CY_ASYNC_LOCK_CORO_HPP__
