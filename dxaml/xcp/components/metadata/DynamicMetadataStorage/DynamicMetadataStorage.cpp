// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <TypeTableStructs.h>
#include <DynamicMetadataStorage.h>

using namespace DirectUI;

static std::unique_ptr<DynamicMetadataStorage> s_storage;

// Not thread-safe.
DynamicMetadataStorage* DynamicMetadataStorage::GetInstance()
{
    if (!s_storage)
    {
        s_storage.reset(new DynamicMetadataStorage());
        FAIL_FAST_ASSERT(s_storage);
    }

    return s_storage.get();
}

void DynamicMetadataStorage::Reset()
{
    DynamicMetadataStorage* storage = s_storage.get();

    if (storage)
    {
        storage->ResetInstance();
    }
}

void DynamicMetadataStorage::Destroy()
{
    DynamicMetadataStorage* storage = s_storage.release();

    if (storage)
    {
        delete storage;
    }

    // Verify we are null here because we don't want to re-enter.
    FAIL_FAST_ASSERT(s_storage == nullptr);
}

void DynamicMetadataStorage::ResetInstance()
{
    // First, mark all custom properties as invalid.

    for (auto index = m_valid.m_propertyFirstIndex; index < m_customPropertiesCache.size(); ++index)
    {
        CDependencyProperty* property = m_customPropertiesCache[index];

        if (property->Is<CCustomDependencyProperty>())
        {
            reinterpret_cast<CCustomDependencyProperty*>(property)->Invalidate();
        }
        else if (property->Is<CCustomProperty>())
        {
            reinterpret_cast<CCustomProperty*>(property)->Invalidate();
        }
        else
        {
            ASSERT(false);
        }
    }

    // Reset providers.

    m_metadataProvider                      = nullptr;
    m_overriddenMetadataProvider            = nullptr;
    m_queuedDPRegistrations                 = nullptr;

    // Clear internal caches.

    m_customNamespacesByNameCache.clear();

    m_customTypesByNameCache.clear();
    m_customTypesByCustomNameCache.clear();

    m_customDPsByTypeAndNameCache           = nullptr;
    m_customPropertiesByTypeAndNameCache    = nullptr;

    // and save indices for the boundary between stale and good metadata.

    m_valid.m_namespaceFirstIndex           = m_customNamespacesCache.size();
    m_valid.m_typeFirstIndex                = m_customTypesCache.size();
    m_valid.m_propertyFirstIndex            = m_customPropertiesCache.size();
}
