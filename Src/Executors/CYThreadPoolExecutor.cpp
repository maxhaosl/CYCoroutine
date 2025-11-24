#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYThreadPoolExecutor.hpp"
#include "CYCoroutine/Common/CYDebugString.hpp"
#include "CYCoroutine/Common/Structure/CYStringUtils.hpp"
#include "CYCoroutine/Results/Impl/CYBinarySemaphore.hpp"

#include <algorithm>

using CYCOROUTINE_NAMESPACE::CYThreadPoolExecutor;
using CYCOROUTINE_NAMESPACE::CYIdleWorkerSet;
using CYCOROUTINE_NAMESPACE::CYThreadPoolWorker;

CYCOROUTINE_NAMESPACE_BEGIN

namespace
{
    struct CYThreadPoolPerThreadData
    {
        size_t nThreadIndex;
        const size_t nThreadHashId;
        CYThreadPoolWorker* pPoolWorker;

        static size_t CalculateHashId() noexcept
        {
            const auto nThreadId = CYThread::GetVirtualId();
            const std::hash<size_t> hash;
            return hash(nThreadId);
        }

        CYThreadPoolPerThreadData() noexcept
            : pPoolWorker(nullptr)
            , nThreadIndex(static_cast<size_t>(-1))
            , nThreadHashId(CalculateHashId())
        {
        }
    };

    thread_local CYThreadPoolPerThreadData m_objThreadPoolData;
}  // namespace

class alignas(CACHE_LINE_ALIGNMENT) CYThreadPoolWorker
{
public:
    CYThreadPoolWorker(CYThreadPoolExecutor& objParentPool, size_t index, size_t nPoolSize, std::chrono::milliseconds maxIdleTime, const FuncThreadDelegate& funStartedCallBack, const FuncThreadDelegate& funTerminatedCallBack);

    CYThreadPoolWorker(CYThreadPoolWorker&& rhs) noexcept;
    ~CYThreadPoolWorker() noexcept;

    void EnqueueForeign(CYTask& task);
    void EnqueueForeign(std::span<CYTask> tasks);
    void EnqueueForeign(std::deque<CYTask>::iterator begin, std::deque<CYTask>::iterator end);
    void EnqueueForeign(std::span<CYTask>::iterator begin, std::span<CYTask>::iterator end);

    void EnqueueLocal(CYTask& task);
    void EnqueueLocal(std::span<CYTask> tasks);

    void ShutDown();
    bool AppearsEmpty() const noexcept;
    std::chrono::milliseconds MaxWorkerIdleTime() const noexcept;

private:
    void BalanceWork();

    bool WaitForTask(UniqueLock& lock);
    bool DrainQueueImpl();
    bool DrainQueue();

    void WorkLoop();
    void EnsureWorkerActive(bool bFirstEnqueuer, UniqueLock& lock);

private:
    bool m_bIdle;
    bool m_bAbort;
    const size_t m_nIndex;
    const size_t m_nPoolSize;
    const std::chrono::milliseconds m_maxIdleTime;
    const std::string m_strWorkerName;
    alignas(CACHE_LINE_ALIGNMENT) std::mutex m_lock;

    std::deque<CYTask> m_lstPublicTaskQueue;
    cy_binary_semaphore m_semaphore;

    std::deque<CYTask> m_lstPrivTaskQueue;
    std::vector<size_t> m_lstIdleWorker;
    std::atomic_bool m_bAtomicAbort;
    CYThreadPoolExecutor& m_objParentPool;

    CYThread m_thread;
    std::atomic_bool m_bTaskFoundOrAbort;
    const FuncThreadDelegate m_funcStartedCallBack;
    const FuncThreadDelegate m_funcTerminatedCallback;
};

//////////////////////////////////////////////////////////////////////////
CYIdleWorkerSet::CYIdleWorkerSet(size_t size)
    : m_nApproxSize(0)
    , m_ptrIdleFlags(MakeUnique<CYPaddedFlag[]>(size))
    , m_nSize(size)
{
}

