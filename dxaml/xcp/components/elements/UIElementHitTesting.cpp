// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "uielement.h"
#include "HitTestParams.h"
#include "Projection.h"
#include "RectangleGeometry.h"
#include "XamlIslandRoot.h"
#include "stack_vector.h"
#include "RootScale.h"

template <typename HitType> bool CUIElement::CollectTransformsAndTransformToInner(HitTestParams& hitTestParams, HitType& transformedTarget)
{
    CMILMatrix transformMatrix;
    const bool localTransformIsIdentity = GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, &transformMatrix);
    const CMILMatrix4x4 transform3DMatrix = GetHitTestingTransform3DMatrix(true /* includePropertiesSetInComposition */);
    const bool has3D = !transform3DMatrix.Is2D();
    const CMILMatrix4x4 projectionMatrix = GetProjectionMatrix4x4();

    if (localTransformIsIdentity && transform3DMatrix.IsIdentity() && projectionMatrix.IsIdentity())
    {
        // Everything is identity. No need to do anything.
        return true;
    }
    else
    {
        //
        // Update cumulative transform
        //
        // If this element is on a 3D branch, or if the element itself has 3D, then prepend the element's combined local
        // transform (both 2D and 3D) to the existing cumulative transform. The cumulative transform will be used once we
        // hit an element with 3D, to transform the global hit test point/rect into that element's local plane.
        //
        ASSERT(has3D == Has3DDepth());
        if (Has3DDepthOnSelfOrSubtree())
        {
            if (!localTransformIsIdentity)
            {
                hitTestParams.UpdateCombinedTransformMatrix(&transformMatrix);
            }

            // Populate the builder if there is a 3D transform.
            // TODO: HitTest: HasTransform3DForHitTesting isn't needed anymore
            if (has3D && !transform3DMatrix.IsIdentity())
            {
                // Populate the builder with any 3D transforms.
                hitTestParams.UpdateCombinedTransformMatrix(&transform3DMatrix);
            }

            // Populate builder with projection
            if (!projectionMatrix.IsIdentity())
            {
                hitTestParams.UpdateCombinedTransformMatrix(&projectionMatrix);
            }
        }

        //
        // Transform the incoming point to local space, if this element is entirely 2D. Otherwise, we have to use the
        // world space point saved in the hitTestParams and bring it into this element's local space.
        //
        if (has3D)
        {
            // 3D path - transform the world space point from the HitTestParams down to this element.
            return TransformWorldToLocal3D(hitTestParams, transformedTarget);
        }
        else
        {
            // 2D path - transform the parent space point.
            bool continueHitTest = true;

            if (!localTransformIsIdentity)
            {
                if (transformMatrix.Invert())
                {
                    TransformTargetThroughMatrix(transformMatrix, transformedTarget);
                }
                else
                {
                    // 2D transform isn't invertible.
                    continueHitTest = false;
                }
            }

            // There's no 3D depth, but that doesn't mean there's no 4x4 transform to consider for hit testing.
            // We might have one that happens to be purely 2D. Account for it here.
            if (continueHitTest && !transform3DMatrix.IsIdentity())
            {
                ASSERT(transform3DMatrix.Is2D());

                CMILMatrix transformAs2D = transform3DMatrix.Get2DRepresentation();
                ASSERT(!transformAs2D.IsIdentity());

                if (transformAs2D.Invert())
                {
                    TransformTargetThroughMatrix(transformAs2D, transformedTarget);
                }
                else
                {
                    // 3D transform is purely 2D, but isn't invertible.
                    continueHitTest = false;
                }
            }

            if (!projectionMatrix.IsIdentity())
            {
                continueHitTest = TransformTargetThroughMatrix4x4(projectionMatrix, transformedTarget);
            }

            return continueHitTest;
        }
    }
}

template bool CUIElement::CollectTransformsAndTransformToInner<XPOINTF>(HitTestParams& hitTestParams, XPOINTF& transformedPoint);
template bool CUIElement::CollectTransformsAndTransformToInner<HitTestPolygon>(HitTestParams& hitTestParams, HitTestPolygon& transformedPolygon);

