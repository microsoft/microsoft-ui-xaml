// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      GPU rendering element tree walker

#pragma once

#ifndef HWWALK_H
#define HWWALK_H

#include "MaxTextureSizeProvider.h"
#include <fwd/windows.ui.composition.h>

class CUIElement;
class CShape;
class CRectangle;
class CImage;
class CTextBoxBase;
class CTextBox;
class CMediaBase;
class CPanel;
class CRichTextBlock;
class CRichTextBlockOverflow;
class CTextSelectionGripper;
class CGlyphs;
class CTextBoxView;
class CLayoutTransitionElement;
class CPopup;
class CGeometry;
class CSwapChainElement;
class CRenderTargetElementData;
class CListViewBaseItemChrome;
class CListViewBaseItemSecondaryChrome;
class CContentControl;
class CCalendarViewBaseItemChrome;
class CXamlLight;
class CWindowRenderTarget;
class CFrameworkElement;
class CImageBrush;
enum class CalendarViewBaseItemChromeLayerPosition;
enum ListViewBaseItemChromeLayerPosition;

class HWTexture;
class HWCompNode;
class HWCompTreeNode;
class HWCompLeafNode;
class HWCompRenderDataNode;
class HWCompMediaNode;
class HWTextureManager;
class HWWalk;
class SurfaceCache;

class IMaskPartRenderParams;
class IContentRenderer;
class HWRealization;
class HWShapeRealization;

class HWClip;

class TransformAndClipStack;
class CTransformToRoot;

class DCompTreeHost;

class CompositorTreeHost;

enum HWTextureFlags
{
    HWTextureFlags_None               = 0,
    HWTextureFlags_IncludePadding     = 0x1,
    HWTextureFlags_IsVirtual          = 0x2,
    HWTextureFlags_IsOpaque           = 0x4,
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Parameters for the hardware render walk
//
//------------------------------------------------------------------------------
struct HWRenderParamsBase
{
    HWRenderParamsBase()
        : pTransformsAndClipsToCompNode(NULL)
        , opacityToCompNode(1.0f)
        , opacityFromCompNodeToRoot(1.0f)
        , pHWWalk(NULL)
        , pRenderTarget(NULL)
        , pContentRenderer(NULL)
        , isTransformAnimating(FALSE)
        , isOpacityAnimating(FALSE)
        , forceRedraw(FALSE)
        , isEnteringScene(FALSE)
        , hasRealizationTransformAboveCompNodeChanged(FALSE)
        , forceUpdateCompNodes(FALSE)
        , hasOpacityAnimatingChanged(FALSE)
        , hasPropertyToCompNodeChanged(FALSE)
        , m_useDCompAnimations(false)
        , m_isHitTestVisibleSubtree(true)
        , m_shouldRerenderSubtreeForNewLight(false)
        , m_isInXamlIsland(false)
        , m_xamlLights(nullptr)
        , m_skipLightsOnThisElementNoRef(nullptr)
    {
    }

    bool NeedsToWalkSubtree() const
    {
        return HasInheritedDirtiness();
    }

    bool HasInheritedDirtiness() const
    {
        return forceRedraw
            || isEnteringScene
            || hasOpacityAnimatingChanged
            || hasRealizationTransformAboveCompNodeChanged
            || hasPropertyToCompNodeChanged
            || m_shouldRerenderSubtreeForNewLight;
    }

    virtual CMILMatrix GetRasterizationScaleTransform(_In_ CUIElement *pElement) const;

    //
    // The transforms and cumulative convex polygon clips relative to the previous HWCompTreeNode.
    // Not owned by HWRenderParams, generally by the stack frame.
    //
    // TODO: JCOMP: Projection implies comp node now.  This doesn't need to be a full stack anymore.
    // Rotation/skew implies comp node too, so this can just be a matrix and a XRECTF.
    const TransformAndClipStack *pTransformsAndClipsToCompNode;

