#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/Impl/CYSharedResultState.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

CYSharedAwaitContext* CYSharedResultStateBase::ResultReadyConstant() noexcept
{
    return reinterpret_cast<CYSharedAwaitContext*>(-1);
}

EResultStatus CYSharedResultStateBase::Status() const noexcept
{
    return m_status.load(std::memory_order_acquire);
}

bool CYSharedResultStateBase::await(CYSharedAwaitContext& awaiter) noexcept
{
    while (true)
    {
        auto awaiter_before = m_awaiters.load(std::memory_order_acquire);
        if (awaiter_before == ResultReadyConstant())
        {
            return false;
        }

        awaiter.next = awaiter_before;
        const auto swapped = m_awaiters.compare_exchange_weak(awaiter_before, &awaiter, std::memory_order_acq_rel);
        if (swapped)
        {
            return true;
        }
    }
}

void CYSharedResultStateBase::Wait() noexcept
{
    if (Status() == EResultStatus::STATUS_RESULT_IDLE)
    {
        m_status.wait(EResultStatus::STATUS_RESULT_IDLE, std::memory_order_acquire);
    }
}

CYCOROUTINE_NAMESPACE_END