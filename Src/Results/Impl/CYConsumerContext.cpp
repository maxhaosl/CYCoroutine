#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Executors/CYExecutor.hpp"
#include "CYCoroutine/Results/Impl/CYConsumerContext.hpp"
#include "CYCoroutine/Results/Impl/CYSharedResultState.hpp"
#include "CYCoroutine/Results/Impl/CYBinarySemaphore.hpp"

using CYCOROUTINE_NAMESPACE::CYWhenAnyContext;
using CYCOROUTINE_NAMESPACE::CYConsumerContext;
using CYCOROUTINE_NAMESPACE::CYAwaitViaFunctor;
using CYCOROUTINE_NAMESPACE::CYResultStateBase;

CYCOROUTINE_NAMESPACE_BEGIN

template<class TYPE, class... ARGS_TYPE>
void Build(TYPE& o, ARGS_TYPE&&... args) noexcept
{
    new (std::addressof(o)) TYPE(std::forward<ARGS_TYPE>(args)...);
}

template<class TYPE>
void Destroy(TYPE& o) noexcept
{
    o.~TYPE();
}

CYCOROUTINE_NAMESPACE_END

/*
 * CYAwaitViaFunctor
 */

    CYAwaitViaFunctor::CYAwaitViaFunctor(coroutine_handle<void> handleCaller, bool* pbInterrupted) noexcept
    : m_handleCaller(handleCaller)
    , m_pbInterrupted(pbInterrupted)
{
    assert(static_cast<bool>(handleCaller));
    assert(!handleCaller.done());
    assert(pbInterrupted != nullptr);
}

CYAwaitViaFunctor::CYAwaitViaFunctor(CYAwaitViaFunctor&& rhs) noexcept
    : m_handleCaller(std::exchange(rhs.m_handleCaller, {}))
    , m_pbInterrupted(std::exchange(rhs.m_pbInterrupted, nullptr))
{
}

CYAwaitViaFunctor ::~CYAwaitViaFunctor() noexcept
{
    if (m_pbInterrupted == nullptr)
    {
        return;
    }

    *m_pbInterrupted = true;
    m_handleCaller();
}

void CYAwaitViaFunctor::operator()() noexcept
{
    assert(m_pbInterrupted != nullptr);
    m_pbInterrupted = nullptr;
    m_handleCaller();
}

/*
 * CYWhenAnyContext
 */

 /*
  *   k_pProcessing -> k_pDoneProcessing -> (completed) CYResultStateBase*
  *     |                                             ^
  *     |                                             |
  *     ----------------------------------------------
  */

const CYResultStateBase* CYWhenAnyContext::k_pProcessing = reinterpret_cast<CYResultStateBase*>(-1);
const CYResultStateBase* CYWhenAnyContext::k_pDoneProcessing = nullptr;

CYWhenAnyContext::CYWhenAnyContext(coroutine_handle<void> handleCoro) noexcept
    : m_status(k_pProcessing)
    , m_coro_handle(handleCoro)
{
    assert(static_cast<bool>(handleCoro));
    assert(!handleCoro.done());
}

bool CYWhenAnyContext::AnyResultFinished() const noexcept
{
    const auto status = m_status.load(std::memory_order_acquire);
    assert(status != k_pDoneProcessing);
    return status != k_pProcessing;
}

bool CYWhenAnyContext::FinishProcessing() noexcept
{
    assert(m_status.load(std::memory_order_relaxed) != k_pDoneProcessing);

    // tries to turn k_pProcessing -> k_pDoneProcessing.
    auto expected_state = k_pProcessing;
    const auto res = m_status.compare_exchange_strong(expected_state, k_pDoneProcessing, std::memory_order_acq_rel);
    return res;  // if k_pProcessing -> k_pDoneProcessing, then no CYResult finished before the CAS, suspend.
}

void CYWhenAnyContext::TryResume(CYResultStateBase& CompletedResult) noexcept
{
    /*
     * tries to turn m_status into the CompletedResult ptr
     * if m_status == k_pProcessing, we just leave the pointer and bail out, the processor thread will pick
     *  the pointer up and resume from there
     * if m_status == k_pDoneProcessing AND we were able to CAS it into the completed CYResult
     *   then we were the first ones to complete, processing is done for all input-results
     *   and we resume the caller
     */

    while (true)
    {
        auto status = m_status.load(std::memory_order_acquire);
        if (status != k_pProcessing && status != k_pDoneProcessing)
        {
            return;  // another CYTask finished before us, bail out
        }

        if (status == k_pDoneProcessing)
        {
            const auto swapped = m_status.compare_exchange_strong(status, &CompletedResult, std::memory_order_acq_rel);

            if (!swapped)
            {
                return;  // another CYTask finished before us, bail out
            }

            // k_pDoneProcessing -> CYResultStateBase ptr, we are the first to finish and CAS the status
            m_coro_handle();
            return;
        }

        assert(status == k_pProcessing);
        const auto res = m_status.compare_exchange_strong(status, &CompletedResult, std::memory_order_acq_rel);

        if (res)
        {  // k_pProcessing -> completed CYResultStateBase*
            return;
        }

        // either another CYResult raced us, either m_status is now k_pDoneProcessing, retry and act accordingly
    }
}

