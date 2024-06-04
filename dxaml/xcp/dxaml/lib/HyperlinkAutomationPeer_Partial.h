// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "HyperlinkAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the HyperlinkAutomationPeer
    PARTIAL_CLASS(HyperlinkAutomationPeer)
    {
        public:
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue) override;
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue) override;
            IFACEMETHOD(IsControlElementCore)(_Out_ BOOLEAN* pReturnValue) override;
            IFACEMETHOD(IsContentElementCore)(_Out_ BOOLEAN* pReturnValue) override;

            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(IsEnabledCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(GetBoundingRectangleCore)(_Out_ wf::Rect* returnValue) override;
            IFACEMETHOD(IsKeyboardFocusableCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(GetClickablePointCore)(_Out_ wf::Point* returnValue) override;
            IFACEMETHOD(IsOffscreenCore)(_Out_ BOOLEAN * returnValue) override;

            IFACEMETHOD(GetAcceleratorKeyCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetAccessKeyCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetAutomationIdCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetHelpTextCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetItemStatusCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetItemTypeCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetLabeledByCore)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;
            IFACEMETHOD(GetLiveSettingCore)(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue) override;

            // Support the IInvokeProvider interface.
            _Check_return_ HRESULT InvokeImpl();

            _Check_return_ HRESULT get_Owner(_Outptr_ xaml_docs::IHyperlink** pValue);
            _Check_return_ HRESULT put_Owner(_In_ xaml_docs::IHyperlink* pOwner);

        private:
            // Keep a weak ref to the owner; we don't want to keep it alive, and using a weak reference (rather than an
            // uncounted raw pointer) prevents problems during the time period between GC and finalization if the
            // owner is a CLR object.
            ctl::WeakRefPtr m_wpOwner;
    };
}
