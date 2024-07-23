// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WinTextCore.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <GraphicsUtility.h>
#include "FocusRectangle.h"
#include "focusmgr.h"

#include "TimeMgr.h"
#include <FxCallbacks.h>
#include <ImageDecodeBoundsFinder.h>
#include <VisualContentRenderer.h>
#include <HWCompNodeWinRT.h>
#include <HWRedirectedCompTreeNodeWinRT.h>
#include <HWWindowedPopupCompTreeNodeWinRT.h>
#include "MediaPlayerPresenter.h"
#include "ColorUtil.h"
#include <XamlLightCollection.h>
#include <XamlLight.h>
#include <FrameworkTheming.h>
#include "application.h"
#include <PixelFormat.h>
#include <FeatureFlags.h>
#include "XboxUtility.h"
#include <XamlOneCoreTransforms.h>
#include <RootScale.h>
#include "GraphicsTelemetry.h"

using namespace RuntimeFeatureBehavior;

#define DBG_REALIZATIONS 0
#define DBG_REALIZATIONS_VERBOSE 0

//------------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function that gets the scale part of the world transformation
//      into a Matrix.  Used to calculate the realization scale when creating
//      caches.
//
//      Should never be called on the base class.
//
//------------------------------------------------------------------------------
CMILMatrix HWRenderParamsBase::GetRasterizationScaleTransform(_In_ CUIElement *pElement) const
{
    XCP_FAULT_ON_FAILURE(FALSE);
    return CMILMatrix(true);
}

CMILMatrix HWRenderParams::GetRasterizationScaleTransform(_In_ CUIElement* pElement) const
{
    CMILMatrix rasterizationTransform = pTransformToRoot->GetRasterizationMatrix(pElement);
    rasterizationTransform.SetDx(0);
    rasterizationTransform.SetDy(0);
    return rasterizationTransform;
}

