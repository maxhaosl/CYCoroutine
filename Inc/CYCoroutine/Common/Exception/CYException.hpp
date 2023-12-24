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

#ifndef __CY_EXCEPTION_HPP__
#define __CY_EXCEPTION_HPP__

#include "CYCoroutine/Common/Exception/CYBaseException.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

static constexpr int K_LOG_LEVEL_UNKNOWN = 0;

template<int _LEVEL = K_LOG_LEVEL_UNKNOWN, int _SEVER_CODE = UNKNOWN_SEVER_CODE>
class CYException : public CYBaseException
{
public:
	/**
	* @brief The level of the exception
	*/
	static const int LEVEL = _LEVEL;

	/**
	* @brief The severity code of the exception
	*/
	static const int SEVER_CODE = _SEVER_CODE;

	/**
	* @brief Constructor.
	*/
	CYException(const TString& strMsg, const TString strFile, const TString& strFunction, int nLine) :
		CYBaseException(_LEVEL, _SEVER_CODE, strMsg, strFile, strFunction, nLine)
	{
	}

	/**
	* @brief Constructor.
	*/
	CYException(const TString& strMsg, const CYBaseException& objCause, const TString strFile, const TString& strFunction, int nLine) :
		CYBaseException(_LEVEL, _SEVER_CODE, strMsg, objCause, strFile, strFunction, nLine)
	{
	}

	/*
	* @brief Helper to throw exception if condition is true.
	*/
	inline static void IfTrueThrow(bool bCondition, const TString& strMsg, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/)
	{
		if (!bCondition)
			return;

		throw new CYException(strMsg, strFile, strFunction, nLine);
	}

	/*
	* @brief Helper to throw exception if condition is true
	*/
	inline static void IfTrueReThrow(bool bCondition, const TString& strMsg, const CYBaseException& objCause, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/)
	{
		if (!bCondition)
			return;

		throw new CYException(strMsg, objCause, strFile, strFunction, nLine);
	}

	/*
	* @brief Helper to throw exception if condition is false.
	*/
	inline static void IfFalseThrow(bool bCondition, const TString& strMsg, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/)
	{
		if (bCondition)
			return;

		throw new CYException(strMsg, strFile, strFunction, nLine);
	}

	/*
	* @brief Helper to throw exception if condition is false
	*/
	inline static void IfFalseReThrow(bool bCondition, const TString& strMsg, const CYBaseException& objCause, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/)
	{
		if (bCondition)
			return;

		throw new CYException(strMsg, objCause, strFile, strFunction, nLine);
	}
};

CYCOROUTINE_NAMESPACE_END

#define IfTrueThrow(bCondition, strMsg) CYCOROUTINE_NAMESPACE::CYException<K_LOG_LEVEL_UNKNOWN>::IfTrueThrow(bCondition, strMsg, __TFILE__, __TFUNCTION__, __TLINE__)
#define IfTrueReThrow(bCondition, strMsg, objCause)  CYCOROUTINE_NAMESPACE::CYException<K_LOG_LEVEL_UNKNOWN>::IfTrueReThrow(bCondition, strMsg, objCause, __TFILE__, __TFUNCTION__, __TLINE__)
#define IfFalseThrow(bCondition, strMsg) CYCOROUTINE_NAMESPACE::CYException<K_LOG_LEVEL_UNKNOWN>::IfFalseThrow(bCondition, strMsg, __TFILE__, __TFUNCTION__, __TLINE__)
#define IfFalseReThrow(bCondition, strMsg, objCause)  CYCOROUTINE_NAMESPACE::CYException<K_LOG_LEVEL_UNKNOWN>::IfFalseReThrow(bCondition, strMsg, objCause, __TFILE__, __TFUNCTION__, __TLINE__)

#endif //__CY_EXCEPTION_HPP__