    XFLOAT opacityToCompNode;                   // Cumulative opacity to the previous compnode
    XFLOAT opacityFromCompNodeToRoot;           // Cumulative opacity to from comp not to root - only useful if isOpacityAnimating is FALSE
    HWWalk *pHWWalk;
    CWindowRenderTarget *pRenderTarget; // TODO: This thing is passed around a lot of places. Is it actually used anywhere?
    IContentRenderer *pContentRenderer;
    // TODO: Convert to bool and pack
    bool isTransformAnimating : 1; // True if there is any transform-affecting animation running on this element or an ancestor. This includes projections.
    bool isOpacityAnimating : 1; // True if there is any opacity-affecting animation running on this element or an ancestor.

    // Inherited dirty flags.
    // NOTE: Any additions here need to consider the 'redirected' drawing case as well, see RenderProperties.
    bool forceRedraw                  : 1;  // True iff the render walk must re-create all render data (e.g. device lost recovery)
    bool isEnteringScene              : 1;  // True iff this element or an ancestor was not in the scene last time it was rendered.
    bool hasOpacityAnimatingChanged   : 1;  // True iff the 'isOpacityAnimating' flag is different this frame than last.
    bool hasPropertyToCompNodeChanged : 1;  // True iff one of the collected properties from the nearest comp node to here has changed.
                                             // This includes the reference 'nearest' comp node itself changing.
    bool hasRealizationTransformAboveCompNodeChanged : 1;  // True iff the world transform has changed from the root to the parent comp node since last frame.
                                                 // This is not necessarily true if the parent comp node has changed, but hasPropertyToCompNodeChanged will be.
    bool forceUpdateCompNodes           : 1; // True iff we're forcing an update of the properties on CompNodes in this subtree.

    //
    // The window size, with zoom scale taken into account. Full screen elements will size themselves to this, and will be
    // stretched by the scale at the root to become the size of the full window. Layout also happens at this size.
    //
    // e.g. A 1920x1200 window at zoom level 1.4x will have effective window size 1371.4 x 857.1. Full screen elements size
    // themselves to this, then they are attached to the DComp tree under the root visual with 1.4x scale. The final size
    // of these elements will be 1371.4 * 1.4 = 1920 by 857.1 * 1.4 = 1200.
    //
    XSIZEF effectiveWindowSize;

    // A list of lights that are attached to the ancestor chain at any point in the walk. Updated as we visit elements during the
    // render walk. Any UIElement with a XamlLight.TargetId will look in this list for lights with matching target IDs.
    xvector<CXamlLight*>* m_xamlLights;
    // We make an exception for the public root - any lights set on the root scroll viewer will be treated as if they were set on the root
    // visual. We assume that the app only put them on the root scroll viewer because that's as high up as the app can reach in the UIElement
    // tree. When we render the root visual, we attach the lights from the root scroll viewer immediately, which means when we render the
    // root scroll viewer, we want to skip attaching lights to prevent attaching them twice. This field exists to check for the root scroll.
    // viewer. The alternative is to check each element that we render against the root scroll viewer, which requires a walk up the tree for
    // every UIElement. Note that if Window.Content is a canvas, then there is no root scroll viewer, and we'll peek into the canvas instead.
    CUIElement* m_skipLightsOnThisElementNoRef;

    bool m_useDCompAnimations : 1;
    bool m_isHitTestVisibleSubtree : 1;     // If false, all SpriteVisuals created in this subtree will be marked as hit-test-invisible

    // Used when a XamlLight enters the tree. That entire subtree must be rewalked to pick up the light's new targets. Now that brushes
    // are targeted, we don't have a good way of retargeting existing sprite visuals, because we don't have a way of associating the
    // existing sprite visuals with the Xaml brushes that created them. For now we dirty the entire subtree and have it regenerate its
    // content.
    bool m_shouldRerenderSubtreeForNewLight : 1;

