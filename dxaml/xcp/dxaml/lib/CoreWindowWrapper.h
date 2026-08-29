// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.ui.core.h>

namespace DirectUI
{
    class CoreWindowWrapper
    {
    public:
        static _Check_return_ HRESULT Create(_In_ wuc::ICoreWindow* pCoreWindow, _Outptr_ CoreWindowWrapper** ppCoreWindowWrapper);
        ~CoreWindowWrapper();

        wuc::ICoreWindow* GetCoreWindow()
        {
            return m_spCoreWindow.Get();
        }

        HWND GetHWND();
        static HWND GetHWND(_In_ wuc::ICoreWindow* coreWindow);

        //
        // IMPORTANT: Don't use CoreDispatcher in XAML framework code.
        //
        // CoreDispatcher is exposed through our API for apps to use,
        // but we need to avoid taking any internal dependencies on it,
        // because it's not available in all environments we support.
        //
        // Instead, use DirectUI::IDispatcher. This is available through:
        //     DXamlCore::GetXamlDispatcher()
        //     DependencyObject::GetXamlDispatcher()
        //
        wuc::ICoreDispatcher* GetCoreDispatcher()
        {
            return m_spCoreDispatcher.Get();
        }

        _Check_return_ HRESULT AddActivatedHandler(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT RemoveActivatedHandler(_In_ EventRegistrationToken token);
        _Check_return_ HRESULT AddClosedHandler(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT RemoveClosedHandler(_In_ EventRegistrationToken token);
        _Check_return_ HRESULT AddSizeChangedHandler(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT RemoveSizeChangedHandler(_In_ EventRegistrationToken token);
        _Check_return_ HRESULT AddVisibilityChangedHandler(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT RemoveVisibilityChangedHandler(_In_ EventRegistrationToken token);

        _Check_return_ HRESULT SetPosition(_In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height);

    private:
        CoreWindowWrapper();
        static _Check_return_ HRESULT SetPositionImpl(_In_ HWND hWnd, _In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height);

        ctl::ComPtr<wuc::ICoreWindow> m_spCoreWindow;
        ctl::ComPtr<wuc::ICoreDispatcher> m_spCoreDispatcher;
    };
}
