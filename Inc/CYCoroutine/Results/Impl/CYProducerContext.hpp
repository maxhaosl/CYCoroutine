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

#ifndef __CY_PRODUCER_CONTEXT_HPPP__
#define __CY_PRODUCER_CONTEXT_HPPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"

#include <cassert>
#include <exception>

CYCOROUTINE_NAMESPACE_BEGIN

template<class TYPE>
class CYProducerContext
{
public:
    virtual ~CYProducerContext() noexcept
    {
        switch (m_eStatus)
        {
        case EResultStatus::STATUS_RESULT_VALUE:
        {
            m_storage.object.~TYPE();
            break;
        }

        case EResultStatus::STATUS_RESULT_EXCEPTION:
        {
            m_storage.exception.~exception_ptr();
            break;
        }

        case EResultStatus::STATUS_RESULT_IDLE:
        {
            break;
        }

        default:
        {
            assert(false);
        }
        }
    }

    template<class... ARGS_TYPES>
    void BuildResult(ARGS_TYPES&&... args) noexcept(noexcept(TYPE(std::forward<ARGS_TYPES>(args)...)))
    {
        assert(m_eStatus == EResultStatus::STATUS_RESULT_IDLE);
        new (std::addressof(m_storage.object)) TYPE(std::forward<ARGS_TYPES>(args)...);
        m_eStatus = EResultStatus::STATUS_RESULT_VALUE;
    }

    void BuildException(const std::exception_ptr& exception) noexcept
    {
        assert(m_eStatus == EResultStatus::STATUS_RESULT_IDLE);
        new (std::addressof(m_storage.exception)) std::exception_ptr(exception);
        m_eStatus = EResultStatus::STATUS_RESULT_EXCEPTION;
    }

    EResultStatus Status() const noexcept
    {
        return m_eStatus;
    }

    TYPE Get()
    {
        return std::move(GetRef());
    }

    TYPE& GetRef()
    {
        assert(m_eStatus != EResultStatus::STATUS_RESULT_IDLE);
        if (m_eStatus == EResultStatus::STATUS_RESULT_VALUE)
        {
            return m_storage.object;
        }

        assert(m_eStatus == EResultStatus::STATUS_RESULT_EXCEPTION);
        assert(static_cast<bool>(m_storage.exception));
        std::rethrow_exception(m_storage.exception);
    }

private:
    union CYStorage
    {
        TYPE object;
        std::exception_ptr exception;

        CYStorage() noexcept
        {
        }
        ~CYStorage() noexcept
        {
        }
    };

private:
    CYStorage m_storage;
    EResultStatus m_eStatus = EResultStatus::STATUS_RESULT_IDLE;
};

//////////////////////////////////////////////////////////////////////////
template<>
class CYProducerContext<void>
{
public:
    virtual ~CYProducerContext() noexcept
    {
        if (m_eStatus == EResultStatus::STATUS_RESULT_EXCEPTION)
        {
            m_storage.exception.~exception_ptr();
        }
    }

    CYProducerContext& operator=(CYProducerContext&& rhs) noexcept
    {
        assert(m_eStatus == EResultStatus::STATUS_RESULT_IDLE);
        m_eStatus = std::exchange(rhs.m_eStatus, EResultStatus::STATUS_RESULT_IDLE);

        if (m_eStatus == EResultStatus::STATUS_RESULT_EXCEPTION)
        {
            new (std::addressof(m_storage.exception)) std::exception_ptr(rhs.m_storage.exception);
            rhs.m_storage.exception.~exception_ptr();
        }

        return *this;
    }

    void BuildResult() noexcept
    {
        assert(m_eStatus == EResultStatus::STATUS_RESULT_IDLE);
        m_eStatus = EResultStatus::STATUS_RESULT_VALUE;
    }

    void BuildException(const std::exception_ptr& except) noexcept
    {
        assert(m_eStatus == EResultStatus::STATUS_RESULT_IDLE);
        new (std::addressof(m_storage.exception)) std::exception_ptr(except);
        m_eStatus = EResultStatus::STATUS_RESULT_EXCEPTION;
    }