void CUIElement::FillAncestorChainForTransformToRoot(Jupiter::stack_vector<CUIElement*, 32>& ancestorChain, bool includeThisElement, bool useTargetInformation)
{
    CUIElement* element;
    if (includeThisElement)
    {
        element = this;
    }
    else
    {
        element = GetUIElementAdjustedParentInternal(FALSE);
    }

    // Note: GetUIElementAdjustedParentInternal knows how to follow Popup.Child back to the Popup.
    for (;
        element != nullptr;
        element = element->GetUIElementAdjustedParentInternal(FALSE))
    {
        // Find if there's an LTE targeting the element. If there is, put the LTE on the ancestor chain. Otherwise use the element
        // itself. Note that we continue walking up the tree using the element itself, even if we found an LTE. When rendering,
        // the LTE will replace only its target's local transforms with its own. It still inherits transforms from its target's
        // ancestor chain.
        CUIElement* firstLTETargetingElement = element->GetFirstLTETargetingThis();
        if (firstLTETargetingElement != nullptr && !useTargetInformation)
        {
            ancestorChain.m_vector.push_back(firstLTETargetingElement);
        }
        else
        {
            ancestorChain.m_vector.push_back(element);
        }
    }
}

// FindElementsInHostCoordinates can restrict the hit test to start at a particular subtree instead of starting at the root,
// so we need to collect 3D transforms along 3D branches and transform the hit test target down to that subtree. We then start
// a normal hit test walk on that subtree. Popup hit testing need to do the same thing for a hit test on a popup in the tree.
template <typename HitType> bool CUIElement::PrepareHitTestParamsStartingHere(HitTestParams& hitTestParams, HitType& transformedTarget)
{
    // Build a chain of elements from the root down to this element, then transform the points down that chain.
    // We can use raw pointers. The ancestor chain isn't going to get released in the middle of this method.
    // Use a stack_vector to prevent repeated allocations of this transient vector.
    //
    // Start at the ancestor of this element. The hit test is expected to start at this element after we return from this method,
    // which will transform the target through this element's transforms first thing. In the case of LTE hit testing, the hit test
    // will start at the LTE and skip the transforms on this element.
    Jupiter::stack_vector<CUIElement*, 32> ancestorChain;
    FillAncestorChainForTransformToRoot(ancestorChain, false /* includeThisElement */, false /* useTargetInformation */);

    bool wasTransformed = true;

    // Throw away whatever transforms that have already been collected. We're about to walk up to the root and collect again,
    // and we don't want to double-count anything (e.g. the zoom scale on the root visual). Also reset the hit test target since
    // we're about to start over again.
    hitTestParams.combinedTransformMatrix.SetToIdentity();
    hitTestParams.GetWorldSpaceHitTarget(transformedTarget);

    for (auto reverse = ancestorChain.m_vector.rbegin();
        wasTransformed && reverse != ancestorChain.m_vector.rend();
        ++reverse)
    {
        wasTransformed = (*reverse)->CollectTransformsAndTransformToInner(hitTestParams, transformedTarget);
    }

    return wasTransformed;
}

template bool CUIElement::PrepareHitTestParamsStartingHere<XPOINTF>(HitTestParams& hitTestParams, XPOINTF& transformedPoint);
template bool CUIElement::PrepareHitTestParamsStartingHere<HitTestPolygon>(HitTestParams& hitTestParams, HitTestPolygon& transformedPolygon);

bool CUIElement::TransformToOuter2D(const HitTestParams* hitTestParams, XRECTF_RB& bounds)
{
    CMILMatrix transformMatrix;
    const bool localTransformIsIdentity = GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, &transformMatrix);
    const CMILMatrix4x4 transform3DMatrix = GetHitTestingTransform3DMatrix(true /* includePropertiesSetInComposition */);
    const bool has3D = !transform3DMatrix.Is2D();
    const CMILMatrix4x4 projectionMatrix = GetProjectionMatrix4x4();

    // This method isn't meant to be used if there's a 3D transform on this element. In that case, we should gather the
    // transforms up to the root and transform the bounds directly to world space.
    ASSERT(!has3D);

    bool transformSucceeded = true;

    IFCFAILFAST(ApplyUIEClipToBounds(&bounds));

    if (!ShouldApplyLayoutClipAsAncestorClip())
    {
        IFCFAILFAST(ApplyLayoutClipToBounds(&bounds));
    }

    if (localTransformIsIdentity && transform3DMatrix.IsIdentity() && projectionMatrix.IsIdentity())
    {
        // Everything is identity. No need to do any transforming.
    }
    else
    {
        XPOINTF points[] = {
            {bounds.left, bounds.top},
            {bounds.right, bounds.top},
            {bounds.right, bounds.bottom},
            {bounds.left, bounds.bottom} };

        if (transformSucceeded && !projectionMatrix.IsIdentity())
        {
            transformSucceeded = projectionMatrix.Transform2DPoints_DivideW(points, points);
        }

        if (transformSucceeded && !transform3DMatrix.IsIdentity())
        {
            CMILMatrix transform2DMatrix = transform3DMatrix.Get2DRepresentation();
            transform2DMatrix.Transform(points);    // This always succeeds.
        }

        if (transformSucceeded && !localTransformIsIdentity)
        {
            transformMatrix.Transform(points);      // This always succeeds.
        }

        bounds = BoundPoints_RB(points);
    }

    if (transformSucceeded && ShouldApplyLayoutClipAsAncestorClip())
    {
        IFCFAILFAST(ApplyLayoutClipToBounds(&bounds));
    }

    return transformSucceeded;
}

