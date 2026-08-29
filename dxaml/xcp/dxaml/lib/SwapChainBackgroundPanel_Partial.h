// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SwapChainPanelOwner.h"
#include "SwapChainBackgroundPanel.g.h"


namespace DirectUI
{
    PARTIAL_CLASS(SwapChainBackgroundPanel),
        public ISwapChainBackgroundPanelNative,
        public ISwapChainPanelOwner
    {
        friend class SwapChainBackgroundPanelFactory;
        BEGIN_INTERFACE_MAP(SwapChainBackgroundPanel, SwapChainBackgroundPanelGenerated)
            INTERFACE_ENTRY(SwapChainBackgroundPanel, ISwapChainBackgroundPanelNative)
            INTERFACE_ENTRY(SwapChainBackgroundPanel, ISwapChainBackgroundPanel)
        END_INTERFACE_MAP(SwapChainBackgroundPanel, SwapChainBackgroundPanelGenerated)

    public:
        IFACEMETHOD(SetSwapChain)(_In_opt_ IDXGISwapChain *pSwapChain) override;

        _Check_return_ HRESULT OnTreeParentUpdated(
            _In_opt_ CDependencyObject *pNewParent,
            BOOLEAN isParentAlive) override;

        CSwapChainPanel* GetSwapChainPanel() override;

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    };
}
