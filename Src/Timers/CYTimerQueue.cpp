#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYExecutor.hpp"
#include "CYCoroutine/Timers/CYTimer.hpp"
#include "CYCoroutine/Timers/CYTimerQueue.hpp"
#include "Src/Executors/CYExecutorDefine.hpp"

#include <set>
#include <unordered_map>

#include <cassert>

using namespace std::chrono;

CYCOROUTINE_NAMESPACE_BEGIN

namespace
{
    struct CYDeadlineComparator
    {
        bool operator()(const TimerPtr& ptrA, const TimerPtr& ptrB) const noexcept
        {
            return ptrA->GetDeadLine() < ptrB->GetDeadLine();
        }
    };

    class CYTimerQueueInternal
    {
        using TimerSet = std::multiset<TimerPtr, CYDeadlineComparator>;
        using TimerSetIterator = typename TimerSet::iterator;
        using IteratorMap = std::unordered_map<TimerPtr, TimerSetIterator>;

    private:
        TimerSet m_setTimers;
        IteratorMap m_mapIterator;

        void AddTimerInternal(TimerPtr ptrNewTimer)
        {
            assert(m_mapIterator.find(ptrNewTimer) == m_mapIterator.end());
            auto timer_it = m_setTimers.emplace(ptrNewTimer);
            m_mapIterator.emplace(std::move(ptrNewTimer), timer_it);
        }

        void RemoveTimerInternal(TimerPtr ptrExistTimer)
        {
            auto timer_it = m_mapIterator.find(ptrExistTimer);
            if (timer_it == m_mapIterator.end())
            {
                assert(ptrExistTimer->IsOneShot() || ptrExistTimer->Cancelled());  // the timer was already deleted by
                // the queue when it was fired.
                return;
            }

            auto set_iterator = timer_it->second;
            m_setTimers.erase(set_iterator);
            m_mapIterator.erase(timer_it);
        }

        void ProcessRequestQueue(RequestQueue& queue)
        {
            for (auto& request : queue)
            {
                auto& ptrTimer = request.first;
                const auto opt = request.second;

                if (opt == ETimerRequest::TYPE_TIMER_REQUEST_ADD)
                {
                    AddTimerInternal(std::move(ptrTimer));
                }
                else
                {
                    RemoveTimerInternal(std::move(ptrTimer));
                }
            }
        }

        void ResetContainersMemory() noexcept
        {
            assert(Empty());
            TimerSet timers;
            std::swap(m_setTimers, timers);
            IteratorMap iterator_mapper;
            std::swap(m_mapIterator, iterator_mapper);
        }

    public:
        bool Empty() const noexcept
        {
            assert(m_mapIterator.size() == m_setTimers.size());
            return m_setTimers.empty();
        }

        TimePoint ProcessTimers(RequestQueue& queue)
        {
            ProcessRequestQueue(queue);

            const auto now = high_resolution_clock::now();

            while (true)
            {
                if (m_setTimers.empty())
                {
                    break;
                }

                TimerSet temp_set;

                auto iterFirstMap = m_setTimers.begin();  // closest deadline
                auto ptrTimer = *iterFirstMap;
                const auto isOneShot = ptrTimer->IsOneShot();

                if (!ptrTimer->Expired(now))
                {
                    // if this timer is not Expired, the next ones are guaranteed not to, as
                    // the set is ordered by deadlines.
                    break;
                }

                // we are going to modify the timer, so first we extract it
                auto timer_node = m_setTimers.extract(iterFirstMap);

                // we cannot use the naked node_handle according to the standard. it must
                // be contained somewhere.
                auto temp_it = temp_set.insert(std::move(timer_node));

                // we fire it only if it's not cancelled
                const auto cancelled = ptrTimer->Cancelled();
                if (!cancelled)
                {
                    (*temp_it)->Fire();
                }

                if (isOneShot || cancelled)
                {
                    m_mapIterator.erase(ptrTimer);
                    continue;  // let the timer die inside temp_set
                }

                // regular timer, re-insert into the right position
                timer_node = temp_set.extract(temp_it);
                auto new_it = m_setTimers.insert(std::move(timer_node));
                // AppleClang doesn't have std::unordered_map::Contains yet
                assert(m_mapIterator.find(ptrTimer) != m_mapIterator.end());
                m_mapIterator[ptrTimer] = new_it;  // update the iterator map, multiset::extract invalidates the
                // timer
            }

            if (m_setTimers.empty())
            {
                ResetContainersMemory();
                return now + std::chrono::hours(24);
            }

            // get the closest deadline.
            return (**m_setTimers.begin()).GetDeadLine();
        }
    };
}  // namespace

//////////////////////////////////////////////////////////////////////////
CYTimerQueue::CYTimerQueue(milliseconds nMaxWaitTime, const FuncThreadDelegate& funThreadStartedCallback, const FuncThreadDelegate& funThreadTerminatedCallback)
    : m_funcStartedCallBack(funThreadStartedCallback)
    , m_funcTerminatedCallback(funThreadTerminatedCallback)
    , m_bAtomicAbort(false)
    , m_bAbort(false)
    , m_bIdle(true)
    , m_objMaxWaitingTime(nMaxWaitTime)

{
}

CYTimerQueue::~CYTimerQueue() noexcept
{
    ShutDown();
    assert(!m_objWorkerThread.Joinable());
}

