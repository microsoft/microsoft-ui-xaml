// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "VisualStateGroupCollectionCustomRuntimeDataSerializer.h"
#include <xamlbinaryformatsubwriter2.h>
#include <xamlbinaryformatsubreader2.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>
#include "CustomRuntimeDataSerializer.h"
#include "VisualTransitionTableOptimizedLookup.h"
#include <CValue.h>
#include "CustomWriterRuntimeDataTypeIndex.h"

namespace CustomRuntimeDataSerializationHelpers
{
    template<>
    _Check_return_ HRESULT Serialize(_In_ const VisualStateEssence& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(Serialize(target.m_name, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_deferredStoryboardToken, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_hasStoryboard, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_deferredPropertySetterTokens, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_stateTriggerValues, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_extensibleStateTriggerTokens, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_stateTriggerCollectionTokens, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_staticResourceTriggerTokens, writer, streamOffsetTokenTable));
        return S_OK;
    }

    template<>
    VisualStateEssence Deserialize<VisualStateEssence>(_In_ XamlBinaryFormatSubReader2* reader, _In_ CustomWriterRuntimeDataTypeIndex typeIndex)
    {
        VisualStateEssence essence;
        essence.m_name = Deserialize<xstring_ptr>(reader);
        essence.m_deferredStoryboardToken = Deserialize<StreamOffsetToken>(reader);
        essence.m_hasStoryboard = Deserialize<bool>(reader);

        if(typeIndex != CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v1)
        {
            essence.m_deferredPropertySetterTokens = Deserialize<std::vector<StreamOffsetToken>>(reader);
            essence.m_stateTriggerValues = Deserialize<std::vector<std::vector<int>>>(reader);

            if(typeIndex != CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v2)
            {
                essence.m_extensibleStateTriggerTokens = Deserialize<std::vector<StreamOffsetToken>>(reader);
                essence.m_stateTriggerCollectionTokens = Deserialize<std::vector<StreamOffsetToken>>(reader);

                if(typeIndex != CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v3 &&
                        typeIndex != CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v4)
                {
                    essence.m_staticResourceTriggerTokens = Deserialize<std::vector<StreamOffsetToken>>(reader);
                }
            }
        }

        return essence;
    }

    template<>
    _Check_return_ HRESULT Serialize(_In_ const VisualStateGroupEssence& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(Serialize(target.m_name, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_hasDynamicTimelines, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_deferredSelf, writer, streamOffsetTokenTable));
        return S_OK;
    }

    template<>
    VisualStateGroupEssence Deserialize<VisualStateGroupEssence>(_In_ XamlBinaryFormatSubReader2* reader)
    {
        VisualStateGroupEssence essence;
        essence.m_name = Deserialize<xstring_ptr>(reader);
        essence.m_hasDynamicTimelines = Deserialize<bool>(reader);
        essence.m_deferredSelf = Deserialize<StreamOffsetToken>(reader);
        return essence;
    }

    template<>
    _Check_return_ HRESULT Serialize(_In_ const VisualTransitionEssence& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(Serialize(target.m_toState, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_fromState, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_deferredTransitionToken, writer, streamOffsetTokenTable));
        return S_OK;
    }

    template<>
    VisualTransitionEssence Deserialize<VisualTransitionEssence>(_In_ XamlBinaryFormatSubReader2* reader)
    {
        VisualTransitionEssence essence;
        essence.m_toState = Deserialize<xstring_ptr>(reader);
        essence.m_fromState = Deserialize<xstring_ptr>(reader);
        essence.m_deferredTransitionToken = Deserialize<StreamOffsetToken>(reader);
        return essence;
    }

    template<>
    _Check_return_ HRESULT Serialize(_In_ const VisualTransitionTableOptimizedLookup& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(Serialize(target.m_visualStateToTransitionMap, writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(target.m_groupToDefaultTransitionMap, writer, streamOffsetTokenTable));
        return S_OK;
    }

    template<>
    VisualTransitionTableOptimizedLookup Deserialize<VisualTransitionTableOptimizedLookup>(_In_ XamlBinaryFormatSubReader2* reader)
    {
        VisualTransitionTableOptimizedLookup table(nullptr);
        table.m_visualStateToTransitionMap = Deserialize<std::vector<std::pair<VisualStateIndexes, unsigned int>>>(reader);
        table.m_groupToDefaultTransitionMap = Deserialize<std::vector<int>>(reader);
        return table;
    }

    template<>
    _Check_return_ HRESULT Serialize(_In_ const VisualStateGroupCollectionCustomRuntimeData& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_visualStateToGroupMap, writer, streamOffsetTokenTable));
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_visualStates, writer, streamOffsetTokenTable));
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_visualStateGroups, writer, streamOffsetTokenTable));
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_visualTransitions, writer, streamOffsetTokenTable));
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_unexpectedTokensDetected, writer, streamOffsetTokenTable));
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(*target.m_visualTransitionLookup, writer, streamOffsetTokenTable));
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_entireCollectionToken, writer, streamOffsetTokenTable));
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_seenNameDirectives, writer, streamOffsetTokenTable));
        return S_OK;
    }

    template<>
    VisualStateGroupCollectionCustomRuntimeData Deserialize<VisualStateGroupCollectionCustomRuntimeData>(_In_ XamlBinaryFormatSubReader2* reader, _In_ CustomWriterRuntimeDataTypeIndex typeIndex)
    {
        VisualStateGroupCollectionCustomRuntimeData data;

        data.m_visualStateToGroupMap = Deserialize<std::vector<unsigned int>>(reader);
        data.m_visualStates = Deserialize<std::vector<VisualStateEssence>>(reader, typeIndex);
        data.m_visualStateGroups = Deserialize<std::vector<VisualStateGroupEssence>>(reader);
        data.m_visualTransitions = Deserialize<std::vector<VisualTransitionEssence>>(reader);
        data.m_unexpectedTokensDetected = Deserialize<bool>(reader);
        data.m_version = typeIndex;

        data.m_visualTransitionLookup.reset(
            new VisualTransitionTableOptimizedLookup(Deserialize<VisualTransitionTableOptimizedLookup>(reader)));
        data.m_entireCollectionToken = Deserialize<StreamOffsetToken>(reader);

        if (typeIndex == CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v4 ||
            typeIndex == CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5)
        {
            data.m_seenNameDirectives = Deserialize<std::vector<xstring_ptr>>(reader);
        }

        return data;
    }

    template<>
    _Check_return_ HRESULT Serialize(_In_ const VisualStateIndexes& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(Serialize(static_cast<unsigned int>(target.FromIndex), writer, streamOffsetTokenTable));
        IFC_RETURN(Serialize(static_cast<unsigned int>(target.ToIndex), writer, streamOffsetTokenTable));
        return S_OK;
    }

    template<>
    VisualStateIndexes Deserialize<VisualStateIndexes>(_In_ XamlBinaryFormatSubReader2* reader)
    {
        VisualStateIndexes indexes;
        indexes.FromIndex = Deserialize<unsigned int>(reader);
        indexes.ToIndex = Deserialize<unsigned int>(reader);
        return indexes;
    }
}