void CYIdleWorkerSet::SetIdle(size_t nIdleThread) noexcept
{
    const auto beforeStatus = m_ptrIdleFlags[nIdleThread].eFlag.exchange(EIdlStatus::STATUS_IDLE_IDLE, std::memory_order_relaxed);
    if (beforeStatus == EIdlStatus::STATUS_IDLE_IDLE)
    {
        return;
    }

    m_nApproxSize.fetch_add(1, std::memory_order_relaxed);
}

void CYIdleWorkerSet::SetActive(size_t nIdleThread) noexcept
{
    const auto beforeStatus = m_ptrIdleFlags[nIdleThread].eFlag.exchange(EIdlStatus::STATUS_IDLE_ACTIVE, std::memory_order_relaxed);
    if (beforeStatus == EIdlStatus::STATUS_IDLE_ACTIVE)
    {
        return;
    }

    m_nApproxSize.fetch_sub(1, std::memory_order_relaxed);
}

bool CYIdleWorkerSet::TryAcquireFlag(size_t index) noexcept
{
    const auto workerStatus = m_ptrIdleFlags[index].eFlag.load(std::memory_order_relaxed);
    if (workerStatus == EIdlStatus::STATUS_IDLE_ACTIVE)
    {
        return false;
    }

    const auto beforeStatus = m_ptrIdleFlags[index].eFlag.exchange(EIdlStatus::STATUS_IDLE_ACTIVE, std::memory_order_relaxed);
    const auto swapped = (beforeStatus == EIdlStatus::STATUS_IDLE_IDLE);
    if (swapped)
    {
        m_nApproxSize.fetch_sub(1, std::memory_order_relaxed);
    }

    return swapped;
}

size_t CYIdleWorkerSet::FindIdleWorker(size_t nCallerIndex) noexcept
{
    if (m_nApproxSize.load(std::memory_order_relaxed) <= 0)
    {
        return static_cast<size_t>(-1);
    }

    const auto nStartPos = (nCallerIndex != static_cast<size_t>(-1)) ? nCallerIndex : (m_objThreadPoolData.nThreadHashId % m_nSize);

    for (size_t i = 0; i < m_nSize; i++)
    {
        const auto index = (nStartPos + i) % m_nSize;
        if (index == nCallerIndex)
        {
            continue;
        }

        if (TryAcquireFlag(index))
        {
            return index;
        }
    }

    return static_cast<size_t>(-1);
}

void CYIdleWorkerSet::FindIdleWorkers(size_t nCallerIndex, std::vector<size_t>& lstResultBuffer, size_t nMaxCount) noexcept
{
    assert(lstResultBuffer.capacity() >= nMaxCount);

    const auto nApproxSize = m_nApproxSize.load(std::memory_order_relaxed);
    if (nApproxSize <= 0)
    {
        return;
    }

    assert(nCallerIndex < m_nSize);
    assert(nCallerIndex == m_objThreadPoolData.nThreadIndex);

    size_t nCount = 0;
    const auto nMaxWaiters = std::min(static_cast<size_t>(nApproxSize), nMaxCount);

    for (size_t i = 0; (i < m_nSize) && (nCount < nMaxWaiters); i++)
    {
        const auto index = (nCallerIndex + i) % m_nSize;
        if (index == nCallerIndex)
        {
            continue;
        }

        if (TryAcquireFlag(index))
        {
            lstResultBuffer.emplace_back(index);
            ++nCount;
        }
    }
}

CYThreadPoolWorker::CYThreadPoolWorker(CYThreadPoolExecutor& objParentPool, size_t index, size_t nPoolSize, std::chrono::milliseconds maxIdleTime, const FuncThreadDelegate& funStartedCallBack, const FuncThreadDelegate& funTerminatedCallBack)
    : m_bAtomicAbort(false)
    , m_objParentPool(objParentPool)
    , m_nIndex(index)
    , m_nPoolSize(nPoolSize)
    , m_maxIdleTime(maxIdleTime)
    , m_strWorkerName(MakeExecutorWorkerName(objParentPool.strName))
    , m_semaphore(0)
    , m_bIdle(true)
    , m_bAbort(false)
    , m_bTaskFoundOrAbort(false)
    , m_funcStartedCallBack(funStartedCallBack)
    , m_funcTerminatedCallback(funTerminatedCallBack)
{
    m_lstIdleWorker.reserve(nPoolSize);
}