bool CYWhenAnyContext::ResumeInline(CYResultStateBase& CompletedResult) noexcept
{
    auto status = m_status.load(std::memory_order_acquire);
    assert(status != k_pDoneProcessing);

    if (status != k_pProcessing)
    {
        return false;
    }

    // either we succeed turning k_pProcessing to &CompletedResult, then we can resume inline, either we failed
    // meaning another thread had turned k_pProcessing -> &CompletedResult, either way, testing if the cas succeeded
    // is redundant as we need to resume inline.
    m_status.compare_exchange_strong(status, &CompletedResult, std::memory_order_acq_rel);
    return false;
}

const CYResultStateBase* CYWhenAnyContext::CompletedResult() const noexcept
{
    return m_status.load(std::memory_order_acquire);
}

/*
 * CYConsumerContext
 */

CYConsumerContext::~CYConsumerContext() noexcept
{
    Destroy();
}

void CYConsumerContext::Destroy() noexcept
{
    switch (m_status)
    {
    case EConsumerStatus::STATUS_CONSUMER_IDLE:
    {
        return;
    }
    case EConsumerStatus::STATUS_CONSUMER_AWAIT:
    {
        return CYCOROUTINE_NAMESPACE::Destroy(m_storage.handleCaller);
    }
    case EConsumerStatus::STATUS_CONSUMER_WAITFOR:
    {
        return CYCOROUTINE_NAMESPACE::Destroy(m_storage.wait_for_ctx);
    }
    case EConsumerStatus::STATUS_CONSUMER_WHENANY:
    {
        return CYCOROUTINE_NAMESPACE::Destroy(m_storage.ptrWhenAnyCtx);
    }
    case EConsumerStatus::STATUS_CONSUMER_SHARED:
    {
        return CYCOROUTINE_NAMESPACE::Destroy(m_storage.ptrSharedCtx);
    }
    }

    assert(false);
}

void CYConsumerContext::Clear() noexcept
{
    Destroy();
    m_status = EConsumerStatus::STATUS_CONSUMER_IDLE;
}

void CYConsumerContext::SetAwaitHandle(coroutine_handle<void> handleCaller) noexcept
{
    assert(m_status == EConsumerStatus::STATUS_CONSUMER_IDLE);
    m_status = EConsumerStatus::STATUS_CONSUMER_AWAIT;
    Build(m_storage.handleCaller, handleCaller);
}

void CYConsumerContext::SetWaitForContext(const SharePtr<cy_binary_semaphore>& ptrWaitCtx) noexcept
{
    assert(m_status == EConsumerStatus::STATUS_CONSUMER_IDLE);
    m_status = EConsumerStatus::STATUS_CONSUMER_WAITFOR;
    Build(m_storage.wait_for_ctx, ptrWaitCtx);
}

void CYConsumerContext::SetWhenAnyContext(const SharePtr<CYWhenAnyContext>& ptrWhenAnyCtx) noexcept
{
    assert(m_status == EConsumerStatus::STATUS_CONSUMER_IDLE);
    m_status = EConsumerStatus::STATUS_CONSUMER_WHENANY;
    Build(m_storage.ptrWhenAnyCtx, ptrWhenAnyCtx);
}

void CYConsumerContext::SetSharedContext(const SharePtr<CYSharedResultStateBase>& ptrSharedCtx) noexcept
{
    assert(m_status == EConsumerStatus::STATUS_CONSUMER_IDLE);
    m_status = EConsumerStatus::STATUS_CONSUMER_SHARED;
    Build(m_storage.ptrSharedCtx, ptrSharedCtx);
}

void CYConsumerContext::ResumeConsumer(CYResultStateBase& self) const
{
    switch (m_status)
    {
    case EConsumerStatus::STATUS_CONSUMER_IDLE:
    {
        return;
    }

    case EConsumerStatus::STATUS_CONSUMER_AWAIT:
    {
        auto handleCaller = m_storage.handleCaller;
        assert(static_cast<bool>(handleCaller));
        assert(!handleCaller.done());
        return handleCaller();
    }

    case EConsumerStatus::STATUS_CONSUMER_WAITFOR:
    {
        const auto ptrWaitCtx = m_storage.wait_for_ctx;
        assert(static_cast<bool>(ptrWaitCtx));
        return ptrWaitCtx->release();
    }

    case EConsumerStatus::STATUS_CONSUMER_WHENANY:
    {
        const auto ptrWhenAnyCtx = m_storage.ptrWhenAnyCtx;
        return ptrWhenAnyCtx->TryResume(self);
    }

    case EConsumerStatus::STATUS_CONSUMER_SHARED:
    {
        const auto ptrWeakSharedCtx = m_storage.ptrSharedCtx;
        const auto ptrSharedCtx = ptrWeakSharedCtx.lock();
        if (static_cast<bool>(ptrSharedCtx))
        {
            ptrSharedCtx->OnResultFinished();
        }
        return;
    }
    }

    assert(false);
}