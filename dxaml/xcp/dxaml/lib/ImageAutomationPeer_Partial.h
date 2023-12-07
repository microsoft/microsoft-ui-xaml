// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ImageAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ImageAutomationPeer
    PARTIAL_CLASS(ImageAutomationPeer)
    {
    public:
        // TODO: All this should be overriding Impl methods but it inherits from FrameworkAutomationPeer which does it by overriding
        //                 the non-Impl methods so this class has to follow suit.  Fixing it in FrameworkAutomationPeer would be a big undertaking
        //                 since there are a lot of peers that derive from that.
        IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue) override;
        IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue) override;
        IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue) override;

        _Check_return_ HRESULT GetFullDescriptionCoreImpl(_Out_ HSTRING* returnValue) override;
    };
}
