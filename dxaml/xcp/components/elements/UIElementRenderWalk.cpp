// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "uielement.h"
#include <MetadataAPI.h>
#include <MultiParentShareableDependencyObject.h>
#include <TransitionTarget.h>
#include <layouttransition.h>
#include <LayoutTransitionElement.h>
#include <UIElementCollection.h>
#include <MUX-ETWEvents.h>
#include "framework.h"

using namespace DirectUI;

// Returns true if any render-affecting property has changed on this UIElement or on any DO in its subgraph. T
// This method is shared between PC and SW rendering.
// All flags checked here need to be cleaned in NWClean.
bool CUIElement::NWNeedsRendering()
{
    return NWNeedsSubgraphRendering()
        || NWNeedsElementRendering()
        || PCIsRedirectionDataDirty();
}

// Returns true if any render-affecting property has changes on this UIElement, not
// including changes in its subgraph or in redirection data it collects.
bool CUIElement::NWNeedsElementRendering()
{
    return m_fNWVisibilityDirty
        || m_fNWTransformDirty
        || m_fNWProjectionDirty
        || m_isTransform3DDirty
        || NWNeedsElementRenderingInnerProjection()
        // These flags are only consumed by PC rendering, but they need to be checked and cleaned
        // consistently by both walks. Animations are typically disabled in software rendering, but even
        // if these flags are set in the software walk it shouldn't cause any harm - transitioning an element
        // between one type of walk to the other will completely regenerate its render data anyway.
        || PCHasOpacityAnimationDirty()
        || PCHasCompositionNodeDirty()
        || PCIsCompositeModeDirty()
        || m_isLightTargetDirty
        || m_isLightCollectionDirty
        || m_isEntireSubtreeDirty;
}

// Returns true if any render-affecting property that applies inside (excluding) the projection has changed
// on this UIElement, or any render-affecting property has changed on any DO in its subgraph.
bool CUIElement::NWNeedsElementRenderingInnerProjection()
{
    return m_fNWClipDirty
        || m_fNWLayoutClipDirty
        || m_fNWOpacityDirty
        || NWIsContentDirty();
}

// Returns true if a render-affecting property changed in the subgraph of this element,
// not including any changes on the element itself.
bool CUIElement::NWNeedsSubgraphRendering() const
{
    return m_fNWSubgraphDirty;
}

// Helper to propagate rendering changes to any registered layout transition elements using
// this element as their target. This is necessary because dirty state propagation is typically
// handled via parent element pointers, but elements undergoing layout transitions are still
// parented in the tree separately from the layout transition element used to render their content elsewhere.
void CUIElement::NWPropagateDirtyFlagForLayoutTransitions(DirtyFlags flags)
{
    xvector<CLayoutTransitionElement*>::const_reverse_iterator rend = m_pLayoutTransitionRenderers->rend();
    for (xvector<CLayoutTransitionElement*>::const_reverse_iterator it = m_pLayoutTransitionRenderers->rbegin(); it != rend; ++it)
    {
        CLayoutTransitionElement::NWSetSubgraphDirty(*it, flags);
    }
}

// Set dirty flags on the element and propagate dirtiness to parent. Marks the child bounds as dirty if bounds are modified.
void CUIElement::NWSetDirtyFlagsFromChildAndPropagate(DirtyFlags flags,bool renderConditionFlag)
{
    bool propagateDirty = false;

    if (flags_enum::is_set(flags, DirtyFlags::Render) && !renderConditionFlag)
    {
        renderConditionFlag = TRUE;

        propagateDirty = TRUE;
    }

    if (flags_enum::is_set(flags, DirtyFlags::Bounds) && !AreChildBoundsDirty())
    {
        // Propagate up unless the inner bounds were already dirty.
        if (!AreInnerBoundsDirty())
        {
            propagateDirty = TRUE;
        }
        else
        {
            // If inner bounds were dirty, outer bounds must be dirty too.
            // No need to propagate flags again.
            ASSERT(AreOuterBoundsDirty());
        }

        InvalidateChildBounds();
    }

    if (propagateDirty)
    {
        NWPropagateDirtyFlag(flags);
    }
}

