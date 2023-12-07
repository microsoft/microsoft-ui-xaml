// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <VisualStateGroupCollectionCustomRuntimeDataSerializer.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>
#include <StreamOffsetToken.h>

struct VisualStateIndexes
{
    VisualStateIndexes() = default;
    int FromIndex = -1;
    int ToIndex = -1;
};

// This class provides some performance benefits, VisualTransition
// lookups are never performed using the VisualState Name strings, but
// also is important 
class VisualTransitionTableOptimizedLookup
{
    friend _Check_return_ HRESULT CustomRuntimeDataSerializationHelpers::Serialize(const VisualTransitionTableOptimizedLookup& target, XamlBinaryFormatSubWriter2* writer, const std::vector<unsigned int>& streamOffsetTokenTable);
    friend VisualTransitionTableOptimizedLookup CustomRuntimeDataSerializationHelpers::Deserialize<VisualTransitionTableOptimizedLookup>(_In_ XamlBinaryFormatSubReader2* reader);

public:
    VisualTransitionTableOptimizedLookup(
        std::function<bool(const wchar_t*, unsigned int*)> visualStateIndexLookupFunc);

    void RecordTransitionGroup(_In_ unsigned int groupIndex,
        _In_ unsigned int transitionIndex, _In_ const VisualTransitionEssence& transition);

    void BuildLookupTable(_In_ std::vector<VisualTransitionEssence>& transitions);

    _Success_(return) _Must_inspect_result_
    bool TryGetVisualTransitionIndex(
        _In_ unsigned int groupIndex, _In_ int fromIndex, 
        _In_ unsigned int toIndex, _Out_ unsigned int* transitionIndex);

private:

    std::vector<std::pair<VisualStateIndexes, unsigned int>> m_visualStateToTransitionMap;
    std::vector<int> m_groupToDefaultTransitionMap;
    std::function<bool(const wchar_t*, unsigned int*)> m_visualStateIndexLookupFunc;
};
