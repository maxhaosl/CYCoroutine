#include "Src/Engine/CYExecutorCollection.hpp"
#include "CYCoroutine/Executors/CYExecutor.hpp"
#include <assert.h>

CYCOROUTINE_NAMESPACE_BEGIN

/*
    CYExecutorCollection;
*/
void CYExecutorCollection::RegisterExecutor(SharePtr<CYExecutor> ptrExecutor)
{
    assert(static_cast<bool>(ptrExecutor));

    UniqueLock lock(m_lock);
    assert(std::find(m_lstExecutors.begin(), m_lstExecutors.end(), ptrExecutor) == m_lstExecutors.end());
    m_lstExecutors.emplace_back(std::move(ptrExecutor));
}

void CYExecutorCollection::ShutDownALL()
{
    UniqueLock lock(m_lock);
    for (auto& ptrExecutor : m_lstExecutors)
    {
        assert(static_cast<bool>(ptrExecutor));
        ptrExecutor->ShutDown();
    }

    m_lstExecutors = {};
}

CYCOROUTINE_NAMESPACE_END