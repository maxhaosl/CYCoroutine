#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Engine/CYCoroutineEngine.hpp"
#include "CYCoroutine/Engine/CYCoroutineEngineDefine.hpp"
#include "CYCoroutine/Executors/CYInlineExecutor.hpp"
#include "CYCoroutine/Executors/CYManualExecutor.hpp"
#include "CYCoroutine/Executors/CYThreadExecutor.hpp"
#include "CYCoroutine/Executors/CYThreadPoolExecutor.hpp"
#include "CYCoroutine/Executors/CYWorkerThreadExecutor.hpp"
#include "CYCoroutine/Timers/CYTimerQueue.hpp"
#include "Src/CYCoroutinePrivDefine.hpp"
#include "Src/Engine/CYExecutorCollection.hpp"
#include "Src/Executors/CYExecutorDefine.hpp"

#include <algorithm>
#include <thread>
#ifdef WIN32
#include <xcall_once.h>
#endif

CYCOROUTINE_NAMESPACE_BEGIN

SharePtr<CYCoroutineEngine> CYCoroutineEngine::m_ptrInstance;
namespace
{
    std::once_flag  g_objTimerFlag;
    std::once_flag  g_objInlineFlag;
    std::once_flag  g_objThreadFlag;
    std::once_flag  g_objThreadPoolFlag;
    std::once_flag  g_objBackgroundFlag;
    SharePtr<std::once_flag>  g_ptrInstanceFlag = MakeShared<std::once_flag>();

}// namespace

/*
    CYCoroutineEngine
*/
CYCoroutineEngine::CYCoroutineEngine()
    : CYCoroutineEngine(CYCoroutineOptions())
{
}

CYCoroutineEngine::CYCoroutineEngine(const CYCoroutineOptions& options)
    : m_objEngineOptions(options)
{
    try
    {
        m_ptrRegisteredExecutors = MakeShared<CYExecutorCollection>();
    }
    catch (...)
    {
        std::abort();
    }
}

CYCoroutineEngine::~CYCoroutineEngine() noexcept
{
    try
    {
        if (m_ptrTimerQueue) m_ptrTimerQueue->ShutDown();
        if (m_ptrRegisteredExecutors) m_ptrRegisteredExecutors->ShutDownALL();
        m_ptrRegisteredExecutors.reset();
    }
    catch (...)
    {
        std::abort();
    }
}

SharePtr<CYTimerQueue> CYCoroutineEngine::TimerQueue() const noexcept
{
    std::call_once(g_objTimerFlag, [&]() {
        m_ptrTimerQueue = MakeShared<CYTimerQueue>(m_objEngineOptions.maxTimerQueueWaitTime, m_objEngineOptions.funStartedCallBack, m_objEngineOptions.funTerminatedCallBack);
        });

    return m_ptrTimerQueue;
}

SharePtr<CYInlineExecutor> CYCoroutineEngine::InlineExecutor() const noexcept
{
    std::call_once(g_objInlineFlag, [&]() {
        m_ptrInlineExecutor = MakeShared<CYInlineExecutor>();
        m_ptrRegisteredExecutors->RegisterExecutor(m_ptrInlineExecutor);
        });

    return m_ptrInlineExecutor;
}

SharePtr<CYThreadPoolExecutor> CYCoroutineEngine::ThreadPoolExecutor() const noexcept
{
    std::call_once(g_objThreadPoolFlag, [&]() {
        m_ptrThreadPoolExecutor = MakeShared<CYThreadPoolExecutor>("CYThreadPoolExecutor", m_objEngineOptions.maxCpuThreads, m_objEngineOptions.maxThreadPoolExecutorWaitTime, m_objEngineOptions.funStartedCallBack, m_objEngineOptions.funTerminatedCallBack);
        m_ptrRegisteredExecutors->RegisterExecutor(m_ptrThreadPoolExecutor);
        });

    return m_ptrThreadPoolExecutor;
}

SharePtr<CYThreadPoolExecutor> CYCoroutineEngine::BackgroundExecutor() const noexcept
{
    std::call_once(g_objBackgroundFlag, [&]() {
        m_ptrBackgroundExecutor = MakeShared<CYThreadPoolExecutor>("CYBackgroundExecutor", m_objEngineOptions.maxBackgroundThreads, m_objEngineOptions.maxBackgroundExecutorWaitTime, m_objEngineOptions.funStartedCallBack, m_objEngineOptions.funTerminatedCallBack);
        m_ptrRegisteredExecutors->RegisterExecutor(m_ptrBackgroundExecutor);
        });

    return m_ptrBackgroundExecutor;
}

SharePtr<CYThreadExecutor> CYCoroutineEngine::ThreadExecutor() const noexcept
{
    std::call_once(g_objThreadFlag, [&]() {
        m_ptrThreadExecutor = MakeShared<CYThreadExecutor>(m_objEngineOptions.funStartedCallBack, m_objEngineOptions.funTerminatedCallBack);
        m_ptrRegisteredExecutors->RegisterExecutor(m_ptrThreadExecutor);
        });

    return m_ptrThreadExecutor;
}

SharePtr<CYWorkerThreadExecutor> CYCoroutineEngine::MakeWorkerThreadExecutor()
{
    auto ptrExecutor = MakeShared<CYWorkerThreadExecutor>();
    m_ptrRegisteredExecutors->RegisterExecutor(ptrExecutor);
    return ptrExecutor;
}

SharePtr<CYManualExecutor> CYCoroutineEngine::MakeManualExecutor()
{
    auto ptrExecutor = MakeShared<CYManualExecutor>();
    m_ptrRegisteredExecutors->RegisterExecutor(ptrExecutor);
    return ptrExecutor;
}

std::tuple<unsigned int, unsigned int, unsigned int> CYCoroutineEngine::Version() noexcept
{
    return { COROUTINE_VERSION_MAJOR, COROUTINE_VERSION_MINOR, COROUTINE_VERSION_REVISION };
}

//////////////////////////////////////////////////////////////////////////
SharePtr<CYCoroutineEngine> CYCoroutineEngine::GetInstance()
{
    std::call_once(*g_ptrInstanceFlag.get(), [&]() {
        m_ptrInstance = MakeShared<CYCoroutineEngine>();
        });
    return m_ptrInstance;
}
void CYCoroutineEngine::FreeInstance()
{
    m_ptrInstance.reset();
    g_ptrInstanceFlag = MakeShared<std::once_flag>();
}
CYCOROUTINE_NAMESPACE_END
