// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MediaPlayerElementAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the MediaPlayerElementAutomationPeer
    PARTIAL_CLASS(MediaPlayerElementAutomationPeer)
    {
        public:
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetLocalizedControlTypeCore)(_Out_ HSTRING* returnValue) final;
    };
}
