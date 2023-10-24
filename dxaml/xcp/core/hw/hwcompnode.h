// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Custom compositor tree node for GPU rasterization

#pragma once
#ifndef HWCOMPNODE_H
#define HWCOMPNODE_H

#include "ReferenceCount.h"
#include "palgfx.h"
#include "PalTypes.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include "CDependencyObject.h"
#include "EnumDefs.g.h"
#include "TransformToRoot.h"
#include "PalDirectManipulationService.h"
#include <fwd/windows.ui.composition.h>

class HWCompTreeNode;
class CWindowRenderTarget;
class DCompTreeHost;
class CompositorTreeHost;
class CCoreServices;
class HWTexture;
class DManipDataBase;

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Base class for composition nodes.
//
//------------------------------------------------------------------------------
class HWCompNode : public CDependencyObject
{
public:
    // Tree walk to push updates from compositor tree to underlying DComp tree.
    _Check_return_ HRESULT UpdateTree(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        bool useDCompAnimations,
        bool disablePixelSnapping);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<HWCompNode>::Index;
    }

    virtual _Check_return_ HRESULT Remove();

    // Returns the DComp visual as a WUComp::IVisual.
    virtual xref_ptr<WUComp::IVisual> GetWUCVisual() const;

    // Windowed popups have one WUC visual for the windowed popup, and a different visual used as a placeholder in the main tree.
    // This returns the one in the main tree when we need it for things like the reference visual when adding siblings.
    virtual xref_ptr<WUComp::IVisual> GetWUCVisualInMainTree() const;

    virtual _Check_return_ HRESULT EnsureVisual(_In_opt_ DCompTreeHost *pDCompTreeHost) = 0;

    virtual void ClearVisualContent() { }

    HWCompNode* GetPreviousSibling();

    CompositorTreeHost* GetCompositorTreeHost() const { return m_pCompositorTreeHostNoRef; }

    bool UsesPlaceholderVisual() const { return m_usesPlaceholderVisual; }

    bool HasHitTestVisibleContent() const;

protected:
    HWCompNode(
        _In_ CCoreServices *coreServices,
        _In_ CompositorTreeHost *pCompositorTreeHost);
    ~HWCompNode() override;

    virtual _Check_return_ HRESULT UpdateTreeVirtual(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        bool useDCompAnimations,
        bool disablePixelSnapping);

    CompositorTreeHost *m_pCompositorTreeHostNoRef;
    // Uses a placeholder visual in the DComp tree, because the real visual is added
    // elsewhere. Currently used only by HWWindowedPopupCompTreeNode, when the popup is
    // windowed and the real visual is added to that window.
    bool m_usesPlaceholderVisual : 1;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Base class for leaf nodes in the composition tree.
//
//------------------------------------------------------------------------------
class HWCompLeafNode : public HWCompNode
{
public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<HWCompLeafNode>::Index;
    }

    _Check_return_ HRESULT EnsureVisual(_In_opt_ DCompTreeHost *pDCompTreeHost) override;

protected:
    HWCompLeafNode(
        _In_ CCoreServices *coreServices,
        _In_ CompositorTreeHost *pCompositorTreeHost);

    ~HWCompLeafNode() override;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Stores rendering content in the composition tree.
//      This is a generic base class that doesn't actually "know" what type
//      of content is being rendered.  See derived types for specific usage.
//
//------------------------------------------------------------------------------
class HWCompRenderDataNode : public HWCompLeafNode
{
public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<HWCompRenderDataNode>::Index;
    }

    virtual IUnknown *EnsureRenderDataParent(_In_ DCompTreeHost* dcompTreeHost) = 0;
    virtual IUnknown *GetRenderDataParent() const = 0;

    // Assume primitive render data nodes always have content. SpriteVisual nodes will check for SpriteVisuals.
    virtual bool HasHitTestVisibleVisuals() const { return true; }

protected:
    HWCompRenderDataNode(
        _In_ CCoreServices *coreServices,
        _In_ CompositorTreeHost *pCompositorTreeHost);
    ~HWCompRenderDataNode() override;
};

