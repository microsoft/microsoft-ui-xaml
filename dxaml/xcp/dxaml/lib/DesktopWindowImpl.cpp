// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Window.g.h"
#include <DesktopWindowImpl.h>
#include "WindowActivatedEventArgs.g.h"
#include "WindowEventArgs.g.h"
#include "WindowSizeChangedEventArgs.g.h"
#include "WindowVisibilityChangedEventArgs.g.h"
#include "WRLHelper.h"
#include <windowsx.h>
#include "JupiterControl.h"
#include "WindowChrome_Partial.h"
#include <namescope\inc\NameScopeRoot.h>
#include "ColorUtil.h"
#include "FrameworkTheming.h"
#include "WindowHelpers.h"
#include "XamlIsland_Partial.h"
#include <Theme.h>
#include <dwmapi.h>
#include <windowing.h>
#include "Microsoft.UI.Windowing.h"
#include <FrameworkUdk/Theming.h>
#include <Microsoft.UI.Interop.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;

using TakeFocusRequestedHandler = wf::ITypedEventHandler<
    xaml_hosting::DesktopWindowXamlSource*,
    xaml_hosting::DesktopWindowXamlSourceTakeFocusRequestedEventArgs*>;

// ----------------------------------------------------------------------
//                          DesktopWindowImpl
// ----------------------------------------------------------------------

extern HINSTANCE g_hInstance;

// win32 window class name for top-level WinUI desktop windows
static const wchar_t s_windowClassName[]    = L"WinUIDesktopWin32WindowClass";

// Default window title for top-level WinUI desktop windows
static const wchar_t s_defaultWindowTitle[] = L"WinUI Desktop";

// Note that win32 class registration, win32 window and DesktopWindowXamlSource
// creation, and DWXS::Initialize failures are non-recoverable errors.
//
// Additionally, DXamlCore must exist before Microsoft::UI::XAML::Window can be
// created, and DXamlCore will outlive any Microsoft::UI::XAML::Window instances.
// DXamlCore::GetCurrent() will always return a valid DXamlCore instance when
// called from a non-static Microsoft::UI::XAML::Window method.
DesktopWindowImpl::DesktopWindowImpl(Window* parentWindow) : m_dxamlWindowInstance(parentWindow)
{
    // Cache DXamlCore instance for this thread
    m_dxamlCoreNoRef = DirectUI::DXamlCore::GetCurrent();

    // Register the win32 window class
    RegisterDesktopWindowClass();

    // Create the win32 window associated with this WinUI Desktop DirectUI::Window
    CreateDesktopWindow();

    // In Island w/o corewindow, Configure the JupiterWindow with m_hwnd as HWND and then trigger the initial size change.
    CJupiterControl* pjc = m_dxamlCoreNoRef->GetControl();
    if (pjc)
    {
        // Kick the initial size
        IFCFAILFAST(pjc->OnJupiterWindowSizeChanged(*(pjc->GetJupiterWindow()), m_hwnd.get()));

        // We'll receive an initial WM_SIZE event on startup, which we'll respond to and raise Window.SizeChanged. This
        // is desired behavior. This initial SizeChanged notification is also where we cache the initial size of the
        // window to return if the app is minimized right away.
    }
}

void DesktopWindowImpl::OnCreate() noexcept
{
    // Register the HWND-to-Window mapping with DXamlCore
    m_dxamlCoreNoRef->m_handleToDesktopWindowMap[m_hwnd.get()] = m_dxamlWindowInstance;

    // Creating WindowChrome here
    IFCFAILFAST(ctl::make(m_hwnd.get(), &m_windowChrome));
    // applying the window chrome style
    IFCFAILFAST(m_windowChrome->ApplyStyling());
    // Create the DesktopWindowXamlSource instance hosted by this WinUI Desktop DirectUI::Window
    IFCFAILFAST(ctl::make<DirectUI::DesktopWindowXamlSource>(&m_desktopWindowXamlSource));

    ::ABI::Microsoft::UI::WindowId parentWindowId {};
    IFCFAILFAST(::ABI::Microsoft::UI::GetWindowIdFromWindow(m_hwnd.get(), &parentWindowId));

    IFCFAILFAST(m_desktopWindowXamlSource.Cast<DirectUI::DesktopWindowXamlSourceGenerated>()->Initialize(parentWindowId));

    wrl::ComPtr<ixp::IDesktopChildSiteBridge> bridge;
    IFCFAILFAST(m_desktopWindowXamlSource->get_SiteBridge(&bridge));

    wrl::ComPtr<ixp::IDesktopSiteBridge> desktopSiteBridge;
    IFCFAILFAST(bridge.As(&desktopSiteBridge));

    ::ABI::Microsoft::UI::WindowId bridgeWindowId {};
    IFCFAILFAST(desktopSiteBridge->get_WindowId(&bridgeWindowId));
    IFCFAILFAST(::ABI::Microsoft::UI::GetWindowFromWindowId(bridgeWindowId, &m_bridgeWindowHandle));

    IFCFAILFAST(m_desktopWindowXamlSource->put_Content(m_windowChrome.Get()));

    // add a noref instance to it on window chrome
    m_windowChrome->SetDesktopWindow(this);

    // Register for DesktopWindowXamlSource's TakeFocusRequested event to support tab Focus cycling
    IFCFAILFAST(m_desktopWindowXamlSource->add_TakeFocusRequested(
                wrl::Callback<TakeFocusRequestedHandler>(this, &DesktopWindowImpl::OnDesktopWindowXamlSourceTakeFocusRequested).Get(),
                &m_takeFocusRequestedEventToken));

    // Turn off transparent islands by default. We'll turn them back on on a per-window/per-island basis as system
    // backdrop brushes are used (see put_SystemBackdrop).
    ctl::ComPtr<DirectUI::XamlIsland> xamlIsland;
    ctl::ComPtr<xaml_hosting::IXamlIsland> ixamlIsland = m_desktopWindowXamlSource->GetXamlIslandNoRef();
    IFCFAILFAST(ixamlIsland.As(&xamlIsland));
    CXamlIslandRoot* xamlIslandRoot = static_cast<CXamlIslandRoot*>(xamlIsland->GetHandle());
    xamlIslandRoot->SetHasTransparentBackground(false);

    // calling this api first time creates an appwindow object in ixp layer
    // we call it here so that the newly created appwindow object can subclass desktopwindow during init itself
    // instead of whenever user code calls appwindow api, providing subclassing consistency
    ctl::ComPtr<ixp::IAppWindow> appWindow;
    IFCFAILFAST(get_AppWindowImpl(&appWindow));
}

