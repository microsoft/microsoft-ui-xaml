// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RadioButtonAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the RadioButtonAutomationPeer
    PARTIAL_CLASS(RadioButtonAutomationPeer)
    {
        public:
            // Initializes a new instance of the RadioButtonAutomationPeer class.
            RadioButtonAutomationPeer();
            ~RadioButtonAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);

            _Check_return_ HRESULT SelectImpl();
            _Check_return_ HRESULT AddToSelectionImpl();
            _Check_return_ HRESULT RemoveFromSelectionImpl();
            _Check_return_ HRESULT get_IsSelectedImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_SelectionContainerImpl(_Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue);

            _Check_return_ HRESULT RaiseIsSelectedPropertyChangedEvent(_In_ BOOLEAN bOldValue, _In_ BOOLEAN bNewValue);

    };
}
