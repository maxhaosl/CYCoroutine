#include "CYCoroutine/Common/CYDebugString.hpp"

#include <cstdarg>
#include <time.h>

#if	defined(_DEBUG) || defined(_WIN32)
#include <windows.h>
#endif

CYCOROUTINE_NAMESPACE_BEGIN

/**
 * @brief Print debug string for windows.
*/
void DebugString(const TChar* format, ...)
{
#if	defined(_DEBUG) || defined(_WIN32)
	TChar	buffer[512];
	memset(buffer, 0, sizeof(buffer));

	va_list	args;
	va_start(args, format);
	cy_vsnprintf_s(buffer, _countof(buffer), format, args);
	va_end(args);

	SYSTEMTIME	st;
	GetLocalTime(&st);

	TChar szMessage[512];
	cy_sprintf_s(szMessage, 512, TEXT("[%04d/%02d/%02d %02d:%02d:%02d] %s"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, buffer);
	OutputDebugString(szMessage);
#endif	//_DEBUG
}

CYCOROUTINE_NAMESPACE_END