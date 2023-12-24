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

#ifndef __CY_RESULT_CORO_HPP__
#define __CY_RESULT_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Common/CYBind.hpp"
#include "CYCoroutine/Common/Exception/CYException.hpp"
#include "CYCoroutine/Results/CYResultAwaitable.hpp"
#include "CYCoroutine/Results/Impl/CYResultState.hpp"

#include <type_traits>

CYCOROUTINE_NAMESPACE_BEGIN

template<class TYPE>
class CYResult
{
    static constexpr auto IsValideResult = std::is_same_v<TYPE, void> || std::is_nothrow_move_constructible_v<TYPE>;
    static_assert(IsValideResult, "<<TYPE>> should be no-throw-move constructible or void.");

    friend class CYWhenResultHelper;
    friend struct CYSharedResultHelper;
public:
    CYResult() noexcept = default;
    CYResult(CYResult&& rhs) noexcept = default;

    CYResult(CYConsumerResultStatePtr<TYPE> ptrState) noexcept
        : m_ptrState(std::move(ptrState))
    {
    }
    CYResult(CYResultState<TYPE>* ptrState) noexcept
        : m_ptrState(ptrState)
    {
    }

    CYResult& operator=(CYResult&& rhs) noexcept
    {
        if (this != &rhs)
        {
            m_ptrState = std::move(rhs.m_ptrState);
        }

        return *this;
    }

public:
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_ptrState);
    }

    EResultStatus Status() const
    {
        IfEmptyThrow(TEXT("CYResult::Status() - result is empty."));
        return m_ptrState->Status();
    }

    void Wait() const
    {
        IfEmptyThrow(TEXT("CYResult::wait() - result is empty."));
        m_ptrState->Wait();
    }

    template<class DURATION_TYPE, class RATIO_TYPE>
    EResultStatus WaitFor(std::chrono::duration<DURATION_TYPE, RATIO_TYPE> duration) const
    {
        IfEmptyThrow(TEXT("CYResult::WaitFor() - result is empty."));
        return m_ptrState->WaitFor(duration);
    }

    template<class ClockType, class DURATION_TYPE>
    EResultStatus WaitUntil(std::chrono::time_point<ClockType, DURATION_TYPE> timeoutTime) const
    {
        IfEmptyThrow(TEXT("CYResult::WaitUntil() - result is empty."));
        return m_ptrState->WaitUntil(timeoutTime);
    }

    TYPE Get()
    {
        IfEmptyThrow(TEXT("CYResult::get() - result is empty."));
        m_ptrState->Wait();

        CYJoinedConsumerResultStatePtr<TYPE> ptrState(m_ptrState.release());
        return ptrState->Get();
    }

    auto operator co_await()
    {
        IfEmptyThrow(TEXT("CYResult::operator co_await() - result is empty."));
        return CYAwaitable<TYPE> {std::move(m_ptrState)};
    }

    auto Resolve()
    {
        IfEmptyThrow(TEXT("CYResult::Resolve() - result is empty."));
        return CYResolveAwaitable<TYPE> {std::move(m_ptrState)};
    }

private:
    CYResult(const CYResult& rhs) = delete;
    CYResult& operator=(const CYResult& rhs) = delete;

private:
    CYConsumerResultStatePtr<TYPE> m_ptrState;

    void IfEmptyThrow(const TChar* pszMessage) const
    {
        IfTrueThrow(static_cast<bool>(!m_ptrState), pszMessage);
    }
};

//////////////////////////////////////////////////////////////////////////
template<class TYPE>
class CYResultPromise
{
    static constexpr auto IsValideResult = std::is_same_v<TYPE, void> || std::is_nothrow_move_constructible_v<TYPE>;
    static_assert(IsValideResult, "CYResultPromise<TYPE> - <<TYPE>> should be no-throw-move constructible or void.");

public:
    CYResultPromise()
    {
        m_objProducerState.reset(new CYResultState<TYPE>());
        m_objConsumerState.reset(m_objProducerState.get());
    }

