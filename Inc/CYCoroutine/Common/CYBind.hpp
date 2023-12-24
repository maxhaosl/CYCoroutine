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

#ifndef __CY_BIND_CORO_HPP__
#define __CY_BIND_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include <tuple>
#include <type_traits>

CYCOROUTINE_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
template<class CALLABLE_TYPE>
auto&& Bind(CALLABLE_TYPE&& callable)
{
    return std::forward<CALLABLE_TYPE>(callable);
}

//////////////////////////////////////////////////////////////////////////
template<class CALLABLE_TYPE, class... ARGS_TYPES>
auto Bind(CALLABLE_TYPE&& callable, ARGS_TYPES&&... args)
{
    constexpr static auto inti = std::is_nothrow_invocable_v<CALLABLE_TYPE, ARGS_TYPES...>;
    return [callable = std::forward<CALLABLE_TYPE>(callable), tuple = MakeTuple(std::forward<ARGS_TYPES>(args)...)]() mutable noexcept(inti) -> decltype(auto)
        {
            return std::apply(callable, tuple);
        };
}

//////////////////////////////////////////////////////////////////////////
template<class CALLABLE_TYPE>
auto&& BindWithTryCatchImpl(std::true_type /*is_noexcept*/, CALLABLE_TYPE&& callable)
{
    return std::forward<CALLABLE_TYPE>(callable);
}

//////////////////////////////////////////////////////////////////////////
template<class CALLABLE_TYPE>
auto BindWithTryCatchImpl(std::false_type /*is_noexcept*/, CALLABLE_TYPE&& callable)
{
    return [callable = std::forward<CALLABLE_TYPE>(callable)]() mutable noexcept {
        try
        {
            callable();
        }
        catch (...)
        {
            // do nothing
        }
        };  // no args to Bind
}

//////////////////////////////////////////////////////////////////////////
template<class CALLABLE_TYPE>
auto BindWithTryCatch(CALLABLE_TYPE&& callable)
{
    using isNoexcept = typename std::is_nothrow_invocable<CALLABLE_TYPE>::type;
    return BindWithTryCatchImpl(isNoexcept{}, std::forward<CALLABLE_TYPE>(callable));
}

//////////////////////////////////////////////////////////////////////////
template<class CALLABLE_TYPE, class... ARGS_TYPES>
auto BindWithTryCatch(CALLABLE_TYPE&& callable, ARGS_TYPES&&... args)
{
    return BindWithTryCatch(Bind(std::forward<CALLABLE_TYPE>(callable), std::forward<ARGS_TYPES>(args)...));
}

CYCOROUTINE_NAMESPACE_END

#endif //__CY_BIND_CORO_HPP__