// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SwapChainPanelOwner.h"
#include "SwapChainPanel.g.h"

class CSwapChainPanel;


namespace DirectUI
{
    PARTIAL_CLASS(SwapChainPanel),
        public ISwapChainPanelNative2,
        public ISwapChainPanelOwner
    {
        friend class SwapChainPanelFactory;
        BEGIN_INTERFACE_MAP(SwapChainPanel, SwapChainPanelGenerated)
            INTERFACE_ENTRY(SwapChainPanel, ISwapChainPanelNative)
            INTERFACE_ENTRY(SwapChainPanel, ISwapChainPanelNative2)
        END_INTERFACE_MAP(SwapChainPanel, SwapChainPanelGenerated)

    public:
        SwapChainPanel() = default;
        ~SwapChainPanel() override = default;

        IFACEMETHOD(SetSwapChainHandle)(_In_opt_ HANDLE swapChainHandle) override;

        IFACEMETHOD(SetSwapChain)(_In_opt_ IDXGISwapChain *pSwapChain) override;

        CSwapChainPanel* GetSwapChainPanel() override;

        _Check_return_ HRESULT CreateCoreIndependentInputSourceImpl(
            _In_ ixp::InputPointerSourceDeviceKinds deviceKinds,
            _Outptr_ ixp::IInputPointerSource** ppPointerInputSource);

        _Check_return_ HRESULT SetUseTransparentVisualIfNeeded(ctl::WeakRefPtr wpThis);

    protected:
        _Check_return_ HRESULT Initialize() override;

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        wil::critical_section m_Lock;
    };
}
