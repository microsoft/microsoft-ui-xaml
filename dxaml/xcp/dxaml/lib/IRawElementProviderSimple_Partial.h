// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IRawElementProviderSimple.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(IRawElementProviderSimple)
    {
        public:
            // Initializes a new instance of the RawElementProviderSimple class.
            IRawElementProviderSimple();
            IRawElementProviderSimple(xaml_automation_peers::IAutomationPeer* pAutomationPeer);

            // Destroys an instance of the RawElementProviderSimple class.
            ~IRawElementProviderSimple() override;

            _Check_return_ HRESULT GetAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer);
            _Check_return_ HRESULT SetAutomationPeer(_In_ xaml_automation_peers::IAutomationPeer* pAutomationPeer);

        private:
            xaml_automation_peers::IAutomationPeer* m_pAutomationPeer;
    };
}
