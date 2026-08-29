// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ButtonBaseAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ButtonBaseAutomationPeer
    PARTIAL_CLASS(ButtonBaseAutomationPeer)
    {
        public:
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue);

        protected:
            // Initializes a new instance of the ButtonBaseAutomationPeer class.
            ButtonBaseAutomationPeer();
            ~ButtonBaseAutomationPeer() override;
    };
}
