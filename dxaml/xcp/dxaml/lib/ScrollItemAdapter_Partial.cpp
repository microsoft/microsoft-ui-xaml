// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScrollItemAdapter.g.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Set the owner of Adapter
_Check_return_ HRESULT
ScrollItemAdapter::put_Owner(
    _In_ xaml_automation_peers::IFrameworkElementAutomationPeer* automationPeer)
{
    IFC_RETURN(ctl::AsWeak(automationPeer, &m_automationPeerWeakRef));

    return S_OK;
}

_Check_return_ HRESULT ScrollItemAdapter::ScrollIntoViewImpl()
{
    ctl::ComPtr<xaml_automation_peers::IFrameworkElementAutomationPeer> automationPeer =
        m_automationPeerWeakRef.AsOrNull<xaml_automation_peers::IFrameworkElementAutomationPeer>();

    if (automationPeer)
    {
        ctl::ComPtr<xaml::IUIElement> spOwner;
        wf::Rect rect = {};
        wf::Size size = {};

        IFC_RETURN(automationPeer->get_Owner(&spOwner));
        IFC_RETURN(spOwner.Cast<UIElement>()->get_RenderSize(&size));

        rect.X = 0;
        rect.Y = 0;
        rect.Width = size.Width;
        rect.Height = size.Height;

        spOwner.Cast<UIElement>()->BringIntoView(rect, true /*forceIntoView*/, false /*useAnimation*/, false /*skipDuringManipulation*/);
    }

    return S_OK;
}
