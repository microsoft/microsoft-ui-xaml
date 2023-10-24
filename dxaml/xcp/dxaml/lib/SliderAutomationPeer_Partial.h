// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SliderAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the SliderAutomationPeer
    PARTIAL_CLASS(SliderAutomationPeer)
    {
        public:
            // Initializes a new instance of the SliderAutomationPeer class.
            SliderAutomationPeer();
            ~SliderAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            IFACEMETHOD(GetClickablePointCore)(_Out_ wf::Point* pReturnValue);
            IFACEMETHOD(GetOrientationCore)(_Out_ xaml_automation_peers::AutomationOrientation* pReturnValue);

            _Check_return_ HRESULT ChildIsAcceptable(
                _In_ xaml::IUIElement* pElement,
                _Out_ BOOLEAN* bchildIsAcceptable) override;
    };
}
