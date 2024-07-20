// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CDependencyObject.h>
#include <ResourceDictionaryCustomRuntimeData.h>
#include <CustomWriterRuntimeContext.h>
#include <ResourceDictionary2.h>
#include <CustomWriterRuntimeObjectCreator.h>
#include <ObjectWriterStack.h>
#include <ObjectWriterFrame.h>
#include <XamlSchemaContext.h>
#include <ThemeResource.h>

#define RDLOG(...)

// Sets the CustomWriter data and context that will be needed to load deferred resources.
_Check_return_ HRESULT CResourceDictionary2::SetCustomWriterRuntimeData(
    _In_ std::shared_ptr<ResourceDictionaryCustomRuntimeData> data,
    _In_ std::unique_ptr<CustomWriterRuntimeContext> context)
{
    m_spRuntimeData = std::move(data);
    m_spRuntimeContext = std::move(context);

    std::shared_ptr<ParserErrorReporter> parserErrorReporter;
    IFC_RETURN(m_spRuntimeContext->GetSchemaContext()->GetErrorService(parserErrorReporter));
    IFC_RETURN(m_spRuntimeData->ResolveConditionalResources(parserErrorReporter));
    return S_OK;
}

_Check_return_ HRESULT CResourceDictionary2::LoadValueIfExists(
    _In_ const xstring_ptr& key,
    _In_ bool isImplicitKey,
    _Out_ bool& keyFound,
    _Out_ std::shared_ptr<CDependencyObject>& value)
{
    StreamOffsetToken token;
    keyFound = false;
    // If the key doesn't exist in the dictionary we return an empty element.
    auto success = m_spRuntimeData->TryGetResourceOffset(key, isImplicitKey, token);
    if (!success)
    {
        value.reset();
        return S_OK;
    }

    xref_ptr<CThemeResource> resourceAsThemeResourceAlias;

    CustomWriterRuntimeObjectCreator creator(NameScopeRegistrationMode::RegisterEntries, m_spRuntimeContext.get());
    IFC_RETURN(creator.CreateInstance(token, &value, &resourceAsThemeResourceAlias));
    
    if (resourceAsThemeResourceAlias == nullptr)
    {
        keyFound = true;
    }
    else
    {
        // TODO: If/when we go and fix MSFT:17941291, then we'll have to actually do something here, rather
        // than pretend the key doesn't exist
    }

    return S_OK;
}

_Check_return_
HRESULT CResourceDictionary2::LoadAllRemainingDeferredResources(
    _In_ const ResourceMapType& loadedResources,
    _Out_ std::vector<std::pair<xstring_ptr, std::shared_ptr<CDependencyObject>>>& implicitResources,
    _Out_ std::vector<std::pair<xstring_ptr, std::shared_ptr<CDependencyObject>>>& explicitResources)
{
    // We don't preserve the ordering of keys here. Ideally we'd keep the illusion
    // that the keys were in the same index order as they appeared in the developer's XAML file
    // but we don't preserve that ordering in XBFv2 and found the tradeoff to be not worth
    // it from a performance standpoint for the value it provides. The order we load the keys
    // here is arbitrary w.r.t the developer's intent. Not only is it ordered by the type of
    // key, but it also is dependent on the previous access order as already-instantiated
    // keys are already present in the dictionary.
    auto instantiateAndAddKey = [this](const xstring_ptr& key, bool isImplicit,
        std::vector<std::pair<xstring_ptr, std::shared_ptr<CDependencyObject>>>& dest)
    {
        bool keyFound = false;
        std::shared_ptr<CDependencyObject> resource;
        IFC_RETURN(LoadValueIfExists(key, isImplicit, keyFound, resource));
        ASSERT(keyFound);
        dest.emplace_back(key, resource);
        return S_OK;
    };

    implicitResources.clear();
    for (const auto& key : m_spRuntimeData->GetImplicitKeys())
    {
        if (loadedResources.find(ResourceKeyStorage(key, true)) == loadedResources.end())
        {
            IFC_RETURN(instantiateAndAddKey(key, true, implicitResources));
        }
    }

    explicitResources.clear();
    for (const auto& key : m_spRuntimeData->GetExplicitKeys())
    {
        if (loadedResources.find(ResourceKeyStorage(key, false)) == loadedResources.end())
        {
            IFC_RETURN(instantiateAndAddKey(key, false, explicitResources));
        }
    }

    return S_OK;
}

bool CResourceDictionary2::ContainsKey(
    _In_ const xstring_ptr& key,
    _In_ bool isImplicitKey) const
{
    StreamOffsetToken token;
    return m_spRuntimeData->TryGetResourceOffset(key, isImplicitKey, token);
}

std::size_t CResourceDictionary2::GetInitialImplicitStyleKeyCount() const
{
    return m_spRuntimeData->GetImplicitResourceCount();
}

const std::vector<xstring_ptr>& CResourceDictionary2::GetInitialResourcesWithXNames() const
{
    return m_spRuntimeData->GetResourcesWithXNames();
}

std::size_t CResourceDictionary2::size() const
{
    return m_spRuntimeData->size();
}
