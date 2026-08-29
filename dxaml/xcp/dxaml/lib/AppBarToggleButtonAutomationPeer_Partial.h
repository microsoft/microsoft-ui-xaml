// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AppBarToggleButtonAutomationPeer.g.h"
#include "AppBarToggleButton.g.h"

namespace DirectUI
{
    // Represents the AppBarToggleButtonAutomationPeer
    PARTIAL_CLASS(AppBarToggleButtonAutomationPeer)
    {
        public:
            // Initializes a new instance of the AppBarToggleButtonAutomationPeer class.
            AppBarToggleButtonAutomationPeer();
            ~AppBarToggleButtonAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetLocalizedControlTypeCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAcceleratorKeyCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(IsKeyboardFocusableCore)(_Out_ BOOLEAN* returnValue) override;

            _Check_return_ HRESULT ToggleImpl();
            _Check_return_ HRESULT get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue);

            static _Check_return_ HRESULT ConvertToToggleState(_In_ IInspectable* value, _Out_ xaml_automation::ToggleState* pToggleState);

            _Check_return_ HRESULT RaiseToggleStatePropertyChangedEvent(
                _In_ IInspectable* pOldValue,
                _In_ IInspectable* pNewValue) override;

            _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* returnValue) final;
            _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* returnValue) final;

        private:
            _Check_return_ HRESULT GetOwningAppBarToggleButton(_Outptr_ AppBarToggleButton** owningAppBarToggleButton);
    };
}
