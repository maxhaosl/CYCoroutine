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

#ifndef ___CY_LIST_CORO_HPP___
#define ___CY_LIST_CORO_HPP___

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include <cassert>

CYCOROUTINE_NAMESPACE_BEGIN

template<class NODE_TYPE>
class CYList
{
public:
    CYList() noexcept = default;

    CYList(CYList& rhs) noexcept
        : m_pHead(rhs.m_pHead)
        , m_pTail(rhs.m_pTail)
    {
        rhs.m_pHead = nullptr;
        rhs.m_pTail = nullptr;
    }

    CYList(CYList&& rhs) noexcept
        : m_pHead(rhs.m_pHead)
        , m_pTail(rhs.m_pTail)
    {
        rhs.m_pHead = nullptr;
        rhs.m_pTail = nullptr;
    }

public:
    bool Empty() const noexcept
    {
        CheckState();
        return m_pHead == nullptr;
    }

    void PushBack(NODE_TYPE& node) noexcept
    {
        CheckState();

        if (m_pHead == nullptr)
        {
            m_pHead = m_pTail = &node;
            return;
        }

        m_pTail->next = &node;
        m_pTail = &node;
    }

    NODE_TYPE* PopFront() noexcept
    {
        CheckState();
        const auto node = m_pHead;
        if (node == nullptr)
        {
            return nullptr;
        }

        m_pHead = m_pHead->next;
        if (m_pHead == nullptr)
        {
            m_pTail = nullptr;
        }

        return node;
    }

private:
    NODE_TYPE* m_pHead = nullptr;
    NODE_TYPE* m_pTail = nullptr;

    void CheckState() const noexcept
    {
        if (m_pHead == nullptr)
        {
            assert(m_pTail == nullptr);
            return;
        }

        assert(m_pTail != nullptr);
    }
};

CYCOROUTINE_NAMESPACE_END

#endif //___CY_LIST_CORO_HPP___