CYThreadPoolWorker::CYThreadPoolWorker(CYThreadPoolWorker&& rhs) noexcept
    : m_objParentPool(rhs.m_objParentPool)
    , m_nIndex(rhs.m_nIndex)
    , m_nPoolSize(rhs.m_nPoolSize)
    , m_maxIdleTime(rhs.m_maxIdleTime)
    , m_semaphore(0)
    , m_bIdle(true)
    , m_bAbort(true)
{
    std::abort();  // shouldn't be called
}

CYThreadPoolWorker::~CYThreadPoolWorker() noexcept
{
    assert(m_bIdle);
    assert(!m_thread.Joinable());
}

void CYThreadPoolWorker::BalanceWork()
{
    const auto nTaskCount = m_lstPrivTaskQueue.size();
    if (nTaskCount < 2)
    {  // no point in donating tasks
        return;
    }

    // we assume all threads but us are idle, we also save At least one CYTask for ourselves
    const auto nMaxIdleWorkerCount = std::min(m_nPoolSize - 1, nTaskCount - 1);
    if (nMaxIdleWorkerCount == 0)
    {
        return;  // a thread-pool with a single thread
    }

    m_objParentPool.FindIdleWorkers(m_nIndex, m_lstIdleWorker, nMaxIdleWorkerCount);
    const auto idle_count = m_lstIdleWorker.size();
    if (idle_count == 0)
    {
        return;
    }

    assert(idle_count <= nTaskCount);
    const auto nTotalWorkerCount = (idle_count + 1);  // nCount ourselves, otherwise we'll donate everything.
    const auto nDonationCount = nTaskCount / nTotalWorkerCount;
    auto nExtra = nTaskCount - nDonationCount * nTotalWorkerCount;

    size_t begin = 0;
    size_t end = nDonationCount;

    for (const auto nIdleWorkerIndex : m_lstIdleWorker)
    {
        assert(nIdleWorkerIndex != m_nIndex);
        assert(nIdleWorkerIndex < m_nPoolSize);
        assert(begin < nTaskCount);

        if (nExtra != 0)
        {
            end++;
            nExtra--;
        }

        assert(end <= nTaskCount);

        auto iterDonationBegin = m_lstPrivTaskQueue.begin() + begin;
        auto iterDonationEnd = m_lstPrivTaskQueue.begin() + end;

        assert(iterDonationBegin < m_lstPrivTaskQueue.end());
        assert(iterDonationEnd <= m_lstPrivTaskQueue.end());

        m_objParentPool.WorkerAt(nIdleWorkerIndex).EnqueueForeign(iterDonationBegin, iterDonationEnd);

        begin = end;
        end += nDonationCount;
    }

    assert(m_lstPrivTaskQueue.size() == nTaskCount);

    // clear everything we've donated.
    assert(std::all_of(m_lstPrivTaskQueue.begin(), m_lstPrivTaskQueue.begin() + begin, [](auto& task) {
        return !static_cast<bool>(task);
        }));

    assert(std::all_of(m_lstPrivTaskQueue.begin() + begin, m_lstPrivTaskQueue.end(), [](auto& task) {
        return static_cast<bool>(task);
        }));

    m_lstPrivTaskQueue.erase(m_lstPrivTaskQueue.begin(), m_lstPrivTaskQueue.begin() + begin);

    assert(!m_lstPrivTaskQueue.empty());

    m_lstIdleWorker.clear();
}

