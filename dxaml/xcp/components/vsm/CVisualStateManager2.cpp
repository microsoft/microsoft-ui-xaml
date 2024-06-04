// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CVisualStateManager2.h>

#include "VisualStateManagerActuator.h"
#include <VisualStateGroupContext.h>

#include <Ccontrol.h>
#include <DOPointerCast.h>
#include <VisualTransition.h>
#include <storyboard.h>
#include <VisualTransitionCompletedData.h>

// For fault-in logic
#include <CustomWriterRuntimeObjectCreator.h>
#include <VisualStateGroupCollection.h>
#include <VisualStateCollection.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>
#include <VisualTransitionCollection.h>
#include <VisualStateGroup.h>
#include <VisualState.h>
#include <StateTriggerCollection.h>
#include <AdaptiveTrigger.h>
#include <StateTriggerBase.h>
#include <QualifierFactory.h>
#include <corep.h>
#include <VisualStateManagerDataSource.h>
#include <FxCallbacks.h>

#include "MUX-ETWEvents.h"

//#define VSMLOG(...) LOG(__VA_ARGS__)
#define VSMLOG(...)

_Check_return_ HRESULT CVisualStateManager2::GoToStateOptimized(
    _In_ CControl *pControl,
    _In_z_ const WCHAR* pStateName,
    _In_ bool useTransitions,
    _Out_ bool* succeeded)
{
    return CVisualStateManager2::GoToStateOptimizedImpl(
        pControl,
        pStateName,
        VisualStateToken(),
        useTransitions,
        succeeded);
}

_Check_return_ HRESULT CVisualStateManager2::GoToStateOptimized(
    _In_ CControl *pControl,
    _In_ VisualStateToken token,
    _In_ bool useTransitions,
    _Out_ bool* succeeded)
{
    return CVisualStateManager2::GoToStateOptimizedImpl(
        pControl,
        nullptr,
        token,
        useTransitions,
        succeeded);
}

_Check_return_ HRESULT CVisualStateManager2::GoToStateOptimizedImpl(
    _In_ CControl *pControl,
    _In_z_ const WCHAR* pStateName,
    _In_ VisualStateToken token,
    _In_ bool useTransitions,
    _Out_ bool* succeeded)
{
    VSMLOG(L"[VSM]: GoToStateOptimized to state %s", pStateName);
    *succeeded = false;

    // Try to obtain the CVisualStateGroupCollection object from the passed in Control.
    auto groupCollection = GetGroupCollectionFromControl(pControl);
    if (groupCollection)
    {
        groupCollection->MarkVsmActivity();
        // Create the data source and validate that the visual state actually exists.
        auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());
        int stateIndex = -1;
        int groupIndex = -1;

        bool foundVisualState = false;
        if(pStateName)
        {
            foundVisualState = dataSource->TryGetVisualState(pStateName, &stateIndex, &groupIndex);
        }
        else
        {
            foundVisualState = dataSource->TryGetVisualState(token, &stateIndex, &groupIndex);
        }

        if(foundVisualState)
        {
            ASSERT(stateIndex != -1 && groupIndex != -1);

            VisualStateManagerActuator actuator(
                static_cast<CFrameworkElement*>(pControl->GetFirstChildNoAddRef()), dataSource.get());
            auto& groupContext = dataSource->GetGroupContext(groupIndex);

            // If we're already in the correct state then we don't actually need to do any work.
            // If we're in the middle of transitioning to the correct state and this GoToState call
            // was requested with no transitions we complete the storyboards and snap the state to
            // the end.
            if (stateIndex == groupContext.CurrentVisualStateIndex())
            {
                // This is really sublte- we don't check to see if we're still in the transitioning
                // state before completing. This allows GoToState(useAnimations:false) to reapply
                // Binding expressions because animations will always recompute their final value
                // when we call Complete on them.
                if (!useTransitions)
                {
                    VSMLOG(L"[VSM]: Snapping to end.");
                    IFC_RETURN(actuator.SnapToEnd(groupIndex, false));
                    IFC_RETURN(actuator.ReevaluateAppliedPropertySetters(groupIndex, stateIndex));
                }
            }
            else
            {
                IFC_RETURN(actuator.ChangeVisualState(groupIndex, groupContext.CurrentVisualStateIndex(), stateIndex, useTransitions));
            }
            *succeeded = true;
        }
    }

    return S_OK;
}

