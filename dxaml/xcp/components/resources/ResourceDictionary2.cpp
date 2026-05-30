// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CDependencyObject.h>
#include <ResourceDictionaryCustomRuntimeData.h>
#include <CustomWriterRuntimeContext.h>
#include <ResourceDictionary2.h>
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
    // Reset the object creator to ensure any previous state is cleared out.
    m_spObjectCreator = nullptr;

    m_spRuntimeData = std::move(data);
    m_spRuntimeContext = std::move(context);

    std::shared_ptr<ParserErrorReporter> parserErrorReporter;
    IFC_RETURN(m_spRuntimeContext->GetSchemaContext()->GetErrorService(parserErrorReporter));
    IFC_RETURN(m_spRuntimeData->ResolveConditionalResources(parserErrorReporter));

    return S_OK;
}

_Check_return_ HRESULT CResourceDictionary2::LoadValueIfExists(
    _In_ const ResourceKey& key,
    _Out_ bool& keyFound,
    _Out_ xref_ptr<CDependencyObject>& value)
{
    StreamOffsetToken token;
    keyFound = false;
    // If the key doesn't exist in the dictionary we return an empty element.
    auto success = m_spRuntimeData->TryGetResourceOffset(key, token);
    if (!success)
    {
        value.reset();
        return S_OK;
    }

    if (!m_spObjectCreator)
    {
        // The creator will use the context's weak references to some DOs (such as the root instance
        // and the event root) to create its (really the binary parser's + its own context's) references
        // for more convenient access. These references must also be weak to avoid cycles since
        // the root references are to the ResourceDictionary that owns this object. We can guarantee
        // that those effectively raw pointers cannot go away while this object exists.
        m_spObjectCreator = std::make_unique<CustomWriterRuntimeObjectCreator>(
            NameScopeRegistrationMode::RegisterEntries,
            m_spRuntimeContext.get(),
            CustomWriterRuntimeObjectCreator::ContextReference::Weak);
    }

    xref_ptr<CThemeResource> resourceAsThemeResourceAlias;
    IFC_RETURN(m_spObjectCreator->CreateInstance(token, &value, &resourceAsThemeResourceAlias));
    
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
        _In_ ResourceMap& existingResources,
        _Out_ std::vector<std::pair<ResourceKeyStorage, xref_ptr<CDependencyObject>>>& loadedResources)
{
    // We don't preserve the ordering of keys here. Ideally we'd keep the illusion
    // that the keys were in the same index order as they appeared in the developer's XAML file
    // but we don't preserve that ordering in XBFv2 and found the tradeoff to be not worth
    // it from a performance standpoint for the value it provides. The order we load the keys
    // here is arbitrary w.r.t the developer's intent. Not only is it ordered by the type of
    // key, but it also is dependent on the previous access order as already-instantiated
    // keys are already present in the dictionary.

    if (!m_spObjectCreator)
    {
        // The creator will use the context's weak references to some DOs (such as the root instance
        // and the event root) to create its (really the binary parser's + its own context's) references
        // for more convenient access. These references must also be weak to avoid cycles since
        // the root references are to the ResourceDictionary that owns this object. We can guarantee
        // that those effectively raw pointers cannot go away while this object exists.
        m_spObjectCreator = std::make_unique<CustomWriterRuntimeObjectCreator>(
            NameScopeRegistrationMode::RegisterEntries,
            m_spRuntimeContext.get(),
            CustomWriterRuntimeObjectCreator::ContextReference::Weak);
    }

    for (const auto& resource : *m_spRuntimeData)
    {
        auto& key = resource.first;
        if (existingResources.find(key) == existingResources.end())
        {
            xref_ptr<CDependencyObject> value;
            xref_ptr<CThemeResource> unused;
            auto token = resource.second;

            IFC_RETURN(m_spObjectCreator->CreateInstance(token, &value, &unused));
            loadedResources.emplace_back(key, std::move(value));
        }
    }

    return S_OK;
}

bool CResourceDictionary2::ContainsKey(
    _In_ const ResourceKey& key) const
{
    StreamOffsetToken token;
    return m_spRuntimeData->TryGetResourceOffset(key, token);
}

std::size_t CResourceDictionary2::GetInitialImplicitStyleKeyCount() const
{
    return m_spRuntimeData->GetImplicitResourceCount();
}

const std::vector<xstring_ptr>& CResourceDictionary2::GetInitialResourcesToLoad() const
{
    return m_spRuntimeData->GetResourcesForAutoUndeferral();
}

std::size_t CResourceDictionary2::size() const
{
    return m_spRuntimeData->size();
}