DesktopWindowImpl::~DesktopWindowImpl()
{
    // If the Window instance has been programmatically deleted it may not have closed
    // its members.  Make sure to close if we have not done so.
    if (!m_bIsClosed)
    {
        // this code will run only in special case where CLoseImpl has not been called
        VERIFYHR(m_dxamlWindowInstance->SetTitleBar(nullptr));
        VERIFYHR(m_dxamlWindowInstance->put_Content(nullptr));
        Shutdown();
    }
}

// ----------------------------------------------------------------------
//                               IWindow
// ----------------------------------------------------------------------

_Check_return_ HRESULT DesktopWindowImpl::get_BoundsImpl(_Out_ wf::Rect* pValue)
{
    IFC_RETURN(CheckIsWindowClosed());

    WINDOWPLACEMENT placement;
    bool isMinimized = false;
    if (::GetWindowPlacement(m_hwnd.get(), &placement) != 0)
    {
        isMinimized = (placement.showCmd == SW_SHOWMINIMIZED);
    }

    // ::GetClientRect reports 0x0 if the window is minimized. If that's the case, then we'll return the previously
    // cached window bounds instead. That's the behavior on WPF as well as system Xaml.
    if (isMinimized)
    {
        pValue->Width = m_cachedWindowBounds.Width;
        pValue->Height = m_cachedWindowBounds.Height;
    }
    else
    {
        RECT clientRect = {};
        if (::GetClientRect(m_hwnd.get(), &clientRect) == 0)
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());

            IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(hr, ERROR_WINDOW_DESKTOP_BOUNDS_FAILED));

            // GLE may return 0, even after win32 API fails
            if (SUCCEEDED(hr))
            {
                IFC_RETURN(E_FAIL);
            }

            IFC_RETURN(hr);
        }

        double rasterizationScale = 0;
        const auto dpi = ::GetDpiForWindow(m_hwnd.get());
        rasterizationScale = static_cast<float>(dpi) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);

        // Adjust dimensions for rasterization scale
        ASSERT(rasterizationScale);
        pValue->Width = static_cast<float>((clientRect.right - clientRect.left) / rasterizationScale);
        pValue->Height = static_cast<float>((clientRect.bottom - clientRect.top) / rasterizationScale);

        m_cachedWindowBounds.Width = pValue->Width;
        m_cachedWindowBounds.Height = pValue->Height;
    }

    return S_OK;
}

