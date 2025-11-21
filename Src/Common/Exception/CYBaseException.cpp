#include "CYCoroutine/Common/Exception/CYBaseException.hpp"

#ifdef WIN32
#include <io.h>
#else
#include <exception>
#include <errno.h>
#endif
#include <sstream>
#include <string>

#ifdef CYCOROUTINE_WIN_OS
#include <windows.h>
#endif

CYCOROUTINE_NAMESPACE_BEGIN

/**
* @brief Constructor.
*/
CYBaseException::CYBaseException(const CYBaseMessage& strMsg)
    : CYBaseMessage(strMsg)
{
    m_nTypeIndex = 25;
    m_strMsg = strMsg.GetFormatMessage();
}

/**
* @brief Constructor.
*/
CYBaseException::CYBaseException(const CYBaseMessage& strMsg, const CYBaseException& objCause)
    : CYBaseMessage(strMsg)
{
    m_strMsg = strMsg.GetFormatMessage();
    this->m_ptrCause = MakeUnique<CYBaseException>(objCause);
}

/**
* @brief Constructor.
*/
CYBaseException::CYBaseException(int nType, int severityCode, const TString& strMsg, const TString strFile, const TString& strFunction, int nLine)
    : CYBaseMessage(TEXT(""), nType, severityCode, strMsg, strFile, strFunction, nLine)
{
#ifdef WIN32
    m_nErrNo = ::GetLastError();
#else
    m_nErrNo = errno;
#endif
}

/**
* @brief Constructor.
*/
CYBaseException::CYBaseException(int nType, int severityCode, const TString& strMsg, const CYBaseException& objCause, const TString strFile, const TString& strFunction, int nLine)
    : CYBaseMessage(TEXT(""), nType, severityCode, strMsg, strFile, strFunction, nLine)
{
#ifdef WIN32
    m_nErrNo = ::GetLastError();
#else
    m_nErrNo = errno;
#endif
    this->m_ptrCause = MakeUnique<CYBaseException>(objCause);
}

/**
* @brief Inherited from std::exception
*/
char const* CYBaseException::what() const noexcept
{
    m_strFormatMsg = GetFormatMessage();
#if CY_USE_UNICODE
    m_strWhat = CYStringUtils::WString2String(m_strFormatMsg);
    return m_strWhat.c_str();
#else
    return m_strFormatMsg.c_str();
#endif
}

/**
 * @brief Get Caused by.
*/
inline const CYBaseException* CYBaseException::GetCause() const
{
    return m_ptrCause.get();
}

/**
 * @brief Get Type Index
*/
int32_t CYBaseException::GetTypeIndex()
{
    return m_nTypeIndex;
}

/**
 * @brief Inherited from CYBaseMessage.
*/
const TString CYBaseException::GetFormatMessage() const 
{
    TOStringStream ss;
    if (this->m_strMsg.length() != 0)
        ss << this->m_strMsg;
    else
    {
        //int nType = GetLevelType();
        TChar cType = TEXT('U');

        int severityCode = GetSeverCode();
        TOStringStream ssSeverityCode;
        if (severityCode != UNKNOWN_SEVER_CODE)
            ssSeverityCode << TEXT(':') << severityCode;
        TOStringStream ssThreadId;
        ssThreadId << TEXT("T:") << GetThreadId();

        TOStringStream ssSysMessage;
        if (m_nErrNo != 0)
        {
            TString s;
            ssSysMessage << TEXT("(errno=") << m_nErrNo;
#ifdef WIN32
            LPVOID lpMsgBuf;
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                m_nErrNo,
                0,
                (LPTSTR)&lpMsgBuf,
                0,
                nullptr
            );
//             LPCTSTR p = (LPCTSTR)lpMsgBuf;
//             ssSysMessage << TEXT(" ") << p;
            s = (LPCTSTR)lpMsgBuf;
//             TOStringStream wss;
//             for (LPCTSTR p = (LPCTSTR)lpMsgBuf; *p; ++p)
//                 s.append(1, wss.narrow(*p));
            LocalFree(lpMsgBuf);
#else
            s = strerror(m_nErrNo);
#endif
            size_t len = s.length();
            ssSysMessage << TEXT(" ") << s.substr(0, len - 2);
            ssSysMessage << TEXT(")");
        }

        ss  << TEXT("[")
            << GetTimeStamp().ToString()
            << TEXT(' ')
            << cType
            << ssSeverityCode.str()
            << TEXT(' ')
            << ssThreadId.str()
            << TEXT(']')
            << TEXT(' ')
            << GetFunction()
            << TEXT(' ')
            << CYBaseMessage::GetMsg();

        TString s(ssSysMessage.str());
        if (s.length() != 0)
            ss << TEXT(' ') << s;
    }
    TString s;
    const CYBaseException* cause = GetCause();
    if (cause != nullptr)
    {
        ss << TEXT(' ') << TEXT("***caused by***") << TEXT('\n');
        s = cause->GetFormatMessage();
        ss << s;
    }
    return ss.str();
}

/*
* Helper to throw exception if condition is true
*/
inline void CYBaseException::IfTrueThrow(bool bCondition, const CYBaseMessage& strMsg)
{
    if (!bCondition)
        return;

    throw new CYBaseException(strMsg);
}

/*
* Helper to throw exception if condition is false
*/
inline void CYBaseException::IfFalseThrow(bool bCondition, const CYBaseMessage& strMsg)
{
    if (bCondition)
        return;

    throw new CYBaseException(strMsg);
}

/*
* Helper to throw exception if condition is true
*/
inline void CYBaseException::IfTrueThrow(bool bCondition, const CYBaseMessage& strMsg, const CYBaseException& objCause)
{
    if (!bCondition)
        return;

    throw new CYBaseException(strMsg, objCause);
}

/*
* Helper to throw exception if condition is false
*/
inline void CYBaseException::IfFalseThrow(bool bCondition, const CYBaseMessage& strMsg, const CYBaseException& objCause)
{
    if (bCondition)
        return;

    throw new CYBaseException(strMsg, objCause);
}

/*
* Helper to throw exception if condition is true.
*/
inline void CYBaseException::IfTrueThrow(bool bCondition, int nType, int nServerCode, const TString& strMsg, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/)
{
    if (!bCondition)
        return;

    throw new CYBaseException(nType, nServerCode, strMsg, strFile, strFunction, nLine);
}

/*
* Helper to throw exception if condition is false.
*/
inline void CYBaseException::IfFalseThrow(bool bCondition, int nType, int nServerCode, const TString& strMsg, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/)
{
    if (bCondition)
        return;

    throw new CYBaseException(nType, nServerCode, strMsg, strFile, strFunction, nLine);
}

/*
* Helper to throw exception if condition is true
*/
inline void CYBaseException::IfTrueThrow(bool bCondition, int nType, int nServerCode, const TString& strMsg, const CYBaseException& objCause, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/)
{
    if (!bCondition)
        return;

    throw new CYBaseException(nType, nServerCode, strMsg, objCause, strFile, strFunction, nLine);
}

/*
* Helper to throw exception if condition is false
*/
inline void CYBaseException::IfFalseThrow(bool bCondition, int nType, int nServerCode, const TString& strMsg, const CYBaseException& objCause, const TString strFile/* = __TFILE__*/, const TString& strFunction/* = __TFUNCTION__*/, int nLine/* = __TLINE__*/)
{
    if (bCondition)
        return;

    throw new CYBaseException(nType, nServerCode, strMsg, objCause, strFile, strFunction, nLine);
}

CYCOROUTINE_NAMESPACE_END