void HWRenderParams::ExtractTransformToRoot(_Out_ CTransformToRoot* pTransformToRoot2D)
{
    *pTransformToRoot2D = *pTransformToRoot;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
HWWalk::HWWalk(_In_ CWindowRenderTarget *pRenderTarget, _In_ MaxTextureSizeProvider& maxTextureSizeProvider)
    : m_pRenderTargetNoRef(pRenderTarget)
    , m_pTextureManager(NULL)
    , m_pSurfaceCache(NULL)
    , m_pCurrentContainerNoRef(NULL)
    , m_pCurrentSpriteVisualNoRef(NULL)
    , m_pParentCompNodeNoRef(NULL)
    , m_pPreviousChildCompNodeNoRef(NULL)
    , m_pOverrideRenderDataListNoRef(NULL)
    , m_inSwapChainPanelSubtree(false)
    , m_maxTextureSizeProvider(maxTextureSizeProvider)
    , m_elementsVisited(0)
    , m_elementsRendered(0)
{
    XCP_WEAK(&m_pRenderTargetNoRef);
    XCP_WEAK(&m_pCurrentContainerNoRef);
    XCP_WEAK(&m_pCurrentSpriteVisualNoRef);
    XCP_WEAK(&m_pParentCompNodeNoRef);
    XCP_WEAK(&m_pPreviousChildCompNodeNoRef);
    XCP_WEAK(&m_pOverrideRenderDataListNoRef);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     dtor
//
//-------------------------------------------------------------------------
HWWalk::~HWWalk()
{
    ReleaseInterface(m_pTextureManager);
    ReleaseInterface(m_pSurfaceCache);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Initialize cross frame resources
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
HWWalk::Initialize(_In_ CCoreServices *pCore)
{
    IFC_RETURN(SurfaceCache::Create(pCore, &m_pSurfaceCache));

    IFC_RETURN(HWTextureManager::Create(
        m_pRenderTargetNoRef,
        m_maxTextureSizeProvider,
        pCore->GetAtlasRequestProvider(),
        &m_pTextureManager
        ));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Create a HWWalk object
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
HWWalk::Create(
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_ CCoreServices *pCore,
    _In_ MaxTextureSizeProvider& maxTextureSizeProvider,
    _Outptr_ HWWalk **ppHwWalk
    )
{
    HRESULT hr = S_OK;
    HWWalk *pHwWalk = NULL;

    pHwWalk = new HWWalk(pRenderTarget, maxTextureSizeProvider);

    IFC(pHwWalk->Initialize(pCore));

    *ppHwWalk = pHwWalk;
    pHwWalk = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pHwWalk);
    RRETURN(hr);
}

WUComp::IVisual* HWWalk::GetLastSpriteVisual() const
{
    return m_pCurrentSpriteVisualNoRef;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Handles determination of whether the subgraph is in the scene or not, and updates
//      the element as needed if so.
//      Guaranteed to update the HWWalk with the correct ptr into the render data list for
//      the next element.
//
//-------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::Render(
    _In_ CUIElement *pUIElement,
    _In_ const HWRenderParams &parentRP,
    bool redirectedDraw
    )
{
    HRESULT hr = S_OK;

    ASSERT(parentRP.pTransformToRoot != NULL);
    ASSERT(parentRP.pTransformsAndClipsToCompNode != NULL);

    // An element that requires redirected drawing is rendered first at its location
    // in the tree, and again later from a redirection root element.
    //
    // A redirected draw should only occur for elements that require it.
    //
    // If we're directly drawing an element that requires redirected drawing, do nothing.
    // We'll walk to it again from the redirection root to actually draw it later.
    //
    // Elements that require redirected drawing create compositor nodes attached at the end
    // of the tree under their redirection root, so their content draws on top.  However, they
    // store a pointer to a comp node earlier in the tree so that the independently-animated position
    // of their target is available, and that's used to position the redirected content on the render thread.
    const bool requiresRedirectedDrawing = pUIElement->IsRedirectionElement();
    ASSERT(requiresRedirectedDrawing || !redirectedDraw);
    ASSERT(!requiresRedirectedDrawing || GetRedirectionTarget(pUIElement) != NULL);

    if (parentRP.pHWWalk->m_inSwapChainPanelSubtree && requiresRedirectedDrawing && !redirectedDraw)
    {
        // We've encountered a redirection element that is the child of a SwapChainPanel.
        // Set a flag on this node so that the redirection part of the render-walk can determine if it
        // should create a CompNode for hit-test-invisibility purposes.
        pUIElement->SetIsRedirectedChildOfSwapChainPanel(true);
    }

    // If the element is the target of a layout transition, it shouldn't draw to the screen when walked -
    // the LayoutTransitionElement targeting it handles rendering its subgraph instead. If the element
    // itself is redirected (i.e. a Popup), it means it should render when walked later from a specific parent
    // and not when potentially first encountered.
    //
    // When we do encounter these elements earlier in the 'wrong' z-order in the tree, we need to treat
    // the Render call as a true no-op.
    // - Preserve all dirty flags - the element may be rendered again later on this frame.
    //    If it isn't, it means the redirected path isn't connected yet, which is also okay (like a closed Popup).
    //    In that case, dirty flags will be preserved until that path is created, and the creation of that
    //    link will dirty the tree and cause the element to finally draw.
    // - Do not leave the scene. We handle rendering later, which will handle leaving the scene later as well
    //    if needed. The Popup.Child's render data is invalidated/cleared when entering/leaving the live tree on
    //    Open/Close, just like attaching/detaching any normal element. Things are slightly trickier for the LTE.Target,
    //    since when the transition is over the element is walked during the normal 'direct' render walk again. To ensure
    //    render data is updated correctly here, the LTE.Target's render data is managed by the LTE when attached/detached
    //    in the same way as Popup.Child, by treating the LTE.Target's subgraph as being removed and re-added to the tree.
    //    See CUIElement::AddLayoutTransitionRenderer and CUIElement::RemoveLayoutTransitionRenderer
    // - Do not update the 'last render data' pointer, because again, we're at the 'wrong' z-order in this Render call.
    //    We don't want the contents of a Popup's sibling to be inserted after the Popup.Child's contents, because we intentional
    //    delay adding the Popup.Child's content until later so that it resides higher in the z-ordering.
    //
    // Since we never render LTEs for RTBs, elements with LTs should be rendered as if they are without any.
    // Hence we do not skip the render in such cases.
    const bool hiddenForLayoutTransition = pUIElement->IsHiddenForLayoutTransition();
    const bool skipRenderWhileRedirected = (requiresRedirectedDrawing && !redirectedDraw)
                                          || hiddenForLayoutTransition;

    if (!skipRenderWhileRedirected)
    {
        // Skip rendering this element unless one of the following is true:
        // - the subgraph must be re-rendered because of a change inherited from an ancestor
        // - the element has not been rendered
        // - the element has been rendered, but either this element or a descendent is dirty and needs to be updated
        // - the element is redirected. This case is specifically for Popup, since it isn't marked dirty by changes
        //    in the Popup.Child's subgraph. Those changes do propagate to the PopupRoot, though, which will walk to
        //    the Popup instead of the Popup.Child.
        if (   parentRP.NeedsToWalkSubtree()
            || !pUIElement->IsInPCScene()
            || pUIElement->NWNeedsRendering()
            || requiresRedirectedDrawing)
        {
            // We skip rendering collapsed elements except in the case of when an implicit Hide animation is playing.
            // In this case we may still need to RenderWalk this element to create its CompNode, and to make incremental
            // updates to be consistent with the tree removal case.
            const bool skipRenderWhileCollapsed = !pUIElement->IsVisible() && !pUIElement->IsKeepVisible();

            // These flags are initialized with more complex logic.
            bool skipRenderWhileInheritedCollapsed = false;
            bool skipRenderWhileTransparent = false;
            bool skipRenderWhileClippedOut = false;
            bool skipRenderWhileLayoutClippedOut = false;
            bool skipRenderWhileTransformTooSmall = false;
            bool skipRenderWhileParentHasZeroSizedClip = parentRP.pTransformsAndClipsToCompNode->HasZeroSizedClip() && !pUIElement->IsForceNoCulling();

            TraceLoggingProviderWrite(
                GraphicsTelemetry, "RenderWalk_RenderElement",
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(pUIElement), "ElementPointer"),
                TraceLoggingWideString(pUIElement->GetClassName().GetBuffer(), "ClassName"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            // Skip rendering this element subgraph if possible.
            // IMPORTANT: The checks in Cleanup for cleaning specific dirty flags need to reflect the order we check each condition here.
            //            If we clean the wrong dirty flag when multiple conditions are true, we may never detect the change that causes the
            //            subgraph to become visible again.
            if (!skipRenderWhileCollapsed
                && !skipRenderWhileParentHasZeroSizedClip)
            {
                IFC(RenderProperties(
                    pUIElement,
                    parentRP,
                    requiresRedirectedDrawing,
                    &skipRenderWhileInheritedCollapsed,
                    &skipRenderWhileTransparent,
                    &skipRenderWhileClippedOut,
                    &skipRenderWhileLayoutClippedOut,
                    &skipRenderWhileTransformTooSmall
                    ));

                HWWalk::TraceElementAccessibility(pUIElement);
            }

            {
                bool clearSubgraphRenderData; // initialized in each branch
                if (skipRenderWhileParentHasZeroSizedClip)
                {
                    // Leave all the dirty flags alone. Some ancestor needs to change to unclip this element.

                    // Remove render data from scene.
                    clearSubgraphRenderData = TRUE;
                }
                else if (skipRenderWhileCollapsed)
                {
                    // Preserve dirty flags in the subgraph until the element is visible again. Clean only the visibility
                    // flag, so that the walk can detect when the visibility changes.
                    pUIElement->m_fNWVisibilityDirty = FALSE;

                    // Remove render data from scene.
                    clearSubgraphRenderData = TRUE;
                }
                else if (skipRenderWhileInheritedCollapsed)
                {
                    // Preserve all dirty flags - the element has inherited being collapsed from another point in the tree
                    // not on the path used to render this element.  When that context element becomes visible again and dirties
                    // the tree, this element will pick up that change and be allowed to render too.
                    //
                    // We know that this element will be walked, even without being marked dirty directly, because it is redirected
                    // and will have a comp node, which means we'll always walk down its path.
                    ASSERT(requiresRedirectedDrawing);

                    // Remove render data from scene.
                    clearSubgraphRenderData = TRUE;
                }
                else if (skipRenderWhileTransparent)
                {
                    // Preserve dirty flags in the subgraph until the element is opaque again.  Clean only the opacity
                    // flag, so that the walk can detect when the opacity changes.
                    pUIElement->m_fNWOpacityDirty = FALSE;

                    // Remove render data from scene.
                    clearSubgraphRenderData = TRUE;
                }
                // These "skip render" flags work in layers. They're checked in a certain order, and it's important that we clear their
                // dirty flags in the same order. When processing layout clips as ancestor clips, we check layout clips before we check
                // the explicit UIElement clip. We need to clear the dirty flag in the same order.
                // Otherwise, we could clean the UIE clip dirty flag while leaving the layout dirty flag set, when it was the layout clip
                // that caused us to skip rendering. When the layout clip is updated later to allow rendering, we fail to propagate that
                // up to the root of the tree, because we see that the layout clip is already dirty.
                else if (skipRenderWhileLayoutClippedOut
                    && pUIElement->ShouldApplyLayoutClipAsAncestorClip())
                {
                    // Preserve dirty flags in the subgraph until the element is unclipped again.  Clean only the layout
                    // clip flag, so that the walk can detect when the layout clip changes.
                    pUIElement->m_fNWLayoutClipDirty = FALSE;

                    // Remove render data from scene.
                    clearSubgraphRenderData = TRUE;
                }
                else if (skipRenderWhileClippedOut)
                {
                    // Preserve dirty flags in the subgraph until the element is unclipped again.  Clean only the clip
                    // flag, so that the walk can detect when the clip changes.
                    pUIElement->m_fNWClipDirty = FALSE;

                    // Remove render data from scene.
                    clearSubgraphRenderData = TRUE;
                }
                else if (skipRenderWhileLayoutClippedOut
                    && !pUIElement->ShouldApplyLayoutClipAsAncestorClip())
                {
                    // Preserve dirty flags in the subgraph until the element is unclipped again.  Clean only the layout
                    // clip flag, so that the walk can detect when the layout clip changes.
                    pUIElement->m_fNWLayoutClipDirty = FALSE;

                    // Remove render data from scene.
                    clearSubgraphRenderData = TRUE;
                }
                else if (skipRenderWhileTransformTooSmall)
                {
                    // Preserve dirty flags in subgraph until the element is not too small to render.  Clean only the transform
                    // flag, so that the walk can detect when the transform changes.
                    pUIElement->m_fNWTransformDirty = FALSE;

                    // Remove render data from scene.
                    clearSubgraphRenderData = TRUE;
                }
                else
                {
                    // Dirty flags should have been cleaned in RenderContentAndChildren, with some exceptions:
                    ASSERT(!pUIElement->NWNeedsRendering()
                        // Elements that are rendered as part of secondary LTEs will create temporary visuals and comp nodes,
                        // and don't touch the tree's dirty state
                        || parentRP.pHWWalk->GetOverrideRenderDataList() != nullptr);

                    // If the element was rendered, it should either be in the scene or in the overridden storage of some parent element in the scene
                    // of the render walk is for RenderTargetElement.
                    ASSERT(pUIElement->IsInPCScene() || parentRP.pHWWalk->GetOverrideRenderDataList() != nullptr);
                    clearSubgraphRenderData = FALSE;
                }

                // If the render walk was pruned because the content isn't visible, do a simple walk down the tree to ensure
                // all render data is removed from the persistent scene.
                if (clearSubgraphRenderData)
                {
                    pUIElement->LeavePCSceneRecursive();
                }
            }
        }

        // Update the render walk state with the last SpriteVisual and comp node contributed from the subgraph.
        // This cached information allows the walk to skip branches of the tree without walking all the way down
        // to the element that actually contributed the SpriteVisual or comp node.
        parentRP.pHWWalk->UpdateLastSpriteVisualAndCompNodeFromSubgraph(pUIElement, parentRP);
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fires ETW event related to the elements accessibility
//
//-------------------------------------------------------------------------
/*static*/ void
HWWalk::TraceElementAccessibility(
    _In_ CUIElement* pUIElement)
{
    if (!EventEnabledElementAccessibilityInfo() || pUIElement == nullptr)
    {
        return;
    }

    // Make sure the UIElement is a control, except for CPage which derives from CUserControl
    // and we want to ignore the page
    if (pUIElement->OfTypeByIndex<KnownTypeIndex::Control>() &&
        !pUIElement->OfTypeByIndex<KnownTypeIndex::Page>())
    {
        CValue accessibilityView;
        if (FAILED(pUIElement->GetValueByIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView, &accessibilityView)))
        {
            return;
        }

        UINT32 accessibilityViewValue = accessibilityView.AsEnum();

        // only care if this is a ControlElement. content view is subset of control view so anything which is in
        // content view will have ControlElement as true
        if (accessibilityViewValue == static_cast<UINT32>(xaml_automation_peers::AccessibilityView_Content) ||
            accessibilityViewValue == static_cast<UINT32>(xaml_automation_peers::AccessibilityView_Control))
        {
            CAutomationPeer* automationPeer = pUIElement->OnCreateAutomationPeer();

            xstring_ptr automationName;
            xstring_ptr controlType;

            // If this element doesn't have an automation peer and isn't a UserControl,
            // then ignore it. if an app has a UserControl it needs to override OnCreateAutomationPeer
            // and supply it's own AutomationPeer. For other types, if they don't have a peer we don't
            // care
            if (automationPeer)
            {
                IGNOREHR(automationPeer->GetName(&automationName));
                IGNOREHR(automationPeer->GetLocalizedControlType(&controlType));
            }
            else if (!pUIElement->OfTypeByIndex<KnownTypeIndex::UserControl>())
            {
                return;
            }

            // passing nullptr to trace events will make the string in the payload be "NULL".
            // so if there is no name or control type, pass an empty string rather than nullptr.
            TraceElementAccessibilityInfo(
                reinterpret_cast<ULONGLONG>(static_cast<CDependencyObject*>(pUIElement)),
                reinterpret_cast<ULONGLONG>(pUIElement->GetParent()),
                !automationName.IsNullOrEmpty() ? automationName.GetBuffer() : L"",
                !controlType.IsNullOrEmpty() ? controlType.GetBuffer() : L""
                );
        }
    }
}

void HWWalk::ResetEtwData()
{
    m_elementsVisited = 0;
    m_elementsRendered = 0;
}

int HWWalk::GetElementsVisited() const
{
    return m_elementsVisited;
}

int HWWalk::GetElementsRenderedCount() const
{
    return m_elementsRendered;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Handles element properties and composition nodes.
//
//-------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::RenderProperties(
    _In_ CUIElement *pUIElement,
    _In_ const HWRenderParams &parentRP,
    bool requiresRedirectedDrawing,
    _Out_ bool *pSkipRenderWhileInheritedCollapsed,
    _Out_ bool *pSkipRenderWhileTransparent,
    _Out_ bool *pSkipRenderWhileClippedOut,
    _Out_ bool *pSkipRenderWhileLayoutClippedOut,
    _Out_ bool *pSkipRenderWhileTransformTooSmall
    ) noexcept
{
    xref_ptr<HWCompTreeNode> pRedirectionCompNode;
    xref_ptr<HWCompTreeNode> pElementCompNode;
    xref_ptr<HWCompLeafNode> pContentCompNode;
    xref_ptr<HWCompRenderDataNode> pPostSubgraphCompNode;
    xref_ptr<WUComp::IContainerVisual> spRenderDataParent;

    // Copy render params since they'll be modified to handle properties, flags, etc.
    HWRenderParams myRP(parentRP);

    bool skipRenderWhileInheritedCollapsed = false;
    bool skipRenderWhileTransparent = false;
    bool skipRenderWhileClippedOut = false;
    bool skipRenderWhileLayoutClippedOut = false;
    bool skipRenderWhileTransformTooSmall = false;

    xref_ptr<CTransitionTarget> transitionTarget;

    parentRP.pHWWalk->m_elementsVisited++;

    //
    // Push local properties (transform, clip, and opacity)
    //
    ASSERT(myRP.opacityToCompNode >= 0 && myRP.opacityToCompNode <= 1);
    ASSERT(pUIElement->GetOpacityCombined() >= 0 && pUIElement->GetOpacityCombined() <= 1);
    XFLOAT combinedOpacityToCompNode = myRP.opacityToCompNode * pUIElement->GetOpacityCombined();

    // Opacity on elements affect the readability for visual impaired users when using HighContrast. This overrides the opacity for elements
    // that are not fully transparent to provide a better experience in HighContrast mode.
    if (ShouldOverrideRenderOpacity(combinedOpacityToCompNode, pUIElement))
    {
        combinedOpacityToCompNode = 1;
    }

    // The matrices are scoped here to ensure a reference to it on the stack remains valid for the method's entire scope.
    CTransformToRoot combinedTransformToRoot = *myRP.pTransformToRoot;
    CMILMatrix transformsFromRedirectionTargetToRedirectionCompNode;
    TransformAndClipStack combinedTransformsAndClipsToCompNode;
    IFC_RETURN(combinedTransformsAndClipsToCompNode.Set(myRP.pTransformsAndClipsToCompNode));

    const bool isXamlIsland = pUIElement->GetTypeIndex() == KnownTypeIndex::XamlIslandRoot;
    const bool isRootAndOneCoreStrict = pUIElement->GetTypeIndex() == KnownTypeIndex::RootVisual && XamlOneCoreTransforms::IsEnabled();
    if (isXamlIsland || isRootAndOneCoreStrict)
    {
        const float scale = RootScale::GetRasterizationScaleForElement(pUIElement);
        combinedTransformToRoot.SetTo2DScale(scale);
    }

    if (pUIElement->ShouldApplyLayoutClipAsAncestorClip())
    {
        // The LayoutClip is in the parent coordinate space and is applied above any RenderTransform.
        // This is a behavior change starting with Redstone, the previous behavior was to apply the LayoutClip below RenderTransforms.
        // We think this behavior makes more sense - the LayoutClip should ideally act as an ancestor clip, not a self-clip.
        IFC_RETURN(IntersectClipInCompNodeSpace(
            pUIElement->HasLayoutClip() ? pUIElement->LayoutClipGeometry : nullptr,
            &combinedTransformsAndClipsToCompNode,
            &skipRenderWhileLayoutClippedOut
            ));
    }

    {
        CMILMatrix localTransform;
        // When rendering, we don't care about properties set by composition on the handoff visual. That will prevent problems like
        // creating a tiny mask because there's a WUC scale animation going on.
        if (!pUIElement->GetLocalTransform(TransformRetrievalOptions::None, &localTransform))
        {
            combinedTransformToRoot.Prepend(localTransform);
            combinedTransformsAndClipsToCompNode.PrependTransform(localTransform);
        }
    }

    if (pUIElement->HasActiveProjection())
    {
        XSIZEF elementSize;
        IFC_RETURN(pUIElement->GetElementSizeForProjection(&elementSize));
        auto projection = pUIElement->GetProjection();

        CMILMatrix4x4 projectionTransform = projection->GetOverallProjectionMatrix(elementSize);
        std::unique_ptr<CMILMatrix4x4> pProjectionTransform = std::make_unique<CMILMatrix4x4>(projectionTransform);

        combinedTransformToRoot.Prepend(*pProjectionTransform);
        IFC_RETURN(combinedTransformsAndClipsToCompNode.PrependProjection(pProjectionTransform.release()));
    }

    if (pUIElement->GetRasterizationScale() != 1.0)
    {
        combinedTransformToRoot.MultiplyRasterizationScale(pUIElement->GetRasterizationScale());
    }

    if (combinedTransformsAndClipsToCompNode.HasZeroSizedClip())
    {
        // Don't bother intersecting the clip if we're already clipped to nothing - we'll return skipRenderWhileLayoutClippedOut.
        ASSERT(skipRenderWhileLayoutClippedOut);
    }
    else
    {
        IFC_RETURN(IntersectClipInCompNodeSpace(
            pUIElement->GetClip(),
            &combinedTransformsAndClipsToCompNode,
            &skipRenderWhileClippedOut
            ));
    }

    if (!pUIElement->ShouldApplyLayoutClipAsAncestorClip())
    {
        // Don't bother intersecting the clip if we're already clipped to nothing - we'll return skipRenderWhileClippedOut.
        if (combinedTransformsAndClipsToCompNode.HasZeroSizedClip())
        {
            ASSERT(skipRenderWhileClippedOut);
        }
        else
        {
            IFC_RETURN(IntersectClipInCompNodeSpace(
                pUIElement->HasLayoutClip() ? pUIElement->LayoutClipGeometry : nullptr,
                &combinedTransformsAndClipsToCompNode,
                &skipRenderWhileLayoutClippedOut
                ));
        }
    }

    transitionTarget = pUIElement->GetTransitionTarget();
    if (transitionTarget != nullptr)
    {
        XRECTF transitionClip = {0, 0, pUIElement->GetActualWidth(), pUIElement->GetActualHeight()};
        IFC_RETURN(transitionTarget->ApplyClip(
            transitionClip,
            &combinedTransformsAndClipsToCompNode
            ));
    }

    if (requiresRedirectedDrawing)
    {
        ASSERT(pUIElement->RequiresCompositionNode());

        // The redirected comp node is the nearest ancestor comp node of the target element.
        // The prepend transform collected while walking up to it represents the fixed transform
        // from the target to that comp node.  Together, that prepend transform plus the comp node
        // allow this element's HwRedirectedCompNode to position its content on the render thread exactly
        // where its target would have been positioned, including taking independent-animations into account.
        CUIElement *pTarget = GetRedirectionTarget(pUIElement);
        ASSERT(pTarget != nullptr);

        bool areAllAncestorsVisible = false;
        // If this element needs to be kept visible for a hide animation, we walk it to get the redirected transforms anyway.
        // This comes up for collapsed popups.
        if ((pTarget->IsVisible() || pTarget->IsKeepVisible())
            && !pTarget->SkipRenderForLayoutTransition())
        {
            // TODO: HWPC: What if the LayoutTransitionElement's target was already animating?

            // TODO: HWPC: Layout transitions shouldn't be running on non-live elements

            // Notes:
            // The behavior regarding which transforms are included in transformsFromRedirectionTargetToRedirectionCompNode varies,
            // depending on what type of redirection element is involved:
            // Popups:  In this case we use the Popup itself as the target for gathering the transforms.
            //          Since the Popup itself requires a CompNode, it will apply its own transform to the CompNode during PushProperties,
            //          and the walk below will incorporate the rest of the transforms, starting with the Popup's parent,
            //          thus, all transforms in the ancestor chain are incorporated.
            // LTEs:  In this case, the LTE's target is the target for gathering the transforms.
            //        Since LTE targets do not require CompNodes, and we start the process of gathering ancestor transforms at the
            //        parent of the LTE target, we may not incorporate the local transform of the LTE target itself.
            //        This is (apparently) by design, as LTEs act as a "stand-in" for the LTE target, using its local transform
            //        instead of the LTE target's local transform.
            CUIElement *pParent = pTarget->GetUIElementAdjustedParentInternal(TRUE /* publicOnly */);
            ASSERT(pParent != nullptr || !pTarget->IsActive());
            if (pParent != nullptr)
            {
                // Walk up to find the nearest comp node to the target and the transform from the target to the comp node.
                // Also, calculate the transform to root and whether that transform is animating.
                // Do we need to collect any inherited PC dirty flags on the walk up here?
                // - forceRender
                //      Unnecessary. This is either set at the root, in which case the localRP have it too, or its used by
                //      override render data, which again we want to come from the localRP where we're drawing the content,
                //      not from the target.
                // - isEnteringScene
                //      Unnecessary. LTE and Popup manage entering and leaving the scene already. The target's visibility info
                //      collected from the walk here is sufficient to prune the tree, and isEnteringScene will be set again for this
                //      redirected subgraph if it re-enters the scene later because the target becomes visible again.
                // - hasPropertyToCompNodeChanged
                //      Unnecessary. The walk is guaranteed to process to here, because this is a redirection element, and the entire
                //      subgraph is drawn relative to this comp node. This would only be needed if we wanted to try and prune the walk
                //      up to collect the redirection properties itself based on dirtiness.
                ASSERT(pRedirectionCompNode == nullptr);
                bool isRedirectedTransformAnimating;

                // TODO: JComp: absolutely positioned LTEs render with only the local transforms. The transform to root doesn't
                // need to be updated by the walk up
                IFC_RETURN(pParent->GetRedirectionTransformsAndParentCompNode(
                    myRP.pRenderTarget,
                    &transformsFromRedirectionTargetToRedirectionCompNode,
                    TRUE, /* allowClosedPopup */
                    &combinedTransformToRoot,
                    &isRedirectedTransformAnimating,
                    &areAllAncestorsVisible,
                    reinterpret_cast<HWCompTreeNode**>(pRedirectionCompNode.ReleaseAndGetAddressOf())
                    ));

                myRP.isTransformAnimating = isRedirectedTransformAnimating;
            }
        }

        // Skip rendering the target of the transition if it isn't in the live tree, or if the target or one of its ancestors was collapsed.
        if (!areAllAncestorsVisible || pRedirectionCompNode == nullptr)
        {
            // Elements that aren't live should never find comp nodes on the walk up.
            ASSERT(!areAllAncestorsVisible || !pTarget->IsActive());

            skipRenderWhileInheritedCollapsed = TRUE;
        }
    }

    // Skip rendering the subgraph if this element is transparent.
    // We could calculate and use the world opacity to be slightly more accurate, but it would only matter
    // for rounding error when many nested elements had near-zero opacities.
    skipRenderWhileTransparent = OpacityToBlendInt(combinedOpacityToCompNode) <= 0;

    // In SpriteVisuals mode, we can set hit-test invisibility directly on the SpriteVisuals.
    if (!pUIElement->IsEnabledAndVisibleForDCompHitTest())
    {
        myRP.m_isHitTestVisibleSubtree = false;
    }

    if (pUIElement->IsEntireSubtreeDirty())
    {
        myRP.forceUpdateCompNodes = TRUE;
    }

    // Update animation flags.  Animations can disable early-exit optimizations because the local value might
    // change independently on the render thread and cause content to be visible again - not marshalling that
    // content over will cause glitches.
    const bool requiresCompositionNode = pUIElement->RequiresCompositionNode();
    if (requiresCompositionNode)
    {
        // If this element's transform is being animated, it affects texture realization in the subgraph.
        // Direct manipulation is treated just like an independent transform animation, since they behave similarly
        // from a rendering perspective.
        // Propagate that info down the tree.
        const bool isLocalPositionAnimatingIndependently = pUIElement->IsTransformOrOffsetAffectingPropertyIndependentlyAnimating();
        if (isLocalPositionAnimatingIndependently)
        {
            myRP.isTransformAnimating = TRUE;
        }

        if (pUIElement->IsOpacityIndependentlyAnimating()
            || pUIElement->HasWUCOpacityAnimation())
        {
            // If the local opacity is being independently animated, don't skip rendering to avoid glitches for animations
            // starting at zero opacity. The same applies if there's a WUC animation running on the UIElement.Opacity facade.
            // Note that this animation could be from an implicit OpacityTransition set on the UIElement.
            skipRenderWhileTransparent = FALSE;

            myRP.isOpacityAnimating = TRUE;
        }

        if (isLocalPositionAnimatingIndependently || pUIElement->IsClipIndependentlyAnimating())
        {
            // If the local clip is being independently animated, or the local transform is which affects the clip,
            // we can't skip rendering here, again, to avoid glitches in case the content is animating into a visible position.
            skipRenderWhileClippedOut = FALSE;
            skipRenderWhileLayoutClippedOut = FALSE;
        }
    }

    // To provide basic stability for the PC, check for near zero here by providing a conservate limit of a
    // world transform scale of 0.000001 in either x or y.
    {
        const XFLOAT scaleLimit = 0.000001f;
        XFLOAT minScaleX;
        XFLOAT minScaleY;

        combinedTransformToRoot.GetMinimumScaleDimensions(&minScaleX, &minScaleY);

        if (minScaleX < scaleLimit || minScaleY < scaleLimit)
        {
            // If the transform is animating, don't skip rendering - reset this transform-to-root to a
            // safe, known default value for creating realizations.  This also ensures that animations
            // up from zero scale have realizations available at some default size.
            // However, there are some caveats.
            // - This may also cause unnecessary realizations to be created for content clipped out by a zero scale but otherwise animated.
            // TODO: HWPC: This is fixable by tracking scale animations with a separate flag from other transform animations
            // - This may cause content at extremely small scales to pop in and out (because sometimes it's culled, sometimes it isn't).
            //   This is an edge case and shouldn't typically produce noticeable artifacts, unless the content itself is also very large.
            if (myRP.isTransformAnimating)
            {
                const float plateauScale = RootScale::GetRasterizationScaleForElement(pUIElement);
                combinedTransformToRoot.SetTo2DScale(plateauScale);
            }
            // If the transform isn't animating, it's safe to just cull out this subgraph.
            else
            {
                skipRenderWhileTransformTooSmall = TRUE;
            }
        }
    }

    if (skipRenderWhileTransparent || skipRenderWhileClippedOut || skipRenderWhileLayoutClippedOut)
    {
        // Note: We explicitly ignore the case where the transform is too small. The render transform maps to the WUC visual's
        // TransformMatrix property, and we don't expect apps to be placing implicit animations on it. Instead, the app can change
        // the WUC visual's Scale property, which Xaml does not write to or use for culling purposes.


        if (pUIElement->IsUsingHandOffVisual() || pUIElement->IsForceNoCulling())
        {
            // If the element is on the path to a hand off visual, then we can't cull it. We need to render the element so that the
            // WUC visual will be inserted into the tree, for implicit animations and for the app to access.
            // We also handle the "ForceNoCulling" flag here - this is more general purpose and is set on a branch of the tree.
            //
            // Note: MediaPlayerElement's Closed Captions rendering depends on this opt-out following the fix for
            //       Bug 40801660: [MediaPlayerElement] CC: Layer opacity should be applied when transparency + edge effects are requested
            //
            //       With that fix, captions are rendered via a CompositionVisualSurface snapshot, and a container with 0x0 clip
            //       is applied to avoid double-rendering. A dummy handoff is additionally obtained to ensure the subtree isn't culled.
            //       A change here would need corresponding update to CCueStyler::SetTextBlockEffect and CCueRenderer::AddCueItem.
            skipRenderWhileTransparent = FALSE;
            skipRenderWhileClippedOut = FALSE;
            skipRenderWhileLayoutClippedOut = FALSE;
        }
    }

    // The "ForceNoCulling" flag is consumed by RenderTargetBitmap live capture, which requires a CompNode be generated for all culled
    // elements.  This includes the skipRenderWhileTransformTooSmall scenario.  Force no culling for this scenario as well as the above.
    if (skipRenderWhileTransformTooSmall && pUIElement->IsForceNoCulling())
    {
        skipRenderWhileTransformTooSmall = FALSE;
    }

    // Skip rendering this element subgraph if possible.
    // IMPORTANT: The checks in Render for cleaning specific dirty flags need to reflect the order we check each condition here.
    //            If we clean the wrong dirty flag when multiple conditions are true, we may never detect the change that causes the
    //            subgraph to become visible again.
    if (   !skipRenderWhileInheritedCollapsed
        && !skipRenderWhileTransparent
        && !skipRenderWhileClippedOut
        && !skipRenderWhileLayoutClippedOut
        && !skipRenderWhileTransformTooSmall)
    {
        HWCompTreeNode *pParentCompNodeNoRef = myRP.pHWWalk->GetParentCompNode();
        HWCompNode *pLastChildAddedPresubgraphNoRef = myRP.pHWWalk->GetPreviousChildCompNode();

        // Note whether the isOpacityAnimating flag has changed - this might force more updates to realizations in the subgraph.
        myRP.hasOpacityAnimatingChanged = myRP.hasOpacityAnimatingChanged || pUIElement->PCHasOpacityAnimationDirty();

        // If creating a compositor node, don't push local properties here - creating the compositor node will
        // clone them for the render thread, in case any is the target of an independent animation.
        if (requiresCompositionNode)
        {
            // Because a new composition node is being created
            // record the opacity from the new composition node to the root
            myRP.opacityFromCompNodeToRoot *= combinedOpacityToCompNode;

            // Reset properties to defaults for walking into the subgraph.
            // Properties are now all relative to this node.
            combinedTransformsAndClipsToCompNode.Reset();
            combinedOpacityToCompNode = 1.0f;

            // Property changes above this node can be ignored. However, if this node is new, then the properties have
            // changed for all content below it because the frame of reference changed.
            myRP.hasPropertyToCompNodeChanged = pUIElement->PCHasCompositionNodeDirty() || myRP.forceUpdateCompNodes;

            // All manipulatable elements have pixel snapping applied to their CompNode.
            // Here we pixel snap the transform-to-root to match our notion of the transform
            // to DComp's actual transform used while compositing.  This allows us to more
            // accurately pixel snap our content, most notably text.
            if (pUIElement->IsManipulatable())
            {
                combinedTransformToRoot.PixelSnap();
            }
        }
        else
        {
            // If the element doesn't require composition, it should not have a cached composition peer.
            ASSERT(pUIElement->GetCompositionPeer() == nullptr);

            // The properties since the last comp node have changed if any of this element's properties changed, or if the frame
            // of reference changed because this element used to have a comp node and does not now.
            myRP.hasPropertyToCompNodeChanged = myRP.hasPropertyToCompNodeChanged
                                             || myRP.forceUpdateCompNodes
                                             || pUIElement->PCArePropertiesDirty()
                                             || pUIElement->PCHasCompositionNodeDirty();
        }

        // Push the updated properties on the stack.
        myRP.pTransformToRoot = &combinedTransformToRoot;
        myRP.pTransformsAndClipsToCompNode = &combinedTransformsAndClipsToCompNode;
        myRP.opacityToCompNode = combinedOpacityToCompNode;

        // Only elements that may be updated independently by the render thread need compositor nodes.
        // Cached elements do not require cache nodes (unlike cached elements in Silverlight), unless they are
        // being independently-animated or manipulated where they're treated just like any other element.
        if (requiresCompositionNode)
        {
            bool hasRealizationTransformAboveCompNodeChanged;

            // Create a composition node with current transform to-next-cache
            IFC_RETURN(EnsureCompositionPeer(
                pUIElement,
                parentRP,
                myRP,
                pRedirectionCompNode,
                requiresRedirectedDrawing ? &transformsFromRedirectionTargetToRedirectionCompNode : nullptr,
                pElementCompNode.ReleaseAndGetAddressOf(),
                pContentCompNode.ReleaseAndGetAddressOf(),
                spRenderDataParent.ReleaseAndGetAddressOf(),
                &hasRealizationTransformAboveCompNodeChanged
                ));

            // Set the parent comp node for the subgraph to this element's comp node.
            myRP.pHWWalk->SetParentCompNode(pElementCompNode);

            // The last child added to this element at this point is always its content node (the only child so far).
            // There's no need to cache this ptr on the UIElement as the last comp node in the subgraph, since there will always
            // be a post-subgraph node that overwrites the cache later in this method.
            // Note: When we're doing synchronous comp tree updates, this will be null. That's fine - a null reference
            // node will cause the next child comp node should be inserted at the beginning of the this comp node's child
            // collection, which is what we want.
            myRP.pHWWalk->SetPreviousChildCompNode(pContentCompNode);

            // There are two cases for the render data parent.
            // - If the content node is render-data, the render data parent will always owned by that node.
            // - If the content is some other type (swap chain, media), then the content primitive group is used in
            //    a special way. On a frame where this node was just created, it will be the split part of the preceding
            //    render data parent which will be split again and merged into the post-subgraph render-data node.
            //    On subsequent walks, spRenderDataParent will be nullptr. When the content is not a render-data node,
            //    it's not expected that any primitives will be added to spRenderDataParent in this subgraph.
            myRP.pHWWalk->SetCurrentContainer(spRenderDataParent.get());

            // This flag will only be TRUE in cases where the node has been carried over from last frame.
            myRP.hasRealizationTransformAboveCompNodeChanged |= hasRealizationTransformAboveCompNodeChanged;
        }

        xvector<CXamlLight*> myLights;

        // Pick up any lights that are set on this UIElement.XamlLights.
        // If there are lights, we'll put them in a new vector and set that vector on the render params. This avoids having to
        // remove those lights again when we walk out of this subtree.
        // If a new light has entered the tree, then we have to rewalk this subtree to pick up its targets.
        bool hasLightEnteredTree = false;

        if (pUIElement->OfTypeByIndex<KnownTypeIndex::RootVisual>())
        {
            // If there are lights set on the root scroll viewer, we assume that the app put them there with the intention of
            // applying them to the entire tree, and only placed them there because that's as high as the app can reach in the
            // public UIElement tree. Fish out those lights and put them at the real root, which is above the root scroll viewer.
            // This way those lights can reach the popup root, the full window media root, and the LTE root as well. Note that if
            // Window.Content is a canvas, then there will be no root scroll viewer, and we'll peek at the canvas instead.
            CRootVisual* rootVisualNoRef = static_cast<CRootVisual*>(pUIElement);
            CUIElement* rootScrollViewerOrCanvas = rootVisualNoRef->GetRootScrollViewerOrCanvas();
            if (rootScrollViewerOrCanvas != nullptr)
            {
                // Get the lights from the public root, but attach them to the root visual's comp node. The WUC light can only
                // target visuals under its CoordinateSpace visual, so we have to attach it up high.
                hasLightEnteredTree = AccumulateLightsAndCheckLightsEnteringTree(rootScrollViewerOrCanvas, rootVisualNoRef, &myLights, myRP.m_isInXamlIsland);
            }

            // Also remember what the root scroll viewer (or canvas) is. When we render it for real we want to skip its light collection
            // so that it doesn't get added twice. We cache it in the render params to avoid doing a walk up on every single element
            // to get the root scroll viewer.
            myRP.m_skipLightsOnThisElementNoRef = rootScrollViewerOrCanvas;
        }
        else if (pUIElement != myRP.m_skipLightsOnThisElementNoRef)
        {
            hasLightEnteredTree = AccumulateLightsAndCheckLightsEnteringTree(pUIElement, pUIElement, &myLights, myRP.m_isInXamlIsland);
        }

        if (myLights.size() > 0)
        {
            if (myRP.m_xamlLights != nullptr)
            {
                for (const auto& existingLight : *(myRP.m_xamlLights))
                {
                    myLights.push_back(existingLight);
                }
            }
            myRP.m_xamlLights = &myLights;
        }
        if (hasLightEnteredTree)
        {
            myRP.m_shouldRerenderSubtreeForNewLight = true;
        }

        //
        // Render the element's content and children.
        //
        HRESULT renderContentHR = RenderContentAndChildren(
            pUIElement,
            myRP,
            requiresRedirectedDrawing,
            pElementCompNode != nullptr /*elementHasCompNode*/
            );

        // If we got a device lost HR when rendering the content, and if we need a comp node, then continue rendering to make
        // sure we've called EnsurePostSubgraphNode. Otherwise we can leave the tree in a partially-split state, and hit problems
        // when recovering from the device loss. Namely, SpriteVisuals that should go into the post-subgraph node are instead
        // left inside the element's comp node. When the element cleans up its comp node, those SpriteVisuals all get released.
        // We then walk into the element's next siblings to run cleanup, but all their SpriteVisuals were just released, so we AV.
        if (requiresCompositionNode
            && (SUCCEEDED(renderContentHR) || GraphicsUtility::IsDeviceLostError(renderContentHR)))
        {
            // Now that we're done walking this subgraph, update the reference comp nodes one more time.
            // If the parent is nullptr, this is the root node and the walk is ending.
            if (pParentCompNodeNoRef != nullptr)
            {
                // The parent comp node is restored (back to the parent of this element's comp node).
                myRP.pHWWalk->SetParentCompNode(pParentCompNodeNoRef);

                // The last child added to this parent comp node is this element's comp node. This information is needed to correctly
                // place the next comp node in pParentCompNodeNoRef's child collection - it goes after pElementCompNode.
                myRP.pHWWalk->SetPreviousChildCompNode(pElementCompNode);

                // When synchronously updating the comp tree, there are no render data comp nodes, so there are no post subgraph nodes.
                // We'll update the WUC visual rendering context (where to add the next SpriteVisual and what visual to add it after)
                // in UpdateLastSpriteVisualAndCompNodeFromSubgraph when we return to Render.
                //
                // Note that there is an exception - in multi-path LTEs, where we clone comp nodes for an entire UIElement
                // subtree, UpdateLastSpriteVisualAndCompNodeFromSubgraph will not update anything. This is fine for rendering UIElements
                // that don't need a comp node, since those scenarios will walk and render the entire subtree unconditionally. But
                // for UIElements that need comp nodes, we still need to clone those comp nodes and build the cloned comp node tree,
                // which changes the rendering context like what parent to add new SpriteVisuals into. Make those updates here.
                myRP.pHWWalk->SetCurrentContainer(static_cast<HWCompTreeNodeWinRT*>(pParentCompNodeNoRef)->GetContainerVisualForChildren());
                myRP.pHWWalk->SetLastSpriteVisual(static_cast<HWCompTreeNodeWinRT*>(pElementCompNode.get())->GetReferenceVisualForIncrementalRendering());
            }
        }

        IFC_RETURN(renderContentHR);

        // Store the last leaf-node added in the subgraph as the previous child, if it exists.
        // If the last child added when walking into this element is the same when walking out, there were no comp nodes
        // in the subgraph and the cached ptr should be nullptr.
        // This state should not be modified in RTB or override walks.
        if (myRP.pHWWalk->GetOverrideRenderDataList() == nullptr)
        {
            HWCompNode *pLastCompNodeAddedInSubgraphNoRef = myRP.pHWWalk->GetPreviousChildCompNode();

            // With synchronous comp tree updates, there are no render data comp nodes, so there are no post subgraph nodes. The UIElement
            // will instead store the last comp tree node in its subtree. This comp node is used as a reference when adding new comp nodes
            // in the tree while the element is clean for rendering. Take this tree for example:
            //
            //  <Root HasCompNode>
            //      <Element A>
            //          <Element B HasCompNode />
            //      </Element A>
            //      <Element C />
            //      <Element D />
            //  </Root>
            //
            // If D got a comp node this frame, then it needs to be added to the Root's comp node, above the comp node for B. If the subtree
            // A and B are not dirty for rendering, then we won't walk into A, but we still need to pick up B's comp node in order to
            // place D's new comp node. We use A's "last comp node in subtree" for that.

            // Only store this comp node if the subtree added a new comp node. Otherwise the reference comp node didn't change
            // after walking into and out of this subtree, so we can use the reference comp node that we already have.
            // UpdateLastSpriteVisualAndCompNodeFromSubgraph has the matching GetLastCompNode null check.
            if (pLastChildAddedPresubgraphNoRef != pLastCompNodeAddedInSubgraphNoRef)
            {
                pUIElement->StoreLastCompNode(pLastCompNodeAddedInSubgraphNoRef);
            }
            else
            {
                pUIElement->StoreLastCompNode(nullptr);
            }
        }
    }

    *pSkipRenderWhileInheritedCollapsed = skipRenderWhileInheritedCollapsed;
    *pSkipRenderWhileTransparent = skipRenderWhileTransparent;
    *pSkipRenderWhileClippedOut = skipRenderWhileClippedOut;
    *pSkipRenderWhileLayoutClippedOut = skipRenderWhileLayoutClippedOut;
    *pSkipRenderWhileTransformTooSmall = skipRenderWhileTransformTooSmall;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the element that should be used to provide the transform
//      for rendering the specified redirected element.
//
//------------------------------------------------------------------------
/*static*/ _Ret_maybenull_ CUIElement*
HWWalk::GetRedirectionTarget(_In_ CUIElement *pUIE)
{
    const KnownTypeIndex index = pUIE->GetTypeIndex();
    if (index == KnownTypeIndex::Popup)
    {
        // Popups are walked by the redirection root, but are also used as the target
        // for picking up position in the regular visual tree.
        return pUIE;
    }
    else if (index == KnownTypeIndex::LayoutTransitionElement)
    {
        // Layout transition elements are walked by the redirection root, and they
        // use their target element to pick up position, etc, in the regular visual tree.
        CLayoutTransitionElement *pLTE = static_cast<CLayoutTransitionElement*>(pUIE);
        return pLTE->m_pTarget;
    }
    else
    {
        // Not an element that requires redirected drawing.
        return NULL;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if a redirected element is absolutely positioned.
//      This means its content is still taken from its redirection target,
//      but its position is not.
//
//------------------------------------------------------------------------
/*static*/ bool
HWWalk::IsAbsolutelyPositioned(_In_ CUIElement *pUIE)
{
    // Only valid to call this on redirected elements.
    ASSERT(GetRedirectionTarget(pUIE) != NULL);

    const KnownTypeIndex index = pUIE->GetTypeIndex();
    if (index == KnownTypeIndex::LayoutTransitionElement)
    {
        const CLayoutTransitionElement *pLTE = static_cast<const CLayoutTransitionElement*>(pUIE);
        return pLTE->IsAbsolutelyPositioned();
    }
    else
    {
        // Other redirected elements are not absolutely positioned.
        return false;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pushes a hardware acceleratable clip into the clip in the render
//      params. We only support rectangular clips in Jupiter, but with
//      transforms on clips these may be rectangular but represented
//      in point form, so we use the HWClip class.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::IntersectClipInCompNodeSpace(
    _In_opt_ CGeometry *pClip,
    _Inout_ TransformAndClipStack *pCombinedTransformsAndClipsToCompNode,
    _Out_ bool *pSkipRenderWhileClippedOut
    )
{
    if (pClip != NULL)
    {
        ASSERT(pClip->GetTypeIndex() == KnownTypeIndex::RectangleGeometry);

        CRectangleGeometry *pRectangle = static_cast<CRectangleGeometry *>(pClip);
        ASSERT(IsCloseReal(pRectangle->m_eRadiusX, 0.0f) && IsCloseReal(pRectangle->m_eRadiusY, 0.0f));

        IFC_RETURN(CRectangleGeometry::ApplyClip(pRectangle, pCombinedTransformsAndClipsToCompNode));
    }

    *pSkipRenderWhileClippedOut = pCombinedTransformsAndClipsToCompNode->HasZeroSizedClip();

    return S_OK;
}

/*static*/ void
HWWalk::IntersectClipRectInCompNodeSpace(
    XRECTF& clipRect,
    _Inout_ TransformAndClipStack *combinedTransformsAndClipsToCompNode,
    _Out_ bool *skipRenderWhileClippedOut
    )
{
    ASSERT(!IsInfiniteRectF(clipRect));

    HWClip localHWClip;
    localHWClip.Set(&clipRect);

    IFCFAILFAST(combinedTransformsAndClipsToCompNode->IntersectLocalSpaceClip(&localHWClip));

    *skipRenderWhileClippedOut = combinedTransformsAndClipsToCompNode->HasZeroSizedClip();
}

/*static*/
bool HWWalk::IsShapeThatRequiresSoftwareRendering(_In_ CUIElement* pUIElement)
{
    return (pUIElement->OfTypeByIndex<KnownTypeIndex::Shape>() &&
           (pUIElement->GetTypeIndex() != KnownTypeIndex::Rectangle || IsShapeMaskRequiredForRectangle(static_cast<CRectangle*>(pUIElement))));
}

/*static*/
bool HWWalk::IsBorderLikeElementThatRequiresSoftwareRendering(_In_ CUIElement* pUIElement)
{
    bool result = false;
    CFrameworkElement * pFrameworkElement = nullptr;

    if (pUIElement->OfTypeByIndex<KnownTypeIndex::Border>())
    {
        pFrameworkElement = static_cast<CBorder*>(pUIElement);
    }
    else if (pUIElement->OfTypeByIndex<KnownTypeIndex::Panel>())
    {
        pFrameworkElement = static_cast<CPanel*>(pUIElement);
    }
    else if (pUIElement->OfTypeByIndex<KnownTypeIndex::ContentPresenter>())
    {
        pFrameworkElement = static_cast<CContentPresenter*>(pUIElement);
    }

    if (pFrameworkElement != nullptr)
    {
        result = !!IsShapeMaskRequiredForBorder(pFrameworkElement);
    }

    return result;
}

/*static*/
bool HWWalk::ShouldImageUseNineGrid(_In_ CImage* pImage)
{
    // When a NineGrid Image has Stretch Mode == None and its parent is a Canvas.
    // We don't do any NineGrid stretching and draw the image at its source size.
    if (pImage->HasNinegrid())
    {
        CUIElement* pParent = pImage->GetUIElementAdjustedParentInternal(FALSE);

        if (!(pImage->m_Stretch == DirectUI::Stretch::None &&
              pParent != nullptr &&
              pParent->GetTypeIndex() == KnownTypeIndex::Canvas))
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Content rendering
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::LayoutTransitionElementRenderTarget(
    _In_ CLayoutTransitionElement *pLTElement,
    _In_ const HWRenderParams &lteRP
    )
{
    if (pLTElement->m_pTarget != NULL)
    {
        ASSERT(!pLTElement->m_pTarget->IsRenderWalkRoot());

        // Have to clone the HWRenderParams here because we're skipping the call to Render/RenderProperties that normally does it.
        HWRenderParams targetRP(lteRP);
        // TODO: INCWALK: Is this all I need to do to safely copy?

        // LTEs targeting other LTEs are unexpected, but LTEs targeting Popups are supported.
        const bool requiresRedirectedDrawing = pLTElement->m_pTarget->IsRedirectionElement();

        // Call directly into RenderContentAndChildren in order to skip applying the target element's own properties, and any animations
        // affecting them. This also skips the early-exit check for the target element subgraph being completely clean, so we'll iterate
        // through its content and immediate children, but if those are all clean we still won't regenerate anything.
        IFC_RETURN(RenderContentAndChildren(
            pLTElement->m_pTarget,
            targetRP,
            requiresRedirectedDrawing,
            FALSE /*elementHasCompNode, the target will not have one of its own because we skipped its properties intentionally*/
            ));

        // Update the render walk ptr into the render data list with the last SpriteVisual contributed from the target's subgraph.
        // This is normally done in Render, but since we skipped directly to RenderContentAndChildren it needs to be updated here too.
        lteRP.pHWWalk->UpdateLastSpriteVisualAndCompNodeFromSubgraph(pLTElement->m_pTarget, targetRP);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders the Popup's content (Popup.Child)
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::PopupRenderChild(
    _In_ CPopup *pPopup,
    _In_ const HWRenderParams &myRP
    )
{
    CUIElement* popupChild;

    popupChild = pPopup->m_unloadingChild;
    if (popupChild != nullptr)
    {
        ASSERT(!popupChild->IsRenderWalkRoot());
        IFC_RETURN(Render(popupChild, myRP, FALSE /* redirectedDraw */));
    }

    // If the popup is only rendering a hide animation on a previous child, it will not have a current child.
    popupChild = pPopup->m_pChild;
    if (popupChild != nullptr)
    {
        ASSERT(!popupChild->IsRenderWalkRoot());
        IFC_RETURN(Render(popupChild, myRP, FALSE /* redirectedDraw */));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders element's content, as needed, and proceeds recursively into the subgraph.
//      Should only be called if the element or something in its subgraph is dirty.
//      Always updates the UIElement's cached ptr in the render data list as a result.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT HWWalk::RenderContentAndChildren(
    _In_ CUIElement *pUIElement,
    _Inout_ HWRenderParams& myRP,
    bool requiresRedirectedDrawing,
    bool elementHasCompNode
    ) noexcept
{
    bool setRenderDataOverride = false;
    bool hasBrushAnimation = pUIElement->IsBrushIndependentlyAnimating() || pUIElement->HasActiveBrushTransitions();

    HWElementRenderParams myElementRP;
    if (hasBrushAnimation)
    {
        xref_ptr<CSolidColorBrush> fillBrush;
        xref_ptr<CSolidColorBrush> strokeBrush;
        pUIElement->GetIndependentlyAnimatedBrushes(fillBrush.ReleaseAndGetAddressOf(), strokeBrush.ReleaseAndGetAddressOf());

        WUCBrushManager* wucBrushManager = pUIElement->GetDCompTreeHost()->GetWUCBrushManager();

        if (fillBrush)
        {
            myElementRP.isFillBrushAnimating = !!fillBrush->IsIndependentTarget() || wucBrushManager->HasActiveBrushTransition(pUIElement, ElementBrushProperty::Fill);
        }

        if (strokeBrush)
        {
            myElementRP.isStrokeBrushAnimating = !!strokeBrush->IsIndependentTarget() || wucBrushManager->HasActiveBrushTransition(pUIElement, ElementBrushProperty::Stroke);
        }
    }

    pUIElement->EnsurePropertyRenderData(RWT_WinRTComposition);

    // LayoutTransitionElements allow for multi-path rendering of their target element in a single frame.
    // The target subgraph needs multiple versions of its render data in the scene, connected to different comp nodes,
    // but elements only support caching one copy of their data for now. To work around this limitation, the LTE
    // itself manages the extra versions of the subgraph's render data separately. The render walk can proceed
    // multiple times (to generate nested comp nodes, etc), just inserting render data into a different list.
    //
    // The one downside of this approach is that these subsequent walks must assume all content is dirty, because
    // all the dirty flag management and incremental walking relies on the element's own caches, which aren't
    // being used here.
    if (requiresRedirectedDrawing && pUIElement->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        CLayoutTransitionElement *pLTE = static_cast<CLayoutTransitionElement*>(pUIElement);

        // If there's already overridden storage, it means this is a multi-path transition nested inside another multi-path transition.
        // This should work fine - the outer primary transition would use the multi-path storage here, but the other outer transitions
        // will use their own multi-path storage for the entire tree, including _all_ render walk paths for the inner element.
        if (!pLTE->IsPrimaryTransitionForTarget() && myRP.pHWWalk->GetOverrideRenderDataList() == nullptr)
        {
            // For now, overridden render data is always completely re-created each UI thread frame, so clear any old data first.
            pLTE->ClearSecondaryRenderData();

            PCRenderDataList *pOverrideList = pLTE->GetSecondaryTransitionRenderDataNoRef();

            myRP.pHWWalk->SetOverrideRenderDataList(pOverrideList);
            setRenderDataOverride = true;

            // Force the render walk to regenerate all content in the subgraph so that we get a complete version for the override list.
            myRP.forceRedraw = TRUE;
        }
    }

    PCRenderDataList* pOverrideRenderData = myRP.pHWWalk->GetOverrideRenderDataList();
    WUComp::IVisual* currentOverrideVisualNoRef = nullptr;

    bool isContentDirty; // intentionally uninitialized, set in each branch below

    // There's no need to clear local render data if the list we're contributing to was overridden. This allows the
    // subgraph to be walked and produce content multiple times in the same frame. The override walk does
    // _not_ manage the elements in the subgraph being in the scene.
    if (pOverrideRenderData == nullptr)
    {
        if (pUIElement->IsInPCScene())
        {
            // If an ancestor is entering the scene, nothing in the subgraph should _already be_ in the scene.
            // If this element is being walked multiple times (i.e. if there's override render data) this might not be true.
            ASSERT(!myRP.isEnteringScene);

            // This element's content is dirty if it changed directly, or the element inherited dirtiness from an ancestor.
            isContentDirty = myRP.HasInheritedDirtiness() || pUIElement->NWIsContentDirty();

            // If the content is going to be regenerated, clear PC composition flags but keep the caches
            if (isContentDirty)
            {
                pUIElement->ReEnterPCScene();
            }
        }
        else
        {
            // If this element was not in the scene, the entire subgraph should not have been in the scene.
            // Now that this element is entering the scene, we need to be sure to walk the entire subgraph to add
            // everything else to the scene as well, even if the individual elements are not dirty.
            myRP.isEnteringScene = TRUE;
            isContentDirty = true;

            pUIElement->EnterPCScene();

            // If there is a UIA client listening, register this element to have the StructureChanged
            // automation event fired for it.
            if (myRP.pHWWalk->m_UIAClientsListeningToStructure)
            {
                pUIElement->RegisterForStructureChangedEvent(
                    AutomationEventsHelper::StructureChangedType::Added);
            }
        }
    }
    else
    {
        ASSERT(myRP.forceRedraw);
        isContentDirty = true;

        // The root element of the override walk needs to be in the scene, as usual - it's everything in the subgraph
        // (the LTE's target and below) that should not have its state modified by this override walk.
        if (setRenderDataOverride)
        {
            pUIElement->EnterPCScene();
        }
    }

    if (pUIElement->IsEntireSubtreeDirty())
    {
        myRP.forceRedraw = TRUE;
    }

    // Now, proceed with rendering the content and the children.
    {
        // Populated if needed. Reused to render post children text content.
        CTransformToRoot textTransformToRoot;

        IContentRenderer * pContentRenderer = myRP.pContentRenderer;
        ElementRenderingContext context(pContentRenderer, pUIElement, &myRP, &myElementRP, &textTransformToRoot);
        RealizationUpdateContext rucontext(pContentRenderer);

        const KnownTypeIndex typeIndex = pUIElement->GetTypeIndex();
        WUComp::IVisual* lastSpriteVisualBeforeElementNoRef = myRP.pHWWalk->GetLastSpriteVisual();

        // Use overridden storage if it exists, otherwise use the appropriate storage on the element itself.
        PCRenderDataList *pPreChildrenRenderData = pOverrideRenderData;
        if (pPreChildrenRenderData == nullptr)
        {
            IFC_RETURN(pUIElement->GetPCPreChildrenRenderDataNoRef(&pPreChildrenRenderData));
        }
        else if (!pOverrideRenderData->empty())
        {
            IFC_RETURN(pOverrideRenderData->back(currentOverrideVisualNoRef));
        }

        // The UI thread rasterization walk will ignore elements that need to be drawn with redirection.
        // Draw the current element's content, and then its children.

        // Do pre-children content rendering if necessary.
        if (isContentDirty)
        {
            myRP.pHWWalk->m_elementsRendered++;

            CaptureRenderData capture(pContentRenderer, pPreChildrenRenderData, pOverrideRenderData != nullptr);
            ASSERT(pPreChildrenRenderData == nullptr || pPreChildrenRenderData->empty() || pOverrideRenderData != nullptr);

            // Check for elements that call into software walks for content rasterization.
            if (IsShapeThatRequiresSoftwareRendering(pUIElement))
            {
                CShape *pShapeElement = static_cast<CShape*>(pUIElement);

                if (myRP.pHWWalk->UseWUCShapes())
                {
                    // Render shapes with new WUC APIs
                    IFC_RETURN(pContentRenderer->RenderShape(pShapeElement));
                }
                else
                {
                    ShapeStrokePart strokeMaskPart(pShapeElement);
                    ShapeFillPart fillMaskPart(pShapeElement);
                    IFC_RETURN(pContentRenderer->MaskCombinedRenderHelper(
                        pShapeElement,
                        &strokeMaskPart,
                        &fillMaskPart,
                        false   // forceMaskDirty
                        ));
                }

                IFC_RETURN(pShapeElement->NotifyRenderContent(HWRenderVisibility::Visible));
            }
            else
            {
                // Otherwise, the element's content will be rendered with WinRT.
                IFC_RETURN(pUIElement->EnsureContentRenderData(RWT_WinRTComposition));

                if (typeIndex == KnownTypeIndex::PopupRoot)
                {
                    IFC_RETURN(pContentRenderer->PopupRootRenderContent(static_cast<CPopupRoot*>(pUIElement)));
                }
                else if (pUIElement->OfTypeByIndex<KnownTypeIndex::Panel>())
                {
                    // Background/Border drawing for all CPanel derived types
                    IFC_RETURN(pContentRenderer->PanelRenderContent(static_cast<CPanel *>(pUIElement)));
                }
                else if (pUIElement->OfTypeByIndex<KnownTypeIndex::ContentPresenter>())
                {
                    if (typeIndex == KnownTypeIndex::ListViewItemPresenter || typeIndex == KnownTypeIndex::GridViewItemPresenter)
                    {
                        IFC_RETURN(pContentRenderer->ListViewBaseItemChromeRenderLayer(
                            static_cast<CListViewBaseItemChrome *>(pUIElement),
                            ListViewBaseItemChromeLayerPosition_PrimaryChrome_Pre
                            ));
                    }
                    else
                    {
                        // Background/Border drawing for ContentPresenter
                        IFC_RETURN(pContentRenderer->ContentPresenterRenderContent(static_cast<CContentPresenter *>(pUIElement)));
                    }
                }
                else if (pUIElement->OfTypeByIndex<KnownTypeIndex::UserControl>() && static_cast<CUserControl *>(pUIElement)->IsAllowForBackgroundRender())
                {
                    // Background drawing is enabled for this UserControl.
                    IFC_RETURN(pContentRenderer->UserControlRenderBackground(static_cast<CUserControl *>(pUIElement)));
                }
                else
                {
                    switch (typeIndex)
                    {
                        case KnownTypeIndex::Image:
                            IFC_RETURN(pContentRenderer->ImageRenderContent(static_cast<CImage *>(pUIElement)));
                            break;
                        case KnownTypeIndex::Glyphs:
                            myRP.ExtractTransformToRoot(&textTransformToRoot);
                            IFC_RETURN(pContentRenderer->GlyphsRenderContent(static_cast<CGlyphs *>(pUIElement)));
                            break;
                        case KnownTypeIndex::TextBlock:
                            myRP.ExtractTransformToRoot(&textTransformToRoot);
                            IFC_RETURN(pContentRenderer->TextBlockRenderContent(static_cast<CTextBlock *>(pUIElement)));
                            break;
                        case KnownTypeIndex::RichTextBlock:
                            myRP.ExtractTransformToRoot(&textTransformToRoot);
                            IFC_RETURN(pContentRenderer->RichTextBlockRenderContent(static_cast<CRichTextBlock *>(pUIElement)));
                            break;
                        case KnownTypeIndex::RichTextBlockOverflow:
                            myRP.ExtractTransformToRoot(&textTransformToRoot);
                            IFC_RETURN(pContentRenderer->RichTextBlockOverflowRenderContent(static_cast<CRichTextBlockOverflow *>(pUIElement)));
                            break;
                        case KnownTypeIndex::TextBoxView:
                            myRP.ExtractTransformToRoot(&textTransformToRoot);
                            IFC_RETURN(pContentRenderer->TextBoxViewRenderContent(static_cast<CTextBoxView *>(pUIElement)));
                            break;
                        case KnownTypeIndex::Rectangle: // simple rectangles
                            if (myRP.pHWWalk->UseWUCShapes())
                            {
                                IFC_RETURN(pContentRenderer->RenderShape(static_cast<CShape*>(pUIElement)));
                            }
                            else
                            {
                                IFC_RETURN(pContentRenderer->RectangleRenderContent(static_cast<CRectangle*>(pUIElement)));
                            }
                            break;
                        case KnownTypeIndex::Border:
                            IFC_RETURN(pContentRenderer->BorderRenderContent(static_cast<CBorder *>(pUIElement)));
                            break;
                        case KnownTypeIndex::ContentControl:
                            IFC_RETURN(pContentRenderer->ContentControlItemRender(static_cast<CContentControl *>(pUIElement)));
                            break;
                        case KnownTypeIndex::ListViewBaseItemSecondaryChrome:
                            IFC_RETURN(pContentRenderer->ListViewBaseItemChromeRenderLayer(
                                static_cast<CListViewBaseItemSecondaryChrome *>(pUIElement)->m_pPrimaryChromeNoRef,
                                ListViewBaseItemChromeLayerPosition_SecondaryChrome_Pre
                                ));
                            break;
                        case KnownTypeIndex::CalendarViewItem:
                        case KnownTypeIndex::CalendarViewDayItem:
                            IFC_RETURN(pContentRenderer->CalendarViewBaseItemChromeRenderLayer(
                                static_cast<CCalendarViewBaseItemChrome*>(pUIElement),
                                CalendarViewBaseItemChromeLayerPosition::Pre
                                ));
                            break;
                        default:
                            // Nothing to draw or not supported in PC
                            break;
                    }
                }
            }
        }

        IFC_RETURN(myRP.pHWWalk->UpdateLastSpriteVisualFromContent(pPreChildrenRenderData, currentOverrideVisualNoRef));

        // Determine if we're about to render the subtree of a SwapChainPanel.
        // There are 3 cases:
        // 1) This element is a SwapChainPanel
        // 2) This element is a SwapChainBackgroundPanel
        // 3) This element is a redirection element (eg Popup) and was earlier detected as being the child of the above 2
        bool inSwapChainPanelSubtreeOld = myRP.pHWWalk->m_inSwapChainPanelSubtree;
        if (typeIndex == KnownTypeIndex::SwapChainPanel ||
            typeIndex == KnownTypeIndex::SwapChainBackgroundPanel ||
            pUIElement->IsRedirectedChildOfSwapChainPanel())
        {
            myRP.pHWWalk->m_inSwapChainPanelSubtree = true;
        }

        // Update the element if it has a comp node and needs retargeting.
        // If it was a light target but isn't anymore and has lost its comp node, then it should have untargeted itself when
        // it cleaned up its comp node.
        if ((isContentDirty || pUIElement->m_isLightTargetDirty)
            && pUIElement->RequiresCompositionNode())
        {
            UpdateLightTargets(pUIElement, myRP);
        }

        // Iterate through children. This will render them if they're dirty, and update the last SpriteVisual regardless.
        IFC_RETURN(RenderChildren(pUIElement, myRP));

        // Restore the flag to what it was before we walked this subtree.
        myRP.pHWWalk->m_inSwapChainPanelSubtree = inSwapChainPanelSubtreeOld;

        // Use overridden storage if it exists, otherwise use the appropriate storage on the element itself.
        PCRenderDataList *pPostChildrenRenderData = pOverrideRenderData;
        if (pPostChildrenRenderData == nullptr)
        {
            IFC_RETURN(pUIElement->GetPCPostChildrenRenderDataNoRef(&pPostChildrenRenderData));
        }
        else if (!pOverrideRenderData->empty())
        {
            IFC_RETURN(pOverrideRenderData->back(currentOverrideVisualNoRef));
        }

        // Do post-children rendering (caret, selection, etc.) if necessary.
        if (isContentDirty)
        {
            CaptureRenderData capture(pContentRenderer, pPostChildrenRenderData, pOverrideRenderData != nullptr);
            ASSERT(pPostChildrenRenderData == nullptr || pPostChildrenRenderData->empty() || pOverrideRenderData != nullptr);

            // Check for border-like elements that have rounded corner clipping
            if (pUIElement->RequiresCompNodeForRoundedCorners())
            {
                if (typeIndex == KnownTypeIndex::Border ||
                    typeIndex == KnownTypeIndex::CalendarViewItem ||
                    typeIndex == KnownTypeIndex::CalendarViewDayItem ||
                    pUIElement->OfTypeByIndex<KnownTypeIndex::Panel>() ||
                    (pUIElement->OfTypeByIndex<KnownTypeIndex::ContentPresenter>() &&
                        !(typeIndex == KnownTypeIndex::ListViewItemPresenter || typeIndex == KnownTypeIndex::GridViewItemPresenter)))
                {
                    IFC_RETURN(pContentRenderer->BorderLikeElementPostChildrenRender(static_cast<CFrameworkElement*>(pUIElement)));
                }
                else
                {
                    // CListViewBaseItemChrome deriving from CContentPresenter, ListViewItemPresenter/GridViewItemPresenter can get a CornerRadius.
                    // The CListViewBaseItemChrome's inner/outer borders and backplate apply the CornerRadius instead of the parent ContentPresenter
                    // so the BorderLikeElementPostChildrenRender call is skipped.
                    ASSERT(typeIndex == KnownTypeIndex::ListViewItemPresenter || typeIndex == KnownTypeIndex::GridViewItemPresenter);
                }
            }
            else
            {
                switch (typeIndex)
                {
                    case KnownTypeIndex::TextBlock:
                        IFC_RETURN(pContentRenderer->TextBlockPostChildrenRender(static_cast<CTextBlock *>(pUIElement)));
                        break;

                    case KnownTypeIndex::RichTextBlock:
                        IFC_RETURN(pContentRenderer->RichTextBlockPostChildrenRender(static_cast<CRichTextBlock *>(pUIElement)));
                        break;

                    case KnownTypeIndex::RichTextBlockOverflow:
                        IFC_RETURN(pContentRenderer->RichTextBlockOverflowPostChildrenRender(static_cast<CRichTextBlockOverflow *>(pUIElement)));
                        break;
                    case KnownTypeIndex::ContentControl:
                        IFC_RETURN(pContentRenderer->ContentControlPostChildrenRender(static_cast<CContentControl *>(pUIElement)));
                        break;
                    case KnownTypeIndex::ListViewBaseItemSecondaryChrome:
                        IFC_RETURN(pContentRenderer->ListViewBaseItemChromeRenderLayer(
                            static_cast<CListViewBaseItemSecondaryChrome *>(pUIElement)->m_pPrimaryChromeNoRef,
                            ListViewBaseItemChromeLayerPosition_SecondaryChrome_Post
                            ));
                        break;
                    case KnownTypeIndex::ListViewItemPresenter:
                    case KnownTypeIndex::GridViewItemPresenter:
                        IFC_RETURN(pContentRenderer->ListViewBaseItemChromeRenderLayer(
                            static_cast<CListViewBaseItemChrome *>(pUIElement),
                            ListViewBaseItemChromeLayerPosition_PrimaryChrome_Post
                            ));
                        break;
                    case KnownTypeIndex::CalendarViewItem:
                    case KnownTypeIndex::CalendarViewDayItem:
                        IFC_RETURN(pContentRenderer->CalendarViewBaseItemChromeRenderLayer(
                            static_cast<CCalendarViewBaseItemChrome*>(pUIElement),
                            CalendarViewBaseItemChromeLayerPosition::Post
                            ));
                        break;
                    default:
                        break;
                }
            }

            // Draw the focus rect, if pUIElement is responsible for drawing it.
            CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(pUIElement);
            focusManager->RenderFocusRectForElementIfNeeded(pUIElement, pContentRenderer);
        }

        IFC_RETURN(myRP.pHWWalk->UpdateLastSpriteVisualFromContent(pPostChildrenRenderData, currentOverrideVisualNoRef));

        // We didn't skip rendering so all dirty state was consumed; clean all dirty flags on this element.
        // The recursive HWWalk will have cleaned flags as needed on elements in the subgraph.
        //
        // This gets tricky if we reached this element through redirected rendering via a LayoutTransitionElement,
        // because there can be multiple LTEs that all point to this element. In that case, there is a "primary"
        // LTE and many "secondary" LTEs. Every LTE will explicitly render the target element, which means the target
        // gets rendered several times. However, the element has only one place to cache render data like comp nodes
        // and sprite visuals. The primary LTE is the one responsible for the render walk where we cache this render
        // data. All secondary LTEs will create temporary render data that's not stored on the target element's subtree,
        // but rather on the secondary LTE. This is the "OverrideRenderDataList". The secondary LTEs also force a full
        // redraw of the target subtree, so that it can re-create the temporary render data.
        //
        // Since the secondary LTEs aren't responsible for creating the cached render data, they shouldn't be
        // responsible for clearing dirty flags, either. We leave that for the primary LTE to do. The case we want
        // to avoid is for a secondary LTE to render the target subtree first, not cache anything, yet clean all the
        // dirty flags. The primary LTE renders later, sees that the subtree is marked clean, skips rendering, and
        // skips updating the cached render data pointers. The cached render data pointers can then be left dangling
        // once old comp nodes from the previous frame are released.
        if (myRP.pHWWalk->GetOverrideRenderDataList() != nullptr)
        {
            // This is part of a subtree that's rendering due to a secondary LTE, which forces a redraw on its target
            // subtree. Don't clear dirty flags.
            ASSERT(myRP.forceRedraw);

            // The exception is if we're rendering the secondary LTE itself. We do want to clean dirty flags in that
            // case. The secondary LTE is the one that set the OverrideRenderDataList and marked this flag.
            if (setRenderDataOverride)
            {
                pUIElement->NWCleanDirtyFlags();
            }
        }
        else
        {
            // If there's no OverrideRenderDataList, then either we're rendering due to a primary LTE, or we're not
            // doing redirected rendering at all. Clear the dirty flags in both cases.
            pUIElement->NWCleanDirtyFlags();
        }

        // Cache the last SpriteVisual for the entire subgraph on the element itself, so we can pick up in the right place
        // to continue adding content on an incremental walk.
        // Don't touch these caches if this is a multi-path walk where we're using overridden storage, unless this is
        // the element that owns the override storage itself.
        if (pOverrideRenderData == nullptr || setRenderDataOverride)
        {
            WUComp::IVisual* lastSpriteVisualInElementSubgraphNoRef = myRP.pHWWalk->GetLastSpriteVisual();

            // If the render data ptr didn't change after rendering content and subgraph, then this subgraph is empty
            // and the cache should be cleared.
            if (lastSpriteVisualBeforeElementNoRef != lastSpriteVisualInElementSubgraphNoRef)
            {
                pUIElement->StoreLastSpriteVisual(lastSpriteVisualInElementSubgraphNoRef);
            }
            else
            {
                pUIElement->StoreLastSpriteVisual(nullptr);
            }
        }

        // Cache whether the transform was animating for redirection elements, so they can determine when the bit
        // changes in RenderProperties.
        if (requiresRedirectedDrawing)
        {
            pUIElement->SetWasRedirectedTransformAnimating(myRP.isTransformAnimating);
        }
    }

    // Now that this element is done rendering, we're done overriding where render data is stored.
    if (setRenderDataOverride)
    {
        myRP.pHWWalk->SetOverrideRenderDataList(nullptr);
    }
    else
    {
        // Ensure the override render data list is set/unset correctly during the walk. No nesting is allowed.
        ASSERT(myRP.pHWWalk->GetOverrideRenderDataList() == pOverrideRenderData);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders this node's children.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::RenderChildren(
    _In_ CUIElement *pUIElement,
    _In_ const HWRenderParams &rp
    )
{
    const KnownTypeIndex typeIndex = pUIElement->GetTypeIndex();
    switch (typeIndex)
    {
        case KnownTypeIndex::ConnectedAnimationRoot :
            IFC_RETURN(RenderConnectedAnimationUnloadingElements(static_cast<CConnectedAnimationRoot*>(pUIElement), rp));
            break;

        case KnownTypeIndex::PopupRoot:
            IFC_RETURN(PopupRootRenderChildren(static_cast<CPopupRoot*>(pUIElement), rp));
            break;

        case KnownTypeIndex::Popup:
            IFC_RETURN(PopupRenderChild(static_cast<CPopup*>(pUIElement), rp));
            break;

        case KnownTypeIndex::LayoutTransitionElement:
            IFC_RETURN(LayoutTransitionElementRenderTarget(static_cast<CLayoutTransitionElement*>(pUIElement), rp));
            break;

        case KnownTypeIndex::RenderTargetBitmapRoot:
            // The subgraph under RenderTargetBitmapRoot need not be rendered.
            // Needed element subtree will be picked in a separate render walk to RenderTargetBitmaps.
            break;

        case KnownTypeIndex::CalendarViewItem:
        case KnownTypeIndex::CalendarViewDayItem:
            IFC_RETURN(CalendarViewBaseItemRenderChildren(static_cast<CCalendarViewBaseItemChrome*>(pUIElement), rp));
            break;

        case KnownTypeIndex::XamlIslandRootCollection:
            // We handle rendering the XamlIslandRoot roots in separate render walks
            break;

        default:
        {
            // TODO: INCWALK: The 'redirection' flag shouldn't be needed for LTEs since they're always parented to TransitionRoots, and TransitionRoots are special children outside the regular collection
            const bool redirectedDrawForChildren = (typeIndex == KnownTypeIndex::TransitionRoot);

            IFC_RETURN(RenderChildrenDefault(pUIElement, rp, redirectedDrawForChildren));
        }
    }

    // Special case for LTE embedded in the tree in the HWWalk.
    // GetChildrenInRenderOrder does not return these transition roots to limit the impact of the
    // feature to just the HWWalk, since that's the only place they're supported.
    CTransitionRoot* localTransitionRootNoRef = pUIElement->GetLocalTransitionRoot(false);
    if (localTransitionRootNoRef)
    {
        // Transition roots themselves never need redirected drawing.
        IFC_RETURN(Render(localTransitionRootNoRef, rp, FALSE /*redirectedDraw*/));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method which iterates children of an element
//      and calls render on them.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::RenderChildrenDefault(
    _In_ CUIElement *pUIElement,
    _In_ const HWRenderParams &rp,
    bool redirectedDrawForChildren
    )
{
    XUINT32 childCount;
    CUIElement **ppUIElements;

    pUIElement->GetChildrenInRenderOrder(
        &ppUIElements,
        &childCount
        );

    for (XUINT32 i = 0; i < childCount; i++)
    {
        ASSERT(!ppUIElements[i]->IsRenderWalkRoot());
        IFC_RETURN(Render(ppUIElements[i], rp, redirectedDrawForChildren));
    }

    return S_OK;
}


/*static*/ _Check_return_ HRESULT
HWWalk::CalendarViewBaseItemRenderChildren(
    _In_ CCalendarViewBaseItemChrome *pUIElement,
    _In_ const HWRenderParams &rp)
{
    unsigned childCount = 0;
    CUIElement **ppUIElements = nullptr;

    pUIElement->GetChildrenInRenderOrder(
        &ppUIElements,
        &childCount
        );

    // 1. render outer border then template child when present
    auto outerBorder = pUIElement->GetOuterBorder();
    auto templateChild = pUIElement->GetFirstChildNoAddRef();

    if (outerBorder || templateChild)
    {
        if (outerBorder)
        {
            IFC_RETURN(Render(outerBorder, rp, FALSE /* redirectedDraw */));
        }

        if (templateChild)
        {
            IFC_RETURN(Render(templateChild, rp, FALSE /* redirectedDraw */));
        }

        // CalendarView does its own tree walk so we have to establish a new rendering context.
        // BUGBUG: This breaks on redirected render
        PCRenderDataList *pTemplateChildPostChildrenRenderData = nullptr;
        CUIElement* childNoRef = templateChild ? templateChild : outerBorder;

        IFC_RETURN(childNoRef->GetPCPostChildrenRenderDataNoRef(&pTemplateChildPostChildrenRenderData));

        {
            IContentRenderer* pContentRenderer = rp.pContentRenderer;
            ElementRenderingContext context(pContentRenderer, pUIElement, &rp, nullptr, nullptr);

            {
                CaptureRenderData capture(pContentRenderer, pTemplateChildPostChildrenRenderData, true /* fAppendOnly */);

                // 2. render density bars
                IFC_RETURN(pUIElement->RenderDensityBars(pContentRenderer));
            }
        }
    }

    // 3. render other children
    for (XUINT32 i = 0; i < childCount; i++)
    {
        if (ppUIElements[i] == templateChild || ppUIElements[i] == outerBorder)
            continue;
        ASSERT(!ppUIElements[i]->IsRenderWalkRoot());
        IFC_RETURN(Render(ppUIElements[i], rp, FALSE /* redirectedDraw */));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders this PopupRoot's children.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::PopupRootRenderChildren(
    _In_ CPopupRoot *pPopupRoot,
    _In_ const HWRenderParams &rp
    )
{
    HRESULT hr = S_OK;

    CXcpList<CPopup> *pChildren = NULL;

    if (pPopupRoot->m_pOpenPopups)
    {
        // The popup root's CDependencyObject::m_pChildren are a list of popup contents. We want to render the
        // popups themselves.

        pChildren = new CXcpList<CPopup>;

        // Get the FIFO ordered list
        // This includes unloading popups, which get rendered too.
        pPopupRoot->m_pOpenPopups->GetReverse(pChildren);

        for (CXcpList<CPopup>::XCPListNode *pNode = pChildren->GetHead(); pNode != NULL; pNode = pNode->m_pNext)
        {
            if (pNode->m_pData->m_overlayElement)
            {
                ASSERT(!pNode->m_pData->m_overlayElement->IsRenderWalkRoot());
                IFC(Render(pNode->m_pData->m_overlayElement, rp, FALSE /* redirectedDraw */));
            }

            ASSERT(!pNode->m_pData->IsRenderWalkRoot());
            IFC(Render(pNode->m_pData, rp, TRUE /* redirectedDraw */));
        }
    }

Cleanup:
    if (pChildren)
    {
        pChildren->Clean(FALSE);
        SAFE_DELETE(pChildren);
    }

    RRETURN(hr);
}

// ------------------------------------------------------------------------
//
//  Synopsis:
//      Renders elements that are part of connected animations, but not
//      being being redendered as either part of the normal tree or
//      unloading storage (such as hidden popups)
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::RenderConnectedAnimationUnloadingElements(
    _In_ CConnectedAnimationRoot *pRoot,
    _In_ const HWRenderParams &rp
)
{
    if (pRoot->NeedsUnloadingHWWalk())
    {
        ASSERT(pRoot->GetContext()->GetConnectedAnimationServiceNoRef());
        // We only need to walk elements that haven't been prevously walked.  Mostly, these elements
        // will get walked with the unloading storage from their parent elements, however, there are cases
        // (such as popups) where whole branches of the tree may be ignored and if our animation element
        // is buried under there, then those unloading storage elements won't get walked.
        for (auto& element : pRoot->GetUnloadingElements())
        {
            if (element->GetCompositionPeer() == nullptr)
            {
                IFC_RETURN(Render(element, rp, FALSE /* redirectedDraw */));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Render the frame with the specified compositor
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWWalk::RenderRoot(
    _In_ CUIElement* pVisualRoot,
    _In_ CWindowRenderTarget *pRenderTarget,
    bool forceRedraw,
    XFLOAT zoomScale,
    bool hasUIAClientsListeningToStructure
    )
{
    RRETURN(RenderRootImpl(
        pVisualRoot,
        pRenderTarget,
        forceRedraw,
        zoomScale,
        NULL /* pPrependTransformAndClip */,
        true /* useDCompAnimations */,
        hasUIAClientsListeningToStructure
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Render the frame with the specified compositor
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWWalk::RenderRootImpl(
    _In_ CUIElement* pVisualRoot,
    _In_ CWindowRenderTarget *pRenderTarget,
    bool forceRedraw,
    XFLOAT zoomScale,
    _In_opt_ TransformAndClipStack *pPrependTransformAndClip,
    bool useDCompAnimations,
    bool hasUIAClientsListeningToStructure
    )
{
    HRESULT hr = S_OK;

    CTextCore *pTextCoreNoRef = NULL;

    HWRenderParams rp;

    rp.pRenderTarget = pRenderTarget;
    rp.pHWWalk = this;
    rp.forceRedraw = forceRedraw;

    // UI Automation needs to fire the StructureChanged event on qualifying elements, but we only
    // take action if there is actually a client listening to this event in the first place.
    m_UIAClientsListeningToStructure = hasUIAClientsListeningToStructure;

    // Initialize the realization transform.
    CTransformToRoot transformToRoot;

    if (pPrependTransformAndClip)
    {
        CMILMatrix rootTransform;
        pPrependTransformAndClip->Get2DTransformInLeafmostProjection(&rootTransform);
        transformToRoot.Prepend(rootTransform);
    }
    rp.pTransformToRoot = &transformToRoot;

    // Full screen elements will size themselves to the effective window size. They'll attach their DComp visuals below
    // the root visual, which has the zoom scale set on it. After the zoom scale is applied, they'll be sized to the
    // full window.
    rp.effectiveWindowSize.width = pRenderTarget->GetWidth() / zoomScale;
    rp.effectiveWindowSize.height = pRenderTarget->GetHeight() / zoomScale;

    // TODO: HWPC: Could clip to window bounds here to cull the tree walk?  Happens w/ prepend transform in SCBP case
    // Initialize the rendering transform.
    TransformAndClipStack transformsAndClips;

    // Using VERIFYHR instead of IFC() because the latter breaks the build by skipping initialization of visualContentRenderer below.
    VERIFYHR(transformsAndClips.PushTransformsAndClips(pPrependTransformAndClip));
    rp.pTransformsAndClipsToCompNode = &transformsAndClips;

    rp.m_useDCompAnimations = useDCompAnimations;

    // Instantiate an instance of the appropriate type of content renderer.
    // These objects are super lightweight so we can allocate both on the stack and choose which one to use.
    uint32_t maxTextureSize = m_maxTextureSizeProvider.GetMaxTextureSize();
    VisualContentRenderer visualContentRenderer(maxTextureSize);

    rp.pContentRenderer = static_cast<IContentRenderer *>(&visualContentRenderer);
    rp.pContentRenderer->SetRenderParams(&rp);

    // Set the pointer into the persistent render data list to the head of the list.
    ResetRenderingContext();

    //
    // Perform the render walk.
    //
    ASSERT(pVisualRoot->IsRenderWalkRoot());

    ASSERT(pVisualRoot->GetContext()->m_fInRenderWalk == FALSE);
    pVisualRoot->GetContext()->m_fInRenderWalk = TRUE;

    rp.m_isInXamlIsland = pVisualRoot->OfTypeByIndex<KnownTypeIndex::XamlIslandRoot>();

    IFC(Render(pVisualRoot, rp, FALSE /*isRedirectedDraw*/));

    ASSERT(pVisualRoot->GetContext()->m_fInRenderWalk == TRUE);
    pVisualRoot->GetContext()->m_fInRenderWalk = FALSE;

    // Process pending text realizations.
    // NOTE: If GetTextCore fails, no text realizations were created and there is no flush required.
    if (pTextCoreNoRef != NULL || SUCCEEDED(pVisualRoot->GetContext()->GetTextCore(&pTextCoreNoRef)))
    {
        IFC(pTextCoreNoRef->GetWinTextCore()->FlushTextRealizations());
    }

Cleanup:
    // Reset our render walk flag if we hit a device lost so that we don't assert after recovery
    if (GraphicsUtility::IsDeviceLostError(hr))
    {
        pVisualRoot->GetContext()->m_fInRenderWalk = FALSE;
    }

    if (SUCCEEDED(hr))
    {
        pVisualRoot->NWCleanDirtyFlags();
        visualContentRenderer.DirtyElementsForNextFrame();
    }

    // To ensure proper cleanup of D2DDeviceContexts, need to process pending text realizations if FAILED(hr).
    // NOTE: If pTextCoreNoRef != NULL, FlushTextRealizations() was already called.
    //       If GetTextCore() fails, no text realizations were created and there is no need to flush them.
    if (FAILED(hr) && pTextCoreNoRef == NULL)
    {
        if (SUCCEEDED(pVisualRoot->GetContext()->GetTextCore(&pTextCoreNoRef)))
        {
            VERIFYHR(pTextCoreNoRef->GetWinTextCore()->FlushTextRealizations());
        }
    }

    ResetRenderingContext();

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Callback when the device is lost.
//
//------------------------------------------------------------------------
void
HWWalk::HandleDeviceLost(bool cleanupDComp)
{
    m_pTextureManager->CleanupDeviceRelatedResources(cleanupDComp);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure this element has a composition node
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::EnsureCompositionPeer(
    _In_ CUIElement *pUIElement,
    _In_ const HWRenderParams &parentRP,
    _In_ const HWRenderParams &myRP,
    _In_opt_ HWCompTreeNode *pRedirectionTarget,
    _In_opt_ const CMILMatrix *pTransformsFromRedirectionTargetToRedirectionCompNode,
    _Outptr_ HWCompTreeNode **ppElementNode,
    _Outptr_ HWCompLeafNode **ppContentNode,
    _Outptr_result_maybenull_ WUComp::IContainerVisual** ppRenderDataParent,
    _Out_ bool *pHasRealizationTransformAboveCompNodeChanged
    )
{
    HRESULT hr = S_OK;

    HWCompTreeNode *pElementNode = NULL;
    HWCompLeafNode *pContentNode = NULL;
    xref_ptr<WUComp::IContainerVisual> spRenderDataParentUnderCompNode;

    bool hasRealizationTransformAboveCompNodeChanged = false;

    HWCompTreeNode *pParentCompNodeNoRef = myRP.pHWWalk->GetParentCompNode();

    const bool isOverrideWalk = (myRP.pHWWalk->GetOverrideRenderDataList() != NULL);

    HWCompTreeNode *pCompositionPeerNoRef = NULL;

    if (!isOverrideWalk)
    {
        // In the normal case, get the cached peer from the UIElement, if it exists already.
        pCompositionPeerNoRef = pUIElement->GetCompositionPeer();
    }
    else
    {
        // If this is an override walk, it means this branch of the tree is being rendered multiple times.
        // It is not safe to re-use the composition peer here. Instead, we will create a brand new comp node and attach
        // it in the new location in the tree. These additional comp nodes will be re-created every frame, just like
        // all the render data is in an override walk.
        ASSERT(pCompositionPeerNoRef == NULL);
    }

    // NOTE: The parentRP are passed in here because the command for the prepend properties are required, which
    //           exclude the local property values (which have been included in myRP).
    IFC(EnsureElementAndContentNodes(
        pUIElement,
        pCompositionPeerNoRef,
        myRP,
        parentRP,
        pRedirectionTarget,
        pTransformsFromRedirectionTargetToRedirectionCompNode,
        &pElementNode,
        &pContentNode,
        spRenderDataParentUnderCompNode.ReleaseAndGetAddressOf()
        ));

    if (!isOverrideWalk)
    {
        // In the normal case, save this node on the UIElement. This will insert it into the tree if needed.
        IFC(pUIElement->EnsureCompositionPeer(
            pElementNode,
            pParentCompNodeNoRef,
            myRP.pHWWalk->GetPreviousChildCompNode(),
            myRP.pHWWalk->GetLastSpriteVisual()
            ));

        // Determine whether the world transform to this comp node has changed since last frame. This checks
        // myRP since the local transform and/or redirection transform must be included in this check, for accuracy.
        // If so, the render walk will need to visit the elements in the subgraph, even if they are not otherwise
        // dirty themselves, because their realization scale has changed and they might require an update.
        // The flag is never set during animations to avoid unnecessary render walk costs, since realizations are
        // not updated during animations.
        // TFS Bug #13148817:  In the case of a brand new CompNode, we also need to update the CompNode's TransformToRoot, otherwise
        // we'll incorrectly use the identity transform as the TransformToRoot which will cause incorrect realizations.
        if (!myRP.isTransformAnimating || pCompositionPeerNoRef == nullptr)
        {
            // In this case we only need to check if TransformToRoot has changed its scale/sub-pixel offsets.  Only in this
            // case do we need to re-generate our realizations, otherwise we can leave them as is and get huge savings.
            hasRealizationTransformAboveCompNodeChanged = pUIElement->UpdateCompositionPeerTransformToRoot(myRP.pTransformToRoot);
        }
        else
        {
            // While the transform animation is occurring, keep the realization transform locked to our last known value.
            // This allows our comparison code above to accurately detect when new realizations are needed when the animation completes.
            // This also prevents elements that are dirty from "popping" their realization scale to a transient, animating scale.
            *(myRP.pTransformToRoot) = pElementNode->GetTransformToRoot();
        }
    }
    else
    {
        // In the override case, insert the new comp nodes in the tree temporarily (just for this frame) since
        // we don't do any sort of incremental updates in this situation.
        CompositorTreeHost *pCompTreeHostNoRef = myRP.pRenderTarget->GetCompositorTreeHost();

        ASSERT(pParentCompNodeNoRef != NULL);

        pElementNode->SetAllowReuseHandOffVisual(false);

        HWCompTreeNodeWinRT* parentCompNodeWinRT = static_cast<HWCompTreeNodeWinRT*>(pParentCompNodeNoRef);
        parentCompNodeWinRT->InsertChildSynchronous(
            pUIElement->GetDCompTreeHost(),
            pElementNode,
            myRP.pHWWalk->GetPreviousChildCompNode(),
            myRP.pHWWalk->GetLastSpriteVisual(),
            pUIElement,
            false /* ignoreInsertVisualErrors */);

        pCompTreeHostNoRef->TrackTemporaryNode(*pElementNode);

        // There's no need to detect transform changes since everything in the override case is forcibly redrawn.
        ASSERT(myRP.forceRedraw);
    }

    *ppElementNode = pElementNode;
    pElementNode = NULL;

    *ppContentNode = pContentNode;
    pContentNode = NULL;

    *ppRenderDataParent = spRenderDataParentUnderCompNode.detach();

    *pHasRealizationTransformAboveCompNodeChanged = hasRealizationTransformAboveCompNodeChanged;

Cleanup:
    ReleaseInterfaceNoNULL(pElementNode);
    ReleaseInterfaceNoNULL(pContentNode);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure this element has a composition node
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::EnsureElementAndContentNodes(
    _In_ CUIElement *pUIElement,
    _In_opt_ HWCompTreeNode *pExistingCompositionPeer,
    _In_ const HWRenderParams &myRP,
    _In_ const HWRenderParams &parentRP,
    _In_opt_ HWCompTreeNode *pRedirectionTarget,
    _In_opt_ const CMILMatrix *pTransformsFromRedirectionTargetToRedirectionCompNode,
    _Outptr_ HWCompTreeNode **ppElementNode,
    _Outptr_ HWCompLeafNode **ppContentNode,
    _Outptr_result_maybenull_ WUComp::IContainerVisual** ppRenderDataParent
    ) noexcept
{
    HRESULT hr = S_OK;

    // TODO: JCOMP: pUIElement should be 'const' here, but requires lots of churn

    HWCompTreeNode *pElementNode = NULL;
    HWCompLeafNode *pNewContentNode = NULL;
    wrl::ComPtr<WUComp::IContainerVisual> renderDataParentUnderCompNode;

    CompositorTreeHost *pCompTreeHostNoRef = parentRP.pHWWalk->GetCompositorTreeHost(parentRP.pRenderTarget);
    DCompTreeHost *pDCompTreeHostNoRef = parentRP.pRenderTarget->GetDCompTreeHost();

    bool isNewCompositorNode = false;

    // Element types that require redirected drawing will always require it when they are in the visual tree.
    // Therefore, we do not need to handle switching back and forth between having a tree node and a redirected one dynamically.
    ASSERT(pExistingCompositionPeer == NULL
        || (pRedirectionTarget != NULL && pExistingCompositionPeer->OfTypeByIndex<KnownTypeIndex::HWRedirectedCompTreeNodeWinRT>())
        || (pRedirectionTarget == NULL && pExistingCompositionPeer->GetTypeIndex() == KnownTypeIndex::HWCompTreeNode));

    // Ensure the composition peer exists.
    SetInterface(pElementNode, pExistingCompositionPeer);
    if (pElementNode == NULL)
    {
        isNewCompositorNode = TRUE;

        // This element has split the current partition of the tree into 3 layers (presubgraph, the subgraph of pElementNode,
        // and postsubgraph).
        // -  Content in the subgraph will be added to the content node, initially. It is possible that a descendent will split
        //     this partition into further layers, at which point it is the descendent's responsibility to create the
        //     appropriate composition nodes.
        // -  Presubgraph content may have been added to the content node of a descendent, if this element is the first to
        //     split its subgraph partition. If the parent's subgraph was split already, presubgraph content is added to the
        //     postsubgraph node of the previous partitioning element.
        // -  Postsubgraph content will be added to postsubgraph node, created here, initially. If another element in this
        //     partition splits the tree further, again it is that element's responsibility to create additional composition nodes.

        // Create a comp node peer for this UIElement. This will map the element's properties over to the composition tree.

        if (pRedirectionTarget != NULL)
        {
            ASSERT(GetRedirectionTarget(pUIElement) != NULL);
            ASSERT(pTransformsFromRedirectionTargetToRedirectionCompNode != NULL);

            if (pUIElement->GetTypeIndex() == KnownTypeIndex::Popup
                && static_cast<CPopup*>(pUIElement)->IsWindowed())
            {
                HWWindowedPopupCompTreeNodeWinRT *windowedPopupNode = new HWWindowedPopupCompTreeNodeWinRT(
                    pUIElement->GetContext(),
                    pCompTreeHostNoRef,
                    pDCompTreeHostNoRef);

                pElementNode = windowedPopupNode;
            }
            else
            {
                HWRedirectedCompTreeNodeWinRT *redirectedCompTreeNode = new HWRedirectedCompTreeNodeWinRT(
                    pUIElement->GetContext(),
                    pCompTreeHostNoRef,
                    pDCompTreeHostNoRef);

                pElementNode = redirectedCompTreeNode;
            }
        }
        else
        {
            IFC(HWCompTreeNodeWinRT::Create(
                pUIElement->GetContext(),
                pCompTreeHostNoRef,
                pDCompTreeHostNoRef,
                FALSE /* isTemporaryCompNode */,
                &pElementNode));
        }
    }

    // Update the composition peer with data that may have changed since last frame.
    {
        ASSERT(!parentRP.pTransformsAndClipsToCompNode->HasProjection());
        CMILMatrix prependTransform;
        parentRP.pTransformsAndClipsToCompNode->Get2DTransformInLeafmostProjection(&prependTransform);

        // The prepend clip is in the local space of the comp node that it's set on.
        HWClip prependHWClip;
        IFC(parentRP.pTransformsAndClipsToCompNode->TransformToLocalSpace(&prependHWClip));
        ASSERT(prependHWClip.IsRectangular());

        XRECTF prependClip;
        prependHWClip.GetRectangularClip(&prependClip);

        // Update the prepend properties if this is a new comp node, or if a prepend property change was detected.
        if (isNewCompositorNode || parentRP.hasPropertyToCompNodeChanged)
        {
            IFC(pElementNode->SetPrependProperties(
                &prependTransform,
                &prependClip,
                parentRP.opacityToCompNode));
        }

        // Update the element data if this is a new comp node, or the element itself is dirty.
        //
        // Also update the element data if something was dirty up to the previous comp node. This is to handle RTL scenarios.
        // We don't have a comp node flag for "this element is RTL". Rather, we have a flag for "this element's RTL is different
        // from its parent". So if the parent's RTL-ness changed, this element's flag will need to be updated even if it's not
        // otherwise dirty. This will be detected as a transform being dirty to the parent comp node.
        if (isNewCompositorNode
            || pUIElement->NWNeedsElementRendering()
            || parentRP.hasPropertyToCompNodeChanged
            )
        {
            IFC(pElementNode->SetElementData(
                parentRP.pRenderTarget,
                pUIElement,
                myRP.m_isHitTestVisibleSubtree));
        }

        // TODO: JCOMP: Should only need to generate this command if redirection element was invalidated.
        if (pRedirectionTarget != NULL)
        {
            const bool isAbsolutelyPositioned = IsAbsolutelyPositioned(pUIElement);

            // Note: This is called every UI thread frame. It marks the comp tree to rebuild the redirection transforms,
            // which might change even though the redirection target stays the same.
            HWRedirectedCompTreeNodeWinRT* redirectedNode = static_cast<HWRedirectedCompTreeNodeWinRT*>(pElementNode);
            redirectedNode->SetRedirectionTarget(
                isAbsolutelyPositioned ? NULL : pRedirectionTarget,
                pTransformsFromRedirectionTargetToRedirectionCompNode);
        }
    }

    // Split the render data if we made a new comp node.
    if (isNewCompositorNode && parentRP.pHWWalk->m_pCurrentContainerNoRef != NULL)
    {
        // When doing synchronous updates of the comp tree, there are no render data nodes, so there's nothing to split.
        HWCompTreeNodeWinRT* elementNodeWinRTNoRef = static_cast<HWCompTreeNodeWinRT*>(pElementNode);
        renderDataParentUnderCompNode = elementNodeWinRTNoRef->GetContainerVisualForChildren();
    }

    // It's possible for the content node to change dynamically. An animating media element that switches from drawing
    // its PosterSource to drawing a media swap chain will transition from a render data node to a media node, for example.
    // That's why each case below needs to check that the type of content node matches what is expected.

    // Ensure the content node for the composition peer exists and is up-to-date.
    // This is the layer that this element's content, and other content in its subgraph, will be drawn from.
    // All will be affected by the element peer's properties.
    // TODO: JCOMP: Could consider content-dirtiness for media, scbp, etc to skip generating redundant CompTreeHost commands.
    if (pUIElement->OfTypeByIndex<KnownTypeIndex::MediaPlayerPresenter>())
    {
        if (pElementNode->GetContentNode() == nullptr
            || !pElementNode->GetContentNode()->OfTypeByIndex<KnownTypeIndex::HWCompMediaNode>())
        {
            IFC(HWCompMediaNode::Create(
                pElementNode->GetContext(),
                pCompTreeHostNoRef,
                reinterpret_cast<HWCompMediaNode**>(&pNewContentNode)));

            IFC(pElementNode->SetContentNode(
                pNewContentNode,
                pDCompTreeHostNoRef,
                false /* isMultitargetLTEForSwapChainPanel */
                ));
        }

        ASSERT(pElementNode->GetContentNode() != nullptr
            && pElementNode->GetContentNode()->OfTypeByIndex<KnownTypeIndex::HWCompMediaNode>());

        CMediaPlayerPresenter* pMediaPlayerPresenterNoRef = static_cast<CMediaPlayerPresenter*>(pUIElement);

        HWCompMediaNode* mediaCompNode = static_cast<HWCompMediaNode*>(pElementNode->GetContentNode());
        IFC(mediaCompNode->SetMedia(
            pDCompTreeHostNoRef,
            pMediaPlayerPresenterNoRef->GetSwapChainHandle(),
            *pMediaPlayerPresenterNoRef->GetDestinationRect(),
            pMediaPlayerPresenterNoRef->GetStretchTransform()->GetXMATRIX(),
            *pMediaPlayerPresenterNoRef->GetStretchClip()));
    }
    else if (pUIElement->HasSwapChainContent())
    {
        if (pUIElement->OfTypeByIndex<KnownTypeIndex::SwapChainElement>())
        {
            CSwapChainElement *pSwapChainElement = static_cast<CSwapChainElement*>(pUIElement);

            CDependencyObject *pParent = pUIElement->GetParentInternal(false /*publicOnly*/);
            ASSERT(pParent->OfTypeByIndex<KnownTypeIndex::SwapChainPanel>() || pParent->OfTypeByIndex<KnownTypeIndex::SwapChainBackgroundPanel>());
            const bool isSwapChainBackgroundPanel = (pParent->GetTypeIndex() == KnownTypeIndex::SwapChainBackgroundPanel);
            bool stretchToFit = isSwapChainBackgroundPanel;

            if (pElementNode->GetContentNode() == NULL
                || !pElementNode->GetContentNode()->OfTypeByIndex<KnownTypeIndex::HWCompSwapChainNode>())
            {
                IFC(HWCompSwapChainNode::Create(
                    pElementNode->GetContext(),
                    isSwapChainBackgroundPanel ? nullptr : static_cast<CSwapChainPanel*>(pParent),
                    pCompTreeHostNoRef,
                    reinterpret_cast<HWCompSwapChainNode**>(&pNewContentNode)));

                // Note: This may fail, if we're in a portaling LTE scenario that's targeting a SwapChainPanel.
                // Portaling scenarios have multiple LayoutTransitionElements rendering the same target, and in the case
                // of SwapChainPanel the SCP has a single Visual hosting the swap chain that can't physically be
                // parented to multiple places in the tree. Xaml is going to get E_INVALIDARG when trying to parent it
                // the second time. Detect this scenario and pass down a flag that lets Xaml tolerate the error.
                IFC(pElementNode->SetContentNode(
                    pNewContentNode,
                    pDCompTreeHostNoRef,
                    !!myRP.pHWWalk->GetOverrideRenderDataList() /* isMultitargetLTEForSwapChainPanel */
                    ));
            }

            ASSERT(pElementNode->GetContentNode()
                   && pElementNode->GetContentNode()->OfTypeByIndex<KnownTypeIndex::HWCompSwapChainNode>());

            HWCompSwapChainNode* swapChainCompNode = static_cast<HWCompSwapChainNode*>(pElementNode->GetContentNode());
            IFC(swapChainCompNode->SetSwapChain(
                pDCompTreeHostNoRef,
                pSwapChainElement,
                parentRP.effectiveWindowSize.width,
                parentRP.effectiveWindowSize.height,
                stretchToFit));
        }
        else
        {
            IFCFAILFAST(E_UNEXPECTED); // No element type should get here
        }
    }
    else
    {
        if (   pElementNode->GetContentNode() == NULL
            || !pElementNode->GetContentNode()->OfTypeByIndex<KnownTypeIndex::HWCompRenderDataNode>())
        {
            // This element is rendering with a comp node for the first time, or it changed its render data
            // to a ContainerVisual from some other format. If it's a new node, we should have split the
            // current render data parent to get the SpriteVisual under the new node. If it changed render data
            // types, then we need to make a blank ContainerVisual for it.

            // When doing synchronous comp tree updates, there are no render data comp nodes. We're in sprite visuals mode,
            // so the HWCompTreeNode is already capable of holding SpriteVisuals directly.

            if (renderDataParentUnderCompNode == nullptr)
            {
                HWCompTreeNodeWinRT* elementNodeWinRTNoRef = static_cast<HWCompTreeNodeWinRT*>(pElementNode);
                renderDataParentUnderCompNode = elementNodeWinRTNoRef->GetContainerVisualForChildren();
            }
        }
    }

    // Ensure the composition peer is set on the UIElement and is in the correct location in the tree.
    ASSERT(pElementNode != NULL);

    SetInterface(*ppElementNode, pElementNode);

    SetInterface(*ppRenderDataParent, renderDataParentUnderCompNode.Get());

Cleanup:
    ReleaseInterfaceNoNULL(pElementNode);
    ReleaseInterfaceNoNULL(pNewContentNode);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculates the source and destination rects, the stretch transform,
//      and the stretch clip.
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
HWWalk::CalculateMediaEngineAndDCompParameters(
    _Inout_ CFrameworkElement *pElement,
    bool inProperFullWindowMode,
    XSIZE *pWindowSize,
    _Out_ XRECTF_RB *pNormalizedSourceRect,
    _Out_ XRECT *pDestinationRect,
    _Out_ CMILMatrix *pStretchTransform,
    _Out_ XRECTF *pStretchClip
    )
{
    // The subrect of the frame that will be visible in the media element, normalized to [0, 1]. Used by
    // stretch modes Uniform and UniformToFill.
    XRECTF_RB normalizedSourceRect;
    normalizedSourceRect.left = normalizedSourceRect.top = 0.0f;
    normalizedSourceRect.right = normalizedSourceRect.bottom = 1.0f;

    // The size of the target swap chain. Set to either the natural size of the content (for stretch modes
    // None and Fill) or to the size of the media element itself (for stretch modes Uniform and UniformToFill).
    XRECT destRect = { 0, 0, 0, 0 };

    // The clip to apply to the DComp visual containing the media swap chain. Used for stretch mode None and
    // used rarely for stretch modes Uniform and UniformToFill. Default to an empty clip. If the source is
    // not ready, then we render nothing.
    XRECTF stretchClip = { 0.0f, 0.0f, 0.0f, 0.0f };

    DirectUI::Stretch stretch;
    CMILMatrix stretchTransform(TRUE);
    XRECT mediaNaturalSize = { };

    stretch = static_cast<CMediaPlayerPresenter*>(pElement)->m_Stretch;
    IFC_RETURN(static_cast<CMediaPlayerPresenter*>(pElement)->GetNaturalBounds(mediaNaturalSize));

    if (mediaNaturalSize.Width > 0 && mediaNaturalSize.Height > 0)
    {
        XRECTF mediaNaturalSizeF = {
            static_cast<XFLOAT>(mediaNaturalSize.X),
            static_cast<XFLOAT>(mediaNaturalSize.Y),
            static_cast<XFLOAT>(mediaNaturalSize.Width),
            static_cast<XFLOAT>(mediaNaturalSize.Height)
        };

        XRECTF elementSize;
        elementSize.X = elementSize.Y = 0.0f;
        DirectUI::AlignmentX alignmentX;
        DirectUI::AlignmentY alignmentY;

        IFC_RETURN(static_cast<CMediaPlayerPresenter*>(pElement)->UpdateInternalSize(elementSize.Width, elementSize.Height, alignmentX, alignmentY));

        if (inProperFullWindowMode)
        {
            //
            // In proper full window mode, the media swap chain is attached under the root visual, and will have the
            // plateau scale applied to it. Size it to the full window and append the inverse plateau scale transform
            // so that the final scale is identity. The swap chain then qualifies for DFlip because its dimensions
            // match the screen dimensions.
            //

            // There's no need to take UIElement.RasterizationScale into account here. We're interested in the true size
            // of the window, and the only contributing factors are the size available for layout and the zoom scale.
            // RasterizationScale can affect the size of media playing in the tree by requiring it to be backed by a
            // bigger swap chain, but it can't have an effect on the size of the window.
            elementSize.Width = static_cast<XFLOAT>(pWindowSize->Width);
            elementSize.Height = static_cast<XFLOAT>(pWindowSize->Height);
        }

        //
        // Calculate the transform that can be applied to the swap chain to get the stretch mode on the media
        // element.
        //
        // Stretch is applied in one of two ways:
        //   1. By DComp - the swap chain is set to the natural size of the media. This transform then applies
        //      to the swap chain. A clip is applied to the DComp visual if needed. This is used for stretch
        //      modes None and Fill.
        //
        //   2. By MF - UpdateVideoStream updates the target swap chain with a source subrect of the media. The
        //      target rect is sized to the media element, and the source rect is calculated from the media
        //      element's size and the stretch transform. The DComp visual does not have a transform set on it,
        //      and only has a clip when handling edge cases. This is used for stretch modes Uniform and
        //      UniformToFill.
        //
        IFC_RETURN(CTileBrush::ComputeStretchMatrix(
            &mediaNaturalSizeF,
            &elementSize,
            alignmentX,
            alignmentY,
            stretch,
            &stretchTransform
            ));

        //
        // Stretch is applied
        //
        // IMFMediaEngineEx::UpdateVideoStream will decode a subrect of the frame directly into a target
        // rect while preserving the aspect ratio, filling it in one dimension and letterboxing in the other.
        // We can use it to implement some stretching and clipping behavior by decoding the portion of the
        // video that's visible into a swap chain that's the size of the element. There are limitations though:
        //
        //   1. The target rect must have integer sizes. If the element dimensions contain decimals, we have
        //      to do additional clipping ourselves.
        //
        //   2. Aspect ratio is preserved in the stretch. Stretch="Fill" ignores the aspect ratio when stretching,
        //      which UpdateVideoStream does not support.
        //
        //   3. The video will always be stretched to fill the swap chain completely in one dimension.
        //      Stretch="None" can require letterboxing in both dimensions, which UpdateVideoStream does not
        //      support.
        //
        switch (stretch)
        {
            case DirectUI::Stretch::Uniform:
            case DirectUI::Stretch::UniformToFill:
                GetUpdateVideoStreamRelatedParameters(
                    pElement,
                    &elementSize,
                    &mediaNaturalSizeF,
                    inProperFullWindowMode,
                    &stretchTransform,
                    &normalizedSourceRect,
                    &destRect,
                    &stretchClip
                    );
                break;

            case DirectUI::Stretch::Fill:
                // Stretch="Fill" applies a nonuniform stretch, so we can't use UpdateVideoStream (limitation #2 above).
                // Use DComp to stretch the media to fit the element. We've already calculated stretchTransform to
                // scale the natural size of the media to the element. Just use the natural size of the media as the
                // destination size.
                destRect = mediaNaturalSize;

                // No need to clip. The media fills up the element exactly.
                SetInfiniteClip(&stretchClip);
                break;

            case DirectUI::Stretch::None:
                //
                // Stretch="None" will potentially need to letterbox the media in both dimensions, which UpdateVideoStream
                // can't do (limitation #3 above), so we use DComp to stretch and clip the media. Note that in this case,
                // the letterboxes will be transparent rather than black. It's possible to draw letterboxes using XAML and
                // DComp, but this scenario isn't common enough to justify adding more rendering code to XAML media.
                //

                // stretchTransform was calculated to scale the natural size of the media to the element. Just use the
                // natural size of the media as the destination size.
                destRect = mediaNaturalSize;

                // The transform will be applied to the clip, so set the clip in media space. Once stretchTransform is applied,
                // the clip will become elementBounds in the media element's space.
                if (mediaNaturalSize.Width > elementSize.Width || mediaNaturalSize.Height > elementSize.Height)
                {
                    //
                    // stretchTransform maps from swap chain space to element space. We want to specify the
                    // clip in swap chain space (since that avoids creating another DComp visual due to property
                    // ordering). Transform the element bounds into swap chain space and use that as the clip.
                    //
                    CMILMatrix inverseStretch = stretchTransform;
                    if (inverseStretch.Invert())
                    {
                        inverseStretch.TransformBounds(&elementSize, &stretchClip);
                    }
                }
                else
                {
                    // No need to clip. The content is smaller than the element.
                    SetInfiniteClip(&stretchClip);
                }
                break;
        }
    }

    *pNormalizedSourceRect = normalizedSourceRect;
    *pDestinationRect = destRect;
    *pStretchTransform = stretchTransform;
    *pStretchClip = stretchClip;

    return S_OK;
}

float GetScaleFactorFromNaturalSize(_In_ const XRECTF* const pNaturalSize, _In_ const XRECT* const pDestRect)
{
    float scaleFactor = 1.0f;

    const float destWidth = static_cast<float>(pDestRect->Width);
    const float destHeight = static_cast<float>(pDestRect->Height);

    const float naturalWidth = pNaturalSize->Width;
    const float naturalHeight = pNaturalSize->Height;

    if (naturalWidth > 0.0f && naturalHeight > 0.0f)
    {
        const float offsetWidth = destWidth - naturalWidth;
        const float offsetHeight = destHeight - naturalHeight;
        if (offsetWidth > 0.0f || offsetHeight > 0.0f)
        {
            if (offsetWidth < offsetHeight)
            {
                scaleFactor = destWidth / naturalWidth;
            }
            else
            {
                scaleFactor = destHeight / naturalHeight;
            }
        }
    }

    return scaleFactor;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculates the normalized source rect, the destination rect, the
//      transform, and the clip rect for IMFMediaEngineEx::UpdateVideoStream.
//      Used for stretch modes Uniform and UniformToFill.
//
//------------------------------------------------------------------------
/* static */ void
HWWalk::GetUpdateVideoStreamRelatedParameters(
    _In_ CFrameworkElement *pElement,
    _In_ const XRECTF *pElementSize,
    _In_ const XRECTF *pMediaNaturalSizeF,
    bool inProperFullWindowMode,
    _Inout_ CMILMatrix *pStretchTransform,
    _Out_ XRECTF_RB *pNormalizedSourceRect,
    _Out_ XRECT *pDestRect,
    _Out_ XRECTF *pStretchClip
    )
{
    //
    // Uniform and UniformToFill use UpdateVideoStream to stretch and clip. Map the portion of the media that
    // is showing to the size of the media element.
    //

    XRECTF roundedElementSize = {0};
    bool isDestinationRectRounded = false;
    XRECTF elementBoundsInMediaSpace;
    XRECTF roundedElementBoundsInMediaSpace;
    bool isSourceRectRounded = false;

    DirectUI::Stretch stretch = { };
    stretch = static_cast<CMediaPlayerPresenter*>(pElement)->m_Stretch;

    ASSERT(stretch == DirectUI::Stretch::Uniform || stretch == DirectUI::Stretch::UniformToFill);

    const float plateauScale = RootScale::GetRasterizationScaleForElement(pElement);
    const float elementRasterizationScale = static_cast<float>(pElement->GetRasterizationScaleIncludingAncestors());
    // For non-full-window media, RasterizationScale has an effect. We'll need to decode into a larger swap
    // chain so that the media still looks sharp if scaled up by RasterizationScale.
    const float plateauScaleWithRasterizationScale = plateauScale * elementRasterizationScale;

    // Adjusts media swap chain size based on the plateau scale so the content isn't decoded at a smaller
    // size and then scaled up by the plateau scale.  This is done by pre-adjusting the destination rect
    // and stretch transform based on the plateau scale.
    //
    // Stretch modes None and Fill already use the natural video size so those don't need to be adjusted.
    // This check is done before calling this method which only handles Uniform and UniformToFill.
    //
    // ProperFullWindowMode uses the full window content and it already inversely applies the plateau
    // scale so those do not need to be adjusted.
    const bool shouldAdjustVideoSizeWithPlateauScale = !inProperFullWindowMode;

    // stretchTransform maps from swap chain space to element space. We want to specify the
    // clip in swap chain space (since that avoids creating another DComp visual due to property
    // ordering). Transform the element bounds into swap chain space and use that as the clip.
    CMILMatrix inverseStretch = *pStretchTransform;
    if (inverseStretch.Invert())
    {
        inverseStretch.TransformBounds(pElementSize, &elementBoundsInMediaSpace);

        //
        // The source rect elementBoundsInMediaSpace will be stretched into the destination rect
        // elementSize by UpdateVideoStream. However, there are 3 additional restrictions:
        //
        // 4. The source rect must be aligned on even pixels. Otherwise, the edge may show
        //    letterboxing.
        //
        //    We want to avoid unintentional letterboxing here, so we expand the source rect to align to
        //    even pixels. We then update the destination rect so that the overall scale stays the same.
        //
        // 5. The destination rect must use integers.
        //
        //    We round up the destination rect (which could be the original rect, or the updated rect
        //    after expanding the source) to the nearest integer to get around this. We then apply a clip
        //    at the size of the unrounded destination rect.
        //
        //    Note: This rounding slightly increases the scale of the stretch. The difference is less than
        //          1 pixel on each side. The destination size is expected to be much larger than the error,
        //          so we accept it. The alternative is to keep the destination rect as a decimal and to
        //          stretch in DComp rather than use UpdateVideoStream. We deliberately choose performance
        //          over pixel-perfect correctness here.
        //
        // 6. The destination rect will be clamped to (0, 0) if it starts at a negative value.
        //
        //    This is normally not a problem, but we could run into this if we expand the source rect
        //    towards the left, then update and round the destination rect. The original destination rect
        //    would have started at 0, but it shifted to the left after the source rect expanded, then
        //    shifted to the left again when it was rounded.
        //
        //    We don't want the destination rect to be clamped, otherwise the source could be stretched
        //    into the clamped destination rect and be letterboxed along the left or top edges. To get
        //    around this restriction, we shift the rect to start at (0, 0), then apply a small translation
        //    in DComp to move the top-left back to the correct position. We then clip to the unrounded
        //    destination size.
        //
        // The workarounds for these 3 restrictions can introduce clips and transforms in DComp. In the
        // case of full-screen media, these clips and transforms could disqualify the media visual for
        // DFlip, which degrades performance. Here, we also deliberately choose performance over
        // correctness, and disable expanding the source rect and clipping the rounded destination.
        // This can introduce two problems:
        //
        // 1. Due to restriction #4, we can get unintentional letterboxing if we don't expand the source
        //    rect. This is acceptable because the letterboxing is small compared to the entire screen,
        //    and is less noticeable because there won't be any content outside the black letterbox that
        //    contrasts the color.
        //
        // 2. Not clipping the rounded destination rect could cause rendering outside the intended area.
        //    Since we're rendering in full-screen, the intended area is the entire screen, so there's
        //    no problem here.
        //

        //
        // e.g. The natural size of the source is 10 x 8, and the media element is 10 x 10 with
        //      Stretch="UniformToFill". Normally, we use the source rect LTRB = {1, 0, 9, 8}
        //      and the destination rect LTRB = {0, 0, 10, 10}.
        //
        //      1. The source isn't aligned to even pixels, so we expand it to LTRB = {0, 0, 10, 8}.
        //      2. The destination is then updated to LTRB = {-1.25, 0, 11.25, 10}.
        //      3. The destination is then rounded to LTRB = {-2, 0, 12, 10}.
        //      4. The destination rect for UpdateVideoStream is then shifted to LTRB = {0, 0, 14, 10}.
        //      5. The media visual is clipped by LTRB = {2, 0, 12, 10} (the original 10x10 with the same shift).
        //      6. The media visual is then translated by (-2, 0) to get LTRB = {0, 0, 10, 10}.
        //

        if (!inProperFullWindowMode && stretch == DirectUI::Stretch::UniformToFill)
        {
            // Expand the source rect to align to even pixel boundaries.
            RoundSourceRect(
                &elementBoundsInMediaSpace,
                &roundedElementBoundsInMediaSpace,
                &isSourceRectRounded
                );
        }
        else
        {
            roundedElementBoundsInMediaSpace = elementBoundsInMediaSpace;
        }

        // If the source rect expanded, we need to update the destination rect for the new source size.
        if (isSourceRectRounded)
        {
            pStretchTransform->TransformBounds(&roundedElementBoundsInMediaSpace, &roundedElementSize);
        }
        else
        {
            roundedElementSize = *pElementSize;
        }

        if (shouldAdjustVideoSizeWithPlateauScale)
        {
            roundedElementSize.Width *= plateauScaleWithRasterizationScale;
            roundedElementSize.Height *= plateauScaleWithRasterizationScale;
        }

        // The destination takes integer sizes, so expand the destination rect if it's fractional.
        XFLOAT right = roundedElementSize.X + roundedElementSize.Width;
        XFLOAT bottom = roundedElementSize.Y + roundedElementSize.Height;
        roundedElementSize.X = static_cast<XFLOAT>(XcpFloor(roundedElementSize.X));
        roundedElementSize.Y = static_cast<XFLOAT>(XcpFloor(roundedElementSize.Y));
        roundedElementSize.Width = static_cast<XFLOAT>(XcpCeiling(right) - roundedElementSize.X);
        roundedElementSize.Height = static_cast<XFLOAT>(XcpCeiling(bottom) - roundedElementSize.Y);

        isDestinationRectRounded =
            !inProperFullWindowMode
            && (roundedElementSize.X != pElementSize->X
                || roundedElementSize.Y != pElementSize->Y
                || roundedElementSize.Width != pElementSize->Width
                || roundedElementSize.Height != pElementSize->Height);
    }

    // We should have rounded the destination rect already.
    ASSERT(FractionReal(roundedElementSize.X) == 0.0f
        && FractionReal(roundedElementSize.Y) == 0.0f
        && FractionReal(roundedElementSize.Width) == 0.0f
        && FractionReal(roundedElementSize.Height) == 0.0f
        );

    // The destination is the size of the media element itself. If the destination's aspect ratio is different
    // from the source's aspect ratio, then the source will be centered and letterboxed.
    // Shift the destination rect to start at (0, 0). We'll shift it back with a DComp transform later.
    if (inProperFullWindowMode)
    {
        // If we're rendering in full screen, then we never expanded the source rect, which means the destination
        // rect remains at (0,0).
        ASSERT(roundedElementSize.X == 0.0f && roundedElementSize.Y == 0.0f);
    }
    pDestRect->X = 0;
    pDestRect->Y = 0;
    pDestRect->Width = static_cast<XINT32>(roundedElementSize.Width);
    pDestRect->Height = static_cast<XINT32>(roundedElementSize.Height);

    // With Stretch="Uniform", the entire content will be showing, so the source rect can be left at [0, 1].
    // With Stretch="UniformToFill", only the middle portion of the media will be showing, which corresponds
    // to the element's bounds in the content's coordinate space.
    if (stretch == DirectUI::Stretch::UniformToFill)
    {
        pNormalizedSourceRect->left = roundedElementBoundsInMediaSpace.X / pMediaNaturalSizeF->Width;
        pNormalizedSourceRect->top = roundedElementBoundsInMediaSpace.Y / pMediaNaturalSizeF->Height;
        pNormalizedSourceRect->right = (roundedElementBoundsInMediaSpace.X + roundedElementBoundsInMediaSpace.Width) / pMediaNaturalSizeF->Width;
        pNormalizedSourceRect->bottom = (roundedElementBoundsInMediaSpace.Y + roundedElementBoundsInMediaSpace.Height) / pMediaNaturalSizeF->Height;
    }

    // Reset the transform on the DComp visual. UpdateVideoStream will take care of all stretching.
    pStretchTransform->SetToIdentity();

    if (inProperFullWindowMode)
    {
        //
        // In proper full window mode, the media swap chain is attached under the root visual, and will have the
        // plateau scale applied to it. Apply the inverse plateau scale so that the net scale is identity. This
        // will allow full window media swap chains to get DFlip even under plateau scaling.
        //
        // Note that RasterizationScale does not have an effect on the size of the window, and will not affect
        // the size of the full-window swap chain. It's also not a scale that we apply anywhere in the tree, unlike
        // the plateau scale. So, there's no need to undo it.
        pStretchTransform->Scale(1.0f / plateauScale, 1.0f / plateauScale);
    }
    else
    {
        // If we shifted the rounded destination to (0, 0), we need to shift it back.
        pStretchTransform->SetDx(roundedElementSize.X);
        pStretchTransform->SetDy(roundedElementSize.Y);

        if (shouldAdjustVideoSizeWithPlateauScale)
        {
            pStretchTransform->Scale(1.0f / plateauScaleWithRasterizationScale, 1.0f / plateauScaleWithRasterizationScale);
        }
    }

    // The destination rect is sized to the media element, so there's usually no need to clip the DComp visual.
    // However, the destination rect must have integer sizes (limitation #1 above), so if the media element has
    // fractional sizes then we still have to apply a clip. The exception to this case is if we're rendering in
    // full screen. Clipping to the entire screen has no effect and can be skipped.
    if (isDestinationRectRounded)
    {
        ASSERT(!inProperFullWindowMode);

        *pStretchClip = *pElementSize;
        pStretchClip->X -= roundedElementSize.X;
        pStretchClip->Y -= roundedElementSize.Y;

        // Adjust the stretch clip for the plateau scale as well.
        // This is different from roundedElement size in that it keeps the fractional component.
        if (shouldAdjustVideoSizeWithPlateauScale)
        {
            pStretchClip->Width *= plateauScaleWithRasterizationScale;
            pStretchClip->Height *= plateauScaleWithRasterizationScale;
        }
    }
    else
    {
        SetInfiniteClip(pStretchClip);
    }

}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Expands the source rect to something aligned with even integers.
//
//------------------------------------------------------------------------
/* static */ void
HWWalk::RoundSourceRect(
    _In_ const XRECTF *pSourceRect,
    _Out_ XRECTF *pRoundedSourceRect,
    _Out_ bool *pRectChanged
    )
{
    XRECT_RB roundedSourceRect;
    bool rectChanged = false;

    ASSERT(pSourceRect->X >= 0.0f && pSourceRect->Y >= 0.0f);

    if (FractionReal(pSourceRect->X) != 0
        || FractionReal(pSourceRect->Y) != 0
        || FractionReal(pSourceRect->Width) != 0
        || FractionReal(pSourceRect->Height) != 0
        )
    {
        rectChanged = TRUE;
    }

    roundedSourceRect.left = XcpFloor(pSourceRect->X);
    roundedSourceRect.top = XcpFloor(pSourceRect->Y);
    roundedSourceRect.right = XcpCeiling(pSourceRect->X + pSourceRect->Width);
    roundedSourceRect.bottom = XcpCeiling(pSourceRect->Y + pSourceRect->Height);

    if (roundedSourceRect.left % 2 != 0)
    {
        roundedSourceRect.left -= 1;
        rectChanged = TRUE;
    }

    if (roundedSourceRect.top % 2 != 0)
    {
        roundedSourceRect.top -= 1;
        rectChanged = TRUE;
    }

    if (roundedSourceRect.right % 2 != 0)
    {
        roundedSourceRect.right += 1;
        rectChanged = TRUE;
    }

    if (roundedSourceRect.bottom % 2 != 0)
    {
        roundedSourceRect.bottom += 1;
        rectChanged = TRUE;
    }

    pRoundedSourceRect->X = static_cast<XFLOAT>(roundedSourceRect.left);
    pRoundedSourceRect->Y = static_cast<XFLOAT>(roundedSourceRect.top);
    pRoundedSourceRect->Width = static_cast<XFLOAT>(roundedSourceRect.right - roundedSourceRect.left);
    pRoundedSourceRect->Height = static_cast<XFLOAT>(roundedSourceRect.bottom - roundedSourceRect.top);
    *pRectChanged = rectChanged;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handles the decode to render size logic by adjusting the bounds
//      rectangle to determine the size of the decoded image surface
//      and then requesting a decode to that size.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
HWWalk::RequestImageDecode(
    _In_ const XRECTF *pBounds,
    _In_ const HWRenderParamsBase& myRP,
    _In_ CImageBrush *pImageBrush,
    _In_opt_ CUIElement *pUIElement,
    _In_opt_ const XTHICKNESS *pNinegrid)
{
    // Before the image has finished the initial decode to determine its natural size,
    // the element will report a 0x0 size during Measure/Arrange, which will cause
    // us to draw with a 0x0 render bounds.  We should not make any decode requests
    // if either dimension is 0 as there is nothing visible on screen yet.
    if (pBounds->Width != 0.0f && pBounds->Height != 0.0)
    {
        XRECTF decodeBounds = { 0, 0, 0, 0 };
        bool requestDecode = true;

        // When NineGrid is used, DecodeToRenderSize is unnecessary as the image is already optimized
        // for stretching.  Explicitly use the natural bounds in this case.
        if (pNinegrid != NULL)
        {
            pImageBrush->m_pImageSource->TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::NineGrid);
            pImageBrush->GetNaturalBounds(&decodeBounds);
        }
        else
        {
            requestDecode = pImageBrush->ShouldRequestDecode();
            if (requestDecode)
            {
                XRECTF tempBounds = *pBounds;

                // Adjust the bounds based on the stretch of the brush in order to get the
                // proper decode resolution for the stretch
                IFC_RETURN(pImageBrush->AdjustDecodeRectForStretch(&tempBounds));

                // Adjust the bounds based on the world transform from the UI element
                CMILMatrix transform2D = myRP.GetRasterizationScaleTransform(pUIElement);

                transform2D.TransformBounds(&tempBounds, &decodeBounds);

                // Windows Blue Bug #598504
                // DecodeToRenderSize does not work properly when used in combination with
                // tiled images.  If the natural size of the image is different than the decoded
                // size, the brush transform is not computed correctly and causes
                // images to scale to the wrong size.  At this point in time (in Escrow for
                // Spring 14 release) it is too risky to fix the brush transform code,
                // so for now the fix is to detect tiling would be used and fall back to
                // decoding to the natural size.
                XSIZE size = {XcpCeiling(decodeBounds.Width), XcpCeiling(decodeBounds.Height)};
                if (CTiledSurface::NeedsToBeTiled(size, pImageBrush->GetContext()->GetMaxTextureSize()))
                {
                    pImageBrush->m_pImageSource->TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::UsesImageTiling);
                    pImageBrush->GetNaturalBounds(&decodeBounds);
                }
            }
        }

        // It is not safe to request decodes from the render walk.  Enqueue instead.
        //
        // For deferred decoding, request decode.  This won't do anything
        // if the decode was previously completed at an appropriate size
        if (requestDecode)
        {
            IFC_RETURN(pImageBrush->QueueRequestDecode(
                XcpRound(decodeBounds.Width),
                XcpRound(decodeBounds.Height),
                true /* retainPlaybackState */));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function that determines if a particular shape needs to be
//      rendered using a set of shape masks or if we can accelerate it by
//      other means...
//
//------------------------------------------------------------------------------
/* static */ bool
HWWalk::IsShapeMaskRequiredForRectangle(
    _In_ CRectangle *pRectangleElement
    )
{
    // TODO: HWPC: Handle stroked rectangles with ninegrids
    //
    // We have a special path for rectangles w/o stroke and/or rounded corners:
    //
    return !IsCloseReal(pRectangleElement->m_eRadiusX, 0.0f)
        || !IsCloseReal(pRectangleElement->m_eRadiusY, 0.0f)
        || pRectangleElement->GetStroke();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function that determines if a particular border needs to be
//      rendered using a set of shape masks or if we can accelerate it by
//      other means.
//
//------------------------------------------------------------------------------
/* static */ bool HWWalk::IsShapeMaskRequiredForBorder(_In_ CFrameworkElement *pFrameworkElement)
{
    ASSERT(pFrameworkElement->OfTypeByIndex<KnownTypeIndex::Panel>()
        || pFrameworkElement->OfTypeByIndex<KnownTypeIndex::Border>()
        || pFrameworkElement->OfTypeByIndex<KnownTypeIndex::ContentPresenter>());

    XCORNERRADIUS cornerRadius = pFrameworkElement->GetCornerRadius();
    CBrush * pBackgroundBrush = pFrameworkElement->GetBackgroundBrush();
    CBrush * pBorderBrush = pFrameworkElement->GetBorderBrush();

    // Borders can often be drawn as simple SpriteVisuals, like Rectangles.
    // We will fallback to using a rasterized mask to render the shape if any of the following are true:
    //      1. The Border has rounded corners and a BorderBrush or BackgroundBrush.
    //      2. The Border has a BorderBrush that cannot be drawn as a hollow ninegrid.
    bool hasRoundedCorners = (cornerRadius.topLeft > 0.0f)
                           || (cornerRadius.topRight > 0.0f)
                           || (cornerRadius.bottomRight > 0.0f)
                           || (cornerRadius.bottomLeft > 0.0f);

    XTHICKNESS unusedNinegrid;
    return (pBackgroundBrush != nullptr && hasRoundedCorners)
        // Force integer insets when we calculate the nine grid here. This is the method that determines
        // whether we need a mask, and if we do then we'll be calculating the nine grid while forcing
        // integer insets, so keep the calculations consistent.
        // TODO: I'm pretty sure the GetNinegridForBorderElement call here can be deleted entirely. If
        // we get this far in the statement, then there are no rounded corners, and square borders don't
        // need a mask to render. There's an edge case around degenerate borders whose border thickness
        // exceeds the border size that needs to be checked first.
        || (pBorderBrush != nullptr && (hasRoundedCorners || !GetNinegridForBorderElement(pFrameworkElement, true /* forceIntegerInsets */, &unusedNinegrid)));
}

//
// Calculates a set of nine grid insets used to draw the border and background of a Border-like element (i.e. Border,
// Grid, or ContentPresenter). Returns true iff such a nine grid exists.
//
// A border might not be able to fit in a nine grid. For example, this border has a large radius at the top-left and
// bottom-right, and a small radius at the other two corners:
//
//      <Border CornerRadius="50,10,50,10" Width="60" Height="60" Background="Green" />
//
//                                -+ossooooooooss+
//                          `:osso/.             \d+
//                      `-oy+/                     y+
//                   `.oy+`                         M
//                 `+h+`                            M
//               `oh/                              .m
//              /h/                                os
//            .y+                                  N.
//           /d.                                  /d
//          oy`                                  `m.
//         oy                                    d/
//        +h                                    yo
//       .m`                                  `yo
//       d:                                  -d:
//      -N                                  +y.
//      so                                /h/
//      N.                              /ho`
//      M                            `+y+`
//      M                         `+yo.`
//      +y                     /+yo-`
//       +d:             ./osso:`
//         +ssoooooooosso+-
//
// There's no way to divide that into a nine grid that does any nine grid stretching or has a hollow center. There
// are also other degenerate cases where the border is all rounded corners. In these cases we wouldn't save anything
// by using a nine grid.
//
// This method is used in two scenarios:
//
//   1. When we draw a rounded corner border, it's used to calculate the nine grid insets for mapping from the
//      alpha mask to the visual. We want these insets to be whole numbers (after multiplying by the display scale)
//      in order to map 1:1 from the alpha mask, so we do a layout rounded ceiling operation on the final insets.
//      This ceiling operation may bump the insets up by a pixel due to floating point error, but that doesn't show
//      any visible symptoms. We just end up allocating the mask a pixel or two wider and sample an extra pixel or
//      two from the edges. The actual border stroke drawn into that mask uses only the border thicknesses and not
//      the nine grid insets we calculate here. In these cases, forceIntegerInsets will be true.
//
//   2. When we draw a square corner border, it's used to calculate the nine grid insets that represent the border
//      thicknesses directly. These border thicknesses have already gone through layout rounding once. We don't want
//      to apply a layout rounded ceiling operation on the final insets because floating point error may push the
//      thicknesses up by a pixel. In these cases, forceIntegerInsets will be false.
//
// Note: Border/Grid/ContentPresenter don't have any overlap between the background and the border. The background
// starts at the inside edge of the border. Rectangle/Shape do have overlap. The fill starts in the middle of the
// stroke.
//
/*static*/ bool HWWalk::GetNinegridForBorderElement(
    _In_ CUIElement* pUIElement,
    const bool forceIntegerInsets,
    _Out_ XTHICKNESS* ninegrid)
{
    // This optimization only applies to border-like elements, but BaseContentRenderer::MaskCombinedRenderHelper
    // is used by other elements like CalendarView to draw chrome.
    if (pUIElement->OfTypeByIndex<KnownTypeIndex::Panel>()
        || pUIElement->OfTypeByIndex<KnownTypeIndex::Border>()
        || pUIElement->OfTypeByIndex<KnownTypeIndex::ContentPresenter>())
    {
        CFrameworkElement* pFrameworkElement = static_cast<CFrameworkElement*>(pUIElement);
        XFLOAT width = pFrameworkElement->GetActualWidth();
        XFLOAT height = pFrameworkElement->GetActualHeight();
        XCORNERRADIUS cornerRadius = pFrameworkElement->GetCornerRadius();
        XTHICKNESS borderThickness;

        const bool useLayoutRounding = pFrameworkElement->GetUseLayoutRounding();

        if (useLayoutRounding)
        {
            width = pFrameworkElement->LayoutRound(width);
            height = pFrameworkElement->LayoutRound(height);

            borderThickness = CBorder::GetLayoutRoundedThickness(pFrameworkElement);
        }
        else
        {
            borderThickness = pFrameworkElement->GetBorderThickness();
        }

        GetNinegridForBorderParameters(borderThickness, cornerRadius, forceIntegerInsets, pFrameworkElement, ninegrid);

        //
        // If the border completely overlaps the background, there's no overdraw benefit from using a nine grid. There are
        // also cases where the nine grid we calculate would actually be larger than the element (see picture in banner).
        // We skip using a nine grid in those cases as well. In those cases we just use an unstretched mask.
        //
        // NOTE: We could choose to draw a ninegrid for degenerate Borders if they don't require a mask (square corners),
        //           in order to save the rasterization cost. However, we'd need to handle some extra degenerate cases here,
        //           since when the BorderThickness exceeds the Border's width/height, it gets clipped at the bottom/right side.
        //
        return width > (ninegrid->left + ninegrid->right)
            && height > (ninegrid->top + ninegrid->bottom);
    }
    else
    {
        return false;
    }
}

/* static */ void HWWalk::GetNinegridForBorderParameters(
    const XTHICKNESS& borderThickness,
    const XCORNERRADIUS& cornerRadius,
    const bool useLayoutRounding,
    _In_opt_ CUIElement* layoutRoundingElement,
    _Out_ XTHICKNESS* ninegrid)
{
    //
    // Nine grids are used to draw both the border and the background of border-like elements (Border/Grid/ContentPresenter).
    // They're used for different purposes depending on what they're drawing.
    //
    //    - For the border portion, a hollow nine grid is used to avoid overdraw.
    //
    //    - For both the border and the background, a nine grid can stretch up a small alpha mask to save memory. We can
    //      collapse the alpha mask into just the four corner regions plus an extra pixel along both dimensions in the
    //      middle that will be stretched to fill the straight edges. So
    //
    //             /osoooooooooooooooooooooooooooooooooso\
    //           /m/                                     +m\
    //          /h                                         h\
    //          M`                                         `M
    //          M                                           M
    //          M                                           M
    //          M                                           M
    //          M                                           M
    //          M                                           M
    //          M                                           M
    //          M`                                         `M
    //          \h                                         h/
    //           \m\                                     /m/
    //             \osoooooooooooooooooooooooooooooooooso/
    //
    //      gets collapsed to
    //
    //             /ososo\
    //           /m/     +m\
    //          /h         h\
    //          M`         `M
    //          M           M
    //          M`         `M
    //          \h         h/
    //           \m\     /m/
    //             \ososo/
    //
    // In both these cases, the nine grid insets that we calculate are the same. This is because of a special property
    // for Xaml's border and background alpha masks - they have the same dimensions. In the case of a border with both
    // a border and a background, we'll render the border's alpha mask at the size of the entire element with only the
    // border pixels. We'll render the background's alpha mask at those same dimensions, but the outer regions of that
    // mask corresponding to the border will have transparent pixels. This same surface size means we can use the same
    // nine grid insets to draw both the border visual and the background visual.
    //
    // The corner radius measures from the middle of the stroke, so the distance from the center of the arc to the
    // outside of the border is corner radius + stroke thickness / 2. There are degenerate cases (e.g. the corner
    // radius is 0) where the center of the arc is inside the stroke. In those cases we have to at least fit the
    // entire stroke into the nine grid cell. So the insets of the nine grid becomes the max of:
    //  - stroke thickness
    //  - corner radius + stroke thickness / 2
    //
    ninegrid->top = MAX(borderThickness.top, borderThickness.top/2 + MAX(cornerRadius.topLeft, cornerRadius.topRight));
    ninegrid->bottom = MAX(borderThickness.bottom, borderThickness.bottom/2 + MAX(cornerRadius.bottomLeft, cornerRadius.bottomRight));
    ninegrid->left = MAX(borderThickness.left, borderThickness.left/2 + MAX(cornerRadius.topLeft, cornerRadius.bottomLeft));
    ninegrid->right = MAX(borderThickness.right, borderThickness.right/2 + MAX(cornerRadius.bottomRight, cornerRadius.topRight));

    if (useLayoutRounding)
    {
        // If the caller asks for layout rounding, then they must also provide a reference element to do the rounding.
        ASSERT(layoutRoundingElement);
        ninegrid->top = layoutRoundingElement->LayoutRoundCeiling(ninegrid->top);
        ninegrid->bottom = layoutRoundingElement->LayoutRoundCeiling(ninegrid->bottom);
        ninegrid->left = layoutRoundingElement->LayoutRoundCeiling(ninegrid->left);
        ninegrid->right = layoutRoundingElement->LayoutRoundCeiling(ninegrid->right);
    }
}

// Insert a SpriteVisual into the current ContainerVisual at the appropriate child position
_Check_return_ HRESULT HWWalk::InsertVisualIntoCurrentContainer(
    _In_ WUComp::IVisual* visual,
    _Outptr_ WUComp::IVisualCollection** ppVisualCollectionNoRef)
{
    ASSERT(m_pCurrentContainerNoRef != nullptr);

    xref_ptr<WUComp::IVisualCollection> spVisualCollection;

    if (m_pCurrentSpriteVisualNoRef == nullptr)
    {
        // Bug 43071616: [Watson Failure] caused by STOWED_EXCEPTION_80000013_dcompi.dll!Microsoft::UI::Composition::ContainerVisual::Api::get_Children
        // We're seeing this call return RO_E_CLOSED when we're rendering File Explorer's desktop right click menu. The
        // stack unwound and we lost the context for the error, and we don't see anything out of place when looking at
        // the UIElement tree when we later crash. Failfast here so we'll have more context on what went wrong (and
        // don't have to guess about the UIElement tree that crashed).
        HRESULT hr = m_pCurrentContainerNoRef->get_Children(spVisualCollection.ReleaseAndGetAddressOf());
        if (hr == RO_E_CLOSED)
        {
            IFCFAILFAST(hr);
        }
        IFC_RETURN(hr);
        IFC_RETURN(spVisualCollection->InsertAtBottom(visual));
    }
    else
    {
        IFC_RETURN(m_pCurrentContainerNoRef->get_Children(spVisualCollection.ReleaseAndGetAddressOf()));
        IFC_RETURN(spVisualCollection->InsertAbove(visual, m_pCurrentSpriteVisualNoRef));
    }

    SetLastSpriteVisual(visual);
    *ppVisualCollectionNoRef = spVisualCollection.detach();

    return S_OK;
}

// Insert a SpriteVisual after the current SpriteVisual, when the caller already knows the parent collection
_Check_return_ HRESULT HWWalk::InsertVisualIntoCollection(
    _In_ WUComp::IVisual* visual,
    _In_ WUComp::IVisualCollection* visualCollection)
{
    ASSERT(m_pCurrentContainerNoRef != nullptr);
    ASSERT(m_pCurrentSpriteVisualNoRef != nullptr);

    IFC_RETURN(visualCollection->InsertAbove(visual, m_pCurrentSpriteVisualNoRef));
    SetLastSpriteVisual(visual);

    return S_OK;
}

void HWWalk::ResetRenderingContext()
{
    m_pParentCompNodeNoRef = nullptr;
    m_pPreviousChildCompNodeNoRef = nullptr;
    m_pCurrentContainerNoRef = nullptr;
    m_pCurrentSpriteVisualNoRef = nullptr;
}

void HWWalk::SetLastSpriteVisual(_In_opt_ WUComp::IVisual* spriteVisual)
{
    m_pCurrentSpriteVisualNoRef = spriteVisual;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the ptr into the persistent render data list to the last-added content.
//      This must be called before the render walk proceeds from drawing one element's content
//      to drawing another element.
//
//      If the most recently added content is the same as the previous end of the list, no new
//      content was added and no update should be made. This is only important for the override
//      walk, where render data is accumulated in a single list that can cross comp node boundaries.
//      The current primitive index in the walk always needs to be from the current primitive group,
//      so it should only be updated when new content is added.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT HWWalk::UpdateLastSpriteVisualFromContent(
    _In_ PCRenderDataList* renderData,
    _In_opt_ WUComp::IVisual* previousLastSpriteVisual)
{
    if (!renderData->empty())
    {
        WUComp::IVisual* lastSpriteVisual;
        IFC_RETURN(renderData->back(lastSpriteVisual));

        // previousLastSpriteVisual is NULL in most render walks, so this check always succeeds.
        // It will only fail in an override walk when no new content was added.
        if (previousLastSpriteVisual != lastSpriteVisual)
        {
            SetLastSpriteVisual(lastSpriteVisual);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the ptrs into the persistent render data structures (both the current ContainerVisual
//      and the compositor tree).
//      This must be called before the render walk proceeds from one element to the next so
//      that incremental updates are added in the right order.
//
//------------------------------------------------------------------------------
void HWWalk::UpdateLastSpriteVisualAndCompNodeFromSubgraph(
    _In_ CUIElement *pUIElement,
    _In_ const HWRenderParams &rp)
{
    // If this is a multi-path walk and storage is overridden, we don't want to use the cache from the element
    // because it isn't updated by that walk.
    //
    // Note that this rendering context (the parent for new SpriteVisuals and the reference visual used to insert
    // the next SpriteVisual) is mostly not needed for multi-path LTEs, because the entire subgraph is
    // visited and all render data is regenerated. There is an exception - when we build the cloned comp tree, we
    // still need rendering context to insert the cloned comp nodes. That information is not updated here. Instead,
    // it's updated at the end of RenderProperties as we walk out of UIElements that need comp nodes.
    if (rp.pHWWalk->GetOverrideRenderDataList() == nullptr)
    {
        if (pUIElement->GetCompositionPeer() == nullptr)
        {
            // When we walk out of an element without a comp node, the next SpriteVisual should be added to the
            // same comp node, after the final SpriteVisual in the subtree, so there's no need to call SetCurrentContainer.
            WUComp::IVisual* pLastSpriteVisualFromChild = pUIElement->GetLastSpriteVisual();
            if (pLastSpriteVisualFromChild)
            {
                SetLastSpriteVisual(pLastSpriteVisualFromChild);
            }

            HWCompNode* lastCompNodeInSubtree = pUIElement->GetLastCompNode();
            if (lastCompNodeInSubtree != nullptr)
            {
                // If this element's subtree introduced a comp node, use the last comp node in its subtree as the reference
                // when adding new comp nodes above this subtree. If this element's subtree didn't introduce a comp node, then
                // we can use the one we already have.
                SetPreviousChildCompNode(pUIElement->GetLastCompNode());
            }

            // No need to call SetParentCompNode. The next element's comp node, if it has one, will be parented to the
            // same comp node parent as this element's comp node.
        }
        else
        {
            // When we walk out of an element with a comp node, the next sprite visual should be added to the parent
            // comp node, after the element's comp node's prepend visual.
            HWCompTreeNodeWinRT* compNode = static_cast<HWCompTreeNodeWinRT*>(pUIElement->GetCompositionPeer());
            HWCompTreeNodeWinRT* compNodeParent = static_cast<HWCompTreeNodeWinRT*>(compNode->GetParentInternal(false /* publicOnly */));
            ASSERT(compNodeParent != nullptr
                || pUIElement->OfTypeByIndex<KnownTypeIndex::RootVisual>()
                || pUIElement->OfTypeByIndex<KnownTypeIndex::XamlIslandRoot>());
            if (compNodeParent != nullptr)
            {
                SetCurrentContainer(compNodeParent->GetContainerVisualForChildren());
                SetLastSpriteVisual(compNode->GetReferenceVisualForIncrementalRendering());

                // The last child added to this parent comp node is this element's comp node. This information is needed to
                // correctly place the next comp node in pParentCompNodeNoRef's child collection - it goes after pElementCompNode.
                SetPreviousChildCompNode(compNode);

                // No need to call SetParentCompNode. The next element's comp node, if it has one, will be parented to the
                // same comp node parent as this element's comp node.
                ASSERT(GetParentCompNode() == compNodeParent);
            }
        }
    }
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the current comp node ancestor, used to add new comp nodes in the subgraph
//      or to re-parent existing ones.
//
//      The comp node tree persists frame over frame, so these pointers are used as a bookmark
//      into that data structure so changes in the UIElement tree map into the correct analogues
//      in the comp node tree.
//
//------------------------------------------------------------------------------
void
HWWalk::SetParentCompNode(
    _In_ HWCompTreeNode *pParentCompNode
    )
{
    m_pParentCompNodeNoRef = pParentCompNode;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the last leaf comp node walked into and current primitive group to add content to.
//
//      The comp node tree persists frame over frame, so these pointers are used as a bookmark
//      into that data structure so changes in the UIElement tree map into the correct analogues
//      in the comp node tree.
//
//------------------------------------------------------------------------------
void HWWalk::SetPreviousChildCompNode(_In_ HWCompNode *pPreviousChild)
{
    m_pPreviousChildCompNodeNoRef = pPreviousChild;
}

void HWWalk::SetCurrentContainer(_In_opt_ WUComp::IContainerVisual* container)
{
    m_pCurrentContainerNoRef = container;
    m_pCurrentSpriteVisualNoRef = nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the appropriate CompositorTreeHost
//
//------------------------------------------------------------------------
CompositorTreeHost*
HWWalk::GetCompositorTreeHost(_In_ CWindowRenderTarget *pRenderTarget) const
{
    return pRenderTarget->GetCompositorTreeHost();
}

bool HWWalk::AccumulateLightsAndCheckLightsEnteringTree(
    _In_ CUIElement* uiElementToGetLights,
    _In_ CUIElement* uiElementToAttachLights,
    _In_ xvector<CXamlLight*>* lights,
    bool isInXamlIsland)
{
    bool hasLightEnteredTree = false;

    const auto& xamlLights = uiElementToGetLights->GetXamlLightCollection();
    if (xamlLights != nullptr)
    {
        bool wasLightAdded = false;
        for (const auto& xamlLightDO : *xamlLights)
        {
            CXamlLight* xamlLight = static_cast<CXamlLight*>(xamlLightDO);
            if (!isInXamlIsland
                || xamlLight->IsEnabledInXamlIsland())
            {
                lights->push_back(xamlLight);
                wasLightAdded = true;

                if (xamlLight->m_needsSubtreeRewalk)
                {
                    hasLightEnteredTree = true;
                    xamlLight->m_needsSubtreeRewalk = false;
                }
            }
        }

        if (wasLightAdded)
        {
            ASSERT(uiElementToAttachLights->RequiresCompositionNode());
            HWCompTreeNodeWinRT* compNode = static_cast<HWCompTreeNodeWinRT*>(uiElementToAttachLights->GetCompositionPeer());

            if (compNode != nullptr)
            {
                compNode->SetLightsAttachedToElement();
            }
            else
            {
                // The CompNode may legitimately be NULL if this element is the target of an LTE, or under a portaling transition.
                // See additional comments in UpdateLightTargets().  Just ignore this case for now.
            }
        }
    }

    return hasLightEnteredTree;
}

void HWWalk::UpdateLightTargets(
    _In_ CUIElement* uiElement,
    _In_ HWRenderParams& rp)
{
    // This element should require a comp node (being the target of a light => needing a comp node). It doesn't necessarily have one
    // already. Usually we should have created one when we walked to it, but if this element is the direct target of an LTE, we would
    // have skipped RenderProperties (which creates the comp node) and have gone directly to RenderContentAndChildren.
    ASSERT(uiElement->RequiresCompositionNode());

    std::vector<CXamlLight*> lightsTargetingElement;
    if (rp.m_xamlLights != nullptr)
    {
        for (const auto& light : *(rp.m_xamlLights))
        {
            if (light->HasWUCLight() && uiElement->IsTargetedByLight(light))
            {
                lightsTargetingElement.push_back(light);
            }
        }
    }

    // When a UIElement is explicitly targeted by a light (as opposed to targeted through a brush), we target the container visual
    // for that UIElement, rather than its sprite visuals. Since the container visual may not exist until later in the render walk
    // when the comp node creates it, we'll also delay setting the visual on the WUC light until that point. Stash the list of lights
    // away on the comp node. It will take care of the rest during PushProperties.
    HWCompTreeNodeWinRT* compNode = static_cast<HWCompTreeNodeWinRT*>(uiElement->GetCompositionPeer());
    if (compNode != nullptr)
    {
        compNode->SetLightsTargetingElement(std::move(lightsTargetingElement));
    }
    else
    {
        // Do nothing for now.
        // This element is explicitly targeted by a light in the parent chain, but it's also the target of an LTE, or possibly under
        // a portaling transition, where the primary LTE didn't render.
        // Comp nodes are created in RenderProperties, but the LTE went directly to RenderContentAndChildren for its target. In this case, the
        // LTE should be the target of the light instead. That means LTEs need to check for light target IDs on their target elements.
        // This is an edge case of an edge case (most lights are targeted via brushes, plus LTEs are rare), so we'll ignore it for now.
    }
}

bool HWWalk::UseWUCShapes()
{
    bool velocityEnabled = Feature_WUCShapes::IsEnabled();
    bool testsUseWUC = DCompTreeHost::WUCShapesEnabled();
    return velocityEnabled || testsUseWUC;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      ctor - Create rendering parameters for generating the background part
//      of a border.
//
//------------------------------------------------------------------------------
BorderBackgroundPart::BorderBackgroundPart(
    _In_ CFrameworkElement* pFrameworkElement
    )
    : m_pFrameworkElement(pFrameworkElement)
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the bounds for realizing the brush for the element part.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
BorderBackgroundPart::GetBrushBounds(
    _Out_ XRECTF& partBounds
    )
{
    XRECTF_RB contentBounds;
    IFC_RETURN(m_pFrameworkElement->GetContentInnerBounds(&contentBounds));

    partBounds = ToXRectF(contentBounds);

    bool extendBackgroundUnderBorder =
        (m_pFrameworkElement->GetBackgroundSizing() == DirectUI::BackgroundSizing::OuterBorderEdge);

    if (!extendBackgroundUnderBorder)
    {
        CBorder::HelperDeflateRect(partBounds, m_pFrameworkElement->GetBorderThickness(), partBounds);
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the brush for rendering the element part.
//
//------------------------------------------------------------------------------
CBrush*
BorderBackgroundPart::GetBrush(
    )
{
    return m_pFrameworkElement->GetBackgroundBrush().get();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the cached texture for the element part.
//
//------------------------------------------------------------------------------
HWTexture*
BorderBackgroundPart::GetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization
    )
{
    return pHwShapeRealization->GetFillHwTexture();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Store the cached texture for the element part.
//
//------------------------------------------------------------------------------
void
BorderBackgroundPart::SetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization,
    _In_opt_ HWTexture* pTexture
    )
{
    pHwShapeRealization->SetFillHwTexture(pTexture);
}

bool BorderBackgroundPart::ShouldUseNineGrid()
{
    // The background portion can either use a collapsed nine grid or no nine grid at all. There's no benefit to using
    // a 1:1 mapped nine grid.
    return ShouldUseCollapsedNineGrid();
}

bool BorderBackgroundPart::ShouldUseCollapsedNineGrid()
{
    // Only use the nine grid memory optimization for border-like elements (Border/Grid/ContentPresenter) with a solid
    // color, gradient, or XamlCompositionBrushBase backrounds. Image brushes can have additional clips due to the image's
    // stretch mode, and we haven't tested all combinations for correctness yet.
    CBrush* backgroundBrush = GetBrush();
    return !backgroundBrush
        || backgroundBrush->OfTypeByIndex(KnownTypeIndex::SolidColorBrush)
        || backgroundBrush->OfTypeByIndex(KnownTypeIndex::LinearGradientBrush)
        || backgroundBrush->OfTypeByIndex(KnownTypeIndex::XamlCompositionBrushBase);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      ctor - Create rendering parameters for generating the outline part
//      of a border.
//
//------------------------------------------------------------------------------
BorderOutlinePart::BorderOutlinePart(
    _In_ CFrameworkElement* pFrameworkElement
    )
    : m_pFrameworkElement(pFrameworkElement)
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the bounds for realizing the brush for the element part.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
BorderOutlinePart::GetBrushBounds(
    _Out_ XRECTF& partBounds
    )
{
    XRECTF_RB contentBounds;
    IFC_RETURN(m_pFrameworkElement->GetContentInnerBounds(&contentBounds));

    partBounds = ToXRectF(contentBounds);

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the brush for rendering the element part.
//
//------------------------------------------------------------------------------
CBrush*
BorderOutlinePart::GetBrush(
    )
{
    return m_pFrameworkElement->GetBorderBrush();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the cached texture for the element part.
//
//------------------------------------------------------------------------------
HWTexture*
BorderOutlinePart::GetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization
    )
{
    return pHwShapeRealization->GetStrokeHwTexture();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Store the cached texture for the element part.
//
//------------------------------------------------------------------------------
void
BorderOutlinePart::SetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization,
    _In_opt_ HWTexture* pTexture
    )
{
    pHwShapeRealization->SetStrokeHwTexture(pTexture);
}

bool BorderOutlinePart::ShouldUseNineGrid()
{
    return true;
}

bool BorderOutlinePart::ShouldUseCollapsedNineGrid()
{
    // The border portion can use either a collapsed nine grid to get memory benefits as well as avoiding overdraw, or
    // just a 1:1 mapped nine grid to avoid overdraw.
    // Only use the nine grid memory optimization for border-like elements (Border/Grid/ContentPresenter) with a solid
    // color, gradient, or XamlCompositionBrushBase backrounds. Image brushes can have additional clips due to the image's
    // stretch mode, and we haven't tested all combinations for correctness yet.
    CBrush* borderBrush = GetBrush();
    return !borderBrush
        || borderBrush->OfTypeByIndex(KnownTypeIndex::SolidColorBrush)
        || borderBrush->OfTypeByIndex(KnownTypeIndex::LinearGradientBrush)
        || borderBrush->OfTypeByIndex(KnownTypeIndex::XamlCompositionBrushBase);
}

bool BorderOutlinePart::ShouldUseHollowNineGrid() const
{
    return true;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      ctor - Create rendering parameters for generating the fill part
//      of a shape.
//
//------------------------------------------------------------------------------
ShapeFillPart::ShapeFillPart(
    _In_ CShape* pShape
    )
    : m_pShape(pShape)
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the bounds for realizing the brush for the element part.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
ShapeFillPart::GetBrushBounds(
    _Out_ XRECTF& partBounds
    )
{
    XRECTF_RB fillBounds;
    IFC_RETURN(m_pShape->GetFillBounds(&fillBounds));

    partBounds = ToXRectF(fillBounds);

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the brush for rendering the element part.
//
//------------------------------------------------------------------------------
CBrush*
ShapeFillPart::GetBrush(
    )
{
    return m_pShape->m_pFill;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the cached texture for the element part.
//
//------------------------------------------------------------------------------
HWTexture*
ShapeFillPart::GetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization
    )
{
    return pHwShapeRealization->GetFillHwTexture();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Store the cached texture for the element part.
//
//------------------------------------------------------------------------------
void
ShapeFillPart::SetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization,
    _In_opt_ HWTexture* pTexture
    )
{
    pHwShapeRealization->SetFillHwTexture(pTexture);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      ctor - Create rendering parameters for generating the stroke part
//      of a shape.
//
//------------------------------------------------------------------------------
ShapeStrokePart::ShapeStrokePart(
    _In_ CShape* pShape
    )
    : m_pShape(pShape)
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the bounds for realizing the brush for the element part.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
ShapeStrokePart::GetBrushBounds(
    _Out_ XRECTF& partBounds
    )
{
    XRECTF_RB strokeBounds;
    IFC_RETURN(m_pShape->GetStrokeBounds(&strokeBounds));

    partBounds = ToXRectF(strokeBounds);

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the brush for rendering the element part.
//
//------------------------------------------------------------------------------
CBrush*
ShapeStrokePart::GetBrush(
    )
{
    return m_pShape->GetStroke().get();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the cached texture for the element part.
//
//------------------------------------------------------------------------------
HWTexture*
ShapeStrokePart::GetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization
    )
{
    return pHwShapeRealization->GetStrokeHwTexture();
}


//------------------------------------------------------------------------------
//
//  Synopsis:
//      Store the cached texture for the element part.
//
//------------------------------------------------------------------------------
void
ShapeStrokePart::SetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization,
    _In_opt_ HWTexture* pTexture
    )
{
    pHwShapeRealization->SetStrokeHwTexture(pTexture);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      ctor - Create rendering parameters for generating the path owned by the
//      given CListViewBaseItemChrome.
//
//------------------------------------------------------------------------------
ChromedPathFillPart::ChromedPathFillPart(
    _In_ CListViewBaseItemChrome* pChrome,
    _In_ CBrush* pBrush
    )
    : m_pBrush(pBrush)
    , m_pChrome(pChrome)
    , m_pSecondaryChrome(NULL)
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      ctor - Create rendering parameters for generating the path owned by the
//      given CListViewBaseItemSecondaryChrome.
//
//------------------------------------------------------------------------------
ChromedPathFillPart::ChromedPathFillPart(
    _In_ CListViewBaseItemSecondaryChrome* pSecondaryChrome,
    _In_ CBrush* pBrush
    )
    : m_pBrush(pBrush)
    , m_pChrome(NULL)
    , m_pSecondaryChrome(pSecondaryChrome)
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the bounds for realizing the brush for the element part.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
ChromedPathFillPart::GetBrushBounds(
    _Out_ XRECTF& partBounds
    )
{
    EmptyRectF(&partBounds);

    if (m_pBrush)
    {
        XRECTF_RB fillBounds;
        EmptyRectF(&fillBounds);

        if (m_pChrome)
        {
            IFC_RETURN(m_pChrome->GetCheckMarkBounds(&fillBounds));
        }
        else if (m_pSecondaryChrome)
        {
            IFC_RETURN(m_pSecondaryChrome->GetEarmarkBounds(&fillBounds));
        }

        partBounds = ToXRectF(fillBounds);
    }


    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the brush for rendering the element part.
//
//------------------------------------------------------------------------------
CBrush*
ChromedPathFillPart::GetBrush(
    )
{
    return m_pBrush;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Get the cached texture for the element part.
//
//------------------------------------------------------------------------------
HWTexture*
ChromedPathFillPart::GetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization
    )
{
    return pHwShapeRealization->GetFillHwTexture();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Store the cached texture for the element part.
//
//------------------------------------------------------------------------------
void
ChromedPathFillPart::SetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization,
    _In_opt_ HWTexture* pTexture
    )
{
    pHwShapeRealization->SetFillHwTexture(pTexture);
}

ChromedPathStrokePart::ChromedPathStrokePart(
    _In_ const XRECTF& bounds,
    _In_ CBrush* pBrush)
    : m_pBrushNoRef(pBrush)
    , m_bounds(bounds)
{
}

_Check_return_ HRESULT ChromedPathStrokePart::GetBrushBounds(_Out_ XRECTF& partBounds)
{
    EmptyRectF(&partBounds);

    if (m_pBrushNoRef != nullptr)
    {
        partBounds = m_bounds;
    }

    return S_OK;
}

CBrush* ChromedPathStrokePart::GetBrush()
{
    return m_pBrushNoRef;
}

HWTexture* ChromedPathStrokePart::GetCachedHwTexture(_In_ HWShapeRealization *pHwShapeRealization)
{
    return pHwShapeRealization->GetStrokeHwTexture();
}

void ChromedPathStrokePart::SetCachedHwTexture(
    _In_ HWShapeRealization *pHwShapeRealization,
    _In_opt_ HWTexture* pTexture)
{
    pHwShapeRealization->SetStrokeHwTexture(pTexture);
}

bool ShouldOverrideRenderOpacity(float opacity, CUIElement *pUIElement)
{
    if (opacity > 0 && opacity < 1 &&
        pUIElement->GetContext()->GetFrameworkTheming()->HasHighContrastTheme())
    {
        CValue highContrastAdjustment;
        IFCFAILFAST(pUIElement->GetValueByIndex(KnownPropertyIndex::UIElement_HighContrastAdjustment, &highContrastAdjustment));
        auto elementHighContrastAdjustment = static_cast<DirectUI::ElementHighContrastAdjustment>(highContrastAdjustment.AsEnum());

        if (elementHighContrastAdjustment == DirectUI::ElementHighContrastAdjustment::Application)
        {
            DirectUI::ApplicationHighContrastAdjustment applicationHighContrastAdjustment;
            IFCFAILFAST(CApplication::GetApplicationHighContrastAdjustment(&applicationHighContrastAdjustment));

            return (applicationHighContrastAdjustment == DirectUI::ApplicationHighContrastAdjustment::Auto);
        }
        else
        {
            return (elementHighContrastAdjustment == DirectUI::ElementHighContrastAdjustment::Auto);
        }
    }

    return false;
}

