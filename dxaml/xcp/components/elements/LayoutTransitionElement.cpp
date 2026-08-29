// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <core.h>
#include <corep.h>
#include <UIElement.h>
#include <TransitionTarget.h>
#include <LayoutTransitionElement.h>
#include <Panel.h>
#include <Canvas.h>
#include <TransitionRoot.h>
#include <CompositeTransform.h>
#include <DCompTreeHost.h>

//------------------------------------------------------------------------
//
//  ctor
//
//------------------------------------------------------------------------
CLayoutTransitionElement::CLayoutTransitionElement(
    _In_ CUIElement *pTargetUIE,
    bool isAbsolutelyPositioned
    )
    : CUIElement(pTargetUIE->GetContext())
    , m_pTarget(pTargetUIE)
    , m_destinationOffset()
    , m_isAbsolutelyPositioned(isAbsolutelyPositioned)
    , m_isPrimaryTransition(FALSE)
    , m_isRegisteredOnCore(FALSE)
    , m_pSecondaryPathRenderData(NULL)
{
    pTargetUIE->AddRef();

    // When an element gets a 3D transform, or when a 3D element is parented somewhere in the tree, it sends a
    // flag up the tree to mark the 3D branches. This walk knows how to propagate from an LTE's target up to
    // the LTE. But if a new LTE is created and targets an existing 3D branch of the tree, the target won't
    // propagate anything to the LTE. It's up to the LTE to check its target explicitly.
    UpdateHas3DDepthInSubtree();
}

//------------------------------------------------------------------------
//
//  dtor
//
//------------------------------------------------------------------------
CLayoutTransitionElement::~CLayoutTransitionElement()
{
    // If this LTE hasn't detached itself from the target then do so now. Otherwise the target is left with a dangling
    // pointer up to this deleted LTE, and if it attempts to propagate dirty flags then it will hit an AV. Normally
    // LTEs are cleaned up with DetachTransition before they're released, but if a subtree containing an LTE is detached,
    // then we go through LeaveImpl which does not clean up. Ths is usually not a problem because the target of the LTE
    // is in the same subtree underneath the LTE and will be deleted first, but this is not the case in ResetVisualTree,
    // since VisualRoot::ResetRoots tears down the popup root before the root scroll viewer.
    if (m_isRegisteredOnCore)
    {
        CUIElement *targetNoRef = GetTargetElement();
        VERIFYHR(targetNoRef->RemoveLayoutTransitionRenderer(this));
    }

    auto core = GetContext();
    ReleaseInterface(m_pTarget);

    ClearSecondaryRenderData();
    SAFE_DELETE(m_pSecondaryPathRenderData);

    if (m_isRegisteredOnCore && core)
    {
        VERIFYHR(core->UnregisterRedirectionElement(this));
        m_isRegisteredOnCore = FALSE;
    }
}
