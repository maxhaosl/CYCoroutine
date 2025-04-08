#include "CYCoroutine/CYCoroutineDefine.hpp"

#include "CYCoroutine/Timers/CYTimer.hpp"
#include "CYCoroutine/Timers/CYTimerQueue.hpp"

#include "CYCoroutine/Common/Exception/CYException.hpp"
#include "CYCoroutine/Executors/CYExecutor.hpp"
#include "CYCoroutine/Results/CYResult.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
CYTimerStateBase::CYTimerStateBase(size_t nDueTime, size_t nFrequency, SharePtr<CYExecutor> ptrExecutor, WeakPtr<CYTimerQueue> ptrTimerQueue, bool isOneShot) noexcept
    : m_ptrTimerQueue(std::move(ptrTimerQueue))
    , m_ptrExecutor(std::move(ptrExecutor))
    , m_nDueTime(nDueTime)
    , m_nFrequency(nFrequency)
    , m_tpDeadLine(MakeDeadLine(milliseconds(nDueTime)))
    , m_bCancelled(false)
    , m_IsOneShot(isOneShot)
{
    assert(static_cast<bool>(m_ptrExecutor));
}

void CYTimerStateBase::Fire()
{
    const auto nFrequency = m_nFrequency.load(std::memory_order_relaxed);
    m_tpDeadLine = MakeDeadLine(milliseconds(nFrequency));

    assert(static_cast<bool>(m_ptrExecutor));

    m_ptrExecutor->Post([self = shared_from_this()]() mutable {
        self->Execute();
        });
}

//////////////////////////////////////////////////////////////////////////
CYTimer::CYTimer(SharePtr<CYTimerStateBase> ptrTimerImpl) noexcept
    : m_ptrState(std::move(ptrTimerImpl))
{
}

CYTimer::~CYTimer() noexcept
{
    Cancel();
}

void CYTimer::IfEmptyThrow(const char* pszErrorMessage) const
{
    if (static_cast<bool>(m_ptrState))
    {
        return;
    }

    IfTrueThrow(true, AtoT(pszErrorMessage));
}

milliseconds CYTimer::GetDueTime() const
{
    IfEmptyThrow("CYTimer::GetDueTime() - timer is empty.");
    return milliseconds(m_ptrState->GetDueTime());
}

milliseconds CYTimer::GetFrequency() const
{
    IfEmptyThrow("CYTimer::GetFrequency() - timer is empty.");
    return milliseconds(m_ptrState->GetFrequency());
}

SharePtr<CYExecutor> CYTimer::GetExecutor() const
{
    IfEmptyThrow("CYTimer::GetExecutor() - timer is empty.");
    return m_ptrState->GetExecutor();
}

WeakPtr<CYTimerQueue> CYTimer::GetTimerQueue() const
{
    IfEmptyThrow("CYTimer::GetTimerQueue() - timer is empty.");
    return m_ptrState->GetTimerQueue();
}

void CYTimer::Cancel()
{
    if (!static_cast<bool>(m_ptrState))
    {
        return;
    }

    auto state = std::move(m_ptrState);
    state->Cancel();

    auto timerQueue = state->GetTimerQueue().lock();

    if (!static_cast<bool>(timerQueue))
    {
        return;
    }

    timerQueue->RemoveInternalTimer(std::move(state));
}

void CYTimer::SetFrequency(milliseconds nFrequency)
{
    IfEmptyThrow("CYTimer::SetFrequency() - timer is empty.");
    return m_ptrState->SetNewFrequency(static_cast<size_t>(nFrequency.count()));
}

CYTimer& CYTimer::operator=(CYTimer&& rhs) noexcept
{
    if (this == &rhs)
    {
        return *this;
    }

    if (static_cast<bool>(*this))
    {
        Cancel();
    }

    m_ptrState = std::move(rhs.m_ptrState);
    return *this;
}

CYCOROUTINE_NAMESPACE_END