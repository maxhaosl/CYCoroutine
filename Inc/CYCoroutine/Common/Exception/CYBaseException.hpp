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

#ifndef __CY_BASE_EXCEPTION_HPP__
#define __CY_BASE_EXCEPTION_HPP__

#include "CYCoroutine/Common/Message/CYBaseMessage.hpp"
#include "CYCoroutine/Common/Structure/CYStringUtils.hpp"
#ifdef WIN32
#include <io.h>
#else
#include <exception>
#endif

#include <string>
#include <sstream>

CYCOROUTINE_NAMESPACE_BEGIN

class CYBaseException : public std::exception, public CYBaseMessage
{
public:
    /**
    * @brief Constructor.
    */
    CYBaseException(const CYBaseMessage& strMsg);

    /**
    * @brief Constructor.
    */
    CYBaseException(const CYBaseMessage& strMsg, const CYBaseException& objCause);

    /**
    * @brief Constructor.
    */
    CYBaseException(int nType, int nServerCode, const TString& strMsg, const TString strFile, const TString& strFunction, int nLine);

    /**
    * @brief Constructor.
    */
    CYBaseException(int nType, int nServerCode, const TString& strMsg, const CYBaseException& cause, const TString strFile, const TString& strFunction, int nLine);

    /**
     * @brief Destructor.
    */
    virtual ~CYBaseException() noexcept = default;

    /**
    * @brief Inherited from std::exception
    */
    virtual char const* what() const noexcept override;

public:
    /**
    * Inherited from CYBaseMessage, the result is in the following format: [YYYYMMDD hh:mm:ss.nnnnnn |{T,D,I,W,E,F,U}: severCode] location message [(errno=x system message)] [***caused by***] \r\n
    */
    virtual const TString GetFormatMessage() const override;

    /**
     * @brief Get Type Index
    */
    virtual int32_t GetTypeIndex() override;

    /**
     * @brief Get Caused by.
    */
    inline const CYBaseException* GetCause() const;

    /*
    * Helper to throw exception if condition is true
    */
    inline static void IfTrueThrow(bool bCondition, const CYBaseMessage& strMsg);

    /*
    * Helper to throw exception if condition is false
    */
    inline static void IfFalseThrow(bool bCondition, const CYBaseMessage& strMsg);

    /*
    * Helper to throw exception if condition is true
    */
    inline static void IfTrueThrow(bool bCondition, const CYBaseMessage& strMsg, const CYBaseException& objCause);

    /*
    * Helper to throw exception if condition is false
    */
    inline static void IfFalseThrow(bool bCondition, const CYBaseMessage& strMsg, const CYBaseException& objCause);

    /*
    * Helper to throw exception if condition is true.
    */
    inline static void IfTrueThrow(bool bCondition, int nType, int nServerCode, const TString& strMsg, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/);

    /*
    * Helper to throw exception if condition is false.
    */
    inline static void IfFalseThrow(bool bCondition, int nType, int nServerCode, const TString& strMsg, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/);

    /*
    * Helper to throw exception if condition is true
    */
    inline static void IfTrueThrow(bool bCondition, int nType, int nServerCode, const TString& strMsg, const CYBaseException& objCause, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/);

    /*
    * Helper to throw exception if condition is false
    */
    inline static void IfFalseThrow(bool bCondition, int nType, int nServerCode, const TString& strMsg, const CYBaseException& objCause, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/);

private:
    int                         m_nTypeIndex = 0;
    int							m_nErrNo = 0;
    mutable TString				m_strFormatMsg;
    TString				        m_strMsg;
    mutable std::string			m_strWhat;
    SharePtr<CYBaseException>	m_ptrCause;
};

CYCOROUTINE_NAMESPACE_END

#endif //__CY_BASE_EXCEPTION_HPP__
