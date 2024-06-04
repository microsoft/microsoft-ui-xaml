// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <xcperrorresource.h>

#include <ObjectWriterNodeList.h>
#include <ParserErrorService.h>
#include <SubObjectWriterResult.h>
#include <XamlPredicateHelpers.h>
#include <XamlPredicateService.h>
#include <XamlQualifiedObject.h>
#include <XamlSchemaContext.h>
#include <XbfVersioning.h>
#include <XStringBuilder.h>

#include "CustomWriterRuntimeDataTypeIndex.h"
#include "ResourceDictionaryCustomRuntimeData.h"
#include "ResourceDictionaryCustomRuntimeDataSerializer.h"

#pragma region Compiletime Methods

// Add a resource with its StreamOffsetToken to the custom runtime data. This method is ONLY to be
// used during XAML compile time as it modifies the internal state of the runtime data, which is shared
// between instances at runtime.
_Check_return_ HRESULT ResourceDictionaryCustomRuntimeData::AddResourceOffset(
    _In_ const xstring_ptr& key,
    _In_ const StreamOffsetToken& token,
    _In_ bool isImplicitKey,
    _In_ bool hasXName)
{
    // Check first if the key already exists in the conditional store
    auto& conditionalStore = (isImplicitKey) ? m_conditionalImplicitKeyResources : m_conditionalExplicitKeyResources;
    bool conditionalKeyExists = (conditionalStore.find(key) != conditionalStore.end());

    if (!IsInConditionalScope() && !conditionalKeyExists)
    {
        auto& store = (isImplicitKey) ? m_implicitKeyResourcesMap : m_explicitKeyResourcesMap;
        if (!store.emplace(key, token).second)
        {
            // Return E_DO_RESOURCE_KEYCONFLICT instead of AG_E_PARSER2_OW_DUPLICATE_KEY as the former
            // is an HRESULT whereas the latter is just an internal error code.
            // FUTURE: CustomWriters should probably be given access to an error reporter instance
            // so that they can log specific error messages
            IFC_RETURN(static_cast<HRESULT>(E_DO_RESOURCE_KEYCONFLICT));
        }
    }
    else
    {
        auto entryItr = conditionalStore.emplace(key, std::vector<StreamOffsetToken>()).first;
        entryItr->second.push_back(token);
        RecordConditionallyDeclaredObject(token);

        // If this was previously declared outside of a conditional scope, move it over. Odds are better than even
        // that at runtime it'll be a AG_E_PARSER2_OW_DUPLICATE_KEY once all relevant conditional scope predicates are
        // evaluated, but since we can't know for sure, we'll let it pass for now and check at runtime when looking
        // up the correct StreamOffsetToken to use.
        auto& originalStore = (isImplicitKey) ? m_implicitKeyResourcesMap : m_explicitKeyResourcesMap;
        auto originalEntry = originalStore.find(key);
        if (originalEntry != originalStore.end())
        {
            entryItr->second.push_back(originalEntry->second);
            originalStore.erase(key);
        }
    }

    if (hasXName)
    {
        m_resourcesWithXNames.emplace_back(key);
    }

    return S_OK;
}

CustomWriterRuntimeDataTypeIndex ResourceDictionaryCustomRuntimeData::GetTypeIndexForSerialization(_In_ const TargetOSVersion& targetOS) const
{
    return Parser::Versioning::GetResourceDictionarySerializationVersion(targetOS);
}

_Check_return_
HRESULT ResourceDictionaryCustomRuntimeData::SerializeImpl(_In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTable)
{
    IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(*this, writer, streamOffsetTable));

    return S_OK;
}

_Check_return_
HRESULT ResourceDictionaryCustomRuntimeData::ToString(_In_ bool verboseData, _Out_ xstring_ptr& strValue) const
{
    XStringBuilder strBuilder;
    const UINT32 strCount = 256;
    WCHAR *pstrBuffer = nullptr;

    IFC_RETURN(strBuilder.InitializeAndGetFixedBuffer(strCount, &pstrBuffer));

    IFC_RETURN(StringCchPrintf(
        pstrBuffer,
        strCount,
        L"[ResourceDictionary]"));

    IFC_RETURN(strBuilder.DetachString(&strValue));

    return S_OK;
}


