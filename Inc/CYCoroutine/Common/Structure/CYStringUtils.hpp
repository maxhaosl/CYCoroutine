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

#ifndef __CY_STRING_UTILS_HPP__
#define __CY_STRING_UTILS_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"

#include <string>

CYCOROUTINE_NAMESPACE_BEGIN

class CYStringUtils
{
public:
	/**
	 * @brief wstring convert to string.
	*/
    static std::string WString2String(const std::wstring strSrc);

	/**
	 * @brief string convert to wstring.
	*/
    static std::wstring String2WString(const std::string strSrc);

#ifdef UNICODE
	static std::wstring String2TString(const std::wstring strSrc)
	{
		return strSrc;
	}

    static std::wstring String2TString(const std::string strSrc)
    {
		return String2WString(strSrc);
    }

    static std::string TString2String(const std::wstring strSrc)
    {
        return WString2String(strSrc);
    }

    static std::string TString2String(const std::string strSrc)
    {
        return strSrc;
    }
#else
    static std::string String2TString(const std::wstring strSrc)
    {
        return WString2String(strSrc);
    }

    static std::string String2TString(const std::string strSrc)
    {
        return strSrc;
    }

    static std::string TString2String(const std::wstring strSrc)
    {
        return WString2String(strSrc);
    }

    static std::string TString2String(const std::string strSrc)
    {
        return strSrc;
    }
#endif
};

#define AtoT(x) CYStringUtils::String2TString(x).c_str()

CYCOROUTINE_NAMESPACE_END

#endif // __CY_STRING_UTILS_HPP__