bool CVisualStateManager2::DoesVisualStateExist(
    _In_ CControl *pControl, _In_z_ const WCHAR* pStateName)
{
    // Try to obtain the CVisualStateGroupCollection object from the passed in Control.
    auto groupCollection = GetGroupCollectionFromControl(pControl);
    if (!groupCollection) return false;

    // Create the data source and validate that the visual state actually exists.
    auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());
    int stateIndex = -1;
    int groupIndex = -1;
    return dataSource->TryGetVisualState(pStateName, &stateIndex, &groupIndex);
}

_Check_return_ HRESULT CVisualStateManager2::FaultInChildren(_In_ CVisualStateGroupCollection* groupCollection)
{
    VSMLOG(L"[VSM]: FaultInChildren");
    TraceFaultInBehaviorBegin(L"VisualStateManager");
    auto scopeGuard = wil::scope_exit([&]
    {
        TraceFaultInBehaviorEnd();
    });

    ASSERT(!groupCollection->GetFaultedInChildren());
    groupCollection->MarkFaultedInChildren();
    auto customRuntimeData = groupCollection->GetCustomRuntimeData();

    // Determine if FaultInChildren was called before triggers were initialized for this control.
    bool isImmediateFaultIn = groupCollection->GetStateTriggerVariantMaps().empty();

    CustomWriterRuntimeObjectCreator creator(
        NameScopeRegistrationMode::RegisterEntries,
        groupCollection->GetCustomRuntimeContext());

    std::vector<StreamOffsetToken> indexRangesToSkip;
    if(!isImmediateFaultIn)
    {
        indexRangesToSkip = customRuntimeData->GetStateTriggerCollectionTokens();
    }

    IFC_RETURN(creator.ApplyStreamToExistingInstance(
        groupCollection->GetCustomRuntimeData()->GetEntireGroupCollectionToken(),
        groupCollection,
        indexRangesToSkip));

    ASSERT(groupCollection->size() == 0 || groupCollection->GetGroupContext().size() == 0 ||
        groupCollection->size() == groupCollection->GetGroupContext().size());

    int currentGroupIndex = 0;

    for (auto& context : groupCollection->GetGroupContext())
    {
        auto doGroup = static_cast<CVisualStateGroup*>((*groupCollection)[currentGroupIndex]);

        int visualStateIndex = context.CurrentVisualStateIndex() == -1 ? -1 :
            groupCollection->GetCustomRuntimeData()->GetGroupVisualStateIndex(context.CurrentVisualStateIndex());

        ASSERT(
                (!doGroup->m_pVisualStates && context.CurrentVisualStateIndex() == -1)
            ||
                (
                    context.CurrentVisualStateIndex() == -1
                ||
                    doGroup->m_pVisualStates->size() > static_cast<size_t>(visualStateIndex)
                )
            );

        // If the storyboard is 'active'  but part of a transition it will be present in the
        // context's ActiveStoryboards collection, but not in ActiveStoryboards. We check for
        // context's ActiveStoryboards collection, but not in the actual CDOCollection. We check for
        // a faulted-in active storyboards collection to avoid the edge case where we're in
        // a transition but there wasn't an already-active visual state.
        if (groupCollection->m_pActiveStoryboards)
        {
            // will all belong to the current VisualState or current VisualTransition.
            for (auto& storyboard : context.ActiveStoryboards())
            {
                // Storyboards that are dynamically generated will always be part of the storyboard
                // collection on VSGC. Storyboards that are Transitions already belong to a transition
                // which is transferred below.
                if (storyboard.second == VisualStateGroupContext::StoryboardType::State)
                {
                    VSMLOG(L"[VSM]: Transfering storyboard ownership...");
                    ASSERT(context.CurrentVisualStateIndex() != -1);

                    if ((storyboard.first)->HasManagedPeer())
                    {
                        IFC_RETURN((storyboard.first)->PegManagedPeer());
                    }

                    // Because ActiveStoryboards doesn't hold a ref on the current storyboard
                    // it's possible that we could be removing the last ref on it when we remove
                    // it from the ActiveStoryboards DOCollection. We hold a temporary ref on it
                    // to protect.
                    auto storyboardReference = xref_ptr<CStoryboard>((storyboard.first));

                    groupCollection->m_pActiveStoryboards->remove((storyboard.first));
                    CValue storyboardAsCValue;
                    storyboardAsCValue.WrapObjectNoRef((storyboard.first));

                    // Register any needed TemplateNameScope entries so this now-public
                    // entry can respond to x:Name FindName / GetTemplateChild calls.
                    IFC_RETURN(creator.RegisterTemplateNameScopeEntriesIfNeeded(storyboard.first));

                    IFC_RETURN((*doGroup->m_pVisualStates)[visualStateIndex]->SetValueByIndex(
                        KnownPropertyIndex::VisualState_Storyboard, storyboardAsCValue));

                    if ((storyboard.first)->HasManagedPeer())
                    {
                        (storyboard.first)->UnpegManagedPeer();
                    }
                }
            }
        }

        // For each group there will only be one active Transition at any given time (this
        // should be made into not a vector soon). Take this transition and swap it with the
        // transition in our new object graph.
        ASSERT(context.ActiveTransitions().size() == 0 || groupCollection->m_pActiveTransitions);
        for (auto& transition : context.ActiveTransitions())
        {
            VSMLOG(L"[VSM]: Transfering transition ownership...");
            ASSERT(context.CurrentState() == VisualStateGroupContext::State::Transitioning);

            if (transition->HasManagedPeer())
            {
                IFC_RETURN(transition->PegManagedPeer());
            }
            // Because ActiveTransitions doesn't hold a ref on the current transition
            // it's possible that we could be removing the last ref on it when we remove
            // it from the DOCollection. We hold a temporary ref on it
            // to protect.
            auto transitionReference = xref_ptr<CVisualTransition>(transition);

            // Register any needed TemplateNameScope entries so this now-public
            // entry can respond to x:Name FindName / GetTemplateChild calls.
            IFC_RETURN(creator.RegisterTemplateNameScopeEntriesIfNeeded(transition));

            groupCollection->m_pActiveTransitions->remove(transition);

            auto realTransitionIter = std::find_if(doGroup->m_pTransitions->begin(), doGroup->m_pTransitions->end(),
                [transition](CDependencyObject* obj) {
                auto candidateTransition = static_cast<CVisualTransition*>(obj);
                return transition->m_strFrom.Equals(candidateTransition->m_strFrom)
                    && transition->m_strTo.Equals(candidateTransition->m_strTo);
            });

            ASSERT(realTransitionIter != doGroup->m_pTransitions->end());
            size_t offset = std::distance(doGroup->m_pTransitions->begin(), realTransitionIter);

            doGroup->m_pTransitions->remove(*realTransitionIter);
            doGroup->m_pTransitions->insert(doGroup->m_pTransitions->begin() + offset, xref_ptr<CDependencyObject>(transition));

            if (transition->HasManagedPeer())
            {
                transition->UnpegManagedPeer();
            }
        }

        // Finally update the current state property to be accurate.
        if (context.CurrentVisualStateIndex() != -1)
        {
            IFC_RETURN(doGroup->SetCurrentVisualState(
                static_cast<CVisualState*>((*doGroup->m_pVisualStates)[visualStateIndex])));
        }

        currentGroupIndex++;
    }

    // Move VisualStateTokens to VisualStates
    int vsIndex = 0;
    for (auto& doStateGroup : *groupCollection)
    {
        auto stateGroup = static_cast<CVisualStateGroup*>(doStateGroup);
        if (!stateGroup->m_pVisualStates) continue;

        for (auto& visualState : *(stateGroup->m_pVisualStates))
        {
            static_cast<CVisualState*>(visualState)->SetOptimizedIndex(vsIndex++);
        }
    }

    if(!isImmediateFaultIn)
    {
        IFC_RETURN(CVisualStateManager2::FaultInStateTriggers(groupCollection, creator));
    }


    return S_OK;
}

