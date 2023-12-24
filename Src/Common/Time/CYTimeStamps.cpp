#include "CYCoroutine/Common/Time/CYTimeStamps.hpp"
#include "CYCoroutine/Common/Exception/CYException.hpp"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <format>

CYCOROUTINE_NAMESPACE_BEGIN

/**
* Creates using a current time
*/
CYTimeStamps::CYTimeStamps()
{
	SetTime();
}

CYTimeStamps::~CYTimeStamps()
{
}

/**
* Sets itself to be a current time
*/
void CYTimeStamps::SetTime()
{
	m_strTime.erase();
	SetLocalTimeData();
}

/**
 * @brief Set Offset second.
*/
void CYTimeStamps::SetOffsetTime(int nOffsetSec)
{
	time_t ttime;
	struct tm objTM;

	memset(&objTM, 0, sizeof(objTM));
	objTM.tm_year = m_nYY - 1900;
	objTM.tm_mon = m_nMM - 1;
	objTM.tm_mday = m_nDD;
	objTM.tm_hour = m_nHR;
	objTM.tm_min = m_nMN;
	objTM.tm_sec = m_nSC;

	ttime = mktime(&objTM) + nOffsetSec;

	objTM = *localtime(&ttime);

	m_nYY = objTM.tm_year + 1900;
	m_nMM = objTM.tm_mon + 1;
	m_nDD = objTM.tm_mday;
	m_nHR = objTM.tm_hour;
	m_nMN = objTM.tm_min;
	m_nSC = objTM.tm_sec;
}

/**
 * @brief Set Local Time.
*/
inline void CYTimeStamps::SetLocalTimeData()
{
	auto now = std::chrono::system_clock::now();
	auto tt = std::chrono::system_clock::to_time_t(now);
	auto ptm = *std::localtime(&tt);

	auto now_sc = std::chrono::time_point_cast<std::chrono::seconds>(now);
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);

	m_nYY = ptm.tm_year + 1900;
	m_nMM = ptm.tm_mon + 1;
	m_nDD = ptm.tm_mday;
	m_nHR = ptm.tm_hour;
	m_nMN = ptm.tm_min;
	m_nSC = ptm.tm_sec;

	m_nMMN = static_cast<int>(abs(now_ms.time_since_epoch().count() - now_sc.time_since_epoch().count() * 1000));
}

const int64_t CYTimeStamps::GetTime() const
{
	int64_t dwTime = 0;
	dwTime = m_nYY - 2000;			// year
	dwTime = dwTime * 100 + m_nMM;	// month
	dwTime = dwTime * 100 + m_nDD;	// day
	dwTime = dwTime * 100 + m_nHR;	// hour
	dwTime = dwTime * 100 + m_nMN;	// minute
	dwTime = dwTime * 100 + m_nSC;	// minute

	return dwTime;
}

/**
 * @brief Get Time
*/
const TString CYTimeStamps::GetTimeStr() const
{
#ifdef WIN32
	return std::format(TEXT("{:0>4}{:0>2}{:0>2}{:0>2}{:0>2}{:0>2}"), m_nYY, m_nMM, m_nDD, m_nHR, m_nMN, m_nSC);
#else
    TOStringStream ss;
    ss << std::setw(4) << std::setfill(TEXT('0')) << m_nYY <<
        std::setw(1) << TEXT('-') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nMM <<
        std::setw(1) << TEXT('-') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nDD <<
        std::setw(1) << TEXT(' ') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nHR <<
        std::setw(1) << TEXT(':') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nMN <<
        std::setw(1) << TEXT(':') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nSC;
   return ss.str();
#endif
}

/**
* Formats the timestamp - YYYYMMDD hh:mm:ss:nnnnnn where nnnnnn is microseconds.
* @returns reference to a formatted time stamp
*/
const TString CYTimeStamps::ToString() const
{
#ifdef WIN32
	return std::format(TEXT("{:0>4}-{:0>2}-{:0>2} {:0>2}:{:0>2}:{:0>2}.{:0>3}"), m_nYY, m_nMM, m_nDD, m_nHR, m_nMN, m_nSC, m_nMMN);
#else
    TOStringStream ss;
    ss << std::setw(4) << std::setfill(TEXT('0')) << m_nYY <<
        std::setw(1) << TEXT('-') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nMM <<
        std::setw(1) << TEXT('-') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nDD <<
        std::setw(1) << TEXT(' ') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nHR <<
        std::setw(1) << TEXT(':') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nMN <<
        std::setw(1) << TEXT(':') <<
        std::setw(2) << std::setfill(TEXT('0')) << m_nSC <<
        std::setw(1) << TEXT('.') <<
        std::setw(3) << std::setfill(TEXT('0')) << m_nMMN;
   // return std::format(TEXT("{:0>4}-{:0>2}-{:0>2} {:0>2}:{:0>2}:{:0>2}.{:0>3}"), m_nYY, m_nMM, m_nDD, m_nHR, m_nMN, m_nSC, m_nMMN);
   return ss.str();
#endif
}

CYCOROUTINE_NAMESPACE_END
