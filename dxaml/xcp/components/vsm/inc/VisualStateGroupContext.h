// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include <vector_set.h>
#include <qualifiers\inc\VariantMap.h>
#include <vsm\inc\VisualStateSetterHelper.h>

class CStoryboard;
class CVisualTransition;

class VisualStateGroupContext
{
public:
    VisualStateGroupContext()
        : m_currentState(State::Idle)
        , m_currentVisualStateIndex(-1)
        , m_pendingStoryboard(nullptr)
        , m_pendingStoryboardCount(0)
    {}

    enum class State
    {
        Idle,
        Transitioning,
        StateApplied
    };

    enum class StoryboardType
    {
        Dynamic,
        State,
        Transition
    };

    State CurrentState() const { return m_currentState; }
    int CurrentVisualStateIndex() const { return m_currentVisualStateIndex; }

    void BeginTransitionToState(int stateIndex)
    {
        m_currentVisualStateIndex = stateIndex;
        m_currentState = State::Transitioning;
    }

    void CompleteTransitionToState()
    {
        m_currentState = (m_currentVisualStateIndex != -1) ? State::StateApplied : State::Idle;
    }

    std::vector<std::pair<CStoryboard*, StoryboardType>>& ActiveStoryboards() { return m_activeStoryboards; }
    std::vector<CVisualTransition*>& ActiveTransitions() { return m_activeTransitions; }
    VisualStateSetterHelper::ResolvedVisualStateSetterCollection& ActivePropertySetters() { return m_activePropertySetters; }
    _Ret_maybenull_ CStoryboard* GetPendingStoryboard() const { return m_pendingStoryboard; }
    void SetPendingStoryboard(_In_opt_ CStoryboard* storyboard) { m_pendingStoryboard = storyboard; }
    VisualStateSetterHelper::ResolvedVisualStateSetterCollection& PendingPropertySetters() { return m_pendingPropertySetters; }

    void IncrementPendingStoryboardCompletionCallbackCounter() { m_pendingStoryboardCount++; }
    int DecrementPendingStoryboardCompletionCallbackCounter() { return --m_pendingStoryboardCount; }

    std::shared_ptr<StateTriggerVariantMap>& GetStateTriggerVariantMap() { return m_pStateTriggerVariantMap; }

private:
    State m_currentState;
    int m_currentVisualStateIndex;

    int m_pendingStoryboardCount;

    std::shared_ptr<StateTriggerVariantMap> m_pStateTriggerVariantMap;

    // These vectors maintain the list of storyboards and transitions for each
    // of the VisualStateGroups. In the new design they are kept in a DOCollection
    // on the VisualStateGroupCollection DO. These lists serve to uniquely associate
    // them with certain VSGs. These lists are ONLY for book-keeping purposes and do
    // not keep strong refs on their elements.
    std::vector<std::pair<CStoryboard*, StoryboardType>> m_activeStoryboards;
    VisualStateSetterHelper::ResolvedVisualStateSetterCollection m_activePropertySetters;
    std::vector<CVisualTransition*> m_activeTransitions;
    CStoryboard* m_pendingStoryboard;
    VisualStateSetterHelper::ResolvedVisualStateSetterCollection m_pendingPropertySetters;
};