_Check_return_
HRESULT ResourceDictionaryCustomRuntimeData::PrepareStream(_In_ std::shared_ptr<SubObjectWriterResult>& customWriterStream)
{
    if (!ShouldEncodeAsCustomData())
    {
        // If we ultimately decided to skip deferral of the corresponding ResourceDictionary, we need to recreate the original
        // nodestream
        auto& nodeListVector = customWriterStream->GetNodeList()->GetNodeList();
        bool encounteredFirstMarker = false;
        bool encounteredAddToDictionary = false;
        unsigned int depth = 0;
        auto itr = nodeListVector.begin();

        for (; itr != nodeListVector.end();)
        {
            auto& node = *itr;

            if (encounteredFirstMarker)
            {
                if (node.RequiresNewScope())
                {
                    ++depth;
                }
                else if (node.RequiresScopeToEnd())
                {
                    --depth;
                }
            }

            // strip the stream offset markers
            if (node.GetNodeType() == ObjectWriterNodeType::StreamOffsetMarker)
            {
                encounteredFirstMarker = true;
                nodeListVector.erase(itr);
            }
            else if (!encounteredFirstMarker)
            {
                nodeListVector.erase(itr);
            }
            else
            {
                ++itr;
                if (depth == 0)
                {
                    break;
                }
                else if (depth == 1 && (node.GetNodeType() == ObjectWriterNodeType::AddToDictionary || node.GetNodeType() == ObjectWriterNodeType::AddToDictionaryWithKey))
                {
                    encounteredAddToDictionary = true;
                }
            }
        }

        while (itr != nodeListVector.end())
        {
            nodeListVector.erase(itr);
        }

        if (!encounteredAddToDictionary)
        {
            if (m_explicitKeyResourcesMap.size() == 1)
            {
                auto spKey = std::make_shared<XamlQualifiedObject>();
                CValue strValue;
                strValue.SetString((*m_explicitKeyResourcesMap.begin()).first);

                spKey->SetValue(strValue);
                IFC_RETURN(customWriterStream->GetNodeList()->AddNode(ObjectWriterNode::MakeAddToDictionaryWithKeyNode(nodeListVector.rbegin()->GetLineInfo(), spKey)));
            }
            else
            {
                IFC_RETURN(customWriterStream->GetNodeList()->AddNode(ObjectWriterNode::MakeAddToDictionaryNode(nodeListVector.rbegin()->GetLineInfo())));
            }
        }
    }
    else
    {
        IFC_RETURN(customWriterStream->GetNodeList()->Optimize());
    }

    return S_OK;
}
#pragma endregion

#pragma region Runtime Methods

// Tries to get the StreamOffsetToken corresponding to the given key. Returns a boolean indicating success.
_Success_(return != false)
bool ResourceDictionaryCustomRuntimeData::TryGetResourceOffset(
    _In_ const xstring_ptr& key,
    _In_ bool isImplicitKey,
    _Out_ StreamOffsetToken& token)
{
    bool foundValue = false;

    auto& primaryStore = (isImplicitKey) ? m_implicitKeyResourcesMap : m_explicitKeyResourcesMap;
    auto search = primaryStore.find(key);

    if (search != primaryStore.end())
    {
        token = search->second;
        foundValue = true;
    }

    return foundValue;
}

_Check_return_ HRESULT ResourceDictionaryCustomRuntimeData::ResolveConditionalResources(_In_ std::shared_ptr<ParserErrorReporter> parserErrorReporter)
{
    IFC_RETURN(ResolveConditionalResourcesHelper(false, parserErrorReporter));
    IFC_RETURN(ResolveConditionalResourcesHelper(true, parserErrorReporter));

    m_conditionalResourcesResolved = true;

    return S_OK;
}