bool CYThreadPoolWorker::WaitForTask(UniqueLock& lock)
{
    assert(lock.owns_lock());

    if (!m_lstPublicTaskQueue.empty() || m_bAbort)
    {
        return true;
    }

    lock.unlock();

    m_objParentPool.MarkWorkerIdle(m_nIndex);

    auto event_found = false;
    const auto deadline = std::chrono::steady_clock::now() + m_maxIdleTime;

    while (true)
    {
        if (!m_semaphore.try_acquire_until(deadline))
        {
            if (std::chrono::steady_clock::now() <= deadline)
            {
                continue;  // handle spurious wake-ups
            }
            else
            {
                break;
            }
        }

        if (!m_bTaskFoundOrAbort.load(std::memory_order_relaxed))
        {
            continue;
        }

        lock.lock();
        if (m_lstPublicTaskQueue.empty() && !m_bAbort)
        {
            lock.unlock();
            continue;
        }

        event_found = true;
        break;
    }

    if (!lock.owns_lock())
    {
        lock.lock();
    }

    if (!event_found || m_bAbort)
    {
        m_bIdle = true;
        lock.unlock();
        return false;
    }

    assert(!m_lstPublicTaskQueue.empty());
    m_objParentPool.MarkWorkerActive(m_nIndex);
    return true;
}

bool CYThreadPoolWorker::DrainQueueImpl()
{
    auto aborted = false;

    while (!m_lstPrivTaskQueue.empty())
    {
        BalanceWork();

        if (m_bAtomicAbort.load(std::memory_order_relaxed))
        {
            aborted = true;
            break;
        }

        assert(!m_lstPrivTaskQueue.empty());
        auto task = std::move(m_lstPrivTaskQueue.back());
        m_lstPrivTaskQueue.pop_back();
        task();
    }

    if (aborted)
    {
        UniqueLock lock(m_lock);
        m_bIdle = true;
        return false;
    }

    return true;
}

bool CYThreadPoolWorker::DrainQueue()
{
    UniqueLock lock(m_lock);
    if (!WaitForTask(lock))
    {
        return false;
    }

    assert(lock.owns_lock());
    assert(!m_lstPublicTaskQueue.empty() || m_bAbort);

    m_bTaskFoundOrAbort.store(false, std::memory_order_relaxed);

    if (m_bAbort)
    {
        m_bIdle = true;
        return false;
    }

    assert(m_lstPrivTaskQueue.empty());
    std::swap(m_lstPrivTaskQueue, m_lstPublicTaskQueue);  // reuse underlying allocations.
    lock.unlock();

    return DrainQueueImpl();
}

void CYThreadPoolWorker::WorkLoop()
{
    m_objThreadPoolData.pPoolWorker = this;
    m_objThreadPoolData.nThreadIndex = m_nIndex;

    UniquePtr<CYBaseException> excp;
    try
    {
        while (true)
        {
            if (!DrainQueue())
            {
                return;
            }
        }
    }
    catch (CYBaseException* e)
    {
        excp.reset(e);
        DebugString(AtoT(excp->what()));
        UniqueLock lock(m_lock);
        m_bIdle = true;
    }
}

void CYThreadPoolWorker::EnsureWorkerActive(bool bFirstEnqueuer, UniqueLock& lock)
{
    assert(lock.owns_lock());

    if (!m_bIdle)
    {
        lock.unlock();

        if (bFirstEnqueuer)
        {
            m_semaphore.release();
        }

        return;
    }

    auto staleWorker = std::move(m_thread);
    m_thread = CYThread(m_strWorkerName,
        [this] {
            WorkLoop();
        },
        m_funcStartedCallBack,
        m_funcTerminatedCallback);

    m_bIdle = false;
    lock.unlock();

    if (staleWorker.Joinable())
    {
        staleWorker.Join();
    }
}

void CYThreadPoolWorker::EnqueueForeign(CYTask& task)
{
    UniqueLock lock(m_lock);
    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(m_objParentPool.strName);
    }

    m_bTaskFoundOrAbort.store(true, std::memory_order_relaxed);

    const auto is_empty = m_lstPublicTaskQueue.empty();
    m_lstPublicTaskQueue.emplace_back(std::move(task));
    EnsureWorkerActive(is_empty, lock);
}

void CYThreadPoolWorker::EnqueueForeign(std::span<CYTask> tasks)
{
    UniqueLock lock(m_lock);
    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(m_objParentPool.strName);
    }

    m_bTaskFoundOrAbort.store(true, std::memory_order_relaxed);

    const auto is_empty = m_lstPublicTaskQueue.empty();
    m_lstPublicTaskQueue.insert(m_lstPublicTaskQueue.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
    EnsureWorkerActive(is_empty, lock);
}

