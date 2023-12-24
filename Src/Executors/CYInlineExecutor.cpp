#include "CYCoroutine/Executors/CYInlineExecutor.hpp"
#include "Src/Executors/CYExecutorDefine.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

CYInlineExecutor::CYInlineExecutor() noexcept
    : CYExecutor("CYInlineExecutor")
    , m_bAbort(false)
{
}

void CYInlineExecutor::Enqueue(CYTask task)
{
    IfAbortThrow();
    task();
}

void CYInlineExecutor::Enqueue(std::span<CYTask> tasks)
{
    IfAbortThrow();
    for (auto& task : tasks)
    {
        task();
    }
}

int CYInlineExecutor::MaxConcurrencyLevel() const noexcept
{
    return INLINE_EXECUTOR_MAX_CONCURRENCY_LEVEL;
}

void CYInlineExecutor::ShutDown()
{
    m_bAbort.store(true, std::memory_order_relaxed);
}

bool CYInlineExecutor::ShutdownRequested() const
{
    return m_bAbort.load(std::memory_order_relaxed);
}

void CYInlineExecutor::IfAbortThrow() const
{
    if (m_bAbort.load(std::memory_order_relaxed))
    {
        ThrowRuntimeShutdownException(strName);
    }
}

CYCOROUTINE_NAMESPACE_END