/* static */
void CUIElement::NWSetOpacityDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

        //
        // Opacity - Dirties: (Render)
        //
        pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pUIE->m_fNWOpacityDirty);
        pUIE->m_fNWOpacityDirty = TRUE;
    }
}

// This RENDERCHANGEDPFN marks this UIElement's composite mode as dirty for rendering, and handles logic
// for requiring independent composition for this element.
/* static */
void CUIElement::NWSetCompositeModeDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    // CompositeMode cannot be changed independently
    ASSERT(!flags_enum::is_set(flags, DirtyFlags::Independent));

    // Propagate dirty flags and mark composite mode as dirty
    pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pUIE->m_fNWCompositeModeDirty);
    pUIE->m_fNWCompositeModeDirty = TRUE;

    // Now decide if the new composite mode changes the requirement of independent composition.
    ElementCompositeMode newCompositeMode = pUIE->GetCompositeMode();
    bool requiresComposition = false;

    switch (newCompositeMode)
    {
    case DirectUI::ElementCompositeMode::Inherit:
        // Inherit does not require composition because the element will draw
        // as part of its ancestor compnode, which already carries the inherited
        // composite mode.
        requiresComposition = FALSE;
        break;

    case DirectUI::ElementCompositeMode::MinBlend:
    case DirectUI::ElementCompositeMode::SourceOver:
    case DirectUI::ElementCompositeMode::DestInvert:
        // Simple policy here:  Just create a CompNode for any element that has
        // an "explicit" composite mode set.  This could be optimized but it
        // would be complicated and does not seem worth the effort since the app
        // must explicitly set a composite mode to get here.
        requiresComposition = TRUE;
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    // Now toggle our "requires composition" flag as necessary.
    if (requiresComposition)
    {
        if (!pUIE->IsUsingCompositeMode())
        {
            HRESULT hr = pUIE->SetRequiresComposition(
                CompositionRequirement::UsesCompositeMode,
                IndependentAnimationType::None
                );
            XCP_FAULT_ON_FAILURE(SUCCEEDED(hr));
        }
    }
    else
    {
        if (pUIE->IsUsingCompositeMode())
        {
            pUIE->UnsetRequiresComposition(
                CompositionRequirement::UsesCompositeMode,
                IndependentAnimationType::None
                );
        }
    }
}

/* static */ void CUIElement::NWSetLightTargetDirty(_In_ CDependencyObject *pTarget, bool isLightTarget)
{
    CUIElement* uiElement = static_cast<CUIElement*>(pTarget);
    uiElement->m_isLightTargetDirty = TRUE;
    uiElement->NWSetDirtyFlagsAndPropagate(DirtyFlags::Render, FALSE);

    if (isLightTarget && !uiElement->m_isLightTargetOrHasLight)
    {
        uiElement->SetRequiresComposition(CompositionRequirement::XamlLight, IndependentAnimationType::None);
    }
    else if (!isLightTarget && uiElement->m_isLightTargetOrHasLight && !uiElement->HasXamlLights())
    {
        uiElement->UnsetRequiresComposition(CompositionRequirement::XamlLight, IndependentAnimationType::None);
    }
}

/* static */ void CUIElement::NWSetLightCollectionDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    CUIElement* uiElement = static_cast<CUIElement*>(pTarget);
    uiElement->m_isLightCollectionDirty = TRUE;
    uiElement->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, FALSE);

    bool hasXamlLights = uiElement->HasXamlLights();
    if (hasXamlLights && !uiElement->m_isLightTargetOrHasLight)
    {
        uiElement->SetRequiresComposition(CompositionRequirement::XamlLight, IndependentAnimationType::None);
    }
    else if (!hasXamlLights && uiElement->m_isLightTargetOrHasLight && !uiElement->IsLightTarget())
    {
        uiElement->UnsetRequiresComposition(CompositionRequirement::XamlLight, IndependentAnimationType::None);
    }
}