// WinUI Deskstop Visible tracks minimized and restored window states.
_Check_return_ HRESULT DesktopWindowImpl::get_VisibleImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(CheckIsWindowClosed());

    *pValue = !IsIconic(m_hwnd.get()) && IsWindowVisible(m_hwnd.get());

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::get_ContentImpl(_Outptr_result_maybenull_ xaml::IUIElement** ppValue)
{
    IFC_RETURN(CheckIsWindowClosed());

    ctl::ComPtr<IInspectable> content;
    IFC_RETURN(m_windowChrome->get_Content(content.ReleaseAndGetAddressOf()));
    IFC_RETURN(content.AsOrNull<xaml::IUIElement>().CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::put_ContentImpl(_In_opt_ xaml::IUIElement* pContent)
{
    IFC_RETURN(CheckIsWindowClosed());

    IFC_RETURN(m_windowChrome->put_Content(static_cast<IInspectable*>(pContent)));

    // Parenting the Window's content to the WindowChrome causes it to get disconnected from the
    // Window's namescope. Although named objects that live on Entered properties will have their
    // names registered in the new namescope, those that don't (e.g. objects assigned to custom
    // properties) will not be and so we need to transfer the Window's namescope registration's
    // to the WindowChrome's namescope to prevent those objects' names from being orphaned.
    auto pCore = m_dxamlCoreNoRef->GetHandle();
    auto pHandle = m_dxamlWindowInstance->GetHandle();

    // pHandle may be null if dxaml window instance is being deleted.
    if (pHandle)
    {
        auto pWindowNameScopeOwner = pHandle->GetStandardNameScopeOwner();
        auto pWindowChromeNameScopeOwner = m_windowChrome->GetHandle()->GetStandardNameScopeOwner();

        if (pCore->GetNameScopeRoot().HasStandardNameScopeTable(pWindowNameScopeOwner))
        {
            pCore->GetNameScopeRoot().EnsureNameScope(pWindowChromeNameScopeOwner, nullptr);
            pCore->GetNameScopeRoot().MoveNameScopeTableEntries(pWindowNameScopeOwner, pWindowChromeNameScopeOwner, Jupiter::NameScoping::NameScopeType::StandardNameScope);
        }
    }

    return S_OK;
}

// Window::CoreWindow always returns NULL on WinUI Desktop
_Check_return_ HRESULT DesktopWindowImpl::get_CoreWindowImpl(_Outptr_result_maybenull_ wuc::ICoreWindow** ppValue)
{
    IFC_RETURN(CheckIsWindowClosed());

    *ppValue = nullptr;

    return S_OK;
}

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
_Check_return_ HRESULT DesktopWindowImpl::get_DispatcherImpl(_Outptr_result_maybenull_ wuc::ICoreDispatcher** ppValue)
{
    // The CoreDispatcher is available in WinUI Desktop on the internal UWP Window
    // held by DXamlCore, but should never be used. Obtain DispatcherQueue from the
    // current thread and use that instead.  Dispatcher will return NULL on WinUI
    // Desktop to prevent misuse.
    *ppValue = nullptr;

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::get_TitleImpl(_Out_ HSTRING* pValue)
{
    IFC_RETURN(CheckIsWindowClosed());

    *pValue = nullptr;

    const auto windowTextLength = ::GetWindowTextLengthW(m_hwnd.get());
    if (windowTextLength > 0)
    {
        std::wstring windowText(windowTextLength + 1, L'\0');
        const auto retrievedSize = GetWindowTextW(m_hwnd.get(), windowText.data(), windowText.capacity());

        if (retrievedSize != windowTextLength)
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());

            IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(hr, ERROR_WINDOW_DESKTOP_TITLE_FAILED));

            // GLE may return 0, even after win32 API fails
            if (SUCCEEDED(hr))
            {
                IFC_RETURN(E_FAIL);
            }

            IFC_RETURN(hr);
        }

        IFC_RETURN(wrl_wrappers::HStringReference(windowText.data()).CopyTo(pValue));
    }

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::put_TitleImpl(_In_opt_ HSTRING value)
{
    IFC_RETURN(CheckIsWindowClosed());

    if (!SetWindowText(m_hwnd.get(), xephemeral_string_ptr(value).GetBuffer()))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());

        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(hr, ERROR_WINDOW_DESKTOP_TITLE_FAILED));

        // GLE may return 0, even after win32 API fails
        if (SUCCEEDED(hr))
        {
            IFC_RETURN(E_FAIL);
        }

        IFC_RETURN(hr);
    }

    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::add_Activated(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(m_activatedEventSource.Add(pHandler, pToken));

    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::remove_Activated(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_activatedEventSource.Remove(token));

    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::add_Closed(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(m_closedEventSource.Add(pHandler, pToken));

    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::remove_Closed(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_closedEventSource.Remove(token));

    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::add_SizeChanged(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*> * pHandler, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(m_sizeChangedEventSource.Add(pHandler, pToken));

    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::remove_SizeChanged(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_sizeChangedEventSource.Remove(token));

    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::add_VisibilityChanged(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(m_visibilityChangedEventSource.Add(pHandler, pToken));

    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::remove_VisibilityChanged(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_visibilityChangedEventSource.Remove(token));

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::ActivateImpl()
{
    IFC_RETURN(CheckIsWindowClosed());

    // Show the top-level win32 window
    auto nCmdShow = SW_SHOW;
    if (IsZoomed(m_hwnd.get()))
    {
        nCmdShow = SW_SHOWMAXIMIZED;
    }
    else if (IsIconic(m_hwnd.get()))
    {
        nCmdShow = SW_RESTORE;
    }
    ::ShowWindow(m_hwnd.get(), nCmdShow);
    ::UpdateWindow(m_hwnd.get());

    // It is necessary to set the top-level win32 window as the active window after
    // attaching DesktopWindowXamlSource.
    ::SetActiveWindow(m_hwnd.get());

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::CloseImpl()
{

    if (!m_bIsClosed && !m_bIsClosing)
    {
        auto guard = wil::scope_exit([&, this]()
        {
            // incase of any failure, closing and closed states
            // will reset so that closing operation can be attempted again
            m_bIsClosing = false;
            m_bIsClosed = false;
        });

        m_bIsClosing = true;
        // Create and populate the window closed event args
        ctl::ComPtr<WindowEventArgs> windowClosedEventArgs;
        IFC_RETURN(ctl::make(&windowClosedEventArgs));
        IFC_RETURN(windowClosedEventArgs->put_Handled(FALSE));
        // Raise the window closed event
        IFC_RETURN(m_closedEventSource.Raise(m_dxamlWindowInstance, windowClosedEventArgs.Get()));

        BOOLEAN handled = FALSE;
        IFC_RETURN(windowClosedEventArgs->get_Handled(&handled));
        if (handled)
        {
            // don't proceed to close if closing event has been handled
            // m_bIsClosing will get reset to false
            return S_OK;
        }
        m_desktopWindowXamlSource->PrepareToClose();

        // set these to null before marking window as closed as they fail if called after m_bIsClosed is set
        // because they check if window is closed already
        IFC_RETURN(m_dxamlWindowInstance->SetTitleBar(nullptr));
        IFC_RETURN(m_dxamlWindowInstance->put_Content(nullptr));

        m_windowChrome->SetDesktopWindow(nullptr);

        // Mark Desktop Window instance as 'closed'
        m_bIsClosed = true;
        guard.release(); // success, no need to reset closing and closed states

        if (!m_bMinimizedOrHidden)
        {
            // if this call fails, we don't want to reset shutdown status
            // beter to crash
            VERIFYHR(RaiseWindowVisibilityChangedEvent(FALSE));
        }
        // Close win32 window, cleanup, and unregister from hwnd mapping from DXamlCore
        Shutdown();
    }
    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::get_CompositorImpl(_Outptr_result_maybenull_ WUComp::ICompositor** compositor)
{
    IFC_RETURN(CheckIsWindowClosed());

    IFCPTR_RETURN(compositor);
    *compositor = nullptr;

    *compositor = DXamlCore::GetCurrent()->GetHandle()->GetCompositor();
    AddRefInterface(*compositor);

    return S_OK;
}

// ----------------------------------------------------------------------
//                          IWindowPrivate
// ----------------------------------------------------------------------

_Check_return_ HRESULT DesktopWindowImpl::get_TransparentBackgroundImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_NOTIMPL, ERROR_WINDOW_DESKTOP_NOT_IMPLEMENTED));

    return E_NOTIMPL;
}

_Check_return_ HRESULT DesktopWindowImpl::put_TransparentBackgroundImpl(_In_ BOOLEAN value)
{
    IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_NOTIMPL, ERROR_WINDOW_DESKTOP_NOT_IMPLEMENTED));

    return E_NOTIMPL;
}

_Check_return_ HRESULT DesktopWindowImpl::ShowImpl()
{
    if (ShowWindow(m_hwnd.get(), SW_RESTORE))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());

        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(hr, ERROR_WINDOW_DESKTOP_SIZE_OR_POSITION_FAILED));

        // GLE may return 0, even after win32 API fails
        if (SUCCEEDED(hr))
        {
            IFC_RETURN(E_FAIL);
        }

        IFC_RETURN(hr);
    }

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::HideImpl()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DesktopWindowImpl::MoveWindowImpl(_In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height)
{
    // Moving the Window should not activate it
    if (SetWindowPos(m_hwnd.get(), nullptr, x, y, width, height, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());

        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(hr, ERROR_WINDOW_DESKTOP_SIZE_OR_POSITION_FAILED));

        // GLE may return 0, even after win32 API fails
        if (SUCCEEDED(hr))
        {
            IFC_RETURN(E_FAIL);
        }

        IFC_RETURN(hr);
    }

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::SetAtlasSizeHintImpl(UINT width, UINT height)
{
    IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_NOTIMPL, ERROR_WINDOW_DESKTOP_NOT_IMPLEMENTED));

    return E_NOTIMPL;
}