// HWCompWinRTVisualRenderDataNode is the WinRT Visual-oriented derived class
// which knows about the WinRT ContainerVisual and SpriteVisual.
class HWCompWinRTVisualRenderDataNode : public HWCompRenderDataNode
{
public:
    HWCompWinRTVisualRenderDataNode(
        _In_ CCoreServices *coreServices,
        _In_ CompositorTreeHost *pCompositorTreeHost,
        _In_ WUComp::IContainerVisual *pContainerVisual);

    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *coreServices,
        _In_ CompositorTreeHost *pCompositorTreeHost,
        _In_ WUComp::IContainerVisual *pContainerVisual,
        _Outptr_ HWCompWinRTVisualRenderDataNode **ppCompositorLeafNode);

    _Check_return_ HRESULT EnsureVisual(_In_opt_ DCompTreeHost *pDCompTreeHost) override;
    xref_ptr<WUComp::IVisual> GetWUCVisual() const override;

    IUnknown *EnsureRenderDataParent(_In_ DCompTreeHost* dcompTreeHost) override;
    IUnknown *GetRenderDataParent() const override;

    void ClearVisualContent() final;

    bool HasHitTestVisibleVisuals() const override;

    static bool HasHitTestVisibleSpriteVisuals(_In_ WUComp::IContainerVisual* containerVisual);

protected:
    ~HWCompWinRTVisualRenderDataNode() override;

private:
    xref_ptr<WUComp::IContainerVisual> m_containerVisual;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Stores media content in the composition tree.
//
//------------------------------------------------------------------------------
class HWCompMediaNode : public HWCompLeafNode
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *coreServices,
        _In_ CompositorTreeHost *pCompositorTreeHost,
        _Outptr_ HWCompMediaNode **ppCompositorNode);

    _Check_return_ HRESULT SetMedia(
        _In_ DCompTreeHost *pVisualTreeHost,
        _In_ XHANDLE swapchainHandle,
        _In_ const XRECT &destinationRect,
        _In_ const XMATRIX &stretchTransform,
        _In_ const XRECTF &stretchClip);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<HWCompMediaNode>::Index;
    }

    _Check_return_ HRESULT EnsureVisual(_In_opt_ DCompTreeHost* pDCompTreeHost) override;
    xref_ptr<WUComp::IVisual> GetWUCVisual() const override;

protected:
    HWCompMediaNode(
        _In_ CCoreServices *coreServices,
        _In_ CompositorTreeHost *pCompositorTreeHost);

    ~HWCompMediaNode() override;

private:
    XRECT m_destinationRect;
    XHANDLE m_swapchainHandleNoRef;

    xref_ptr<WUComp::ISpriteVisual> m_spriteVisual;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Stores a swap chain in the composition tree.
//
//------------------------------------------------------------------------------
class HWCompSwapChainNode : public HWCompLeafNode
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *coreServices,
        _In_ CSwapChainPanel* swapChainPanel,
        _In_ CompositorTreeHost *pCompositorTreeHost,
        _Outptr_ HWCompSwapChainNode **ppCompositorLeafNode);

    _Check_return_ HRESULT SetSwapChain(
        _In_ DCompTreeHost *pVisualTreeHost,
        _In_ CSwapChainElement *pSwapChainElement,
        float width,
        float height,
        bool stretchToFit);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<HWCompSwapChainNode>::Index;
    }

    _Check_return_ HRESULT EnsureVisual(_In_opt_ DCompTreeHost* pDCompTreeHost) override;
    xref_ptr<WUComp::IVisual> GetWUCVisual() const override;

protected:
    HWCompSwapChainNode(
        _In_ CCoreServices *coreServices,
        _In_ CSwapChainPanel* swapChainPanel,
        _In_ CompositorTreeHost *pCompositorTreeHost);

    ~HWCompSwapChainNode() override;

