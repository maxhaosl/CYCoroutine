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

#ifndef __CY_THREAD_POOL_EXECUTOR_CORO_HPP__
#define __CY_THREAD_POOL_EXECUTOR_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYDerivableExecutor.hpp"
#include "CYCoroutine/Threads/CYCacheLine.hpp"
#include "CYCoroutine/Threads/CYThread.hpp"

#include <deque>
#include <mutex>

CYCOROUTINE_NAMESPACE_BEGIN

class CYIdleWorkerSet
{
    enum class EIdlStatus
    {
        STATUS_IDLE_IDLE, STATUS_IDLE_ACTIVE
    };

    struct alignas(CACHE_LINE_ALIGNMENT) CYPaddedFlag
    {
        std::atomic<EIdlStatus> eFlag{ EIdlStatus::STATUS_IDLE_ACTIVE };
    };

public:
    CYIdleWorkerSet(size_t size);
    virtual ~CYIdleWorkerSet() noexcept = default;

    void SetIdle(size_t nIdleThread) noexcept;
    void SetActive(size_t nIdleThread) noexcept;

    size_t FindIdleWorker(size_t nCallerIndex) noexcept;
    void FindIdleWorkers(size_t nCallerIndex, std::vector<size_t>& lstResultBuffer, size_t nMaxCount) noexcept;

private:
    bool TryAcquireFlag(size_t index) noexcept;

private:
    std::atomic_intptr_t m_nApproxSize;
    const UniquePtr<CYPaddedFlag[]> m_ptrIdleFlags;
    const size_t m_nSize;

};

//////////////////////////////////////////////////////////////////////////
class CYThreadPoolWorker;
class CYCOROUTINE_API alignas(CACHE_LINE_ALIGNMENT) CYThreadPoolExecutor final
    : public CYDerivableExecutor<CYThreadPoolExecutor>
{
    friend class CYThreadPoolWorker;
 public:
    CYThreadPoolExecutor(std::string_view strPoolName, size_t nPoolSize, std::chrono::milliseconds maxIdleTime, const FuncThreadDelegate& funStartedCallBack = {}, const FuncThreadDelegate& funTerminatedCallBack = {});
    virtual ~CYThreadPoolExecutor() override;

    void Enqueue(CYTask task) override;
    void Enqueue(std::span<CYTask> tasks) override;

    int  MaxConcurrencyLevel() const noexcept override;

    bool ShutdownRequested() const override;
    void ShutDown() override;

    std::chrono::milliseconds MaxWorkerIdleTime() const noexcept;

private:
    void MarkWorkerIdle(size_t index) noexcept;
    void MarkWorkerActive(size_t index) noexcept;
    void FindIdleWorkers(size_t nCallerIndex, std::vector<size_t>& buffer, size_t nMaxCount) noexcept;

    CYThreadPoolWorker& WorkerAt(size_t index) noexcept;

private:
    std::vector<CYThreadPoolWorker> m_lstWorkers;
    alignas(CACHE_LINE_ALIGNMENT) CYIdleWorkerSet m_objIdleWorkers;
    alignas(CACHE_LINE_ALIGNMENT) std::atomic_size_t m_nRoundRobinCursor;
    alignas(CACHE_LINE_ALIGNMENT) std::atomic_bool m_bAbort;

};
CYCOROUTINE_NAMESPACE_END

#endif //__CY_THREAD_POOL_EXECUTOR_CORO_HPP__