void CYThreadPoolWorker::EnqueueForeign(std::deque<CYTask>::iterator begin, std::deque<CYTask>::iterator end)
{
    UniqueLock lock(m_lock);
    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(m_objParentPool.strName);
    }

    m_bTaskFoundOrAbort.store(true, std::memory_order_relaxed);

    const auto is_empty = m_lstPublicTaskQueue.empty();
    m_lstPublicTaskQueue.insert(m_lstPublicTaskQueue.end(), std::make_move_iterator(begin), std::make_move_iterator(end));
    EnsureWorkerActive(is_empty, lock);
}

void CYThreadPoolWorker::EnqueueForeign(std::span<CYTask>::iterator begin, std::span<CYTask>::iterator end)
{
    UniqueLock lock(m_lock);
    if (m_bAbort)
    {
        ThrowRuntimeShutdownException(m_objParentPool.strName);
    }

    m_bTaskFoundOrAbort.store(true, std::memory_order_relaxed);

    const auto is_empty = m_lstPublicTaskQueue.empty();
    m_lstPublicTaskQueue.insert(m_lstPublicTaskQueue.end(), std::make_move_iterator(begin), std::make_move_iterator(end));
    EnsureWorkerActive(is_empty, lock);
}

void CYThreadPoolWorker::EnqueueLocal(CYTask& task)
{
    if (m_bAtomicAbort.load(std::memory_order_relaxed))
    {
        ThrowRuntimeShutdownException(m_objParentPool.strName);
    }

    m_lstPrivTaskQueue.emplace_back(std::move(task));
}

