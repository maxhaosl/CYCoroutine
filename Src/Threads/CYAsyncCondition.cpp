#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/CYResumeOn.hpp"
#include "CYCoroutine/Threads/CYAsyncCondition.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

/*
    CYAWaiter
*/
CYAWaiter::CYAWaiter(CYAsyncCondition& parent, CScopedAsyncLock& lock) noexcept
    : m_parent(parent)
    , m_lock(lock)
{
}

constexpr bool CYAWaiter::await_ready() const noexcept
{
    return false;
}

void CYAWaiter::await_suspend(coroutine_handle<void> handleCaller)
{
    m_handleCaller = handleCaller;

    UniqueLock lock(m_parent.m_lock);
    m_lock.UnLock();

    m_parent.m_awaiters.PushBack(*this);
}

void CYAWaiter::await_resume() const noexcept
{
}

void CYAWaiter::resume() noexcept
{
    assert(static_cast<bool>(m_handleCaller));
    assert(!m_handleCaller.done());
    m_handleCaller();
}

/*
    CYAsyncCondition
*/
CYAsyncCondition::~CYAsyncCondition() noexcept
{
#ifdef CYCOROUTINE_DEBUG_MODE
    UniqueLock lock(m_lock);
    assert(m_awaiters.Empty() && "CYAsyncCondition is deleted while being used.");
#endif
}

void CYAsyncCondition::VerifyAwaitParams(const SharePtr<CYExecutor>& ptrResumeExecutor, const CScopedAsyncLock& lock)
{
    if (!static_cast<bool>(ptrResumeExecutor))
    {
        throw std::invalid_argument("CYAsyncCondition::await() - ptrResumeExecutor is null.");
    }

    if (!lock.OwnsLock())
    {
        throw std::invalid_argument("CYAsyncCondition::await() - lock is unlocked.");
    }
}

CYLazyResult<void> CYAsyncCondition::AWaitImpl(SharePtr<CYExecutor> ptrResumeExecutor, CScopedAsyncLock& lock)
{
    co_await CYAWaiter(*this, lock);
    assert(!lock.OwnsLock());
    co_await ResumeOn(ptrResumeExecutor);  // TODO: optimize this when get_current_executor is available
    co_await lock.Lock(ptrResumeExecutor);
}

CYLazyResult<void> CYAsyncCondition::await(SharePtr<CYExecutor> ptrResumeExecutor, CScopedAsyncLock& lock)
{
    VerifyAwaitParams(ptrResumeExecutor, lock);
    return AWaitImpl(std::move(ptrResumeExecutor), lock);
}

void CYAsyncCondition::NotifyOne()
{
    UniqueLock lock(m_lock);
    const auto awaiter = m_awaiters.PopFront();
    lock.unlock();

    if (awaiter != nullptr)
    {
        awaiter->resume();
    }
}

void CYAsyncCondition::NotifyALL()
{
    UniqueLock lock(m_lock);
    auto awaiters = std::move(m_awaiters);
    lock.unlock();

    while (true)
    {
        const auto awaiter = awaiters.PopFront();
        if (awaiter == nullptr)
        {
            return;  // no more awaiters
        }

        awaiter->resume();
    }
}

CYCOROUTINE_NAMESPACE_END