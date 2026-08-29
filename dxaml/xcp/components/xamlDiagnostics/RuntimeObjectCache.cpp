// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeObject.h"
#include "RuntimeObjectCache.h"
#include "RuntimeApplication.h"
#include <DependencyLocator.h>
#include "XcpAllocationDebug.h"
#include "RuntimeElement.h"

namespace Diagnostics
{

    bool RuntimeObjectCache::TryFindInCache(InstanceHandle handle, std::shared_ptr<RuntimeObject>& object)
    {
        auto lock = m_cacheLock.lock_shared();
        auto iter = m_weakCache.find(handle);
        if (iter != m_weakCache.end() && !iter->second.expired())
        {
            object = iter->second.lock();
            return true;
        }

        return false;
    }

    bool RuntimeObjectCache::TryFindInCache(_In_ IInspectable* obj, std::shared_ptr<RuntimeObject>& object)
    {
        auto lock = m_cacheLock.lock_shared();
        auto iter = m_weakCache.find(MakeHandle(obj));
        if (iter != m_weakCache.end() && !iter->second.expired())
        {
            object = iter->second.lock();
            return true;
        }

        return false;
    }

    bool RuntimeObjectCache::TryFindAssociatedApplication(DWORD threadId, std::shared_ptr<RuntimeApplication>& application)
    {
        auto lock = m_cacheLock.lock_shared();
        std::shared_ptr<RuntimeApplication> result;
        for (auto& cached : m_parentlessObjects)
        {
            if (cached->TryGetAsApplication(result) && result->GetAssociatedThreadId() == threadId)
            {
                application = result;
                return true;
            }
        }

        return false;
    }

    void RuntimeObjectCache::AddToCache(std::shared_ptr<RuntimeObject> object)
    {
        auto lock = m_cacheLock.lock_exclusive();
        if (!object->HasParent())
        {
            m_parentlessObjects.emplace_back(object);
        }

        auto cache = m_weakCache.emplace(object->GetHandle(), object);
        if (!cache.second)
        {
            // This objects handle was recycled, update the object
            if (!cache.first->second.expired())
            {
                // the app object is special, if this is an issue we can have a separate
                // cache for that
                std::shared_ptr<RuntimeApplication> result;
                auto obj = cache.first->second.lock();
                ASSERT(obj == object || obj->TryGetAsApplication(result));
            }
            cache.first->second = object;
        }
    }

    void RuntimeObjectCache::RemoveFromCache(std::shared_ptr<RuntimeObject> object)
    {
        auto lock = m_cacheLock.lock_exclusive();
        auto iter = std::find(m_parentlessObjects.begin(), m_parentlessObjects.end(), object);
        if (iter != m_parentlessObjects.end())
        {
            // Ignore any leaks associated with reallocations due to shrink_to_fit. If any object
            // is left in the cache it will still be reported as leaked.
#if XCP_MONITOR
            auto ignoreleaks = XcpDebugStartIgnoringLeaks();
#endif
            m_parentlessObjects.erase(iter);
#if XCP_MONITOR
            m_parentlessObjects.shrink_to_fit();
#endif
        }
    }

    void RuntimeObjectCache::InvalidateHandle(InstanceHandle handle)
    {
        auto lock = m_cacheLock.lock_exclusive();
        m_weakCache.erase(handle);
    }

    std::shared_ptr<RuntimeObjectCache> GetRuntimeObjectCache(bool create)
    {
        // We mark this as ignorable so that we can use the frameworks leak detection
        // and validate that the cache is empty
#if XCP_MONITOR
        auto lock = XcpDebugStartIgnoringLeaks();
#endif
        // Ideally the cache could be per-thread. But our XamlDiagnostics APIs for getting and registering objects from handles
        // need to be callable from any thread. For now just let there be a single one. We could look into dispatching
        // onto each known UI thread later, however this will probably add unnecessary complexity to our code
        static PROVIDE_DEPENDENCY(RuntimeObjectCache);
        static DependencyLocator::Dependency<RuntimeObjectCache> s_cache;
        return s_cache.Get(create);
    }

    std::vector<std::shared_ptr<RuntimeElement>> RuntimeObjectCache::Roots(DWORD threadId) const
    {
        std::vector<std::shared_ptr<RuntimeElement>> roots;
        roots.reserve(m_parentlessObjects.size());
        for (auto& object : m_parentlessObjects)
        {
            std::shared_ptr<RuntimeElement> element;
            if (object->TryGetAsElement(element) &&
                element->GetAssociatedThreadId() == threadId &&
                element->IsRoot())
            {
                roots.push_back(std::move(element));
            }
        }
        return roots;
    }

    std::shared_ptr<RuntimeObjectCache> GetRuntimeObjectCache()
    {
        return GetRuntimeObjectCache(true);
    }

    std::shared_ptr<RuntimeObjectCache> TryGetRuntimeObjectCache()
    {
        return GetRuntimeObjectCache(false);
    }
}
