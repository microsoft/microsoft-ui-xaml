// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollItemAdapter.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ScrollItemAdapter)
    {
        public:
            _Check_return_ HRESULT put_Owner(_In_ xaml_automation_peers::IFrameworkElementAutomationPeer* automationPeer);

            _Check_return_ HRESULT ScrollIntoViewImpl();

        private:
            // Automation Peer that owns this adapter
            ctl::WeakRefPtr m_automationPeerWeakRef;
    };
}
