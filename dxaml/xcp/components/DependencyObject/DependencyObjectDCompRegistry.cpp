// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyObjectDCompRegistry.h"
#include "CDependencyObject.h"
#include "XamlTelemetry.h"

void DependencyObjectDCompRegistry::EnsureObjectWithDCompResourceRegistered(_In_ CDependencyObject* pObject)
{
    // Note: unordered_map::emplace/insert no-ops if the element is already in the list, which can prevent new objects from
    // being inserted if there's a dangling weak ref in the map from an old object with the same address.
    m_objectsWithDCompResourcesNoRef[pObject] = xref::get_weakref(pObject);
}

void DependencyObjectDCompRegistry::UnregisterObject(_In_ CDependencyObject* pObject)
{
    m_objectsWithDCompResourcesNoRef.erase(pObject);

    // Shrink down the backing vector if there are too many empty buckets. We have scenarios where lots of tabs are
    // opened and closed, and after garbage collection this set has over 8000 buckets yet only ~20 items, which
    // unnecessarily takes up memory. As a heuristic, shrink when there are 100x more buckets than items. Also only
    // shrink if there are more than 500 buckets so we don't thrash when there are only a few elements.
    if (m_objectsWithDCompResourcesNoRef.bucket_count() > 500 && m_objectsWithDCompResourcesNoRef.load_factor() < 0.01)
    {
        // Note: rehash in <xhash> only increases the number of buckets. We have to build a new unordered_set.
        std::unordered_map<CDependencyObject*, xref::weakref_ptr<CDependencyObject>> shrunk(std::make_move_iterator(m_objectsWithDCompResourcesNoRef.begin()), std::make_move_iterator(m_objectsWithDCompResourcesNoRef.end()));

        TraceLoggingProviderWrite(
            XamlTelemetry, "Memory_ResizeDCompRegistryUnorderedSet",
            TraceLoggingUInt64(m_objectsWithDCompResourcesNoRef.bucket_count(), "OriginalBucketCount"),
            TraceLoggingUInt64(shrunk.bucket_count(), "NewBucketCount"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        m_objectsWithDCompResourcesNoRef = std::move(shrunk);
    }
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