xref_ptr<CVisualStateGroupCollection> CVisualStateManager2::GetGroupCollectionFromControl(_In_ CControl* pControl)
{
    xref_ptr<CUIElement> implementationRoot = pControl->GetImplementationRoot();
    if (implementationRoot)
    {
        return static_cast<const CFrameworkElement*>(implementationRoot.get())->GetVisualStateGroupsNoCreate();
    }

    return nullptr;
}

xref_ptr<CVisualStateGroupCollection> CVisualStateManager2::GetGroupCollectionFromVisualState(_In_ const CVisualState* state)
{
    CVisualStateGroupCollection* visualStateGroupCollection = nullptr;

    // Walk up the tree until we find the VisualStateGroupCollection for this VisualState
    CDependencyObject* candidate = state->GetParentInternal(false);
    while (candidate && !visualStateGroupCollection)
    {
        visualStateGroupCollection = do_pointer_cast<CVisualStateGroupCollection>(candidate);
        candidate = candidate->GetParentInternal(false);
    }

    return xref_ptr<CVisualStateGroupCollection>(visualStateGroupCollection);
}

HRESULT __stdcall CVisualStateManager2::OnStoryboardCompleted(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs)
{
    VSMLOG(L"[VSM]: OnStoryboardCompleted");
    auto storyboard = do_pointer_cast<CStoryboard>(sender);

    auto data = std::move(storyboard->m_pVisualTransitionCompletedData);

    // When we're a faulted in VSM we're reusing the same Storyboards for subsequent GoToState calls.
    // Storyboards generally have an invarient that for every Start->Completion there will be one and
    // only one Completed event. When we leaving the visual tree in the middle of a VSM transition we'll
    // synchronously process the completed data to put the VSM in the correct state, but leave
    // m_pVisualTransitionCompletedData on the Storyboard such that when this asynchronous event handler
    // is fired we simply examine it, notice the data has already been processed, and delete it.
    //
    // When we have a set of operations that looks like this:
    // 1) GoToState State1 with Transitions
    // 2) LeaveImpl, SkipToFill
    // 3) Enter tree again
    // 4) Call GoToState State2
    // 5) Call GoToState State1
    //
    // We have two pending asynchronous OnStoryboardCompleted events queued on the dispatcher, the first
    // one will come along and prematurely complete the transition. This will create a minor visual
    // glitch as the final VSM state will be set prematurely before the transition animation has finished
    // playing. The final OnCompleted event will be cancelled because we stop and remove this storyboard
    // to complete the transition. An even more unlikely scenario is that it too has already been queued
    // at this point, which results in the same outcome. It's silently ignored as m_pVTCD is null.
    //
    // Alternatively we could add a counter to the data structure and catch this, but given the complexity
    // of Storyboard's state machine and the edge-case nature of this state it's likely not worth while.
    if (!data) return S_OK;

    IFC_RETURN(storyboard->RemoveEventListener(EventHandle(KnownEventIndex::Timeline_Completed),
        &data->m_EventListenerToken));

    // Option 1: We are a transition storyboard and we have a VisualTransition parent, which
    // has a parent chain that looks like this:
    // Standard: SB-> VT -> VTCollection -> VSG -> VSGCollection
    // Optimized: SB -> VT -> VTCollection -> VSGCollection
    // Option 2: We are a normal storyboard and we have a a parent chain that looks like this:
    // Standard: SB -> VS -> VSCollection -> VSG -> VSGCollection
    // Optimized: SB -> SBCollection -> VSGCollection
    CVisualStateGroupCollection* visualStateGroupCollection = nullptr;

    // If the completed event has been prematurely handled it's possible that the storyboard
    // has been removed from its owning collection and we'll be unable to locate the group
    // collection. This is expected behavior.
    CDependencyObject* candidate = storyboard->GetParentInternal(false);
    while (candidate && !visualStateGroupCollection)
    {
        visualStateGroupCollection = do_pointer_cast<CVisualStateGroupCollection>(candidate);
        candidate = candidate->GetParentInternal(false);
    }

    // In all cases where we might be unable to locate a group collection- i.e. removal from the tree
    // etc- we should have already been processed synchronously. This happens in VSGC's Leave.
    ASSERT(visualStateGroupCollection || data->GetHasBeenProcessed());
    if (visualStateGroupCollection)
    {
        auto dataSource = CreateVisualStateManagerDataSource(visualStateGroupCollection);
        VisualStateManagerActuator actuator(
            do_pointer_cast<CFrameworkElement>(visualStateGroupCollection->GetParent()), dataSource.get());
        IFC_RETURN(actuator.FinishVisualTransition(storyboard, data.get()));
    }

    return S_OK;
}