    // Marks when we're rendering inside a Xaml island. MUX reveal lights are disabled inside Xaml islands.
    bool m_isInXamlIsland : 1;
};




//------------------------------------------------------------------------------
//
//  Synopsis:
//      Parameters for the hardware render walk
//
//------------------------------------------------------------------------------
struct HWRenderParams : public HWRenderParamsBase
{
    CMILMatrix GetRasterizationScaleTransform(_In_ CUIElement *pElement) const override;

    void ExtractTransformToRoot(_Out_ CTransformToRoot *pTransformToRoot2D);

    //
    // The world transform to the root. Used to size and position realizations.
    //
    CTransformToRoot *pTransformToRoot;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Parameters for the hardware render walk, generated and used by one
//      element and not passed to children.
//
//------------------------------------------------------------------------------
struct HWElementRenderParams
{
    HWElementRenderParams()
        : isFillBrushAnimating(false)
        , isStrokeBrushAnimating(false)
        , m_realizationScaleX(1.0f)
        , m_realizationScaleY(1.0f)
    {
    }

    bool isFillBrushAnimating;
    bool isStrokeBrushAnimating;

    // The realization scales used for alpha masks are needed when setting insets/inset scales on a WUC
    // CompositionNineGridBrush.
    float m_realizationScaleX;
    float m_realizationScaleY;
};

//------------------------------------------------------------------------------
//
//  Class:     HWWalk
//
//  Synopsis:  GPU rendering element tree walker
//
//------------------------------------------------------------------------------
class HWWalk final : public CInterlockedReferenceCount
{
public:
    friend class CListViewBaseItemChrome;
    friend class CCalendarViewBaseItemChrome;

    _Check_return_ static HRESULT Create(
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ CCoreServices *pCore,
        _In_ MaxTextureSizeProvider& maxTextureSizeProvider,
        _Outptr_ HWWalk **ppHwWalk
        );

    WUComp::IVisual* GetLastSpriteVisual() const;

    void ResetRenderingContext();

    _Check_return_ HRESULT InsertVisualIntoCurrentContainer(
        _In_ WUComp::IVisual* visual,
        _Outptr_ WUComp::IVisualCollection** ppVisualCollectionNoRef);

    _Check_return_ HRESULT InsertVisualIntoCollection(
        _In_ WUComp::IVisual* visual,
        _In_ WUComp::IVisualCollection* visualCollection);

    void UpdateLastSpriteVisualAndCompNodeFromSubgraph(
        _In_ CUIElement *pChild,
        _In_ const HWRenderParams &rp);

    void SetParentCompNode(_In_ HWCompTreeNode *pParentCompNode);

    void SetPreviousChildCompNode(_In_ HWCompNode *pPreviousChild);

    void SetCurrentContainer(_In_opt_ WUComp::IContainerVisual* container);

    HWCompTreeNode* GetParentCompNode() const { return m_pParentCompNodeNoRef; }
    HWCompNode* GetPreviousChildCompNode() const { return m_pPreviousChildCompNodeNoRef; }

    PCRenderDataList* GetOverrideRenderDataList()
    {
        return m_pOverrideRenderDataListNoRef;
    }

    void SetOverrideRenderDataList(_In_ PCRenderDataList *pOverrideList)
    {
        ASSERT(m_pOverrideRenderDataListNoRef == NULL || pOverrideList == NULL);
        m_pOverrideRenderDataListNoRef = pOverrideList;
    }

    HWTextureManager *GetTextureManager()
    {
        return m_pTextureManager;
    }

    SurfaceCache* GetSurfaceCache()
    {
        return m_pSurfaceCache;
    }

    CompositorTreeHost* GetCompositorTreeHost(_In_ CWindowRenderTarget *pRenderTarget) const;

    void HandleDeviceLost(bool cleanupDComp);

    _Check_return_ HRESULT RenderRoot(
        _In_ CUIElement* pVisualRoot,
        _In_ CWindowRenderTarget * pRenderTarget,
        bool forceRedraw,
        XFLOAT zoomScale,
        bool hasUIAClientsListeningToStructure
        );

