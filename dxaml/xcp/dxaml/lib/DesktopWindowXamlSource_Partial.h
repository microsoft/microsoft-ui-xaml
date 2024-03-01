// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DesktopWindowXamlSource.g.h"
#include <fwd/Microsoft.UI.Xaml.hosting.h>
#include <Microsoft.UI.Content.h>

namespace DirectUI
{
    class WindowsXamlManager;

    PARTIAL_CLASS(DesktopWindowXamlSource)
    {
        friend class DesktopWindowXamlSourceGenerated;

    public:
        DesktopWindowXamlSource();
        ~DesktopWindowXamlSource() override;

        // IClosable
        IFACEMETHOD(Close)() override;

    public:
        // Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop implementation
        IFACEMETHOD(get_SystemBackdrop)(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush) override;
        IFACEMETHOD(put_SystemBackdrop)(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush) override;

        _Check_return_ HRESULT get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop** iSystemBackdrop);
        _Check_return_ HRESULT put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop* iSystemBackdrop);

        _Check_return_ xaml_hosting::IXamlIslandRoot* GetXamlIslandRootNoRef();
        void PrepareToClose();

    private:
        _Check_return_ HRESULT InitializeImpl(_In_ mu::WindowId parentWnd);

        _Check_return_ HRESULT get_ContentImpl(_Outptr_ xaml::IUIElement** ppValue);
        _Check_return_ HRESULT put_ContentImpl(_In_opt_ xaml::IUIElement* pValue);

        _Check_return_ HRESULT get_SiteBridgeImpl(_Outptr_result_maybenull_ ixp::IDesktopChildSiteBridge** ppValue);

        _Check_return_ HRESULT NavigateFocusImpl(
            _In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request,
            _Outptr_ xaml_hosting::IXamlSourceFocusNavigationResult** ppResult);

        _Check_return_ HRESULT get_HasFocusImpl(_Out_ boolean* pResult);

        _Check_return_ HRESULT get_ShouldConstrainPopupsToWorkAreaImpl(_Outptr_ boolean* pValue);
        _Check_return_ HRESULT put_ShouldConstrainPopupsToWorkAreaImpl(_In_opt_ boolean value);

        _Check_return_ HRESULT GetGotFocusEventSourceNoRef(_Outptr_ GotFocusEventSourceType** ppEventSource) override;
        _Check_return_ HRESULT GetTakeFocusRequestedEventSourceNoRef(_Outptr_ TakeFocusRequestedEventSourceType** ppEventSource) override;

        _Check_return_ HRESULT AttachToWindow(_In_ HWND parentHwnd);
        _Check_return_ HRESULT CheckWindowingModelPolicy();

        static void FireFrameworkTelemetry();
        static void InstrumentUsage(_In_ bool fRemove);

    protected:

        _Check_return_ HRESULT Initialize() override;
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        HWND GetChildHwnd() const
        {
            return m_childHwnd;
        }

        _Check_return_ HRESULT get_FocusController(_Outptr_ xaml_hosting::IFocusController** ppValue) const;

        _Check_return_ HRESULT ConnectToHwndIslandSite(_In_ HWND parentHwnd);

        _Check_return_ HRESULT OnFocusControllerGotFocus(_In_ IInspectable* sender, _In_ IInspectable* args);
        _Check_return_ HRESULT OnFocusControllerLosingFocus(_In_ IInspectable* sender, _In_ IInspectable* args);

        _Check_return_ HRESULT ReleaseFocusController();

    private:

        bool m_bClosed = false;
        bool m_initializedCalled = false;
        bool m_bridgeClosed {false};
        ctl::ComPtr<WindowsXamlManager> m_spXamlCore;
        ctl::ComPtr<xaml_hosting::IXamlIslandRoot> m_spXamlIsland;
        ctl::ComPtr<ixp::IContentSiteBridge> m_contentBridge;
        ctl::ComPtr<ixp::IDesktopChildSiteBridge> m_contentBridgeDW;
        ctl::ComPtr<ixp::IDesktopSiteBridge> m_desktopBridge;

        HWND m_childHwnd = {};

        ctl::ComPtr<xaml::Media::ISystemBackdrop> m_systemBackdrop;

        ctl::ComPtr<xaml_hosting::IFocusController> m_spFocusController;
        ctl::ComPtr<TakeFocusRequestedEventSourceType> m_spLosingFocusEventSource;
        EventRegistrationToken m_losingFocusEventCookie = {};
        ctl::ComPtr<GotFocusEventSourceType> m_spGotFocusEventSource;
        EventRegistrationToken m_gotFocusEventCookie = {};
        EventRegistrationToken m_bridgeClosedToken {};
    };
}

