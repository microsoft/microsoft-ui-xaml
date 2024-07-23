// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Storyboard.h"
#include "ParallelTimeline.h"
#include "DynamicTimeline.h"
#include "TimelineCollection.h"
#include "DoubleAnimation.h"
#include "DoubleAnimationUsingKeyFrames.h"
#include "DoubleKeyFrame.h"
#include "KeyTime.h"
#include "DependencyObject.h"
#include <FeatureFlags.h>
#include "CFrame.g.h"

#include <vsm\inc\DynamicTimelineHelper.h>

using namespace DirectUI;

//------------------------------------------------------------------------
//  Synopsis:
//      Helper that searches for a timeline targetting a specific object
//      and rewrites to target a different object.
//------------------------------------------------------------------------
_Check_return_ HRESULT
RewriteTransitionTargets(
    _In_ CParallelTimeline* pStoryboard,
    _In_ CUIElement* pOwnerOfTransitionTarget,  // we did not yet force resolve of TT on pTarget, so we'll compare to it through pTarget as we go
    _In_ CTransitionTarget* newTransitionTarget,
    _In_opt_ CLayoutTransitionElement* pNewLayoutTransitionElement)
{
    HRESULT hr = S_OK;
    CTimeline* pTimeline = NULL;

    if (!pStoryboard)
    {
        RRETURN(hr);
    }

    XUINT32 nChildCount = 0;
    XUINT32 nIndex = 0;
    CTimelineCollection* pChildren = NULL;

    pChildren = pStoryboard->m_pChild;
    if (pChildren)
    {
        nChildCount = pChildren->GetCount();
        for (nIndex = 0; nIndex < nChildCount; nIndex++)
        {
            pTimeline = static_cast<CTimeline*>(pChildren->GetItemWithAddRef(nIndex));
            CParallelTimeline* pChildStoryboard = do_pointer_cast<CParallelTimeline>(pTimeline);    // todo: does this work??
            if (pChildStoryboard)
            {
                IFC(RewriteTransitionTargets(pChildStoryboard, pOwnerOfTransitionTarget, newTransitionTarget, pNewLayoutTransitionElement));
            }
            else if (pTimeline->IsAnimation())  // metadata describes animation as inheriting from timeline
            {
                CAnimation* pAnimation = static_cast<CAnimation*>(pTimeline);
                if (pAnimation)
                {
                    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strUIElementProjectionPropertyPath, L"(UIElement.Projection)");
                    CDependencyObject* pNewTarget = NULL;
                    const CDependencyProperty* pProperty = NULL;

                    // TransitionTargets do not support projections presently. If a transition involves a plane projection (e.g. PointerUpThemeAnimation, CommonNavigationTransitionInfo),
                    // we will attach it directly to the LTE. We also don't want to setup a default projection on every LTE because it is
                    // potentially very expensive to render. Similarly, we want to avoid making it a requisite for the redirection targets of LTEs
                    // to have a projection attached just for the transition since it is redundant. Thus, instead of resolving the property path of the
                    // animation right away (and possibly failing to do so), we will parse it ourselves to see if the target property involves a projection.
                    // If it does, we will attach a plane projection to the LTE.
                    if (!pAnimation->m_strTargetProperty.IsNullOrEmpty() && pAnimation->m_strTargetProperty.StartsWith(strUIElementProjectionPropertyPath, xstrCompareCaseSensitive))
                    {
                        bool isOwnerOfTransitionTarget = false;
                        xref_ptr<CDependencyObject> targetObject = pAnimation->GetTargetObject();

                        // If the target object of the animation is NULL, that means that the animation
                        // does not have a specific target and will fallback to animate the target object of the
                        // storyboard it belongs to.
                        if (!targetObject)
                        {
                            targetObject = pStoryboard->GetTargetObject();
                        }

                        isOwnerOfTransitionTarget = (targetObject.get() == pOwnerOfTransitionTarget);

                        if (isOwnerOfTransitionTarget)
                        {
                            CDependencyObject* pLayoutTransitionElementAsDO = static_cast<CDependencyObject*>(pNewLayoutTransitionElement);

                            auto projection = pNewLayoutTransitionElement->GetProjection();
                            if (!projection)
                            {
                                CREATEPARAMETERS cp(pNewLayoutTransitionElement->GetContext());
                                IFC(CPlaneProjection::Create(reinterpret_cast<CDependencyObject**>(projection.ReleaseAndGetAddressOf()), &cp));

                                IFC(pNewLayoutTransitionElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Projection, projection.get()));
                            }

                            pNewTarget = projection.get();

                            IFC(pAnimation->GetContext()->ParsePropertyPath(
                                    &pLayoutTransitionElementAsDO,
                                    &pProperty,
                                    pAnimation->m_strTargetProperty));
                        }
                    }
                    else
                    {
                        xref::weakref_ptr<CDependencyObject> pDORef;
                        CDependencyObject* pTarget = nullptr;

                        // this will resolve and potentially create TransitionTargets
                        IFC(pAnimation->GetAnimationTargets(&pDORef, &pProperty));

                        // the target will likely be an object on TransitionTarget (like CompositeTransform)
                        pTarget = pDORef.lock();

                        if (pTarget)
                        {
                            auto transitionTarget = pOwnerOfTransitionTarget->GetTransitionTarget();
                            if (transitionTarget)
                            {
                                if (pTarget == transitionTarget)
                                {
                                    pNewTarget = newTransitionTarget;
                                }
                                else if (pTarget == transitionTarget->m_pxf)
                                {
                                    pNewTarget = newTransitionTarget->m_pxf;
                                }
                                else if (pTarget == transitionTarget->m_pClipTransform)
                                {
                                    pNewTarget = newTransitionTarget->m_pClipTransform;
                                }
                            }
                        }
                    }

                    if (pNewTarget)
                    {
                        IFC(pTimeline->SetTargetObject(pNewTarget));

                        // manually setting, will make sure in the next Resolve (triggered by the onBegin)
                        // will not result in overwriting these properties (or actually failing since
                        // the old propertypath won't resolve anymore against the new target).
                        pTimeline->SetTargetProperty(pProperty);
                    }
                }
            }

            // Release references added in this loop iteration.
            ReleaseInterface(pTimeline);
        }
    }

Cleanup:
    ReleaseInterface(pTimeline);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Synopsis: takes an internal trigger and determines the public trigger for it.
//  The public trigger has a lower granularity.
//
//  Note: I've decided that reparenting (used to publicly represented as a layouttrigger)
//  is a good trigger to publicly have, so the granularity is currently the same.
//  However, keeping this method since we expect other triggers (visibility) to come in that
//  will need to be translated to a public concept.
//------------------------------------------------------------------------
_Check_return_ HRESULT
GetPublicTransitionTrigger(_In_ DirectUI::TransitionTrigger trigger, _In_ DirectUI::TransitionTrigger& publicTrigger)
{
    switch (trigger)
    {
    case DirectUI::TransitionTrigger::Reparent:
        {
            publicTrigger = DirectUI::TransitionTrigger::Reparent;
            break;
        }
    case DirectUI::TransitionTrigger::Layout:
        {
            publicTrigger = DirectUI::TransitionTrigger::Layout;
            break;
        }
    case DirectUI::TransitionTrigger::Load:
        {
            publicTrigger = DirectUI::TransitionTrigger::Load;
            break;
        }
    case DirectUI::TransitionTrigger::Unload:
        {
            publicTrigger = DirectUI::TransitionTrigger::Unload;
            break;
        }
    default:
        {
            IFC_RETURN(E_FAIL);
        }
    }
    return S_OK;

}


LayoutTransitionStorage::~LayoutTransitionStorage()
{
    IGNOREHR(ClearStoryboards());
}


//-------------------------------------------------------------------------
//  Synopsis:   Registers a UIElement as needing for a transition.
//  Usage:      Use to register the new transition to the workitems that layout
//              will process.
//  Note:       Calling this method replaces transitions that were previously registered.
//-------------------------------------------------------------------------
void LayoutTransitionStorage::RegisterElementForTransitions(_In_ CUIElement* pTarget, _In_ xvector<CTransition*>& transitions, _In_ TransitionTrigger trigger)
{
    if (!pTarget->GetLayoutStorage())
    {
        return;
    }

    UnregisterElementForTransitions(pTarget);

    bool isAnimationEnabled = true;
    // Check animations are disabled across the system
    IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));

    LayoutTransitionStorage* pStorage = pTarget->GetLayoutTransitionStorage();
    // take refs of the new transitions
    if (transitions.size() > 0 &&           // only a transition if there are applicable transitions to handle the event
        isAnimationEnabled)                                // Animations are turned off in the Control panel settings
    {
        CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
        pLayoutManager->RegisterElementForDeferrredTransition(pTarget, this);    // see note in the else clause
        for(auto transition : transitions)
        {
            m_registeredTransitions.emplace_back(transition);   // todo: this inverts the order of transitions. Reason about whether that is important or not.
        }

        pStorage->SetTrigger(trigger);  // remember that these transitions were created because of this trigger
    }
    else
    {
        // NOTE: we do not unregister this element for deferred transitions. That means that an element might be in the layoutmanagers deferred list
        // more than once or in the wrong order. The realization logic has been hardened to deal with this.
        // I find the ordering of the deferred registration of low importance at the moment, but this might have to change
        // in the future.
        pStorage->SetTrigger(DirectUI::TransitionTrigger::NoTrigger);
    }
}

