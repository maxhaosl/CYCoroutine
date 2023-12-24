#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Common/Exception/CYException.hpp"
#include "CYCoroutine/Executors/CYExecutor.hpp"
#include "CYCoroutine/Threads/CYThread.hpp"
#include "Src/Executors/CYExecutorDefine.hpp"
#include "CYCoroutine/Common/Structure/CYStringUtils.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

void ThrowRuntimeShutdownException(std::string_view strExecutorName)
{
    const auto error_msg = std::string(strExecutorName) + " - shutdown has been called on this CYExecutor.";
    IfTrueThrow(true, AtoT(error_msg.c_str()));
}

std::string MakeExecutorWorkerName(std::string_view strExecutorName)
{
    return std::string(strExecutorName) + " worker";
}

CYCOROUTINE_NAMESPACE_END