_Check_return_ HRESULT ResourceDictionaryCustomRuntimeData::ResolveConditionalResourcesHelper(
    _In_ bool implicitResources,
    _In_ std::shared_ptr<ParserErrorReporter> parserErrorReporter)
{
    auto& conditionalStore = (implicitResources) ? m_conditionalImplicitKeyResources : m_conditionalExplicitKeyResources;
    auto& primaryStore = (implicitResources) ? m_implicitKeyResourcesMap : m_explicitKeyResourcesMap;

    if (!m_conditionalResourcesResolved)
    {
        StreamOffsetToken token;
        for (auto& conditionalResource : conditionalStore)
        {
            bool foundValue = false;

            // Each candidate StreamOffsetToken has zero or more XamlPredicateAndArgs associated with it.
            // Evaluate each set of XamlPredicateAndArgs to determine if the resource exists at runtime.
            // Only one candidate should meet this requirement, which is what we'll return. If more than
            // one candidate matches, then AG_E_PARSER2_OW_DUPLICATE_KEY is thrown. We loop through all candidates,
            // even if a match is found, in order to check for this condition.
            for (const auto& candidate : conditionalResource.second)
            {
                if (!IsTokenForIgnoredConditionalObject(candidate))
                {
                    if (foundValue)
                    {
                        // We already found a matching candidate, so this means the same key
                        // has been used multiple times which is an error.
                        // Report the error (via an originate exception), then throw E_FAIL.
                        IFC_RETURN(parserErrorReporter->SetError(AG_E_PARSER2_OW_DUPLICATE_KEY, 0, 0, conditionalResource.first));
                        IFC_RETURN(E_FAIL);
                    }

                    token = candidate;
                    foundValue = true;
                }
            }

            if (foundValue)
            {
                // Now that we know which conditionally declared resource is The One (for the lifetime of the app, that is),
                // insert it into the primary store
                primaryStore.emplace(std::make_pair(conditionalResource.first, token));
            }
        }

        conditionalStore.clear();
        conditionalStore.shrink_to_fit();
    }

    return S_OK;
}

// Returns the number of resources contained in this data structure
std::size_t ResourceDictionaryCustomRuntimeData::size()
{
    ASSERT(m_conditionalResourcesResolved);
    auto explicitKeyResourcesSize = m_explicitKeyResourcesMap.size() + m_conditionalExplicitKeyResources.size();

    return explicitKeyResourcesSize + GetImplicitResourceCount();
}

std::size_t ResourceDictionaryCustomRuntimeData::GetImplicitResourceCount()
{
    ASSERT(m_conditionalResourcesResolved);

    return m_implicitKeyResourcesMap.size() + m_conditionalImplicitKeyResources.size();
}

const std::vector<xstring_ptr> ResourceDictionaryCustomRuntimeData::GetExplicitKeys()
{
    ASSERT(m_conditionalResourcesResolved);

    std::vector<xstring_ptr> keyList;
    keyList.reserve(m_explicitKeyResourcesMap.size() + m_conditionalExplicitKeyResources.size());
    for (const auto& kvp : m_explicitKeyResourcesMap)
    {
        keyList.emplace_back(kvp.first);
    }
    for (const auto& kvp : m_conditionalExplicitKeyResources)
    {
        keyList.emplace_back(kvp.first);
    }
    return keyList;
}

const std::vector<xstring_ptr> ResourceDictionaryCustomRuntimeData::GetImplicitKeys()
{
    ASSERT(m_conditionalResourcesResolved);

    std::vector<xstring_ptr> keyList;
    keyList.reserve(m_implicitKeyResourcesMap.size() + m_conditionalImplicitKeyResources.size());
    for (const auto& kvp : m_implicitKeyResourcesMap)
    {
        keyList.emplace_back(kvp.first);
    }
    for (const auto& kvp : m_conditionalImplicitKeyResources)
    {
        keyList.emplace_back(kvp.first);
    }
    return keyList;
}

#pragma endregion
