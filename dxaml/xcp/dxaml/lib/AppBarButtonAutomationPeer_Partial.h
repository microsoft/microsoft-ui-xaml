// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AppBarButtonAutomationPeer.g.h"
#include "AppBarButton.g.h"

namespace DirectUI
{
    // Represents the AppBarButtonAutomationPeer
    PARTIAL_CLASS(AppBarButtonAutomationPeer)
    {
        public:
            // Initializes a new instance of the AppBarButtonAutomationPeer class.
            AppBarButtonAutomationPeer();
            ~AppBarButtonAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetLocalizedControlTypeCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAcceleratorKeyCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(IsKeyboardFocusableCore)(_Out_ BOOLEAN* returnValue) override;

            // IExpandCollapseProvider
            _Check_return_ HRESULT get_ExpandCollapseStateImpl(_Out_ xaml_automation::ExpandCollapseState* returnValue);

            _Check_return_ HRESULT ExpandImpl();
            _Check_return_ HRESULT CollapseImpl();

            _Check_return_ HRESULT RaiseExpandCollapseAutomationEvent(_In_ BOOLEAN isOpen);

            _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* returnValue) final;
            _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* returnValue) final;

        private:
            _Check_return_ HRESULT GetOwningAppBarButton(_Outptr_ AppBarButton** owningAppBarButton);
    };
}
