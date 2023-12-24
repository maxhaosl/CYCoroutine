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

#ifndef __CY_WORKER_THREAD_EXECUTOR_CORO_HPP__
#define __CY_WORKER_THREAD_EXECUTOR_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYDerivableExecutor.hpp"
#include "CYCoroutine/Threads/CYCacheLine.hpp"
#include "CYCoroutine/Threads/CYThread.hpp"

#include <deque>
#include <mutex>
#include <semaphore>

CYCOROUTINE_NAMESPACE_BEGIN

class CYCOROUTINE_API alignas(CACHE_LINE_ALIGNMENT) CYWorkerThreadExecutor final : public CYDerivableExecutor<CYWorkerThreadExecutor>
{
public:
    CYWorkerThreadExecutor(const FuncThreadDelegate & funStartedCallBack = {}, const FuncThreadDelegate & funTerminatedCallBack = {});

    void Enqueue(CYTask task) override;
    void Enqueue(std::span<CYTask> tasks) override;

    int  MaxConcurrencyLevel() const noexcept override;

    bool ShutdownRequested() const override;
    void ShutDown() override;

private:
    void MakeOSWorkerThread();
    bool DrainQueueImpl();
    bool DrainQueue();
    void WaitForTask(UniqueLock& lock);
    void WorkLoop();

    void EnqueueLocal(CYTask& task);
    void EnqueueLocal(std::span<CYTask> task);

    void EnqueueForeign(CYTask& task);
    void EnqueueForeign(std::span<CYTask> task);

private:
    bool             m_bPublicAbort;
    std::atomic_bool m_bPrivateAbort;
    alignas(CACHE_LINE_ALIGNMENT) std::mutex m_lock;

    std::deque<CYTask> m_lstPrivTaskQueue;
    std::deque<CYTask> m_lstPublicTaskQueue;

    CYThread m_thread;
    std::atomic_bool m_bAtomicAbort;
    std::binary_semaphore m_semaphore;

    const std::function<void(std::string_view)> m_funcStartedCallBack;
    const std::function<void(std::string_view)> m_funcTerminatedCallback;
};

CYCOROUTINE_NAMESPACE_END

#endif //__CY_WORKER_THREAD_EXECUTOR_CORO_HPP__
