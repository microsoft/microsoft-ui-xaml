// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CustomRuntimeDataSerializer.h"
#include "CustomWriterRuntimeDataTypeIndex.h"
#include "ResourceDictionaryCustomRuntimeDataSerializer.h"

#include <ResourceDictionaryCustomRuntimeData.h>
#include <xamlbinaryformatsubreader2.h>
#include <xamlbinaryformatsubwriter2.h>
#include <XbfVersioning.h>

namespace CustomRuntimeDataSerializationHelpers
{
    template<>
    _Check_return_ HRESULT Serialize(_In_ const ResourceDictionaryCustomRuntimeData& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        auto typeIndex = Parser::Versioning::GetResourceDictionarySerializationVersion(writer->GetTargetOSVersion());

        // A switch/case statement, despite being more verbose, makes it more obvious what the actual format is for any given
        // version of this data structure given that changes introduced in subsequent versions tend to be complex rather
        // than simply additive.
        switch (typeIndex)
        {
            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v1:
            {
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_explicitKeyResourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_resourcesWithXNames, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_implicitKeyResourcesMap, writer, streamOffsetTokenTable));
                // In ResourceDictionary_v1, we stored a vector containing the keys for all implicit data templates.
                // However, XAML doesn't truly support implicit data templates (unlike SL5 and WPF) so we can just serialize an empty vector
                // This was removed in ResourceDictionary_v2
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(std::vector<xstring_ptr>(), writer, streamOffsetTokenTable));
                // Prior to ResourceDictionary_v3, we stored a vector containing the list of implicit style keys.
                {
                    std::vector<xstring_ptr> implicitStyleKeys;
                    implicitStyleKeys.reserve(target.m_implicitKeyResourcesMap.size());
                    for (const auto& item : target.m_implicitKeyResourcesMap)
                    {
                        implicitStyleKeys.emplace_back(item.first);
                    }
                    IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(implicitStyleKeys, writer, streamOffsetTokenTable));
                }
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v2:
            {
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_explicitKeyResourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_resourcesWithXNames, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_implicitKeyResourcesMap, writer, streamOffsetTokenTable));
                // Prior to ResourceDictionary_v3, we stored a vector containing the list of implicit style keys.
                {
                    std::vector<xstring_ptr> implicitStyleKeys;
                    implicitStyleKeys.reserve(target.m_implicitKeyResourcesMap.size());
                    for (const auto& item : target.m_implicitKeyResourcesMap)
                    {
                        implicitStyleKeys.emplace_back(item.first);
                    }
                    IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(implicitStyleKeys, writer, streamOffsetTokenTable));
                }
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3:
            {
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_explicitKeyResourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_resourcesWithXNames, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_implicitKeyResourcesMap, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_conditionalExplicitKeyResources, writer, streamOffsetTokenTable));
                IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_conditionalImplicitKeyResources, writer, streamOffsetTokenTable));
            }
            break;

            default:
                ASSERT(false); // unknown case; investigate
        }

        return S_OK;
    }

    template<>
    ResourceDictionaryCustomRuntimeData Deserialize(_In_ XamlBinaryFormatSubReader2* reader, _In_ CustomWriterRuntimeDataTypeIndex typeIndex)
    {
        ResourceDictionaryCustomRuntimeData data;

        // A switch/case statement, despite being more verbose, makes it more obvious what the actual format is for any given
        // version of this data structure given that changes introduced in subsequent versions tend to be complex rather
        // than simply additive.
        switch (typeIndex)
        {
            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v1:
            {
                data.m_explicitKeyResourcesMap = Deserialize<containers::vector_map<xstring_ptr, StreamOffsetToken>>(reader);
                data.m_resourcesWithXNames = Deserialize<std::vector<xstring_ptr>>(reader);
                data.m_implicitKeyResourcesMap = Deserialize<containers::vector_map<xstring_ptr, StreamOffsetToken>>(reader);
                // In ResourceDictionary_v1, we stored a vector containing the keys for all implicit data templates.
                // However, XAML doesn't truly support implicit data templates (unlike SL5 and WPF) so for ResourceDictionary_v2,
                // we removed all references to implicit data templates, but the stored data still needs to be deserialized from
                // ResourceDictionary_v1.
                Deserialize<std::vector<xstring_ptr>>(reader);
                // Prior to ResourceDictionary_v3, we stored a vector containing the list of implicit style keys. This is actually
                // redundant given the presence of m_implicitKeyResources (plus we only actually need the number of keys, not the
                // keys themselves), so we were basically just wasting space. Starting with ResourceDictionary_v3, this information
                // is no longer stored, but we still need to deserialize the data from v1 and v2.
                Deserialize<std::vector<xstring_ptr>>(reader);
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v2:
            {
                data.m_explicitKeyResourcesMap = Deserialize<containers::vector_map<xstring_ptr, StreamOffsetToken>>(reader);
                data.m_resourcesWithXNames = Deserialize<std::vector<xstring_ptr>>(reader);
                data.m_implicitKeyResourcesMap = Deserialize<containers::vector_map<xstring_ptr, StreamOffsetToken>>(reader);
                // Prior to ResourceDictionary_v3, we stored a vector containing the list of implicit style keys. This is actually
                // redundant given the presence of m_implicitKeyResources (plus we only actually need the number of keys, not the
                // keys themselves), so we were basically just wasting space. Starting with ResourceDictionary_v3, this information
                // is no longer stored, but we still need to deserialize the data from v1 and v2.
                Deserialize<std::vector<xstring_ptr>>(reader);
            }
            break;

            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3:
            {
                data.m_explicitKeyResourcesMap = Deserialize<containers::vector_map<xstring_ptr, StreamOffsetToken>>(reader);
                data.m_resourcesWithXNames = Deserialize<std::vector<xstring_ptr>>(reader);
                data.m_implicitKeyResourcesMap = Deserialize<containers::vector_map<xstring_ptr, StreamOffsetToken>>(reader);
                data.m_conditionalExplicitKeyResources = Deserialize<containers::vector_map<xstring_ptr, std::vector<StreamOffsetToken>>>(reader);
                data.m_conditionalImplicitKeyResources = Deserialize<containers::vector_map<xstring_ptr, std::vector<StreamOffsetToken>>>(reader);
            }
            break;

            default:
                ASSERT(false); // unknown case; investigate
        }

        return data;
    }
}