    EResultStatus Status() const noexcept
    {
        return m_eStatus;
    }

    void Get() const
    {
        GetRef();
    }

    void GetRef() const
    {
        assert(m_eStatus != EResultStatus::STATUS_RESULT_IDLE);
        if (m_eStatus == EResultStatus::STATUS_RESULT_EXCEPTION)
        {
            assert(static_cast<bool>(m_storage.exception));
            std::rethrow_exception(m_storage.exception);
        }
    }

private:
    union CYStorage
    {
        std::exception_ptr exception;

        CYStorage() noexcept
        {
        }
        ~CYStorage() noexcept
        {
        }
    };

private:
    CYStorage m_storage;
    EResultStatus m_eStatus = EResultStatus::STATUS_RESULT_IDLE;

};

//////////////////////////////////////////////////////////////////////////
template<class TYPE>
class CYProducerContext<TYPE&>
{
public:
    virtual ~CYProducerContext() noexcept
    {
        if (m_eStatus == EResultStatus::STATUS_RESULT_EXCEPTION)
        {
            m_storage.exception.~exception_ptr();
        }
    }

    CYProducerContext& operator=(CYProducerContext&& rhs) noexcept
    {
        assert(m_eStatus == EResultStatus::STATUS_RESULT_IDLE);
        m_eStatus = std::exchange(rhs.m_eStatus, EResultStatus::STATUS_RESULT_IDLE);

        switch (m_eStatus)
        {
        case EResultStatus::STATUS_RESULT_VALUE:
        {
            m_storage.pointer = rhs.m_storage.pointer;
            break;
        }

        case EResultStatus::STATUS_RESULT_EXCEPTION:
        {
            new (std::addressof(m_storage.exception)) std::exception_ptr(rhs.m_storage.exception);
            rhs.m_storage.exception.~exception_ptr();
            break;
        }

        case EResultStatus::STATUS_RESULT_IDLE:
        {
            break;
        }

        default:
        {
            assert(false);
        }
        }

        return *this;
    }

    void BuildResult(TYPE& reference) noexcept
    {
        assert(m_eStatus == EResultStatus::STATUS_RESULT_IDLE);

        auto pointer = std::addressof(reference);
        assert(pointer != nullptr);
        assert(reinterpret_cast<size_t>(pointer) % alignof(TYPE) == 0);

        m_storage.pointer = pointer;
        m_eStatus = EResultStatus::STATUS_RESULT_VALUE;
    }

    void BuildException(const std::exception_ptr& exception) noexcept
    {
        assert(m_eStatus == EResultStatus::STATUS_RESULT_IDLE);
        new (std::addressof(m_storage.exception)) std::exception_ptr(exception);
        m_eStatus = EResultStatus::STATUS_RESULT_EXCEPTION;
    }

    EResultStatus Status() const noexcept
    {
        return m_eStatus;
    }

    TYPE& Get() const
    {
        return GetRef();
    }

    TYPE& GetRef() const
    {
        assert(m_eStatus != EResultStatus::STATUS_RESULT_IDLE);

        if (m_eStatus == EResultStatus::STATUS_RESULT_VALUE)
        {
            assert(m_storage.pointer != nullptr);
            assert(reinterpret_cast<size_t>(m_storage.pointer) % alignof(TYPE) == 0);
            return *m_storage.pointer;
        }

        assert(m_eStatus == EResultStatus::STATUS_RESULT_EXCEPTION);
        assert(static_cast<bool>(m_storage.exception));
        std::rethrow_exception(m_storage.exception);
    }

private:
    union CYStorage
    {
        TYPE* pointer;
        std::exception_ptr exception;

        CYStorage() noexcept
        {
        }
        ~CYStorage() noexcept
        {
        }
    };

private:
    CYStorage m_storage;
    EResultStatus m_eStatus = EResultStatus::STATUS_RESULT_IDLE;
};

CYCOROUTINE_NAMESPACE_END


#endif //__CY_PRODUCER_CONTEXT_HPPP__