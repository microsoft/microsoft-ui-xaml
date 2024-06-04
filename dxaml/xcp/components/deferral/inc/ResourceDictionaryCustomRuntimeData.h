// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CustomWriterRuntimeData.h>
#include <ResourceDictionaryCustomRuntimeDataSerializer.h>
#include <StreamOffsetToken.h>
#include <xstring_ptr.h>
#include <cstdint>

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
        _In_ bool hasXName);

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
    bool TryGetResourceOffset(_In_ const xstring_ptr& key, _In_ bool isImplicitKey,
        _Out_ StreamOffsetToken& token);

    std::size_t size();
    std::size_t GetImplicitResourceCount();

    // These methods carry a performance penalty. Because the list of keys
    // is only needed in the case of faulting in all the resources we don't make a point to
    // store it in this long-term object. Instead the vector is created on-demand.
    const std::vector<xstring_ptr> GetExplicitKeys();
    const std::vector<xstring_ptr> GetImplicitKeys();

    const std::vector<xstring_ptr>& GetResourcesWithXNames() const { return m_resourcesWithXNames; }

    // Resolves conditionally declared resources. Must be called when this object is associated with
    // a CResourceDictionary2.
    _Check_return_ HRESULT ResolveConditionalResources(_In_ std::shared_ptr<ParserErrorReporter> parserErrorReporter);

#pragma endregion

private:
    _Check_return_ HRESULT ResolveConditionalResourcesHelper(
        _In_ bool implicitResources,
        _In_ std::shared_ptr<ParserErrorReporter> parserErrorReporter);

    // Maps for runtime lookup of explicit and implicit keys. The key collections are kept
    // separate because explicit key strings can match implicit key strings, which are merely
    // the name of the type the key is implicitly targetting.
    containers::vector_map<xstring_ptr, StreamOffsetToken> m_explicitKeyResourcesMap;
    containers::vector_map<xstring_ptr, StreamOffsetToken> m_implicitKeyResourcesMap;

    // These hold conditionally declared resources (i.e. resources prefixed with an xmlns prefix that
    // maps to a conditional namespace). We use a vector_map here because we expect it to be small enough
    // that the difference between O(log n) and O(1) lookup is insignificant, and to avoid the need for the
    // vector->unordered_map trickery we do for the main dictionaries in the name of minimizing space overhead.
    containers::vector_map<xstring_ptr, std::vector<StreamOffsetToken>> m_conditionalExplicitKeyResources;
    containers::vector_map<xstring_ptr, std::vector<StreamOffsetToken>> m_conditionalImplicitKeyResources;

    std::vector<xstring_ptr> m_resourcesWithXNames;

    bool m_conditionalResourcesResolved = false;
};

