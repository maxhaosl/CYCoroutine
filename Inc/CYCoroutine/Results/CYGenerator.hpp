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

#ifndef __CY_GENERATOR_CORO_HPP__
#define __CY_GENERATOR_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/Impl/CYGeneratorState.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

template<typename TYPE>
class CYGenerator
{
public:
    using promise_type = CYGeneratorState<TYPE>;
    using iterator = CYGeneratorIterator<TYPE>;
    static_assert(!std::is_same_v<TYPE, void>, "<<TYPE>> can not be void.");

public:
    CYGenerator(coroutine_handle<promise_type> handle) noexcept
        : m_handleCoro(handle)
    {
    }

    CYGenerator(CYGenerator&& rhs) noexcept
        : m_handleCoro(std::exchange(rhs.m_handleCoro, {}))
    {
    }

    virtual ~CYGenerator() noexcept
    {
        if (static_cast<bool>(m_handleCoro))
        {
            m_handleCoro.destroy();
        }
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_handleCoro);
    }

    iterator begin()
    {
        IfTrueThrow(!static_cast<bool>(m_handleCoro), TEXT("CYGenerator::begin - CYGenerator is empty."));

        assert(!m_handleCoro.done());
        m_handleCoro.resume();

        if (m_handleCoro.done())
        {
            m_handleCoro.promise().throw_if_exception();
        }

        return iterator{ m_handleCoro };
    }

    static CYGeneratorEndIterator end() noexcept
    {
        return {};
    }

private:
    CYGenerator(const CYGenerator& rhs) = delete;
    CYGenerator& operator=(CYGenerator&& rhs) = delete;
    CYGenerator& operator=(const CYGenerator& rhs) = delete;

private:
    coroutine_handle<promise_type> m_handleCoro;
};
CYCOROUTINE_NAMESPACE_END

#endif //__CY_GENERATOR_CORO_HPP__