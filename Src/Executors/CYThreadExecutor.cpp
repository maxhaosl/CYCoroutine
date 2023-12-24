#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYThreadExecutor.hpp"
#include "Src/Executors/CYExecutorDefine.hpp"

using CYCOROUTINE_NAMESPACE::CYThreadExecutor;

CYThreadExecutor::CYThreadExecutor(const FuncThreadDelegate& funStartedCallBack, const FuncThreadDelegate& funTerminatedCallBack)
    : CYDerivableExecutor<CYThreadExecutor>("CYThreadExecutor")
    , m_bAbort(false)
    , m_bAtomicAbort(false)
    , m_funcStartedCallBack(funStartedCallBack)
    , m_funcTerminatedCallback(funTerminatedCallBack)
{
}


CYThreadExecutor::~CYThreadExecutor() noexcept
{
    assert(m_lstWorkers.empty());
    assert(m_lstLastRetired.empty());
}

void CYThreadExecutor::EnqueueImpl(UniqueLock& lock, CYTask& task)
{
    assert(lock.owns_lock());

    auto& new_thread = m_lstWorkers.emplace_front();
    new_thread = CYThread(MakeExecutorWorkerName(strName),
        [this, iter = m_lstWorkers.begin(), task = std::move(task)]() mutable {
            task();
            RetireWorker(iter);
        },
        m_funcStartedCallBack, m_funcTerminatedCallback);
}

void CYThreadExecutor::Enqueue(CYTask task)
{
    UniqueLock lock(m_lock);
    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(strName);
    }

    EnqueueImpl(lock, task);
}

void CYThreadExecutor::Enqueue(std::span<CYTask> tasks)
{
    UniqueLock lock(m_lock);
    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(strName);
    }

    for (auto& task : tasks)
    {
        EnqueueImpl(lock, task);
    }
}

int CYThreadExecutor::MaxConcurrencyLevel() const noexcept
{
    return THREAD_EXECUTOR_MAX_CONCURRENCY_LEVEL;
}

bool CYThreadExecutor::ShutdownRequested() const
{
    return m_bAtomicAbort.load(std::memory_order_relaxed);
}

void CYThreadExecutor::ShutDown()
{
    const auto abort = m_bAtomicAbort.exchange(true, std::memory_order_relaxed);
    if (abort)
    {
        return;  // shutdown had been called before.
    }

    UniqueLock lock(m_lock);
    m_bAbort = true;
    m_condition.wait(lock, [this] {
        return m_lstWorkers.empty();
        });

    if (m_lstLastRetired.empty())
    {
        return;
    }

    assert(m_lstLastRetired.size() == 1);
    m_lstLastRetired.front().Join();
    m_lstLastRetired.clear();
}

void CYThreadExecutor::RetireWorker(std::list<CYThread>::iterator it)
{
    UniqueLock lock(m_lock);
    auto lstLastRetired = std::move(m_lstLastRetired);
    m_lstLastRetired.splice(m_lstLastRetired.begin(), m_lstWorkers, it);

    lock.unlock();
    m_condition.notify_one();

    if (lstLastRetired.empty())
    {
        return;
    }

    assert(lstLastRetired.size() == 1);
    lstLastRetired.front().Join();
}