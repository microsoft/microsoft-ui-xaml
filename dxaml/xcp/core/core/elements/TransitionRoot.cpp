// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Used as a root for an element to be showed on top of everything
// while a transition is going on.

#include "precomp.h"
#include "HitTestParams.h"

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Override to return the children of the transition root in the order
//      they were added, instead of respecting z-order or projections set
//      on the children.
//
//-------------------------------------------------------------------------
void
CTransitionRoot::GetChildrenInRenderOrder(
    _Outptr_opt_result_buffer_(*puiChildCount) CUIElement ***pppUIElements,
    _Out_ XUINT32 *puiChildCount
    )
{
    *pppUIElements = NULL;
    *puiChildCount = 0;

    CUIElementCollection *pChildren = GetChildren();
    if (pChildren)
    {
        auto& collection = pChildren->GetCollection();
        if (collection.size() > 0)
        {
            *pppUIElements = doarray_to_elementarray_cast(collection.data());
            *puiChildCount = static_cast<XUINT32>(collection.size());
        }
    }
}

void CTransitionRoot::ReEvaluateRequiresCompNode()
{
    // 19H1 Bug #19746384:  If an LTE receives a prepend clip, and also has a TransformParent that carries a transform,
    // this produces an incorrect clip, due to the TransformParent transforming the clip when it should not.
    // The fix is to make TransitionRoot generate a CompNode, optimized to only create one if it has children.
    bool requiresCompNode = false;

    CUIElementCollection *children = GetChildren();
    if (children != nullptr)
    {
        auto& collection = children->GetCollection();
        if (collection.size() > 0)
        {
            requiresCompNode = true;
        }
    }

    if (requiresCompNode)
    {
        if (!IsTransitionRootWithChildren())
        {
            SetRequiresComposition(CompositionRequirement::TransitionRootWithChildren, IndependentAnimationType::None);
        }
    }
    else
    {
        if (IsTransitionRootWithChildren())
        {
            UnsetRequiresComposition(CompositionRequirement::TransitionRootWithChildren, IndependentAnimationType::None);
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the combined outer bounds of all children.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTransitionRoot::GenerateChildOuterBounds(
    _In_opt_ HitTestParams *hitTestParams,
    _Out_ XRECTF_RB* pBounds
    )
{
    EmptyRectF(pBounds);

    XUINT32 childCount = 0;
    CUIElement** ppUIElements = NULL;

    GetChildrenInRenderOrder(
        &ppUIElements,
        &childCount
        );

    for (XUINT32 i = 0; i < childCount; ++i)
    {
        XRECTF_RB childBounds = { };
        CLayoutTransitionElement *pLTE = static_cast<CLayoutTransitionElement*>(ppUIElements[i]);
        CUIElement *pTarget = pLTE->GetTargetElement();

        // TODO: HWPC: Refactor this into a shared base class with CPopupRoot? Very similar.
        if (pTarget->IsVisible() && pTarget->AreAllAncestorsVisible())
        {
            IFC_RETURN(pLTE->GetOuterBounds(hitTestParams, &childBounds)); // TODO: Enable Transform3D.

            // Transform bounds through the target's parent chain to get the bounds in the local space of the
            // TransitionRoot's parent.
            IFC_RETURN(TransformInnerToOuterChain(
                pTarget->GetUIElementAdjustedParentInternal(FALSE),
                GetUIElementAdjustedParentInternal(FALSE),
                &childBounds,
                &childBounds));
        }

        UnionRectF(pBounds, &childBounds);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTransitionRoot::BoundsTestChildren(
    _In_ const XPOINTF& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    RRETURN(BoundsTestChildrenImpl(target, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, pResult));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given polygon.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTransitionRoot::BoundsTestChildren(
    _In_ const HitTestPolygon& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    RRETURN(BoundsTestChildrenImpl(target, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, pResult));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given point/polygon.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CTransitionRoot::BoundsTestChildrenImpl(
    _In_ const HitType& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    BoundsWalkHitResult hitResult = BoundsWalkHitResult::Continue;
    BoundsWalkHitResult childHitResult = BoundsWalkHitResult::Continue;

    XUINT32 childCount = 0;
    CUIElement** ppUIElements = NULL;

    GetChildrenInRenderOrder(
        &ppUIElements,
        &childCount
        );

    // Test bounds in reverse order (front to back).
    for (XUINT32 i = childCount; i > 0 && flags_enum::is_set(childHitResult, BoundsWalkHitResult::Continue); --i)
    {
        CLayoutTransitionElement *pLTE = static_cast<CLayoutTransitionElement*>(ppUIElements[i-1]);
        CUIElement *pTarget = pLTE->GetTargetElement();

        if (pTarget->IsVisible() && pTarget->AreAllAncestorsVisible())
        {
            HitType lteTarget;
            bool transformSucceeded = false;

            //
            // Normally, the transition root is rooted to an ancestor of the LTE's target:
            //
            //      <CommonParent>
            //          <TransitionRoot this>
            //              <LayoutTransitionElement Target="target" />
            //          </TransitionRoot>
            //          <Element1>
            //              <Element2>
            //                  <Element3>
            //                      <TargetElement target />
            //                  </Element3>
            //              </Element2>
            //          </Element1>
            //      </CommonParent>
            //
            // In this example, the transition root is rooted to <CommonParent>, which is an ancestor of the TargetElement.
            // Note that XAML always places an LTE above its target in the tree. That is, an LTE will always target towards the
            // leaf of the tree. XAML also nearly always places the TransitionRoot on the ancestor chain of the LTE target's
            // parent (Element3). Here, the ancestor chain is Element2-Element1-CommonParent, and the TransitionRoot is placed
            // inside CommonParent.
            //
            // Rendering via LTE means that the properties on the target itself (TargetElement) are ignored. When hit testing the
            // LTE, we need to transform the point down to the coordinate space of the target's parent (Element3), then proceed
            // with the hit testing walk. Currently, we're in the coordinate space of TransitionRoot, so we need to transform up
            // to CommonParent, then down to Element3. Since the TransitionRoot is guaranteed to have identity as a transform,
            // this problem is equivalent as transforming directly from CommonParent down to Element3. Since the TransitionRoot
            // is placed in the ancestor chain, we can just walk the path from the target's parent (Element3) up to the TransitionRoot's
            // parent (CommonParent).
            //
            // When Popups are involved, the tree becomes much more complicated:
            //
            //      <RootVisual>
            //          <PopupRoot>
            //              <TransitionRoot this>
            //                  <LayoutTransitionElement Target="popupTarget" />
            //              </TransitionRoot>
            //              <TargetElement popupTarget />
            //          </PopupRoot>
            //          <Element1>
            //              <Element2>
            //                  <Element3>
            //                      <Popup target="popupTarget" />
            //                  </Element3>
            //              </Element2>
            //          </Element1>
            //      </RootVisual>
            //
            // This situation can come up during readerboard animations of a popup. The LTEs for the readerboard are attached
            // under the popup root.
            //
            // There are a few tricky things in this scenario:
            //   1. The LTE targets the popup's target, not the popup.
            //   2. The LTE target's parent is the logical parent. In this case, it's Popup, not PopupRoot.
            //   3. The TransitionRoot is not placed on the ancestor chain of the LTE target's parent. Here, the target's parent
            //      is Popup, and its ancestor chain is Element3-Element2-Element1-RootVisual. The TransitionRoot is placed in the
            //      PopupRoot, which isn't in this chain.
            //
            // In order to correctly hit test here, we again need to transform the point down to the coordinate space of the target's
            // parent (Popup). However, since the TransitionRoot is placed differently, it's no longer possible to walk from the target's
            // parent (Popup) up to the TransitionRoot's parent (PopupRoot) - they are on separate branches of the tree. Since we can't
            // optimize, we instead do a walk up and a walk back down. First take the coordinate from PopupRoot up to RootVisual, then
            // take it from RootVisual down to Popup. The hit testing walk can proceed normally after that.
            //

            HitTestParams newParams(hitTestParams);

            if (pLTE->IsAbsolutelyPositioned())
            {
                // TODO: HitTest: hit test these things, if we care about supporting them.
                // They mess up bounds checking because they can be attached anywhere and skip over the transforms of a bunch of
                // elements between their targets and the closest comp node.
                transformSucceeded = false;
            }
            else
            {
                // We don't worry about the popup case above. We don't need to transform the local space point up to the root, because the
                // hit test params already has the world space point saved on it. We can just transform that point to the coordinate space
                // of the target element directly.
                transformSucceeded = pTarget->PrepareHitTestParamsStartingHere(newParams, lteTarget);
            }

            if (transformSucceeded)
            {
                IFC_RETURN(pLTE->BoundsTestInternal(lteTarget, pCallback, &newParams, canHitDisabledElements, canHitInvisibleElements, &childHitResult));

                // If any child wanted to include its parent chain, copy the flag.
                if (flags_enum::is_set(childHitResult, BoundsWalkHitResult::IncludeParents))
                {
                    hitResult = flags_enum::set(hitResult, BoundsWalkHitResult::IncludeParents);
                }
            }
        }
    }

    // If the child element wanted the bounds walk to stop, remove the continue flag
    // from the result.
    if (!flags_enum::is_set(childHitResult, BoundsWalkHitResult::Continue))
    {
        hitResult = flags_enum::unset(hitResult, BoundsWalkHitResult::Continue);
    }

    if (pResult != NULL)
    {
        *pResult = hitResult;
    }

    return S_OK;
}