    void ResetEtwData();
    int GetElementsVisited() const;
    int GetElementsRendered() const;

    static bool GetNinegridForBorderElement(
        _In_ CUIElement *pUIElement,
        const bool forceIntegerInsets,
        _Out_ XTHICKNESS *pNinegrid);

    static void GetNinegridForBorderParameters(
        const XTHICKNESS& borderThickness,
        const XCORNERRADIUS& cornerRadius,
        const bool useLayoutRounding,
        _In_opt_ CUIElement* layoutRoundingElement,
        _Out_ XTHICKNESS* ninegrid);

    static _Check_return_ HRESULT CalculateMediaEngineAndDCompParameters(
        _Inout_ CFrameworkElement *pElement,
        bool inProperFullWindowMode,
        XSIZE *pWindowSize,
        _Out_ XRECTF_RB *pNormalizedSourceRect,
        _Out_ XRECT *pDestinationRect,
        _Out_ CMILMatrix *pStretchTransform,
        _Out_ XRECTF *pStretchClip
        );

    static bool ShouldImageUseNineGrid(_In_ CImage* pImage);
    static bool IsShapeThatRequiresSoftwareRendering(_In_ CUIElement* pUIElement);
    static bool IsBorderLikeElementThatRequiresSoftwareRendering(_In_ CUIElement* pUIElement);

private:
    HWWalk(_In_ CWindowRenderTarget *pRenderTarget, _In_ MaxTextureSizeProvider& maxTextureSizeProvider);
    ~HWWalk() override;

    void SetLastSpriteVisual(_In_opt_ WUComp::IVisual* sprite);

    _Check_return_ HRESULT UpdateLastSpriteVisualFromContent(
        _In_ PCRenderDataList* renderData,
        _In_opt_ WUComp::IVisual* previousLastSpriteVisual);

    _Check_return_ HRESULT Initialize(_In_ CCoreServices *pCore);

    _Check_return_ HRESULT RenderRootImpl(
        _In_ CUIElement* pVisualRoot,
        _In_ CWindowRenderTarget *pRenderTarget,
        bool forceRedraw,
        XFLOAT zoomScale,
        _In_opt_ TransformAndClipStack *pPrependTransformAndClip,
        bool useDCompAnimations,
        bool hasUIAClientsListeningToStructure
        );

    static _Check_return_ HRESULT CalendarViewBaseItemRenderChildren(
        _In_ CCalendarViewBaseItemChrome *pItem,
        _In_ const HWRenderParams &rp
        );

    static _Check_return_ HRESULT LayoutTransitionElementRenderTarget(
        _In_ CLayoutTransitionElement *pLTElement,
        _In_ const HWRenderParams &myRP
        );

    static _Check_return_ HRESULT PopupRenderChild(
        _In_ CPopup *pPopup,
        _In_ const HWRenderParams &myRP
        );

    static _Check_return_ HRESULT PopupRootRenderChildren(
        _In_ CPopupRoot *pPopupRoot,
        _In_ const HWRenderParams &rp
        );

    static _Check_return_ HRESULT RenderConnectedAnimationUnloadingElements(
        _In_ CConnectedAnimationRoot *pRoot,
        _In_ const HWRenderParams &rp
    );

    static _Check_return_ HRESULT Render(
        _In_ CUIElement *pUIElement,
        _In_ const HWRenderParams &rp,
        bool redirectedDraw
        );

    static _Check_return_ HRESULT RenderProperties(
        _In_ CUIElement *pUIElement,
        _In_ const HWRenderParams &parentRP,
        bool requiresRedirectedDrawing,
        _Out_ bool *pSkipRenderWhileInheritedCollapsed,
        _Out_ bool *pSkipRenderWhileTransparent,
        _Out_ bool *pSkipRenderWhileClippedOut,
        _Out_ bool *pSkipRenderWhileLayoutClippedOut,
        _Out_ bool *pSkipRenderWhileTransformTooSmall
        ) noexcept;

