// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PopupRootAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the PopupRootAutomationPeer
    PARTIAL_CLASS(PopupRootAutomationPeer)
    {
    public:
        // Initializes a new instance of the PopupRootAutomationPeer class.
        PopupRootAutomationPeer();
        ~PopupRootAutomationPeer() override;

        IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
        IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
        IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
        IFACEMETHOD(IsControlElementCore)(_Out_ BOOLEAN* returnValue);
        IFACEMETHOD(IsContentElementCore)(_Out_ BOOLEAN* returnValue);
        IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue);
        IFACEMETHOD(GetAutomationIdCore)(_Out_ HSTRING* returnValue);
        IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);

        _Check_return_ HRESULT GetFlowsFromCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue) final;
        _Check_return_ HRESULT GetFlowsToCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue) final;

        _Check_return_ HRESULT InvokeImpl();

    protected:
        _Check_return_ HRESULT GetLightDismissingPopupAP(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
    };
}
