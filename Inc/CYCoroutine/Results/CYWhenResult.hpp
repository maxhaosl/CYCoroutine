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

#ifndef __CY_WHEN_RESULT_CORO_HPP__
#define __CY_WHEN_RESULT_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Common/Exception/CYException.hpp"
#include "CYCoroutine/Results/CYLazyResult.hpp"
#include "CYCoroutine/Results/CYResumeOn.hpp"

#include <memory>
#include <tuple>
#include <vector>

CYCOROUTINE_NAMESPACE_BEGIN

class CYWhenResultHelper
{
public:
    template<typename TUPLE_TYPE>
    static CYResultStateBase& At(TUPLE_TYPE& tuple, size_t n) noexcept
    {
        auto seq = std::make_index_sequence<std::tuple_size<TUPLE_TYPE>::value>();
        return AtImpl(seq, tuple, n);
    }

    template<typename TYPE>
    static CYResultStateBase& At(std::vector<CYResult<TYPE>>& vector, size_t n) noexcept
    {
        assert(n < vector.size());
        return GetStateBase(vector[n]);
    }

    template<class... TYPES>
    static size_t Size(std::tuple<TYPES...>& tuple) noexcept
    {
        return std::tuple_size_v<std::tuple<TYPES...>>;
    }

    template<class TYPE>
    static size_t Size(const std::vector<TYPE>& vec) noexcept
    {
        return vec.size();
    }

    template<class... RESULT_TYPES>
    static void IfEmptyThrowTuple(const char* pszErrorMessage, RESULT_TYPES&&... results)
    {
        IfEmptyThrowImpl(pszErrorMessage, std::forward<RESULT_TYPES>(results)...);
    }

