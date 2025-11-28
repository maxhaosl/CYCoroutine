#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Engine/CYCoroutineEngineDefine.hpp"
#include "CYCoroutine/Threads/CYThread.hpp"
#include "Src/CYCoroutinePrivDefine.hpp"

#include <atomic>

CYCOROUTINE_NAMESPACE_BEGIN

namespace
{
    std::uintptr_t generate_thread_id() noexcept
    {
        static std::atomic_uintptr_t s_id_seed = 1;
        return s_id_seed.fetch_add(1, std::memory_order_relaxed);
    }

    struct thread_per_thread_data
    {
        const std::uintptr_t id = generate_thread_id();
    };

    thread_local thread_per_thread_data s_tl_thread_per_data;
}  // namespace


cy_jthread::id CYThread::GetId() const noexcept
{
    return m_thread.get_id();
}

std::uintptr_t CYThread::GetVirtualId() noexcept
{
    return s_tl_thread_per_data.id;
}

bool CYThread::Joinable() const noexcept
{
    return m_thread.joinable();
}

void CYThread::Join()
{
    m_thread.join();
}

size_t CYThread::NumberOfCpu() noexcept
{
    const auto hc = cy_jthread::hardware_concurrency();
    return (hc != 0) ? hc : DEFAULT_NUMBER_OF_CORES;
}

#ifdef CYCOROUTINE_WIN_OS

#include <Windows.h>

void CYThread::SetName(std::string_view strName) noexcept
{
    const std::wstring utf16_name(strName.begin(), strName.end());
    SetThreadDescription(GetCurrentThread(), utf16_name.data());
}

#elif defined(CYCOROUTINE_MINGW_OS)

#    include <pthread.h>

void CYThread::SetName(std::string_view strName) noexcept
{
    ::pthread_setname_np(::pthread_self(), strName.data());
}

#elif defined(CYCOROUTINE_UNIX_OS)

#    include <pthread.h>

void CYThread::SetName(std::string_view strName) noexcept
{
    ::pthread_setname_np(::pthread_self(), strName.data());
}

#elif defined(CYCOROUTINE_MAC_OS)

#    include <pthread.h>

void CYThread::SetName(std::string_view strName) noexcept
{
    ::pthread_setname_np(strName.data());
}

#endif

CYCOROUTINE_NAMESPACE_END
