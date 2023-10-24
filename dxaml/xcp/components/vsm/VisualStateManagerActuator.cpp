// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <VisualStateGroupContext.h>
#include "VisualStateManagerActuator.h"
#include <VisualStateManagerDataSource.h>
#include <CVisualStateManager2.h>
#include <DynamicTransitionStoryboardGenerator.h>
#include <DynamicTimelineHelper.h>

#include <VisualTransition.h>
#include <storyboard.h>
#include <TimelineCollection.h>
#include <DynamicTimeline.h>
#include <DOCollection.h>
#include <CDependencyObject.h>
#include <DOPointerCast.h>
#include <animation.h>
#include <VisualTransitionCompletedData.h>
#include <framework.h>
#include <EventArgs.h>
#include <corep.h>
#include <StateTriggerBase.h>
#include <Timeline.h>
#include <DynamicTimeline.h>
#include <storyboard.h>
#include <TimelineCollection.h>
#include "TimelineLookupList.h"
#include <MetadataAPI.h>
#include <ContentRoot.h>

#define EVENTLISTENER_INTERNAL 4

//#define VSMLOG(...) LOG(__VA_ARGS__)
#define VSMLOG(...)

_Check_return_ HRESULT VisualStateManagerActuator::ChangeVisualState(_In_ int groupIndex, _In_ int startIndex,
    _In_ int endIndex, _In_ bool useTransitions)
{
    VSMLOG(L"[VSMA]: ChangeVisualState");

    // Attempt to get or create the property setters, transition storyboard, and final storyboard
    // for the given state.
    VisualStateSetterHelper::ResolvedVisualStateSetterCollection destinationPropertySetters;
    IFC_RETURN(m_dataSource->TryGetOrCreatePropertySettersForVisualState(endIndex, &destinationPropertySetters));
    std::shared_ptr<CStoryboard> destinationStoryboard;
    IFC_RETURN(m_dataSource->TryGetOrCreateStoryboardForVisualState(endIndex, &destinationStoryboard));

    // Clear any pending property setters, as they are made moot by the new VisualState
    m_dataSource->ClearPendingPropertySetters(groupIndex);

    // When animations are disabled in the case of UISettings global accessibility overrides we
    // simply don't play or even bother to create VisualTransition storyboards.
    std::shared_ptr<CVisualTransition> transition;
    if (useTransitions)
    {
        IFC_RETURN(m_dataSource->TryGetOrCreateTransition(startIndex, endIndex, &transition));
    }
    else
    {
        transition = std::shared_ptr<CVisualTransition>();
    }

    // Store away all the currently active storyboards in case we're going to be creating a dynamic storyboard
    // later on in this method.

    std::vector<CStoryboard*> previouslyActiveAnimations;
    for (auto& storyboard : m_dataSource->GetActiveStoryboards(groupIndex))
    {
        if (!(storyboard.first)->IsInStoppedState())
        {
            previouslyActiveAnimations.push_back(storyboard.first);
        }
    }

    // Cache the currently applied property setters so we can clear them at the end
    VisualStateSetterHelper::ResolvedVisualStateSetterCollection previouslyActivePropertySetters;
    previouslyActivePropertySetters.reserve(m_dataSource->GetActivePropertySetters(groupIndex).size());
    previouslyActivePropertySetters.insert(
        previouslyActivePropertySetters.cend(),
        m_dataSource->GetActivePropertySetters(groupIndex).begin(),
        m_dataSource->GetActivePropertySetters(groupIndex).end());

    // Add the destination storyboard, and the transition to
    // their respective collections. This will cause them to enter the visual tree and get
    // inheritance context updates.
    //
    // We also setup a scopeguard to remove them from the collection if we're unable
    // to Start the storyboards.
    if (destinationStoryboard)
    {
        m_dataSource->AddActiveStoryboard(groupIndex, xref_ptr<CStoryboard>(destinationStoryboard.get()), VisualStateGroupContext::StoryboardType::State);
        VSMLOG(L"[VSMA]: ChangeVisualState - Adding destination storyboard.");
    }
    auto destinationStoryboardCleanupGuard = wil::scope_exit([&] { if (destinationStoryboard) {
            m_dataSource->RemoveActiveStoryboard(groupIndex, destinationStoryboard.get()); }
    });
    if (transition)
    {
        m_dataSource->AddActiveTransition(groupIndex, xref_ptr<CVisualTransition>(transition.get()));
        VSMLOG(L"[VSMA]: ChangeVisualState - Adding transition.");
    }
    auto transitionCleanupGuard = wil::scope_exit([&] { if (transition) {
            m_dataSource->RemoveActiveTransition(groupIndex, transition.get()); }
    });
    if (transition && transition->m_pStoryboard)
    {
        m_dataSource->AddActiveStoryboard(groupIndex, xref_ptr<CStoryboard>(transition->m_pStoryboard), VisualStateGroupContext::StoryboardType::Transition);
        VSMLOG(L"[VSMA]: ChangeVisualState - Adding transition storyboard.");
    }
    auto transitionStoryboardCleanupGuard = wil::scope_exit([&] { if (transition && transition->m_pStoryboard) {
        m_dataSource->RemoveActiveStoryboard(groupIndex, transition->m_pStoryboard); }
    });

    // Ensure that dynamic timelines are created and set to either generate steady state or full timelines.
    // This must be done BEFORE we attempt to generate a dynamic storyboard because that algorithm will
    // iterator through the children of the generated dynamic timeline to duplicate them. This must be
    // done AFTER we add the destination and transition storyboards to the visual tree to ensure the dynamic
    // timelines can properly bind to their targets.
    IFC_RETURN(CalculateAndMarkDynamicTimelineSteadyState(
        destinationStoryboard.get(),
        transition ? transition->m_pStoryboard : nullptr,
        useTransitions && !transition));

    const bool zeroDurationTransition = !transition || transition->IsZeroDuration();

    // NOTE: 'DynamicTimeline' refers to the theme-based animations like FadeOutThemeAnimations.
    //       'DynamicStoryboard' refers to the Storyboard we generate by looking at the current
    //        animations and the destination storyboard and building a storyboard that smoothly transitions
    //        between them.

    // If the visual transition is of a non-zero duration we'll try to build a storyboard that smoothly
    // transitions any properties that aren't explicitly transitioned in the VisualTransition's Storyboard.
    xref_ptr<CStoryboard> dynamicStoryboard;
    if (!zeroDurationTransition)
    {
        IFC_RETURN(DynamicTransitionStoryboardGenerator::GenerateDynamicTransitionAnimations(
                m_target,
                transition.get(),
                previouslyActiveAnimations,
                destinationStoryboard.get(),
                previouslyActivePropertySetters,
                destinationPropertySetters,
                dynamicStoryboard.ReleaseAndGetAddressOf()
            ));

        m_dataSource->AddActiveStoryboard(groupIndex, xref_ptr<CStoryboard>(dynamicStoryboard.get()), VisualStateGroupContext::StoryboardType::Dynamic);
        VSMLOG(L"[VSMA]: ChangeVisualState - Adding dynamic storyboard.");
    }
    auto dynamicStoryboardCleanupGuard = wil::scope_exit([&] { if (dynamicStoryboard) {
            m_dataSource->RemoveActiveStoryboard(groupIndex, dynamicStoryboard.get()); }
    });

    // If we have a non zero VisualTransition and a destination storyboard then
    // we're not actually starting all the storyboards in this method, we have the final
    // destination storyboard left to start. We create a callback and save away some data
    // to make sure that happens.
    if (!zeroDurationTransition)
    {
        ASSERT(dynamicStoryboard || transition->m_pStoryboard);

        CValue handlerCValue;
        handlerCValue.SetInternalHandler(CVisualStateManager2::OnStoryboardCompleted);

        auto subscribeFunc = [&](CStoryboard* storyboard) {
            VisualTransitionCompletedData* pData = nullptr;
            IFC_RETURN(VisualTransitionCompletedData::Create(groupIndex, endIndex, transition.get(), &pData));
            storyboard->m_pVisualTransitionCompletedData = std::unique_ptr<VisualTransitionCompletedData>(pData);
            IFC_RETURN(storyboard->AddEventListener(EventHandle(KnownEventIndex::Timeline_Completed),
                &handlerCValue, EVENTLISTENER_INTERNAL, &pData->m_EventListenerToken));
            m_dataSource->IncrementPendingStoryboardCompletionCallbackCounter(groupIndex);
            return S_OK;
        };
        VSMLOG(L"[VSMA]: ChangeVisualState - Subscribing to animation callback.");
        if (dynamicStoryboard)
        {
            IFC_RETURN(subscribeFunc(dynamicStoryboard.get()));
        }

        if (transition->m_pStoryboard)
        {
            IFC_RETURN(subscribeFunc(transition->m_pStoryboard));
        }

        if (!destinationPropertySetters.empty())
        {
            m_dataSource->AddPendingPropertySetters(groupIndex, destinationPropertySetters);
        }

        m_dataSource->SetPendingStoryboard(groupIndex, destinationStoryboard.get());
    }

    // Finally we're far enough along to attempt to actually start all these storyboards. These
    // actions can potentially fail if they can't resolve their TargetName properties.
    if (transition && transition->m_pStoryboard)
    {
        IFC_RETURN(AttemptStartStoryboard(transition->m_pStoryboard, VisualStateGroupContext::StoryboardType::Transition));
    }

    if (zeroDurationTransition)
    {
        ASSERT(!dynamicStoryboard);
        IFC_RETURN(m_dataSource->SetAndApplyActivePropertySetters(groupIndex, destinationPropertySetters));
        if (destinationStoryboard)
        {
            IFC_RETURN(AttemptStartStoryboard(destinationStoryboard.get(), VisualStateGroupContext::StoryboardType::State));
        }
    }

    if (dynamicStoryboard)
    {
        IFC_RETURN(AttemptStartStoryboard(dynamicStoryboard.get(), VisualStateGroupContext::StoryboardType::Dynamic));
    }

    // Now that we're out of the unexpected-conditions woods we can update the persisted state
    // for future GoToState calls.
    IFC_RETURN(m_dataSource->BeginTransitionToState(groupIndex, endIndex));
    if (zeroDurationTransition)
    {
        IFC_RETURN(m_dataSource->CompleteTransitionToState(groupIndex));
    }

    if (!zeroDurationTransition)
    {
        // If we have a non-zero transition ensure it stays in the visual tree by
        // dismissing it's scope guard.
        transitionCleanupGuard.release();
        transitionStoryboardCleanupGuard.release();
    }
    destinationStoryboardCleanupGuard.release();
    dynamicStoryboardCleanupGuard.release();


    // Finally remove the existing storyboards, starting the new storyboards ensures we do
    // a smooth handoff of the timelines.
    std::vector<CStoryboard*> existingStoryboards = GetExistingStoryboards(
        groupIndex,
        transition ? transition->m_pStoryboard : nullptr,
        destinationStoryboard.get(),
        dynamicStoryboard.get());
    IFC_RETURN(StopAndRemoveStoryboards(groupIndex, existingStoryboards));

    // Remove the old property setters.
    // We need to check our cached list of old modified objects / properties
    // against the list of current modified objects/properties and make sure we don't clear anything
    // that was just (or will be) set, whether by setter or storyboard.
    SelectExistingPropertySettersToRemove(
        previouslyActivePropertySetters,
        destinationPropertySetters,
        transition ? transition->m_pStoryboard : nullptr,
        destinationStoryboard.get(),
        dynamicStoryboard.get());
    IFC_RETURN(StopAndRemovePropertySetters(groupIndex, previouslyActivePropertySetters));

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerActuator::FinishVisualTransition(_In_ CStoryboard* storyboard, VisualTransitionCompletedData* data)
{
    VSMLOG(L"[VSMA]: FinishVisualTransition");

    // This could ethier be a asynchronous complete event or a synchronous completion. There are
    // edge cases where the asynchronous storyboard completed event has already been scheduled but
    // before it runs we do something that requires synchronously processing this event, in those
    // cases we set the HasBeenProcessed flag and don't do any work here above and beyond removing
    // the event registration.
    if (!data->GetHasBeenProcessed())
    {
#if DBG
        auto& groupContext = m_dataSource->GetGroupContext(data->GetGroupIndex());

        ASSERT(groupContext.CurrentVisualStateIndex() == data->GetStateIndex());
        ASSERT(groupContext.CurrentState() == VisualStateGroupContext::State::Transitioning);
#endif

        data->SetHasBeenProcessed(true);
        int pendingCallbacks = m_dataSource->DecrementPendingStoryboardCompletionCallbackCounter(data->GetGroupIndex());
        ASSERT(pendingCallbacks >= 0);

        if (pendingCallbacks == 0)
        {
            VSMLOG(L"[VSMA]: FinishVisualTransition - Removing the visual transition.");
            auto& pendingPropertySetters = m_dataSource->GetPendingPropertySetters(data->GetGroupIndex());
            IFC_RETURN(m_dataSource->SetAndApplyActivePropertySetters(data->GetGroupIndex(), pendingPropertySetters));

            auto pendingStoryboard = m_dataSource->GetPendingStoryboard(data->GetGroupIndex());
            if (pendingStoryboard)
            {
                IFC_RETURN(AttemptStartStoryboard(pendingStoryboard, VisualStateGroupContext::StoryboardType::State));
            }

            // A typical VisualTransition completion occurs when the VSGC stays in the visual tree
            // and the storyboard completion event fires normally. In this case we do the hand-off
            // between the pending storyboard and the current Storyboard correctly and ensure that
            // smooth handoff occurs (the new storyboard must start before the previous storyboard
            // for smooth handoff).
            //
            // This stands in contrast to the logic used in ProcessCompletedDataIfExists, which will simply
            // immediately stop and remove the storyboards without starting the new pending storyboard, a subtly
            // different behavior which is useful in these two situations:
            // - When SkipToFill is called we simply skip the pending storyboard to end and don't worry about
            //   animation handoff
            // - When changing states in the middle of a transition we want the new storyboards to pickup any
            //   handoff behavior, but we don't care about the now irrelevant pending storyboard.
            //
            // Also note that StopAndRemoveStoryboards below actually does end up calling
            // ProcessCompletionDataIfExists. That becomes effectively a no-op because we've already
            // processed the completion data and VisualStateManager2 has cleared the pointer to
            // the data. We keep the clearing of the visual transition data in CVisualStateManager2
            // as it should ONLY happen either during a completion event or during the destruction of
            // the Storyboard during tree teardown.
            //
            // IMPORTANT: If you're making changes to this method it's likely you'll need to make changes
            // to ProcessCompletedDataIfExists as well.
            std::vector<CStoryboard*> existingStoryboards = GetExistingStoryboards(
                data->GetGroupIndex(), nullptr, pendingStoryboard, nullptr);
            IFC_RETURN(StopAndRemoveStoryboards(data->GetGroupIndex(), existingStoryboards));

            // If the size of the active storyboard collection contains storyboards outside
            // the pending storyboard it means a state change didn't clean up properly, we've broken an invariant.
            ASSERT((pendingStoryboard && m_dataSource->GetActiveStoryboards(data->GetGroupIndex()).size() == 1) ||
                m_dataSource->GetActiveStoryboards(data->GetGroupIndex()).size() == 0);

            m_dataSource->SetPendingStoryboard(data->GetGroupIndex(), nullptr);
            m_dataSource->ClearPendingPropertySetters(data->GetGroupIndex());
            IFC_RETURN(m_dataSource->CompleteTransitionToState(data->GetGroupIndex()));

            // Finally remove this transition from the group collection. It's storyboard
            // should have been stopped above, and there's no longer any purpose for it.
            m_dataSource->RemoveActiveTransition(data->GetGroupIndex(), data->GetVisualTransition());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerActuator::SynchronouslyResetToNullState(_In_ int groupIndex)
{
    const auto& activeStoryboards = m_dataSource->GetActiveStoryboards(groupIndex);
    std::vector<CStoryboard*> toRemove;
    toRemove.reserve(activeStoryboards.size());
    for (auto& storyboard : activeStoryboards)
    {
        toRemove.push_back(storyboard.first);
    }

    // Keep in mind that if we had active transition storyboards the VSM was in the process of
    // transitioning to a state it never fully reached. This is really important. When the developer
    // either directly or indirectly interrupts a state transition we never fire the StateChanged event.
    // The VSM never fully entered the target state. We avoid any code that would transition us
    // inappropriately to that state after this moment in execution by preemptively processing
    // the transition data on the pending storyboards. This heads off any already-fired asynchronous
    // completed events and prevents them from mucking up our current state.
    IFC_RETURN(StopAndRemoveStoryboards(groupIndex, toRemove));

    // Remove the currently applied property setters
    IFC_RETURN(StopAndRemovePropertySetters(groupIndex, m_dataSource->GetActivePropertySetters(groupIndex)));

    IFC_RETURN(m_dataSource->TransitionToNullState(groupIndex));

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerActuator::RefreshAllAppliedPropertySetters()
{
    for (int groupIndex = 0; groupIndex < m_dataSource->GetGroupCount(); groupIndex++)
    {
        auto& groupContext = m_dataSource->GetGroupContext(groupIndex);
        if (groupContext.CurrentState() == VisualStateGroupContext::State::StateApplied)
        {
            IFC_RETURN(ReevaluateAppliedPropertySetters(groupIndex, groupContext.CurrentVisualStateIndex()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerActuator::ReevaluateAppliedPropertySetters(_In_ int groupIndex, _In_ int stateIndex)
{
    VisualStateSetterHelper::ResolvedVisualStateSetterCollection propertySetters;
    IFC_RETURN(m_dataSource->TryGetOrCreatePropertySettersForVisualState(stateIndex, &propertySetters));
    IFC_RETURN(m_dataSource->SetAndApplyActivePropertySetters(groupIndex, propertySetters, true));

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerActuator::CalculateAndMarkDynamicTimelineSteadyState(
    _In_opt_ CStoryboard* destinationStoryboard,
    _In_opt_ CStoryboard* transitionStoryboard,
    _In_ bool destinationStoryboardResponsibleForDynamicTransitions)
{
    if (destinationStoryboard)
    {
        // When there's not a transition we want to have the final storyboard generate the dynamic timelines
        // associated with Theme transitions if animations are enabled or the Storyboard is marked essential.
        bool generateDestinationDynamicAnimations =
            destinationStoryboardResponsibleForDynamicTransitions &&
            (m_dataSource->IsAnimationEnabled() || destinationStoryboard->m_fIsEssential);

        Jupiter::Animation::PropagateDynamicTimelineGenerationOptionToChildren(destinationStoryboard,
            generateDestinationDynamicAnimations ?
                DynamicTimelineGenerationMode::Transition : DynamicTimelineGenerationMode::SteadyState);
        IFC_RETURN(Jupiter::Animation::TryGenerateDynamicChildrenStoryboardsForChildren(destinationStoryboard, nullptr));
    }

    if (transitionStoryboard)
    {
        Jupiter::Animation::PropagateDynamicTimelineGenerationOptionToChildren(transitionStoryboard,
            DynamicTimelineGenerationMode::Transition);
        IFC_RETURN(Jupiter::Animation::TryGenerateDynamicChildrenStoryboardsForChildren(transitionStoryboard, nullptr));
    }

    return S_OK;
}

std::vector<CStoryboard*> VisualStateManagerActuator::GetExistingStoryboards(
    _In_ int groupIndex,
    _In_opt_ CStoryboard* transitionStoryboard,
    _In_opt_ CStoryboard* destinationStoryboard,
    _In_opt_ CStoryboard* dynamicStoryboard)
{
    return GetExistingStoryboards(groupIndex,
        std::array < CStoryboard*, 3 > {{transitionStoryboard, destinationStoryboard, dynamicStoryboard}});
}

_Check_return_ HRESULT VisualStateManagerActuator::SnapToEnd(_In_ int groupIndex, _In_ bool forceSynchronous)
{
    auto storyboards = m_dataSource->GetActiveStoryboards(groupIndex);
    for (auto& storyboard : storyboards)
    {
        if (forceSynchronous)
        {
            IFC_RETURN((storyboard.first)->SkipToFill());

            // For synchronous storyboards if we're leaving the tree we have
            // this interesting edge case where the Storyboard will have finished with
            // VisualTransition data set. We just skipped it to the end, but it's
            // likely it could still have a VisualTransitionCompleted instance
            // to be handled. We'll synchronously handle that data here as we do
            // when calling StopAndRemoveStoryboards. After we've finished
            // leaving the visual tree there's no promise that the VSGC will
            // still be available. This has the side-effect of ensuring that the Completed
            // event will fire consistently when the instance is removed from the tree.
            if (storyboard.first->m_pVisualTransitionCompletedData)
            {
                // There's an even more interesting edge case here as well. We can have a scenario that
                // looks like this occur:
                // - We leave the livee tree, since VisualStateGroupCollection's m_vsmActivitySinceLastLeave flag was
                //   set from previous VSM activity we skip all the animations to end and that involves doing this
                //   synchronous process of their completed data.
                // - A control calls GoToState from the Unloaded event, resetting the m_vsmActivitySinceLastLeave flag
                //   back to true
                // - The XAML fragment that used to be in the live tree is destructed. We execute this same
                //   SnapToEnd call when the non-live Leave occurs during tree teardown and the
                //   VisualStateGroupCollection is removed from the owning element.
                // - The event handlers for the Storyboard finally fire, removing this structure
                //   and releasing the storyboards
                //
                // To protect this scenario we'll harden this method against reentrancy and ONLY execute this
                // logic if the transition hasn't been processed. This is ONLY for handling duplicate tree leaves,
                // in ALL other cases this method should not be called more than once.
                if (!storyboard.first->m_pVisualTransitionCompletedData->GetHasBeenProcessed())
                {
                    TryProcessCompletedDataIfExists(storyboard.first, groupIndex);
                }
            }
            else if (storyboard.second != VisualStateGroupContext::StoryboardType::State)
            {
                // Yet another fun edge case is when we have a VisualTransition with a dynamic timeline.
                // When the first storyboard completes we leave it in the ActiveStoryboard collection such
                // that we can perform the smooth animation handoff when the second storyboard completes. If
                // we skip the storyboards to the end before this second storyboard completes we end up
                // needing to also remove the already-completed Storyboard from the active storyboards
                // as well.
                m_dataSource->RemoveActiveStoryboard(groupIndex, storyboard.first);
            }
        }
        else
        {
            IFC_RETURN((storyboard.first)->Complete());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerActuator::StopAndRemoveStoryboards(_In_ int groupIndex,
    _In_ std::vector<CStoryboard*>& storyboards)
{
    // For every storyboard, stop it, remove any event listeners
    // added to it, and remove it from the collection.
    for (auto& storyboard : storyboards)
    {
        // We take a strong ref on the storyboard here. Removing the storyboard
        // from either the active storyboards or the transition can cause it to be
        // completely destructed. We want to keep ownership of it for the duration
        // of this call.
        xref_ptr<CStoryboard> strongRef(storyboard);

        IFC_RETURN(StopAndRemoveStoryboard(groupIndex, strongRef));
    }

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerActuator::StopAndRemoveStoryboard(_In_ int groupIndex,
    _In_ const xref_ptr<CStoryboard>& storyboard)
{
    VSMLOG(L"[VSMA]: StopAndRemove - Existing storyboard: HasData: %d, IsCompleted: %d",
        !!storyboard->m_pVisualTransitionCompletedData,
        storyboard->IsCompletedEventFired());

    if (storyboard->m_pVisualTransitionCompletedData
        && !storyboard->IsCompletedEventFired())
    {
        // This makes sure it won't fire at any time in the future (it could still have
        // already fired asynchronously and be pending, at which point this will happen in
        // the completed event handler instead)
        //
        // This is an optimization more than anything. All invariants would be perserved
        // if this code was removed.
        IFC_RETURN(storyboard->RemoveEventListener(
            EventHandle(KnownEventIndex::Timeline_Completed),
            &storyboard->m_pVisualTransitionCompletedData->m_EventListenerToken));
    }

    IFC_RETURN(storyboard->StopPrivate());
    if (!TryProcessCompletedDataIfExists(storyboard, groupIndex))
    {
        m_dataSource->RemoveActiveStoryboard(groupIndex, storyboard);
    }

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerActuator::StopAndRemovePropertySetters(
    _In_ int groupIndex,
    _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& setterModifiedValues)
{
    IFC_RETURN(m_dataSource->UnapplyPropertySetters(groupIndex, setterModifiedValues));

    return S_OK;
}

bool VisualStateManagerActuator::TryProcessCompletedDataIfExists(_In_ CStoryboard* storyboard, _In_ int groupIndex)
{
    // If the storyboard has a VisualTransitionCompletedData value we need to
    // do the processing to make sure the VisualTransition is removed and the pending
    // counter is decremented.
    if (storyboard->m_pVisualTransitionCompletedData)
    {
        VSMLOG(L"[VSMA]: StopAndRemove - Preemptively processing completed data.");
        // If the storyboard's completion data had already been processed then it must
        // have been processed by the OnCompleted handler which should have cleared the
        // pointer before entering this method.
        ASSERT(!storyboard->m_pVisualTransitionCompletedData->GetHasBeenProcessed());
        storyboard->m_pVisualTransitionCompletedData->SetHasBeenProcessed(true);

        int pendingCallbacks = m_dataSource->DecrementPendingStoryboardCompletionCallbackCounter(
            storyboard->m_pVisualTransitionCompletedData->GetGroupIndex());
        ASSERT(pendingCallbacks >= 0);

        m_dataSource->RemoveActiveStoryboard(groupIndex, storyboard);

        if (pendingCallbacks == 0)
        {
            VSMLOG(L"[VSMA]: StopAndRemove - Removing the visual transition.");
            // We don't actually complete the VisualTransition on the Group's context
            // here. If we were interrupted by another GoToState call then we really
            // didn't complete the transition and don't mark it as such.

            m_dataSource->SetPendingStoryboard(storyboard->m_pVisualTransitionCompletedData->GetGroupIndex(), nullptr);
            m_dataSource->ClearPendingPropertySetters(storyboard->m_pVisualTransitionCompletedData->GetGroupIndex());
            auto transition = storyboard->m_pVisualTransitionCompletedData->GetVisualTransition();
            ASSERT(transition);

            // At this point it's quite possible the Storyboard is destructed as the VisualTransition
            // was likely the last owner. Do not perform calls on it after this point.
            m_dataSource->RemoveActiveTransition(storyboard->m_pVisualTransitionCompletedData->GetGroupIndex(), transition);
        }
        return true;
    }
    return false;
}

_Check_return_ HRESULT VisualStateManagerActuator::StartAndAddStoryboard(_In_ int groupIndex, _In_ const xref_ptr<CStoryboard>& storyboard, VisualStateGroupContext::StoryboardType type)
{
    m_dataSource->AddActiveStoryboard(groupIndex, storyboard, VisualStateGroupContext::StoryboardType::State);
    IFC_RETURN(AttemptStartStoryboard(storyboard.get(), type));

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerActuator::AttemptStartStoryboard(_In_ CStoryboard* storyboard, VisualStateGroupContext::StoryboardType type)
{
    IFC_RETURN(storyboard->BeginPrivate(TRUE));

    // We do this next bit because of course VSM is a special snowflake and unlike when
    // Storyboards are typically started, VSM applies their first-frame values synchronously.
    IFC_RETURN(StoryboardHelpers::ZeroSeekAlignedToLastTick(storyboard));

    // When animations aren't enabled we skip all non-essential transition storyboards to their
    // end frame. Note that we are careful to still apply the first frame above as well- this ensures
    // that both the beginning and end property values are still set, giving the targets an
    // oppertunity to properly react to the first and last frame of the animation.
    if (!m_dataSource->IsAnimationEnabled() &&
        (type == VisualStateGroupContext::StoryboardType::Transition ||
            type == VisualStateGroupContext::StoryboardType::Dynamic) && !storyboard->m_fIsEssential)
    {
        IFC_RETURN(StoryboardHelpers::EndSeekAlignedToLastTick(storyboard));
    }

    return S_OK;
}

// Creates StateTriggerVariantMaps, creates qualifiers and populates the VariantMap, and performs initial VisualState selection.
_Check_return_ HRESULT VisualStateManagerActuator::InitializeStateTriggers(_In_ CControl* pControl)
{
    int vsIndex = 0;
    VisualTree *visualTree = VisualTree::GetForElementNoRef(pControl);

    // If we're not in the live tree, we'll use the main visual tree as a fallback.
    // If we enter the live tree, we'll re-evaluate our state triggers.
    if (!visualTree)
    {
        // If we are not in the live tree yet, we can use the one from the CoreWindow to
        // start with which will be the correct one if this element does not go under an
        // island (non island scenarios). We will re-evaluate when the control enters the
        // tree anyway.

        if (const auto contentRootCoordinator = pControl->GetContext()->GetContentRootCoordinator())
        {
            if (CContentRoot* contentRoot = contentRootCoordinator->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
            {
                visualTree = contentRoot->GetVisualTreeNoRef();
            }
        }
    }

    // If we still don't have a visual tree, we'll just bail.
    if (!visualTree)
    {
        return S_OK;
    }

    std::shared_ptr<QualifierContext> qualifierContext = visualTree->GetQualifierContext();

    // Define callback called each time a qualifier is created. Adds the new qualifier to the group's VariantMap.
    auto onNewQualifier = [this, &vsIndex, &qualifierContext](VisualStateToken token, std::shared_ptr<IQualifier> qualifier, xref_ptr<CDependencyObject> trigger)
    {
        std::shared_ptr<StateTriggerVariantMap> variantMap;
        IFC_RETURN(m_dataSource->TryGetOrCreateStateTriggerVariantMap(vsIndex, &variantMap));
        variantMap->SetQualifierContext(qualifierContext);

        // Turn off StateTriggerVariantMap evaluations while StateTriggers collection is modified
        variantMap->DisableEvaluations();
        IFC_RETURN(variantMap->Add(token, qualifier, trigger));

        if(trigger)
        {
            CStateTriggerBase* stateTriggerBase = static_cast<CStateTriggerBase*>(trigger.get());
            stateTriggerBase->m_owningVariantMaps.push_back(variantMap);
        }

        return S_OK;
    };

    // Create all qualifiers and add them to the VariantMap
    auto vsCount = m_dataSource->GetVisualStateCount();
    for(vsIndex = 0; vsIndex < vsCount; ++vsIndex)
    {
        IFC_RETURN(m_dataSource->GetQualifiersFromStateTriggers(vsIndex, onNewQualifier));
    }

    // Setup VariantMapChanged callback and perform initial evaluation
    auto variantMaps = m_dataSource->GetStateTriggerVariantMaps();

    for(auto& vm : variantMaps)
    {
        vm->SetQualifierContext(qualifierContext);
        CVisualStateManager2::AssignStateTriggerChangedCallback(pControl, *vm);
        vm->EnableEvaluations();
        IFC_RETURN(vm->Evaluate());
    }

    return S_OK;
}

// Determines the set of previously active property setters that need to be explicitly unapplied
// (i.e. reverted to base value) given the new set of property setters and storyboards.
// If a previously active affected object/property pair is going to be modified by the property setters
// or storyboards that will be applied in the near future, then there is no need to explicitly unapply them
// since the new setters/storyboards will implicitly clear the value applied by the old property setter,
// so remove them from our list to prevent clobbering.
// Modifies 'previouslyActivePropertySetters' in-place.
_Check_return_ HRESULT VisualStateManagerActuator::SelectExistingPropertySettersToRemove(
    _Inout_ VisualStateSetterHelper::ResolvedVisualStateSetterCollection& previouslyActivePropertySetters,
    _In_ VisualStateSetterHelper::ResolvedVisualStateSetterCollection& destinationPropertySetters,
    _In_opt_ CStoryboard* transitionStoryboard,
    _In_opt_ CStoryboard* destinationStoryboard,
    _In_opt_ CStoryboard* dynamicStoryboard)
{
    containers::vector_set<std::tuple<xref::weakref_ptr<CDependencyObject>, const CDependencyProperty*>> collectionToMatchSet;
    {
        std::vector<CStoryboard*> storyboardVector;
        {
            storyboardVector.reserve(3);
            if (transitionStoryboard) { storyboardVector.push_back(transitionStoryboard); }
            if (destinationStoryboard) { storyboardVector.push_back(destinationStoryboard); }
            if (dynamicStoryboard) { storyboardVector.push_back(dynamicStoryboard); }
        }
        TimelineLookupList activeStoryboards;
        IFC_RETURN(activeStoryboards.Initialize(storyboardVector));

        // Do a little work now to insert the current storyboards into a vector_set so that we can save work
        // later when searching for matches
        collectionToMatchSet.reserve(destinationPropertySetters.size() + std::distance(activeStoryboards.begin(), activeStoryboards.end()));
        {
            for (auto& setter : destinationPropertySetters)
            {
                collectionToMatchSet.emplace(setter.get_TargetObject(), setter.get_TargetProperty());
            }

            for (auto& timeline : activeStoryboards)
            {
                collectionToMatchSet.emplace(timeline.m_doWeakRef, DirectUI::MetadataAPI::GetDependencyPropertyByIndex(timeline.m_propertyIndex));
            }
        }
        collectionToMatchSet.shrink_to_fit();

        // Move all elements in 'previouslyActivePropertySetters' that have the same target object/property as any element in
        // 'collectionToMatchSet' (which contains the storyboards/setters belonging to the new state) to the end of the container
        // and then erase them, leaving only the elements which *don't* match something that is about to be applied in the new state
        previouslyActivePropertySetters.erase(
            std::remove_if(
                previouslyActivePropertySetters.begin(),
                previouslyActivePropertySetters.end(),
                    [collectionToMatchSet](VisualStateSetterHelper::ResolvedVisualStateSetter previouslyActiveSetter)
                    {
                        auto result = std::find_if(
                            collectionToMatchSet.begin(),
                            collectionToMatchSet.end(),
                            [previouslyActiveSetter](std::tuple<xref::weakref_ptr<CDependencyObject>, const CDependencyProperty*> element)
                            {
                                return std::get<0>(element) == previouslyActiveSetter.get_TargetObject()
                                    && std::get<1>(element) == previouslyActiveSetter.get_TargetProperty();
                            });
                        return result != collectionToMatchSet.end();
                }),
            previouslyActivePropertySetters.end());
    }
    return S_OK;
}