void CYThreadPoolWorker::EnqueueLocal(std::span<CYTask> tasks)
{
    if (m_bAtomicAbort.load(std::memory_order_relaxed))
    {
        ThrowRuntimeShutdownException(m_objParentPool.strName);
    }

    m_lstPrivTaskQueue.insert(m_lstPrivTaskQueue.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
}

void CYThreadPoolWorker::ShutDown()
{
    assert(!m_bAtomicAbort.load(std::memory_order_relaxed));
    m_bAtomicAbort.store(true, std::memory_order_relaxed);

    {
        UniqueLock lock(m_lock);
        m_bAbort = true;
    }

    m_bTaskFoundOrAbort.store(true, std::memory_order_relaxed);  // make sure the store is finished before notifying the worker.

    m_semaphore.release();

    if (m_thread.Joinable())
    {
        m_thread.Join();
    }

    decltype(m_lstPublicTaskQueue) lstPublicQueue;
    decltype(m_lstPrivTaskQueue) lstPrivateQueue;

    {
        UniqueLock lock(m_lock);
        lstPublicQueue = std::move(m_lstPublicTaskQueue);
        lstPrivateQueue = std::move(m_lstPrivTaskQueue);
    }

    lstPublicQueue.clear();
    lstPrivateQueue.clear();
}

std::chrono::milliseconds CYThreadPoolWorker::MaxWorkerIdleTime() const noexcept
{
    return m_maxIdleTime;
}

bool CYThreadPoolWorker::AppearsEmpty() const noexcept
{
    return m_lstPrivTaskQueue.empty() && !m_bTaskFoundOrAbort.load(std::memory_order_relaxed);
}

CYThreadPoolExecutor::CYThreadPoolExecutor(std::string_view strPoolName, size_t nPoolSize, std::chrono::milliseconds maxIdleTime, const FuncThreadDelegate& funStartedCallBack, const FuncThreadDelegate& funTerminatedCallBack)
    : CYDerivableExecutor<CYThreadPoolExecutor>(strPoolName)
    , m_nRoundRobinCursor(0)
    , m_objIdleWorkers(nPoolSize)
    , m_bAbort(false)
{
    m_lstWorkers.reserve(nPoolSize);

    for (size_t i = 0; i < nPoolSize; i++)
    {
        m_lstWorkers.emplace_back(*this, i, nPoolSize, maxIdleTime, funStartedCallBack, funTerminatedCallBack);
    }

    for (size_t i = 0; i < nPoolSize; i++)
    {
        m_objIdleWorkers.SetIdle(i);
    }
}

CYThreadPoolExecutor::~CYThreadPoolExecutor() = default;

void CYThreadPoolExecutor::FindIdleWorkers(size_t nCallerIndex, std::vector<size_t>& buffer, size_t nMaxCount) noexcept
{
    m_objIdleWorkers.FindIdleWorkers(nCallerIndex, buffer, nMaxCount);
}

CYThreadPoolWorker& CYThreadPoolExecutor::WorkerAt(size_t index) noexcept
{
    assert(index <= m_lstWorkers.size());
    return m_lstWorkers[index];
}

void CYThreadPoolExecutor::MarkWorkerIdle(size_t index) noexcept
{
    assert(index < m_lstWorkers.size());
    m_objIdleWorkers.SetIdle(index);
}

void CYThreadPoolExecutor::MarkWorkerActive(size_t index) noexcept
{
    assert(index < m_lstWorkers.size());
    m_objIdleWorkers.SetActive(index);
}

void CYThreadPoolExecutor::Enqueue(CYTask task)
{
    const auto pPoolWorker = m_objThreadPoolData.pPoolWorker;
    const auto nThisWorkerIndex = m_objThreadPoolData.nThreadIndex;

    if (pPoolWorker != nullptr && pPoolWorker->AppearsEmpty())
    {
        return pPoolWorker->EnqueueLocal(task);
    }

    const auto nIdleWorkerPos = m_objIdleWorkers.FindIdleWorker(nThisWorkerIndex);
    if (nIdleWorkerPos != static_cast<size_t>(-1))
    {
        return m_lstWorkers[nIdleWorkerPos].EnqueueForeign(task);
    }

    if (pPoolWorker != nullptr)
    {
        return pPoolWorker->EnqueueLocal(task);
    }

    const auto nNextWorker = m_nRoundRobinCursor.fetch_add(1, std::memory_order_relaxed) % m_lstWorkers.size();
    m_lstWorkers[nNextWorker].EnqueueForeign(task);
}

void CYThreadPoolExecutor::Enqueue(std::span<CYTask> tasks)
{
    if (m_objThreadPoolData.pPoolWorker != nullptr)
    {
        return m_objThreadPoolData.pPoolWorker->EnqueueLocal(tasks);
    }

    if (tasks.size() < m_lstWorkers.size())
    {
        for (auto& task : tasks)
        {
            Enqueue(std::move(task));
        }

        return;
    }

    const auto nTaskCount = tasks.size();
    const auto nTotalWorkerCount = m_lstWorkers.size();
    const auto nDonationCount = nTaskCount / nTotalWorkerCount;
    auto nExtra = nTaskCount - nDonationCount * nTotalWorkerCount;

    size_t begin = 0;
    size_t end = nDonationCount;

    for (size_t i = 0; i < nTotalWorkerCount; i++)
    {
        assert(begin < nTaskCount);

        if (nExtra != 0)
        {
            end++;
            nExtra--;
        }

        assert(end <= nTaskCount);

        auto iterTasksBegin = tasks.begin() + begin;
        auto iterTasksEnd = tasks.begin() + end;

        assert(iterTasksBegin < tasks.end());
        assert(iterTasksEnd <= tasks.end());

        m_lstWorkers[i].EnqueueForeign(iterTasksBegin, iterTasksEnd);

        begin = end;
        end += nDonationCount;
    }
}

int CYThreadPoolExecutor::MaxConcurrencyLevel() const noexcept
{
    return static_cast<int>(m_lstWorkers.size());
}

bool CYThreadPoolExecutor::ShutdownRequested() const
{
    return m_bAbort.load(std::memory_order_relaxed);
}

void CYThreadPoolExecutor::ShutDown()
{
    const auto abort = m_bAbort.exchange(true, std::memory_order_relaxed);
    if (abort)
    {
        return;  // shutdown had been called before.
    }

    for (auto& worker : m_lstWorkers)
    {
        worker.ShutDown();
    }
}

std::chrono::milliseconds CYThreadPoolExecutor::MaxWorkerIdleTime() const noexcept
{
    return m_lstWorkers[0].MaxWorkerIdleTime();
}

CYCOROUTINE_NAMESPACE_END