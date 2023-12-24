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

#ifndef __CY_GENERATOR_STATE_CORO_HPP__
#define __CY_GENERATOR_STATE_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

template<typename type>
class CYGenerator;

template<typename TYPE>
class CYGeneratorState
{
public:
    using value_type = std::remove_reference_t<TYPE>;

public:
    CYGenerator<TYPE> get_return_object() noexcept
    {
        return CYGenerator<TYPE> {coroutine_handle<CYGeneratorState<TYPE>>::from_promise(*this)};
    }

    suspend_always initial_suspend() const noexcept
    {
        return {};
    }

    suspend_always final_suspend() const noexcept
    {
        return {};
    }

    suspend_always yield_value(value_type& ref) noexcept
    {
        m_pValue = std::addressof(ref);
        return {};
    }

    suspend_always yield_value(value_type&& ref) noexcept
    {
        m_pValue = std::addressof(ref);
        return {};
    }

    void unhandled_exception() noexcept
    {
        m_pException = std::current_exception();
    }

    void return_void() const noexcept
    {
    }

    value_type& value() const noexcept
    {
        assert(m_pValue != nullptr);
        assert(reinterpret_cast<std::intptr_t>(m_pValue) % alignof(value_type) == 0);
        return *m_pValue;
    }

    void throw_if_exception() const
    {
        if (static_cast<bool>(m_pException))
        {
            std::rethrow_exception(m_pException);
        }
    }

private:
    value_type* m_pValue = nullptr;
    std::exception_ptr m_pException;
};

//////////////////////////////////////////////////////////////////////////
struct CYGeneratorEndIterator
{
};

//////////////////////////////////////////////////////////////////////////
template<typename TYPE>
class CYGeneratorIterator
{
public:
    using value_type = std::remove_reference_t<TYPE>;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;

public:
    CYGeneratorIterator(coroutine_handle<CYGeneratorState<TYPE>> handle) noexcept
        : m_handleCoro(handle)
    {
        assert(static_cast<bool>(m_handleCoro));
    }

    CYGeneratorIterator& operator++()
    {
        assert(static_cast<bool>(m_handleCoro));
        assert(!m_handleCoro.done());
        m_handleCoro.resume();

        if (m_handleCoro.done())
        {
            m_handleCoro.promise().throw_if_exception();
        }

        return *this;
    }

    void operator++(int)
    {
        (void)operator++();
    }

    reference operator*() const noexcept
    {
        assert(static_cast<bool>(m_handleCoro));
        return m_handleCoro.promise().value();
    }

    pointer operator->() const noexcept
    {
        assert(static_cast<bool>(m_handleCoro));
        return std::addressof(operator*());
    }

    friend bool operator==(const CYGeneratorIterator& it0, const CYGeneratorIterator& it1) noexcept
    {
        return it0.m_handleCoro == it1.m_handleCoro;
    }

    friend bool operator==(const CYGeneratorIterator& it, CYGeneratorEndIterator) noexcept
    {
        return it.m_handleCoro.done();
    }

    friend bool operator==(CYGeneratorEndIterator end_it, const CYGeneratorIterator& it) noexcept
    {
        return (it == end_it);
    }

    friend bool operator!=(const CYGeneratorIterator& it, CYGeneratorEndIterator end_it) noexcept
    {
        return !(it == end_it);
    }

    friend bool operator!=(CYGeneratorEndIterator end_it, const CYGeneratorIterator& it) noexcept
    {
        return it != end_it;
    }

private:
    coroutine_handle<CYGeneratorState<TYPE>> m_handleCoro;

};

CYCOROUTINE_NAMESPACE_END

#endif //__CY_GENERATOR_STATE_CORO_HPP__