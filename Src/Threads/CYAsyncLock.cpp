#include "CYCoroutine/Results/CYResumeOn.hpp"
#include "CYCoroutine/Threads/CYAsyncLock.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

/*
    CAsyncLockAwaiter
*/
CAsyncLockAwaiter::CAsyncLockAwaiter(CAsyncLock& parent, UniqueLock& lock) noexcept
    : m_parent(parent)
    , m_lock(std::move(lock))
{
}

constexpr bool CAsyncLockAwaiter::await_ready() const noexcept
{
    return false;
}

constexpr void CAsyncLockAwaiter::await_resume() const noexcept
{
}

void CAsyncLockAwaiter::await_suspend(coroutine_handle<void> hHandle)
{
    assert(static_cast<bool>(hHandle));
    assert(!hHandle.done());
    assert(!static_cast<bool>(m_handleResume));
    assert(m_lock.owns_lock());

    m_handleResume = hHandle;
    m_parent.m_lstAwaiters.PushBack(*this);

    auto lock = std::move(m_lock);  // will unlock underlying lock
}

void CAsyncLockAwaiter::retry() noexcept
{
    m_handleResume.resume();
}

/*
    CAsyncLock
*/

CAsyncLock::~CAsyncLock() noexcept
{
#ifdef CYCOROUTINE_DEBUG_MODE
    UniqueLock lock(m_mutexAwaiter);
    assert(!m_bLocked && "CAsyncLock is dstroyed while it's locked.");
#endif
}

CYLazyResult<CScopedAsyncLock> CAsyncLock::LockImpl(SharePtr<CYExecutor> ptrResumeExecutor, bool bWithRAIIGuard)
{
    auto resume_synchronously = true;  // indicates if the locking coroutine managed to lock the lock on first attempt

    while (true)
    {
        UniqueLock lock(m_mutexAwaiter);
        if (!m_bLocked)
        {
            m_bLocked = true;
            lock.unlock();
            break;
        }

        co_await CAsyncLockAwaiter(*this, lock);

        resume_synchronously = false;  // if we haven't managed to lock the lock on first attempt, we need to resume using ptrResumeExecutor
    }

    if (!resume_synchronously)
    {
        try
        {
            co_await ResumeOn(ptrResumeExecutor);
        }
        catch (...)
        {
            UniqueLock lock(m_mutexAwaiter);
            assert(m_bLocked);
            m_bLocked = false;
            const auto awaiter = m_lstAwaiters.PopFront();
            lock.unlock();

            if (awaiter != nullptr)
            {
                awaiter->retry();
            }

            throw;
        }
    }

#ifdef CYCOROUTINE_DEBUG_MODE
    const auto current_count = m_nThreadCountInCriticalSection.fetch_add(1, std::memory_order_relaxed);
    assert(current_count == 0);
#endif

    if (bWithRAIIGuard)
    {
        co_return CScopedAsyncLock(*this, std::adopt_lock);
    }

    co_return CScopedAsyncLock(*this, std::defer_lock);
}

CYLazyResult<CScopedAsyncLock> CAsyncLock::Lock(SharePtr<CYExecutor> ptrResumeExecutor)
{
    if (!static_cast<bool>(ptrResumeExecutor))
    {
        throw std::invalid_argument("CAsyncLock::lock() - given resume CYExecutor is null.");
    }

    return LockImpl(std::move(ptrResumeExecutor), true);
}

CYLazyResult<bool> CAsyncLock::TryLock()
{
    auto bRet = false;

    UniqueLock lock(m_mutexAwaiter);
    if (!m_bLocked)
    {
        m_bLocked = true;
        lock.unlock();
        bRet = true;
    }
    else
    {
        lock.unlock();
    }

#ifdef CYCOROUTINE_DEBUG_MODE
    if (bRet)
    {
        const auto current_count = m_nThreadCountInCriticalSection.fetch_add(1, std::memory_order_relaxed);
        assert(current_count == 0);
    }
#endif

    co_return bRet;
}