/* static */
void CUIElement::NWSetContentDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        //
        // Content - Dirties: (Render)
        //
        if (flags_enum::is_set(flags, DirtyFlags::ForcePropagate))
        {
            // Currently the only flag supported in combination with ForcePropagate is Render.
            ASSERT(flags_enum::is_set(flags, DirtyFlags::Render) != 0);
            pUIE->NWSetDirtyFlagsAndPropagate(flags, FALSE);
        }
        else
        {
            pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pUIE->m_fNWContentDirty);
        }
        pUIE->m_fNWContentDirty = TRUE;
        pUIE->OnContentDirty();
    }
    else if (flags_enum::is_set(flags, DirtyFlags::Bounds) != 0)
    {
        // Independent changes can only dirty bounds.
        ASSERT(flags == (DirtyFlags::Independent | DirtyFlags::Bounds));

        pUIE->NWSetDirtyFlagsAndPropagate(flags, FALSE);
    }
}

void CUIElement::OnContentDirty()
{
}

/* static */
void CUIElement::NWSetContentAndBoundsDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    // Independent changes of rendering shape data are not supported.
    ASSERT(!flags_enum::is_set(flags, DirtyFlags::Independent));
    CUIElement::NWSetContentDirty(pTarget, DirtyFlags::Bounds);
}

/* static */
void CUIElement::NWSetSubgraphDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        //
        // Children - Dirties: (Render)
        //
        if (flags_enum::is_set(flags, DirtyFlags::ForcePropagate))
        {
            pUIE->NWSetDirtyFlagsFromChildAndPropagate(flags | DirtyFlags::Render, FALSE);
        }
        else
        {
            pUIE->NWSetDirtyFlagsFromChildAndPropagate(flags | DirtyFlags::Render, pUIE->m_fNWSubgraphDirty);
        }
        pUIE->m_fNWSubgraphDirty = TRUE;

        if (pTarget->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
        {
            CFrameworkElement* frameworkElement = static_cast<CFrameworkElement*>(pTarget);
            frameworkElement->UpdateRequiresCompNodeForRoundedCorners();
        }
    }
    else
    {
        // Independent changes can only dirty bounds, and if we got this far, bounds should have
        // been dirtied somewhere.
        ASSERT(flags == (DirtyFlags::Independent | DirtyFlags::Bounds));

        pUIE->NWSetDirtyFlagsFromChildAndPropagate(flags, FALSE);
    }
}

// Sets this element dirty and forces the next render walk to walk the entire subtree.
// This is different behavior than NWSetSubgraphDirty, which does not walk the entire subtree.
// Use this function with care!  It has performance impact.
void CUIElement::SetEntireSubtreeDirty()
{
    if (!m_isEntireSubtreeDirty)
    {
        NWSetDirtyFlagsAndPropagate(DirtyFlags::Render, FALSE);
        m_isEntireSubtreeDirty = TRUE;
    }
}

// Set the "ForceNoCulling" flag on this element and all elements up the tree to the root.
// This flag acts as a "branch flag", leaving a trail of breadcrumbs for the RenderWalk to follow.
// Note:  This flag does not propagate up through LayoutTransitionElements as it's not necessary -
// since LTE's are parented to a TransitionRoot, which is always a direct child of the main branch,
// we can let LTE's query their target for this flag and still get the correct answer (see IsForceNoCulling()).
void CUIElement::SetAndPropagateForceNoCulling(bool forceNoCulling)
{
    auto current = this;
    while (current != nullptr)
    {
        if (current->m_forceNoCulling != forceNoCulling)
        {
            if (forceNoCulling)
            {
                current->m_forceNoCulling = true;
            }
            else
            {
                CUIElementCollection* children = current->GetChildren();
                if (children != nullptr)
                {
                    for (auto& child : *children)
                    {
                        CUIElement* childElement = static_cast<CUIElement*>(child);
                        if (childElement->m_forceNoCulling)
                        {
                            // At least one child still has the flag turned on.  Stop propagating, to preserve that branch.
                            return;
                        }
                    }
                }
                current->m_forceNoCulling = false;
            }
            current = static_cast<CUIElement*>(current->GetParentInternal(false));
        }
        else
        {
            return;
        }
    }
}