//-------------------------------------------------------------------------
//  Synopsis:   Removes all transitions that were registered to be realized.
//
//-------------------------------------------------------------------------
void LayoutTransitionStorage::UnregisterElementForTransitions(_In_ CUIElement* pTarget)
{
    if (m_bRegisteredInLayoutManager)   // this check should be internal logic in layoutmanager, but I do not wish to pay the perf for doing the lookup for the layoutmanager if not necessary
    {
        auto pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
        pLayoutManager->UnregisterElementForDeferredTransition(pTarget, this);
    }

    // clear out all earlier transitions that were registered for this element but not executed yet.
    m_registeredTransitions.clear();
}

//------------------------------------------------------------------------
//  Synopsis: Cleans up the reference this storage keeps to a live storyboard
//  by stopping.
//------------------------------------------------------------------------
_Check_return_
HRESULT
LayoutTransitionStorage::RegisterStoryboard(_In_ CStoryboard* pStoryboard)
{
    IFC_RETURN(m_transitionStoryboards.push_back(pStoryboard));
    AddRefInterface(pStoryboard);   // take ownership

    return S_OK;
}

//------------------------------------------------------------------------
//  Synopsis: Cleans up the reference this storage keeps to a live storyboard
//  by stopping.
//  Note: does _not_ remove from storyboard vector (perf).
//------------------------------------------------------------------------
_Check_return_ HRESULT LayoutTransitionStorage::CleanupStoryboard(_In_ CStoryboard* pStoryboard)
{
    HRESULT hr = S_OK;
    LayoutTransitionCompletedData* pData = NULL;

    if (pStoryboard)
    {
        pData = pStoryboard->m_pLayoutTransitionCompletedData;
        if (pData)  // pData is null if we failed in SetupTransition before assigning a pData
        {
            IFC(pStoryboard->RemoveEventListener(EventHandle(KnownEventIndex::Timeline_Completed), &pData->m_EventListenerToken));
            delete pStoryboard->m_pLayoutTransitionCompletedData;
            pStoryboard->m_pLayoutTransitionCompletedData = NULL;
        }

        // want to clear out the storyboard before stopping, since it has side effects
        // that are effected by having a valid instance in storage
        IFC(pStoryboard->StopPrivate());   // will reset animated value to localvalue
    }

Cleanup:
    ReleaseInterface(pStoryboard);  // always release our hold
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Synopsis: Cleans up the references to all storyboards and clears the list.
//------------------------------------------------------------------------
_Check_return_ HRESULT LayoutTransitionStorage::ClearStoryboards()
{
    for(xvector<CStoryboard*>::iterator it = m_transitionStoryboards.begin(); it != m_transitionStoryboards.end(); ++it)
    {
        IFC_RETURN(CleanupStoryboard(*it));
    }
    m_transitionStoryboards.clear();
    SetTrigger(DirectUI::TransitionTrigger::NoTrigger);

    return S_OK;
}

//------------------------------------------------------------------------
// Synopsis: the primary brush is the LTE providing location and opacity information.
// it basically means: this is what we consider to be what is on the screen.
// we only check the first two LTE's and use the one with the highest opacity
// since that is the one that has focused the users eye.
// As a secondary, more expensive criteria, we can use the localclip
//------------------------------------------------------------------------
_Check_return_ CLayoutTransitionElement* LayoutTransitionStorage::GetPrimaryBrush()
{
    XUINT32 cBrushes = m_transitionElements.size();
    CUIElement* pParent = NULL;
    CLayoutTransitionElement* pPrimaryBrush = NULL;
    CLayoutTransitionElement* pBrushOne = cBrushes > 0 ? m_transitionElements[0] : NULL;
    CLayoutTransitionElement* pBrushTwo = cBrushes > 1 ? m_transitionElements[1] : NULL;

    // default is first LTE
    pPrimaryBrush = pBrushOne;

    if (pBrushTwo)
    {
        if (pBrushOne->GetOpacityCombined() != pBrushTwo->GetOpacityCombined())
        {
            pPrimaryBrush = pBrushOne->GetOpacityCombined() > pBrushTwo->GetOpacityCombined() ? pBrushOne : pBrushTwo;
        }
        else
        {
            XRECTF elementBounds = {0, 0, pBrushOne->GetActualWidth(), pBrushOne->GetActualHeight() };
            XRECTF brushOneBounds = elementBounds;
            XRECTF brushTwoBounds = elementBounds;

            // use the localclip AND the clip of the parent.
            // fundamentally we should be using all clips, but it is too expensive to walk all the way up
            // since we use a clip on the direct parent for the moco scenarios, for now just also using the parents
            // clip is enough

            // local clip is applied to localbounds (based on 0,0)
            pBrushOne->GetTransitionTarget()->ApplyClip(elementBounds, &brushOneBounds);
            pBrushTwo->GetTransitionTarget()->ApplyClip(elementBounds, &brushTwoBounds);

            // add offsets to get actual locations relative to parent
            brushOneBounds.X = pBrushOne->GetPositionRelativeToParent().x;
            brushOneBounds.Y = pBrushOne->GetPositionRelativeToParent().y;
            brushTwoBounds.X = pBrushTwo->GetPositionRelativeToParent().x;
            brushTwoBounds.Y = pBrushTwo->GetPositionRelativeToParent().y;

            // clip of parent  (which is the parent of the transition root)
            ASSERT(pBrushOne->GetTargetElement()); // if we have a LTE, it needs to have a target
            pParent = do_pointer_cast<CUIElement>(pBrushOne->GetTargetElement()->GetParentInternal(false));

            if (pParent)
            {
                // assuming both parents are the same. Do not know of any scenarios where this is not the case
                ASSERT(pParent == pBrushTwo->GetTargetElement()->GetParentInternal(false));

                const CMILMatrix localTransform(TRUE);
                auto clip = pParent->GetClip();
                CRectangleGeometry* pClip = static_cast<CRectangleGeometry*>(clip.get());

                if (pClip != NULL)
                {
                    ASSERT(clip->GetTypeIndex() == KnownTypeIndex::RectangleGeometry);
                    CRectangleGeometry::ApplyClip(pClip, &localTransform, &brushOneBounds);
                    CRectangleGeometry::ApplyClip(pClip, &localTransform, &brushTwoBounds);
                }

                // I should truly be also looking at layoutclip here, however, this is a hotpath and I wish to limit
                // the amount of expensive lookups we do here. We use the localclip of the parent as an input to the
                // AddDeleteThemeTransition.
            }

            // pick most visible brush
            pPrimaryBrush = brushOneBounds.Width * brushOneBounds.Height < brushTwoBounds.Width * brushTwoBounds.Height ? pBrushTwo : pBrushOne;
        }

    }
    return pPrimaryBrush;
}

//------------------------------------------------------------------------
//  Synopsis: Registers a brush, and puts it in the tree.
//------------------------------------------------------------------------
_Check_return_ HRESULT
LayoutTransitionStorage::RegisterBrushRepresentation(
    _In_ CUIElement* pTarget,
    _In_ CLayoutTransitionElement* pLT,
    _In_ TransitionParent transitionParent
    )
{
    HRESULT hr = S_OK;
    bool addedRenderer = false;

    CUIElement* pParentForTransition = NULL;
    CTransitionRoot* layerNoRef = NULL;

    IFCPTR(pLT);

    if (transitionParent == DirectUI::TransitionParent::ParentToCommonParent)
    {
        // todo: this is the place where we will have the nearest common parent algorithm when that feature is approved
        pParentForTransition = do_pointer_cast<CUIElement>(pTarget->GetParentInternal());
        // allow for this to be null, since we will fallback to using the upper root
        if (pParentForTransition)
        {
            layerNoRef = pParentForTransition->GetLocalTransitionRoot(true);
        }
    }
    else if (transitionParent == DirectUI::TransitionParent::ParentToGrandParent)
    {
        pParentForTransition = do_pointer_cast<CUIElement>(pTarget->GetParentInternal());
        // allow for this to be null, since we will fallback to using the upper root
        if (pParentForTransition)
        {
            pParentForTransition = do_pointer_cast<CUIElement>(pParentForTransition->GetParentInternal());

            if (pParentForTransition)
            {
                layerNoRef = pParentForTransition->GetLocalTransitionRoot(true);
            }
        }
    }

    if (!layerNoRef)
    {
         // fallback
         layerNoRef = VisualTree::GetTransitionRootForElement(pTarget);
    }

    IFC(pLT->AttachTransition(pTarget, layerNoRef));
    addedRenderer = TRUE;

    // lifetime is managed by putting the element in the panel
    IFC(m_transitionElements.push_back(pLT));

Cleanup:
    if (FAILED(hr) && addedRenderer)
    {
        VERIFYHR(pTarget->RemoveLayoutTransitionRenderer(pLT));
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Synopsis: Cleans up the bitmap being used to show the animation of
//  an element.
//  Note: only expected to be called from FAILED(hr) methods. Usually
//  you would use CleanupBrushRepresentations.
//------------------------------------------------------------------------
_Check_return_ HRESULT
LayoutTransitionStorage::UnregisterBrushRepresentation(
    _In_ CUIElement* pTarget,
    _In_ CLayoutTransitionElement* pLT,
    _In_ bool removeFromInternalStorage
    )
{
    CTransitionRoot* pLayer = do_pointer_cast<CTransitionRoot>(pLT->GetParentInternal(false));
    // if we don't have this, we are seriously in trouble: we won't be able to ever remove the LTE, leaving the lte visible on the screen forever
    IFCPTR_RETURN(pLayer);

    // TODO: JCOMP: pLT can be a dangling ptr after this call. Seems like m_transitionElements should hold a strong ref.
    IFC_RETURN(pLT->DetachTransition(pTarget, pLayer));

    if (removeFromInternalStorage)
    {
        xvector<CLayoutTransitionElement*>::const_reverse_iterator rend = m_transitionElements.rend();
        for (xvector<CLayoutTransitionElement*>::const_reverse_iterator it = m_transitionElements.rbegin(); it != rend; ++it)
        {
            if (*it == pLT)
            {
                IFC_RETURN(m_transitionElements.erase(it));
                break;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Synopsis: Cleans up the bitmaps being used to show the animation of
//  an element.
//------------------------------------------------------------------------
_Check_return_ HRESULT
LayoutTransitionStorage::CleanupBrushRepresentations(_In_ CUIElement* pTarget)
{
    HRESULT recordHr = S_OK;

    CTransitionRoot* pLayer = VisualTree::GetTransitionRootForElement(pTarget);
    if (pLayer)
    {
        for(xvector<CLayoutTransitionElement*>::iterator it = m_transitionElements.begin(); it != m_transitionElements.end(); ++it)
        {
            RECORDFAILURE(UnregisterBrushRepresentation(pTarget, *it, FALSE)); // this walks the vector in UIElement, possibly just clear in one go.
        }
    }

    m_transitionElements.clear();

    RRETURN(recordHr);
}

//------------------------------------------------------------------------
//  Synopsis: Updates running transitions to a new location.
//------------------------------------------------------------------------
_Check_return_ HRESULT LayoutTransitionStorage::UpdateBrushes(_In_ XRECTF finalRect)
{
    HRESULT hr = S_OK;
    XPOINTF newDestination = { 0, 0 };
    newDestination.x = finalRect.X;
    newDestination.y = finalRect.Y;

    for(xvector<CLayoutTransitionElement*>::iterator it = m_transitionElements.begin(); it != m_transitionElements.end(); ++it)
    {
        (*it)->SetDestinationOffset(newDestination);
    }

    RRETURN(hr);
}

_Check_return_
HRESULT
LayoutTransitionStorage::GetTriggerForPublicConsumption(_Out_ DirectUI::TransitionTrigger *pTrigger)
{
    RRETURN(GetPublicTransitionTrigger(GetTrigger(), *pTrigger));
}

CTransition::CTransition(_In_ CCoreServices *pCore)
    : CMultiParentShareableDependencyObject(pCore)
    , m_pStaggerFunction(nullptr)
{
}

CTransition::~CTransition()
{
    ReleaseInterface(m_pStaggerFunction);
}

namespace CoreImports
{
    _Check_return_ HRESULT Transition_SetIsStaggeringEnabled(
        _In_ void *pTransition,
        _In_ bool value)
    {
        CTransition *pElement = NULL;

        // Do paramater validation
        IFCPTR_RETURN(pTransition);
        pElement = static_cast<CTransition*>(pTransition);

        pElement->SetIsStaggeringEnabled(value);

        return S_OK;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis: Allows the current transition to prepare snapshot data.
//
//  (static)
//------------------------------------------------------------------------
_Check_return_
HRESULT CTransition::OnLayoutChanging(_In_ CUIElement* pTarget, _In_ XRECTF finalRect)
{
    LayoutTransitionStorage* pStorage = NULL;
    CLayoutManager* pLayoutManager = NULL;
    IFCPTR_RETURN(pTarget);

    pStorage = pTarget->GetLayoutTransitionStorage();

    pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
    IFCEXPECT_RETURN(pLayoutManager);

    // we're running layout on this element, might want to update the magic number for enteredTreeCounter and leaveTreeCounter
    if (pTarget->m_enteredTreeCounter == EnteredInThisTick)
    {
        pTarget->m_enteredTreeCounter = pLayoutManager->GetLayoutCounter();
    }
    if (pTarget->m_leftTreeCounter == LeftInThisTick)
    {
        pTarget->m_leftTreeCounter = pLayoutManager->GetLayoutCounter();
    }

    // before layout starts (and new visualoffset and rendersizes are set), information that was
    // gathered during last layout cycle becomes the new assumed position of this element.
    if (pStorage && pLayoutManager->GetLayoutCounter() >= pStorage->m_nextGenerationCounter)
    {
        pStorage->m_currentOffset = pStorage->m_nextGenerationOffset;
        pStorage->m_currentSize = pStorage->m_nextGenerationSize;
    }

    if (!pLayoutManager->GetIsInLayoutTransitionPhase())
    {
        // do not react to layout changes inside a layouttransition
        // all transitions will have been created during an actual measure/arrange

        if (pStorage)
        {
            // regardless of whether we wish to animate, the destination should always be correct.
            // When a new slot comes in that does not exceed the configured delta, the transition destination
            // might not be updated. However, after the transition has finished we wish to make sure
            // we end up in the correct location as layed out right now.
            pStorage->m_arrangeInput = finalRect;
        }

    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis: Finish setup and register transition instances.
//
//  (static)
//------------------------------------------------------------------------
_Check_return_
HRESULT CTransition::OnLayoutChanged(_In_ CUIElement* pTarget)
{
    HRESULT hr = S_OK;
    CLayoutManager* pLayoutManager = NULL;
    LayoutTransitionStorage* pStorage = NULL;
    TransitionTrigger currentTrigger = DirectUI::TransitionTrigger::NoTrigger;
    TraceProcessLayoutForTransitionBegin();

    IFCEXPECT(pTarget);

    pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
    IFCEXPECT(pLayoutManager);

    pStorage = pTarget->GetLayoutTransitionStorage();

    if (!pStorage)
    {
        // never setup storage, so apparently no transition possible
        // (transitions should pro-actively setup storage)
        goto Cleanup;
    }
    currentTrigger = pStorage->GetTrigger();


    // applies only on the very first creation of this element: the nextgeneration
    // counter is 0 means it has never run layout before.
    // In this case the 'next generation' informationset, is the set that should be used for
    // a potential load transition.
    if (pStorage && pStorage->m_nextGenerationCounter == 0 && pLayoutManager)
    {
        // never had layout before
        CTransition::SetNextGenerationInformationFromLayout(pTarget,pStorage, pLayoutManager->GetNextLayoutCounter());
        pStorage->m_currentOffset = pStorage->m_nextGenerationOffset;
        pStorage->m_currentSize = pStorage->m_nextGenerationSize;
    }

    if (pLayoutManager->GetIsInLayoutTransitionPhase())
    {
        // do not react to layout changes inside a layouttransition
        // all transitions will have been created during an actual measure/arrange
        goto Cleanup;
    }

    // cancelling current transitions, only necessary if unloading?
    // todo: cancel transitions even when there is no transition to replace them
//  if (!pCurrentLT && CLayoutTransition::HasActiveTransitionAnimation(pTarget) &&
//      pTarget->HasLayoutStorage() &&
//      pStorage->m_pTransition->OfTypeByIndex<KnownTypeIndex::UnloadTransition>())
//  {
//      // Unload transitions do not need to this cleanup since they will not run layout again.
//      // Their cancellation will merely remove them from the visual tree
//
//      // currently in a transition, but no transition to replace is applicable
//      // solution: interrupt current transition.
//      // scenario: load transition is on-going and a new layoutpass is occurring while
//      // element has no layouttransition
//
//      // when the transition is cancelled the transitiondestination is used to place the element.
//      // however, layout has just run and made that destination out-of-date
//      // update with new destination
//      XRECTF newDestination = {pTarget->GetActualOffsetX(), pTarget->GetActualOffsetY(), pTarget->GetActualWidth(), pTarget->GetActualHeight()};
//      // measure and arrange have run with these values, so layout is valid
//
//      // measure and arrange may come in with a target that is the same as our current animation.
//      // in that case we do not want to cancel. The situation in which this occurs is a parents
//      // transition completing which schedules a measure and arrange for an element that is loading.
//      pStorage = pTarget->GetLayoutTransitionStorage();
//      if(newDestination.Width != pStorage->m_sizeDestination.width
//          || newDestination.Height != pStorage->m_sizeDestination.height
//          || newDestination.X != pStorage->m_transformDestination.GetDx()
//          || newDestination.Y != pStorage->m_transformDestination.GetDy())
//      {
//          const bool fNewWalk = pTarget->GetContext()->IsNewWalkEnabled();
//          pTarget->RenderSize = newDestination.Size();
//          pTarget->VisualOffset.x = newDestination.X;
//          pTarget->VisualOffset.y = newDestination.Y;
//          if (fNewWalk)
//          {
//              // Mark this element's rendering content as dirty.
//              CUIElement::NWSetContentDirty(pTarget);
//          }
//          else
//          {
//              XUINT32 affectedFlags = 0;
//              affectedFlags |= PROP_AFFECT_LAYOUT | PROP_AFFECT_XFORM;
//              IFC(pTarget->GetContext()->SetFrameDirty());
//              pTarget->SetAffected(affectedFlags);
//          }
//
//          IFC(pLayoutManager->RegisterElementForLayoutTransition(pTarget, NULL));    // will correctly interrupt current transition and unregister
//      }
//      goto Cleanup;
//  }

    if (pStorage)
    {
        // regardless of whether we wish to animate, the destination should always be correct.
        // When a new slot comes in that does not exceed the configured delta, the transition destination
        // might not be updated. However, after the transition has finished we wish to make sure
        // we end up in the correct location as layed out right now.
        XRECTF newDestination = {pTarget->GetActualOffsetX(), pTarget->GetActualOffsetY(), pTarget->GetActualWidth(), pTarget->GetActualHeight()};
        pStorage->m_arrangeOutput = newDestination;
    }

    if (currentTrigger != DirectUI::TransitionTrigger::Unload)   // no other transition possible when unloading
    {
        if (pLayoutManager->GetAllowTransitionsToRunUnderCurrentSubtree())
        {
            // before calling to Impl, make sure the current offset is correct.
            // we _only_ want to update the current offset if there is an active transition, otherwise
            // trust the last set information.
            // this is the version of m_currentOffset and m_currentSize that is used to trigger transitions.
            CTransition::SetInformationFromAnimation(pTarget, pStorage, pLayoutManager->GetNextLayoutCounter());

            //  CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
            XUINT16 layoutCounter = pLayoutManager->GetLayoutCounter();
            XUINT16 enterCounter = pTarget->m_enteredTreeCounter;
            XUINT16 leftCounter = pTarget->m_leftTreeCounter;

            // note how unloading is not triggered from layout, so we only manage load and layout

            // load transition
            if (enterCounter == layoutCounter && leftCounter != layoutCounter)
            {
                IFC(CTransition::ProcessLoadTrigger(pTarget, pStorage));
            }
            // layout transition
            else if (enterCounter < layoutCounter || (enterCounter == layoutCounter && leftCounter == layoutCounter))
            {
                IFC(CTransition::ProcessLayoutTrigger(pTarget, pStorage));
            }
        }
        else if (pStorage)
        {
            // we would normally cancel a transition and setup a new one, from the old location. However, if we are
            // not allowed to setup new transitions (scenario is scrolling a listbox), we do want the LTE to match
            // the new location of the UIElement
            if (CTransition::HasActiveTransition(pTarget))
            {
                IFC(pStorage->UpdateBrushes(pStorage->m_arrangeOutput));
            }
            else
            {
                // if we got layout while we were explicitly ignoring a transition, we want our next layout to
                // 'act' as though it originated from the newly set location.
                // This whole flag 'allow transitions to run' is really a hack, and just used by virtualizing panels.
                // It should be interpreted as 'we are correcting layout after a manipulation'.
                CTransition::SetNextGenerationInformationFromLayout(pTarget, pStorage, pLayoutManager->GetLayoutCounter());
            }
        }
    }

Cleanup:
    // call to update current information. If we have a transition at this point, it will use that information
    // otherwise use this layout information
    if (pStorage && pLayoutManager && pLayoutManager->GetAllowTransitionsToRunUnderCurrentSubtree())
    {
        CTransition::SetNextGenerationInformationFromLayout(pTarget, pStorage, pLayoutManager->GetNextLayoutCounter());
    }
    TraceProcessLayoutForTransitionEnd();
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Synopsis: Allows transitions to be registered
//------------------------------------------------------------------------
_Check_return_ HRESULT CTransition::ProcessLoadTrigger(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage)
{
    xvector<CTransition*> applicableTransitions;
    TransitionTrigger trigger = DirectUI::TransitionTrigger::Load;
    XRECTF newDestination = {pTarget->GetActualOffsetX(), pTarget->GetActualOffsetY(), pTarget->GetActualWidth(), pTarget->GetActualHeight()};

    // register this transition instance. This will unregister any other transition already registered.
    // build up list of transition instances that will participate
    IFC_RETURN(CTransition::AppendAllTransitionsNoAddRefs(pTarget, trigger, applicableTransitions));

    // first thing that we need is to cancel transitions, so that we get the correct information inside of pStorage.
    // notice how we can make multiple calls to cancellation (each transition for this pTarget will hit this code)
    // but transitions that are being called are _current_ animations, while we are not setting up _future_ transitions.
    IFC_RETURN(CancelTransitions(pTarget));

    pStorage->m_sizeDestination = newDestination.Size();
    pStorage->m_transformDestination.SetDx(newDestination.X);
    pStorage->m_transformDestination.SetDy(newDestination.Y);

    // setup start according to destination snapshot
    pStorage->m_sizeStart = pStorage->m_sizeDestination;
    pStorage->m_transformStart = pStorage->m_transformDestination;

    // get opacity
    {
        XFLOAT actualOpacity = pTarget->GetOpacityToRoot();

        pStorage->m_opacityStart = 1.0f;    // probably animated
        pStorage->m_opacityDestination = actualOpacity;
    }

    pStorage->m_scaleStart = 1.0f;

    pStorage->RegisterElementForTransitions(pTarget, applicableTransitions, trigger);  // register transitions to be called back later for setting up storyboard

    return S_OK;

}
_Check_return_ HRESULT CTransition::ProcessLayoutTrigger(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage)
{
    bool exceededDelta = false;
    XRECTF newDestination = {pTarget->GetActualOffsetX(), pTarget->GetActualOffsetY(), pTarget->GetActualWidth(), pTarget->GetActualHeight()};
    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
    XUINT16 layoutCounter = pLayoutManager->GetLayoutCounter();
    XUINT16 enteredCounter = pTarget->m_enteredTreeCounter;
    XUINT16 leftCounter = pTarget->m_leftTreeCounter;
    TransitionTrigger trigger = DirectUI::TransitionTrigger::Layout;
    xvector<CTransition*> applicableTransitions;
    XFLOAT  rDelta = 5.0f;


    if (enteredCounter == layoutCounter && leftCounter == layoutCounter)
    {
        trigger = DirectUI::TransitionTrigger::Reparent;
    }

        // compare against the destination if we are in progress, otherwise against current situation
    if (HasTransitionAnimations(pTarget))
    {
        exceededDelta = XcpAbsF(newDestination.Width - pStorage->m_sizeDestination.width) > rDelta
            || XcpAbsF(newDestination.Height - pStorage->m_sizeDestination.height)        > rDelta
            || XcpAbsF(newDestination.X - pStorage->m_transformDestination.GetDx())       > rDelta
            || XcpAbsF(newDestination.Y - pStorage->m_transformDestination.GetDy())       > rDelta;
    }
    else
    {
        exceededDelta = XcpAbsF(newDestination.Width - pStorage->m_currentSize.width) > rDelta
            || XcpAbsF(newDestination.Height - pStorage->m_currentSize.height)        > rDelta
            || XcpAbsF(newDestination.X - pStorage->m_currentOffset.x)                > rDelta
            || XcpAbsF(newDestination.Y - pStorage->m_currentOffset.y)                > rDelta;
    }

    // if delta was not exceeded, current transitions will not be cancelled and their
    if (exceededDelta)
    {
        // register this transition instance. This will unregister any other transition already registered.
        // build up list of transition instances that will participate
        IFC_RETURN(CTransition::AppendAllTransitionsNoAddRefs(pTarget, trigger, applicableTransitions));

        // first thing that we need is to cancel transitions, so that we get the correct information inside of pStorage.
        // notice how we can make multiple calls to cancellation (each transition for this pTarget will hit this code)
        // but transitions that are being called are _current_ animations, while we are not setting up _future_ transitions.
        // different than the other triggers, we only have to cancel if we are _truly_ setting up a transition (exceeded delta)
        IFC_RETURN(CancelTransitions(pTarget));

        // setup start according to snapshot
        pStorage->m_sizeStart = pStorage->m_currentSize;
        pStorage->m_transformStart.SetDx(pStorage->m_currentOffset.x);
        pStorage->m_transformStart.SetDy(pStorage->m_currentOffset.y);

        pStorage->m_sizeDestination = newDestination.Size();
        pStorage->m_transformDestination.SetDx(newDestination.X);
        pStorage->m_transformDestination.SetDy(newDestination.Y);

        // get opacity
        {
            XFLOAT actualOpacity = pTarget->GetOpacityToRoot();

            if (trigger == DirectUI::TransitionTrigger::Reparent)
            {
                pStorage->m_opacityStart = pStorage->m_opacityCache;
                pStorage->m_opacityDestination = actualOpacity;
            }
            else
            {
                pStorage->m_opacityStart = actualOpacity;
                pStorage->m_opacityDestination = actualOpacity;
            }
        }

        pStorage->m_scaleStart = 1.0f;

        pStorage->RegisterElementForTransitions(pTarget, applicableTransitions, trigger);  // register transitions to be called back later for setting up storyboard
    }
    else if (!HasRunningTransitionAnimations(pTarget))
    {
        // so another layout has come in during this tick that did not exceed our delta. That means that we might have put back
        // the item. If for some reason we already had a layouttransition setup (to run) we should probably not execute it.
        // But on the other hand, if there is a running transition and all you are doing is somehow moving the element (on the target location)
        // by <exceededDelta there is no reason to cancel it. So we only care about not starting a new transition.
        pStorage->UnregisterElementForTransitions(pTarget);
    }


    return S_OK;
}
_Check_return_ HRESULT CTransition::ProcessUnloadTrigger(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage, _In_ xvector<CTransition*>& applicableTransitions)
{
    TransitionTrigger trigger = DirectUI::TransitionTrigger::Unload;
    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
    XRECTF newDestination;
    IFCEXPECT_RETURN(pLayoutManager);

    // transformStart will represent where we are on the screen right now
    // transformDest  will represent where layout things we are being deleted from
    // This is only important when we are in the middle of a transform and are being deleted.
    // You could imagine a transform wanting to fade out plus moving to that location.
    // (AddDeleteThemeTransition)


    if (pStorage && pLayoutManager->GetLayoutCounter() >= pStorage->m_nextGenerationCounter)
    {
        pStorage->m_currentOffset = pStorage->m_nextGenerationOffset;
        pStorage->m_currentSize = pStorage->m_nextGenerationSize;
    }

    newDestination.X = pStorage->m_currentOffset.x;
    newDestination.Y = pStorage->m_currentOffset.y;
    newDestination.Width = pStorage->m_currentSize.width;
    newDestination.Height = pStorage->m_currentSize.height;

    // first thing that we need is to cancel transitions, so that we get the correct information inside of pStorage.
    // notice how we can make multiple calls to cancellation (each transition for this pTarget will hit this code)
    // but transitions that are being called are _current_ animations, while we are not setting up _future_ transitions.
    IFC_RETURN(CancelTransitions(pTarget));

    // setup start according to destination snapshot
    pStorage->m_sizeStart = pStorage->m_currentSize;
    pStorage->m_transformStart.SetDx(pStorage->m_currentOffset.x);
    pStorage->m_transformStart.SetDy(pStorage->m_currentOffset.y);

    pStorage->m_sizeDestination = newDestination.Size();
    pStorage->m_transformDestination.SetDx(newDestination.X);
    pStorage->m_transformDestination.SetDy(newDestination.Y);

    // get opacity
    {
        XFLOAT actualOpacity = pTarget->GetOpacityToRoot();

        pStorage->m_opacityStart = actualOpacity;    // probably animated
        pStorage->m_opacityDestination = 1.0f;
    }

    pStorage->m_scaleStart = 1.0f;

    pStorage->RegisterElementForTransitions(pTarget, applicableTransitions, trigger);  // register transitions to be called back later for setting up storyboard

    return S_OK;
}

//------------------------------------------------------------------------
//  Synopsis: Creates and starts the animation for this transition.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTransition::SetupTransition(
    _In_ CUIElement* pTarget,
    _In_ CTransition* pTransition,
    _In_ XFLOAT staggeredBegin)
{
    HRESULT hr = S_OK;
    CStoryboard** ppStoryboardArray = NULL;
    CStoryboard* pStoryboard = NULL;
    XINT32 cStoryboards = 0;
    LayoutTransitionCompletedData* pData = NULL;
    LayoutTransitionStorage* pStorage = NULL;
    CLayoutTransitionElement* pDestinationElement = NULL;
    CValue Handler;
    CTimeSpan* pBeginTime = NULL;
    auto core = pTarget->GetContext();
    bool coreAllowedTransitionTargetCreation = core->IsAllowingTransitionTargetCreations();
    core->SetAllowTransitionTargetCreation(TRUE);  // we need to allow creation since we are calling Resolve outside of normal BeginStoryboard
    xvector<CLayoutTransitionElement*> addedRenderers;
    TransitionParent transitionParent = DirectUI::TransitionParent::ParentToRoot;

    CDoubleAnimationUsingKeyFrames* pVisibilityAnimation = NULL;
    CDiscreteDoubleKeyFrame* pVisibilityKeyframe = NULL;

    xref_ptr<KeyTimeVO::Wrapper> visibilityKeyTime(KeyTimeVOHelper::Create(core));

    pStorage = pTarget->GetLayoutTransitionStorage();
    IFCEXPECT(pStorage);

    // calls into transition instance to create a storyboard
    IFC(pTransition->CreateStoryboards(pTarget, staggeredBegin, &cStoryboards, &ppStoryboardArray, &transitionParent));

    for (XINT32 index=0; index < cStoryboards; index++)
    {
        if (index == 0)
        {
            pDestinationElement = pStorage->GetPrimaryBrush();
            AddRefInterface(pDestinationElement);   // to match the other codepath
        }
        pStoryboard = ppStoryboardArray[index];

        // manage LTE instance
        if (!pDestinationElement)
        {
            CValue valueTemp;
            XPOINTF start;
            XPOINTF destination;
            destination.x = pStorage->m_transformDestination.GetDx();
            destination.y = pStorage->m_transformDestination.GetDy();
            start.x = pStorage->m_transformStart.GetDx();
            start.y = pStorage->m_transformStart.GetDy();

            // introduce bcb (CLayoutTransitionElement)
            IFC(CLayoutTransitionElement::Create(
                pTarget,
                FALSE /*isAbsolutelyPositioned*/,
                &pDestinationElement
                ));

            IFC(pStorage->RegisterBrushRepresentation(pTarget, pDestinationElement, transitionParent)); // not releasing, will happen in the cleanup
            IFC(addedRenderers.push_back(pDestinationElement)); // do not need to addref, since it is being kept alive by pStorage

            pDestinationElement->SetDestinationOffset(destination);

            if (transitionParent == DirectUI::TransitionParent::ParentToRoot)
            {
                // set the destination elements opacity. This is a tough choice: the transition target is also animating
                // Normally we'll pick up the opacity of the compnode above us, but if we are parented to the transitionroot,
                // that is not going to work, so set it directly
                // This is really more of a problem with PVL: it is using a hardcoded value of 1 as the end destination, where really it should
                // have been the opacityDestination
                valueTemp.SetFloat(pStorage->m_opacityDestination);
                IFC(pDestinationElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, valueTemp));
            }

            // SetValue on the properties that are changed by a transition
            // this causes us to be able to easily stagger an animation by setting a begintime on it
            // NOTE: this means you are going to have to always animate to the destination value, even if you don't want to move it there
            // todo: maybe it would be better to animate to destination if no animation was given.
            auto transitionTarget = pDestinationElement->GetTransitionTarget();
            valueTemp.SetFloat(start.x - destination.x);
            IFC(transitionTarget->m_pxf->SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateX, valueTemp));

            valueTemp.SetFloat(start.y - destination.y);
            IFC(transitionTarget->m_pxf->SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateY, valueTemp));

            valueTemp.SetFloat(pStorage->m_scaleStart);
            IFC(transitionTarget->m_pxf->SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleX, valueTemp));

            valueTemp.SetFloat(pStorage->m_scaleStart);
            IFC(transitionTarget->m_pxf->SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleY, valueTemp));

            if (pStorage->GetTrigger() == DirectUI::TransitionTrigger::Load && staggeredBegin > 0)
            {
                CREATEPARAMETERS cp(core);
                CDOCollection* pVisibilityKeyframes = NULL;

                // start off invisible
                valueTemp.SetFloat(0.0f);
                IFC(pDestinationElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, valueTemp));

                // set to return to original value as soon as animation starts
                IFC(CDoubleAnimationUsingKeyFrames::Create((CDependencyObject**)&pVisibilityAnimation, &cp));
                IFC(CDiscreteDoubleKeyFrame::Create((CDependencyObject**)&pVisibilityKeyframe, &cp));
                valueTemp.SetFloat(transitionParent == DirectUI::TransitionParent::ParentToRoot ? pStorage->m_opacityDestination : 1.0f);
                IFC(pVisibilityKeyframe->SetValueByKnownIndex(KnownPropertyIndex::DoubleKeyFrame_Value, valueTemp));
                pVisibilityKeyframes = pVisibilityAnimation->GetKeyFrameCollection();
                IFC(pVisibilityKeyframes->Append(static_cast<CDependencyObject*>(pVisibilityKeyframe)));
                IFC(pStoryboard->AddChild(pVisibilityAnimation));

                valueTemp.Wrap<valueVO>(visibilityKeyTime);
                IFC(pVisibilityKeyframe->SetValueByKnownIndex(KnownPropertyIndex::DoubleKeyFrame_KeyTime, valueTemp));

                IFC(pVisibilityAnimation->SetTargetObject(pDestinationElement));
                pVisibilityAnimation->SetTargetProperty(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_Opacity));
            }

            valueTemp.SetFloat(pStorage->m_opacityStart);
            IFC(transitionTarget->SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_Opacity, valueTemp));
            // todo: projection, clip, size

        }

        // stagger
        if (pStoryboard->IsPropertyDefaultByIndex(KnownPropertyIndex::Timeline_BeginTime))
        {
            HRESULT recordHr = S_OK;
            CValue valueTemp;
            valueTemp.SetFloat((staggeredBegin));
            CREATEPARAMETERS cpBeginTime(core, valueTemp);
            IFC(CTimeSpan::Create((CDependencyObject**)&pBeginTime, &cpBeginTime));
            RECORDFAILURE(pStoryboard->SetValueByKnownIndex(KnownPropertyIndex::Timeline_BeginTime, pBeginTime));
            ReleaseInterface(pBeginTime);
            IFC(recordHr);
        }

        // rewrite storyboard to target LTE instead of live element
        // only want to retarget timelines that are targetting TransitionTarget, so first need to resolve
        IFC(Jupiter::Animation::TryGenerateDynamicChildrenStoryboardsForChildren(pStoryboard, nullptr));

        // Special case for windowed popups used for MenuFlyout - we need to take the MenuPopupThemeTransition and apply
        // it high enough in the tree to cover the popup contents, shadow, and system backdrop. Detect this case and
        // retarget the Storyboard produced by the MenuPopupThemeTransition. The function that sets up the transition is
        // MenuFlyout::PreparePopupThemeTransitionsAndShadows, and it always targets the grandchild of the popup, so
        // that's what we check for.
        bool isWindowedPopupThemeTransition = false;
        CPopup* popup = nullptr;
        if (pTransition->OfTypeByIndex(KnownTypeIndex::MenuPopupThemeTransition))
        {
            CUIElement* parent = static_cast<CUIElement*>(pTarget->GetParentFollowPopups());
            CUIElement* grandparent = parent ? static_cast<CUIElement*>(parent->GetParentFollowPopups()) : nullptr;
            if (grandparent && grandparent->OfTypeByIndex(KnownTypeIndex::Popup))
            {
                popup = static_cast<CPopup*>(grandparent);
                isWindowedPopupThemeTransition = popup->IsWindowed();
            }
        }

        // If this is a MenuFlyout scenario, retarget the Storyboard to the Popup's secret TransitionTarget. The popup
        // will pick up the animated transform from this TransitionTarget and apply it on a Visual near the root of the
        // popup Visual tree. Otherwise retarget it to the LTE.
        if (isWindowedPopupThemeTransition)
        {
            xref_ptr<CTransitionTarget> popupTransitionTarget = popup->EnsureTransitionTarget();
            IFC(RewriteTransitionTargets(pStoryboard, pTarget, popupTransitionTarget.get(), nullptr));
        }
        else
        {
            IFC(RewriteTransitionTargets(pStoryboard, pTarget, pDestinationElement->GetTransitionTarget(), pDestinationElement));
        }

        // manage storyboard instance - beginning and cancellation
        pData = new LayoutTransitionCompletedData(pTarget);
        IFCEXPECT_ASSERT(!pStoryboard->m_pLayoutTransitionCompletedData);
        Handler.SetInternalHandler(OnTransitionCompleted);
        IFC(pStoryboard->AddEventListener(EventHandle(KnownEventIndex::Timeline_Completed), &Handler, EVENTLISTENER_INTERNAL, &pData->m_EventListenerToken));
        pStoryboard->m_pLayoutTransitionCompletedData = pData;
        pData = NULL;

        // for animation tracking, begin the scenario now with the additional transition information
        pStoryboard->AnimationTrackingBeginScenario(pTransition, pTarget);

        IFC(pStoryboard->BeginPrivate(TRUE /* Top-level Storyboard */));

        ReleaseInterface(pDestinationElement);
    }

    // not register them until the last moment, since cleaning up is hard
    for (XINT32 index=0; index < cStoryboards; index++)
    {
        pStoryboard = ppStoryboardArray[index];
        IFC(pStorage->RegisterStoryboard(pStoryboard));
        ReleaseInterface(pStoryboard);
    }

    // let the world know that we've started a transition
    if (cStoryboards > 0 && core)
    {
        // todo: in final implementation, CTransition should be abstract and thus we
        // should IFC on the fact that we are a custom type
        IFC(FxCallbacks::XcpImports_NotifyLayoutTransitionStart(pTarget));
    }


Cleanup:
    ReleaseInterface(pVisibilityAnimation);
    ReleaseInterface(pVisibilityKeyframe);

    // ultimately will call release on the storyboard
    // every storyboard that was registered was also released.
    if (FAILED(hr) && pStorage)
    {
        for (XINT32 index = 0; index < cStoryboards; index++)
        {
            VERIFYHR(pStorage->CleanupStoryboard(ppStoryboardArray[index]));
        }
    }
    // do not cleanup pStoryboard here, already done through cleanup storyboard.

    delete[] ppStoryboardArray;

    ReleaseInterface(pDestinationElement);
    ReleaseInterface(pBeginTime);
    if (pTarget && core)
    {
        core->SetAllowTransitionTargetCreation(coreAllowedTransitionTargetCreation);
    }

    if (FAILED(hr) && pStorage && pTarget && core)
    {
        // the vector is going to be 2 elements at most, so not caching the end iterator for now.
        for(xvector<CLayoutTransitionElement*>::iterator It = addedRenderers.begin(); It != addedRenderers.end(); ++It)
        {
            VERIFYHR(pStorage->UnregisterBrushRepresentation(pTarget, *It, TRUE));
        }
    }

    // After setup, we would expect to have created a storyboard. If that did not happen (the transition ended up
    // not returning one) and no previous transition setup a storyboard, we should set the trigger back
    // bug: 103076
    //if (!CTransition::HasTransitionAnimations(pTarget) && pStorage)
    //{
    //    pStorage->SetTrigger(DirectUI::TransitionTrigger::NoTrigger);
    //}
    delete pData;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Synopsis: Calls out to the managed peer to create a storyboard.
//  Not batched up.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTransition::CreateStoryboards(_In_ CUIElement* pTarget, _In_ XFLOAT staggeredBegin, _Out_ XINT32* cStoryboards, _Outptr_result_buffer_(*cStoryboards) CStoryboard*** pStoryboardArray, _Out_ TransitionParent* pTransitionParent)
{
    LayoutTransitionStorage* pStorage = NULL;
    XRECTF destination;
    XRECTF start;

    pStorage = pTarget->GetLayoutTransitionStorage();

    start.Size() = pStorage->m_sizeStart;
    start.X = pStorage->m_transformStart.GetDx();
    start.Y = pStorage->m_transformStart.GetDy();

    destination.Size() = pStorage->m_sizeDestination;
    destination.X = pStorage->m_transformDestination.GetDx();
    destination.Y = pStorage->m_transformDestination.GetDy();


    // load, unload and layouttransitions only need to look at the trigger
    // however, if we are hitting this code, it means we need to ask our
    // managed peer to determine what to do.
    if (HasManagedPeer() && GetContext())
    {
        // todo: in final implementation, CTransition should be abstract and thus we
        // should IFC on the fact that we are a custom type
        DirectUI::TransitionTrigger trigger = DirectUI::TransitionTrigger::Layout;
        IFC_RETURN(GetPublicTransitionTrigger(pStorage->GetTrigger(), trigger));

        IFC_RETURN(FxCallbacks::XcpImports_CreateStoryboardsForTransition(
            this,
            pTarget,
            start,
            destination,
            static_cast<XUINT32>(trigger),
            cStoryboards,
            pStoryboardArray,
            pTransitionParent));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Synopsis: determines whether this instance wants to participate in a transition.
//  Calls out to mananged and thus should be used sparingly.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTransition::ParticipateInTransitions(_In_ CUIElement* pTarget, _In_ TransitionTrigger trigger, _Out_ bool* pResult)
{
    *pResult = FALSE;

    // load, unload and layouttransitions only need to look at the trigger
    // however, if we are hitting this code, it means we need to ask our
    // managed peer to determine what to do.
    if (HasManagedPeer() && GetContext())
    {
        // todo: in final implementation, CTransition should be abstract and thus we
        // should IFC on the fact that we are a custom type
        DirectUI::TransitionTrigger publicTrigger = DirectUI::TransitionTrigger::Layout;
        IFC_RETURN(GetPublicTransitionTrigger(trigger, publicTrigger));
        IFC_RETURN(FxCallbacks::XcpImports_ParticipateInTransition(
            this,
            pTarget,
            static_cast<XUINT32>(publicTrigger),
            pResult));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//  Synopsis:   Handles the completed event of our transition.
//              Is used by system (event) to reset element to its final
//              position by removing all effects of a transition on the
//              element. Will cancel the transition.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTransition::OnTransitionCompleted(
    _In_ CDependencyObject* pSender,
    _In_ CEventArgs* pEventArgs
    )
{
    HRESULT hr = S_OK;
    CStoryboard* pStoryboard = NULL;
    CUIElement *pTarget = NULL;
    LayoutTransitionStorage * pStorage = NULL;
    CLayoutManager* pLayoutManager = NULL;
    CLayoutTransitionElement* pPrimaryBrush = NULL;

    pStoryboard = do_pointer_cast<CStoryboard>(pSender);
    IFCEXPECT(pStoryboard);

    // scenario: completed events on multiple animations are being called
    // all animations have indeed already ended and thus this method
    // will call CancelTransitions which will have removed the
    // completedData. That is fine.
    if (!pStoryboard->m_pLayoutTransitionCompletedData)
    {
        goto Cleanup;
    }

    pTarget = pStoryboard->m_pLayoutTransitionCompletedData->m_pTarget;
    ASSERT(pTarget);
    AddRefInterface(pTarget);

    pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
    ASSERT(pLayoutManager);

    pStorage = pTarget->GetLayoutTransitionStorage();
    ASSERT(pStorage);

    pPrimaryBrush = pStorage->GetPrimaryBrush();

    if (!HasRunningTransitionAnimations(pTarget))
    {
        TransitionTrigger trigger = pStorage->GetTrigger();
        // no more running transitions. This was the last one to complete.

        // need to remove all effects of the animation
        // we do this because we never know what has happened to the storyboard. It might be animating
        // to something else than the destination values.
        // need to do this without invalidating layout
        if (pTarget->HasLayoutStorage() && pPrimaryBrush)
        {
            // set these values to the actual values
            // measures and arranges use these values to invalidate the visual for rendering.
//          pTarget->RenderSize.width = pStorage->m_rLayoutTransitionWidth;                                 // currently not supporting animating width/height
//          pTarget->RenderSize.height = pStorage->m_rLayoutTransitionHeight;
            pTarget->VisualOffset = pPrimaryBrush->GetPositionRelativeToParent();
        }

        IFC(CancelTransitions(pTarget));  // note how this call will update the current offset and we are overwriting below.

        // LayoutTransitionStorage may have changed during CancelTransitions call
        pStorage = pTarget->GetLayoutTransitionStorage();

        if (pTarget->HasLayoutStorage() && pStorage)
        {
            // need to completely flush bad storage values used during the LT
            // such as previousconstraint on the children of the element.
            // so: invalidate measure, and set current offset to the completed rect
            // so that we do not accidentally schedule a new LT.
            pStorage->m_currentOffset = pStorage->m_nextGenerationOffset = pStorage->m_arrangeOutput.Point(); ;
            pStorage->m_currentSize = pStorage->m_nextGenerationSize = pStorage->m_arrangeOutput.Size();
            pStorage->m_nextGenerationCounter = pLayoutManager->GetLayoutCounter();
            // finishing an unload should not invalidate measure
            if (trigger != DirectUI::TransitionTrigger::Unload)
            {
                pTarget->FinalRect = pStorage->m_arrangeInput;
                pTarget->InvalidateMeasure();
            }
            pTarget->SetIsLayoutTransitionDirty(FALSE);
        }
    }

Cleanup:
    ReleaseInterface(pTarget);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Synopsis: Cancels all the transitions.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTransition::CancelTransitions(_In_ CUIElement* pTarget)
{
    HRESULT hr = S_OK;
    LayoutTransitionStorage * pStorage = NULL;
    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
    CUIElementCollection * children = NULL;
    TransitionTrigger trigger = DirectUI::TransitionTrigger::NoTrigger;
    CUIElement* pParent = NULL;
    bool bWasUnloading = false;
    CLayoutTransitionElement* pDestinationElement = NULL;

    TraceCancelTransitionsBegin();

    // it is expected that storage does not have a valid pointer to the transition at this point
    // since it is removed in RegisterTransition on layoutmanager before calling this method.
    ASSERT(pTarget);

    // Absolutely positioned transitions are used for drag and drop, and thus will have no
    // layout storage associated with them. Clean them up here.
    IFC(pTarget->RemoveAbsolutelyPositionedLayoutTransitionRenderers());

    pStorage = pTarget->GetLayoutTransitionStorage();
    if (!pStorage)
    {
        goto Cleanup;  // no storage, means per definition no transitions.
    }

    pDestinationElement = pStorage->GetPrimaryBrush();

    trigger = pStorage->GetTrigger();

    // before removing the brush, update the current information in storage
    if (pLayoutManager)
    {
        SetInformationFromAnimation(pTarget, pStorage, pLayoutManager->GetNextLayoutCounter()); // note how removing the storyboard will reset the destinationelements values
    }
    pStorage->m_opacityCache = pDestinationElement ? pDestinationElement->m_eOpacityPrivate : pStorage->m_opacityCache; // in the same vein, only update opacity from destination element if there is one.

    // finally remove the storyboards
    IFC(pStorage->ClearStoryboards());
    pStorage->UnregisterElementForTransitions(pTarget);

    if (pTarget->HasManagedPeer())
    {
        // todo: in final implementation, CTransition should be abstract and thus we
        // should IFC on the fact that we are a custom type
        IFC(FxCallbacks::XcpImports_NotifyLayoutTransitionEnd(pTarget));
    }

    IFC(pStorage->CleanupBrushRepresentations(pTarget));   // cleans up all the brushes

    if (trigger == DirectUI::TransitionTrigger::Unload)
    {
        // unload target
        pParent = do_pointer_cast<CUIElement>(pTarget->GetParentInternal());
        // parent might have been destroyed already. In that case this canceltransition has been triggered through the
        // destructor of its collection.
        if (pParent)
        {
            IFC(DoPointerCast<CUIElementCollection>(children, pParent->GetChildren()));
            IFC(children->RemoveUnloadedElement(pTarget, UC_REFERENCE_ThemeTransition, &bWasUnloading));  // will set the lifecycle to unloaded
        }
    }
Cleanup:
    TraceCancelTransitionsEnd();
    RRETURN(hr);
}

_Check_return_ HRESULT CTransition::ValidateTransitionWasSetup(_In_ CUIElement* pTarget)
{
    LayoutTransitionStorage* pStorage = NULL;
    TransitionTrigger trigger = DirectUI::TransitionTrigger::NoTrigger;
    ASSERT(pTarget);

    pStorage = pTarget->GetLayoutTransitionStorage();
    if (!pStorage)
    {
        return S_OK;
    }

    trigger = pStorage->GetTrigger();
    if (trigger != DirectUI::TransitionTrigger::NoTrigger)
    {
        if (pStorage->m_transitionStoryboards.empty())
        {
            // so we had transitions that when asked for a storyboard, ended up returning nothing
            // we need to fully cancel
            IFC_RETURN(CTransition::CancelTransitions(pTarget));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Helper method
//
//  Synopsis: Gets a transition through the parent.
//  Currently supported:
//   Itemscontrol. ChildTransition of an IC will be templatebound to the
//      panel presenting elements. ChildTransition will not apply to
//      individual elements.
//   ContentControl. ChildTransition of a CC will be templatebound to the
//      cp presenting the content. When switching template the first child
//      of a cc will not inherit the childtransition. User is responsible
//      for setting up a childtransition templatebinding in their own templates.
//   Border. Works as one would expect.
//------------------------------------------------------------------------
_Check_return_ CTransitionCollection* GetInheritedChildTransition(_In_ CUIElement* pTarget)
{
    CTransitionCollection* pChildTransition = NULL;
    CUIElement* pVisualParent = NULL;
    if (!pTarget)
        return NULL;

    pVisualParent = do_pointer_cast<CUIElement>(pTarget->GetParentInternal());
    if (pVisualParent)
    {
        pChildTransition = pVisualParent->GetTransitionsForChildElementNoAddRef(pTarget);
    }

    return pChildTransition;
}

//------------------------------------------------------------------------
//  Synopsis: Gets the transition that should be used.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTransition::AppendAllTransitionsNoAddRefs(_In_ CUIElement* pTarget, _In_ TransitionTrigger trigger, _Inout_ xvector<CTransition*>& applicableTransitions)
{
    CTransitionCollection* pInheritedCollection = NULL;
    if (!pTarget)
        return NULL;

    pInheritedCollection = GetInheritedChildTransition(pTarget);

    auto transitions = pTarget->GetTransitions();
    if (transitions != nullptr)
    {
        IFC_RETURN(transitions->GetApplicableTransitionsNoAddRefs(pTarget, trigger, applicableTransitions));
    }

    // If we have a page and there are no transitions defined, then add the default navigation transition
    if (applicableTransitions.size() == 0 && pTarget->OfTypeByIndex<KnownTypeIndex::Page>())
    {
        CCoreServices* pContext = pTarget->GetContext();
        xaml::IDependencyObject* defaultNavigationTransition = pContext->GetDefaultNavigationTransition();
        if (defaultNavigationTransition == nullptr)
        {
            // It can be expensive to create this transition if the dll isn't already loaded, so we will do a pre check
            // here to see if we really need this.  The page must be under a frame and we must be navigating from a
            // page.
            CContentPresenter* contentPresenter = do_pointer_cast<CContentPresenter>(pTarget->GetParentInternal(false));
            if (contentPresenter != nullptr)
            {
                CFrame* frame = do_pointer_cast<CFrame>(contentPresenter->GetParentInternal(false));
                if (frame != nullptr)
                {
                    // Obviously if we have entries on the backstack then we are navigating (as opposed to being
                    // the initial load, however, we also hit this code as we unload, but before it enters the
                    // backstack.  This is different than when the transition is played.  By then the entry is in
                    // the backstack.  So on our first transtion, at the time of unload of the "from" page, there
                    // is no entry in the back stack.  Now, even though the transition on the "from" page is never
                    // used (we use the navigating away from the "to" page instead), it never the less must have
                    // a transition on it to even hit the transition code (where it will use the "to" page transtions).
                    // So, if we are unloading, a page, we will also assume we are navigating.
                    if (frame->m_backStackDepth > 0 || trigger == DirectUI::TransitionTrigger::Unload)
                    {
                        wrl::ComPtr<xaml::IDependencyObject> newTransition;
                        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.Media.Animation.NavigationThemeTransition").Get(), &newTransition));
                        defaultNavigationTransition = newTransition.Get();
                        pContext->SetDefaultNavigationTransition(defaultNavigationTransition);
                    }
                }
            }
        }
        if (defaultNavigationTransition != nullptr)
        {
            DependencyObject *pSource = static_cast<DependencyObject*>(defaultNavigationTransition);
            CTransition* transition = static_cast<CTransition*>(pSource->GetHandle());

            bool participate = false;
            IFC_RETURN(transition->ParticipateInTransitions(pTarget, trigger, &participate));
            if (participate)
            {
                IFC_RETURN(applicableTransitions.push_back(transition));
            }
        }
    }

    if (pInheritedCollection)
    {
        IFC_RETURN(pInheritedCollection->GetApplicableTransitionsNoAddRefs(pTarget, trigger, applicableTransitions));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Synopsis: TRUE if this element is currently transitioning
//------------------------------------------------------------------------
bool CTransition::HasTransitionAnimations(_In_ CUIElement* pTarget)
{
    return pTarget && pTarget->HasLayoutTransitionStorage() && pTarget->GetLayoutTransitionStorage()->m_transitionStoryboards.size() > 0;
}

bool CTransition::HasTransitionAnimations(_In_ CUIElement* pTarget, _In_ TransitionTrigger trigger)
{
    return pTarget && pTarget->HasLayoutTransitionStorage() && pTarget->GetLayoutTransitionStorage()->GetTrigger() == trigger && pTarget->GetLayoutTransitionStorage()->m_transitionStoryboards.size() > 0;
}
bool CTransition::HasRunningTransitionAnimations(_In_ CUIElement* pTarget)
{
    LayoutTransitionStorage* pStorage = pTarget->GetLayoutTransitionStorage();
    if (pStorage)
    {
        for(xvector<CStoryboard*>::iterator It = pStorage->m_transitionStoryboards.begin(); It != pStorage->m_transitionStoryboards.end(); ++It)
        {
            CStoryboard* pSB = *It;
            if(pSB->IsInActiveState())
            {
                return true;
            }
        }
    }
    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis: TRUE if this element has a transition associated with it.
//
//------------------------------------------------------------------------
bool CTransition::HasActiveTransition(_In_ CUIElement* pTarget)
{
    return pTarget && pTarget->HasLayoutTransitionStorage() && pTarget->GetLayoutTransitionStorage()->GetTrigger() != DirectUI::TransitionTrigger::NoTrigger;
}

bool CTransition::HasActiveTransition(_In_ CUIElement* pTarget, _In_ TransitionTrigger trigger)
{
    return pTarget && pTarget->HasLayoutTransitionStorage() && pTarget->GetLayoutTransitionStorage()->GetTrigger() == trigger;
}

//------------------------------------------------------------------------
//  Synopsis: TRUE if this element could perform a transition ever.
//  Defined as having either a transition collection itself or inherit one from parent or
//  if it is a page (we will default the transition for a page if one isn't specified)
//------------------------------------------------------------------------
bool CTransition::HasPossibleTransition(_In_ CUIElement* pTarget)
{
    if (!pTarget)
    {
        return false;
    }

    return (pTarget->GetTransitions() != nullptr) ||
        GetInheritedChildTransition(pTarget) ||
        pTarget->OfTypeByIndex<KnownTypeIndex::Page>();
}



//------------------------------------------------------------------------
// Synopsis: Updates the current and next generation offset and size.
// This information is used as input to the layoutchanged method
// that will setup a transition with these values as starting args.
//
// Both information sets are updated at the same time since a transition
// element always has the correct current information (as opposed to layout).
//------------------------------------------------------------------------
void CTransition::SetInformationFromAnimation(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage, _In_ XUINT32 nextGenerationCounter)
{
    CLayoutTransitionElement* pPrimaryBrush = pStorage->GetPrimaryBrush();
    if (pPrimaryBrush)
    {
        // have a bcb, so use that instead of the m_rLayoutTransition values
        // for the time being, we are not animating those (we rewrote storyboard
        // to only animate the destionation elements)
        // todo: we should actually grab the high-def version of this information
        // from the HW thread.
        pStorage->m_currentOffset = pStorage->m_nextGenerationOffset = pPrimaryBrush->GetPositionRelativeToParent();

//      pStorage->m_currentSize.width = pStorage->m_nextGenerationSize.width = pStorage->m_rLayoutTransitionWidth;          // currently not supporting w/h
//      pStorage->m_currentSize.height = pStorage->m_nextGenerationSize.height = pStorage->m_rLayoutTransitionHeight;

        // the information is updated to the best of our current knowledge. Use this in the next
        // layoutcycle.
        pStorage->m_nextGenerationCounter = nextGenerationCounter;
    }
}

//---------------------------------------------------------------------------
// Synopsis: Updates the next generation information. This information will
// transition into the current information set on the next layout cycle.
//
// Note how information from the destination element trumps information from layout.
//---------------------------------------------------------------------------
void CTransition::SetNextGenerationInformationFromLayout(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage, _In_ XUINT32 nextGenerationCounter)
{
    CLayoutTransitionElement* pPrimaryBrush = pStorage->GetPrimaryBrush();
    if (pPrimaryBrush)
    {
        // have a bcb, so use that instead of the m_rLayoutTransition values
        // for the time being, we are not animating those (we rewrote storyboard
        // to only animate the destionation elements)
        // todo: we should actually grab the high-def version of this information
        // from the HW thread.
        pStorage->m_nextGenerationOffset = pPrimaryBrush->GetPositionRelativeToParent();

//      pStorage->m_nextGenerationSize.width =  pStorage->m_rLayoutTransitionWidth;          // currently not supporting w/h
//      pStorage->m_nextGenerationSize.height = pStorage->m_rLayoutTransitionHeight;
    }
    else
    {
        // not in a transition, so the offsets that are set by layout are correct
        pStorage->m_nextGenerationOffset.x = pTarget->GetActualOffsetX();
        pStorage->m_nextGenerationOffset.y = pTarget->GetActualOffsetY();

        // it seems counterintuitive not to update size always, but when an element has left the tree
        // and shutdown, we actually remove layoutstorage, so the sizes would return 0
        pStorage->m_nextGenerationSize.width = pTarget->GetActualWidth();
        pStorage->m_nextGenerationSize.height = pTarget->GetActualHeight();
    }
    // information-set has been updated, use it in the next layoutcyle.
    pStorage->m_nextGenerationCounter = nextGenerationCounter;
}


//------------------------------------------------------------------------
//  Synopsis: Returns the constraint that should be used during layout.
//            Since the transition takes care of the full layoutslot
//            which includes margin considerations, we should take care
//            in returning only the animated value if it is actually targetted.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTransition::GetLayoutSlotDuringTransition(_In_ CUIElement* pTarget, _Inout_ XRECTF* pLayoutSlot)
{
    LayoutTransitionStorage* pStorage = NULL;

    IFCEXPECT_RETURN(pTarget);
    IFCEXPECT_RETURN(pLayoutSlot);

    pStorage = pTarget->GetLayoutTransitionStorage();
    IFCEXPECT_RETURN(pStorage);

    if (CTransition::HasTransitionAnimations(pTarget))
    {
//      XPOINTF location = pStorage->m_pDestinationElement->GetPositionRelativeToParent();
//      pLayoutSlot->X = location.x;
//      pLayoutSlot->Y = location.y;
//      pLayoutSlot->Width = pStorage->m_rLayoutTransitionWidth;
//      pLayoutSlot->Height = pStorage->m_rLayoutTransitionHeight;

        ASSERT(FALSE);
        IFC_RETURN(E_NOT_SUPPORTED);    // we no longer support this walk. It will come back in the future
    }
    else
    {
        *pLayoutSlot = pStorage->m_arrangeInput;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Synopsis: TRUE if this element does not disallow running transitions on its
//            subtree.
//------------------------------------------------------------------------
bool CTransition::GetAllowsTransitionsToRun(_In_ CUIElement* pTarget)
{
    CPanel* pVisualParent = do_pointer_cast<CPanel>(pTarget->GetParentInternal(false));
    return !pVisualParent || !pVisualParent->GetIsIgnoringTransitions();
}
