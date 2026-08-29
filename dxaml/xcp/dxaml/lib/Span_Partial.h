// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Span.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(Span)
    {
    protected:
        IFACEMETHOD(OnDisconnectVisualChildren)() override;
        _Check_return_ HRESULT AppendAutomationPeerChildren(_In_ wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAutomationPeerChildren, _In_ INT startPos, _In_ INT endPos) override;
    };
}