HRESULT CVisualStateManager2::OnVisualStateGroupCollectionLeave(_In_ CVisualStateGroupCollection* groupCollection)
{
    VSMLOG(L"[VSM]: OnVisualStateGroupCollectionLeave");
    // When we leave the visual tree all bets are off. We skip animations to their end value and
    // end any ongoing visual transitions and any ongoing destination animations.
    auto dataSource = CreateVisualStateManagerDataSource(groupCollection);
    VisualStateManagerActuator actuator(
        do_pointer_cast<CFrameworkElement>(groupCollection->GetParent()), dataSource.get());
    for (int i = 0; i < dataSource->GetGroupCount(); i++)
    {
        IFC_RETURN(actuator.SnapToEnd(i, true));
    }

    return S_OK;
}

HRESULT CVisualStateManager2::OnVisualStateGroupCollectionNotifyThemeChanged(_In_ CVisualStateGroupCollection* groupCollection)
{
    VSMLOG(L"[VSM]: OnVisualStateGroupCollectionNotifyThemeChanged");
    // MSFT:2520791
    // When the theme changes, we need to reapply any active VSM Setters in order to handle the case where the original base value of
    // a modified property was a ThemeResource, in which case the theme change will trigger a change in the base value and thereby
    // clobber the animated value applied by the VSM Setter (which is undesired behavior)
    auto dataSource = CreateVisualStateManagerDataSource(groupCollection);
    VisualStateManagerActuator actuator(
        do_pointer_cast<CFrameworkElement>(groupCollection->GetParent()), dataSource.get());
    IFC_RETURN(actuator.RefreshAllAppliedPropertySetters());

    return S_OK;
}