    template<class ITERATOR_TYPE>
    static void IfEmptyThrowRange(const char* pszErrorMessage, ITERATOR_TYPE begin, ITERATOR_TYPE end)
    {
        for (; begin != end; ++begin)
        {
            IfTrueThrow(!static_cast<bool>((*begin)), AtoT(pszErrorMessage));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    class CYWhenALLAwaitable
    {
    public:
        CYWhenALLAwaitable(CYResultStateBase& objState) noexcept
            : m_objState(objState)
        {
        }

        bool await_ready() const noexcept
        {
            return false;
        }

        bool await_suspend(coroutine_handle<void> handleCoro) noexcept
        {
            return m_objState.await(handleCoro);
        }

        void await_resume() const noexcept
        {
        }

    private:
        CYResultStateBase& m_objState;
    };

    //////////////////////////////////////////////////////////////////////////
    template<class RESULT_TYPES>
    class CYWhenAnyAwaitable
    {
    public:
        CYWhenAnyAwaitable(RESULT_TYPES& results) noexcept
            : m_results(results)
        {
        }

        bool await_ready() const noexcept
        {
            return false;
        }

        bool await_suspend(coroutine_handle<void> handleCoro)
        {
            m_ptrPromise = MakeShared<CYWhenAnyContext>(handleCoro);

            const auto nRangeLength = CYWhenResultHelper::Size(m_results);
            for (size_t i = 0; i < nRangeLength; i++)
            {
                if (m_ptrPromise->AnyResultFinished())
                {
                    return false;
                }

                auto& objState = CYWhenResultHelper::At(m_results, i);
                const auto status = objState.WhenAny(m_ptrPromise);
                if (status == CYResultStateBase::EResultState::STATE_RESULT_PRODUCER_DONE)
                {
                    return m_ptrPromise->ResumeInline(objState);
                }
            }

            return m_ptrPromise->FinishProcessing();
        }

        size_t await_resume() noexcept
        {
            const auto completed_result_state = m_ptrPromise->CompletedResult();
            auto completed_result_index = std::numeric_limits<size_t>::max();

            const auto nRangeLength = CYWhenResultHelper::Size(m_results);
            for (size_t i = 0; i < nRangeLength; i++)
            {
                auto& objState = CYWhenResultHelper::At(m_results, i);
                objState.TryRewindConsumer();
                if (completed_result_state == &objState)
                {
                    completed_result_index = i;
                }
            }

            assert(completed_result_index != std::numeric_limits<size_t>::max());
            return completed_result_index;
        }
    private:
        RESULT_TYPES& m_results;
        SharePtr<CYWhenAnyContext> m_ptrPromise;
    };

private:
    static void IfEmptyThrowImpl(const char* pszErrorMessage) noexcept
    {
        (void)pszErrorMessage;
    }

    template<class TYPE, class... RESULT_TYPES>
    static void IfEmptyThrowImpl(const char* pszErrorMessage, const CYResult<TYPE>& result, RESULT_TYPES&&... results)
    {
        IfTrueThrow(!static_cast<bool>(result), AtoT(pszErrorMessage));

        IfEmptyThrowImpl(pszErrorMessage, std::forward<RESULT_TYPES>(results)...);
    }

    template<class TYPE>
    static CYResultStateBase& GetStateBase(CYResult<TYPE>& result) noexcept
    {
        assert(static_cast<bool>(result.m_ptrState));
        return *result.m_ptrState;
    }

    template<std::size_t... is, typename TUPLE_TYPE>
    static CYResultStateBase& AtImpl(std::index_sequence<is...>, TUPLE_TYPE& tuple, size_t n) noexcept
    {
        CYResultStateBase* pBaseArray[] = { (&GetStateBase(std::get<is>(tuple)))... };
        assert(pBaseArray[n] != nullptr);
        return *pBaseArray[n];
    }
};

//////////////////////////////////////////////////////////////////////////
template<class SEQUENCE_TYPE>
struct CYWhenAnyResult
{
    std::size_t index;
    SEQUENCE_TYPE results;

    CYWhenAnyResult() noexcept
        : index(static_cast<size_t>(-1))
    {
    }

    template<class... RESULT_TYPES>
    CYWhenAnyResult(size_t index, RESULT_TYPES&&... results) noexcept
        : index(index)
        , results(std::forward<RESULT_TYPES>(results)...)
    {
    }

    CYWhenAnyResult(CYWhenAnyResult&&) noexcept = default;
    CYWhenAnyResult& operator=(CYWhenAnyResult&&) noexcept = default;
};


template<class EXECUTOR_TYPE, class COLLECTION_TYPE>
CYLazyResult<COLLECTION_TYPE> WhenALLImpl(SharePtr<EXECUTOR_TYPE> resume_executor, COLLECTION_TYPE collection)
{
    for (size_t i = 0; i < CYWhenResultHelper::Size(collection); i++)
    {
        auto& state_ref = CYWhenResultHelper::At(collection, i);
        co_await CYWhenResultHelper::CYWhenALLAwaitable{ state_ref };
    }

    co_await ResumeOn(resume_executor);
    co_return std::move(collection);
}

template<class EXECUTOR_TYPE>
CYLazyResult<std::tuple<>> WhenALL(SharePtr<EXECUTOR_TYPE> resume_executor)
{
    if (!static_cast<bool>(resume_executor))
    {
        throw std::invalid_argument("WhenALL() - given resume_executor is null.");
    }

    auto make_lazy_result = []() -> CYLazyResult<std::tuple<>>
        {
            co_return std::tuple<>{};
        };

    return make_lazy_result();
}

template<class EXECUTOR_TYPE, class... RESULT_TYPES>
CYLazyResult<std::tuple<typename std::decay<RESULT_TYPES>::type...>> WhenALL(SharePtr<EXECUTOR_TYPE> resume_executor, RESULT_TYPES&&... results)
{
    CYWhenResultHelper::IfEmptyThrowTuple("WhenALL() - one of the CYResult objects is empty.", std::forward<RESULT_TYPES>(results)...);

    if (!static_cast<bool>(resume_executor))
    {
        throw std::invalid_argument("WhenALL() - given resume_executor is null.");
    }

    return WhenALLImpl(resume_executor, MakeTuple(std::forward<RESULT_TYPES>(results)...));
}

template<class EXECUTOR_TYPE, class ITERATOR_TYPE>
CYLazyResult<std::vector<typename std::iterator_traits<ITERATOR_TYPE>::value_type>> WhenALL(SharePtr<EXECUTOR_TYPE> resume_executor, ITERATOR_TYPE begin, ITERATOR_TYPE end)
{
    CYWhenResultHelper::IfEmptyThrowRange("WhenALL() - one of the CYResult objects is empty.", begin, end);

    if (!static_cast<bool>(resume_executor))
    {
        throw std::invalid_argument("WhenALL() - given resume_executor is null.");
    }

    using type = typename std::iterator_traits<ITERATOR_TYPE>::value_type;

    return WhenALLImpl(resume_executor, std::vector<type> {std::make_move_iterator(begin), std::make_move_iterator(end)});
}

template<class EXECUTOR_TYPE, class TUPLE_TYPE>
CYLazyResult<CYWhenAnyResult<TUPLE_TYPE>> WhenAnyImpl(SharePtr<EXECUTOR_TYPE> resume_executor, TUPLE_TYPE tuple)
{
    const auto completed_index = co_await CYWhenResultHelper::CYWhenAnyAwaitable<TUPLE_TYPE> {tuple};
    co_await ResumeOn(resume_executor);
    co_return CYWhenAnyResult<TUPLE_TYPE> {completed_index, std::move(tuple)};
}

template<class EXECUTOR_TYPE, class type>
CYLazyResult<CYWhenAnyResult<std::vector<type>>> WhenAnyImpl(SharePtr<EXECUTOR_TYPE> resume_executor, std::vector<type> vector)
{
    const auto completed_index = co_await CYWhenResultHelper::CYWhenAnyAwaitable{ vector };
    co_await ResumeOn(resume_executor);
    co_return CYWhenAnyResult<std::vector<type>> {completed_index, std::move(vector)};
}

template<class EXECUTOR_TYPE, class... RESULT_TYPES>
CYLazyResult<CYWhenAnyResult<std::tuple<RESULT_TYPES...>>> WhenAny(SharePtr<EXECUTOR_TYPE> resume_executor, RESULT_TYPES&&... results)
{
    static_assert(sizeof...(RESULT_TYPES) != 0, "WhenAny() - the function must accept At least one CYResult object.");
    CYWhenResultHelper::IfEmptyThrowTuple("WhenAny() - one of the CYResult objects is empty.",
        std::forward<RESULT_TYPES>(results)...);

    if (!static_cast<bool>(resume_executor))
    {
        throw std::invalid_argument("WhenAny() - given resume_executor is null.");
    }

    return WhenAnyImpl(resume_executor, MakeTuple(std::forward<RESULT_TYPES>(results)...));
}

template<class EXECUTOR_TYPE, class ITERATOR_TYPE>
CYLazyResult<CYWhenAnyResult<std::vector<typename std::iterator_traits<ITERATOR_TYPE>::value_type>>> WhenAny(SharePtr<EXECUTOR_TYPE> resume_executor, ITERATOR_TYPE begin, ITERATOR_TYPE end)
{
    CYWhenResultHelper::IfEmptyThrowRange("WhenAny() - one of the CYResult objects is empty.", begin, end);

    if (begin == end)
    {
        throw std::invalid_argument("WhenAny() - given range Contains no elements.");
    }

    if (!static_cast<bool>(resume_executor))
    {
        throw std::invalid_argument("WhenAny() - given resume_executor is null.");
    }

    using type = typename std::iterator_traits<ITERATOR_TYPE>::value_type;
    return WhenAnyImpl(resume_executor, std::vector<type> {std::make_move_iterator(begin), std::make_move_iterator(end)});
}
CYCOROUTINE_NAMESPACE_END

#endif // __CY_WHEN_RESULT_CORO_HPP__