    static _Check_return_ HRESULT RenderContentAndChildren(
        _In_ CUIElement *pUIElement,
        _Inout_ HWRenderParams& myRP,
        bool requiresRedirectedDrawing,
        bool elementHasCompNode
        ) noexcept;

    static _Check_return_ HRESULT RenderChildren(
        _In_ CUIElement *pUIElement,
        _In_ const HWRenderParams &rp
        );

    static _Check_return_ HRESULT RenderChildrenDefault(
        _In_ CUIElement *pUIElement,
        _In_ const HWRenderParams &rp,
        bool redirectedDrawForChildren
        );

    static _Check_return_ HRESULT LeaveSceneRecursive(_In_ CUIElement *pUIElement);

    static CUIElement* GetRedirectionTarget(
        _In_ CUIElement *pUIE
        );

    static bool IsAbsolutelyPositioned(
        _In_ CUIElement *pUIE
        );

    static _Check_return_ HRESULT EnsureCompositionPeer(
        _In_ CUIElement *pUIElement,
        _In_ const HWRenderParams &parentRP,
        _In_ const HWRenderParams &myRP,
        _In_opt_ HWCompTreeNode *pRedirectionTarget,
        _In_opt_ const CMILMatrix *pTransformsFromRedirectionTargetToRedirectionCompNode,
        _Outptr_ HWCompTreeNode **ppElementNode,
        _Outptr_ HWCompLeafNode **ppContentNode,
        _Outptr_result_maybenull_ WUComp::IContainerVisual** ppRenderDataParent,
        _Out_ bool *pHasRealizationTransformAboveCompNodeChanged
        );

    static _Check_return_ HRESULT EnsureElementAndContentNodes(
        _In_ CUIElement *pUIElement,
        _In_opt_ HWCompTreeNode *pExistingCompositionPeer,
        _In_ const HWRenderParams &myRP,
        _In_ const HWRenderParams &parentRP,
        _In_opt_ HWCompTreeNode *pRedirectionTarget,
        _In_opt_ const CMILMatrix *pTransformsFromRedirectionTargetToRedirectionCompNode,
        _Outptr_ HWCompTreeNode **ppElementNode,
        _Outptr_ HWCompLeafNode **ppContentNode,
        _Outptr_result_maybenull_ WUComp::IContainerVisual** ppRenderDataParent
        ) noexcept;

    static _Check_return_ HRESULT IntersectClipInCompNodeSpace(
        _In_opt_ CGeometry *pClip,
        _Inout_ TransformAndClipStack *pCombinedTransformsAndClipsToCompNode,
        _Out_ bool *pSkipRenderWhileClippedOut
        );

    static void IntersectClipRectInCompNodeSpace(
        XRECTF& clipRect,
        _Inout_ TransformAndClipStack *combinedTransformsAndClipsToCompNode,
        _Out_ bool *skipRenderWhileClippedOut
        );

    static bool IsShapeMaskRequiredForRectangle(
        _In_ CRectangle *pRectangleElement
        );

    static bool IsShapeMaskRequiredForBorder(_In_ CFrameworkElement *pFrameworkElement);

    static void GetUpdateVideoStreamRelatedParameters(
        _In_ CFrameworkElement *pElement,
        _In_ const XRECTF *pElementSize,
        _In_ const XRECTF *pMediaNaturalSizeF,
        bool inProperFullWindowMode,
        _Inout_ CMILMatrix *pStretchTransform,
        _Out_ XRECTF_RB *pNormalizedSourceRect,
        _Out_ XRECT *pDestRect,
        _Out_ XRECTF *pStretchClip
        );

    static void RoundSourceRect(
        _In_ const XRECTF *pSourceRect,
        _Out_ XRECTF *pRoundedSourceRect,
        _Out_ bool *pRectChanged
        );

