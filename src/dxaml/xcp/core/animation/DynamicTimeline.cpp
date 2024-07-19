// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DynamicTimeline.h"
#include "TimelineCollection.h"
#include "Storyboard.h"

using namespace DirectUI;

_Check_return_ HRESULT CDynamicTimeline::GenerateChildren()
{
    IFC_RETURN(GenerateChildrenInternal(GetGenerationMode(), true));
    return S_OK;
}

_Check_return_ HRESULT CDynamicTimeline::GenerateChildren(
    DynamicTimelineGenerationMode mode, _Outptr_ CTimelineCollection** ppTimelineCollection)
{
    IFC_RETURN(GenerateChildrenInternal(mode, false, ppTimelineCollection));
    return S_OK;
}

_Check_return_ HRESULT CDynamicTimeline::GenerateChildrenInternal(DynamicTimelineGenerationMode mode, bool internalChildren,
    _Outptr_opt_ CTimelineCollection** ppTimelineCollection)
{
    TraceDynamicTimelineBegin();
    auto traceEndGuard = wil::scope_exit([] { TraceDynamicTimelineEnd(); });
    CValue childrenCollectionCValue;
    xref_ptr<CTimelineCollection> childrenCollection;
    {
        CREATEPARAMETERS cp(GetContext());
        IFC_RETURN(CTimelineCollection::Create((CDependencyObject**)childrenCollection.ReleaseAndGetAddressOf(), &cp));
        childrenCollectionCValue.WrapObjectNoRef(childrenCollection.get());
    }

    // For internal children we simply set this collection to be the internal children collection, as
    // we're often replacing it. We do this first because there is some magic dynamic timeline parenting
    // that happens once this collection is a child of a DynamicTimeline.
    if (internalChildren)
    {
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::DynamicTimeline_Children, childrenCollectionCValue));
    }

    IFC_RETURN(FxCallbacks::XcpImports_GetDynamicTimelines(this,
        mode == DynamicTimelineGenerationMode::SteadyState,
        &childrenCollectionCValue));

    // When we generate our children into a collection for export we have this funky idea of a dynamic timeline
    // parent just to keep things interesting. The purpose is to ensure the targets resolve correclty, one can't
    // help but wonder if there was a better way than introducing yet another parent concept into our framework...
    if (!internalChildren)
    {
        for (auto& child : *childrenCollection)
        {
            auto asTimeline = do_pointer_cast<CTimeline>(child);
            asTimeline->SetDynamicTimelineParent(this);
        }
    }

    if (ppTimelineCollection) *ppTimelineCollection = childrenCollection.detach();
    return S_OK;
}

_Check_return_ HRESULT CDynamicTimeline::ExpandChildren()
{
    if (!HasChildren() && CanBeModified()) IFC_RETURN(GenerateChildren());
    return S_OK;
}

_Check_return_ HRESULT CDynamicTimeline::OnBegin()
{
    CCoreServices* core = GetContext();
    // Typically TransitionTargets are not faulted in when an animation is targetting them-
    // however for LayoutTransitions and DynamicTimelines we have a global flag that allows
    // for these targets to be faulted in as needed to power the transitions.
    bool coreAllowedTransitionTargetCreation = core->IsAllowingTransitionTargetCreations();
    core->SetAllowTransitionTargetCreation(TRUE);
    auto resetTransitionTargetCreationGuard = wil::scope_exit([&] {
        core->SetAllowTransitionTargetCreation(coreAllowedTransitionTargetCreation); });

    IFC_RETURN(ExpandChildren());

    // Instead of doing a tiny big of refactoring we've shamelessly duplicated
    // the logic in CTimelineGroup::OnAddToTimeManager. Be wary.
    if(m_pChild)
    {
        for (const auto& child : *m_pChild)
        {
            auto asTimeline = static_cast<CTimeline*>(child);
            if (m_fIsInTimeManager) IFC_RETURN(asTimeline->OnAddToTimeManager());
            IFC_RETURN(asTimeline->OnBegin());
        }
    }

    // This flag's state is maintained by code here, in CStoryboard,
    // in CParallelTimeline, CAnimation, and CTimeline.
    ResetCompletedEventFired();
    return S_OK;
}

// Once a DynamicTimeline completes animating we intentionally throw away the children
// storyboards we created. They were dynamically generated from PVL data and they are created
// differently depending on if the GoToState call requested this animation generate a steady
// state value. It is frustrating that steady state generation is an internal concept at this
// point and only consumed via private APIs in the VSM.
_Check_return_ HRESULT CDynamicTimeline::FinalizeIteration()
{
    IFC_RETURN(CParallelTimeline::FinalizeIteration());
    IFC_RETURN(ClearValue(GetPropertyByIndexInline(KnownPropertyIndex::DynamicTimeline_Children)));
    return S_OK;
}

void CDynamicTimeline::AnimationTrackingCollectInfoNoRef(
    _Inout_ CDependencyObject** ppTarget,
    _Inout_ CDependencyObject** ppDynamicTimeline)
{
    if (!*ppDynamicTimeline) *ppDynamicTimeline = this;
    CParallelTimeline::AnimationTrackingCollectInfoNoRef(ppTarget, ppDynamicTimeline);
}


