// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CustomRuntimeDataSerializer.h"
#include "CustomWriterRuntimeDataTypeIndex.h"
#include "ResourceDictionaryCustomRuntimeDataSerializer.h"

#include <ResourceDictionaryCustomRuntimeData.h>
#include <ResourceDictionaryKey.h>
#include <vector_map.h>
#include <xamlbinaryformatsubreader2.h>
#include <xamlbinaryformatsubwriter2.h>
#include <XbfVersioning.h>

namespace CustomRuntimeDataSerializationHelpers
{
    _Check_return_ HRESULT Serializer<ResourceKeyStorage>::Write(
        _In_ const ResourceKeyStorage& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(Serialize(target.GetKey(), writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.GetHashAndIsKeyType(), writer, streamOffsetTokenTable));
        return S_OK;
    }

    ResourceKeyStorage Serializer<ResourceKeyStorage>::Read(_In_ XamlBinaryFormatSubReader2* reader)
    {
        auto key = Deserialize<xstring_ptr>(reader);
        auto hashAndIsKeyType = Deserialize<std::uint64_t>(reader);
        return ResourceKeyStorage(key, hashAndIsKeyType);
    }

    template<>
    _Check_return_ HRESULT Serialize(_In_ const ResourceDictionaryCustomRuntimeData& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        auto typeIndex = Parser::Versioning::GetResourceDictionarySerializationVersion(writer->GetTargetOSVersion());

        containers::vector_map<xstring_ptr, StreamOffsetToken> implicitResourcesMap, explicitResourcesMap;
        containers::vector_map<xstring_ptr, std::vector<StreamOffsetToken>> conditionalImplicitResourcesMap, conditionalExplicitResourcesMap;
        std::vector<xstring_ptr> implicitStyleKeys;

        auto constructLegacyMaps = [&]()
        {
            for (const auto& item : target.m_resourcesMap)
            {
                if (item.first.IsKeyType())
                {
                    implicitResourcesMap.emplace(item.first.GetKey(), item.second);
                    implicitStyleKeys.emplace_back(item.first.GetKey());
                }
                else
                {
                    explicitResourcesMap.emplace(item.first.GetKey(), item.second);
                }
            }

            for (const auto& item : target.m_conditionalResourcesMap)
            {
                auto& container = (item.first.IsKeyType()) ? conditionalImplicitResourcesMap : conditionalExplicitResourcesMap;
                container.emplace(item.first.GetKey(), item.second);
            }
        };

        // A switch/case statement, despite being more verbose, makes it more obvious what the actual format is for any given
        // version of this data structure given that changes introduced in subsequent versions tend to be complex rather
        // than simply additive.
        switch (typeIndex)
        {
            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v1:
            {
                constructLegacyMaps();
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(explicitResourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_resourcesForAutoUndeferral, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(implicitResourcesMap, writer, streamOffsetTokenTable));
                // In ResourceDictionary_v1, we stored a vector containing the keys for all implicit data templates.
                // However, XAML doesn't truly support implicit data templates (unlike SL5 and WPF) so we can just serialize an empty vector
                // This was removed in ResourceDictionary_v2
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(std::vector<xstring_ptr>(), writer, streamOffsetTokenTable));
                // Prior to ResourceDictionary_v3, we stored a vector containing the list of implicit style keys.
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(implicitStyleKeys, writer, streamOffsetTokenTable));
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v2:
            {
                constructLegacyMaps();
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(explicitResourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_resourcesForAutoUndeferral, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(implicitResourcesMap, writer, streamOffsetTokenTable));
                // Prior to ResourceDictionary_v3, we stored a vector containing the list of implicit style keys.
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(implicitStyleKeys, writer, streamOffsetTokenTable));
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3:
            {
                constructLegacyMaps();
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(explicitResourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_resourcesForAutoUndeferral, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(implicitResourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(conditionalExplicitResourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(conditionalImplicitResourcesMap, writer, streamOffsetTokenTable));
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v4:
            {
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_resourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_resourcesForAutoUndeferral, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_conditionalResourcesMap, writer, streamOffsetTokenTable));
            }
            break;

            default:
                ASSERT(false); // unknown case; investigate
        }

        return S_OK;
    }

