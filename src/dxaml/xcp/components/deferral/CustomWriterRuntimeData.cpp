// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CustomWriterRuntimeData.h>
#include "CustomWriterRuntimeDataTypeIndex.h"
#include "CustomRuntimeDataSerializer.h"
#include "ResourceDictionaryCustomRuntimeDataSerializer.h"
#include "VisualStateGroupCollectionCustomRuntimeDataSerializer.h"
#include "StyleCustomRuntimeDataSerializer.h"
#include "DeferredElementCustomRuntimeDataSerializer.h"

#include <DeferredElementCustomRuntimeData.h>
#include <ObjectWriterNodeList.h>
#include <ResourceDictionaryCustomRuntimeData.h>
#include <StreamOffsetToken.h>
#include <StyleCustomRuntimeData.h>
#include <SubObjectWriterResult.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>
#include <XamlBinaryFormatSubWriter2.h>
#include <XamlPredicateHelpers.h>
#include <XamlPredicateService.h>
#include <XamlQualifiedObject.h>
#include <XamlSchemaContext.h>

using namespace CustomRuntimeDataSerializationHelpers;

namespace
{
    bool RequiresConditionallyDeclaredObjectsDeserialization(CustomWriterRuntimeDataTypeIndex typeIndex)
    {
        switch (typeIndex)
        {
            case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3:
            {
                return true;
            }
            default:
            {
                return false;
            }
        }
    }
}

_Check_return_
HRESULT CustomWriterRuntimeData::Serialize(_In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTable)
{
    IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(GetTypeIndexForSerialization(writer->GetTargetOSVersion()), writer, streamOffsetTable));
    IFC_RETURN(SerializeImpl(writer, streamOffsetTable));

    if (RequiresConditionallyDeclaredObjectsDeserialization(GetTypeIndexForSerialization(writer->GetTargetOSVersion())))
    {
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(m_conditionallyDeclaredObjects, writer, streamOffsetTable));
    }

    return S_OK;
}

template <typename T>
static std::unique_ptr<CustomWriterRuntimeData> CreateAndDeserializeRuntimeData(
    _In_ XamlBinaryFormatSubReader2* reader,
    _In_ CustomWriterRuntimeDataTypeIndex typeIndex)
{
    return std::unique_ptr<CustomWriterRuntimeData>(new T(Deserialize<T>(reader, typeIndex)));
}

_Check_return_
HRESULT CustomWriterRuntimeData::Deserialize(_In_ XamlBinaryFormatSubReader2* reader, _In_ std::unique_ptr<CustomWriterRuntimeData>& runtimeData)
{
    auto typeIndex = CustomRuntimeDataSerializationHelpers::Deserialize<CustomWriterRuntimeDataTypeIndex>(reader);

    switch (typeIndex)
    {
        case CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v1:
        case CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v2:
        case CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v3:
        case CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v4:
        case CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5:
            runtimeData = CreateAndDeserializeRuntimeData<VisualStateGroupCollectionCustomRuntimeData>(reader, typeIndex);
            break;

        case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v1:
        case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v2:
        case CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3:
            runtimeData = CreateAndDeserializeRuntimeData<ResourceDictionaryCustomRuntimeData>(reader, typeIndex);
            break;

        case CustomWriterRuntimeDataTypeIndex::Style_v1:
        case CustomWriterRuntimeDataTypeIndex::Style_v2:
            runtimeData = CreateAndDeserializeRuntimeData<StyleCustomRuntimeData>(reader, typeIndex);
            break;

        case CustomWriterRuntimeDataTypeIndex::DeferredElement_v1:
        case CustomWriterRuntimeDataTypeIndex::DeferredElement_v2:
        case CustomWriterRuntimeDataTypeIndex::DeferredElement_v3:
            runtimeData = CreateAndDeserializeRuntimeData<DeferredElementCustomRuntimeData>(reader, typeIndex);
            break;

        default:
            // Version mismatched. Runtime data not recognized. 
            IFCFAILFAST(E_FAIL);
            break;
    }

    if (RequiresConditionallyDeclaredObjectsDeserialization(typeIndex))
    {
        runtimeData->m_conditionallyDeclaredObjects = CustomRuntimeDataSerializationHelpers::Deserialize<containers::vector_map<StreamOffsetToken, std::vector<std::shared_ptr<Parser::XamlPredicateAndArgs>>>>(reader);
    }

    return S_OK;
}

_Check_return_
HRESULT CustomWriterRuntimeData::PrepareStream(_In_ std::shared_ptr<SubObjectWriterResult>& customWriterStream)
{
    return customWriterStream->GetNodeList()->Optimize();
}

void CustomWriterRuntimeData::RecordConditionallyDeclaredObject(StreamOffsetToken token)
{
    auto result = m_conditionallyDeclaredObjects.emplace(token, std::vector<std::shared_ptr<Parser::XamlPredicateAndArgs>>(m_conditionalScopes));
    ASSERT(result.second); // token should not have already been inserted
}

// Should the condtionally declared object represented by the input StreamOffsetToken be
// ignored, i.e. does its associated XamlPredicateAndArgs evaluate to false.
bool CustomWriterRuntimeData::IsTokenForIgnoredConditionalObject(StreamOffsetToken token) const
{
    auto result = m_conditionallyDeclaredObjects.find(token);
    if (result != m_conditionallyDeclaredObjects.end())
    {
        auto evaluation = std::find_if(result->second.begin(),
            result->second.end(),
            [](const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs)
            {
                return !Parser::XamlPredicateService::EvaluatePredicate(xamlPredicateAndArgs->PredicateType, xamlPredicateAndArgs->Arguments);
            });

        return (evaluation != result->second.end());
    }

    return false;
}
