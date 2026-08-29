// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "VisualStateGroupCollectionCustomRuntimeData.h"
#include "VisualTransitionTableOptimizedLookup.h"

#include "CustomRuntimeDataSerializer.h"
#include "VisualStateGroupCollectionCustomRuntimeDataSerializer.h"
#include "CustomWriterRuntimeDataTypeIndex.h"

#include <CValue.h>
#include <XbfVersioning.h>
#include <XStringBuilder.h>

VisualStateGroupCollectionCustomRuntimeData::VisualStateGroupCollectionCustomRuntimeData()
    : m_visualTransitionLookup(new VisualTransitionTableOptimizedLookup(
        std::bind(
            &VisualStateGroupCollectionCustomRuntimeData::TryGetVisualStateIndex, 
            this, std::placeholders::_1, std::placeholders::_2)))
    , m_unexpectedTokensDetected(false)
    , m_version(CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v1)
{}

VisualStateGroupCollectionCustomRuntimeData::~VisualStateGroupCollectionCustomRuntimeData()
{}

CustomWriterRuntimeDataTypeIndex VisualStateGroupCollectionCustomRuntimeData::GetTypeIndexForSerialization(_In_ const TargetOSVersion& targetOS) const
{
    return Parser::Versioning::GetVisualStateGroupCollectionSerializationVersion(targetOS);
}

_Check_return_ HRESULT
VisualStateGroupCollectionCustomRuntimeData::SerializeImpl(_In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTable)
{
    IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(*this, writer, streamOffsetTable));

    return S_OK;
}

_Check_return_
HRESULT VisualStateGroupCollectionCustomRuntimeData::ToString(_In_ bool verboseData, _Out_ xstring_ptr& strValue) const
{
    XStringBuilder strBuilder;
    const UINT32 strCount = 256;
    WCHAR *pstrBuffer = nullptr;

    IFC_RETURN(strBuilder.InitializeAndGetFixedBuffer(strCount, &pstrBuffer));

    IFC_RETURN(StringCchPrintf(
        pstrBuffer,
        strCount,
        L"[VisualStateGroupCollection]"));

    IFC_RETURN(strBuilder.DetachString(&strValue));

    return S_OK;
}

unsigned int
VisualStateGroupCollectionCustomRuntimeData::GetGroupIndex(_In_ unsigned int visualStateIndex) const 
{
    ASSERT(m_visualStateToGroupMap.size() > visualStateIndex);
    return m_visualStateToGroupMap[visualStateIndex]; 
}

bool VisualStateGroupCollectionCustomRuntimeData::HasStoryboard(
    _In_ unsigned int visualStateIndex) const
{
    ASSERT(m_visualStates.size() > visualStateIndex);
    return m_visualStates[visualStateIndex].HasStoryboard(); 
}

bool VisualStateGroupCollectionCustomRuntimeData::HasPropertySetters(
    _In_ unsigned int visualStateIndex) const
{
    ASSERT(m_visualStates.size() > visualStateIndex);
    return m_visualStates[visualStateIndex].HasPropertySetters();
}

StreamOffsetToken VisualStateGroupCollectionCustomRuntimeData::GetStoryboard(
    _In_ unsigned int visualStateIndex) const 
{
    ASSERT(m_visualStates.size() > visualStateIndex);
    ASSERT(m_visualStates[visualStateIndex].HasStoryboard());
    return m_visualStates[visualStateIndex].GetStoryboardToken(); 
}

const std::vector<StreamOffsetToken>& VisualStateGroupCollectionCustomRuntimeData::GetPropertySetterTokens(
    _In_ unsigned int visualStateIndex) const
{
    ASSERT(m_visualStates.size() > visualStateIndex);
    ASSERT(m_visualStates[visualStateIndex].HasPropertySetters());
    return m_visualStates[visualStateIndex].GetPropertySetterTokens();
}

_Success_(return) _Must_inspect_result_
bool VisualStateGroupCollectionCustomRuntimeData::TryGetVisualStateIndex(
    _In_z_ const wchar_t* name,
    _Out_ unsigned int * visualStateIndex) const
{
    bool found = false;

    *visualStateIndex = 0;

    for (auto& stateEssence : m_visualStates)
    {
        if (
            (stateEssence.GetName().GetBuffer() == nullptr && name == nullptr) ||
            (
                stateEssence.GetName().GetBuffer() &&
                name &&
                wcscmp(stateEssence.GetName().GetBuffer(), name) == 0
            )
        )
        {
            found = true;
            break;
        }
        (*visualStateIndex)++;
    }

    return found;
}

size_t VisualStateGroupCollectionCustomRuntimeData::GetVisualStateGroupCount() const
{
    return m_visualStateGroups.size();
}

size_t VisualStateGroupCollectionCustomRuntimeData::GetVisualStateCount() const
{
    return m_visualStates.size();
}

bool VisualStateGroupCollectionCustomRuntimeData::HasVisualTransitions(_In_ unsigned int groupIndex) const
{
    return m_visualStateGroups[groupIndex].HasDynamicTimelines();
}


