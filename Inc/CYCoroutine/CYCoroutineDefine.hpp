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

#ifndef __CY_COROUTINE_DEFINE_CORO_HPP__
#define __CY_COROUTINE_DEFINE_CORO_HPP__

#include <memory>
#include <mutex>
#include <tuple>
#include <string>

#include "CYCoroutine/CYTypeDefine.hpp"

#if !__has_include(<coroutine>) && __has_include(<experimental/coroutine>)
#    include <experimental/coroutine>
#    define COROUTINE_NAMESPACE_STD std::experimental
#else

#    include <coroutine>
#    define COROUTINE_NAMESPACE_STD std
#endif

#define CYCOROUTINE_NAMESPACE_BEGIN			namespace cry {
#define CYCOROUTINE_NAMESPACE				cry
#define CYCOROUTINE_NAMESPACE_END			}
#define CYCOROUTINE_NAMESPACE_USE			using namespace	cry

#if defined(__MINGW32__)
#    define CYCOROUTINE_MINGW_OS
#elif defined(_WIN32)
#    define CYCOROUTINE_WIN_OS
#elif defined(unix) || defined(__unix__) || defined(__unix)
#    define CYCOROUTINE_UNIX_OS
#elif defined(__APPLE__) || defined(__MACH__)
#    define CYCOROUTINE_MAC_OS
#elif defined(__FreeBSD__)
#    define CYCOROUTINE_FREE_BSD_OS
#elif defined(__ANDROID__)
#    define CYCOROUTINE_ANDROID_OS
#endif

#if defined(__clang__)
#    define CYCOROUTINE_CLANG_COMPILER
#elif defined(__GNUC__) || defined(__GNUG__)
#    define CYCOROUTINE_GCC_COMPILER
#elif defined(_MSC_VER)
#    define CYCOROUTINE_MSVC_COMPILER
#endif

#if !defined(NDEBUG) || defined(_DEBUG)
#    define CYCOROUTINE_DEBUG_MODE
#endif

#if defined(CYCOROUTINE_WIN_OS)
#    if defined(CRCPP_EXPORT_API)
#        define CYCOROUTINE_API __declspec(dllexport)
#    elif defined(CRCPP_IMPORT_API)
#        define CYCOROUTINE_API __declspec(dllimport)
#    endif
#elif (defined(CRCPP_EXPORT_API) || defined(CRCPP_IMPORT_API)) && __has_cpp_attribute(gnu::visibility)
#    define CYCOROUTINE_API __attribute__((visibility("default")))
#endif

#if !defined(CYCOROUTINE_API)
#    define CYCOROUTINE_API
#endif

#include <exception>

#if defined(_LIBCPP_VERSION)
#    define CRCPP_LIBCPP_LIB
#endif

CYCOROUTINE_NAMESPACE_BEGIN

template<class PROMISE_TYPE>
using coroutine_handle = std::coroutine_handle<PROMISE_TYPE>;
using suspend_never = std::suspend_never;
using suspend_always = std::suspend_always;

struct CYExecutorTag
{
};

struct CYNuLLResult
{
};

enum class EResultStatus
{
    STATUS_RESULT_IDLE      = 0x00, 
    STATUS_RESULT_VALUE     = 0x01, 
    STATUS_RESULT_EXCEPTION = 0x02,
};

CYCOROUTINE_NAMESPACE_END

#endif //__CY_COROUTINE_DEFINE_CORO_HPP__