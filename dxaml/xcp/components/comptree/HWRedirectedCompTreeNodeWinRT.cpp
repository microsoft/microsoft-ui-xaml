// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <HWRedirectedCompTreeNodeWinRT.h>
#include <LocalTransformBuilder.h>
#include <MUX-ETWEvents.h>
#include <UIElement.h>
#include <Popup.h>
#include <LayoutTransitionElement.h>
#include <DCompTreeHost.h>

// These constants need to be kept in sync with test code, see WinRTMockDComp.cpp
static const wchar_t* s_popupPlaceholderVisualTag = L"_XAML_DEBUG_TAG_PopupPlaceholderVisual";

using namespace Microsoft::WRL;

HWRedirectedCompTreeNodeWinRT::HWRedirectedCompTreeNodeWinRT(
    _In_ CCoreServices *core,
    _In_ CompositorTreeHost *compositorTreeHost,
    _In_ DCompTreeHost* dcompTreeHost)
    : HWCompTreeNodeWinRT(core, compositorTreeHost, dcompTreeHost, FALSE /* isPlaceholderCompNode */)
{
    m_transformFromRedirectionTargetToRedirectionCompNode.SetToIdentity();

    TraceCompTreeCreateTreeNodeInfo(
        reinterpret_cast<XUINT64>(this),
        static_cast<XUINT32>(FALSE),
        static_cast<XUINT32>(FALSE)
        );
}

