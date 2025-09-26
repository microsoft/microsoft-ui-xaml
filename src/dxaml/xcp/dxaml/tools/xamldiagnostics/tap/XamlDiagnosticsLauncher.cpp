// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is only used for internal unit testing - this XamlDiagnosticsLauncher.dll
// is different from the one loadeed by XamlDiagnostics::Launch, which VS provides.

#include "precomp.h"
#include "XamlDiagnosticsLauncher.h"
#include "XamlDiagnosticsTap.h"
#include "wil\resource.h"
#include "throw.h"
#include <filesystem>

typedef HRESULT(_stdcall *ConnectToVisualTreeMethod)(
    _In_z_ LPCWSTR endPointName,
    _In_ DWORD pid,
    _In_z_ LPCWSTR wszDllXamlDiagnostics,
    _In_z_ LPCWSTR wszDllTAP,
    _In_ CLSID tapClsid,    
    _In_z_ LPCWSTR initializationData);

class XamlDiagnosticsLauncher;

class XamlDiagnosticsLauncherFactory
    : public wrl::ClassFactory<IXamlDiagnosticsLauncherFactory>
{
public :
    STDMETHOD(GetLauncher)(_COM_Outptr_ IXamlDiagnosticsLauncher** launcher) override;
    STDMETHOD(CreateInstance)(_Inout_opt_ IUnknown* pUnkOuter, REFIID riid, _Outptr_result_nullonfailure_ void** ppvObject);
};

// Temporary dummy callback for use with the DesignerAppManager tests. Once there is better infra we can use
// the same one visual diag tests use.
 class DummyVisualTreeServiceCallback : public Microsoft::WRL::RuntimeClass<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            Microsoft::WRL::ChainInterfaces<IVisualTreeServiceCallback2, IVisualTreeServiceCallback>,
            Microsoft::WRL::FtmBase>
{
    
public:
    IFACEMETHODIMP OnVisualTreeChange(_In_ ParentChildRelation, _In_ VisualElement,_In_ VisualMutationType)
    {
        return S_OK;
    }

    IFACEMETHODIMP OnElementStateChanged(InstanceHandle, VisualElementState, _In_z_ LPCWSTR)
    {
        return S_OK;
    }
};

STDMETHODIMP
XamlDiagnosticsLauncherFactory::CreateInstance(
    _Inout_opt_ IUnknown* pUnkOuter,
    REFIID riid,
    _Outptr_result_nullonfailure_ void** ppvObject)
{
    wrl::ComPtr<IXamlDiagnosticsLauncher> launcher;
    HRESULT hr = E_NOTIMPL;
    // This is to support how xaml diagnostics launches. We call
    // CreatInstance and pass in IID_IObjectWithSite. We'll only
    // support getting the launcher that way.
    if (InlineIsEqualGUID(riid, __uuidof(IObjectWithSite)))
    {
        hr = GetLauncher(&launcher);
    }

    if (SUCCEEDED(hr))
    {
        hr = launcher.CopyTo(riid, ppvObject);
    }

    return hr;
}