void CUIElement::TransformToOuter(HitTestPolygon& polygon, TransformRetrievalOptions transformRetrievalOptions, bool ignoreClipping)
{
    CMILMatrix transformMatrix;
    const bool localTransformIsIdentity = GetLocalTransform(transformRetrievalOptions, &transformMatrix);
    const CMILMatrix4x4 transform3DMatrix = GetHitTestingTransform3DMatrix(true /* includePropertiesSetInComposition */);
    const CMILMatrix4x4 projectionMatrix = GetProjectionMatrix4x4();

    if (!ignoreClipping)
    {
        const auto& clip = GetHandOffVisualClipOrElementClip();
        if (clip)
        {
            ASSERT(clip->OfTypeByIndex<KnownTypeIndex::RectangleGeometry>());
            CRectangleGeometry* rectangleGeometry = static_cast<CRectangleGeometry*>(clip.get());

            XRECTF_RB clipBounds = { };
            IFCFAILFAST(rectangleGeometry->GetBounds(&clipBounds));
            polygon.ClipToRect(ToXRectF(clipBounds));
        }

        if (HasLayoutClip() && !ShouldApplyLayoutClipAsAncestorClip())
        {
            polygon.ClipToRect(LayoutClipGeometry->m_rc);
        }
    }

    if (localTransformIsIdentity && transform3DMatrix.IsIdentity() && projectionMatrix.IsIdentity())
    {
        // Everything is identity. No need to do anything.
    }
    else
    {
        if (!projectionMatrix.IsIdentity())
        {
            polygon.Transform(projectionMatrix);
        }

        if (!transform3DMatrix.IsIdentity())
        {
            polygon.Transform(transform3DMatrix);
        }

        if (!localTransformIsIdentity)
        {
            polygon.Transform(transformMatrix);
        }
    }

    if (!ignoreClipping && HasLayoutClip() && ShouldApplyLayoutClipAsAncestorClip())
    {
        polygon.ClipToRect(LayoutClipGeometry->m_rc);
    }
}

bool CUIElement::TransformWorldToLocal3D(const HitTestParams& hitTestParams, XPOINTF& localSpacePoint) const
{
    return hitTestParams.GetWorldSpacePointInLocalSpace(localSpacePoint);
}

bool CUIElement::TransformWorldToLocal3D(const HitTestParams& hitTestParams, HitTestPolygon& localSpacePolygon) const
{
    return hitTestParams.GetWorldSpaceRectInLocalSpace(localSpacePolygon);
}

void CUIElement::TransformTargetThroughMatrix(const CMILMatrix& matrix, XPOINTF& point) const
{
    matrix.Transform(point, point);
}

void CUIElement::TransformTargetThroughMatrix(const CMILMatrix& matrix, HitTestPolygon& polygon) const
{
    polygon.Transform(matrix);
    // TODO: HitTest: Update usage of HitTestPolygon. Have HitTestPolygon copy itself whenever transformed by anything non-identity.
}