// Initializes StateTriggers for both optimized (XBFv2) and standard/legacy load paths
// Uses VisualStateGroups or CustomRuntimeData on control's VisualStateGroupCollection to
// read VisualState and StateTrigger values
HRESULT CVisualStateManager2::InitializeStateTriggers(_In_ CDependencyObject* pDO, const bool forceUpdate)
{
    // DependencyObject must be a control with a VisualStateGroupCollection and should
    // not have already had its StateTriggers initialized, unless being forced by diagnostics
    CControl* pControl = do_pointer_cast<CControl>(pDO);
    if(!pControl)
    {
        return S_OK;
    }

    xref_ptr<CVisualStateGroupCollection> groupCollection = GetGroupCollectionFromControl(pControl);
    if (!groupCollection)
    {
        return S_OK;
    }

    auto& groupCollectionMap = groupCollection->GetStateTriggerVariantMaps();
    if (forceUpdate)
    {
        // Forcing an update, clear the map
        groupCollectionMap.clear();

        // If we're forcing an update, we should also clear the state trigger variant maps in each group.
        for (auto group : *groupCollection)
        {
            static_cast<CVisualStateGroup*>(group)->m_pStateTriggerVariantMap = nullptr;
        }

        for (auto& groupContext : groupCollection->GetGroupContext())
        {
            groupContext.GetStateTriggerVariantMap() = nullptr;
        }
    }

    if (!groupCollectionMap.empty())
    {
        // If the map isn't clear, then we are already initialized or we weren't forced by diagnostics to update.
        // Adding triggers isn't something usually supported at runtime.
        return S_OK;
    }

    auto dataSource = CreateVisualStateManagerDataSource(groupCollection);
    auto dataSourceMap = dataSource->GetStateTriggerVariantMaps();

    if (!dataSourceMap.empty())
    {
        if (forceUpdate)
        {
            // Forcing an update, clear the map
            dataSourceMap.clear();
        }

        if (!dataSourceMap.empty())
        {
            // If the map isn't clear, then we are already initialized or we weren't forced by diagnostics to update.
            // Adding triggers isn't something usually supported at runtime.
            return S_OK;
        }
    }

    VisualStateManagerActuator actuator(
                static_cast<CFrameworkElement*>(pControl->GetFirstChildNoAddRef()), dataSource.get());
    IFC_RETURN(actuator.InitializeStateTriggers(pControl));

    return S_OK;
}

_Check_return_ HRESULT CVisualStateManager2::ResetVisualStateGroupToNullState(_In_ CControl *pControl, _In_ int groupIndex)
{
    auto groupCollection = GetGroupCollectionFromControl(pControl);
    ASSERT(groupCollection);
    auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());
    VisualStateManagerActuator actuator(
        static_cast<CFrameworkElement*>(pControl->GetFirstChildNoAddRef()), dataSource.get());
    IFC_RETURN(actuator.SynchronouslyResetToNullState(groupIndex));

    return S_OK;
}