private:

    // SwapChainPanels can create independent input sources, which require the visual that will contain the swap
    // chain. An independent input source can be created before a swap chain is actually hooked up, in which case
    // we'll create a visual and save it in the SwapChainPanel. When the swap chain is hooked up later, go back
    // to the SwapChainPanel to retrieve the visual to put the swap chain in.
    // The SwapChainPanel's SwapChainElement keeps this HWCompSwapChainNode alive, and a HWCompSwapChainNode
    // cannot be given to a different SwapChainElement, so a no-ref pointer works here.
    CSwapChainPanel* m_swapChainPanelNoRef { nullptr };

    xref_ptr<WUComp::ISpriteVisual> m_spriteVisual;
};

// TODO: JCOMP: This would be more efficient as a linked list instead of an array
typedef xvector<HWCompNode*> CompNodeCollection;

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Accumulate transforms and acts as a sink for independent animation
//
//------------------------------------------------------------------------------
class HWCompTreeNode : public HWCompNode
{
public:
    ~HWCompTreeNode() override;

    _Check_return_ HRESULT SetContentNode(
        _In_opt_ HWCompLeafNode *pContentNode,
        _In_ DCompTreeHost* dcompTreeHost,
        const bool isMultitargetLTEForSwapChainPanel);

    void ClearVisualContent() override;

    HWCompLeafNode* GetContentNode() const
    {
        return m_pContentNode;
    }

    bool UpdateTransformToRoot(
        _In_ const CTransformToRoot *pTransformToRoot
        );

    const CTransformToRoot& GetTransformToRoot() const { return m_transformToRoot; }

    // Tree walk to push updates from compositor tree to underlying DComp tree.
    _Check_return_ HRESULT UpdateTreeRoot(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        bool useDCompAnimations
        );

    _Check_return_ HRESULT UpdateTreeVirtual(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        bool useDCompAnimations,
        bool disablePixelSnapping) final;

    _Check_return_ HRESULT UpdateTreeChildren(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        bool useDCompAnimations,
        bool disablePixelSnapping);

    _Check_return_ HRESULT InsertChild(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        _In_ HWCompNode *pChild,
        _In_opt_ HWCompNode *pReferenceNode,
        _In_opt_ CUIElement *pElement = nullptr
        );

    _Check_return_ HRESULT InsertChildAtBeginning(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        _In_ HWCompNode *pChild
        );

    _Check_return_ HRESULT RemoveChild(
        _In_ HWCompNode *pChild
        );

    CompNodeCollection::iterator GetChildrenBegin() { return m_children.begin(); }
    CompNodeCollection::iterator GetChildrenEnd() { return m_children.end(); }
    // TODO: RTB const?

    virtual bool HasHandOffVisual() const = 0;
    virtual bool HasHandInVisual() const = 0;

    virtual WUComp::IVisual* GetHandOffVisual() = 0;

    // Discards the WinRT hand-in IVisual previously set by SetHandInVisual.
    virtual void DiscardHandInVisual() = 0;

    void GetPreSubgraphNode(
        _In_ HWCompTreeNode *pChild,
        _Outptr_ HWCompRenderDataNode **ppPreSubgraphNode
        );

    _Check_return_ HRESULT Remove() override;

    _Check_return_ HRESULT RemoveForReparenting();

    _Check_return_ HRESULT SetPrependProperties(
        _In_ const CMILMatrix *pPrependTransform,
        _In_ const XRECTF *pPrependClip,
        XFLOAT prependOpacity
        );

    virtual _Check_return_ HRESULT SetElementData(
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ CUIElement *pUIElement,
        bool isHitTestVisibleSubtree
        ) = 0;

    CUIElement* GetUIElementPeer() { return m_pUIElementNoRef; }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<HWCompTreeNode>::Index;
    }

    void ResetManipulationData();

    bool HasSharedManipulationTransform(bool targetsClip);
    void PrepareForSecondaryCurveUpdate(bool targetsClip);

    IUnknown* GetSharedPrimaryContentTransform() const;
    float GetDirectManipulationContentOffsetX() const;
    float GetDirectManipulationContentOffsetY() const;

    virtual _Check_return_ HRESULT SetConnectedAnimationRunning(_In_ bool isRunning, _Outptr_opt_ WUComp::IVisual ** visual = nullptr) = 0;

    // Set this flag to false when creating multiple temporary nodes for override walk. Multiple nodes cannot safely share the same visual.
    virtual void SetAllowReuseHandOffVisual(bool allowReuseHandOffVisual) = 0;

    virtual bool HasHitTestVisibleContentInSubtree() const;

