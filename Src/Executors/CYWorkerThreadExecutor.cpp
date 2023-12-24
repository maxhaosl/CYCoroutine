#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYWorkerThreadExecutor.hpp"
#include "Src/Executors/CYExecutorDefine.hpp"

static thread_local CYCOROUTINE_NAMESPACE::CYWorkerThreadExecutor* s_tl_this_worker = nullptr;

using CYCOROUTINE_NAMESPACE::CYWorkerThreadExecutor;

CYWorkerThreadExecutor::CYWorkerThreadExecutor(const FuncThreadDelegate& funStartedCallBack, const FuncThreadDelegate& funTerminatedCallBack)
    : CYDerivableExecutor<CYWorkerThreadExecutor>("CYWorkerThreadExecutor")
    , m_bPrivateAbort(false)
    , m_semaphore(0)
    , m_bAtomicAbort(false)
    , m_bPublicAbort(false)
    , m_funcStartedCallBack(funStartedCallBack)
    , m_funcTerminatedCallback(funTerminatedCallBack)
{
}

void CYWorkerThreadExecutor::MakeOSWorkerThread()
{
    m_thread = CYThread(MakeExecutorWorkerName(strName), [this] {
        WorkLoop();
        },
        m_funcStartedCallBack,
        m_funcTerminatedCallback);
}

bool CYWorkerThreadExecutor::DrainQueueImpl()
{
    while (!m_lstPrivTaskQueue.empty())
    {
        auto task = std::move(m_lstPrivTaskQueue.front());
        m_lstPrivTaskQueue.pop_front();

        if (m_bPrivateAbort.load(std::memory_order_relaxed))
        {
            return false;
        }

        task();
    }

    return true;
}

void CYWorkerThreadExecutor::WaitForTask(UniqueLock& lock)
{
    assert(lock.owns_lock());
    if (!m_lstPublicTaskQueue.empty() || m_bPublicAbort)
    {
        return;
    }

    while (true)
    {
        lock.unlock();

        m_semaphore.acquire();

        lock.lock();
        if (!m_lstPublicTaskQueue.empty() || m_bPublicAbort)
        {
            break;
        }
    }
}

bool CYWorkerThreadExecutor::DrainQueue()
{
    UniqueLock lock(m_lock);
    WaitForTask(lock);

    assert(lock.owns_lock());
    assert(!m_lstPublicTaskQueue.empty() || m_bPublicAbort);

    if (m_bPublicAbort)
    {
        return false;
    }

    assert(m_lstPrivTaskQueue.empty());
    std::swap(m_lstPrivTaskQueue, m_lstPublicTaskQueue);  // reuse underlying allocations.
    lock.unlock();

    return DrainQueueImpl();
}

void CYWorkerThreadExecutor::WorkLoop()
{
    s_tl_this_worker = this;

    while (true)
    {
        if (!DrainQueue())
        {
            return;
        }
    }
}

void CYWorkerThreadExecutor::EnqueueLocal(CYTask& task)
{
    if (m_bPrivateAbort.load(std::memory_order_relaxed))
    {
        ThrowRuntimeShutdownException(strName);
    }

    m_lstPrivTaskQueue.emplace_back(std::move(task));
}

void CYWorkerThreadExecutor::EnqueueLocal(std::span<CYTask> tasks)
{
    if (m_bPrivateAbort.load(std::memory_order_relaxed))
    {
        ThrowRuntimeShutdownException(strName);
    }

    m_lstPrivTaskQueue.insert(m_lstPrivTaskQueue.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
}

void CYWorkerThreadExecutor::EnqueueForeign(CYTask& task)
{
    UniqueLock lock(m_lock);
    if (m_bPublicAbort)
    {
        ThrowRuntimeShutdownException(strName);
    }

    const auto is_empty = m_lstPublicTaskQueue.empty();
    m_lstPublicTaskQueue.emplace_back(std::move(task));

    if (!m_thread.Joinable())
    {
        return MakeOSWorkerThread();
    }

    lock.unlock();

    if (is_empty)
    {
        m_semaphore.release();
    }
}

void CYWorkerThreadExecutor::EnqueueForeign(std::span<CYTask> tasks)
{
    UniqueLock lock(m_lock);
    if (m_bPublicAbort)
    {
        ThrowRuntimeShutdownException(strName);
    }

    const auto is_empty = m_lstPublicTaskQueue.empty();
    m_lstPublicTaskQueue.insert(m_lstPublicTaskQueue.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));

    if (!m_thread.Joinable())
    {
        return MakeOSWorkerThread();
    }

    lock.unlock();

    if (is_empty)
    {
        m_semaphore.release();
    }
}

void CYWorkerThreadExecutor::Enqueue(CYTask task)
{
    if (s_tl_this_worker == this)
    {
        return EnqueueLocal(task);
    }

    EnqueueForeign(task);
}

void CYWorkerThreadExecutor::Enqueue(std::span<CYTask> tasks)
{
    if (s_tl_this_worker == this)
    {
        return EnqueueLocal(tasks);
    }

    EnqueueForeign(tasks);
}

int CYWorkerThreadExecutor::MaxConcurrencyLevel() const noexcept
{
    return WORKER_THREAD_MAX_CONCURRENCY_LEVEL;
}

bool CYWorkerThreadExecutor::ShutdownRequested() const
{
    return m_bAtomicAbort.load(std::memory_order_relaxed);
}

void CYWorkerThreadExecutor::ShutDown()
{
    const auto abort = m_bAtomicAbort.exchange(true, std::memory_order_relaxed);
    if (abort)
    {
        return;  // shutdown had been called before.
    }

    {
        UniqueLock lock(m_lock);
        m_bPublicAbort = true;
    }

    m_bPrivateAbort.store(true, std::memory_order_relaxed);
    m_semaphore.release();

    if (m_thread.Joinable())
    {
        m_thread.Join();
    }

    decltype(m_lstPrivTaskQueue) private_queue;
    decltype(m_lstPublicTaskQueue) public_queue;

    {
        UniqueLock lock(m_lock);
        private_queue = std::move(m_lstPrivTaskQueue);
        public_queue = std::move(m_lstPublicTaskQueue);
    }

    private_queue.clear();
    public_queue.clear();
}