// VisualStateGroup, VisualState, and members (StateTriggerCollection) are not created until they are programmatically accessed
// StateTriggers need to be moved to their VisualState.StateTriggerCollections on 'fault in'
HRESULT CVisualStateManager2::FaultInStateTriggers(_In_ CVisualStateGroupCollection* groupCollection, CustomWriterRuntimeObjectCreator& creator)
{
    CControl* pControl = groupCollection->GetOwningControl();
    VisualTree *visualTree = VisualTree::GetForElementNoRef(pControl);

    // If we're not in the live tree, we'll use the main visual tree as a fallback.
    // If we enter the live tree, we'll re-evaluate our state triggers.
    if (!visualTree)
    {
        visualTree = groupCollection->GetContext()->GetMainVisualTree();
    }

    // If we still don't have a visual tree, we'll just bail.
    if (!visualTree)
    {
        return S_OK;
    }

    std::shared_ptr<QualifierContext> qualifierContext = visualTree->GetQualifierContext();

    // Create and add StateTriggerCollection instance to each VisualState. These were intentionally skipped during VSG 'fault-in'.
    auto dataSource = CreateVisualStateManagerDataSource(groupCollection);
    int vsIndex = 0;
    for (auto& group : *groupCollection)
    {
        if (!static_cast<CVisualStateGroup*>(group)->m_pVisualStates) continue;

        for (auto& vsdo : *(static_cast<CVisualStateGroup*>(group)->m_pVisualStates))
        {
            auto vs = static_cast<CVisualState*>(vsdo);
            if (!vs->m_pStateTriggerCollection)
            {
                xref_ptr<CStateTriggerCollection> stateTriggerCollection = nullptr;
                CREATEPARAMETERS cp(vs->GetContext());
                IFC_RETURN(CreateDO(stateTriggerCollection.ReleaseAndGetAddressOf(), &cp));
                IFC_RETURN(vs->SetValueByKnownIndex(KnownPropertyIndex::VisualState_StateTriggers, stateTriggerCollection.get()));

                auto vsGroup = static_cast<CVisualStateGroup*>(group);
                if (!vsGroup->m_pStateTriggerVariantMap && pControl)
                {
                    IFC_RETURN(dataSource->TryGetOrCreateStateTriggerVariantMap(vsIndex, &(vsGroup->m_pStateTriggerVariantMap)));
                    vsGroup->m_pStateTriggerVariantMap->SetQualifierContext(qualifierContext);
                    CVisualStateManager2::AssignStateTriggerChangedCallback(pControl, *vsGroup->m_pStateTriggerVariantMap);
                }
            }
            vs->m_pStateTriggerCollection->m_pVariantMap = static_cast<CVisualStateGroup*>(group)->m_pStateTriggerVariantMap;
            vsIndex++;
        }
    }

    // Move all StateTriggers to their VisualStateGroups
    auto& visualStateGroupContexts = groupCollection->GetGroupContext();
    int vsGroupIndex = 0;
    for(auto& groupContext : visualStateGroupContexts)
    {
        if (!groupContext.GetStateTriggerVariantMap())
        {
            vsGroupIndex++;
            continue;
        }

        auto& items = groupContext.GetStateTriggerVariantMap()->GetVariantMapItems();
        int stateIndex = -1;
        int groupIndex = -1;
        for(auto& item : items)
        {
            if (!dataSource->TryGetVisualState(item.m_target, &stateIndex, &groupIndex)) return E_FAIL;
            stateIndex = groupCollection->GetCustomRuntimeData()->GetGroupVisualStateIndex(stateIndex);
            auto doGroup = static_cast<CVisualStateGroup*>((*groupCollection)[groupIndex]);

            auto visualState = static_cast<CVisualState*>((*doGroup->m_pVisualStates)[stateIndex]);

            // Move ExtensibleTriggers to StateTriggerCollection
            if(item.m_pData)
            {
                CStateTriggerBase* stateTrigger = do_pointer_cast<CStateTriggerBase>(item.m_pData.get());
                if(stateTrigger)
                {
                    IFC_RETURN(creator.RegisterTemplateNameScopeEntriesIfNeeded(stateTrigger));
                    if (stateTrigger->HasManagedPeer())
                    {
                        IFC_RETURN(stateTrigger->PegManagedPeer());
                    }
                    groupCollection->m_pDeferredStateTriggers->remove(stateTrigger);
                    IFC_RETURN(visualState->m_pStateTriggerCollection->Append(stateTrigger));
                    if (stateTrigger->HasManagedPeer())
                    {
                        stateTrigger->UnpegManagedPeer();
                    }
                }
            }
            // Move AdaptiveTriggers to StateTriggerCollection
            else
            {
                xref_ptr<CAdaptiveTrigger> adaptiveTrigger = nullptr;

                CREATEPARAMETERS cp(visualState->GetContext());
                IFC_RETURN(CreateDO(adaptiveTrigger.ReleaseAndGetAddressOf(), &cp));

                adaptiveTrigger->m_minWindowWidth = static_cast<XFLOAT>(item.m_pQualifier->Score(QualifierFlags::Width));
                adaptiveTrigger->m_minWindowHeight = static_cast<XFLOAT>(item.m_pQualifier->Score(QualifierFlags::Height));

                IFC_RETURN(visualState->m_pStateTriggerCollection->Append(adaptiveTrigger));
            }
        }

        groupContext.GetStateTriggerVariantMap().reset();

        ++vsGroupIndex;
    }

    groupCollection->GetStateTriggerVariantMaps().clear();

    // Clear deferred StateTriggers that were temporarily parented on VisualStateGroupCollection
    if (groupCollection->m_pDeferredStateTriggers)
    {
        IFC_RETURN(groupCollection->m_pDeferredStateTriggers->Clear());
    }

    return S_OK;
}