    static _Check_return_ HRESULT RequestImageDecode(
        _In_ const XRECTF *pBounds,
        _In_ const HWRenderParamsBase& myRP,
        _In_ CImageBrush *pImageBrush,
        _In_opt_ CUIElement *pUIElement,
        _In_opt_ const XTHICKNESS *pNinegrid);

    static void TraceElementAccessibility(
        _In_ CUIElement* pUIElement);

    // Collect lights from a UIElement in the render walk
    // Returns whether a light on that UIElement has entered the tree since the previous frame.
    static bool AccumulateLightsAndCheckLightsEnteringTree(
        _In_ CUIElement* uiElementToGetLights,
        _In_ CUIElement* uiElementToAttachLights,
        _In_ xvector<CXamlLight*>* lights,
        bool isInXamlIsland);

    // Check a UIElement for light targets and add its visuals to the CompositionLight.Targets collection of lights that are targeting it
    static void UpdateLightTargets(
        _In_ CUIElement* uiElement,
        _In_ HWRenderParams& rp);

    bool UseWUCShapes();

private:
    friend class BaseContentRenderer;
    friend class VisualContentRenderer;

    CWindowRenderTarget *m_pRenderTargetNoRef;
    HWTextureManager *m_pTextureManager;
    MaxTextureSizeProvider& m_maxTextureSizeProvider;
    SurfaceCache *m_pSurfaceCache;

    // These members are only valid to access during a render walk.
    // They're stored here to minimize the size of the HWRenderParams, since they
    // never change for the duration of a single walk.

    // Valid only during the render walk.
    // A pointer to the current ContainerVisual and the current SpriteVisual.
    // The current SpriteVisual will be updated as we do the render walk, and new SpriteVisuals will added behind it.
    WUComp::IContainerVisual* m_pCurrentContainerNoRef;
    WUComp::IVisual* m_pCurrentSpriteVisualNoRef;   // Typed to IVisual for convenience with inserting in child collections

    // Valid only during the render walk. A pointer to the current tree node, where new comp nodes should be added.
    HWCompTreeNode *m_pParentCompNodeNoRef;

    // Valid only during the render walk. A pointer to the last comp node child added to m_pParentCompNodeNoRef at this
    // point in the walk. This will be a tree node.
    HWCompNode *m_pPreviousChildCompNodeNoRef;

    // Valid only during the render walk. Render data should be stored here, if present, instead of in the storage on the
    // UIElements themselves. This is used for rendering the same subgraph multiple times in a single frame (e.g. more
    // than one LayoutTransitionElement with the same target UIElement).
    PCRenderDataList *m_pOverrideRenderDataListNoRef;

    bool m_UIAClientsListeningToStructure = false;

    bool m_inSwapChainPanelSubtree : 1;  // true while we're render-walking the subtree of a SwapChainPanel
    int m_elementsVisited;  // The number of UIElements we visited each render walk
    int m_elementsRendered; // The number of UIElements we rendered each render walk
};

class IMaskPartRenderParams
{
public:
    virtual _Check_return_ HRESULT GetBrushBounds(
        _Out_ XRECTF& partBounds
        ) = 0;

    virtual CBrush* GetBrush(
        ) = 0;

    virtual HWTexture* GetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization
        ) = 0;

    virtual void SetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization,
        _In_opt_ HWTexture* pTexture
        ) = 0;

    virtual bool ShouldUseNineGrid()
    {
        return false;
    }

    virtual bool ShouldUseCollapsedNineGrid()
    {
        return false;
    }

    virtual bool ShouldUseHollowNineGrid() const
    {
        return false;
    }
};

class BorderBackgroundPart : public IMaskPartRenderParams
{
public:
    BorderBackgroundPart(
        _In_ CFrameworkElement* pFrameworkElement
        );

    _Check_return_ HRESULT GetBrushBounds(
        _Out_ XRECTF& partBounds
        ) override;

    CBrush* GetBrush(
        ) override;

