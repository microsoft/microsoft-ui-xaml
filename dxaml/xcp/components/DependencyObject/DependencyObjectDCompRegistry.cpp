// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyObjectDCompRegistry.h"
#include "CDependencyObject.h"

void DependencyObjectDCompRegistry::EnsureObjectWithDCompResourceRegistered(_In_ CDependencyObject* pObject)
{
    // Note: unordered_map::emplace/insert no-ops if the element is already in the list, which can prevent new objects from
    // being inserted if there's a dangling weak ref in the map from an old object with the same address.
    m_objectsWithDCompResourcesNoRef[pObject] = xref::get_weakref(pObject);
}

void DependencyObjectDCompRegistry::UnregisterObject(_In_ CDependencyObject* pObject)
{
    m_objectsWithDCompResourcesNoRef.erase(pObject);
}

void DependencyObjectDCompRegistry::ReleaseDCompResources()
{
    for (auto& item : m_objectsWithDCompResourcesNoRef)
    {
        if (!item.second.expired())
        {
            auto pDO = item.second.lock();
            if (pDO != nullptr)
            {
                pDO->ReleaseDCompResources();
            }
        }
    }

    m_objectsWithDCompResourcesNoRef.clear();
}

void DependencyObjectDCompRegistry::AbandonRegistry()
{
    m_objectsWithDCompResourcesNoRef.clear();
}
