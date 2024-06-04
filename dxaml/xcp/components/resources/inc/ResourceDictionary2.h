// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector_set.h>
#include "ResourceDictionaryKey.h"

class ResourceDictionaryCustomRuntimeData;
class CustomWriterRuntimeContext;
class CDependencyObject;
struct IDependencyObject;

using ResourceMapType = std::unordered_map<ResourceKeyStorage, CDependencyObject*>;

// CResourceDictionary2 today exists as a thin wrapper around ResourceDictionaryCustomRuntimeData
// with aspirations of one day possibly replacing the current resource dictionary implementation.
class CResourceDictionary2
{
public:
    CResourceDictionary2() = default;
    ~CResourceDictionary2() = default;

    _Check_return_ HRESULT SetCustomWriterRuntimeData(
        _In_ std::shared_ptr<ResourceDictionaryCustomRuntimeData> data,
        _In_ std::unique_ptr<CustomWriterRuntimeContext> context);

    // Tries to load the resource matching the specified key. Throws on runtime object creation failure.
    _Check_return_ HRESULT LoadValueIfExists(
        _In_ const xstring_ptr& key,
        _In_ bool isImplicitKey,
        _Out_ bool& keyFound,
        _Out_ std::shared_ptr<CDependencyObject>& value);

    // Index-accesses and index-removal operations require all deferred resources be instantiated to avoid
    // the complexity of managing what would become a quite sophisicated internal state for what is
    // ultimately a corner case. After this method is called the client generally should throw away this
    // instance of CResourceDictionary2 as it doesn't contain any useful state.
    _Check_return_
    HRESULT LoadAllRemainingDeferredResources(
        _In_ const ResourceMapType& loadedResources,
        _Out_ std::vector<std::pair<xstring_ptr, std::shared_ptr<CDependencyObject>>>& implicitResources,
        _Out_ std::vector<std::pair<xstring_ptr, std::shared_ptr<CDependencyObject>>>& explicitResources);

    std::size_t GetInitialImplicitStyleKeyCount() const;
    const std::vector<xstring_ptr>& GetInitialResourcesWithXNames() const;

    bool ContainsKey(_In_ const xstring_ptr& key, _In_ bool isImplicitKey) const;

    std::size_t size() const;

    CustomWriterRuntimeContext* GetSavedRuntimeContext() const { return m_spRuntimeContext.get(); }

private:
    std::shared_ptr<ResourceDictionaryCustomRuntimeData> m_spRuntimeData;
    std::unique_ptr<CustomWriterRuntimeContext> m_spRuntimeContext;
};