int CVisualStateManager2::GetGroupIndexFromVisualState(
        _In_ CControl *pControl, _In_z_ const WCHAR* pStateName)
{
    auto groupCollection = GetGroupCollectionFromControl(pControl);
    if (!groupCollection) return -1;

    auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());
    int stateIndex = -1;
    int groupIndex = -1;
    if(dataSource->TryGetVisualState(pStateName, &stateIndex, &groupIndex))
    {
        return groupIndex;
    }

    return -1;
}


int CVisualStateManager2::GetGroupIndexFromVisualState(
        _In_ CControl *pControl, _In_ VisualStateToken token)
{
    auto groupCollection = GetGroupCollectionFromControl(pControl);
    if (!groupCollection) return -1;

    auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());
    int stateIndex = -1;
    int groupIndex = -1;
    if(dataSource->TryGetVisualState(token, &stateIndex, &groupIndex))
    {
        return groupIndex;
    }

    return -1;
}

xref_ptr<CDependencyObject> CVisualStateManager2::GetCustomVisualStateManager(_In_ CControl *pControl)
{
    xref_ptr<CUIElement> implementationRoot = pControl->GetImplementationRoot();
    if (implementationRoot)
    {
        const CDependencyProperty* vsm = implementationRoot->GetPropertyByIndexInline(KnownPropertyIndex::VisualStateManager_CustomVisualStateManager);
        CValue vsmValue;
        if(!SUCCEEDED(implementationRoot->GetValue(vsm, &vsmValue))) return nullptr;
        if (auto customVSM = vsmValue.As<valueObject>())
        {
            return xref_ptr<CDependencyObject>(customVSM);
        }
    }

    return nullptr;
}

xref_ptr<CDependencyObject> CVisualStateManager2::GetVisualState(_In_ CControl* pControl, _In_ VisualStateToken token)
{
    auto groupCollection = GetGroupCollectionFromControl(pControl);
    if (!groupCollection) return nullptr;

    auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());
    return dataSource->GetVisualState(token);
};

xstring_ptr CVisualStateManager2::GetVisualStateName(_In_ CControl* pControl, _In_ VisualStateToken token)
{
    auto groupCollection = GetGroupCollectionFromControl(pControl);
    if (!groupCollection) return xstring_ptr();

    auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());
    return dataSource->GetVisualStateName(token);
};