CMILMatrix4x4 CUIElement::GetProjectionMatrix4x4() const
{
    // TODO: HitTest: See CUIElement::SetHandOffVisualTransformMatrix
    // We could be using the hit test stash for this, but for now we skip the stash when projections are involved.

//    if (CanUseHandOffVisualTransformGroupForLocalTransform())
//    {
        //
        // Special case: if a hand off visual is involved then we're listening to its transform changes. In ContainerVisuals
        // mode, the hand off visual's transform includes all transforms on the UIElement, including its Projection. That
        // combined transform will have been accounted for already. If it's 2D, then it will have been returned as part of
        // GetLocalTransform. If it was 3D, then it will have been returned as part of GetHitTestingTransform3DMatrix. There's
        // nothing for us to return here.
        //
//        return CMILMatrix4x4(true);
//    }
//    else
    {
        return HasActiveProjection() ? GetProjection()->GetOverallMatrix() : CMILMatrix4x4(true);
    }
}

bool CUIElement::TransformTargetThroughMatrix4x4(const CMILMatrix4x4& matrix, XPOINTF& point) const
{
    const gsl::span<XPOINTF> pointSpan(&point, 1);
    return matrix.TransformWorldToLocalWithInterpolation(pointSpan, pointSpan);
}

bool CUIElement::TransformTargetThroughMatrix4x4(const CMILMatrix4x4& matrix, HitTestPolygon& polygon) const
{
    return polygon.TransformWorldToLocalWithInterpolation(matrix);
}

