#include "CYCoroutine/Common/Message/CYBaseMessage.hpp"

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

CYCOROUTINE_NAMESPACE_BEGIN

CYBaseMessage::CYBaseMessage(const TString& strChannel, int nMsgType, int nServerCode, const TString& strMsg, const TString& strFile, const TString& strFunction, int nLine)
{
	this->m_strChannel = strChannel;
	this->m_nMsgType = nMsgType;
	this->m_nServerCode = nServerCode;
	this->m_nLine = nLine;
	this->m_strFile = strFile;
	this->m_strFunction = strFunction;
	this->m_strMsg = strMsg;
#ifdef WIN32
	m_nThreadId = GetCurrentThreadId();
#else
    m_nThreadId = (long int)pthread_self();
#endif
}


/**
 * @param strChannel message channel.
 * @param eLogLevel message type
 * @param nServerCode  message severity code
 * @param strMsg text of the message
*/
CYBaseMessage::CYBaseMessage(const TString& strChannel, int nMsgType, int nServerCode, const TString& strMsg)
{
	this->m_strChannel = strChannel;
	this->m_nMsgType = nMsgType;
	this->m_nServerCode = nServerCode;
	this->m_strMsg = strMsg;
#ifdef WIN32
    m_nThreadId = GetCurrentThreadId();
#else
    m_nThreadId = (long int)pthread_self();
#endif
}

/**
 * @brief Get Message Type.
*/
int CYBaseMessage::GetMsgType() const
{
	return m_nMsgType;
}

/**
 * @brief Get Server Code.
*/
int CYBaseMessage::GetSeverCode() const
{
	return m_nServerCode;
}

/**
 * @brief Get Log Message Location.
*/
const TString& CYBaseMessage::GetFunction() const
{
	return m_strFunction;
}

/**
 * @brief Get Log Message.
*/
const TString& CYBaseMessage::GetMsg() const
{
	return m_strMsg;
}

/**
 * @brief Get Log Channel.
*/
const TString& CYBaseMessage::GetChannel() const
{
	return m_strChannel;
}

/**
 * @brief Get Log TimeStamp.
*/
const CYTimeStamps& CYBaseMessage::GetTimeStamp() const
{
	return m_objTimeStamp;
}

/**
 * @brief Get Thread Id.
*/
unsigned long CYBaseMessage::GetThreadId() const
{
	return m_nThreadId;
}

/**
 * @brief Get File Path.
*/
const TString& CYBaseMessage::GetFile() const
{
	return m_strFile;
}

/**
 * @brief Get Code Line.
*/
int CYBaseMessage::GetLine() const
{
	return m_nLine;
}

/**
 * @brief Set Channel.
*/
void CYBaseMessage::SetChannel(const TString& strChannel)
{
	m_strChannel = strChannel;
}

CYCOROUTINE_NAMESPACE_END
