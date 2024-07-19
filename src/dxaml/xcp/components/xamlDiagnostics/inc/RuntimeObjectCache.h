// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "wil\resource.h"
#include <vector>
#include "XamlOM.WinUI.h"
#include <memory>
#include "XcpAllocationDebug.h"

namespace Diagnostics
{
    class RuntimeObject;
    class RuntimeShareableObject;
    class RuntimeApplication;

    class __declspec(uuid("b77ee9b4-fe4b-4dc6-ab26-ed2592841fb4")) RuntimeObjectCache final
    {
        // These classes have special capabilities of removing themselves
        // from the cache once they are properly parented. Roots can remove
        // themeselves once they leave the tree
        friend class RuntimeObject;
        friend class RuntimeShareableObject;
        friend class RuntimeElement;

    public:
        bool TryFindInCache(InstanceHandle handle, std::shared_ptr<RuntimeObject>& object);
        bool TryFindInCache(_In_ IInspectable* insp, std::shared_ptr<RuntimeObject>& object);
        bool TryFindAssociatedApplication(DWORD threadId, std::shared_ptr<RuntimeApplication>& application);
        std::vector<std::shared_ptr<RuntimeElement>> Roots(DWORD threadId) const;
    private:

        void AddToCache(std::shared_ptr<RuntimeObject>);
        void RemoveFromCache(std::shared_ptr<RuntimeObject>);
        void InvalidateHandle(InstanceHandle handle);

        wil::srwlock m_cacheLock;
        // The cache keeps parentless objects alive, once they are parented, the lifetime is managed
        // by their owner
        std::vector<std::shared_ptr<RuntimeObject>> m_parentlessObjects;

#if XCP_MONITOR
        typedef std::map<InstanceHandle, std::weak_ptr<RuntimeObject>,
            std::less<InstanceHandle>,
            XcpAllocation::LeakIgnoringAllocator<std::pair<InstanceHandle, std::weak_ptr<RuntimeObject>>>> WeakRuntimeObjectCache;
#else
        typedef std::map<InstanceHandle, std::weak_ptr<RuntimeObject>> WeakRuntimeObjectCache;
#endif

        // Keep a weak ref map for fast lookups via InstanceHandle
        WeakRuntimeObjectCache m_weakCache;
    };

    std::shared_ptr<RuntimeObjectCache> GetRuntimeObjectCache();
    std::shared_ptr<RuntimeObjectCache> TryGetRuntimeObjectCache();
}
