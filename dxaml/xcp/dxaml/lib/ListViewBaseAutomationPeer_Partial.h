// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListViewBaseAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ListViewBaseAutomationPeer
    PARTIAL_CLASS(ListViewBaseAutomationPeer)
    {
        public:
            // Initializes a new instance of the ListViewBaseAutomationPeer class.
            ListViewBaseAutomationPeer();
            ~ListViewBaseAutomationPeer() override;

            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue) override;
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue) override;
            IFACEMETHOD(IsOffscreenCore)(_Out_ BOOLEAN* returnValue) override;

            _Check_return_ HRESULT SetUiaDropTargetDropEffect(_In_ HSTRING value, _In_ BOOLEAN raisePropertyChanged);

            // IDropTargetProvider
            _Check_return_ HRESULT get_DropEffectImpl(_Out_ HSTRING* pValue);
            _Check_return_ HRESULT get_DropEffectsImpl(_Out_ UINT* pCount, _Outptr_ HSTRING** pValue);
    };
}
