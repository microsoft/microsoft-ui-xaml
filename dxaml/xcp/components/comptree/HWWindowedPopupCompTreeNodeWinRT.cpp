// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <HWWindowedPopupCompTreeNodeWinRT.h>

HWWindowedPopupCompTreeNodeWinRT::HWWindowedPopupCompTreeNodeWinRT(
    _In_ CCoreServices *core,
    _In_ CompositorTreeHost *compositorTreeHost,
    _In_ DCompTreeHost* dcompTreeHost)
    : HWRedirectedCompTreeNodeWinRT(core, compositorTreeHost, dcompTreeHost)
{
    m_usesPlaceholderVisual = true;
}

void HWWindowedPopupCompTreeNodeWinRT::UpdatePrimaryVisualTransformParent(_In_ DCompTreeHost *dcompTreeHost)
{
    // Windowed Popups don't need to set a TransformParent.
    // And actually setting a TransformParent using the existing redirection target causes issues, as we saw in RS1 bug #7106674
    // The redirection target for a Windowed Popup will be the CompNode for the CRootVisual.  A picture of the XAML tree is helpful here:
    // CRootVisual
    // |                            \
    // <whatever>           CPopupRoot
    //                               |
    //                              CPopup (windowed popup)
    //
    // The CRootVisual and the WindowedPopup have completely different DComp visual trees rooted under different DComp targets.
    // The windowed popup code already has special logic to incorporate plateau scale/etc separately so we actually should avoid
    // setting a TransformParent.  Setting one actually transformed the visual, and transformed it incorrectly!
}

bool HWWindowedPopupCompTreeNodeWinRT::GetRedirectionTransformInfo(_In_ RedirectionTransformInfo* rto)
{
    // Windowed popups must not apply anything from the RedirectionTransform.
    // For unparented windowed popups there isn't anything in this transform to begin with,
    // but for parented windowed popups, there could be a transform present which we need to avoid applying.
    // The reason for this is that the windowed popup has its own visual tree which isn't parented to the main tree.
    // We compute this same transform information in CPopup::PositionAndSizeWindowForWindowedPopup and apply this
    // to the island bounds rather than the visuals.

    return false;
}


