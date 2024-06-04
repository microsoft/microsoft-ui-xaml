// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class XamlRenderingBackgroundTask;

    class BackgroundTaskFrameworkContext
        : public ctl::ComBase
    {
    public:
        static _Check_return_ HRESULT GlobalInit();
        static void GlobalDeinit();
        static _Check_return_ HRESULT EnsureFrameworkInitialized(_In_ XamlRenderingBackgroundTask *pBackgroundTask);
        static _Check_return_ HRESULT GetDispatcher(_Outptr_ wuc::ICoreDispatcher** ppValue);
        static xaml_markup::IXamlMetadataProvider* GetMetadataProviderNoRef();

        static void EnableStandaloneHosting();
    protected:
        BackgroundTaskFrameworkContext();
        ~BackgroundTaskFrameworkContext() override;

    private:
        _Check_return_ HRESULT EnsureFrameworkInitializedImpl(_In_ XamlRenderingBackgroundTask *pBackgroundTask);
        _Check_return_ HRESULT CreateFrameworkThread();
        _Check_return_ HRESULT InitializeFrameworkThread();
        static _Check_return_ HRESULT FrameworkThreadProc();
        _Check_return_ HRESULT PostQuitOnFrameworkThread();

        bool m_bStandaloneHosting;
        bool m_threadInitialized;
        HRESULT m_hrThreadInitialization;
        ctl::ComPtr<wuc::ICoreDispatcher> m_spCoreDispatcher;
        ctl::ComPtr<xaml_markup::IXamlMetadataProvider> m_spMetadataProvider;
        wil::critical_section m_Lock;
    };
}

