// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <FrameworkUdk/InputPaneFramework.h>


class FrameworkInputPaneOneCore WrlFinal :
                public  Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
                 udk_::IInputPaneVisibilityChangedListener,
                 IFrameworkInputPane>
{
    // implements some common IInspectable functions
    // https://docs.microsoft.com/en-us/cpp/cppcx/wrl/walkthrough-creating-a-windows-store-app-using-wrl-and-media-foundation?view=vs-2019
    InspectableClass(L"FrameworkInputPaneOneCore", TrustLevel::BaseTrust)
public:
    FrameworkInputPaneOneCore();

    // IFrameworkInputPane
    IFACEMETHODIMP Advise(_In_ IUnknown *pWindow, _In_ IFrameworkInputPaneHandler *pHandler, _Out_ DWORD *pdwCookie) override;
    IFACEMETHODIMP AdviseWithHWND(_In_ HWND hwnd, _In_ IFrameworkInputPaneHandler *pHandler, _Out_ DWORD *pdwCookie) override;
    IFACEMETHODIMP Unadvise(_In_ DWORD dwCookie) override;
    IFACEMETHODIMP Location(_Out_ RECT *prcInputPaneScreenLocation) override;
    IFACEMETHODIMP AdviseInternal(_In_ XHANDLE hWindow, _In_ CFrameworkInputPaneHandler *pHandler, _Out_ DWORD *pdwCookie);
    IFACEMETHODIMP OnFrameworkInputPaneVisibilityChanged(_In_ wf::Rect rcInputPane,
                                                    _In_ boolean bVisible,
                                                    _In_ boolean bEnsureFocusedElementInView,
                                                    _In_ boolean notifyEditFocusLostOnHiding) override;
private:
    ~FrameworkInputPaneOneCore() override;
    CFrameworkInputPaneHandler* m_pFrameworkHandler;
    Microsoft::WRL::ComPtr<wuv::IInputPane> m_inputPane;
    boolean m_bVisible;
    RECT m_rcInputPane;
};