HRESULT CUIElement::TransformToWorldSpace(
    _In_ const XRECTF_RB* localSpaceRect,
    _Out_ XRECTF_RB* worldSpaceRect,
    bool ignoreClipping,
    const bool ignoreClippingOnScrollContentPresenters,
    bool useTargetInformation)
{
    // LayoutTransitionElements can cause us to skip clipping for part of the tree, but if we're skipping clipping
    // to begin with, then there's no need to do that bookkeeping.
    bool originalIgnoreClipping = ignoreClipping;
    CUIElement* sharedLTEAncestor = nullptr;

    // We want to walk a rect from local space up to world space. Represent it as 4 points, walk the points up to the
    // root, then bound the points to get the world space rect. We can keep the points as 2D points until we encounter
    // the first element with 3D, at which point we transform the points into 3D and keep them 3D until the end. If we
    // have 3D points at the end, we do a w divide at the end to get the points back to 2D before bounding them.
    HitTestPolygon polygon;
    polygon.SetRect(ToXRectF(*localSpaceRect));

    // TODO: HitTest: Clips on 3D points can't be applied as we go. Transform them to world space, then do an intersection of all polygons
    //  in world space at the end, then bound the final result. Currently Xaml doesn't do this and nobody seems to mind.
    // TODO: HitTest: Transition clips? Looks like we didn't include them before either, but the downwards hit testing walk looks at them.

    TransformRetrievalOptions transformRetrievalOptions = TransformRetrievalOptions::IncludePropertiesSetInComposition;
    if (useTargetInformation)
    {
        // This flag is used by connected animations to request the final value of active manipulations, rather than the current value.
        transformRetrievalOptions = transformRetrievalOptions | TransformRetrievalOptions::UseManipulationTargets;
    }

    // Build a chain of elements from this up to the root, then transform the points up that chain.
    // We can use raw pointers. The ancestor chain isn't going to get released in the middle of this method.
    // Use stack_vector to prevent repeated allocations of this transient vector.
    Jupiter::stack_vector<CUIElement*, 32> ancestorChain;
    FillAncestorChainForTransformToRoot(ancestorChain, true /* includeThisElement */, useTargetInformation);

    for (CUIElement* element : ancestorChain.m_vector)
    {
        if (!originalIgnoreClipping && element == sharedLTEAncestor)
        {
            // Once we get to the shared LTE ancestor, we can start clipping again.
            // See big comment below.
            ignoreClipping = false;
            sharedLTEAncestor = nullptr;
        }

        // We optionally skip the clip set on CScrollContentPresenters (the ScrollViewer viewport clip). This
        // clip causes problems for the Windows 10X shell's UIA. The shell needs to return automation bounds for
        // elements that are scrolled off-screen so it can scroll them back into view, and these viewport clips
        // are clipping off-screen elements to zero bounds. We don't want to ignore all UIElement clips, because
        // some are intentionally applied and should not be ignored.
        bool isScrollContentPresenter = false;
        if (ignoreClippingOnScrollContentPresenters
            && element->OfTypeByIndex<KnownTypeIndex::ScrollContentPresenter>())
        {
            isScrollContentPresenter = true;
        }

        element->TransformToOuter(
            polygon,
            transformRetrievalOptions,
            ignoreClipping || (ignoreClippingOnScrollContentPresenters && isScrollContentPresenter));

        // TODO: Connected animation problem - useTargetInformation skips LTEs in the old hit testing walk,
        // but not here. It looks like this isn't breaking any tests though...

        if (element->OfTypeByIndex<KnownTypeIndex::Popup>())
        {
            // Popups escape clips, so ignore clipping once we walked past a popup.
            ignoreClipping = true;
            sharedLTEAncestor = nullptr;    // Don't try to restore clipping. We walked past a popup.

            // Setting this causes us to permanently ignore clipping for the rest of this tree walk,
            // even if we later see an LTE, which we want in the case of a popup.
            originalIgnoreClipping = true;
        }
        else if (!originalIgnoreClipping && element->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
        {
            // LTEs skip clips for the branch of the tree between their target and their ancestor element.
            // For a tree that looks like:
            //
            //  <A>
            //      <B>
            //          <C>
            //              <D />
            //          </C>
            //          <LTE Target="D" />
            //      </B>
            //  </A>
            //
            // The LTE will render with D's transform, but will skip the clips set on C and D. It's still
            // subject to clips set on its own ancestor elements, i.e. A and B.
            //
            // So we remember that shared ancestor and ignore clips until we've reached that element.
            //
            // We don't want to look up where that shared ancestor is. Assume it's the LTE's parent. That's
            // how Xaml sets up its LTEs, with the exception of LTEs targeting popups, but there the shared
            // ancestor is the root visual, which means we'll skip all clips up to the root visual anyway.
            //
            // Note that none of this is needed if we were going to ignore clipping to begin with, hence the
            // !originalIgnoreClipping check at the top.
            ignoreClipping = true;

            // We're expecting the LTE to be parented under a CTransitionRoot, and the parent of that is the
            // shared ancestor where we can clip again.
            //
            // Note that this works for properly nested LTEs. We'll hit the inner LTE first and stop clipping,
            // then on the way up we hit the inner LTE's shared ancestor and start clipping again, then we hit
            // the outer LTE and stop clipping again, then we hit the outer LTE's shared ancestor and start
            // clipping a second time. Other types of nested LTEs don't work, and we're ok with that.
            //
            // Note that this also doesn't work if the LTE targets another LTE. We're ok with that too.
            CUIElement* transitionRoot = element->GetUIElementParentInternal(false /* publicParentOnly */);
            // Note: transitionRoot isn't guaranteed to exist. We see cases where a ToolTip is getting the global bounds
            // of its owner after the island has been torn down, which leaves us with a dangling LTE without a
            // TransitionRoot.
            if (transitionRoot && transitionRoot->OfTypeByIndex<KnownTypeIndex::TransitionRoot>())
            {
                sharedLTEAncestor = transitionRoot->GetUIElementParentInternal(false /* publicParentOnly */);
            }
        }

        if (element->OfTypeByIndex<KnownTypeIndex::XamlIsland>())
        {
            // A XamlIsland knows its accumulated scale. Once we have this scale, we just use it. No need to keep walking up the parent chain.
            const float islandScale = RootScale::GetRasterizationScaleForElement(element);
            CMILMatrix scale;
            scale.SetToScaleAboutCenter(islandScale, islandScale, 0, 0);
            polygon.Transform(scale);
            break;
        }
        else if (element->OfTypeByIndex<KnownTypeIndex::RootVisual>())
        {
            //
            // In lifted Xaml, we're letting the ICoreWindowSiteBridge handle the scaling instead of adding
            // a scale transform at the root of the Xaml tree. This means TransformToVisual will no longer see the
            // scale, and will no longer return physical coordinates for things like UIA. We have to explicitly
            // account for the scale at the root in order to fix this. That scale is stored in the RasterizationScale
            // of the RootVisual.
            //
            // The long-term plan is to have lifted Xaml use islands mode for real.
            //      Task 23878262: Move lifted Xaml to islands for real
            //
            const double bridgeScale = element->GetRasterizationScale();
            CMILMatrix scale;
            scale.SetToScaleAboutCenter(static_cast<XFLOAT>(bridgeScale), static_cast<XFLOAT>(bridgeScale), 0, 0);
            polygon.Transform(scale);
            break;
        }

    }

    *worldSpaceRect = polygon.GetPolygonBounds();

    return S_OK;
}