void CYTimerQueue::AddInternalTimer(UniqueLock& lock, TimerPtr ptrNewTimer)
{
    assert(lock.owns_lock());
    m_lstRequestQueue.emplace_back(std::move(ptrNewTimer), ETimerRequest::TYPE_TIMER_REQUEST_ADD);
    lock.unlock();

    m_condition.notify_one();
}

void CYTimerQueue::RemoveInternalTimer(TimerPtr ptrExistTimer)
{
    {
        UniqueLock lock(m_lock);
        m_lstRequestQueue.emplace_back(std::move(ptrExistTimer), ETimerRequest::TYPE_TIMER_REQUEST_REMOVE);
    }

    m_condition.notify_one();
}

void CYTimerQueue::AddTimer(UniqueLock& lock, TimerPtr ptrNewTimer)
{
    assert(lock.owns_lock());

    IfTrueThrow(m_bAbort, TEXT("CYTimerQueue has been shut down."));

    auto old_thread = EnsureWorkerThread(lock);
    AddInternalTimer(lock, ptrNewTimer);

    if (old_thread.Joinable())
    {
        old_thread.Join();
    }
}

void CYTimerQueue::WorkLoop()
{
    TimePoint objNextDeadline;
    CYTimerQueueInternal internal_state;

    while (true)
    {
        UniqueLock lock(m_lock);
        if (internal_state.Empty())
        {
            const auto res = m_condition.wait_for(lock, m_objMaxWaitingTime, [this] {
                return !m_lstRequestQueue.empty() || m_bAbort;
                });

            if (!res)
            {
                m_bIdle = true;
                lock.unlock();
                return;
            }

        }
        else
        {
            m_condition.wait_until(lock, objNextDeadline, [this] {
                return !m_lstRequestQueue.empty() || m_bAbort;
                });
        }

        if (m_bAbort)
        {
            return;
        }

        auto RequestQueue = std::move(m_lstRequestQueue);
        lock.unlock();

        objNextDeadline = internal_state.ProcessTimers(RequestQueue);
        const auto now = ClockType::now();
        if (objNextDeadline <= now)
        {
            continue;
        }
    }
}

bool CYTimerQueue::ShutdownRequested() const noexcept
{
    return m_bAtomicAbort.load(std::memory_order_relaxed);
}

void CYTimerQueue::ShutDown()
{
    const auto bStateBefore = m_bAtomicAbort.exchange(true, std::memory_order_relaxed);
    if (bStateBefore)
    {
        return;  // CYTimerQueue has been shut down already.
    }

    UniqueLock lock(m_lock);
    m_bAbort = true;

    if (!m_objWorkerThread.Joinable())
    {
        return;  // nothing to shut down
    }

    m_lstRequestQueue.clear();
    lock.unlock();

    m_condition.notify_one();
    m_objWorkerThread.Join();
}

CYThread CYTimerQueue::EnsureWorkerThread(UniqueLock& lock)
{
    assert(lock.owns_lock());
    if (!m_bIdle)
    {
        return {};
    }

    auto objOldWorker = std::move(m_objWorkerThread);
    m_objWorkerThread = CYThread(MakeExecutorWorkerName("CYTimerQueue"), [this] {
        WorkLoop();
        }, m_funcStartedCallBack, m_funcTerminatedCallback);

    m_bIdle = false;
    return objOldWorker;
}

CYLazyResult<void> CYTimerQueue::MakeDelayObjectImpl(std::chrono::milliseconds nDueTime, SharePtr<CYTimerQueue> ptrSelf, SharePtr<CYExecutor> ptrExecutor)
{
    class CYDelayObjectAwaitable : public suspend_always
    {

    private:
        const size_t m_nDueTimeMs;
        CYTimerQueue& m_queueParent;
        SharePtr<CYExecutor> m_ptrExecutor;
        bool m_bInterrupted = false;

    public:
        CYDelayObjectAwaitable(size_t nDueTimeMs, CYTimerQueue& queueParent, SharePtr<CYExecutor> ptrExecutor) noexcept
            : m_nDueTimeMs(nDueTimeMs)
            , m_queueParent(queueParent)
            , m_ptrExecutor(std::move(ptrExecutor))

        {
        }

        void await_suspend(coroutine_handle<void> handleCoro) noexcept
        {
            try
            {
                m_queueParent.MakeTimerImpl(m_nDueTimeMs, 0, std::move(m_ptrExecutor), true, CYAwaitViaFunctor{ handleCoro, &m_bInterrupted });
            }
            catch (...)
            {
                // do nothing. ~CYAwaitViaFunctor will resume the coroutine and throw an exception.
            }
        }

        void await_resume() const
        {
            IfTrueThrow(m_bInterrupted, TEXT("CYResult - associated task was interrupted abnormally"));
        }
    };

    co_await CYDelayObjectAwaitable{ static_cast<size_t>(nDueTime.count()), *this, std::move(ptrExecutor) };
}

CYLazyResult<void> CYTimerQueue::MakeDelayObject(std::chrono::milliseconds nDueTime, SharePtr<CYExecutor> ptrExecutor)
{
    if (!static_cast<bool>(ptrExecutor))
    {
        throw std::invalid_argument("CYTimerQueue::MakeDelayObject() - CYExecutor is null.");
    }

    return MakeDelayObjectImpl(nDueTime, shared_from_this(), std::move(ptrExecutor));
}

milliseconds CYTimerQueue::MaxWorkerIdleTime() const noexcept
{
    return m_objMaxWaitingTime;
}

CYCOROUTINE_NAMESPACE_END