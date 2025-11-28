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

#ifndef __CY_THREAD_CORO_HPP__
#define __CY_THREAD_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"

#include <functional>
#include <string_view>
#include <thread>

CYCOROUTINE_NAMESPACE_BEGIN

#ifdef _WIN32
using jthread = std::jthread;
#else
using jthread = std::thread;
#endif

class CYCOROUTINE_API CYThread
{
public:
    CYThread() noexcept = default;
    CYThread(CYThread&&) noexcept = default;

    CYThread& operator=(CYThread&& rhs) noexcept = default;
    virtual ~CYThread() = default;

public:
    template<class CALLABLE_TYPE>
    CYThread(std::string strName, CALLABLE_TYPE&& callable, FuncThreadDelegate funStartedCallBack, FuncThreadDelegate funTerminatedCallBack)
    {
        m_thread = jthread([strName = std::move(strName), callable = std::forward<CALLABLE_TYPE>(callable), funStartedCallBack = std::move(funStartedCallBack), funTerminatedCallBack = std::move(funTerminatedCallBack)]() mutable {

            SetName(strName);

            if (static_cast<bool>(funStartedCallBack))
            {
                funStartedCallBack(strName);
            }

            callable();

            if (static_cast<bool>(funTerminatedCallBack))
            {
                funTerminatedCallBack(strName);
            }
            });
    }

    jthread::id GetId() const noexcept;

    static std::uintptr_t GetVirtualId() noexcept;

    bool Joinable() const noexcept;
    void Join();

    static size_t NumberOfCpu() noexcept;

private:
    jthread m_thread;
    static void SetName(std::string_view strName) noexcept;
};

CYCOROUTINE_NAMESPACE_END

#endif // __CY_THREAD_CORO_HPP__
