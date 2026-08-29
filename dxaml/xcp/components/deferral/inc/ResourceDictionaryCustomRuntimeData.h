// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CustomWriterRuntimeData.h>
#include <ResourceDictionaryCustomRuntimeDataSerializer.h>
#include <StreamOffsetToken.h>
#include <xstring_ptr.h>
#include <cstdint>

#include <resources\inc\ResourceDictionaryMapTypes.h>

class ParserErrorReporter;
class ResourceDictionaryCustomWriter;
class CResourceDictionary2;
enum class CustomWriterRuntimeDataTypeIndex : std::uint16_t;

class ResourceDictionaryCustomRuntimeData
    : public CustomWriterRuntimeData
{
    friend HRESULT CustomRuntimeDataSerializationHelpers::Serialize<ResourceDictionaryCustomRuntimeData>(
        const ResourceDictionaryCustomRuntimeData&,
        XamlBinaryFormatSubWriter2*,
        const std::vector<unsigned int>&);
    friend ResourceDictionaryCustomRuntimeData CustomRuntimeDataSerializationHelpers::Deserialize<ResourceDictionaryCustomRuntimeData>(
        XamlBinaryFormatSubReader2*,
        CustomWriterRuntimeDataTypeIndex);

public:

    ResourceDictionaryCustomRuntimeData() = default;

    // Explicitly delete the copy constructor and copy assignment operator
    ResourceDictionaryCustomRuntimeData(const ResourceDictionaryCustomRuntimeData& other) = delete;
    ResourceDictionaryCustomRuntimeData& operator=(const ResourceDictionaryCustomRuntimeData& other) = delete;

    ResourceDictionaryCustomRuntimeData(ResourceDictionaryCustomRuntimeData&& other) = default;
    ResourceDictionaryCustomRuntimeData& operator=(ResourceDictionaryCustomRuntimeData&& other) = default;

// FUTURE: These two groups of methods could be masked behind two composing classes to make it
// more difficult to call them inadvertantly from the wrong contexts.
#pragma region Compiletime Methods
    // Add a resource with its StreamOffsetToken to the custom runtime data.
    _Check_return_ HRESULT AddResourceOffset(
        _In_ const xstring_ptr& key,
        _In_ const StreamOffsetToken& token,
        _In_ bool isImplicitKey,
        _In_ bool shouldAutoUndefer);

    _Check_return_
    HRESULT PrepareStream(_In_ std::shared_ptr<SubObjectWriterResult>& customWriterStream) override;

    _Check_return_
    HRESULT ToString(_In_ bool verboseData, _Out_ xstring_ptr& strValue) const override;

protected:
    _Check_return_
        HRESULT SerializeImpl(_In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTable) override;
    CustomWriterRuntimeDataTypeIndex GetTypeIndexForSerialization(_In_ const TargetOSVersion& targetOS) const override;

#pragma endregion

#pragma region Runtime Methods
public:
    _Success_(return != false)
    bool TryGetResourceOffset(_In_ const ResourceKey& key,
        _Out_ StreamOffsetToken& token);

    std::size_t size();
    std::size_t GetImplicitResourceCount();

    // Returns the list of resources that should be automatically undeferred (e.g. has x:Name or x:ConnectionId)
    const std::vector<xstring_ptr>& GetResourcesForAutoUndeferral() const { return m_resourcesForAutoUndeferral; }

    // Resolves conditionally declared resources. Must be called when this object is associated with
    // a CResourceDictionary2.
    _Check_return_ HRESULT ResolveConditionalResources(_In_ std::shared_ptr<ParserErrorReporter> parserErrorReporter);

    auto begin() const
    {
        ASSERT(m_conditionalResourcesResolved);
        return m_resourcesMap.begin();
    }
    auto end() const
    {
        ASSERT(m_conditionalResourcesResolved);
        return m_resourcesMap.end();
    }
    auto begin()
    {
        ASSERT(m_conditionalResourcesResolved);
        return m_resourcesMap.begin();
    }
    auto end()
    {
        ASSERT(m_conditionalResourcesResolved);
        return m_resourcesMap.end();
    }

#pragma endregion

private:
    // Unified map for runtime lookup of both explicit and implicit keys.
    // The explicit/implicit distinction is encoded in the ResourceKeyStorage key's IsKeyType() bit.
    DeferredResourceMap m_resourcesMap;

    // Unified map for conditionally declared resources (i.e. resources prefixed with an xmlns prefix that
    // maps to a conditional namespace).
    ConditionalDeferredResourceMap m_conditionalResourcesMap;

    std::vector<xstring_ptr> m_resourcesForAutoUndeferral;

    bool m_conditionalResourcesResolved = false;
};