protected:
    HWCompTreeNode(
        _In_ CCoreServices *coreServices,
        _In_ CompositorTreeHost *pCompositorTreeHost,
        bool isPlaceholderCompNode);

    virtual _Check_return_ HRESULT PushProperties(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        bool useDCompAnimations,
        bool disablePixelSnapping
        ) noexcept = 0 ;

    virtual HWCompTreeNode* GetRedirectionWalkParent();

    void NWPropagateDirtyFlag(DirtyFlags flags) override;

    virtual _Check_return_ HRESULT EnsureDManipData() = 0;
    _Check_return_ HRESULT UpdateDManipData(_In_ CUIElement* pUIElement);

    _Check_return_ HRESULT RemoveCompNode(_In_ CDependencyObject *pParent);

    virtual _Check_return_ HRESULT InsertChildInternal(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        _In_ HWCompNode *pChild,
        _In_opt_ HWCompNode *pReferenceNode,
        _In_opt_ CUIElement *pElement
        ) = 0;

    virtual _Check_return_ HRESULT InsertChildAtBeginningInternal(
        _In_opt_ DCompTreeHost *pDCompTreeHost,
        _In_ HWCompNode *pChild
        ) = 0;

    virtual _Check_return_ HRESULT RemoveChildInternal(
        _In_ HWCompNode *pChild
        ) = 0;

    void GetLastChildAsRenderData(
        _Outptr_ HWCompRenderDataNode **ppLastChildAsRenderData
        );

    virtual void UpdateDManipHitTestVisual(_In_ DCompTreeHost* dcompTreeHost) {}

protected:
    //
    // The prepend data is the cumulative property values from this HWCompTree node up to the
    // parent HWCompTreeNode. They exclude the properties mapped to the UIElementClone from its
    // corresponding UIElement, since they need to be stored separately in case they're
    // independently-animated.
    //
    CMILMatrix m_prependTransform;
    XRECTF m_prependClip;

    // The element that owns this comp node. This back pointer is used to read properties like transforms,
    // opacities, and clips.
    CUIElement* m_pUIElementNoRef;

    // Placeholder nodes are used when the redirection target is in part of the tree with no rendering data.
    // The placeholders are inserted so that property updates can occur and can be collected during redirection,
    // but no render data is ever added to these branches of the tree.
    bool m_isPlaceholderCompNode : 1;

    // Set to true if this node's visual subtree should be hit-tested by DComp
    bool m_isHitTestVisible : 1;

    // Gathering redirection transforms can trigger multiple PushProperties calls to the same comp node. It's important
    // that duplicate calls have no effect, otherwise the transform that was handed out to some previous redirection node
    // could be released and replaced. This dirty flag marks when PushProperties can be skipped for a comp node. A comp
    // node is marked dirty in SetElementData, which is used by the UI thread render walk to update comp nodes for elements
    // that need rendering.
    bool m_isDCompVisualsDirty : 1;

    // Set to true if this node's visual has had the viewport interaction assigned
    bool m_isViewportInteractionAssigned : 1;

    // Cache of disablePixelSnapping value pushed down through the PushProperties walk
    bool m_disablePixelSnapping : 1;

    // Tree children. Draw order is front to back in the m_children array, so the last
    // elements in the vector appear on top.
    CompNodeCollection m_children;

    // The first child of this node. It represents the content in the subgraph owned by this node (up
    // to the point that a descendent comp node might split the tree into additional layers).
    HWCompLeafNode *m_pContentNode;

    // Storage for the UI thread render walk's transform-to-root. It saves memory to store it here
    // rather than on CUIElement, since relatively few UIElements actually have composition peers.
    CTransformToRoot m_transformToRoot;

    XFLOAT m_prependOpacity;

    // Optional DManip data, non-NULL for manipulatable CompNodes, otherwise NULL
    std::unique_ptr<DManipDataBase> m_spDManipData;
};

#endif // HWCOMPNODE_H
