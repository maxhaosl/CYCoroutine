#include "CYCoroutine/Engine/CYCoroutineEngineDefine.hpp"
#include "CYCoroutine/Threads/CYThread.hpp"
#include "Src/CYCoroutinePrivDefine.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

namespace
{
    size_t GetMaxCpuWorkers() noexcept
    {
        return static_cast<size_t>(CYThread::NumberOfCpu() * CPU_THREAD_POOL_WORKER_COUNT_FACTOR);
    }

    size_t GetMaxBackgroundWorkers() noexcept
    {
        return static_cast<size_t>(CYThread::NumberOfCpu() * BACKGROUD_THREAD_POOL_WORKER_COUNT_FACTOR);
    }

    constexpr auto DEFAULT_MAX_WORKER_WAIT_TIME = std::chrono::seconds(MAX_THREAD_POOL_WORKER_WAIT_TIME_SEC);

    [[maybe_unused]]std::once_flag  g_objTimerFlag;
    [[maybe_unused]]std::once_flag  g_objInlineFlag;
    [[maybe_unused]]std::once_flag  g_objThreadFlag;
    [[maybe_unused]]std::once_flag  g_objThreadPoolFlag;
    [[maybe_unused]]std::once_flag  g_objBackgroundFlag;

}// namespace

/*
    CYCoroutineOptions
*/
CYCoroutineOptions::CYCoroutineOptions() noexcept
    : maxCpuThreads(GetMaxCpuWorkers())
    , maxThreadPoolExecutorWaitTime(DEFAULT_MAX_WORKER_WAIT_TIME)
    , maxBackgroundThreads(GetMaxBackgroundWorkers())
    , maxBackgroundExecutorWaitTime(DEFAULT_MAX_WORKER_WAIT_TIME)
    , maxTimerQueueWaitTime(std::chrono::seconds(MAX_TIMER_QUEUE_WORKER_WAIT_TIME_SEC))
{
}

CYCOROUTINE_NAMESPACE_END