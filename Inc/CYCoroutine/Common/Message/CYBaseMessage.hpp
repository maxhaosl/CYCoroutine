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

#ifndef __CY_BASE_MESSAGE_HPP__
#define __CY_BASE_MESSAGE_HPP__

#include "CYCoroutine/Common/Time/CYTimeStamps.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

class CYBaseMessage
{
public:
	/**
	* @param strChannel message channel.
	* @param nMsgType message type
	* @param nServerCode message severity code
	* @param strFunction code location - can be function name or anything else
	* @param m_strMsg text of the message
	* @param inThrowMode if true cannot throw
	*/
	CYBaseMessage(const TString& strChannel, int nMsgType, int nServerCode, const TString& strMsg, const TString& strFile, const TString& strFunction, int nLine);


	/**
	 * @param strChannel message channel.
	 * @param nMsgType message type
	 * @param nServerCode  message severity code
	 * @param strMsg text of the message
	*/
	CYBaseMessage(const TString& strChannel, int nMsgType, int nServerCode, const TString& strMsg);

	/**
	 * @brief Destructor.
	*/
	virtual ~CYBaseMessage() = default;

	/**
	* @brief Every derived class should implement this method.
	*/
	virtual const TString GetFormatMessage() const = 0;

	/**
	 * @brief Get Message Type.
	*/
	int GetMsgType() const;

	/**
	 * @brief Get Server Code.
	*/
	int GetSeverCode() const;

	/**
	 * @brief Get Log Message Function.
	*/
	const TString& GetFunction() const;

	/**
	 * @brief Get Log Message.
	*/
	const TString& GetMsg() const;

	/**
	 * @brief Get Log Channel.
	*/
	const TString& GetChannel() const;

	/**
	 * @brief Get File Path.
	*/
	const TString& GetFile() const;

	/**
	 * @brief Get Code Line.
	*/
	int GetLine() const;

	/**
	 * @brief Set Channel.
	*/
	void SetChannel(const TString& strChannel);

	/**
	 * @brief Get Log TimeStamp.
	*/
	const CYTimeStamps& GetTimeStamp() const;

	/**
	 * @brief Get Thread Id.
	*/
	unsigned long GetThreadId() const;

    /**
     * @brief Get Type Index
    */
	virtual int32_t GetTypeIndex() = 0;

private:
	int    		    m_nMsgType;
	int				m_nLine;
	int				m_nServerCode;
	TString         m_strChannel;
	TString			m_strMsg;
	TString			m_strFile;
	TString			m_strFunction;
	CYTimeStamps	m_objTimeStamp;
	unsigned long	m_nThreadId;
};

CYCOROUTINE_NAMESPACE_END

#endif //__CY_BASE_MESSAGE_HPP__