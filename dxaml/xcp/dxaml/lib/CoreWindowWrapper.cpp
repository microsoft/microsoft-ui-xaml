// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CoreWindowWrapper.h"
#include "Window.g.h"
#include "WindowEventArgs.g.h"
#include "WindowActivatedEventArgs.g.h"
#include "WindowSizeChangedEventArgs.g.h"
#include "WindowVisibilityChangedEventArgs.g.h"
#include "FrameworkApplication_Partial.h"
#include "EventAdapter.h"
#include <CoreWindow.h>
#include <shellscalingapi.h>
#include <windows.ui.core.corewindow-defs.h>
#include <cmath>
#include <windows.graphics.display.h>

using namespace DirectUI;

_Check_return_ HRESULT CoreWindowWrapper::Create(_In_ wuc::ICoreWindow* pCoreWindow, _Outptr_ CoreWindowWrapper** ppCoreWindowWrapper)
{
    HRESULT hr = S_OK;
    CoreWindowWrapper* pCoreWindowWrapper = NULL;

    pCoreWindowWrapper = new CoreWindowWrapper();

    pCoreWindowWrapper->m_spCoreWindow = pCoreWindow;
    IFC(pCoreWindow->get_Dispatcher(pCoreWindowWrapper->m_spCoreDispatcher.ReleaseAndGetAddressOf()));
    *ppCoreWindowWrapper = pCoreWindowWrapper;
    pCoreWindowWrapper = NULL;

Cleanup:
    delete pCoreWindowWrapper;

    RRETURN(hr);
}

CoreWindowWrapper::CoreWindowWrapper()
{
}

CoreWindowWrapper::~CoreWindowWrapper()
{
}

_Check_return_ HRESULT RaiseActivatedEvent(
    _In_ wuc::ICoreWindow* pInnerSender,
    _In_ wuc::IWindowActivatedEventArgs* pInnerArgs,
    _In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pOuterHandler)
{
    static_assert(static_cast<int>(wuc::CoreWindowActivationState_CodeActivated) == static_cast<int>(xaml::WindowActivationState_CodeActivated),
        "CoreWindowActivationState and WindowActivationState enums mismatched for CodeActivated");
    static_assert(static_cast<int>(wuc::CoreWindowActivationState_Deactivated) == static_cast<int>(xaml::WindowActivationState_Deactivated),
        "CoreWindowActivationState and WindowActivationState enums mismatched for Deactivated");
    static_assert(static_cast<int>(wuc::CoreWindowActivationState_PointerActivated) == static_cast<int>(xaml::WindowActivationState_PointerActivated),
        "CoreWindowActivationState and WindowActivationState enums mismatched for PointerActivated");

    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;
    auto state = wuc::CoreWindowActivationState_CodeActivated;
    ctl::ComPtr<wuc::ICoreWindowEventArgs> coreWindowEventArgs;
    ctl::ComPtr<WindowActivatedEventArgs> windowActivatedEventArgs;
    auto pXamlCore = DXamlCore::GetCurrent();
    if (!pXamlCore)
    {
        goto Cleanup;
    }

    IFC(pInnerArgs->QueryInterface(IID_PPV_ARGS(&coreWindowEventArgs)));
    IFC(pInnerArgs->get_WindowActivationState(&state));
    IFC(ctl::ComObject<WindowActivatedEventArgs>::CreateInstance<WindowActivatedEventArgs>(&windowActivatedEventArgs));
    IFC(coreWindowEventArgs->get_Handled(&handled));
    IFC(windowActivatedEventArgs->put_Handled(handled));
    IFC(windowActivatedEventArgs->put_WindowActivationState(static_cast<xaml::WindowActivationState>(state)));

    // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
    IFC(pOuterHandler->Invoke(
        ctl::as_iinspectable(pXamlCore->GetUwpWindowNoRef()),
        windowActivatedEventArgs.Get()));

    IFC(windowActivatedEventArgs->get_Handled(&handled));
    IFC(coreWindowEventArgs->put_Handled(handled));

Cleanup:
    // We will always fail on an out of memory since it isn't recoverable anyway and this way we can
    // determine which handler we were calling
    if (hr == E_OUTOFMEMORY)
    {
        IFCFAILFAST(hr);
    }

    if (FAILED(hr))
    {
        ErrorHelper::ReportUnhandledErrorFromWrappedDelegate(hr);
    }

    return hr;
}

_Check_return_ HRESULT CoreWindowWrapper::AddActivatedHandler(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    typedef EventAdapter<
        wuc::IWindowActivatedEventHandler,
        wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>,
        wuc::ICoreWindow,
        wuc::IWindowActivatedEventArgs> ActivatedEventAdapter;

    ctl::ComPtr<ActivatedEventAdapter> spAdapter;

    IFC_RETURN(ActivatedEventAdapter::Create(
        pHandler,
        RaiseActivatedEvent,
        &spAdapter));

    IFC_RETURN(m_spCoreWindow->add_Activated(spAdapter.Get(), pToken));

    return S_OK;
}

