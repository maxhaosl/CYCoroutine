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

#ifndef __CY_ATOMIC_EX_HPP__
#define __CY_ATOMIC_EX_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"

#include <atomic>
#include <cassert>
#include <type_traits>
#include <mutex>
#include <condition_variable>

#if defined(_MSC_VER)
#define ATOMIC_HAS_WAIT 0
#else
#define ATOMIC_HAS_WAIT 0
#endif

CYCOROUTINE_NAMESPACE_BEGIN

template <typename T>
class CYAtomicBase : public std::atomic<T>
{
public:
    using Base = std::atomic<T>;
    using Base::Base;  

    void wait(const T& oldValue)
    {
        wait_impl(oldValue, std::memory_order_seq_cst);

    }

    void wait(const T& oldValue, std::memory_order order)
    {
        wait_impl(oldValue, order);
    }

    void notify_one()
    {
        notify_one_impl();
    }

    void notify_all()
    {
        notify_all_impl();
    }

    void store(T v, std::memory_order order = std::memory_order_seq_cst)
    {
        Base::store(v, order);
        store_notify_impl();
    }

private:
    void wait_impl(const T& oldValue, std::memory_order)
    {
        if (this->load() != oldValue)
            return;

        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&] { return this->load() != oldValue || m_bWaiting.load(); });

        m_bWaiting = false;
    }

    void notify_one_impl()
    {
        m_bWaiting = true;
        cv_.notify_one();
    }

    void notify_all_impl()
    {
        m_bWaiting = true;
        cv_.notify_all();
    }

    void store_notify_impl()
    {
        m_bWaiting = true;
        cv_.notify_all();
    }
private:
    std::atomic_bool m_bWaiting{ false };
    std::mutex mtx_;
    std::condition_variable cv_;
};


template <typename T>
#if ATOMIC_HAS_WAIT
using cy_atomic = std::atomic<T>;
#else
using cy_atomic = CYAtomicBase<T>;
#endif

#endif // !__CY_ATOMIC_EX_HPP__