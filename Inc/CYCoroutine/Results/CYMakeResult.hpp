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

#ifndef __CY_MAKE_RESULT_CORO_HPP__
#define __CY_MAKE_RESULT_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/CYResult.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
template<class TYPE, class... ARGS_TYPES>
CYResult<TYPE> MakeReadyResult(ARGS_TYPES&&... args)
{
    static_assert(std::is_constructible_v<TYPE, ARGS_TYPES...> || std::is_same_v<TYPE, void>, "MakeReadyResult - <<TYPE>> is not constructible from <<ARGS_TYPES...>");
    static_assert(std::is_same_v<TYPE, void> ? (sizeof...(ARGS_TYPES) == 0) : true, "MakeReadyResult<void> - this overload does not accept any argument.");

    CYProducerResultStatePtr<TYPE> ptrPromise(new CYResultState<TYPE>());
    CYConsumerResultStatePtr<TYPE> ptrState(ptrPromise.get());

    ptrPromise->SetResult(std::forward<ARGS_TYPES>(args)...);
    ptrPromise.reset();  // publish the result;

    return { std::move(ptrState) };
}

//////////////////////////////////////////////////////////////////////////
template<class TYPE>
CYResult<TYPE> MakeExceptionalResult(std::exception_ptr pException)
{
    if (!static_cast<bool>(pException))
    {
        throw std::invalid_argument("make_exception_result() - given exception_ptr is null.");
    }

    CYProducerResultStatePtr<TYPE> ptrPromise(new CYResultState<TYPE>());
    CYConsumerResultStatePtr<TYPE> ptrState(ptrPromise.get());

    ptrPromise->SetException(pException);
    ptrPromise.reset();  // publish the result;

    return { std::move(ptrState) };
}

//////////////////////////////////////////////////////////////////////////
template<class TYPE, class EXCEPTION_TYPE>
CYResult<TYPE> MakeExceptionalResult(EXCEPTION_TYPE objException)
{
    return MakeExceptionalResult<TYPE>(std::make_exception_ptr(objException));
}
CYCOROUTINE_NAMESPACE_END

#endif //__CY_MAKE_RESULT_CORO_HPP__