_Check_return_ HRESULT DesktopWindowImpl::ReleaseGraphicsDeviceOnSuspendImpl(BOOLEAN enable)
{
    IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_NOTIMPL, ERROR_WINDOW_DESKTOP_NOT_IMPLEMENTED));

    return E_NOTIMPL;
}

_Check_return_ HRESULT DesktopWindowImpl::SetAtlasRequestCallbackImpl(_In_opt_ xaml::IAtlasRequestCallback* callback)
{
    IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_NOTIMPL, ERROR_WINDOW_DESKTOP_NOT_IMPLEMENTED));

    return E_NOTIMPL;
}

_Check_return_ HRESULT DesktopWindowImpl::GetWindowContentBoundsForElementImpl(
    _In_ xaml::IDependencyObject* element,
    _Out_ wf::Rect* rect)
{
    IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_NOTIMPL, ERROR_WINDOW_DESKTOP_NOT_IMPLEMENTED));

    return E_NOTIMPL;
}

// --------------------------------------------
//          Desktop Window Methods
// --------------------------------------------
_Check_return_ HRESULT DesktopWindowImpl::RaiseWindowActivatedEvent(_In_ const xaml::WindowActivationState state)
{
    // Create and populate the WindowActivated event args
    ctl::ComPtr<WindowActivatedEventArgs> windowActivatedEventArgs;
    IFC_RETURN(ctl::make(&windowActivatedEventArgs));
    IFC_RETURN(windowActivatedEventArgs->put_Handled(FALSE));
    IFC_RETURN(windowActivatedEventArgs->put_WindowActivationState(state));

    // Raise the WindowActivated event
    IFC_RETURN(m_activatedEventSource.Raise(m_dxamlWindowInstance, windowActivatedEventArgs.Get()));

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::RaiseWindowVisibilityChangedEvent(_In_ const BOOLEAN visible)
{
    // Create and populate the VisibilityChanged event args
    ctl::ComPtr<WindowVisibilityChangedEventArgs> windowVisibilityChangedEventArgs;
    IFC_RETURN(ctl::make(&windowVisibilityChangedEventArgs));

    IFC_RETURN(windowVisibilityChangedEventArgs->put_Visible(visible));
    IFC_RETURN(windowVisibilityChangedEventArgs->put_Handled(FALSE));

    // Raise the VisibilityChanged event
    IFC_RETURN(m_visibilityChangedEventSource.Raise(m_dxamlWindowInstance, windowVisibilityChangedEventArgs.Get()));

    return S_OK;
}


_Check_return_ HRESULT DesktopWindowImpl::OnDesktopWindowXamlSourceTakeFocusRequested(_In_ xaml::Hosting::IDesktopWindowXamlSource* desktopWindowXamlSource, _In_ xaml::Hosting::IDesktopWindowXamlSourceTakeFocusRequestedEventArgs* args)
{
    ctl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequest> ifocusNavigationRequest;
    IFC_RETURN(args->get_Request(&ifocusNavigationRequest));
    GUID correlationId;
    IFC_RETURN(ifocusNavigationRequest->get_CorrelationId(&correlationId));

    if (correlationId == m_lastTakeFocusRequestCorrelationId)
    {
        // If the correlationId is the same as the previous correlationId, do not cycle focus
        // from the last focusable element (forward: e.g. tab on the last element), or from
        // the first focusable element to thelast focusable element (reverse: e.g. shift-tab
        // on the first focusable element) when there is only a single focusable element.

        // WasFocusMoved may be false, but no action is required
        ctl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationResult> spResult;
        IFCFAILFAST(RestoreFocus(&spResult));

        return S_OK;
    }

    // Store this TakeFocusRequested correlation Id for comparison against the next
    // TakeFocusRequested event from DesktopWindowXamlSource.
    m_lastTakeFocusRequestCorrelationId = correlationId;

    xaml_hosting::XamlSourceFocusNavigationReason takeFocusReason = xaml_hosting::XamlSourceFocusNavigationReason_Programmatic;
    IFC_RETURN(ifocusNavigationRequest->get_Reason(&takeFocusReason));

    if (takeFocusReason == xaml_hosting::XamlSourceFocusNavigationReason_First
        || takeFocusReason == xaml_hosting::XamlSourceFocusNavigationReason_Last)
    {
        // The XamlSourceFocusNavigationReason sent by TakeFocusRequested will be 'First'
        // when tabbing past the last focusable element and 'Last' when shift-tabbing
        // from the first focusable element.
        ctl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequest> spRequest;
        wrl_wrappers::HStringReference xamlSourceClassName(RuntimeClass_Microsoft_UI_Xaml_Hosting_XamlSourceFocusNavigationRequest);
        ctl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequestFactory> spRequestFactory = NULL;
        IFC_RETURN(ctl::GetActivationFactory(xamlSourceClassName.Get(), &spRequestFactory));
        spRequestFactory->CreateInstanceWithHintRectAndCorrelationId(takeFocusReason, wf::Rect(), correlationId, &spRequest);

        ctl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationResult> spResultIgnored;
        IFC_RETURN(m_desktopWindowXamlSource->NavigateFocus(spRequest.Get(), &spResultIgnored));
    }

    // (For directional focus Left/Right/Up/Down, don't move or wrap when we get to the sides of the window. Just do
    // nothing, directional focus will stop if there are no more elements in that direction.)        

    return S_OK;
}

LRESULT LResultFromHResult(HRESULT hr)
{
    VERIFYHR(hr);
    return 0;
}

LRESULT DesktopWindowImpl::OnMessage(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) noexcept
{
    // Exit FrameworkApplication::ProcessMessage when the last WinUI Desktop Window
    // is destroyed.
    auto dxamlCore = DirectUI::DXamlCore::GetCurrent();
    if ((WM_DESTROY == uMsg) && dxamlCore->m_handleToDesktopWindowMap.empty())
    {
        PostQuitMessage(0);

        return 0;
    }

    switch (uMsg)
    {
        case WM_SETTINGCHANGE:
            {
                if (CJupiterControl* pjc = m_dxamlCoreNoRef->GetControl())
                {
                    VERIFYHR(pjc->HandleSettingChangedMessage(lParam));
                }
            }
            break;
        case WM_DPICHANGED:
            return OnDpiChanged(wParam, lParam);
        case WM_CLOSE:
            return LResultFromHResult(OnClosed());
        case WM_MOVE:
            return LResultFromHResult(OnMoved(wParam, lParam));
        case WM_SIZE:
            return LResultFromHResult(OnSizeChanged(wParam, lParam));
        case WM_ACTIVATE:
            return LResultFromHResult(OnActivate(wParam, lParam));
        case WM_NCRBUTTONUP:
            return LResultFromHResult(OnNonClientRegionButtonUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
        case WM_ERASEBKGND:
        {
            Theming::Theme appTheme = Theming::Theme::None;
            ctl::ComPtr<xaml::IUIElement> content;
            if(!m_bIsClosed && SUCCEEDED(get_ContentImpl(&content)) && content)
            {
                ctl::ComPtr<DirectUI::FrameworkElement> contentAsFE;
                VERIFYHR(content.As<DirectUI::FrameworkElement>(&contentAsFE));
                ASSERT(contentAsFE != nullptr);

                xaml::ElementTheme actualTheme;
                VERIFYHR(contentAsFE->get_ActualTheme(&actualTheme));
                if (actualTheme != xaml::ElementTheme_Default)
                {
                    appTheme = actualTheme == xaml::ElementTheme_Light ? Theming::Theme::Light : Theming::Theme::Dark;
                }
            }

            auto hdc = (HDC)wParam;
            auto color = ColorUtils::GetWUColor(dxamlCore->GetHandle()->GetFrameworkTheming()->GetHwndBackground(appTheme));
            RECT rc = WindowHelpers::GetClientWindowCoordinates(m_hwnd.get());
            auto oldColor  = ::SetBkColor(hdc, RGB(color.R, color.G, color.B));
            ASSERT(oldColor != CLR_INVALID);
            ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
            // restore old color back
            ::SetBkColor(hdc, oldColor);
            return 1;
        }
        default:
        {
            LRESULT nonClientAreaResult = 0;
            if (m_windowChrome)
            {
                if (m_windowChrome->HandleMessage(uMsg, wParam, lParam, &nonClientAreaResult))
                {
                    return nonClientAreaResult;
                }
            }
        }
    }

    return BaseWindow::OnMessage(uMsg, wParam, lParam);
}

// This is a private method that is only called from a Microsoft::UI::XAML::Window
// instance.  DXamlCore will always outlive any Window instances.
/* static */ Window* DesktopWindowImpl::GetMUXWindowFromHwnd(_In_ DXamlCore* dxamlCore, _In_ const HWND& topLevelDesktopWindowHwnd)
{
    const auto& iter = dxamlCore->m_handleToDesktopWindowMap.find(topLevelDesktopWindowHwnd);
    if (iter != dxamlCore->m_handleToDesktopWindowMap.end())
    {
        // WinUI Desktop Windows should always be registered.  A failed
        // Window lookup is a non-recoverable error.
        return dxamlCore->m_handleToDesktopWindowMap[topLevelDesktopWindowHwnd];
    }

    return nullptr;
}

_Check_return_ HRESULT DesktopWindowImpl::OnActivate(WPARAM wParam, LPARAM lParam)
{
     //
    // Fire the Window Activated event
    //
    if (WA_ACTIVE == LOWORD(wParam))
    {
        // WA_ACTIVE  : The window has been activated programmatically.
        IFC_RETURN(RaiseWindowActivatedEvent(xaml::WindowActivationState_CodeActivated));
    }
    else if (WA_CLICKACTIVE == LOWORD(wParam))
    {
        // WA_CLICKACTIVE : The window has been activated by pointer.
        IFC_RETURN(RaiseWindowActivatedEvent(xaml::WindowActivationState_PointerActivated));
    }
    else // WA_INACTIVE : The window has been deactivated.
    {
        if (!m_bMinimizedOrHidden)
        {
            // Save the handle of the child window that currently has Focus.  The child
            // window will be given focus the next time this window is activated.
            // if triggered by window minimize, GetFocus returns null
            HWND hwndFocus = ::GetFocus();

            if (hwndFocus && IsChild(m_hwnd.get(), hwndFocus))
            {
                m_lastFocusedWindowHandle = hwndFocus;
            }
        }

        IFC_RETURN(RaiseWindowActivatedEvent(xaml::WindowActivationState_Deactivated));
    }

    //
    // If the window has been activated and is not minimized, set focus on the last child window
    // that had focus.  If that child window is the composition island, restore focus.
    //
    if (!m_bMinimizedOrHidden && (WA_INACTIVE != LOWORD(wParam)))
    {
        // Set the focus to the child window that last had focus when this window was deactivated.
        // also enters when on such deactivation cases where m_lastFocusedWindowHandle didn't get set like in case of window minimize
        if (m_lastFocusedWindowHandle == nullptr || (m_lastFocusedWindowHandle == m_bridgeWindowHandle))
        {
            // If focus is being set back on the bridge, set window focus to it and restore focus position.
            SetFocusToBridgeWindow();

            // WasFocusMoved may be false, but no action is required
            ctl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationResult> spResult;
            IFCFAILFAST(RestoreFocus(&spResult));
        }
        else
        {
            ::SetFocus(m_lastFocusedWindowHandle);
        }
    }

    //
    // If this is the first time the window has been activated, fire Window.Visible.
    //
    if (m_bInitialWindowActivation)
    {
        // Force focus on the bridge
        SetFocusToBridgeWindow();

        // Raise VisibilityChanged on initial Window Activation
        m_bInitialWindowActivation = false;
        IFC_RETURN(RaiseWindowVisibilityChangedEvent(TRUE));
    }
    
    return S_OK;
}

// Method Description:
// - Opens the window's system menu.
// - The system menu is the menu that opens when the user presses Alt+Space or
//   right clicks on the title bar.
// - Before updating the menu, we update the buttons like "Maximize" and
//   "Restore" so that they are grayed out depending on the window's state.
// Arguments:
// - cursorX: the cursor's X position in screen coordinates
// - cursorY: the cursor's Y position in screen coordinates
void DesktopWindowImpl::OpenSystemMenu(const int cursorX, const int cursorY) const noexcept
{
    const auto systemMenu = ::GetSystemMenu(m_hwnd.get(), FALSE);

    WINDOWPLACEMENT placement = {};
    if (!::GetWindowPlacement(m_hwnd.get(), &placement))
    {
        return;
    }
    const bool isMaximized = placement.showCmd == SW_SHOWMAXIMIZED;

    // Update the options based on window state.
    MENUITEMINFO mii = {};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;
    mii.fType = MFT_STRING;
    static const auto setState = [&](UINT item, bool enabled) {
        mii.fState = enabled ? MF_ENABLED : MF_DISABLED;
        ::SetMenuItemInfo(systemMenu, item, FALSE, &mii);
    };
    setState(SC_RESTORE, isMaximized);
    setState(SC_MOVE, !isMaximized);
    setState(SC_SIZE, !isMaximized);
    setState(SC_MINIMIZE, true);
    setState(SC_MAXIMIZE, !isMaximized);
    setState(SC_CLOSE, true);
    ::SetMenuDefaultItem(systemMenu, UINT_MAX, FALSE);

    const auto ret = ::TrackPopupMenuEx(systemMenu, TPM_RETURNCMD, cursorX, cursorY, m_hwnd.get(), nullptr);
    if (ret != 0)
    {
        ::PostMessage(m_hwnd.get(), WM_SYSCOMMAND, ret, 0); //send selected menu item as command
    }
}

_Check_return_ HRESULT DesktopWindowImpl::OnNonClientRegionButtonUp(WPARAM wParam, const int x, const int y)
{
    if (wParam == HTCAPTION)
    {
        OpenSystemMenu(x, y);
    }

    return S_OK;
}

void DesktopWindowImpl::SetFocusToBridgeWindow()
{
    if (xaml_hosting::IXamlIsland* island = m_desktopWindowXamlSource->GetXamlIslandNoRef())
    {
        boolean success = false;
        VERIFYHR(island->TrySetFocus(&success));
        ASSERT(success, L"Moving focus to bridge window failed");
    }
}

LRESULT DesktopWindowImpl::OnDpiChanged(
    WPARAM wParam,
    LPARAM lParam)
{
    RECT* const rcWindow = (RECT*)lParam;

    SetWindowPos(m_hwnd.get(),
                    nullptr,
                    rcWindow->left,
                    rcWindow->top,
                    rcWindow->right - rcWindow->left,
                    rcWindow->bottom - rcWindow->top,
                    SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    return 0;
}

_Check_return_ HRESULT DesktopWindowImpl::OnSizeChanged(
    WPARAM wParam,
    LPARAM lParam)
{
    ResizeWindowToDesktopWindowXamlSourceWindowDimensions(wParam, lParam);
    IFC_RETURN(RaiseWindowSizeChangedEvent());

    switch (wParam)
    {
        case SIZE_RESTORED:
        case SIZE_MAXIMIZED:
        {
            if (m_bMinimizedOrHidden)
            {
                m_bMinimizedOrHidden = false;

                IFC_RETURN(RaiseWindowVisibilityChangedEvent(TRUE /* visible */));
            }
        }
        break;

        case SIZE_MINIMIZED:
        {
            if (!m_bMinimizedOrHidden)
            {
                m_bMinimizedOrHidden = true;

                IFC_RETURN(RaiseWindowVisibilityChangedEvent(FALSE /* visible */));
            }
        }
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::OnMoved(WPARAM wParam, LPARAM lParam)
{
    RepositionWindowToDesktopWindowXamlSourceWindowDimensions(wParam, lParam);
    return S_OK;
}


_Check_return_ HRESULT DesktopWindowImpl::OnClosed()
{
    IFC_RETURN(CloseImpl());

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::RestoreFocus(xaml_hosting::IXamlSourceFocusNavigationResult** result)
{
    // Create a XamlSourceFocusNavigationRequest
    ctl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequest> spRequest;
    wrl_wrappers::HStringReference xamlSourceClassName(RuntimeClass_Microsoft_UI_Xaml_Hosting_XamlSourceFocusNavigationRequest);
    ctl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequestFactory> spRequestFactory;
    IFC_RETURN(ctl::GetActivationFactory(xamlSourceClassName.Get(), &spRequestFactory));
    spRequestFactory->CreateInstance(xaml_hosting::XamlSourceFocusNavigationReason_Restore, &spRequest);

    // Move focus to the composition island
    IFCFAILFAST(m_desktopWindowXamlSource->NavigateFocus(spRequest.Get(), result));

    return S_OK;
}

void DesktopWindowImpl::RegisterDesktopWindowClass()
{
    WNDCLASSEXW wndClassEx = {};

    // TODO: WINUI_DESKTOP: This could crash the app if two threads both get
    // inside of this if{} block in trying to register the class.  You could
    // either on failure recheck if the class is registered and failfast then,
    // or add a lock outside the if.
    //
    // Register the desktop window class if it has not already been registered
    if (GetClassInfoExW(g_hInstance, s_windowClassName, &wndClassEx) == 0)
    {
        wndClassEx.cbSize = sizeof(WNDCLASSEX);
        wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
        wndClassEx.cbClsExtra = 0;
        wndClassEx.cbWndExtra = 0;
        wndClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wndClassEx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wndClassEx.lpszClassName = s_windowClassName;

        IFCW32FAILFAST(_RegisterClass(&wndClassEx));
    }
}

void DesktopWindowImpl::CreateDesktopWindow()
{
    _CreateWindow(
        0,                                 // Extended Style
        s_windowClassName,                 // name of window class
        s_defaultWindowTitle,              // title-bar string
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // top-level window
        CW_USEDEFAULT,                     // default horizontal position
        SW_HIDE,                           // If the y parameter is some other value,
                                           // then the window manager calls ShowWindow with that value as the nCmdShow parameter
        CW_USEDEFAULT,                     // default width
        CW_USEDEFAULT,                     // default height
        (HWND)NULL,                        // no owner window
        (HMENU)NULL);                      // use class menu
}

_Check_return_ HRESULT DesktopWindowImpl::RaiseWindowSizeChangedEvent()
{
    if (m_bIsClosed) return S_OK;

    // Create and populate the window size changed event args
    ctl::ComPtr<WindowSizeChangedEventArgs> windowSizeChangedEventArgs;
    IFC_RETURN(ctl::make(&windowSizeChangedEventArgs));

    wf::Rect bounds = {};
    IFC_RETURN(get_BoundsImpl(&bounds));

    wf::Size size = {};
    size.Width = bounds.Width;
    size.Height = bounds.Height;

    // Only raise a SizeChanged event if we find the window size to be different from the last time we raised the
    // event. This handles the case of minimizing and restoring the window, where we also get WM_SIZE messages.
    // Since we report the cached size for a minimized window in get_BoundsImpl, we won't see a different size and
    // we won't raise SizeChanged when minimizing. We'll also ignore the WM_SIZE when restoring because the size
    // matches the previous size.
    if (m_previousWindowSizeChangedSize.Width != size.Width
        || m_previousWindowSizeChangedSize.Height != size.Height)
    {
        IFC_RETURN(windowSizeChangedEventArgs->put_Handled(FALSE));
        IFC_RETURN(windowSizeChangedEventArgs->put_Size(size));

        // Raise the window size changed event
        IFC_RETURN(m_sizeChangedEventSource.Raise(m_dxamlWindowInstance, windowSizeChangedEventArgs.Get()));

        m_previousWindowSizeChangedSize = size;
    }

    return S_OK;
}

// ResizeWindowToDesktopWindowXamlSourceWindowDimensions is only called from Desktop Window's OnSizeChanged
void DesktopWindowImpl::ResizeWindowToDesktopWindowXamlSourceWindowDimensions(WPARAM wParam, LPARAM lParam)
{
    if (m_windowChrome)
    {
        m_windowChrome->ResizeContainer(wParam, lParam);
    }
}

// ResizeWindowToDesktopWindowXamlSourceWindowDimensions is only called from Desktop Window's OnMoved
void DesktopWindowImpl::RepositionWindowToDesktopWindowXamlSourceWindowDimensions(WPARAM wParam, LPARAM lParam)
{
    if (m_windowChrome)
    {
        m_windowChrome->MoveContainer(wParam, lParam);
    }
}


_Check_return_ HRESULT DesktopWindowImpl::get_WindowHandle(_Out_ HWND* pValue)
{
    *pValue = m_hwnd.get();

    return S_OK;
}

void DesktopWindowImpl::Shutdown()
{
    // Unregister from TakeFocusRequested event on DWXS
    IFCFAILFAST(m_desktopWindowXamlSource->remove_TakeFocusRequested(m_takeFocusRequestedEventToken));
    m_takeFocusRequestedEventToken.value = 0;

    m_bridgeWindowHandle = NULL;
    IFCFAILFAST(m_desktopWindowXamlSource->put_Content(nullptr));
    // Explicitly close the private DesktopWindowXamlSource instance
    VERIFYHR(m_desktopWindowXamlSource->Close());
    m_desktopWindowXamlSource.Reset();
    m_windowChrome.Reset();

    // Unregister the HWND-to-Window mapping from DXamlCore.
    auto iter = m_dxamlCoreNoRef->m_handleToDesktopWindowMap.find(m_hwnd.get());
    if (iter == m_dxamlCoreNoRef->m_handleToDesktopWindowMap.end())
    {
        // This is a fatal error.  The mapping must exist on DXamlCore for the
        // win32 Window associated with this Microsoft::UI::Xaml::Window instance.
        XAML_FAIL_FAST();
    }

    m_dxamlCoreNoRef->m_handleToDesktopWindowMap.erase(iter);

    // in case base class destructor doesn't get called
    ::DestroyWindow(m_hwnd.get());
    m_hwnd = NULL;

    // UnPeg the window, only if it's pegged before.
    m_dxamlWindowInstance->UnPeg();
}


// ----------------------------------------------------------------------
//                          instance methods
// ----------------------------------------------------------------------

_Check_return_ HRESULT DesktopWindowImpl::EnsureInitializedForIslands()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DesktopWindowImpl::GetLayoutBounds(_Out_  wf::Rect* pLayoutBounds)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DesktopWindowImpl::GetLayoutBounds(_Out_  XRECTF* pLayoutBounds)
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
//
// HasBounds() checks whether it's currently valid to call get_Bounds().
//
// A XAML Window object can represent several OS-level window constructs
// (i.e. CoreWindow, HWND). It can also represent a state in which there is no
// backing window at all - e.g. when we've just started up and don't have
// a window yet, or when we're shutting down and have de-associated from a
// window.
//
// For these reasons, it's not always possible to obtain the size (bounds)
// of a Window object. If you want to handle these cases, you can call HasBounds()
// to check whether it's safe to call get_Bounds().
//
//-----------------------------------------------------------------------------
bool DesktopWindowImpl::HasBounds()
{
    return false;
}

_Check_return_ HRESULT DesktopWindowImpl::SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow)
{
    return E_NOTIMPL;
}

// Gets the client area rect of the window in window coordinates.  This method uses
// ApplicationView.VisibleBounds to account for OS chrome occlusion and RootScrollViewer
// height to detect an open SIP when ignoreIHM is False.
_Check_return_ HRESULT DesktopWindowImpl::GetVisibleBounds(_In_ bool ignoreIHM, _In_ bool inDesktopCoordinates, _Out_ wf::Rect* pValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DesktopWindowImpl::GetRootScrollViewer(_Outptr_result_maybenull_ IScrollViewer** rootScrollViewer)
{
    ctl::ComPtr<DirectUI::XamlIsland> xamlIsland;
    ctl::ComPtr<xaml_hosting::IXamlIsland> ixamlIsland = m_desktopWindowXamlSource->GetXamlIslandNoRef();
    IFC_RETURN(ixamlIsland.As(&xamlIsland));
    *rootScrollViewer = xamlIsland->GetRootScrollViewer();
    AddRefInterface(*rootScrollViewer);
    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::OnWindowSizeChanged()
{
    return E_NOTIMPL;
}

DXamlCore* DesktopWindowImpl::GetDXamlCore()
{
    return m_dxamlCoreNoRef;
}

void DesktopWindowImpl::SetDXamlCore(_In_opt_ DXamlCore* pDXamlCore)
{
}

WindowType::Enum DesktopWindowImpl::GetWindowType()
{
     return WindowType::None;
}

ctl::ComPtr<xaml::IAtlasRequestCallback> DesktopWindowImpl::GetAtlasRequestCallback()
{
    return m_atlasRequestCallback;
}

// -------------------------------------------------------
//              DXamlTestHooks
// -------------------------------------------------------
static wf::Rect emptyRect;

UINT32 DesktopWindowImpl::GetWidthOverride()
{
    return 0;
}

void DesktopWindowImpl::SetWidthOverride(UINT32 width)
{
}

UINT32 DesktopWindowImpl::GetHeightOverride()
{
    return 0;
}

void DesktopWindowImpl::SetHeightOverride(UINT32 height)
{
}

wf::Rect DesktopWindowImpl::GetLayoutBoundsOverrides()
{
    return wf::Rect();
}

void DesktopWindowImpl::SetLayoutBoundsOverrides(const wf::Rect& rect)
{
}

wf::Rect& DesktopWindowImpl::GetVisibleBoundsOverrides()
{
    return emptyRect;
}

void DesktopWindowImpl::SetVisibleBoundsOverrides(const wf::Rect& rect)
{
}

void DesktopWindowImpl::SetHasSizeOverrides(bool hasSizeOverrides)
{
}

void DesktopWindowImpl::SetShrinkApplicationViewVisibleBounds(bool enabled)
{
}

bool DesktopWindowImpl::ShouldShrinkApplicationViewVisibleBounds() const
{
    return false;
}

HRESULT DesktopWindowImpl::CheckIsWindowClosed()
{
    if (m_bIsClosed)
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALID_OPERATION, ERROR_WINDOW_DESKTOP_ALREADY_CLOSED));

        return E_INVALID_OPERATION;
    }

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::get_ExtendsContentIntoTitleBarImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(CheckIsWindowClosed());
    *pValue = m_windowChrome->IsChromeActive();
    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::put_ExtendsContentIntoTitleBarImpl(_In_ BOOLEAN value)
{
    IFC_RETURN(CheckIsWindowClosed());
    IFC_RETURN(m_windowChrome->SetIsChromeActive(!!value));
    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::SetTitleBarImpl(_In_ xaml::IUIElement* pTitleBar)
{
    IFC_RETURN(CheckIsWindowClosed());
    IFC_RETURN(m_windowChrome->SetTitleBar(pTitleBar));
    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::get_SystemBackdrop(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush)
{
    IFC_RETURN(m_desktopWindowXamlSource->get_SystemBackdrop(systemBackdropBrush));

    return S_OK;
}

IFACEMETHODIMP DesktopWindowImpl::put_SystemBackdrop(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush)
{
    IFC_RETURN(m_desktopWindowXamlSource->put_SystemBackdrop(systemBackdropBrush));
    IFC_RETURN(SetXamlIslandRootBackground(systemBackdropBrush, nullptr));
    return S_OK;
}

void DesktopWindowImpl::MakeWindowRTL()
{
    WindowHelpers::EnableRTL(m_hwnd.get());
}


_Check_return_ HRESULT DesktopWindowImpl::get_AppWindowImpl(_Outptr_result_maybenull_ ixp::IAppWindow** ppValue)
{
    mu::WindowId windowId;
    IFC_RETURN(Windowing_GetWindowIdFromWindow(m_hwnd.get(), &windowId));

    if (!m_appWindowStatics)
    {
        IFC_RETURN(ctl::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Windowing_AppWindow).Get(),
            &m_appWindowStatics));
    }

    ctl::ComPtr<ixp::IAppWindow> appWindow;
    IFC_RETURN(m_appWindowStatics->GetFromWindowId(windowId, &appWindow));
    *ppValue = appWindow.Detach(); // ref pointer to app window object, single copy used every time

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop** systemBackdrop)
{
    IFC_RETURN(m_desktopWindowXamlSource->get_SystemBackdropImpl(systemBackdrop));

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop* systemBackdrop)
{
    IFC_RETURN(m_desktopWindowXamlSource->put_SystemBackdropImpl(systemBackdrop));
    IFC_RETURN(SetXamlIslandRootBackground(nullptr, systemBackdrop));
    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::SetXamlIslandRootBackground(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush, xaml::Media::ISystemBackdrop* systemBackdrop)
{
    ctl::ComPtr<DirectUI::XamlIsland> xamlIsland;
    ctl::ComPtr<xaml_hosting::IXamlIsland> ixamlIsland = m_desktopWindowXamlSource->GetXamlIslandNoRef();
    IFC_RETURN(ixamlIsland.As(&xamlIsland));
    CXamlIslandRoot* xamlIslandRoot = static_cast<CXamlIslandRoot*>(xamlIsland->GetHandle());

    if (systemBackdrop || systemBackdropBrush)
    {
        // Mark the top-level window as the point where HostBackdrop should be captured
        // in case the app uses DesktopAcrylic.
        IFC_RETURN(Theming_TryEnableHostBackdropBrush(m_hwnd.get()));

        // Remove solid color background - we need transparency for system backdrop to show through. Note that in-app
        // acrylic will now blend against a transparent background and look wrong.
        xamlIslandRoot->SetHasTransparentBackground(true);
    }
    else
    {
        // Restore solid color background - we need an opaque background for in-app acrylic to blend against.
        xamlIslandRoot->SetHasTransparentBackground(false);
    }

    return S_OK;
}