_Check_return_ HRESULT CoreWindowWrapper::RemoveActivatedHandler(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_spCoreWindow->remove_Activated(token));
    return S_OK;
}

_Check_return_ HRESULT RaiseClosedEvent(
    _In_ wuc::ICoreWindow* pInnerSender,
    _In_ wuc::ICoreWindowEventArgs* pInnerArgs,
    _In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pOuterHandler)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;
    ctl::ComPtr<WindowEventArgs> windowEventArgs;
    auto pXamlCore = DXamlCore::GetCurrent();
    if (!pXamlCore)
    {
        goto Cleanup;
    }

    IFC(ctl::ComObject<WindowEventArgs>::CreateInstance<WindowEventArgs>(&windowEventArgs));
    IFC(pInnerArgs->get_Handled(&handled));
    IFC(windowEventArgs->put_Handled(handled));

    // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
    IFC(pOuterHandler->Invoke(
        ctl::as_iinspectable(pXamlCore->GetUwpWindowNoRef()),
        windowEventArgs.Get()));

    IFC(windowEventArgs->get_Handled(&handled));
    IFC(pInnerArgs->put_Handled(handled));

Cleanup:
    if (FAILED(hr))
    {
        ErrorHelper::ReportUnhandledErrorFromWrappedDelegate(hr);
    }

    RRETURN(hr);
}

_Check_return_ HRESULT CoreWindowWrapper::AddClosedHandler(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    typedef EventAdapter<
        wuc::IWindowClosedEventHandler,
        wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>,
        wuc::ICoreWindow,
        wuc::ICoreWindowEventArgs> ClosedEventAdapter;

    ctl::ComPtr<ClosedEventAdapter> spAdapter;

    IFC_RETURN(ClosedEventAdapter::Create(
        pHandler,
        RaiseClosedEvent,
        &spAdapter));

    IFC_RETURN(m_spCoreWindow->add_Closed(spAdapter.Get(), pToken));

    return S_OK;
}

_Check_return_ HRESULT CoreWindowWrapper::RemoveClosedHandler(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_spCoreWindow->remove_Closed(token));
    return S_OK;
}

_Check_return_ HRESULT RaiseSizeChangedEvent(
    _In_ wuc::ICoreWindow* pInnerSender,
    _In_ wuc::IWindowSizeChangedEventArgs* pInnerArgs,
    _In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*>* pOuterHandler)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;
    wf::Size size;
    ctl::ComPtr<wuc::ICoreWindowEventArgs> coreWindowEventArgs;
    ctl::ComPtr<WindowSizeChangedEventArgs> windowSizeChangedEventArgs;
    auto pXamlCore = DXamlCore::GetCurrent();
    if (!pXamlCore)
    {
        goto Cleanup;
    }

    IFC(pInnerArgs->QueryInterface(IID_PPV_ARGS(&coreWindowEventArgs)));
    IFC(pInnerArgs->get_Size(&size));
    IFC(ctl::ComObject<WindowSizeChangedEventArgs>::CreateInstance<WindowSizeChangedEventArgs>(&windowSizeChangedEventArgs));
    IFC(coreWindowEventArgs->get_Handled(&handled));
    IFC(windowSizeChangedEventArgs->put_Handled(handled));
    IFC(windowSizeChangedEventArgs->put_Size(size));

    // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
    IFC(pOuterHandler->Invoke(
        ctl::as_iinspectable(pXamlCore->GetUwpWindowNoRef()),
        windowSizeChangedEventArgs.Get()));

    IFC(windowSizeChangedEventArgs->get_Handled(&handled));
    IFC(coreWindowEventArgs->put_Handled(handled));

Cleanup:
    if (FAILED(hr))
    {
        ErrorHelper::ReportUnhandledErrorFromWrappedDelegate(hr);
    }

    RRETURN(hr);
}

_Check_return_ HRESULT CoreWindowWrapper::AddSizeChangedHandler(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    typedef EventAdapter<
        wuc::IWindowSizeChangedEventHandler,
        wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*>,
        wuc::ICoreWindow,
        wuc::IWindowSizeChangedEventArgs> SizeChangedEventAdapter;

    ctl::ComPtr<SizeChangedEventAdapter> spAdapter;

    IFC_RETURN(SizeChangedEventAdapter::Create(
        pHandler,
        RaiseSizeChangedEvent,
        &spAdapter));

    IFC_RETURN(m_spCoreWindow->add_SizeChanged(spAdapter.Get(), pToken));

    return S_OK;
}

