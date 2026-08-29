// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "StyleCustomRuntimeData.h"
#include "StyleCustomWriter.h"
#include "CustomRuntimeDataSerializer.h"
#include "StyleCustomRuntimeDataSerializer.h"
#include "CustomWriterRuntimeDataTypeIndex.h"

#include <CValue.h>
#include <ObjectWriterNodeList.h>
#include <SubObjectWriterResult.h>
#include <XamlQualifiedObject.h>
#include <XamlSchemaContext.h>
#include <XbfVersioning.h>
#include <XStringBuilder.h>

CustomWriterRuntimeDataTypeIndex StyleCustomRuntimeData::GetTypeIndexForSerialization(_In_ const TargetOSVersion& targetOS) const
{
    return Parser::Versioning::GetStyleSerializationVersion(targetOS);
}

_Check_return_ HRESULT
StyleCustomRuntimeData::SerializeImpl(_In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTable)
{
    // Pass this data to helpers, which will do the work of serializing the vector of StyleSetterEssence's.
    // Forward the offset table to use for serializing any StreamOffsetToken's.
    IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(*this, writer, streamOffsetTable));

    return S_OK;
}

_Check_return_ HRESULT
StyleCustomRuntimeData::ToString(_In_ bool verboseData, _Out_ xstring_ptr& strValue) const
{
    XStringBuilder strBuilder;
    const UINT32 strCount = 256;
    WCHAR *pstrBuffer = nullptr;

    IFC_RETURN(strBuilder.InitializeAndGetFixedBuffer(strCount, &pstrBuffer));

    IFC_RETURN(StringCchPrintf(
        pstrBuffer,
        strCount,
        L"[Style]"));

    IFC_RETURN(strBuilder.DetachString(&strValue));

    return S_OK;
}

// Erase all nodes from the node sub-stream except those in the scope following an offset marker.
//
// Example xaml:
//    <Style x:Key='myStyle' TargetType='Button'>
//        <Setter Property='Template'>
//            <Setter.Value>
//                <ControlTemplate TargetType='Button'/>
//            </Setter.Value>
//        </Setter>
//    </Style>
//
// Node list before/after:
//    PushScope
//        PushScope --ERASED
//            PushConstant --ERASED
//            TypeConvertValue --ERASED
//            SetDirectiveProperty [x:Key] --ERASED
//        PopScope --ERASED
//        CreateType [Style] --ERASED
//        BeginInit --ERASED
//        PushScope --ERASED
//            PushResolvedType --ERASED
//            TypeConvertValue --ERASED
//            SetValue [TargetType] --ERASED
//        PopScope --ERASED
//        PushScope --ERASED
//            GetValue [Style.Setters] --ERASED
//            PushScope --ERASED
//                CreateType [Setter] --ERASED
//                BeginInit --ERASED
//                PushScope --ERASED
//                    PushResolvedProperty --ERASED
//                    TypeConvertValue --ERASED
//                    SetValue [Setter.Property] --ERASED
//                PopScope --ERASED
//                StreamOffsetMarker
//                PushScope
//                    CreateType [ControlTemplate]
//                    BeginInit
//                    PushScope
//                        PushResolvedType
//                        TypeConvertValue
//                        SetValue [TargetType]
//                    PopScope
//                    EndInit
//                    SetValue [Setter.Value]
//                PopScope
//                EndInit --ERASED
//                AddToCollection --ERASED
//            PopScope --ERASED
//        PopScope --ERASED
//        EndInit --ERASED
//    PopScope
_Check_return_ HRESULT StyleCustomRuntimeData::PrepareStream(_In_ std::shared_ptr<SubObjectWriterResult>& customWriterStream)
{
    size_t offsetToken = 0;
    auto& nodeList = customWriterStream->GetNodeList()->GetNodeList();
    auto itr = nodeList.begin();

    ASSERT(itr != nodeList.end() && itr->GetNodeType() == ObjectWriterNodeType::PushScope);

    // Skip the initial PushScope node, and stop when we reach its matching PopScope node.
    itr++;
    int depth = 0;
    while (itr != nodeList.end())
    {
        auto& node = *itr;

        if (node.RequiresScopeToEnd())
            depth--;
        else if (node.RequiresNewScope())
            depth++;

        if (depth < 0)
            break;

        if (node.GetNodeType() == ObjectWriterNodeType::StreamOffsetMarker)
            SkipMarkerNodes(nodeList, itr, m_offsetTokenStates[offsetToken++]);
        else
            nodeList.erase(itr);
    }

    IFC_RETURN(customWriterStream->GetNodeList()->Optimize());

    return S_OK;
}

// Progresses through the given node list until reaching the end of the current scope.
void StyleCustomRuntimeData::SkipMarkerNodes(_In_ std::vector<ObjectWriterNode>& nodeList, _In_ std::vector<ObjectWriterNode>::iterator& itr, std::pair<bool, bool> offsetTokenState)
{
    unsigned int depth = 0;
    bool isSetter = offsetTokenState.first;
    bool isOptimizedSetter = isSetter && offsetTokenState.second;

    ASSERT(itr != nodeList.end() && itr->GetNodeType() == ObjectWriterNodeType::StreamOffsetMarker);

    itr++;

    // Just return if the offset marker is for setter nodes that won't be used at run time.
    if (isOptimizedSetter)
        return;

    ASSERT(itr != nodeList.end() && !itr->RequiresScopeToEnd());

    while (itr != nodeList.end())
    {
        auto& node = *itr;

        if (node.RequiresNewScope())
            depth++;
        else if (node.RequiresScopeToEnd())
            depth--;

        itr++;

        if (depth == 0)
            break;
    }

    // If we're handling nodes for a setter, remove the AddToCollection node
    // since there isn't a setters collection in an optimized style.
    if (isSetter)
    {
        // Reverse to node before the last PopScope
        itr--;
        ASSERT(itr != nodeList.begin() && itr->RequiresScopeToEnd());
        itr--;

        if (itr->GetNodeType() == ObjectWriterNodeType::AddToCollection)
            nodeList.erase(itr);  // remove AddToCollection
        else
            itr++;                // not AddToCollection, just skip

        // Advance past PopScope again
        itr++;
    }
}
