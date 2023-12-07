// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents the Faceplate AutomationPeer class for ContentPresenter
//      used as faceplate to display Selected Content in ComboBox.

#pragma once

#include "FaceplateContentPresenterAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the FaceplateContentPresenterAutomationPeer
    PARTIAL_CLASS(FaceplateContentPresenterAutomationPeer)
    {
        public:
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* pReturnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* pReturnValue) final;
            _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* pReturnValue) final;
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING *returnValue);
    };
}