void HWRedirectedCompTreeNodeWinRT::SetRedirectionTarget(
    _In_opt_ HWCompTreeNode* redirectionTarget,
    _In_ const CMILMatrix* prependTransformFromRedirectionTargetToAncestor
    )
{
    m_redirectionTarget = xref::get_weakref(redirectionTarget);
    m_transformFromRedirectionTargetToRedirectionCompNode = *prependTransformFromRedirectionTargetToAncestor;

    TraceCompTreeSetRedirectionTargetInfo(
        reinterpret_cast<XUINT64>(this),
        reinterpret_cast<XUINT64>(redirectionTarget)
        );

    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

void HWRedirectedCompTreeNodeWinRT::UpdatePrependTransform()
{
    // We don't expect any prepend transform to be present for Popups since they are parented to the PopupRoot.
    // We also don't expect any prepend transform to be present for LTEs because LTEs are always parented to a TransitionRoot,
    // and TransitionRoot will generate a CompNode and "absorb" any transforms that would have made it to the LTE.
    FAIL_FAST_ASSERT(m_prependTransform.IsIdentity());

    // We still need to call into the base class to apply the Translation facade which currently is applied to the PrependVisual.
    __super::UpdatePrependTransform();
}

void HWRedirectedCompTreeNodeWinRT::UpdatePrependClip(_In_ DCompTreeHost *dcompTreeHost)
{
    // We don't expect any prepend clip to be present for Popups since they are parented to the PopupRoot.
    // We also don't expect any prepend clip to be present for LTEs because LTEs are always parented to a TransitionRoot,
    // and TransitionRoot will generate a CompNode and "absorb" any clips that would have made it to the LTE.
    FAIL_FAST_ASSERT(IsInfiniteRectF(GetOverallLocalPrependClipRect()));

    // There's no need to call into the base class - everything it does depends directly on a non-empty prepend clip.
}

void HWRedirectedCompTreeNodeWinRT::UpdatePrimaryVisualTransformParent(_In_ DCompTreeHost *dcompTreeHost)
{
    ComPtr<WUComp::IVisual> transformParent;

    HWCompTreeNode* redirectionTarget = m_redirectionTarget.lock_noref();
    if (redirectionTarget != nullptr)
    {
        // It is possible in some rare instances for the redirection target of a comp node to actually be above it in
        // z-order. This can happen with nested Popups, when the inner Popup is opened before the outer one.
        // First ensure the redirection target has created its Primary Visual.
        IFCFAILFAST(redirectionTarget->EnsureVisual(dcompTreeHost));

        HWRedirectedCompTreeNodeWinRT* redirectionTargetWinRT = static_cast<HWRedirectedCompTreeNodeWinRT*>(redirectionTarget);
        VERIFYHR(redirectionTargetWinRT->m_primaryVisual.As(&transformParent));
    }
    // else if m_redirectionTarget is NULL, this is either an absolutely positioned LTE, which don't use any redirection transform at all,
    // or the target comp node has been released (see note on m_redirectionTarget).

    // We set the TransformParent on the Prepend visual, because the Translation property is applied to this visual.
    ComPtr<WUComp::IVisual2> prependVisual;
    VERIFYHR(m_prependVisual.As(&prependVisual));
    IFCFAILFAST(prependVisual->put_ParentForTransform(transformParent.Get()));
}

bool HWRedirectedCompTreeNodeWinRT::GetRedirectionTransformInfo(_In_ RedirectionTransformInfo* rto)
{
    HWCompTreeNode* redirectionTarget = m_redirectionTarget.lock_noref();
    if (redirectionTarget != nullptr)
    {
        rto->redirectionTransform = &m_transformFromRedirectionTargetToRedirectionCompNode;
        rto->visual = m_primaryVisual.Get();
        return true;
    }
    // else if m_redirectionTarget is NULL, this is either an absolutely positioned LTE, which don't use any redirection transform at all,
    // or the target comp node has been released (see note on m_redirectionTarget).

    return false;
}

void HWRedirectedCompTreeNodeWinRT::SetUsesPlaceholderVisual(bool value)
{
    m_usesPlaceholderVisual = value;
}

// Ensure that the placeholder visual has been created and return it.
// Since Windowed Popups create their own islands and DComp target, we do something special
// create the illusion of there being only one DComp visual tree.
// A picture of the visual tree(s) is helpful here:
//
// MainWindow       PopupWindow
// |                |
// DComp Target     Popup Target
// |                |
// Root Visual      Root Visual
// |                |
// Foo              Popup Prepend Visual
// |                |
// Bar              Popup Primary Visual
// |
// etc
// |
// Popup Placeholder Visual
//
// The Popup creates its own Window, target, root visual, and underneath this visual, we put the "normal" visuals for the CompNode
// (eg the Prepend Visual, Primary Visual, etc).  This is how the Popup content is displayed under another window.
// However we also create a Placeholder visual which goes into the "mainline" visual tree, whose purpose
// is only to make the CompNode tree updating code happy, it doesn't actually carry any properties or content.
wrl::ComPtr<ixp::IVisual> HWRedirectedCompTreeNodeWinRT::EnsurePlaceholderVisual(_In_ DCompTreeHost* dcompTreeHost)
{
    if (!m_placeholderVisual)
    {
        CreateWUCSpineVisual(dcompTreeHost, &m_placeholderVisual, s_popupPlaceholderVisualTag);
    }
    return m_placeholderVisual;
}

xref_ptr<WUComp::IVisual> HWRedirectedCompTreeNodeWinRT::GetWUCVisualInMainTree() const
{
    if (m_usesPlaceholderVisual)
    {
        xref_ptr<WUComp::IVisual> placeholder(GetPlaceholderVisual());
        return placeholder;
    }
    else
    {
        return GetWUCVisual();
    }
}

//
// This method returns a visual used in incremental rendering. New visuals will be inserted after this reference visual.
// For a popup using placeholder visuals, most of the content lives in a separate visual tree in a separate island, and we
// insert a placeholder visual into the main visual tree. That placeholder visual should be used when incrementally
// rendering the main visual tree.
//
// For example, in a tree with two windowed popups, the main DComp visual tree will look like:
//
//      <Visual>
//          <Visual>
//               <!-- non-popup content -->
//          </Visual>
//          <Visual /> <!-- Windowed popup 1 placeholder -->
//          <Visual /> <!-- Windowed popup 2 placeholder -->
//      </Visual>
//
// The placeholder for windowed popup 2 needs to be inserted after the placeholder for windowed popup 1, so we have
// to use windowed popup 1's placeholder visual as a reference visual. That's what we're returning here.
//
WUComp::IVisual* HWRedirectedCompTreeNodeWinRT::GetReferenceVisualForIncrementalRendering() const
{
    // This method is only used for synchronous comp tree updates, in which case the placeholder should have been
    // created when we were inserted into the tree.
    if (m_usesPlaceholderVisual)
    {
        return m_placeholderVisual.Get();
    }
    else
    {
        return m_prependVisual.Get();
    }
}
