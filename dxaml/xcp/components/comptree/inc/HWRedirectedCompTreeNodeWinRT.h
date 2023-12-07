// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "HWCompNodeWinRT.h"

// HWRedirectedCompTreeNodeWinRT is a special type of CompNode used for redirection scenarios (Popup and LTEs).
// The redirected CompNode derives its transform from another branch in the UIElement tree.
class HWRedirectedCompTreeNodeWinRT : public HWCompTreeNodeWinRT
{
public:
    explicit HWRedirectedCompTreeNodeWinRT(
        _In_ CCoreServices *core,
        _In_ CompositorTreeHost *compositorTreeHost,
        _In_ DCompTreeHost* dcompTreeHost);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<HWRedirectedCompTreeNodeWinRT>::Index;
    }

    void SetRedirectionTarget(
        _In_opt_ HWCompTreeNode* redirectionTarget,
        _In_ const CMILMatrix* prependTransformFromRedirectionTargetToAncestor
        );

    void SetUsesPlaceholderVisual(bool value);

    wrl::ComPtr<ixp::IVisual> EnsurePlaceholderVisual(_In_ DCompTreeHost *dompTreeHost);

    ixp::IVisual* GetPlaceholderVisual() const { return m_placeholderVisual.Get(); }

    xref_ptr<WUComp::IVisual> GetWUCVisualInMainTree() const override;

    WUComp::IVisual* GetReferenceVisualForIncrementalRendering() const override;

protected:
    void UpdatePrependTransform() override;

    void UpdatePrependClip(_In_ DCompTreeHost *dcompTreeHost) override;

    void UpdatePrimaryVisualTransformParent(_In_ DCompTreeHost *dcompTreeHost) override;

    bool GetRedirectionTransformInfo(_In_ RedirectionTransformInfo* rto) override;

protected:
    //
    // The redirection target's HWCompTreeNode, if it has one (only LTE's have separate targets).
    // Otherwise, for Popup, it's the closest ancestor CompNode of the Popup.
    //
    // We don't want to make this a strong reference to avoid reference cycles, since this can point anywhere in
    // the tree. We made this a weak pointer because there are scenarios involving nested LTEs that can turn this
    // into a dangling pointer:
    //
    //  <RootVisual>
    //      <LTE Outer Target="Canvas1" />
    //      <PopupRoot>
    //          <Canvas3>
    //              <Canvas2 HasCompNode>
    //                  <Canvas1 />
    //              </Canvas2>
    //          </Canvas3>
    //          <LTE Inner Target="Canvas3" />
    //      </PopupRoot>
    //  </RootVisual>
    //
    // Here we have a couple of nested LTEs. The outer LTE points at Canvas1, and uses Canvas2's comp node as the
    // redirection target.
    //
    // Suppose that the inner LTE gets Opacity="0" set on it. These things happen on the next render walk:
    //   1. Outer LTE renders, and picks up Canvas2's comp node as its redirection target.
    //   2. PopupRoot renders, which walks to Canvas3. We skip rendering Canvas3 because there's an LTE pointing at it.
    //   3. Inner LTE renders, and finds itself with Opacity="0". It gets culled from the walk, which makes its
    //      target subtree release comp nodes. Canvas2 then releases its comp node.
    //
    // We get in trouble because Canvas2 released its comp node after the outer LTE referenced it. The outer LTE
    // will AV if it tries to access its redirection target, and a weak ref allows it to avoid the AV.
    //
    // This is an edge scenario because in a majority of cases, LTEs are rendered after children. The outer LTE
    // usually renders after PopupRoot, which avoids this problem completely. In that case, the inner LTE renders
    // first and clears out the comp node on Canvas2. The outer LTE renders second and walks past Canvas2 on its
    // search for a comp node. The root visual is special in that it puts its PopupRoot on top of its TransitionRoot
    // in z-order, which makes the outer LTE render first. We should not change this z-order, otherwise an element
    // beneath a popup can suddenly render on top when it starts unloading from the page.
    //
    // Finally, using a weak ref is not the perfect solution. If the target comp node is released, then we'll render
    // the outer LTE as an absolutely positioned LTE, which can move it on screen and produce a flicker. In practice,
    // this nested LTE scenario is rare enough that not crashing is enough, especially since the target of the outer
    // LTE is in a subtree with Opacity="0" so it will be invisible anyway.
    //
    xref::weakref_ptr<HWCompTreeNode> m_redirectionTarget;

    // The redirected CompNode draws with the same transform as its redirection target. We
    // have the transform from m_redirectionTargetNoRef up to the root (it's the world
    // transform on m_pRedirectionTargetAncestorNoRef). But since there's no guarantee that the
    // redirection target itself has a HWCompTreeNode, we still need the transform from the
    // redirection target up to m_redirectionTargetNoRef.
    //
    // For a tree that looks like:
    //   <root HWCompTreeNode>
    //     <a>
    //       <b HWCompTreeNode>
    //         <c>
    //           <d/>
    //         </c>
    //       </b>
    //       <e HWRedirectedCompTreeNodeDComp RedirectionTarget="d" m_pRedirectionTargetAncestorNoRef="b" />
    //     </a>
    //   </root>
    //
    // This field stores the transform from the redirection target d up to its HWCompTreeNode
    // parent b. This then gets combined with the world transform on b to get the transform from
    // d up to the root.
    CMILMatrix m_transformFromRedirectionTargetToRedirectionCompNode;

    // Placeholder visual, because the real visual is added to the popup's window.
    wrl::ComPtr<ixp::IVisual> m_placeholderVisual;
};