void CAsyncLock::UnLock()
{
    UniqueLock lock(m_mutexAwaiter);
    if (!m_bLocked)
    {  // trying to unlocked non-owned mutex
        lock.unlock();
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted), std::system_category(), "CAsyncLock::UnLock() - trying to unlock an unowned lock.");
    }

    m_bLocked = false;

#ifdef CYCOROUTINE_DEBUG_MODE
    const auto current_count = m_nThreadCountInCriticalSection.fetch_sub(1, std::memory_order_relaxed);
    assert(current_count == 1);
#endif

    const auto awaiter = m_lstAwaiters.PopFront();
    lock.unlock();

    if (awaiter != nullptr)
    {
        awaiter->retry();
    }
}

/*
 *  CScopedAsyncLock
 */
CScopedAsyncLock::CScopedAsyncLock(CScopedAsyncLock&& rhs) noexcept
    : m_pLock(std::exchange(rhs.m_pLock, nullptr))
    , m_bOwns(std::exchange(rhs.m_bOwns, false))
{
}

CScopedAsyncLock::CScopedAsyncLock(CAsyncLock& lock, std::defer_lock_t) noexcept : m_pLock(&lock), m_bOwns(false)
{
}

CScopedAsyncLock::CScopedAsyncLock(CAsyncLock& lock, std::adopt_lock_t) noexcept : m_pLock(&lock), m_bOwns(true)
{
}

CScopedAsyncLock::~CScopedAsyncLock() noexcept
{
    if (m_bOwns && m_pLock != nullptr)
    {
        m_pLock->UnLock();
    }
}

CYLazyResult<void> CScopedAsyncLock::Lock(SharePtr<CYExecutor> ptrResumeExecutor)
{
    if (!static_cast<bool>(ptrResumeExecutor))
    {
        throw std::invalid_argument("CScopedAsyncLock::lock() - given resume CYExecutor is null.");
    }

    if (m_pLock == nullptr)
    {
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted), std::system_category(), "CScopedAsyncLock::lock() - *this doesn't reference any CAsyncLock.");
    }
    else if (m_bOwns)
    {
        throw std::system_error(static_cast<int>(std::errc::resource_deadlock_would_occur), std::system_category(), "CScopedAsyncLock::lock() - *this is already locked.");
    }
    else
    {
        co_await m_pLock->LockImpl(std::move(ptrResumeExecutor), false);
        m_bOwns = true;
    }
}

CYLazyResult<bool> CScopedAsyncLock::TryLock()
{
    if (m_pLock == nullptr)
    {
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted), std::system_category(), "CScopedAsyncLock::TryLock() - *this doesn't reference any CAsyncLock.");
    }
    else if (m_bOwns)
    {
        throw std::system_error(static_cast<int>(std::errc::resource_deadlock_would_occur), std::system_category(), "CScopedAsyncLock::TryLock() - *this is already locked.");
    }
    else
    {
        m_bOwns = co_await m_pLock->TryLock();
    }

    co_return m_bOwns;
}

void CScopedAsyncLock::UnLock()
{
    if (!m_bOwns)
    {
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted), std::system_category(), "CScopedAsyncLock::UnLock() - trying to unlock an unowned lock.");
    }
    else if (m_pLock != nullptr)
    {
        m_pLock->UnLock();
        m_bOwns = false;
    }
}

bool CScopedAsyncLock::OwnsLock() const noexcept
{
    return m_bOwns;
}

CScopedAsyncLock::operator bool() const noexcept
{
    return OwnsLock();
}

void CScopedAsyncLock::Swap(CScopedAsyncLock& rhs) noexcept
{
    std::swap(m_pLock, rhs.m_pLock);
    std::swap(m_bOwns, rhs.m_bOwns);
}

CAsyncLock* CScopedAsyncLock::Release() noexcept
{
    m_bOwns = false;
    return std::exchange(m_pLock, nullptr);
}

CAsyncLock* CScopedAsyncLock::Mutex() const noexcept
{
    return m_pLock;
}

CYCOROUTINE_NAMESPACE_END