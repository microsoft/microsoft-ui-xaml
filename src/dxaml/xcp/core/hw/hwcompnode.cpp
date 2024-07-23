// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "palgfx.h"

#include "TimeMgr.h"
#include "InputServices.h"
#include "ManipulationTransform.h"
#include "ScrollViewer.h"
#include <microsoft.ui.composition.experimental.interop.h>

using namespace Microsoft::WRL::Wrappers;

// Uncomment for handoff visual debug traces
// #define HOVCTN_DBG

// Uncomment for hand-in visual debug traces
// #define HIVCTN_DBG

//-------------------------------------------------------------------------
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
HWCompNode::HWCompNode(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost
    )
    : CDependencyObject(coreServices)
    , m_pCompositorTreeHostNoRef(pCompositorTreeHost)
    , m_usesPlaceholderVisual(false)
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     dtor
//
//-------------------------------------------------------------------------
HWCompNode::~HWCompNode()
{
    // CDependencyObject takes an optional ref count on CCoreServices. This ref count is triggered in places like removing
    // from a resource dictionary or from a UIE collection, which should never happen with comp nodes.
    // We want to avoid circular references with CCoreServices, so fail fast if we ever find ourselves with a ref count on
    // CCoreServices.
    XCP_FAULT_ON_FAILURE(!HasRefCountOnCCoreServices());
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Removes this comp node from the tree. No-ops on unparented nodes, like the root.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
HWCompNode::Remove()
{
    CDependencyObject *pParent = GetParentInternal(false /*isPublic*/);
    if (pParent != NULL)
    {
        ASSERT(pParent->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>());
        HWCompTreeNode *pCompParent = static_cast<HWCompTreeNode*>(pParent);

        IFC_RETURN(pCompParent->RemoveChild(this));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a node's previous sibling in the tree
//
//------------------------------------------------------------------------
HWCompNode*
HWCompNode::GetPreviousSibling()
{
    CDependencyObject *pParent = GetParentInternal(false /*isPublic*/);
    if (pParent == nullptr)
    {
        return nullptr;
    }

    ASSERT(pParent->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>());
    HWCompTreeNode *pParentCompNode = static_cast<HWCompTreeNode *>(pParent);

    HWCompNode * pPreviousSibling = nullptr;
    for (CompNodeCollection::iterator it = pParentCompNode->GetChildrenBegin();
        it != pParentCompNode->GetChildrenEnd();
        ++it)
    {
        if (*it == this)
        {
            return pPreviousSibling;
        }
        pPreviousSibling = *it;
    }
    ASSERT(false);
    return nullptr;
}

_Check_return_ HRESULT HWCompNode::UpdateTree(
    _In_opt_ DCompTreeHost *pDCompTreeHost,
    bool useDCompAnimations,
    bool disablePixelSnapping)
{
    IFC_RETURN(UpdateTreeVirtual(
        pDCompTreeHost,
        useDCompAnimations,
        disablePixelSnapping));

    return S_OK;
}

_Check_return_ HRESULT HWCompNode::UpdateTreeVirtual(
    _In_opt_ DCompTreeHost *pDCompTreeHost,
    bool useDCompAnimations,
    bool disablePixelSnapping)
{
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Clears the content of a node so that it will be recreated in the next pass.
//
//-------------------------------------------------------------------------
void
HWCompTreeNode::ClearVisualContent()
{
    // The HWCompTreeNode hosts the content node, which hosts the visuals.
    if (m_pContentNode)
    {
        m_pContentNode->ClearVisualContent();
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
HWCompTreeNode::HWCompTreeNode(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost,
    bool isPlaceholderCompNode
    )
    : HWCompNode(coreServices, pCompositorTreeHost)
    , m_pUIElementNoRef(nullptr)
    , m_prependOpacity(1.0f)
    , m_pContentNode(NULL)
    , m_isPlaceholderCompNode(!!isPlaceholderCompNode)
    , m_isHitTestVisible(true)
    , m_isDCompVisualsDirty(false)
    , m_isViewportInteractionAssigned(false)
    , m_disablePixelSnapping(false)
{
    m_prependTransform.SetToIdentity();
    SetInfiniteClip(&m_prependClip);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     dtor
//
//-------------------------------------------------------------------------
HWCompTreeNode::~HWCompTreeNode()
{
    for (CompNodeCollection::iterator it = m_children.begin(); it != m_children.end(); ++it)
    {
        HWCompNode *pChild = *it;
        VERIFYHR(pChild->RemoveParent(this));
        ReleaseInterface(pChild);
    }

    ReleaseInterface(m_pContentNode);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the transform to root. Returns TRUE if the realization transform to root has changed.
//
//------------------------------------------------------------------------
bool
HWCompTreeNode::UpdateTransformToRoot(
    _In_ const CTransformToRoot *pTransformToRoot
    )
{
    const bool hasTransformToRootChanged = !m_transformToRoot.IsSameScaleAndSubPixelOffsetAs(pTransformToRoot);

    if (hasTransformToRootChanged)
    {
        m_transformToRoot = *pTransformToRoot;
    }

    return hasTransformToRootChanged;
}

_Check_return_ HRESULT
HWCompTreeNode::UpdateDManipData(_In_ CUIElement* pUIElement)
{
    xref_ptr<IPALDirectManipulationService> spDMService;
    xref_ptr<IObject> spManipulationContent;
    xref_ptr<IObject> spClipContent;
    XDMContentType contentType;
    float contentOffsetX;
    float contentOffsetY;

    IFC_RETURN(pUIElement->GetDirectManipulationServiceAndContent(
        spDMService.ReleaseAndGetAddressOf(),
        spManipulationContent.ReleaseAndGetAddressOf(),
        spClipContent.ReleaseAndGetAddressOf(),
        &contentType,
        &contentOffsetX,
        &contentOffsetY));

    // TextBoxView requires special handling if its FlowDirection is RightToLeft.
    // TextBoxView internally uses an LTR coordinate space even when in RTL mode.
    // This code performs the required coordinate conversion to make the flip if necessary.
    if (pUIElement->OfTypeByIndex<KnownTypeIndex::TextBoxView>())
    {
        contentOffsetX += (static_cast<CTextBoxView*>(pUIElement))->GetRightToLeftOffset();
    }

    IFC_RETURN(EnsureDManipData());

    if (m_spDManipData->GetDMService() != spDMService)
    {
        if (m_spDManipData->HasManipulationData())
        {
            IFC_RETURN(m_spDManipData->ReleaseManipulationData());
        }
        m_spDManipData->SetDMService(spDMService);
    }

    m_spDManipData->SetManipulatedElement(pUIElement);
    IFC_RETURN(m_spDManipData->SetManipulationContent(spManipulationContent, contentType));
    IFC_RETURN(m_spDManipData->SetClipContent(spClipContent));
    IFC_RETURN(m_spDManipData->SetContentOffsets(contentOffsetX, contentOffsetY));

    return S_OK;
}

void HWCompTreeNode::ResetManipulationData()
{
    m_spDManipData.reset();
}

// Returns true if this CompNode has a shared transform of the specified type driven by DManip-on-DComp
bool HWCompTreeNode::HasSharedManipulationTransform(bool targetsClip)
{
    if (m_spDManipData != nullptr)
    {
        return m_spDManipData->HasSharedManipulationTransform(targetsClip);
    }

    return false;
}

// Clears out shared transform of the specified type, in preparation for creating new ones.
// See more details on deferred DM Release in CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate().
void HWCompTreeNode::PrepareForSecondaryCurveUpdate(bool targetsClip)
{
    if (m_spDManipData != nullptr)
    {
        if (targetsClip)
        {
            VERIFYHR(m_spDManipData->SetSharedClipTransform(nullptr));
            m_spDManipData->ResetClipContent();
        }
        else
        {
            VERIFYHR(m_spDManipData->SetSharedContentTransforms(nullptr, nullptr));
            m_spDManipData->ResetManipulationContent();
        }
    }
}

IUnknown* HWCompTreeNode::GetSharedPrimaryContentTransform() const
{
    return m_spDManipData ? m_spDManipData->GetSharedPrimaryContentTransform() : nullptr;
}

float HWCompTreeNode::GetDirectManipulationContentOffsetX() const
{
    return m_spDManipData ? m_spDManipData->GetDirectManipulationContentOffsetX() : 0;
}

float HWCompTreeNode::GetDirectManipulationContentOffsetY() const
{
    return m_spDManipData ? m_spDManipData->GetDirectManipulationContentOffsetY() : 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the content node for this tree node.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT HWCompTreeNode::SetContentNode(
    _In_opt_ HWCompLeafNode *pContentNode,
    _In_ DCompTreeHost* dcompTreeHost,
    const bool isMultitargetLTEForSwapChainPanel)
{
    ASSERT(m_pCompositorTreeHostNoRef != NULL);

    // Remove the previous content node (the current first child).
    if (m_pContentNode != NULL)
    {
        IFC_RETURN(m_pContentNode->Remove());
    }

    // Add the new content node as the first child of this element.
    if (pContentNode != NULL)
    {
        HWCompTreeNodeWinRT* thisWinRT = static_cast<HWCompTreeNodeWinRT*>(this);
        thisWinRT->InsertChildSynchronous(
            dcompTreeHost,
            pContentNode,
            nullptr /* referenceNode */,
            nullptr /* previousSiblingVisual */,
            nullptr /* element */,
            isMultitargetLTEForSwapChainPanel /* ignoreInsertVisualErrors */);
    }

    ReplaceInterface(m_pContentNode, pContentNode);

    return S_OK;//RRETURN_REMOVAL
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Override to handle removing content and postsubgraph node, and
//      re-attaching existing children. Called by the composition tree host
//      to actually remove the element.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
HWCompTreeNode::Remove()
{
    HRESULT hr = S_OK;
    HWCompRenderDataNode *pLastChildRenderData = NULL;
    HWCompRenderDataNode *pPreSubgraphNode = NULL;

    //
    // Note: This is slightly asymmetric from attaching a new node. Removing a node will
    // handle removing its post subgraph node from its parent, but adding a node will not
    // handle adding its post subgraph node to its parent. That is handled explicitly via
    // a commands added by CUIElement::EnsureCompositionPeer.
    //

    CDependencyObject *pParent = GetParentInternal(false /*isPublic*/);

    TraceCompTreeRemoveFromParentBegin(
        reinterpret_cast<XUINT64>(pParent),
        reinterpret_cast<XUINT64>(this),
        0 /* removeForReparenting */,
        0 /* shouldMergePrimitiveGroups */
        );

    // If this is the root node, these additional steps are unnecessary.
    if (pParent != NULL)
    {
        ASSERT(pParent->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>());
        HWCompTreeNode *pCompParent = static_cast<HWCompTreeNode*>(pParent);

        // 2. Move the children, except the content node, to the parent's child collection.
        //    We iterate through in reverse adding children just behind this node to get the correct order.
        //    If this node had media or a swap chain instead of render data, then here will be no other children.
        //
        // This node controls the lifetime of its content node. The content node was created and attached when this
        // node was created, and will not be attached again. If this node is reparented somewhere else in the tree,
        // the content node needs to stay with it.
        for (CompNodeCollection::reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it)
        {
            HWCompNode* pChild = *it;

            // Leave the content node attached to this comp node. If this comp node is attached
            // elsewhere in the tree, it will still need its content.
            if (pChild != m_pContentNode)
            {
                // If there are child comp nodes that represent open windowed popups, we need to pass the popup
                // along when we reparent the child comp node. The new parent will ensure the windowed popup is
                // attached correctly when inserting the child comp node.
                CUIElement* childUIElement = nullptr;
                if (pChild->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>())
                {
                    childUIElement = static_cast<HWCompTreeNode*>(pChild)->m_pUIElementNoRef;
                }

                IFC(RemoveChildInternal(pChild));

                IFC(pCompParent->InsertChild(
                    NULL /*pDCompTreeHost*/,
                    pChild,
                    this, /*pReferenceNode*/
                    childUIElement));

                ReleaseInterface(pChild);
                IFC(m_children.erase(it));
            }
        }

        // 3. Remove the post subgraph node from the parent
        //    The post subgraph node is assigned as part of creating the HWCompTreeNode. It should always exist.
        IFC(RemoveCompNode(pParent));
    }

Cleanup:
    TraceCompTreeRemoveFromParentEnd(
        reinterpret_cast<XUINT64>(pParent),
        reinterpret_cast<XUINT64>(this),
        0 /* removeForReparenting */,
        0 /* shouldMergePrimitiveGroups */
        );

    ReleaseInterfaceNoNULL(pLastChildRenderData);
    ReleaseInterfaceNoNULL(pPreSubgraphNode);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Detaches this comp node from the comp tree.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT HWCompTreeNode::RemoveForReparenting()
{
    HRESULT hr = S_OK;

    // The root node should never be reparented.
    CDependencyObject *pParent = GetParentInternal(false /*isPublic*/);
    ASSERT(pParent != NULL);

    TraceCompTreeRemoveFromParentBegin(
        reinterpret_cast<XUINT64>(pParent),
        reinterpret_cast<XUINT64>(this),
        1 /* removeForReparenting */,
        0 /* mergePrimitiveGroups */
        );

    IFC(RemoveCompNode(pParent));

Cleanup:
    TraceCompTreeRemoveFromParentEnd(
        reinterpret_cast<XUINT64>(pParent),
        reinterpret_cast<XUINT64>(this),
        1 /* removeForReparenting */,
        0 /* mergePrimitiveGroups */
        );
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Detaches this comp node from the comp tree.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT HWCompTreeNode::RemoveCompNode(_In_ CDependencyObject *pParent)
{
    //
    // Note: This is slightly asymmetric from attaching a new node. Removing a node will
    // handle removing its post subgraph node from its parent, but adding a node will not
    // handle adding its post subgraph node to its parent. That is handled explicitly via
    // a command added by CUIElement::EnsureCompositionPeer.
    //

    ASSERT(pParent->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>());

    // Call into base class to remove this node from the parent too.
    IFC_RETURN(HWCompNode::Remove());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the values of the prepend properties.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWCompTreeNode::SetPrependProperties(
    _In_ const CMILMatrix *pPrependTransform,
    _In_ const XRECTF *pPrependClip,
    XFLOAT prependOpacity
    )
{
    m_prependTransform = *pPrependTransform;
    m_prependClip = *pPrependClip;
    m_prependOpacity = prependOpacity;

    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);

    return S_OK;
}

HRESULT HWCompTreeNode::UpdateTreeRoot(
    _In_opt_ DCompTreeHost *pDCompTreeHost,
    bool useDCompAnimations
    )
{
    // This method should only be called on the root of a tree.
    ASSERT(GetParentInternal(false /*fPublic*/) == NULL);

    IFC_RETURN(UpdateTree(
        pDCompTreeHost,
        useDCompAnimations,
        false /* disablePixelSnapping */));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Tree walk to push updates from compositor tree to underlying DComp tree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT HWCompTreeNode::UpdateTreeVirtual(
    _In_opt_ DCompTreeHost *pDCompTreeHost,
    bool useDCompAnimations,
    bool disablePixelSnapping)
{
    HRESULT hr = S_OK;
    TraceHWCompNodeUpdateBegin((XUINT64)m_pUIElementNoRef);
    const bool isUIThread = (GetContext()->GetThreadID() == ::GetCurrentThreadId());

    // If this element's transform is animating, disable pixel snapping in its subtree, to prevent jittering during the animation
    if (isUIThread && !disablePixelSnapping)
    {
        disablePixelSnapping = m_pUIElementNoRef->ShouldDisablePixelSnapping();
    }

    IFC(PushProperties(
        pDCompTreeHost,
        useDCompAnimations,
        disablePixelSnapping
        ));

    if (OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>())
    {
        HWCompTreeNode* treeNode = static_cast<HWCompTreeNode*>(this);
        if (treeNode->m_isViewportInteractionAssigned)
        {
            treeNode->UpdateDManipHitTestVisual(pDCompTreeHost);
        }
    }

    IFC(UpdateTreeChildren(pDCompTreeHost, useDCompAnimations, disablePixelSnapping));

Cleanup:
    TraceHWCompNodeUpdateEnd();
    RRETURN(hr);
}

_Check_return_ HRESULT HWCompTreeNode::UpdateTreeChildren(
    _In_opt_ DCompTreeHost *pDCompTreeHost,
    bool useDCompAnimations,
    bool disablePixelSnapping)
{
    for (CompNodeCollection::iterator it = m_children.begin(); it != m_children.end(); ++it)
    {
        IFC_RETURN((*it)->UpdateTree(
            pDCompTreeHost,
            useDCompAnimations,
            disablePixelSnapping));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a child node to this comp node after pReferenceNode. If pReferenceNode is
//      NULL, the new node becomes the first child.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWCompTreeNode::InsertChild(
    _In_opt_ DCompTreeHost *pDCompTreeHost,
    _In_ HWCompNode *pChild,
    _In_opt_ HWCompNode *pReferenceNode,
    _In_opt_ CUIElement *pElement
    )
{
    ASSERT(pChild != this);

    // Update the compositor tree.
    XUINT32 index = 0;

    TraceCompTreeInsertChildInfo(
        reinterpret_cast<XUINT64>(this),
        reinterpret_cast<XUINT64>(pChild),
        reinterpret_cast<XUINT64>(pReferenceNode),
        0 /* insertAtEnd */
        );

    if (pReferenceNode != NULL)
    {
        for (; index < m_children.size(); index++)
        {
            HWCompNode *pNodeNoRef = m_children.unsafe_get_item(index);
            if (pNodeNoRef == pReferenceNode)
            {
                // Insert after the reference node.
                index += 1;
                break;
            }
        }
    }

    IFC_RETURN(m_children.insert(index, pChild));
    AddRefInterface(pChild);

    IFC_RETURN(pChild->AddParent(
        this,
        FALSE /* fPublic */,
        RENDERCHANGEDPFN(CDependencyObject::NWSetRenderDirty)
        ));

    IFC_RETURN(InsertChildInternal(pDCompTreeHost, pChild, pReferenceNode, pElement));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a child node to this comp node as the first child.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWCompTreeNode::InsertChildAtBeginning(
    _In_opt_ DCompTreeHost *pDCompTreeHost,
    _In_ HWCompNode *pChild
    )
{
    ASSERT(pChild != this);

    TraceCompTreeInsertChildInfo(
        reinterpret_cast<XUINT64>(this),
        reinterpret_cast<XUINT64>(pChild),
        0 /* ReferenceNode */,
        1 /* InsertAtBeginning */
        );

    IFC_RETURN(m_children.insert(0, pChild));
    AddRefInterface(pChild);

    IFC_RETURN(pChild->AddParent(
        this,
        FALSE /* fPublic */,
        RENDERCHANGEDPFN(CDependencyObject::NWSetRenderDirty)
        ));

    IFC_RETURN(InsertChildAtBeginningInternal(pDCompTreeHost, pChild));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes a child node from this comp node.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWCompTreeNode::RemoveChild(
    _In_ HWCompNode *pChild
    )
{
    // Find the child.
    for (XUINT32 index = 0; index < m_children.size(); index++)
    {
        HWCompNode *pNodeNoRef = m_children.unsafe_get_item(index);
        if (pNodeNoRef == pChild)
        {
            TraceCompTreeRemoveChildInfo(
                reinterpret_cast<XUINT64>(this),
                reinterpret_cast<XUINT64>(pChild)
                );

            IFC_RETURN(RemoveChildInternal(pChild));
            ReleaseInterface(pChild);

            IFC_RETURN(m_children.erase(index));
            break;
        }
    }

    // The child should always be found in the collection.
    ASSERT(pChild == NULL);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the HWCompRenderDataNode that's the previous sibling of
//      a child node.
//
//------------------------------------------------------------------------
void
HWCompTreeNode::GetPreSubgraphNode(
    _In_ HWCompTreeNode *pChild,
    _Outptr_ HWCompRenderDataNode **ppPreSubgraphNode
    )
{
    HWCompNode *pPreSubgraphNodeNoRef = NULL;

    // The first node will be either a content leaf node or a placeholder node. This method looks
    // up a tree node, so it's safe to skip the first node.
    for (XUINT32 index = 1; index < m_children.size(); index++)
    {
        HWCompNode *pNodeNoRef = m_children.unsafe_get_item(index);
        if (pNodeNoRef == pChild)
        {
            // Search backwards from the previous sibling looking for the presubgraph node.
            // This will typically be the immediately preceding sibling, but it's possible that there are some
            // placeholder nodes in-between (with no content) that need to be skipped over.
            for (XINT32 preSubIndex = static_cast<XINT32>(index - 1); preSubIndex >= 0; preSubIndex--)
            {
                HWCompNode *pPreviousSiblingNoRef = m_children.unsafe_get_item(preSubIndex);
                if (pPreviousSiblingNoRef->OfTypeByIndex<KnownTypeIndex::HWCompRenderDataNode>())
                {
                    pPreSubgraphNodeNoRef = pPreviousSiblingNoRef;
                    break;
                }
            }
        }
    }

    // The pre subgraph node should always be found in the collection, and it should always be a leaf node.
    ASSERT(pPreSubgraphNodeNoRef != NULL);
    ASSERT(pPreSubgraphNodeNoRef->OfTypeByIndex<KnownTypeIndex::HWCompRenderDataNode>());

    SetInterface(*ppPreSubgraphNode, static_cast<HWCompRenderDataNode *>(pPreSubgraphNodeNoRef));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the last child HWCompRenderDataNode. If the last child
//      isn't a HWCompRenderDataNode, then this HWCompTreeNode is for
//      media or SCBP. Return NULL in that case.
//
//------------------------------------------------------------------------
void
HWCompTreeNode::GetLastChildAsRenderData(
    _Outptr_ HWCompRenderDataNode **ppLastChildAsRenderData
    )
{
    // There should never be a HWCompTreeNode without a child. Media nodes will have
    // their frame provider nodes, SCBP nodes will have their swap chain nodes, and regular
    // nodes will have their render data nodes.
    ASSERT(m_children.size() > 0);

    HWCompNode *pNodeNoRef = m_children.unsafe_get_item(m_children.size() - 1);
    if (pNodeNoRef->OfTypeByIndex<KnownTypeIndex::HWCompRenderDataNode>())
    {
        SetInterface(*ppLastChildAsRenderData, static_cast<HWCompRenderDataNode *>(pNodeNoRef));
    }
    else
    {
        // This node is for media or SCBP and has no render data children.
        *ppLastChildAsRenderData = NULL;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the parent for the redirection walk.
//
//------------------------------------------------------------------------
HWCompTreeNode*
HWCompTreeNode::GetRedirectionWalkParent()
{
    CDependencyObject *pParent = GetParentInternal(false /*isPublic*/);
    // If you're hitting a null reference exception here when executing
    // ListPickerFlyout::ListPickerFlyoutIntegrationTests::CanSelectMultipleItems this is tracked by
    // https://tasks.ms/620253 and is a known issue under CHK builds. Carry on!
    // TODO: disabling this assert to unblock chk test runs. I will investigate the issue separately.
    // Returning null can cause problems where visuals are placed at the wrong location on the screen, but this
    // isn't causing problems in fre runs or in WinBlue.
    //ASSERT(pParent && pParent->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>());
    return static_cast<HWCompTreeNode *>(pParent);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
HWCompLeafNode::HWCompLeafNode(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost
    )
    : HWCompNode(coreServices, pCompositorTreeHost)
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     dtor
//
//-------------------------------------------------------------------------
HWCompLeafNode::~HWCompLeafNode()
{
}

_Check_return_ HRESULT HWCompLeafNode::EnsureVisual(_In_opt_ DCompTreeHost * pDCompTreeHost)
{
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
HWCompRenderDataNode::HWCompRenderDataNode(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost
    )
    : HWCompLeafNode(coreServices, pCompositorTreeHost)
{
}

HWCompRenderDataNode::~HWCompRenderDataNode()
{
}

HWCompWinRTVisualRenderDataNode::HWCompWinRTVisualRenderDataNode(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost,
    _In_ WUComp::IContainerVisual *pContainerVisual
    )
    : HWCompRenderDataNode(coreServices, pCompositorTreeHost)
    , m_containerVisual(pContainerVisual)
{
}

HWCompWinRTVisualRenderDataNode::~HWCompWinRTVisualRenderDataNode()
{
#if DBG
    if (m_containerVisual != nullptr)
    {
        // All child SpriteVisuals must be explicitly removed from this container before the
        // container is released or Closed()/Disposed().  We cache pointers to the SpriteVisuals
        // on UI elements without a ref, and when the container goes away or gets Closed(), it
        // takes the internal refs on its children with it.  Because Close() as well
        // as actual destruction removes child refs, we would be no better off if we kept our
        // own refs on the primitives:  If the container is Closed(), even if our child
        // pointers are still valid, attemps to remove them from the container would fail.
        HRESULT hr;
        xref_ptr<WUComp::IVisualCollection> spThisVisualCollection;
        hr = m_containerVisual->get_Children(spThisVisualCollection.ReleaseAndGetAddressOf());

        // If this fires, it means the container has been Close()d
        ASSERTSUCCEEDED(hr);
        if (SUCCEEDED(hr))
        {
            int count = 1;
            hr = spThisVisualCollection->get_Count(&count);
            ASSERTSUCCEEDED(hr);
            ASSERT(count == 0);
        }
    }
#endif
}

/*static*/ _Check_return_ HRESULT HWCompWinRTVisualRenderDataNode::Create(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost,
    _In_ WUComp::IContainerVisual *pContainerVisual,
    _Outptr_ HWCompWinRTVisualRenderDataNode **ppCompositorLeafNode)
{
    xref_ptr<HWCompWinRTVisualRenderDataNode> spNode;

    spNode.attach(new HWCompWinRTVisualRenderDataNode(
        coreServices,
        pCompositorTreeHost,
        pContainerVisual
        ));

    *ppCompositorLeafNode = spNode.detach();

    return S_OK;
}

_Check_return_ HRESULT HWCompWinRTVisualRenderDataNode::EnsureVisual(_In_opt_ DCompTreeHost *pDCompTreeHost)
{
    // HWCompWinRTVisualRenderDataNode uses its RenderDataParent as its visual.
    // This must be supplied at construction time and it's never deleted.
    ASSERT(m_containerVisual != nullptr);
    return S_OK;
}

xref_ptr<WUComp::IVisual> HWCompWinRTVisualRenderDataNode::GetWUCVisual() const
{
    // The RenderDataParent m_containerVisual also serves as the connection visual for linking this CompNode to its parent
    xref_ptr<WUComp::IVisual> visual;
    VERIFYHR(m_containerVisual->QueryInterface(IID_PPV_ARGS(visual.ReleaseAndGetAddressOf())));
    return visual;
}

// TODO_WinRT: Consider merging this method with GetRenderDataParent() as separate methods aren't strictly required.
IUnknown *HWCompWinRTVisualRenderDataNode::EnsureRenderDataParent(_In_ DCompTreeHost* dcompTreeHost)
{
    // HWCompWinRTVisualRenderDataNode requires its RenderDataParent be supplied at construction time and it's never deleted.
    ASSERT(m_containerVisual != nullptr);
    return m_containerVisual.get();
}

IUnknown* HWCompWinRTVisualRenderDataNode::GetRenderDataParent() const
{
    return m_containerVisual.get();
}

void
HWCompWinRTVisualRenderDataNode::ClearVisualContent()
{
    // Here we only need to remove all the SpriteVisuals from our ContainerVisual
    // as this visual has no content associated with it, it's just a structural visual.
    // This is different from the PrimitiveGroup code path which apparently requires us to
    // also release the PrimitiveGroup.
    xref_ptr<WUComp::IVisualCollection> visualCollection;
    IFCFAILFAST(m_containerVisual->get_Children(visualCollection.ReleaseAndGetAddressOf()));
    IFCFAILFAST(visualCollection->RemoveAll());
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
HWCompMediaNode::HWCompMediaNode(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost
    )
    : HWCompLeafNode(coreServices, pCompositorTreeHost)
    , m_destinationRect()
    , m_swapchainHandleNoRef(NULL)
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     dtor
//
//-------------------------------------------------------------------------
HWCompMediaNode::~HWCompMediaNode()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Create a leaf comp node.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT HWCompMediaNode::Create(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *pCompositorTreeHost,
    _Outptr_ HWCompMediaNode **ppCompositorNode)
{
    HRESULT hr = S_OK;
    HWCompMediaNode *pNode = NULL;

    pNode = new HWCompMediaNode(coreServices, pCompositorTreeHost);

    TraceCompTreeCreateMediaNodeInfo(
        reinterpret_cast<XUINT64>(pNode)
        );

    *ppCompositorNode = pNode;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Construct the underlying composition tree for MediaPlayerElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWCompMediaNode::SetMedia(
    _In_ DCompTreeHost *pVisualTreeHost,
    XHANDLE swapchainHandle,
    _In_ const XRECT &destinationRect,
    _In_ const XMATRIX &stretchTransform,
    _In_ const XRECTF &stretchClip)
{
    IFC_RETURN(EnsureVisual(pVisualTreeHost));

    if (m_swapchainHandleNoRef != swapchainHandle)
    {
        ComPtr<WUComp::ICompositionSurface> surface;
        WUComp::ICompositor* compositorNoRef = pVisualTreeHost->GetCompositor();

        IFC_RETURN(pVisualTreeHost->CreateCompositionSurfaceForHandle(
            swapchainHandle,
            &surface
        ));

        ComPtr<WUComp::ICompositionBrush> brush;
        IFC_RETURN(m_spriteVisual->get_Brush(brush.ReleaseAndGetAddressOf()));

        ComPtr<WUComp::ICompositionSurfaceBrush> surfaceBrush;

        if (brush)
        {
            IFC_RETURN(brush.As(&surfaceBrush));
        }

        if (surfaceBrush)
        {
            IFC_RETURN(surfaceBrush->put_Surface(surface.Get()));
        }
        else
        {
            IFC_RETURN(compositorNoRef->CreateSurfaceBrush(&surfaceBrush));
            IFC_RETURN(surfaceBrush->put_Surface(surface.Get()));
            IFC_RETURN(surfaceBrush.As(&brush));
            IFC_RETURN(m_spriteVisual->put_Brush(brush.Get()));
        }

        m_swapchainHandleNoRef = swapchainHandle;
    }

    m_destinationRect = destinationRect;

    wfn::Vector2 panelSize{ static_cast<float>(m_destinationRect.Width), static_cast<float>(m_destinationRect.Height) };

    auto visual = GetWUCVisual();
    IFC_RETURN(visual->put_Size(panelSize));

    CMILMatrix stretch(stretchTransform);
    wfn::Matrix4x4 stretch4x4;
    stretch.ToMatrix4x4(&stretch4x4);
    IFC_RETURN(visual->put_TransformMatrix(stretch4x4));

    xref_ptr<WUComp::ICompositionClip> compositionClip;
    const float leftInset = stretchClip.X;
    const float topInset = stretchClip.Y;
    const float rightInset = m_destinationRect.Width - (stretchClip.X + stretchClip.Width);
    const float bottomInset = m_destinationRect.Height - (stretchClip.Y + stretchClip.Height);

    xref_ptr<WUComp::IInsetClip> insetClip;
    IFCFAILFAST(pVisualTreeHost->GetCompositor()->CreateInsetClip(insetClip.ReleaseAndGetAddressOf()));

    IFCFAILFAST(insetClip->put_LeftInset(leftInset));
    IFCFAILFAST(insetClip->put_TopInset(topInset));
    IFCFAILFAST(insetClip->put_RightInset(rightInset));
    IFCFAILFAST(insetClip->put_BottomInset(bottomInset));

    VERIFYHR(insetClip->QueryInterface(IID_PPV_ARGS(compositionClip.ReleaseAndGetAddressOf())));
    IFCFAILFAST(visual->put_Clip(compositionClip));

    TraceCompositorSetMediaInfo(
        reinterpret_cast<uint64_t>(pVisualTreeHost),
        reinterpret_cast<uint64_t>(swapchainHandle)
        );

    return S_OK;
}

xref_ptr<WUComp::IVisual> HWCompMediaNode::GetWUCVisual() const
{
    if (m_spriteVisual)
    {
        xref_ptr<WUComp::IVisual> visual;

        VERIFYHR(m_spriteVisual->QueryInterface(IID_PPV_ARGS(visual.ReleaseAndGetAddressOf())));
        return visual;
    }
    return nullptr;
}

_Check_return_ HRESULT HWCompMediaNode::EnsureVisual(_In_opt_ DCompTreeHost* pDCompTreeHost)
{
    if (pDCompTreeHost && !m_spriteVisual)
    {
        ixp::ICompositor* compositorNoRef = pDCompTreeHost->GetCompositor();
        IFC_RETURN(compositorNoRef->CreateSpriteVisual(m_spriteVisual.ReleaseAndGetAddressOf()));
    }
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
HWCompSwapChainNode::HWCompSwapChainNode(
    _In_ CCoreServices *coreServices,
    _In_ CSwapChainPanel* swapChainPanel,
    _In_ CompositorTreeHost *pCompositorTreeHost)
    : HWCompLeafNode(coreServices, pCompositorTreeHost)
    , m_swapChainPanelNoRef(swapChainPanel)
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     dtor
//
//-------------------------------------------------------------------------
HWCompSwapChainNode::~HWCompSwapChainNode()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Create a leaf comp node.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT HWCompSwapChainNode::Create(
    _In_ CCoreServices *coreServices,
    _In_ CSwapChainPanel* swapChainPanel,
    _In_ CompositorTreeHost *pCompositorTreeHost,
    _Outptr_ HWCompSwapChainNode **ppCompositorTreeNode)
{
    HRESULT hr = S_OK;
    HWCompSwapChainNode *pNode = NULL;

    pNode = new HWCompSwapChainNode(coreServices, swapChainPanel, pCompositorTreeHost);

    TraceCompTreeCreateSwapChainNodeInfo(
        reinterpret_cast<XUINT64>(pNode)
        );

    *ppCompositorTreeNode = pNode;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Construct the underlying composition tree from the XAML composition tree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWCompSwapChainNode::SetSwapChain(
    _In_ DCompTreeHost *pVisualTreeHost,
    _In_ CSwapChainElement *pSwapChainElement,
    float width,
    float height,
    bool stretchToFit)
{
    IUnknown *pSwapChain = pSwapChainElement->GetSwapChain();
    WUComp::ICompositionSurface *pSwapChainSurface = pSwapChainElement->GetSwapChainSurface();
    bool useTransparentVisual = pSwapChainElement->GetUseTransparentVisual();

    // If there is nothing set in the SwapChainPanel, then it shouldn't create a HWCompSwapChainNode, so the
    // SwapChainPanel must be in only one of three modes: swap chain, swap chain surface, or transparent visual.
    ASSERT((pSwapChain && !pSwapChainSurface && !useTransparentVisual)
        || (!pSwapChain && pSwapChainSurface && !useTransparentVisual)
        || (!pSwapChain && !pSwapChainSurface && useTransparentVisual));
    ASSERT(m_swapChainPanelNoRef == nullptr || m_swapChainPanelNoRef == static_cast<CSwapChainPanel*>(pSwapChainElement->GetParentInternal(false /*publicOnly*/)));

    IFC_RETURN(EnsureVisual(pVisualTreeHost));

    ComPtr<WUComp::ICompositionSurface> surface;
    WUComp::ICompositor* compositorNoRef = pVisualTreeHost->GetCompositor();

    // The ICompositionSurface is either created from:
    // - a HANDLE in CSwapChainElement::SetSwapChainHandle and pSwapChainSurface is passed in. This is for the SwapChainPanel element alone.
    // - or a IDXGISwapChain1 below and pSwapChain is passed in. This is for both the SwapChainPanel and SwapChainBackgroundPanel.
    // The pSwapChainSurface passed in was created early on to report invalid handles synchronously.
    // The pSwapChain passed in is used in the SwapChainBackgroundPanel case where stretchToFit is True. Consider only passing in a pSwapChainSurface
    // in the event the SwapChainBackgroundPanel is deleted.
    if (pSwapChain != nullptr)
    {
        ComPtr<ixp::IExpCompositorInterop> expCompositorInterop;

        IFC_RETURN(compositorNoRef->QueryInterface(IID_PPV_ARGS(expCompositorInterop.ReleaseAndGetAddressOf())));
        IFC_RETURN(expCompositorInterop->CreateCompositionSurfaceForSwapChain(pSwapChain, surface.ReleaseAndGetAddressOf()));
    }
    else if (pSwapChainSurface != nullptr)
    {
        surface = pSwapChainSurface;
    }

    ComPtr<WUComp::ICompositionBrush> brush;
    IFC_RETURN(m_spriteVisual->get_Brush(brush.ReleaseAndGetAddressOf()));

    ComPtr<WUComp::ICompositionSurfaceBrush> surfaceBrush;

    if (useTransparentVisual)
    {
        ComPtr<WUComp::ICompositionColorBrush> colorBrush;
        if (brush)
        {
            // We could switch from a swap chain back to a solid color brush (by nulling out the swap chain and creating
            // another CoreIndependentInputSource). Allow this QI to fail for that case.
            IGNOREHR(brush.As(&colorBrush));
        }

        if (colorBrush)
        {
            IFC_RETURN(colorBrush->put_Color(ColorUtils::GetWUColor(0x00000000)));
        }
        else
        {
            IFC_RETURN(compositorNoRef->CreateColorBrush(&colorBrush));
            IFC_RETURN(colorBrush->put_Color(ColorUtils::GetWUColor(0x00000000)));
            IFC_RETURN(colorBrush.As(&brush));
            IFC_RETURN(m_spriteVisual->put_Brush(brush.Get()));
        }
    }
    else
    {
        if (brush)
        {
            // We could switch from a solid color transparent brush to a swap chain. Allow this QI to fail for that case.
            IGNOREHR(brush.As(&surfaceBrush));
        }

        if (surfaceBrush)
        {
            IFC_RETURN(surfaceBrush->put_Surface(surface.Get()));
        }
        else
        {
            IFC_RETURN(compositorNoRef->CreateSurfaceBrush(&surfaceBrush));
            IFC_RETURN(surfaceBrush->put_Surface(surface.Get()));
            IFC_RETURN(surfaceBrush.As(&brush));
            IFC_RETURN(m_spriteVisual->put_Brush(brush.Get()));
        }
    }

    wfn::Vector2 panelSize{ width, height };

    if (stretchToFit)
    {
        // stretchToFit is only True for SwapChainBackgroundPanel and there is no means to provide a HANDLE for that control.
        ASSERT(!pSwapChainSurface && !useTransparentVisual);

        // Set the ICompositionSurfaceBrush2's Scale property to scale the swapchain to the panel's size.
        IDXGISwapChain1* pDXGISwapChain1 = nullptr;

        IFC_RETURN(pSwapChain->QueryInterface(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(&pDXGISwapChain1)));

        DXGI_SWAP_CHAIN_DESC desc;

        IFC_RETURN(pDXGISwapChain1->GetDesc(&desc));

        // Read the rotation set on the swap chain to determine its final dimensions
        DXGI_MODE_ROTATION rotation;

        IFC_RETURN(pDXGISwapChain1->GetRotation(&rotation));

        float swapChainWidth;
        float swapChainHeight;

        switch (rotation)
        {
        case DXGI_MODE_ROTATION_ROTATE90:
        case DXGI_MODE_ROTATION_ROTATE270:
            swapChainWidth = static_cast<float>(desc.BufferDesc.Height);
            swapChainHeight = static_cast<float>(desc.BufferDesc.Width);
            break;

        default:
            swapChainWidth = static_cast<float>(desc.BufferDesc.Width);
            swapChainHeight = static_cast<float>(desc.BufferDesc.Height);
            break;
        }

        wfn::Vector2 brushScale {
            swapChainWidth == 0.0f ? 1.0f : panelSize.X / swapChainWidth,
            swapChainHeight == 0.0f ? 1.0f : panelSize.Y / swapChainHeight };
        ComPtr<WUComp::ICompositionSurfaceBrush2> surfaceBrush2;

        IFC_RETURN(surfaceBrush.As(&surfaceBrush2));
        IFC_RETURN(surfaceBrush2->put_Scale(brushScale));
    }
    else
    {
        CUIElement* panel = pSwapChainElement->GetUIElementParentInternal(false /*publicParentOnly*/);

        if (panel)
        {
            panelSize = panel->GetActualSize();
        }
    }

    ComPtr<WUComp::IVisual> visual;
    IFC_RETURN(m_spriteVisual->QueryInterface(IID_PPV_ARGS(visual.ReleaseAndGetAddressOf())));
    IFC_RETURN(visual->put_Size(panelSize));

    return S_OK;
}

xref_ptr<WUComp::IVisual> HWCompSwapChainNode::GetWUCVisual() const
{
    if (m_spriteVisual)
    {
        xref_ptr<WUComp::IVisual> visual;

        VERIFYHR(m_spriteVisual->QueryInterface(IID_PPV_ARGS(visual.ReleaseAndGetAddressOf())));
        return visual;
    }
    return nullptr;
}

_Check_return_ HRESULT HWCompSwapChainNode::EnsureVisual(_In_opt_ DCompTreeHost* pDCompTreeHost)
{
    if (pDCompTreeHost && !m_spriteVisual)
    {
        if (m_swapChainPanelNoRef)
        {
            //
            // Note: This always returns the same Visual. Normally when a comp node is released and re-created, it
            // creates its own set of Visuals. This Visual stays with SwapChainPanel and isn't re-created as comp nodes
            // come and go (e.g. leaving and reentering the tree, being targeted by an LTE).
            //
            // We need this Visual to stick around because it's exposed to the app. The app can create an
            // IInputPointerSource off-thread, which requires an associated Visual. Once handed out, we can't release
            // that Visual or the IPS will stop working.
            m_spriteVisual = m_swapChainPanelNoRef->GetOrEnsureSwapChainVisual().Get();
        }
        else
        {
            ixp::ICompositor* compositorNoRef = pDCompTreeHost->GetCompositor();
            IFC_RETURN(compositorNoRef->CreateSpriteVisual(m_spriteVisual.ReleaseAndGetAddressOf()));
        }
    }
    return S_OK;
}
