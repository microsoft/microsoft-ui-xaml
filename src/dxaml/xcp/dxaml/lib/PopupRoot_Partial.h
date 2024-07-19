// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PopupRoot.g.h"

namespace DirectUI
{
    // Represents the PopupRoot control
    PARTIAL_CLASS(PopupRoot)
    {
        protected:
            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
        public:
            _Check_return_ HRESULT CloseTopmostPopup();
            _Check_return_ HRESULT IsTopmostPopupInLightDismissChain(_Out_ BOOLEAN* pbIsInLightDismissChain);
    };
}
