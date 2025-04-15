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

#ifndef __CY_TYPE_DEFINE_CORO_HPP__
#define __CY_TYPE_DEFINE_CORO_HPP__

#include <string>
#include <iostream>
#include <iosfwd>
#include <fstream>

#include <memory>
#include <mutex>
#include <functional>
#include <cassert>

#ifdef _WIN32
#include <direct.h>
#endif

#if defined(_WIN32)
#ifdef _UNICODE
#define CY_USE_UNICODE  1
#else
#define CY_USE_UNICODE  0
#endif
#endif

// macro
//////////////////////////////////////////////////////////////////////////
#define WIDEN2(x)       L ## x
#define WIDEN(x)        WIDEN2(x)
#define __WFILE__       WIDEN(__FILE__)
#define __WDATE__       WIDEN(__DATE__)
#define __WTIME__       WIDEN(__TIME__)

#if defined(_WIN32) && CY_USE_UNICODE
typedef std::wstring            TString;
typedef std::wstring_view       TStringView;
typedef std::wofstream          TOfStream;
typedef std::wostringstream     TOStringStream;
typedef std::wios               TIos;
typedef std::wostream           TOStream;

typedef std::wifstream          TIfStream;
typedef std::wofstream          TOfStream;
typedef std::wstringstream      TStringStream;

#define cy_csrchr               wcsrchr
#define cy_strlen               wcslen
#define cy_vscprintf            _vscwprintf
#define cy_vsnprintf_s          _vsnwprintf_s
#define cy_mkdir                _wmkdir
#define cy_splitpath            _wsplitpath
#define cy_strcpy               wcscpy
#define cy_fullpath             _wfullpath
#define cy_sprintf_s            swprintf_s
#define cy_strcat_s             wcscat_s
#define cy_tcscpy_s             wcscpy_s
#define cy_tcstok_s             wcstok_s
#define cy_tcscmp               wcscmp
#define cy_strstr               wcsstr
#else
typedef std::string             TString;
typedef std::string_view        TStringView;
typedef std::ofstream           TOfStream;
typedef std::ostringstream      TOStringStream;
typedef std::ios                TIos;
typedef std::ostream            TOStream;

typedef std::ifstream           TIfStream;
typedef std::ofstream           TOfStream;
typedef std::stringstream       TStringStream;

#define cy_csrchr               strrchr
#define cy_strlen               strlen
#define cy_vscprintf            _vscprintf
#define cy_vsnprintf_s          _vsnprintf_s
#define cy_mkdir                _mkdir
#define cy_splitpath            _splitpath
#define cy_strcpy               strcpy
#define cy_fullpath             _fullpath
#define cy_sprintf_s            sprintf_s
#define cy_strcat_s             strcat_s
#define cy_tcscpy_s             strcpy_s
#define cy_tcstok_s             strtok_s
#define cy_tcscmp               strcmp
#define cy_strstr               strstr
#endif

#ifndef SharePtr
#define SharePtr                std::shared_ptr
#endif
#ifndef UniquePtr
#define UniquePtr               std::unique_ptr
#endif
#ifndef WeakPtr
#define WeakPtr                 std::weak_ptr
#endif
#ifndef MakeShared
#define MakeShared              std::make_shared
#endif
#ifndef MakeUnique
#define MakeUnique              std::make_unique
#endif
#ifndef MakeTuple
#define MakeTuple               std::make_tuple
#endif

#ifndef UniqueLock
#define UniqueLock              std::unique_lock<std::mutex>
#endif
#ifndef LockGuard
#define LockGuard               std::lock_guard<std::mutex>
#endif
using FuncThreadDelegate        = std::function<void(std::string_view thread_name)>;

template<class F>
struct function_traits;

template<class R, class... Args>
struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)>
{
};

template<class R, class... Args>
struct function_traits<R(Args...)>
{
    using return_type = R;
    static constexpr std::size_t size = sizeof...(Args);
    template<std::size_t N>
    struct argument
    {
        static_assert(N < size, "error: invalid parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

template<auto Fn>
struct PointerDel final
{
    template<typename T>
    void operator()(T* p) const
    {
        if (!p || !Fn)
            return;

        using Traits = function_traits<decltype(Fn)>;
        if constexpr (std::is_same_v<typename Traits::template argument<0>::type, T**>)
            Fn(&p);
        else if constexpr (std::is_same_v<typename Traits::template argument<0>::type, T*>)
            Fn(p);
        else
            assert(false && "Wrong PointerDel Function");
    }
};

//////////////////////////////////////////////////////////////////////////
#define UNKNOWN_SEVER_CODE      -1

#if defined(_WIN32) && CY_USE_UNICODE
typedef wchar_t          TChar;
#define __TFILE__       __WFILE__
#define __TDATE__       __WDATE__
#define __TTIME__       __WTIME__
#define __TFUNCTION__   __FUNCTIONW__
#define __TLINE__       __LINE__
#else
typedef char            TChar;
#define __TFILE__       __FILE__
#define __TDATE__       __DATE__
#define __TTIME__       __TIME__
#define __TLINE__       __LINE__
#ifdef _WIN32
#define __TFUNCTION__   __func__
#else
#define __TFUNCTION__   __FUNCTION__
#endif
#endif

#ifndef TEXT
#if defined(_WIN32) && CY_USE_UNICODE
#define __TEXT(quote) L##quote 
#define TEXT_BYTE_LEN 2
#else
#define __TEXT(quote) quote    
#define TEXT_BYTE_LEN 1
#endif

#define TEXT(quote) __TEXT(quote)  
#endif

#endif //__CY_TYPE_DEFINE_CORO_HPP__