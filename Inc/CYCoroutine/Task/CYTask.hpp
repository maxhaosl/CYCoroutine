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

#ifndef __CY_TASK_CORO_HPP__
#define __CY_TASK_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

CYCOROUTINE_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
struct CYTaskConstants
{
    static constexpr size_t TOTAL_SIZE = 64;
    static constexpr size_t BUFFER_SIZE = TOTAL_SIZE - sizeof(void*);
};

//////////////////////////////////////////////////////////////////////////
struct CYVTable
{
    void (*FunMoveDestroy)(void* pSrc, void* pDst) noexcept;
    void (*FunExecuteDestroy)(void* pTarget);
    void (*FunDestroy)(void* pTarget) noexcept;

    CYVTable(const CYVTable&) noexcept = default;

    constexpr CYVTable() noexcept
        : FunMoveDestroy(nullptr)
        , FunExecuteDestroy(nullptr)
        , FunDestroy(nullptr)
    {
    }

    constexpr CYVTable(decltype(FunMoveDestroy) funMoveDestroy, decltype(FunExecuteDestroy) funExecuteDestroy, decltype(FunDestroy) funDestroy) noexcept
        : FunMoveDestroy(funMoveDestroy)
        , FunExecuteDestroy(funExecuteDestroy)
        , FunDestroy(funDestroy)
    {
    }

    static constexpr bool TriviallyCopiableDestructible(decltype(FunMoveDestroy) funMove) noexcept
    {
        return funMove == nullptr;
    }

    static constexpr bool TriviallyDestructible(decltype(FunDestroy) funDestroy) noexcept
    {
        return funDestroy == nullptr;
    }
};

//////////////////////////////////////////////////////////////////////////
template<class CALLABLE_TYPE>
class CYCallableVTable
{

private:
    static CALLABLE_TYPE* InlinePtr(void* pSrc) noexcept
    {
        return static_cast<CALLABLE_TYPE*>(pSrc);
    }

    static CALLABLE_TYPE* AllocatedPtr(void* pSrc) noexcept
    {
        return *static_cast<CALLABLE_TYPE**>(pSrc);
    }

    static CALLABLE_TYPE*& AllocatedRefPtr(void* pSrc) noexcept
    {
        return *static_cast<CALLABLE_TYPE**>(pSrc);
    }

    static void MoveDestroyInline(void* pSrc, void* pDst) noexcept
    {
        auto callable_ptr = InlinePtr(pSrc);
        new (pDst) CALLABLE_TYPE(std::move(*callable_ptr));
        callable_ptr->~CALLABLE_TYPE();
    }

    static void MoveDestroyAllocated(void* pSrc, void* pDst) noexcept
    {
        auto callable_ptr = std::exchange(AllocatedRefPtr(pSrc), nullptr);
        new (pDst) CALLABLE_TYPE* (callable_ptr);
    }

    static void ExecuteDestroyInline(void* pTarget)
    {
        auto callable_ptr = InlinePtr(pTarget);
        (*callable_ptr)();
        callable_ptr->~CALLABLE_TYPE();
    }

    static void ExecuteDestroyAllocated(void* pTarget)
    {
        auto callable_ptr = AllocatedPtr(pTarget);
        (*callable_ptr)();
        delete callable_ptr;
    }

    static void DestroyInline(void* pTarget) noexcept
    {
        auto callable_ptr = InlinePtr(pTarget);
        callable_ptr->~CALLABLE_TYPE();
    }

    static void DestroyAllocated(void* pTarget) noexcept
    {
        auto callable_ptr = AllocatedPtr(pTarget);
        delete callable_ptr;
    }

    static constexpr CYVTable MakeVTable() noexcept
    {
        void (*FunMoveDestroy)(void* pSrc, void* pDst) noexcept = nullptr;
        void (*FunDestroy)(void* pTarget) noexcept = nullptr;

        if constexpr (std::is_trivially_copy_constructible_v<CALLABLE_TYPE> && std::is_trivially_destructible_v<CALLABLE_TYPE> && IsInLinable())
        {
            FunMoveDestroy = nullptr;
        }
        else
        {
            FunMoveDestroy = MoveDestroy;
        }

        if constexpr (std::is_trivially_destructible_v<CALLABLE_TYPE> && IsInLinable())
        {
            FunDestroy = nullptr;
        }
        else
        {
            FunDestroy = Destroy;
        }

        return CYVTable(FunMoveDestroy, ExecuteDestroy, FunDestroy);
    }

