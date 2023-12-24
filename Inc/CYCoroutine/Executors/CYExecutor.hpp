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

#ifndef __CY_EXECUTOR_CORO_HPP__
#define __CY_EXECUTOR_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/CYResult.hpp"
#include "CYCoroutine/Task/CYTask.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>


CYCOROUTINE_NAMESPACE_BEGIN

[[noreturn]] CYCOROUTINE_API void ThrowRuntimeShutdownException(std::string_view strExecutorName);
CYCOROUTINE_API std::string MakeExecutorWorkerName(std::string_view strExecutorName);

class CYCOROUTINE_API CYExecutor
{
public:
    CYExecutor(std::string_view strName)
        : strName(strName)
    {
    }

    virtual ~CYExecutor() noexcept = default;

    const std::string strName;

    virtual void Enqueue(CYTask task) = 0;
    virtual void Enqueue(std::span<CYTask> tasks) = 0;

    virtual int MaxConcurrencyLevel() const noexcept = 0;

    virtual bool ShutdownRequested() const = 0;
    virtual void ShutDown() = 0;

    template<class CALLABLE_TYPE, class... ARGS_TYPES>
    void Post(CALLABLE_TYPE&& callable, ARGS_TYPES&&... args)
    {
        return DoPost<CYExecutor>(std::forward<CALLABLE_TYPE>(callable), std::forward<ARGS_TYPES>(args)...);
    }

    template<class CALLABLE_TYPE, class... ARGS_TYPES>
    auto Submit(CALLABLE_TYPE&& callable, ARGS_TYPES&&... args)
    {
        return DoSubmit<CYExecutor>(std::forward<CALLABLE_TYPE>(callable), std::forward<ARGS_TYPES>(args)...);
    }

    template<class CALLABLE_TYPE>
    void BulkPost(std::span<CALLABLE_TYPE> lstCallable)
    {
        return DoBulkPost<CYExecutor>(lstCallable);
    }

    template<class CALLABLE_TYPE, class RETURN_TYPE = std::invoke_result_t<CALLABLE_TYPE>>
    std::vector<CYResult<RETURN_TYPE>> BulkSubmit(std::span<CALLABLE_TYPE> lstCallable)
    {
        return DoBulkSubmit<CYExecutor>(lstCallable);
    }

protected:
    template<class EXECUTOR_TYPE, class CALLABLE_TYPE, class... ARGS_TYPES>
    void DoPost(CALLABLE_TYPE&& callable, ARGS_TYPES&&... args)
    {
        static_assert(std::is_invocable_v<CALLABLE_TYPE, ARGS_TYPES...>, "CYExecutor::post - <<CALLABLE_TYPE>> is not invokable with <<ARGS_TYPES...>>");

        static_cast<EXECUTOR_TYPE*>(this)->Enqueue(BindWithTryCatch(std::forward<CALLABLE_TYPE>(callable), std::forward<ARGS_TYPES>(args)...));
    }

    template<class EXECUTOR_TYPE, class CALLABLE_TYPE, class... ARGS_TYPES>
    auto DoSubmit(CALLABLE_TYPE&& callable, ARGS_TYPES&&... args)
    {
        static_assert(std::is_invocable_v<CALLABLE_TYPE, ARGS_TYPES...>, "CYExecutor::submit - <<CALLABLE_TYPE>> is not invokable with <<ARGS_TYPES...>>");

        using RETURN_TYPE = typename std::invoke_result_t<CALLABLE_TYPE, ARGS_TYPES...>;
        return SubmitBridge<RETURN_TYPE>({}, *static_cast<EXECUTOR_TYPE*>(this), std::forward<CALLABLE_TYPE>(callable), std::forward<ARGS_TYPES>(args)...);
    }

    template<class EXECUTOR_TYPE, class CALLABLE_TYPE>
    void DoBulkPost(std::span<CALLABLE_TYPE> lstCallable)
    {
        assert(!lstCallable.empty());

        std::vector<CYTask> tasks;
        tasks.reserve(lstCallable.size());

        for (auto& callable : lstCallable)
        {
            tasks.emplace_back(BindWithTryCatch(std::move(callable)));
        }

        std::span<CYTask> span = tasks;
        static_cast<EXECUTOR_TYPE*>(this)->Enqueue(span);
    }

    template<class EXECUTOR_TYPE, class CALLABLE_TYPE, class RETURN_TYPE = std::invoke_result_t<CALLABLE_TYPE>>
    std::vector<CYResult<RETURN_TYPE>> DoBulkSubmit(std::span<CALLABLE_TYPE> lstCallable)
    {
        std::vector<CYTask> lstAccumulator;
        lstAccumulator.reserve(lstCallable.size());

        std::vector<CYResult<RETURN_TYPE>> results;
        results.reserve(lstCallable.size());

        for (auto& callable : lstCallable)
        {
            results.emplace_back(BulkSubmitBridge<CALLABLE_TYPE>(lstAccumulator, std::move(callable)));
        }

        assert(!lstAccumulator.empty());
        std::span<CYTask> span = lstAccumulator;
        static_cast<EXECUTOR_TYPE*>(this)->Enqueue(span);
        return results;
    }

private:
    template<class RETURN_TYPE, class EXECUTOR_TYPE, class CALLABLE_TYPE, class... ARGS_TYPES>
    static CYResult<RETURN_TYPE> SubmitBridge(CYExecutorTag, EXECUTOR_TYPE&, CALLABLE_TYPE callable, ARGS_TYPES... args)
    {
        co_return callable(args...);
    }

    struct CYAccumulatingAwaitable
    {
        std::vector<CYTask>& m_lstAccumulator;
        bool m_bInterrupted = false;

        CYAccumulatingAwaitable(std::vector<CYTask>& lstAccumulator) noexcept
            : m_lstAccumulator(lstAccumulator)
        {
        }

        constexpr bool await_ready() const noexcept
        {
            return false;
        }

        void await_suspend(coroutine_handle<void> handleCoro) noexcept
        {
            try
            {
                m_lstAccumulator.emplace_back(CYAwaitViaFunctor(handleCoro, &m_bInterrupted));
            }
            catch (...)
            {
                // do nothing. ~CYAwaitViaFunctor will resume the coroutine and throw an exception.
            }
        }

        void await_resume() const
        {
            IfTrueThrow(m_bInterrupted, TEXT("await_resume Interrupted."));
        }
    };

    template<class CALLABLE_TYPE, class RETURN_TYPE = typename std::invoke_result_t<CALLABLE_TYPE>>
    static CYResult<RETURN_TYPE> BulkSubmitBridge(std::vector<CYTask>& lstAccumulator, CALLABLE_TYPE callable)
    {

        co_await CYAccumulatingAwaitable(lstAccumulator);
        co_return callable();
    }
};

CYCOROUTINE_NAMESPACE_END

#endif //__CY_EXECUTOR_CORO_HPP__
