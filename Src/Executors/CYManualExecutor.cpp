#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYManualExecutor.hpp"
#include "Src/Executors/CYExecutorDefine.hpp"

using CYCOROUTINE_NAMESPACE::CYManualExecutor;

CYCOROUTINE_NAMESPACE_BEGIN

CYManualExecutor::CYManualExecutor()
    : CYDerivableExecutor<CYManualExecutor>("CYManualExecutor")
    , m_bAbort(false)
    , m_bAtomicAbort(false)
{
}

void CYManualExecutor::Enqueue(CYTask task)
{
    UniqueLock lock(m_lock);
    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(strName);
    }

    m_lstTasks.emplace_back(std::move(task));
    lock.unlock();

    m_condition.notify_all();
}

void CYManualExecutor::Enqueue(std::span<CYTask> tasks)
{
    UniqueLock lock(m_lock);
    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(strName);
    }

    m_lstTasks.insert(m_lstTasks.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
    lock.unlock();

    m_condition.notify_all();
}

int CYManualExecutor::MaxConcurrencyLevel() const noexcept
{
    return MANUAL_EXECUTOR_MAX_CONCURRENCY_LEVEL;
}

size_t CYManualExecutor::size() const
{
    UniqueLock lock(m_lock);
    return m_lstTasks.size();
}

bool CYManualExecutor::Empty() const
{
    return size() == 0;
}

size_t CYManualExecutor::LoopImpl(size_t nMaxCount)
{
    if (nMaxCount == 0)
    {
        return 0;
    }

    size_t executed = 0;

    while (true)
    {
        if (executed == nMaxCount)
        {
            break;
        }

        UniqueLock lock(m_lock);
        if (m_bAbort)
        {
            break;
        }

        if (m_lstTasks.empty())
        {
            break;
        }

        auto task = std::move(m_lstTasks.front());
        m_lstTasks.pop_front();
        lock.unlock();

        task();
        ++executed;
    }

    if (ShutdownRequested())
    {
        ThrowRuntimeShutdownException(strName);
    }

    return executed;
}

size_t CYManualExecutor::LoopUntilImpl(size_t nMaxCount, std::chrono::time_point<std::chrono::system_clock> deadline)
{
    if (nMaxCount == 0)
    {
        return 0;
    }

    size_t executed = 0;
    deadline += std::chrono::milliseconds(1);

    while (true)
    {
        if (executed == nMaxCount)
        {
            break;
        }

        const auto now = std::chrono::system_clock::now();
        if (now >= deadline)
        {
            break;
        }

        UniqueLock lock(m_lock);
        const auto found_task = m_condition.wait_until(lock, deadline, [this] {
            return !m_lstTasks.empty() || m_bAbort;
            });

        if (m_bAbort)
        {
            break;
        }

        if (!found_task)
        {
            break;
        }

        assert(!m_lstTasks.empty());
        auto task = std::move(m_lstTasks.front());
        m_lstTasks.pop_front();
        lock.unlock();

        task();
        ++executed;
    }

    if (ShutdownRequested())
    {
        ThrowRuntimeShutdownException(strName);
    }

    return executed;
}

void CYManualExecutor::WaitForTasksImpl(size_t nCount)
{
    if (nCount == 0)
    {
        if (ShutdownRequested())
        {
            ThrowRuntimeShutdownException(strName);
        }
        return;
    }

    UniqueLock lock(m_lock);
    m_condition.wait(lock, [this, nCount] {
        return (m_lstTasks.size() >= nCount) || m_bAbort;
        });

    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(strName);
    }

    assert(m_lstTasks.size() >= nCount);
}

size_t CYManualExecutor::WaitForTasksImpl(size_t nCount, std::chrono::time_point<std::chrono::system_clock> deadline)
{
    deadline += std::chrono::milliseconds(1);

    UniqueLock lock(m_lock);
    m_condition.wait_until(lock, deadline, [this, nCount]
        {
            return (m_lstTasks.size() >= nCount) || m_bAbort;
        });

    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(strName);
    }

    return m_lstTasks.size();
}

bool CYManualExecutor::LoopOnce()
{
    return LoopImpl(1) != 0;
}

bool CYManualExecutor::LoopOnceFor(std::chrono::milliseconds nMaxWaitTime)
{
    if (nMaxWaitTime == std::chrono::milliseconds(0))
    {
        return LoopImpl(1) != 0;
    }

    return LoopUntilImpl(1, TimePointFromNow(nMaxWaitTime));
}

size_t CYManualExecutor::Loop(size_t nMaxCount)
{
    return LoopImpl(nMaxCount);
}

size_t CYManualExecutor::LoopFor(size_t nMaxCount, std::chrono::milliseconds nMaxWaitTime)
{
    if (nMaxCount == 0)
    {
        return 0;
    }

    if (nMaxWaitTime == std::chrono::milliseconds(0))
    {
        return LoopImpl(nMaxCount);
    }

    return LoopUntilImpl(nMaxCount, TimePointFromNow(nMaxWaitTime));
}

size_t CYManualExecutor::Clear()
{
    UniqueLock lock(m_lock);
    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(strName);
    }

    const auto tasks = std::move(m_lstTasks);
    lock.unlock();
    return tasks.size();
}

void CYManualExecutor::WaitForTask()
{
    WaitForTasksImpl(1);
}

bool CYManualExecutor::WaitForTaskFor(std::chrono::milliseconds nMaxWaitTime)
{
    return WaitForTasksImpl(1, TimePointFromNow(nMaxWaitTime)) == 1;
}

void CYManualExecutor::WaitForTasks(size_t nCount)
{
    WaitForTasksImpl(nCount);
}

size_t CYManualExecutor::WaitForTasksFor(size_t nCount, std::chrono::milliseconds nMaxWaitTime)
{
    return WaitForTasksImpl(nCount, TimePointFromNow(nMaxWaitTime));
}

void CYManualExecutor::ShutDown()
{
    const auto abort = m_bAtomicAbort.exchange(true, std::memory_order_relaxed);
    if (abort)
    {
        return;  // shutdown had been called before.
    }

    decltype(m_lstTasks) tasks;
    {
        UniqueLock lock(m_lock);
        m_bAbort = true;
        tasks = std::move(m_lstTasks);
    }

    m_condition.notify_all();

    tasks.clear();
}

bool CYManualExecutor::ShutdownRequested() const
{
    return m_bAtomicAbort.load(std::memory_order_relaxed);
}

CYCOROUTINE_NAMESPACE_END