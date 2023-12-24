#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/Impl/CYResultState.hpp"
#include "CYCoroutine/Results/Impl/CYSharedResultState.hpp"

CYCOROUTINE_NAMESPACE_BEGIN

void CYResultStateBase::AssertDone() const noexcept
{
    assert(m_eResultState.load(std::memory_order_acquire) == EResultState::STATE_RESULT_PRODUCER_DONE);
}

void CYResultStateBase::Wait()
{
    const auto state = m_eResultState.load(std::memory_order_acquire);
    if (state == EResultState::STATE_RESULT_PRODUCER_DONE)
    {
        return;
    }

    auto eExpectedState = EResultState::STATE_RESULT_IDLE;
    const auto bIdle = m_eResultState.compare_exchange_strong(eExpectedState, EResultState::STATE_RESULT_CONSUMER_WAIT, std::memory_order_acq_rel, std::memory_order_acquire);

    if (!bIdle)
    {
        AssertDone();
        return;
    }

    while (true)
    {
        if (m_eResultState.load(std::memory_order_acquire) == EResultState::STATE_RESULT_PRODUCER_DONE)
        {
            break;
        }

        m_eResultState.wait(EResultState::STATE_RESULT_CONSUMER_WAIT, std::memory_order_acquire);
    }

    AssertDone();
}

bool CYResultStateBase::await(coroutine_handle<void> handleCaller) noexcept
{
    const auto state = m_eResultState.load(std::memory_order_acquire);
    if (state == EResultState::STATE_RESULT_PRODUCER_DONE)
    {
        return false;  // don't suspend
    }

    m_objConsumer.SetAwaitHandle(handleCaller);

    auto eExpectedState = EResultState::STATE_RESULT_IDLE;
    const auto idle = m_eResultState.compare_exchange_strong(eExpectedState, EResultState::STATE_RESULT_CONSUMER_SET, std::memory_order_acq_rel, std::memory_order_acquire);

    if (!idle)
    {
        AssertDone();
    }

    return idle;  // if idle = true, suspend
}

CYResultStateBase::EResultState CYResultStateBase::WhenAny(const SharePtr<CYWhenAnyContext>& ptrWhenAnyState) noexcept
{
    const auto state = m_eResultState.load(std::memory_order_acquire);
    if (state == EResultState::STATE_RESULT_PRODUCER_DONE)
    {
        return state;
    }

    m_objConsumer.SetWhenAnyContext(ptrWhenAnyState);

    auto eExpectedState = EResultState::STATE_RESULT_IDLE;
    const auto idle = m_eResultState.compare_exchange_strong(eExpectedState, EResultState::STATE_RESULT_CONSUMER_SET, std::memory_order_acq_rel, std::memory_order_acquire);

    if (!idle)
    {
        AssertDone();
    }

    return state;
}

void CYResultStateBase::Share(const SharePtr<CYSharedResultStateBase>& resultState) noexcept
{
    const auto state = m_eResultState.load(std::memory_order_acquire);
    if (state == EResultState::STATE_RESULT_PRODUCER_DONE)
    {
        return resultState->OnResultFinished();
    }

    m_objConsumer.SetSharedContext(resultState);

    auto eExpectedState = EResultState::STATE_RESULT_IDLE;
    const auto idle = m_eResultState.compare_exchange_strong(eExpectedState, EResultState::STATE_RESULT_CONSUMER_SET, std::memory_order_acq_rel, std::memory_order_acquire);

    if (idle)
    {
        return;
    }

    AssertDone();
    resultState->OnResultFinished();
}

void CYResultStateBase::TryRewindConsumer() noexcept
{
    const auto EResultState = m_eResultState.load(std::memory_order_acquire);
    if (EResultState != EResultState::STATE_RESULT_CONSUMER_SET)
    {
        return;
    }

    auto eExpectedConsumerState = EResultState::STATE_RESULT_CONSUMER_SET;
    const auto consumer = m_eResultState.compare_exchange_strong(eExpectedConsumerState, EResultState::STATE_RESULT_IDLE, std::memory_order_acq_rel, std::memory_order_acquire);

    if (!consumer)
    {
        AssertDone();
        return;
    }

    m_objConsumer.Clear();
}

CYCOROUTINE_NAMESPACE_END