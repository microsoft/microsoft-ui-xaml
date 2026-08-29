// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Hyperlink.g.h"

namespace DirectUI
{
    // Represents the Hyperlink
    PARTIAL_CLASS(Hyperlink)
    {
    public:
        _Check_return_ HRESULT OnClick();

        // UIA callback
        _Check_return_ HRESULT OnCreateAutomationPeer (_Outptr_ xaml_automation_peers::IAutomationPeer **ppAutomationPeer) override;

        _Check_return_ HRESULT AutomationHyperlinkClick();


        _Check_return_ HRESULT FocusImpl(
            _In_ xaml::FocusState value,
            _Out_ BOOLEAN* returnValue);

    };
}