bool CUIElement::IsForceNoCulling() const
{
    if (OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        // We don't propagate this flag through LTE's, instead they can query their target.
        const CLayoutTransitionElement* lte = static_cast<const CLayoutTransitionElement*>(this);
        return lte->GetTargetElement()->IsForceNoCulling();
    }
    else
    {
        return m_forceNoCulling;
    }
}

// Set dirty flags on the element and propagate dirtiness to parent. Marks content bounds as dirty if modified.
//
// NOTE: The dirtiness model here is generalized in a different way than might be expected from the rendering flags.
// All content and property changes invalidate the inner bounds and outer bounds instead of being tracked separately
// even though the bounds themselves are cached separately. This means that changing the RenderTransform causes the
// content bounds to be re-calculated even though that's not required - really only the outer bounds need to be invalidated.
// This hasn't proven to be a performance issue in any practical scenario as of yet.
void CUIElement::NWSetDirtyFlagsAndPropagate(DirtyFlags flags, bool renderConditionFlag)
{
    bool propagateDirty = false;

    if (flags_enum::is_set(flags, DirtyFlags::Render) && !renderConditionFlag)
    {
        propagateDirty = TRUE;
    }

    if (flags_enum::is_set(flags, DirtyFlags::Bounds) && !AreContentInnerBoundsDirty())
    {
        // Propagate up unless the inner bounds were already dirty.
        if (!AreInnerBoundsDirty())
        {
            propagateDirty = TRUE;
        }
        else
        {
            // If inner bounds were dirty, outer bounds must be dirty too.
            // No need to propagate flags again.
            ASSERT(AreOuterBoundsDirty());
        }

        InvalidateElementBounds();
    }

    if (propagateDirty)
    {
        NWPropagateDirtyFlag(flags);
    }
}

// Cleans the dirty flags on this UIElement.
void CUIElement::NWClean()
{
    m_fNWVisibilityDirty = FALSE;
    m_fNWTransformDirty = FALSE;
    m_fNWProjectionDirty = FALSE;
    m_isTransform3DDirty = FALSE;
    m_fNWClipDirty = FALSE;
    m_fNWOpacityDirty = FALSE;
    m_fNWCompositeModeDirty = FALSE;
    m_fNWContentDirty = FALSE;
    m_fNWSubgraphDirty = FALSE;

    m_isEntireSubtreeDirty = FALSE;

    m_fNWLayoutClipDirty = FALSE;

    // TODO: HWPC: The clean methods are shared between SW and PC rendering. Should rename and consolidate all the dirty flag checks.
    m_fNWHasOpacityAnimationDirty = FALSE;
    m_fNWHasCompNodeDirty = FALSE;
    m_fNWRedirectionDataDirty = FALSE;

    m_isLightCollectionDirty = FALSE;
    m_isLightTargetDirty = FALSE;
}

void CUIElement::PropagateNWClean()
{
    if (CUIElementCollection* children = GetChildren())
    {
        for (auto child : *children)
        {
            CUIElement* childElement = static_cast<CUIElement*>(child);

            if (childElement->NWNeedsRendering())
            {
                childElement->PropagateNWClean();
            }
        }
    }

    // Clear common flags.
    NWClean();

    // Clear class-specific flags.
    NWCleanDirtyFlags();
}