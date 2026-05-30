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
    _In_ bool shouldAutoUndefer)
{
    ResourceKeyStorage rks(key, isImplicitKey);

    // Check first if the key already exists in the conditional store
    bool conditionalKeyExists = (m_conditionalResourcesMap.find(rks) != m_conditionalResourcesMap.end());

    if (!IsInConditionalScope() && !conditionalKeyExists)
    {
        if (!m_resourcesMap.emplace(rks, token).second)
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
        auto entryItr = m_conditionalResourcesMap.emplace(rks, std::vector<StreamOffsetToken>()).first;
        entryItr->second.push_back(token);
        RecordConditionallyDeclaredObject(token);

        // If this was previously declared outside of a conditional scope, move it over. Odds are better than even
        // that at runtime it'll be a AG_E_PARSER2_OW_DUPLICATE_KEY once all relevant conditional scope predicates are
        // evaluated, but since we can't know for sure, we'll let it pass for now and check at runtime when looking
        // up the correct StreamOffsetToken to use.
        auto originalEntry = m_resourcesMap.find(rks);
        if (originalEntry != m_resourcesMap.end())
        {
            entryItr->second.push_back(originalEntry->second);
            m_resourcesMap.erase(rks);
        }
    }

    if (shouldAutoUndefer)
    {
        m_resourcesForAutoUndeferral.emplace_back(key);
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
    IFC_RETURN(customWriterStream->GetNodeList()->Optimize());

    return S_OK;
}
#pragma endregion

#pragma region Runtime Methods

// Tries to get the StreamOffsetToken corresponding to the given key. Returns a boolean indicating success.
_Success_(return != false)
bool ResourceDictionaryCustomRuntimeData::TryGetResourceOffset(
    _In_ const ResourceKey& key,
    _Out_ StreamOffsetToken& token)
{
    bool foundValue = false;

    auto search = m_resourcesMap.find(key);
    if (search != m_resourcesMap.end())
    {
        token = search->second;
        foundValue = true;
    }

    return foundValue;
}

_Check_return_ HRESULT ResourceDictionaryCustomRuntimeData::ResolveConditionalResources(_In_ std::shared_ptr<ParserErrorReporter> parserErrorReporter)
{
    if (!m_conditionalResourcesResolved)
    {
        StreamOffsetToken token;
        for (auto& conditionalResource : m_conditionalResourcesMap)
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
                        IFC_RETURN(parserErrorReporter->SetError(AG_E_PARSER2_OW_DUPLICATE_KEY, 0, 0, conditionalResource.first.GetKey()));
                        IFC_RETURN(E_FAIL);
                    }

                    token = candidate;
                    foundValue = true;
                }
            }

            if (foundValue)
            {
                m_resourcesMap.emplace(conditionalResource.first, token);
            }
        }

        m_conditionalResourcesMap.clear();
        m_conditionalResourcesResolved = true;
    }

    return S_OK;
}

// Returns the number of resources contained in this data structure
std::size_t ResourceDictionaryCustomRuntimeData::size()
{
    ASSERT(m_conditionalResourcesResolved);
    return m_resourcesMap.size() + m_conditionalResourcesMap.size();
}

std::size_t ResourceDictionaryCustomRuntimeData::GetImplicitResourceCount()
{
    ASSERT(m_conditionalResourcesResolved);

    std::size_t count = 0;
    for (const auto& kvp : m_resourcesMap)
    {
        if (kvp.first.IsKeyType())
        {
            ++count;
        }
    }
    for (const auto& kvp : m_conditionalResourcesMap)
    {
        if (kvp.first.IsKeyType())
        {
            ++count;
        }
    }
    return count;
}

#pragma endregion