    template<class passed_callable_type>
    static void BuildInLinable(void* pDst, passed_callable_type&& callable)
    {
        new (pDst) CALLABLE_TYPE(std::forward<passed_callable_type>(callable));
    }

    template<class passed_callable_type>
    static void BuildAllocated(void* pDst, passed_callable_type&& callable)
    {
        auto new_ptr = new CALLABLE_TYPE(std::forward<passed_callable_type>(callable));
        new (pDst) CALLABLE_TYPE* (new_ptr);
    }

public:
    static constexpr bool IsInLinable() noexcept
    {
        return std::is_nothrow_move_constructible_v<CALLABLE_TYPE> && sizeof(CALLABLE_TYPE) <= CYTaskConstants::BUFFER_SIZE;
    }

    template<class passed_callable_type>
    static void Build(void* pDst, passed_callable_type&& callable)
    {
        if (IsInLinable())
        {
            return BuildInLinable(pDst, std::forward<passed_callable_type>(callable));
        }

        BuildAllocated(pDst, std::forward<passed_callable_type>(callable));
    }

    static void MoveDestroy(void* pSrc, void* pDst) noexcept
    {
        assert(pSrc != nullptr);
        assert(pDst != nullptr);

        if (IsInLinable())
        {
            return MoveDestroyInline(pSrc, pDst);
        }

        return MoveDestroyAllocated(pSrc, pDst);
    }

    static void ExecuteDestroy(void* pTarget)
    {
        assert(pTarget != nullptr);

        if (IsInLinable())
        {
            return ExecuteDestroyInline(pTarget);
        }

        return ExecuteDestroyAllocated(pTarget);
    }

    static void Destroy(void* pTarget) noexcept
    {
        assert(pTarget != nullptr);

        if (IsInLinable())
        {
            return DestroyInline(pTarget);
        }

        return DestroyAllocated(pTarget);
    }

    static constexpr CALLABLE_TYPE* As(void* pSrc) noexcept
    {
        if (IsInLinable())
        {
            return InlinePtr(pSrc);
        }

        return AllocatedPtr(pSrc);
    }

    static constexpr inline CYVTable s_vtable = MakeVTable();
};

//////////////////////////////////////////////////////////////////////////
class CYCOROUTINE_API CYTask
{
private:
    alignas(std::max_align_t) std::byte m_buffer[CYTaskConstants::BUFFER_SIZE];
    const CYVTable* m_vtable;

    void Build(CYTask&& rhs) noexcept;
    void Build(coroutine_handle<void> handleCoro) noexcept;

    template<class CALLABLE_TYPE>
    void Build(CALLABLE_TYPE&& callable)
    {
        using DecayedType = typename std::decay_t<CALLABLE_TYPE>;

        CYCallableVTable<DecayedType>::Build(m_buffer, std::forward<CALLABLE_TYPE>(callable));
        m_vtable = &CYCallableVTable<DecayedType>::s_vtable;
    }

    template<class CALLABLE_TYPE>
    static bool Contains(const CYVTable* const vTable) noexcept
    {
        return vTable == &CYCallableVTable<CALLABLE_TYPE>::s_vtable;
    }

    bool ContainsCoroutineHandle() const noexcept;

public:
    CYTask() noexcept;
    CYTask(CYTask&& rhs) noexcept;
    CYTask(coroutine_handle<void> handleCoro) noexcept;

    template<class CALLABLE_TYPE>
    CYTask(CALLABLE_TYPE&& callable)
    {
        Build(std::forward<CALLABLE_TYPE>(callable));
    }

    ~CYTask() noexcept;

    CYTask(const CYTask& rhs) = delete;
    CYTask& operator=(const CYTask&& rhs) = delete;

    void operator()();

    CYTask& operator=(CYTask&& rhs) noexcept;

    void Clear() noexcept;

    explicit operator bool() const noexcept;

    template<class CALLABLE_TYPE>
    bool Contains() const noexcept
    {
        using DecayedType = typename std::decay_t<CALLABLE_TYPE>;

        if constexpr (std::is_same_v<DecayedType, coroutine_handle<void>>)
        {
            return ContainsCoroutineHandle();
        }

        return m_vtable == &CYCallableVTable<DecayedType>::s_vtable;
    }
};
CYCOROUTINE_NAMESPACE_END

#endif // __CY_TASK_CORO_HPP__