bool VisualStateGroupCollectionCustomRuntimeData::HasStateTriggers(_In_ unsigned int visualStateIndex) const
{
    ASSERT(m_visualStates.size() > visualStateIndex);
    return m_visualStates[visualStateIndex].HasStateTriggers();
}

const std::vector<std::vector<int>>& VisualStateGroupCollectionCustomRuntimeData::GetStateTriggers(_In_ unsigned int visualStateIndex) const
{
    ASSERT(m_visualStates.size() > visualStateIndex);
    return m_visualStates[visualStateIndex].GetStateTriggerValues();
}

const std::vector<StreamOffsetToken>& VisualStateGroupCollectionCustomRuntimeData::GetExtensibleStateTriggerTokens(_In_ unsigned int visualStateIndex) const
{
    ASSERT(m_visualStates.size() > visualStateIndex);
    return m_visualStates[visualStateIndex].GetExtensibleStateTriggerTokens();
}

// Returns all pairs of StreamOffsetTokens for open and close StateTrigger elements
// These token offsets are used to skip creation of StateTriggers when VisualStateGroupCollection is 'faulted in' 
const std::vector<StreamOffsetToken> VisualStateGroupCollectionCustomRuntimeData::GetStateTriggerCollectionTokens() const
{
    std::vector<StreamOffsetToken> skipIndexRanges;
    for(auto& vs : m_visualStates)
    {
        auto tokens = vs.GetStateTriggerCollectionTokens(); 
        skipIndexRanges.insert(skipIndexRanges.end(), tokens.begin(), tokens.end());
    }

    return skipIndexRanges;
}

const std::vector<StreamOffsetToken>& VisualStateGroupCollectionCustomRuntimeData::GetStaticResourceTriggerTokens(_In_ unsigned int visualStateIndex) const
{
    ASSERT(m_visualStates.size() > visualStateIndex);
    return m_visualStates[visualStateIndex].GetStaticResourceTriggerTokens();
}

// Test hooks
std::vector<std::wstring> VisualStateGroupCollectionCustomRuntimeData::GetVisualStateNamesForGroup(_In_ unsigned int groupIndex) const
{
    std::vector<std::wstring> returnNames;

    unsigned int index = 0;
    for (auto& stateEssence : m_visualStates)
    {
        if (m_visualStateToGroupMap[index] == groupIndex)
        {
            returnNames.emplace_back(stateEssence.GetName().GetBuffer());
        }
        index++;
    }

    return returnNames;
}

std::vector<std::wstring> VisualStateGroupCollectionCustomRuntimeData::GetVisualStateGroupNames() const
{
    std::vector<std::wstring> returnNames;

    for (auto& groupEssense : m_visualStateGroups)
    {
        returnNames.push_back(std::wstring(groupEssense.GetName().GetBuffer()));
    }

    return returnNames;
}

bool VisualStateGroupCollectionCustomRuntimeData::TryGetVisualTransition(
    _In_ int fromIndex, _In_ unsigned int toIndex, _Out_ StreamOffsetToken* token) const
{
    *token = StreamOffsetToken();  // Use default constructor to initialize token
    unsigned int groupIndex = GetGroupIndex(toIndex);
    unsigned int transitionIndex = 0;
    if (m_visualTransitionLookup->TryGetVisualTransitionIndex(groupIndex, fromIndex, toIndex, &transitionIndex))
    {
        *token = m_visualTransitions[transitionIndex].GetVisualTransitionToken();
        return true;
    }
    return false;
}

bool VisualStateGroupCollectionCustomRuntimeData::ShouldBailOut() const
{
    return m_unexpectedTokensDetected;
}

StreamOffsetToken VisualStateGroupCollectionCustomRuntimeData::GetEntireGroupCollectionToken() const
{
    return m_entireCollectionToken;
}

const xstring_ptr& VisualStateGroupCollectionCustomRuntimeData::GetVisualStateName(_In_ unsigned int visualStateIndex) const
{
    ASSERT(m_visualStates.size() > visualStateIndex);
    return m_visualStates[visualStateIndex].GetName();
}

const std::vector<xstring_ptr>& VisualStateGroupCollectionCustomRuntimeData::GetSeenNames() const
{
    return m_seenNameDirectives;
}

unsigned int VisualStateGroupCollectionCustomRuntimeData::GetGroupVisualStateIndex(_In_ unsigned int visualStateIndex) const
{
    // The visualStateToGroup map is filled sequentially, since the visual state index is the absolute
    // index of the visual state we have to subtract out all the visual states for the groups in front
    // of it to remain accurate.
    unsigned int returnIndex = static_cast<unsigned int>(visualStateIndex);
    unsigned int groupIndex = m_visualStateToGroupMap[visualStateIndex];
    for (unsigned int i = 0; i < visualStateIndex; i++)
    {
        if (m_visualStateToGroupMap[i] == groupIndex)
        {
            return returnIndex;
        }
        returnIndex--;
    }
    return 0;
}

bool VisualStateGroupCollectionCustomRuntimeData::ShouldBailOutForUserControls() const
{
    return m_version == CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v1 ||
        m_version == CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v2 ||
        m_version == CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v3;
}
