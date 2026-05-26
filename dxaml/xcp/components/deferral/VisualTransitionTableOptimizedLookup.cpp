// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "VisualTransitionTableOptimizedLookup.h"

VisualTransitionTableOptimizedLookup::VisualTransitionTableOptimizedLookup(
    std::function<bool(const wchar_t*, unsigned int*)> visualStateIndexLookupFunc)
    : m_visualStateIndexLookupFunc(std::move(visualStateIndexLookupFunc))
{
}

void VisualTransitionTableOptimizedLookup::RecordTransitionGroup(_In_ unsigned int groupIndex,
    _In_ unsigned int transitionIndex, _In_ const VisualTransitionEssence& transition)
{
    if (transition.GetFromState().IsNullOrEmpty() &&
        transition.GetToState().IsNullOrEmpty())
    {
        if (m_groupToDefaultTransitionMap.size() <= groupIndex)
        {
            m_groupToDefaultTransitionMap.resize(groupIndex + 1, -1);
        }
        m_groupToDefaultTransitionMap[groupIndex] = transitionIndex;
    }
    m_visualStateToTransitionMap.push_back(std::make_pair(VisualStateIndexes(), transitionIndex));
}

void VisualTransitionTableOptimizedLookup::BuildLookupTable(_In_ std::vector<VisualTransitionEssence>& transitions)
{
    for (auto& stateIndexTransitionPair : m_visualStateToTransitionMap)
    {
        unsigned int visualStateIndex = 0;
        stateIndexTransitionPair.first.FromIndex = m_visualStateIndexLookupFunc(
            transitions[stateIndexTransitionPair.second].GetFromState().GetBuffer(), &visualStateIndex) ?
        visualStateIndex : -1;
        stateIndexTransitionPair.first.ToIndex = m_visualStateIndexLookupFunc(
            transitions[stateIndexTransitionPair.second].GetToState().GetBuffer(), &visualStateIndex) ?
        visualStateIndex : -1;
    }
}

_Success_(return) _Must_inspect_result_
bool VisualTransitionTableOptimizedLookup::TryGetVisualTransitionIndex(
    _In_ unsigned int groupIndex, _In_ int fromIndex,
    _In_ unsigned int toIndex, _Out_ unsigned int* transitionIndex)
{
    int candidateIndex = -1;
    int bestScore = 0;

    for (auto& stateIndexTransitionPair : m_visualStateToTransitionMap)
    {
        bool fromStateMatches = stateIndexTransitionPair.first.FromIndex == fromIndex;
        bool toStateMatches = stateIndexTransitionPair.first.ToIndex == toIndex;

        bool explicitlyNotMatchingState =
            (
            stateIndexTransitionPair.first.FromIndex != fromIndex
            && stateIndexTransitionPair.first.FromIndex != -1
            )
            ||
            (
            stateIndexTransitionPair.first.ToIndex != toIndex
            && stateIndexTransitionPair.first.ToIndex != -1
            );

        if (!explicitlyNotMatchingState)
        {
            int score = 0;
            if (fromStateMatches)
            {
                score += 1;
            }
            if (toStateMatches)
            {
                score += 2;
            }

            if (score > bestScore)
            {
                bestScore = score;
                candidateIndex = static_cast<int>(stateIndexTransitionPair.second);
            }
        }
    }

    if (candidateIndex == -1 && m_groupToDefaultTransitionMap.size() > groupIndex)
    {
        candidateIndex = static_cast<int>(m_groupToDefaultTransitionMap[groupIndex]);
    }

    if (candidateIndex != -1)
    {
        *transitionIndex = static_cast<unsigned int>(candidateIndex);
    }

    return candidateIndex != -1;
}