STDMETHODIMP
XamlDiagnosticsLauncherFactory::GetLauncher(
    _COM_Outptr_ IXamlDiagnosticsLauncher** launcher)
{
    HRESULT hr = S_OK;
    static wrl::ComPtr<IXamlDiagnosticsLauncher> s_launcher;
    if (!s_launcher)
    {
        hr = wrl::MakeAndInitialize<XamlDiagnosticsLauncher>(&s_launcher);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    return s_launcher.CopyTo(launcher);
}

class
    __declspec(uuid("{28cb4df8-85eb-46ee-8d71-c614c2305f74}"))
    XamlDiagnosticsLauncher :
    public wrl::RuntimeClass<
    wrl::RuntimeClassFlags<wrl::ClassicCom>,
    IObjectWithSite,
    IXamlDiagnosticsLauncher,
    wrl::FtmBase>
{
public:

    XamlDiagnosticsLauncher();

    HRESULT RuntimeClassInitialize();

    IFACEMETHOD(SetSite)(
        _In_ IUnknown *pUnkSite) override;

    IFACEMETHOD(GetSite)(
        _In_ REFIID riid,
        _Outptr_ void **ppvSite) override;

    STDMETHOD(ConnectToVisualTree)(
        _In_z_ PCWSTR xamlDiagDll,
        _In_ XamlDiagnosticsLoadedCallback connectionCallback
        ) override;

private:
    XamlDiagnosticsLoadedCallback m_tapLoadedCallback;
    std::filesystem::path m_muxPath;
};

CoCreatableClassWithFactory(XamlDiagnosticsLauncher, XamlDiagnosticsLauncherFactory);

XamlDiagnosticsLauncher::XamlDiagnosticsLauncher()
{
}

HRESULT
XamlDiagnosticsLauncher::RuntimeClassInitialize()
{
    WCHAR muxPath[MAX_PATH];
    GetModuleFileName(GetModuleHandle(L"Microsoft.UI.Xaml.dll"), muxPath, MAX_PATH);
    m_muxPath = std::filesystem::path(muxPath).remove_filename();

    return S_OK;
}


IFACEMETHODIMP 
XamlDiagnosticsLauncher::SetSite(
    _In_ IUnknown *pUnkSite)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<IUnknown> spSiteAsUnknown(pUnkSite);
    wrl::ComPtr<IXamlDiagnostics> xamlDiag;
    hr = spSiteAsUnknown.As(&xamlDiag);
    if (FAILED(hr))
    {
        return hr;
    }

    wrl::ComPtr<IXamlDiagnosticsTap> tap;
    hr = XamlDiagnosticsTap_CreateInstance(xamlDiag.Get(), &tap);

    if (FAILED(hr))
    {
        return hr;
    }

    if (m_tapLoadedCallback)
    {
        m_tapLoadedCallback(tap.Get());
    }
    m_tapLoadedCallback = nullptr;

    wil::unique_event tapConnected;
    if (tapConnected.try_open(XAMLDIAGTAP_UWPSURFACECONNECTED))
    {
        auto vtsCallback = wrl::Make<DummyVisualTreeServiceCallback>();
        wrl::ComPtr<IVisualTreeService> vts;
        hr = xamlDiag.As(&vts);
        if (SUCCEEDED(hr) && SUCCEEDED(vts->AdviseVisualTreeChange(vtsCallback.Get())))
        {
            tapConnected.SetEvent(); // If anyone is listening... (they are, i know it)
        }
    }
  
    return hr;
}

IFACEMETHODIMP 
XamlDiagnosticsLauncher::GetSite(
    _In_ REFIID riid,
    _Outptr_ void **ppvSite)
{
    return E_NOTIMPL;
}

STDMETHODIMP 
XamlDiagnosticsLauncher::ConnectToVisualTree(
    _In_z_ PCWSTR xamlDiagDll,
    _In_ XamlDiagnosticsLoadedCallback connectionCallback
    )
{
    WEX::Common::Throw::If(m_tapLoadedCallback != nullptr, E_FAIL);

    m_tapLoadedCallback = connectionCallback;

    wil::unique_hmodule initializer;

    WEX::Common::Throw::IfNull(xamlDiagDll);
    WEX::Common::Throw::If(m_muxPath.empty(), E_FAIL);

    std::wstring xamlDiagDllPath = m_muxPath / xamlDiagDll;
    initializer.reset(LoadLibraryExW(xamlDiagDllPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH));

    if (!initializer)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    auto pfnConnectToTree = reinterpret_cast<ConnectToVisualTreeMethod>(GetProcAddress(initializer.get(), "InitializeXamlDiagnosticsEx"));
    WEX::Common::Throw::IfNull(pfnConnectToTree);

    // Our test infrastructure initializes MUX much sooner than our tests do. We'll Assert here in case this isn't the case 
    std::wstring xamlDiagnosticsTapPath = m_muxPath / L"XamlDiagnosticsTap.dll";
    HRESULT hr = pfnConnectToTree(L"WinUIVisualDiagConnection1", GetCurrentProcessId(), xamlDiagDll, xamlDiagnosticsTapPath.c_str(), __uuidof(XamlDiagnosticsLauncher), nullptr);
    WEX::Common::Throw::IfFailed(hr, L"Xaml has not been initialized yet");

    return hr;
}
