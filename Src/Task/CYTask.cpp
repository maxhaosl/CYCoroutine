#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Results/Impl/CYConsumerContext.hpp"
#include "CYCoroutine/Task/CYTask.hpp"

#include <cstring>

CYCOROUTINE_NAMESPACE_BEGIN

using CYCOROUTINE_NAMESPACE::CYTask;
using CYCOROUTINE_NAMESPACE::CYVTable;

static_assert(sizeof(CYCOROUTINE_NAMESPACE::CYTask) == CYTaskConstants::TOTAL_SIZE, "CYTask - object size is bigger than a cache-line.");

using CYCOROUTINE_NAMESPACE::CYCallableVTable;
using CYCOROUTINE_NAMESPACE::CYAwaitViaFunctor;

namespace
{
    class CYCoroutineHandleFunctor
    {

    private:
        coroutine_handle<void> m_coro_handle;

    public:
        CYCoroutineHandleFunctor() noexcept
            : m_coro_handle()
        {
        }

        CYCoroutineHandleFunctor(const CYCoroutineHandleFunctor&) = delete;
        CYCoroutineHandleFunctor& operator=(const CYCoroutineHandleFunctor&) = delete;

        CYCoroutineHandleFunctor(coroutine_handle<void> handleCoro) noexcept : m_coro_handle(handleCoro)
        {
        }

        CYCoroutineHandleFunctor(CYCoroutineHandleFunctor&& rhs) noexcept : m_coro_handle(std::exchange(rhs.m_coro_handle, {}))
        {
        }

        ~CYCoroutineHandleFunctor() noexcept
        {
            if (static_cast<bool>(m_coro_handle))
            {
                m_coro_handle.destroy();
            }
        }

        void ExecuteDestroy() noexcept
        {
            auto handleCoro = std::exchange(m_coro_handle, {});
            handleCoro();
        }

        void operator()() noexcept
        {
            ExecuteDestroy();
        }
    };
}  // namespace


using CYCOROUTINE_NAMESPACE::CYCoroutineHandleFunctor;

void CYTask::Build(CYTask&& rhs) noexcept
{
    m_vtable = std::exchange(rhs.m_vtable, nullptr);
    if (m_vtable == nullptr)
    {
        return;
    }

    if (Contains<CYCoroutineHandleFunctor>(m_vtable))
    {
        return CYCallableVTable<CYCoroutineHandleFunctor>::MoveDestroy(rhs.m_buffer, m_buffer);
    }

    if (Contains<CYAwaitViaFunctor>(m_vtable))
    {
        return CYCallableVTable<CYAwaitViaFunctor>::MoveDestroy(rhs.m_buffer, m_buffer);
    }

    const auto FunMoveDestroy = m_vtable->FunMoveDestroy;
    if (CYVTable::TriviallyCopiableDestructible(FunMoveDestroy))
    {
        std::memcpy(m_buffer, rhs.m_buffer, CYTaskConstants::BUFFER_SIZE);
        return;
    }

    FunMoveDestroy(rhs.m_buffer, m_buffer);
}

void CYTask::Build(coroutine_handle<void> handleCoro) noexcept
{
    Build(CYCoroutineHandleFunctor{ handleCoro });
}

bool CYTask::ContainsCoroutineHandle() const noexcept
{
    return Contains<CYCoroutineHandleFunctor>();
}

CYTask::CYTask() noexcept : m_buffer(), m_vtable(nullptr)
{
}

CYTask::CYTask(CYTask&& rhs) noexcept
{
    Build(std::move(rhs));
}

CYTask::CYTask(coroutine_handle<void> handleCoro) noexcept
{
    Build(handleCoro);
}

CYTask::~CYTask() noexcept
{
    Clear();
}

void CYTask::operator()()
{
    const auto vTable = std::exchange(m_vtable, nullptr);
    if (vTable == nullptr)
    {
        return;
    }

    if (Contains<CYCoroutineHandleFunctor>(vTable))
    {
        return CYCallableVTable<CYCoroutineHandleFunctor>::ExecuteDestroy(m_buffer);
    }

    if (Contains<CYAwaitViaFunctor>(vTable))
    {
        return CYCallableVTable<CYAwaitViaFunctor>::ExecuteDestroy(m_buffer);
    }

    vTable->FunExecuteDestroy(m_buffer);
}

CYTask& CYTask::operator=(CYTask&& rhs) noexcept
{
    if (this == &rhs)
    {
        return *this;
    }

    Clear();
    Build(std::move(rhs));
    return *this;
}

void CYTask::Clear() noexcept
{
    if (m_vtable == nullptr)
    {
        return;
    }

    const auto vTable = std::exchange(m_vtable, nullptr);

    if (Contains<CYCoroutineHandleFunctor>(vTable))
    {
        return CYCallableVTable<CYCoroutineHandleFunctor>::Destroy(m_buffer);
    }

    if (Contains<CYAwaitViaFunctor>(vTable))
    {
        return CYCallableVTable<CYAwaitViaFunctor>::Destroy(m_buffer);
    }

    auto funDestroy = vTable->FunDestroy;
    if (CYVTable::TriviallyDestructible(funDestroy))
    {
        return;
    }

    funDestroy(m_buffer);
}

CYTask::operator bool() const noexcept
{
    return m_vtable != nullptr;
}

CYCOROUTINE_NAMESPACE_END