    CYResultPromise(CYResultPromise&& rhs) noexcept :
        m_objProducerState(std::move(rhs.m_objProducerState)), m_objConsumerState(std::move(rhs.m_objConsumerState))
    {
    }

    virtual ~CYResultPromise() noexcept
    {
        BreakTaskIfNeeded();
    }

public:
    CYResultPromise& operator=(CYResultPromise&& rhs) noexcept
    {
        if (this != &rhs)
        {
            BreakTaskIfNeeded();
            m_objProducerState = std::move(rhs.m_objProducerState);
            m_objConsumerState = std::move(rhs.m_objConsumerState);
        }

        return *this;
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_objProducerState);
    }

    template<class... ARGS_TYPES>
    void SetResult(ARGS_TYPES&&... args)
    {
        constexpr auto is_constructable = std::is_constructible_v<TYPE, ARGS_TYPES...> || std::is_same_v<void, TYPE>;
        static_assert(is_constructable, "CYResultPromise::SetResult() - <<TYPE>> is not constructable from <<args...>>");

        IfEmptyThrow(TEXT("CYResultPromise::SetResult() - empty resultPromise."));

        m_objProducerState->SetResult(std::forward<ARGS_TYPES>(args)...);
        m_objProducerState.reset();
    }

    void SetException(std::exception_ptr pException)
    {
        IfEmptyThrow(TEXT("CYResultPromise::SetException() - empty resultPromise."));

        if (!static_cast<bool>(pException))
        {
            throw std::invalid_argument("CYResultPromise::SetException() - exception pointer is null.");
        }

        m_objProducerState->SetException(pException);
        m_objProducerState.reset();
    }

    template<class CALLABLE_TYPE, class... ARGS_TYPES>
    void SetFromFunction(CALLABLE_TYPE&& callable, ARGS_TYPES&&... args) noexcept
    {
        constexpr auto is_invokable = std::is_invocable_r_v<TYPE, CALLABLE_TYPE, ARGS_TYPES...>;

        static_assert(is_invokable, "CYResultPromise::SetFromFunction() - function(args...) is not invokable or its return TYPE can't be used to construct <<TYPE>>");

        IfEmptyThrow(TEXT("CYResultPromise::SetFromFunction() - empty resultPromise."));
        m_objProducerState->FromCallable(Bind(std::forward<CALLABLE_TYPE>(callable), std::forward<ARGS_TYPES>(args)...));
        m_objProducerState.reset();
    }

    CYResult<TYPE> GetResult()
    {
        IfEmptyThrow(TEXT("CYResult::get() - result is empty."));

        IfTrueThrow(!static_cast<bool>(m_objConsumerState), TEXT("CYResultPromise::GetResult() - CYResult was already retrieved."));

        return CYResult<TYPE>(std::move(m_objConsumerState));
    }

private:
    CYResultPromise(const CYResultPromise&) = delete;
    CYResultPromise& operator=(const CYResultPromise&) = delete;

    void IfEmptyThrow(const TChar* pszMessage) const
    {
        IfTrueThrow(!static_cast<bool>(m_objProducerState), pszMessage);
    }

    void BreakTaskIfNeeded() noexcept
    {
        if (!static_cast<bool>(m_objProducerState))
        {
            return;
        }

        if (static_cast<bool>(m_objConsumerState))
        {  // no CYResult to break.
            return;
        }

        auto exception_ptr = std::make_exception_ptr(CYException(TEXT("CYResult - associated task was interrupted abnormally"), __TFILE__, __TFUNCTION__, __TLINE__));
        m_objProducerState->SetException(exception_ptr);
        m_objProducerState.reset();
    }

private:
    CYProducerResultStatePtr<TYPE> m_objProducerState;
    CYConsumerResultStatePtr<TYPE> m_objConsumerState;
};
CYCOROUTINE_NAMESPACE_END

#endif //__CY_RESULT_CORO_HPP__