    HWTexture* GetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization
        ) override;

    void SetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization,
        _In_opt_ HWTexture* pTexture
        ) override;

    bool ShouldUseNineGrid() override;

    bool ShouldUseCollapsedNineGrid() override;

private:
    CFrameworkElement* m_pFrameworkElement;
};

class BorderOutlinePart : public IMaskPartRenderParams
{
public:
    BorderOutlinePart(
        _In_ CFrameworkElement* pFrameworkElement
        );

    _Check_return_ HRESULT GetBrushBounds(
        _Out_ XRECTF& partBounds
        ) override;

    CBrush* GetBrush(
        ) override;

    HWTexture* GetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization
        ) override;

    void SetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization,
        _In_opt_ HWTexture* pTexture
        ) override;

    bool ShouldUseNineGrid() override;

    bool ShouldUseCollapsedNineGrid() override;

    bool ShouldUseHollowNineGrid() const override;

private:
    CFrameworkElement* m_pFrameworkElement;
};


// This class exists to support CListViewBaseItemChrome. It delegates
// various details of the fill to the chrome elements. It supports both
// CListViewBaseItemChrome and CListViewBaseItemSecondaryChrome. If more
// chromes are added, a specific chrome IMaskPartRenderParams interface
// should be added.
class ChromedPathFillPart : public IMaskPartRenderParams
{
public:
    ChromedPathFillPart(
        _In_ CListViewBaseItemChrome* pChrome,
        _In_ CBrush* pBrush
        );

    ChromedPathFillPart(
        _In_ CListViewBaseItemSecondaryChrome* pSecondaryChrome,
        _In_ CBrush* pBrush
        );

    _Check_return_ HRESULT GetBrushBounds(
        _Out_ XRECTF& partBounds
        ) override;

    CBrush* GetBrush(
        ) override;

    HWTexture* GetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization
        ) override;

    void SetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization,
        _In_opt_ HWTexture* pTexture
        ) override;

private:
    CListViewBaseItemChrome* m_pChrome;
    CListViewBaseItemSecondaryChrome* m_pSecondaryChrome;
    CBrush* m_pBrush;
};

class ChromedPathStrokePart : public IMaskPartRenderParams
{
public:
    ChromedPathStrokePart(
        _In_ const XRECTF& bounds,
        _In_ CBrush* pBrush);

    _Check_return_ HRESULT GetBrushBounds(_Out_ XRECTF& partBounds) override;

    CBrush* GetBrush() override;

    HWTexture* GetCachedHwTexture(_In_ HWShapeRealization *pHwShapeRealization) override;

    void SetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization,
        _In_opt_ HWTexture* pTexture) override;

private:
    CBrush* m_pBrushNoRef;
    XRECTF m_bounds;
};

class ShapeFillPart : public IMaskPartRenderParams
{
public:
    ShapeFillPart(
        _In_ CShape* pShape
        );

    _Check_return_ HRESULT GetBrushBounds(
        _Out_ XRECTF& partBounds
        ) override;

    CBrush* GetBrush(
        ) override;

    HWTexture* GetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization
        ) override;

    void SetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization,
        _In_opt_ HWTexture* pTexture
        ) override;

private:
    CShape* m_pShape;
};

class ShapeStrokePart : public IMaskPartRenderParams
{
public:
    ShapeStrokePart(
        _In_ CShape* pShape
        );

    _Check_return_ HRESULT GetBrushBounds(
        _Out_ XRECTF& partBounds
        ) override;

    CBrush* GetBrush(
        ) override;

    HWTexture* GetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization
        ) override;

    void SetCachedHwTexture(
        _In_ HWShapeRealization *pHwShapeRealization,
        _In_opt_ HWTexture* pTexture
        ) override;

private:
    CShape* m_pShape;
};

bool ShouldOverrideRenderOpacity(float opacity, CUIElement *pUIElement);

#endif // HWWALK_H

