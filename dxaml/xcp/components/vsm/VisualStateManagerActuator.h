// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <VisualStateGroupContext.h>

class VisualStateManagerDataSource;
class CStoryboard;
class CFrameworkElement;
class CDependencyObject;
class CEventArgs;
class CVisualTransition;
class VisualTransitionCompletedData;
class CControl;
class CDependencyProperty;

class VisualStateManagerActuator
{
public:
    VisualStateManagerActuator(_In_ CFrameworkElement* target, _In_ VisualStateManagerDataSource* dataSource)
        : m_dataSource(dataSource)
        , m_target(target)
    {}

    _Check_return_ HRESULT ChangeVisualState(_In_ int groupIndex, _In_ int startIndex, _In_ int endIndex, _In_ bool useTransitions);
    _Check_return_ HRESULT SnapToEnd(_In_ int groupIndex, _In_ bool forceSynchronous);
    _Check_return_ HRESULT FinishVisualTransition(_In_ CStoryboard* storyboard, VisualTransitionCompletedData* data);
    _Check_return_ HRESULT InitializeStateTriggers(_In_ CControl* pControl);

    // In the case of modifying the runtime VisualState collection or
    // when using state triggers and falling into trigger state space where
    // no trigger defines the visual state of the control we snap the VSM
    // back to the null state synchronously, killling off any pending 
    // storyboards.
    _Check_return_ HRESULT SynchronouslyResetToNullState(_In_ int groupIndex);

    // Reevaluates and reapplies property setters from all active VisualStates. Delegates the actual work for each
    // VisualState to ReevaluateAppliedPropertySetters().
    _Check_return_ HRESULT RefreshAllAppliedPropertySetters();

    // Reevaluates and reapplies the property setters corresponding to the specified VisualState. This will force
    // Binding reevaluation. Need for the case where GoToState(useTransitions = false) is called with the current
    // VisualState.
    _Check_return_ HRESULT ReevaluateAppliedPropertySetters(_In_ int groupIndex, _In_ int stateIndex);

    // These methods are intended for Edit & Continue scenarios and shouldn't generally be used. They are only
    // intended for modifying storyboards on an active VisualState.
    _Check_return_ HRESULT StopAndRemoveStoryboard(_In_ int groupIndex, _In_ const xref_ptr<CStoryboard>& storyboard);
    _Check_return_ HRESULT StartAndAddStoryboard(_In_ int groupIndex, _In_ const xref_ptr<CStoryboard>& storyboard, VisualStateGroupContext::StoryboardType type);

private:
    _Check_return_ HRESULT AttemptStartStoryboard(_In_ CStoryboard* storyboard, VisualStateGroupContext::StoryboardType type);

    template<size_t newStoryboardCount>
    std::vector<CStoryboard*> GetExistingStoryboards(_In_ int groupIndex,
        _In_ std::array<CStoryboard*, newStoryboardCount> newStoryboards);

    _Check_return_ HRESULT StopAndRemoveStoryboards(_In_ int groupIndex,
        _In_ std::vector<CStoryboard*>& storyboards);

    _Check_return_ HRESULT StopAndRemovePropertySetters(
        _In_ int groupIndex,
        _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& setterModifiedValues);

    _Check_return_ HRESULT CalculateAndMarkDynamicTimelineSteadyState(
        _In_opt_ CStoryboard* destinationStoryboard,
        _In_opt_ CStoryboard* transitionStoryboard,
        _In_ bool destinationStoryboardResponsibleForDynamicTransitions);

    std::vector<CStoryboard*> GetExistingStoryboards(
        _In_ int groupIndex,
        _In_opt_ CStoryboard* transitionStoryboard,
        _In_opt_ CStoryboard* destinationStoryboard,
        _In_opt_ CStoryboard* dynamicStoryboard);

    // This method will process the completion data on a Storyboard if it exists. It is called
    // from the codepath where we are responding to the standard asynchronous completion
    // event, from the path where we need to immediately process this data (e.g. SkipToFill
    // synchronously), and from the path where we're changing states and removing a Storyboard created for 
    // a partially finished VisualTransition.
    //
    // When this method is called from the standard asynchronous processing routine it is effectively
    // a no-op. FinishVisualTransition preprocesses the completion data to ensure a smooth handoff
    // and removes the m_visualTransitionData unique_ptr from the Storyboard. This is needed to ensure
    // the presence of m_visualTransitionData is always maintained up until the moment when the async
    // event fires.
    bool TryProcessCompletedDataIfExists(_In_ CStoryboard* storyboard, _In_ int groupIndex);

    // Determines the set of previously active property setters that need to be explicitly unapplied
    // (i.e. reverted to base value) given the new set of property setters and storyboards. 
    // If a previously active affected object/property pair is going to be modified by the property setters
    // or storyboards that will be applied in the near future, then there is no need to explicitly unapply them
    // since the new setters/storyboards will implicitly clear the value applied by the old property setter,
    // so remove them from our list to prevent clobbering.
    // Modifies 'previouslyActivePropertySetters' in-place.
    _Check_return_ HRESULT SelectExistingPropertySettersToRemove(
        _Inout_ VisualStateSetterHelper::ResolvedVisualStateSetterCollection& previouslyActivePropertySetters,
        _In_ VisualStateSetterHelper::ResolvedVisualStateSetterCollection& destinationPropertySetters,
        _In_opt_ CStoryboard* transitionStoryboard,
        _In_opt_ CStoryboard* destinationStoryboard,
        _In_opt_ CStoryboard* dynamicStoryboard);

    VisualStateManagerDataSource* m_dataSource;

    // We require the target for FindName calls when building 
    // dynamically generated Storyboards.
    CFrameworkElement* m_target;
};

template<size_t newStoryboardCount>
std::vector<CStoryboard*> VisualStateManagerActuator::GetExistingStoryboards(
    _In_ int groupIndex, _In_ std::array<CStoryboard*, newStoryboardCount> newStoryboards)
{
    // Create a small vector of the active storyboards that are no longer
    // being played.
    // Simple n^2 algorithm here. The new storyboards array will only ever be 1 or 2
    // elements large.
    std::vector<CStoryboard*> toRemove;
    for (auto storyboard : m_dataSource->GetActiveStoryboards(groupIndex))
    {
        if (storyboard.first && std::find(newStoryboards.begin(),
            newStoryboards.end(), storyboard.first) == newStoryboards.end())
        {
            toRemove.push_back(storyboard.first);
        }
    }

    return toRemove;
}