    using LegacyMapEntries = std::vector<std::pair<xstring_ptr, StreamOffsetToken>>;
    using LegacyConditionalMapEntries = std::vector<std::pair<xstring_ptr, std::vector<StreamOffsetToken>>>;
    // DeferredResourceMap and ConditionalDeferredResourceMap are defined in ResourceDictionaryMapTypes.h

    // Helper: read a legacy (i.e. prior to v4) xstring_ptr-keyed map (serialized as count + pairs of (xstring_ptr, StreamOffsetToken))
    // and merge entries into the unified ResourceKeyStorage-keyed map.
    static void MergeLegacyMapEntries(
        _In_ LegacyMapEntries&& entries,
        _In_ bool isImplicitKey,
        _Inout_ DeferredResourceMap& target)
    {
        target.reserve(entries.size());
        for (auto& entry : entries)
        {
            target.emplace(ResourceKeyStorage(entry.first, isImplicitKey), std::move(entry.second));
        }
    }

    // Helper: read a legacy (i.e. prior to v4) xstring_ptr-keyed conditional map and merge entries into the
    // unified ResourceKeyStorage-keyed conditional map.
    static void MergeLegacyConditionalMapEntries(
        _In_ LegacyConditionalMapEntries&& entries,
        _In_ bool isImplicitKey,
        _Inout_ ConditionalDeferredResourceMap& target)
    {
        target.reserve(entries.size());
        for (auto& entry : entries)
        {
            target.emplace(ResourceKeyStorage(entry.first, isImplicitKey), std::move(entry.second));
        }
    }

    template<>
    ResourceDictionaryCustomRuntimeData Deserialize(_In_ XamlBinaryFormatSubReader2* reader, _In_ CustomWriterRuntimeDataTypeIndex typeIndex)
    {
        ResourceDictionaryCustomRuntimeData data;

        switch (typeIndex)
        {
            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v1:
            {
                MergeLegacyMapEntries(Deserialize<LegacyMapEntries>(reader), false, data.m_resourcesMap);
                data.m_resourcesForAutoUndeferral = Deserialize<std::vector<xstring_ptr>>(reader);
                MergeLegacyMapEntries(Deserialize<LegacyMapEntries>(reader), true, data.m_resourcesMap);
                Deserialize<std::vector<xstring_ptr>>(reader); // discard implicit data templates (unsupported)
                Deserialize<std::vector<xstring_ptr>>(reader); // discard redundant implicit style keys
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v2:
            {
                MergeLegacyMapEntries(Deserialize<LegacyMapEntries>(reader), false, data.m_resourcesMap);
                data.m_resourcesForAutoUndeferral = Deserialize<std::vector<xstring_ptr>>(reader);
                MergeLegacyMapEntries(Deserialize<LegacyMapEntries>(reader), true, data.m_resourcesMap);
                Deserialize<std::vector<xstring_ptr>>(reader); // discard redundant implicit style keys
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3:
            {
                MergeLegacyMapEntries(Deserialize<LegacyMapEntries>(reader), false, data.m_resourcesMap);
                data.m_resourcesForAutoUndeferral = Deserialize<std::vector<xstring_ptr>>(reader);
                MergeLegacyMapEntries(Deserialize<LegacyMapEntries>(reader), true, data.m_resourcesMap);
                MergeLegacyConditionalMapEntries(Deserialize<LegacyConditionalMapEntries>(reader), false, data.m_conditionalResourcesMap);
                MergeLegacyConditionalMapEntries(Deserialize<LegacyConditionalMapEntries>(reader), true, data.m_conditionalResourcesMap);
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v4:
            {
                data.m_resourcesMap = Deserialize<DeferredResourceMap>(reader);
                data.m_resourcesForAutoUndeferral = Deserialize<std::vector<xstring_ptr>>(reader);
                data.m_conditionalResourcesMap = Deserialize<ConditionalDeferredResourceMap>(reader);
            }
            break;

            default:
                ASSERT(false); // unknown case; investigate
        }

        return data;
    }
}