void CVisualStateManager2::AssignStateTriggerChangedCallback(_In_ CControl* pControl, StateTriggerVariantMap& vm)
{
    xref::weakref_ptr<CControl> control(pControl);
    vm.SetOnSelectionChanged([control](VisualStateToken selection, VisualStateToken previousSelection, bool isInitialEvaluation)
    {
        auto pControl = control.lock();
        if(!pControl) return S_OK;

        bool succeeded = false;
        auto customVSM = CVisualStateManager2::GetCustomVisualStateManager(pControl);
        if (customVSM)
        {
            auto implementationRoot = pControl->GetImplementationRoot();
            if (implementationRoot)
            {
                // Fault-in, get VisualStateToken (may be NULL) and VisualStateGroup index (must be valid)
                auto groupCollection = static_cast<const CFrameworkElement*>(implementationRoot.get())->GetVisualStateGroupsNoCreate();
                IFC_RETURN(groupCollection->EnsureFaultedIn());
                auto token = (!selection.IsEmpty() ? selection : previousSelection);
                auto groupIndex = CVisualStateManager2::GetGroupIndexFromVisualState(pControl, token);

                // Reverse p-invoke into CustomVisualStateManager.GoToStateCore then return
                auto hr = FxCallbacks::VisualStateManager_CustomVSMGoToState(pControl, selection, groupIndex, !isInitialEvaluation, &succeeded);
                if (!succeeded)
                {
                    VSMLOG(L"[VSM]: StateTrigger-initiated GoToState on CustomVisualStateManager failed to transition: %d", hr);
                }

                IFC_RETURN(hr);
                return S_OK;
            }
        }
        else if (selection.IsEmpty())
        {
            int groupIndex = CVisualStateManager2::GetGroupIndexFromVisualState(pControl, previousSelection);

            // The first child element of a control that contains the control's triggers may be removed from the control,
            // but not destroyed. The control's triggers will remain active but cannot act on the parent control until
            // the first child element is re-parented to the control.  If the first child element has been removed from
            // its parent, 'GetGroupIndexFromVisualState' will always fail, returning -1.  Here we protect against calling
            // 'ResetVisualStateGroupToNullState' with an invalid VisualStateGroup index.  The triggers will
            // begin working again when the orphaned element is added back to its parent control.
            if(groupIndex != -1)
            {
                IFC_RETURN(CVisualStateManager2::ResetVisualStateGroupToNullState(pControl, groupIndex));
            }
        }
        else
        {
            auto hr = CVisualStateManager2::GoToStateOptimized(pControl, selection, !isInitialEvaluation, &succeeded);
            if (!succeeded)
            {
                VSMLOG(L"[VSM]: StateTrigger-initiated GoToState failed to transition: %d", hr);
            }
            IFC_RETURN(hr);
        }

        return S_OK;
    });
}

HRESULT CVisualStateManager2::TryRemoveStoryboardFromState(_In_ CVisualState* state, _In_ const xref_ptr<CStoryboard>&  storyboard)
{
    auto groupCollection = GetGroupCollectionFromVisualState(state);
    ASSERT(groupCollection);

    auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());

    int stateIndex = -1;
    int groupIndex = -1;

    bool found = dataSource->TryGetVisualState(state->GetVisualStateToken(), &stateIndex, &groupIndex);
    if(!found)
    {
        IFC_RETURN(E_NOTFOUND);
    }

    // If this is the active state, then stop and remove the storyboard
    if (stateIndex == dataSource->GetGroupContext(groupIndex).CurrentVisualStateIndex())
    {
        auto control = groupCollection->GetOwningControl();
        VisualStateManagerActuator actuator(
            do_pointer_cast<CFrameworkElement>(control->GetFirstChildNoAddRef()), dataSource.get());

        IFC_RETURN(actuator.StopAndRemoveStoryboard(groupIndex, storyboard));
    }

    return S_OK;
}

HRESULT CVisualStateManager2::TryAddStoryboardToState(_In_ CVisualState* state, _In_ const xref_ptr<CStoryboard>& storyboard)
{
    auto groupCollection = GetGroupCollectionFromVisualState(state);
    ASSERT(groupCollection);

    auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());

    int stateIndex = -1;
    int groupIndex = -1;

    bool found = dataSource->TryGetVisualState(state->GetVisualStateToken(), &stateIndex, &groupIndex);
    if (!found)
    {
        IFC_RETURN(E_NOTFOUND);
    }

    // If this is the active state, then add and start the storyboard
    if (stateIndex == dataSource->GetGroupContext(groupIndex).CurrentVisualStateIndex())
    {
        auto control = groupCollection->GetOwningControl();
        VisualStateManagerActuator actuator(
            do_pointer_cast<CFrameworkElement>(control->GetFirstChildNoAddRef()), dataSource.get());

        IFC_RETURN(actuator.StartAndAddStoryboard(groupIndex, storyboard, VisualStateGroupContext::StoryboardType::State));
    }

    return S_OK;
}


bool CVisualStateManager2::IsActiveVisualState(_In_ CVisualState* state)
{
    if (state == nullptr) return false;

    auto groupCollection = GetGroupCollectionFromVisualState(state);
    if (groupCollection)
    {
        auto dataSource = CreateVisualStateManagerDataSource(groupCollection.get());

        int stateIndex = -1;
        int groupIndex = -1;

        bool found = dataSource->TryGetVisualState(state->GetVisualStateToken(), &stateIndex, &groupIndex);
        if (found)
        {
            return stateIndex == dataSource->GetGroupContext(groupIndex).CurrentVisualStateIndex();
        }
    }

    return false;
}