_Check_return_ HRESULT CoreWindowWrapper::RemoveSizeChangedHandler(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_spCoreWindow->remove_SizeChanged(token));
    return S_OK;
}

_Check_return_ HRESULT RaiseVisibilityChangedEvent(
    _In_ wuc::ICoreWindow* pInnerSender,
    _In_ wuc::IVisibilityChangedEventArgs* wucVisibilityChangedEventArgs,
    _In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pOuterHandler)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<WindowVisibilityChangedEventArgs> windowVisibilityChangedEventArgs;
    ctl::ComPtr<xaml::IWindowVisibilityChangedEventArgs> iWindowVisibilityChangedEventArgs;
    ctl::ComPtr<wuc::ICoreWindowEventArgs> iCoreWindowEventArgs;

    auto pXamlCore = DXamlCore::GetCurrent();
    if (!pXamlCore)
    {
        goto Cleanup;
    }

    // WUC event args must be copied to MUX event args before raising the event
    IFC(ctl::make(&windowVisibilityChangedEventArgs));
    boolean isVisible = FALSE;
    IFC(wucVisibilityChangedEventArgs->get_Visible(&isVisible));
    IFC(windowVisibilityChangedEventArgs->put_Visible(isVisible));

    boolean isHandled = FALSE;
    IFC(wucVisibilityChangedEventArgs->QueryInterface(IID_PPV_ARGS(&iCoreWindowEventArgs)));
    IFC(iCoreWindowEventArgs->get_Handled(&isHandled));
    IFC(windowVisibilityChangedEventArgs->put_Handled(isHandled));

    // Raise MUX VisibilityChanged event
    IFC(windowVisibilityChangedEventArgs.As<xaml::IWindowVisibilityChangedEventArgs>(&iWindowVisibilityChangedEventArgs));

    // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
    IFC(pOuterHandler->Invoke(
        ctl::as_iinspectable(pXamlCore->GetUwpWindowNoRef()),
        iWindowVisibilityChangedEventArgs.Get()));

    // Pass the MUX event args Handled value back in to the WUC event args
    IFC(iWindowVisibilityChangedEventArgs->get_Handled(&isHandled));
    IFC(iCoreWindowEventArgs->put_Handled(isHandled));

Cleanup:
    if (FAILED(hr))
    {
        ErrorHelper::ReportUnhandledErrorFromWrappedDelegate(hr);
    }

    RRETURN(hr);
}

_Check_return_ HRESULT CoreWindowWrapper::AddVisibilityChangedHandler(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    typedef EventAdapter<
        wuc::IVisibilityChangedEventHandler,
        wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>,
        wuc::ICoreWindow,
        wuc::IVisibilityChangedEventArgs> VisibilityChangedEventAdapter;

    HRESULT hr = S_OK;
    ctl::ComPtr<VisibilityChangedEventAdapter> spAdapter;

    IFC(VisibilityChangedEventAdapter::Create(
        pHandler,
        RaiseVisibilityChangedEvent,
        &spAdapter));

    IFC(m_spCoreWindow->add_VisibilityChanged(spAdapter.Get(), pToken));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CoreWindowWrapper::RemoveVisibilityChangedHandler(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_spCoreWindow->remove_VisibilityChanged(token));
    return S_OK;
}

_Check_return_ HRESULT CoreWindowWrapper::SetPosition(_In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height)
{
    IFC_RETURN(SetPositionImpl(GetHWND(), x, y, width, height));
    return S_OK;
}

_Check_return_ HRESULT CoreWindowWrapper::SetPositionImpl(_In_ HWND hWnd, _In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height)
{
    double scale = 0.0;

    ctl::ComPtr<wgrd::IDisplayInformation> displayInformation = DXamlCore::GetCurrent()->GetDisplayInformationNoRef();
    ctl::ComPtr<wgrd::IDisplayInformation2> displayInformation2;
    IFC_RETURN(displayInformation.As(&displayInformation2));
    IFC_RETURN(displayInformation2->get_RawPixelsPerViewPixel(&scale));

    x = static_cast<INT>(std::round(scale * x));
    y = static_cast<INT>(std::round(scale * y));
    width = static_cast<INT>(std::round(scale * width));
    height = static_cast<INT>(std::round(scale * height));

    if (!SetWindowPos(hWnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE))
    {
        IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    return S_OK;
}

HWND CoreWindowWrapper::GetHWND(_In_ wuc::ICoreWindow* coreWindow)
{
    HWND hWnd = nullptr;
    ctl::ComPtr<ICoreWindowInterop> coreWindowInterop;

    IFCFAILFAST(ctl::do_query_interface(coreWindowInterop, coreWindow));
    IFCFAILFAST(coreWindowInterop->get_WindowHandle(&hWnd));

    return hWnd;
}

HWND CoreWindowWrapper::GetHWND()
{
    return GetHWND(m_spCoreWindow.Get());
}
