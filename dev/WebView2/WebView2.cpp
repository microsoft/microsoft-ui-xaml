// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include <corewindow.h>
#include "dcomp.h"
#include "InvokableCallbackHelper.h"
#include "math.h"
#include "ResourceAccessor.h"
#include "RuntimeProfiler.h"
#include "UIAutomationCore.h"
#include "UIAutomationCoreApi.h"
#include "WebView2.h"
#include "WebView2AutomationPeer.h"
#include "WebView2Utility.h"
#include "Windows.Globalization.h"
#include <wrl\event.h>

// TODO: Lots of analyze warnings
#pragma warning( disable : 26496 26462 26461 4471)

#include <winuser.h>
#include <uiautomationclient.h>
#include <WebView2Interop.h>

static constexpr wstring_view s_error_wv2_closed{ L"Cannot create CoreWebView2 for Closed WebView2 element."sv };
static constexpr wstring_view s_error_cwv2_not_present{ L"Failed because a valid CoreWebView2 is not present. Make sure one was created, for example by calling EnsureCoreWebView2Async() API."sv };
static constexpr wstring_view s_error_cwv2_not_present_closed{ L"Failed because a valid CoreWebView2 is not present. One existed, but has been closed."sv };

WebView2::WebView2()
{
    if (auto user32module = GetModuleHandleW(L"user32.dll"))
    {
        m_fnClientToScreen = reinterpret_cast<decltype(m_fnClientToScreen)>(GetProcAddress(user32module, "ClientToScreen"));
        m_fnSendMessageW = reinterpret_cast<decltype(m_fnSendMessageW)>(GetProcAddress(user32module, "SendMessageW"));
        m_fnCreateWindowExW = reinterpret_cast<decltype(m_fnCreateWindowExW)>(GetProcAddress(user32module, "CreateWindowExW"));
        m_fnDefWindowProcW = reinterpret_cast<decltype(m_fnDefWindowProcW)>(GetProcAddress(user32module, "DefWindowProcW"));
        m_fnGetFocus = reinterpret_cast<decltype(m_fnGetFocus)>(GetProcAddress(user32module, "GetFocus"));
        m_fnRegisterClassW = reinterpret_cast<decltype(m_fnRegisterClassW)>(GetProcAddress(user32module, "RegisterClassW"));
    }

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_WebView2);

    m_loadedRevoker = Loaded(winrt::auto_revoke, { this, &WebView2::OnLoaded });
    m_unloadedRevoker = Unloaded(winrt::auto_revoke, { this, &WebView2::OnUnloaded });

    // Turn off DirectManipulation for this element so that all scrolling and gesture
    // inside the WebView element will be handled by Anaheim.
    this->ManipulationMode(winrt::ManipulationModes::None);

    // TODO_WebView2: These can be deferred to CreateCoreObjects
    m_manipulationModeChangedToken.value = RegisterPropertyChangedCallback(winrt::UIElement::ManipulationModeProperty(),
        { this, &WebView2::OnManipulationModePropertyChanged });
    m_visibilityChangedToken.value = RegisterPropertyChangedCallback(winrt::UIElement::VisibilityProperty(),
        { this, &WebView2::OnVisibilityPropertyChanged });
    OnVisibilityPropertyChanged(nullptr, nullptr);

    // IsTabStop is false by default for UIElement
    IsTabStop(true);

    // Set the background for WebView2 to ensure it will be visible to hit-testing.
    Background(winrt::SolidColorBrush(winrt::Colors::Transparent()));

}

void WebView2::OnManipulationModePropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/)
{
    MUX_FAIL_FAST_MSG("WebView2.ManipulationMode cannot be set to anything other than \"None\".");
};

void WebView2::OnVisibilityPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/)
{
    UpdateRenderedSubscriptionAndVisibility();
}

void WebView2::UnregisterCoreEventHandlers()
{
    if (m_coreWebView)
    {
        m_coreNavigationStartingRevoker.revoke();
        m_coreSourceChangedRevoker.revoke();
        m_coreNavigationCompletedRevoker.revoke();
        m_coreWebMessageReceivedRevoker.revoke();
        m_coreProcessFailedRevoker.revoke();
    }

    if (m_coreWebViewController)
    {
        m_coreMoveFocusRequestedRevoker.revoke();
        m_coreLostFocusRevoker.revoke();
    }

    if (m_coreWebViewCompositionController)
    {
        m_cursorChangedRevoker.revoke();
    }
}

// Public Close() API
void WebView2::Close()
{
    CloseInternal(false /* inShutdownPath */);
}

// Close Implementation (notice this does not implement IClosable). Also called implicitly as part of destruction.
void WebView2::CloseInternal(bool inShutdownPath)
{
    DisconnectFromRootVisualTarget();

    UnregisterCoreEventHandlers();

    m_xamlRootChangedRevoker.revoke();

    if (m_manipulationModeChangedToken.value != 0)
    {
        UnregisterPropertyChangedCallback(winrt::UIElement::ManipulationModeProperty(), m_manipulationModeChangedToken.value);
        m_manipulationModeChangedToken.value = 0;
    }

    if (m_visibilityChangedToken.value != 0)
    {
        UnregisterPropertyChangedCallback(winrt::UIElement::VisibilityProperty(), m_visibilityChangedToken.value);
        m_visibilityChangedToken.value = 0;
    }

    if (m_coreWebView)
    {
        m_coreWebView = nullptr;
    }

    if (m_coreWebViewController)
    {
        m_coreWebViewController.Close();
        m_coreWebViewController = nullptr;
    }

    if (m_coreWebViewCompositionController)
    {
        m_coreWebViewCompositionController = nullptr;
    }

    // If called from destructor, skip ResetProperties() as property values no longer matter.
    // (Otherwise, Xaml Core will assert on failure to resurrect WV2's DXaml Peer)
    if (!inShutdownPath)
    {
        ResetProperties();
    }

    m_isClosed = true;
}


WebView2::~WebView2()
{
    CloseInternal(true /* inShutdownPath */);
}

void WebView2::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_SourceProperty)
    {
        const auto newValueUri = unbox_value<winrt::Uri>(args.NewValue());

        if (ShouldNavigate(newValueUri))
        {
            if (!m_isClosed)
            {
                OnSourceChanged(newValueUri);
            }
            else
            {
                throw winrt::hresult_error(RO_E_CLOSED, s_error_wv2_closed);
            }
        }
    }
    else if ((property == s_CanGoBackProperty || property == s_CanGoForwardProperty) && !m_canGoPropSetInternally)
    {
        throw winrt::hresult_invalid_argument();
    }
}

void WebView2::HandlePointerPressed(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    // Chromium handles WM_MOUSEXXX for mouse, WM_POINTERXXX for touch
    winrt::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };
    winrt::PointerPoint pointerPoint{ args.GetCurrentPoint(*this) };
    UINT message = 0x0;

    if (deviceType == winrt::PointerDeviceType::Mouse)
    {
        winrt::PointerPointProperties properties{ pointerPoint.Properties() };
        // WebView takes mouse capture to avoid missing pointer released events that occur outside of the element that
        // end pointer pressed state inside the webview. Example, scrollbar is being used and mouse is moved out
        // of webview bounds before being released, the webview will miss the released event and upon reentry into
        // the webview, the mouse will still cause the scrollbar to move as if selected.
        m_hasMouseCapture = this->CapturePointer(args.Pointer());

        if (properties.IsLeftButtonPressed())
        {
            // Double Click is working as well with this code, presumably by being recognized on browser side from WM_LBUTTONDOWN/ WM_LBUTTONUP
            message = WM_LBUTTONDOWN;
            m_isLeftMouseButtonPressed = true;
        }
        else if (properties.IsMiddleButtonPressed())
        {
            message = WM_MBUTTONDOWN;
            m_isMiddleMouseButtonPressed = true;
        }
        else if (properties.IsRightButtonPressed())
        {
            message = WM_RBUTTONDOWN;
            m_isRightMouseButtonPressed = true;
        }
        else if (properties.IsXButton1Pressed())
        {
            message = WM_XBUTTONDOWN;
            m_isXButton1Pressed = true;
        }
        else if (properties.IsXButton2Pressed())
        {
            message = WM_XBUTTONDOWN;
            m_isXButton2Pressed = true;
        }

        else
        {
            MUX_ASSERT(false);
        }
    }
    else if (deviceType == winrt::PointerDeviceType::Touch)
    {
        message = WM_POINTERDOWN;

        int32_t id = pointerPoint.PointerId();
        m_hasTouchCapture.insert(std::pair<int32_t, bool>(id, this->CapturePointer(args.Pointer())));
    }
    else if (deviceType == winrt::PointerDeviceType::Pen)
    {
        message = WM_POINTERDOWN;
        m_hasPenCapture = this->CapturePointer(args.Pointer());
    }

    // Since OnXamlPointerMessage() will mark the args handled, Xaml FocusManager will ignore
    // this Pressed event when it bubbles up to the XamlRoot, not setting focus as expected.
    // Thus, we need to manually set Xaml Focus (Pointer) on WebView2 here.
    // TODO_WebView2: Update this to UIElement::Focus [sync version], when it becomes available.

    if (IsTabStop() == true)
    {
      winrt::FocusManager::TryFocusAsync(*this, winrt::FocusState::Pointer);
    }

    OnXamlPointerMessage(message, args);
  }

void WebView2::HandlePointerReleased(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    // Chromium handles WM_MOUSEXXX for mouse, WM_POINTERXXX for touch
    winrt::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };
    winrt::PointerPoint pointerPoint{ args.GetCurrentPoint(*this) };
    winrt::PointerPointProperties properties{ pointerPoint.Properties() };
    UINT message = 0x0;

    // Get pointer id for handling multi-touch capture
    int32_t id = pointerPoint.PointerId();

    if (deviceType == winrt::PointerDeviceType::Mouse)
    {
        if (m_isLeftMouseButtonPressed)
        {
            message = WM_LBUTTONUP;
            m_isLeftMouseButtonPressed = false;
        }
        else if (m_isMiddleMouseButtonPressed)
        {
            message = WM_MBUTTONUP;
            m_isMiddleMouseButtonPressed = false;
        }
        else if (m_isRightMouseButtonPressed)
        {
            message = WM_RBUTTONUP;
            m_isRightMouseButtonPressed = false;
        }
        else if (m_isXButton1Pressed)
        {
            message = WM_XBUTTONUP;
            m_isXButton1Pressed = false;
        }
        else if (m_isXButton2Pressed)
        {
            message = WM_XBUTTONUP;
            m_isXButton2Pressed = false;
        }
        else
        {
            // It is not guaranteed that we will get a PointerPressed before PointerReleased.
            // For example, the mouse can be pressed in the space next to a webview, dragged
            // into the webview, and then released. This is a valid case and should not crash.
            // Because we can't always know what button was pressed before a release, we can't
            // forward this message on to CoreWebView2.
            return;
        }

        if (m_hasMouseCapture)
        {
            this->ReleasePointerCapture(args.Pointer());
            m_hasMouseCapture = false;
        }
    }
    else
    {
        if (m_hasTouchCapture.count(id) > 0)
        {
            this->ReleasePointerCapture(args.Pointer());
            m_hasTouchCapture.erase(id);
        }

        if (m_hasPenCapture)
        {
            this->ReleasePointerCapture(args.Pointer());
            m_hasPenCapture = false;
        }

        message = WM_POINTERUP;
    }

    OnXamlPointerMessage(message, args);
}

void WebView2::HandlePointerMoved(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    // Chromium handles WM_MOUSEXXX for mouse, WM_POINTERXXX for touch
    winrt::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };
    UINT message = deviceType == winrt::PointerDeviceType::Mouse ? WM_MOUSEMOVE : WM_POINTERUPDATE;
    OnXamlPointerMessage(message, args);
}

void WebView2::HandlePointerWheelChanged(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    // Chromium handles WM_MOUSEXXX for mouse, WM_POINTERXXX for touch
    winrt::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };
    UINT message = deviceType == winrt::PointerDeviceType::Mouse ? WM_MOUSEWHEEL : WM_POINTERWHEEL;
    OnXamlPointerMessage(message, args);
}

void WebView2::HandlePointerExited(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    winrt::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };
    UINT message;

    if (deviceType == winrt::PointerDeviceType::Mouse)
    {
        message = WM_MOUSELEAVE;
        if (!m_hasMouseCapture)
        {
            ResetMouseInputState();
        }
    }
    else
    {
        message = WM_POINTERLEAVE;
    }

    OnXamlPointerMessage(message, args);
}

void WebView2::HandlePointerEntered(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    winrt::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };

    m_isPointerOver = true;
    m_oldCursor = winrt::CoreWindow::GetForCurrentThread().PointerCursor();

    UpdateCoreWindowCursor();

    if (deviceType != winrt::PointerDeviceType::Mouse) //mouse does not have an equivalent pointer_entered event, so only handling pen/touch
    {
        OnXamlPointerMessage(WM_POINTERENTER, args);
    }
}

void WebView2::HandlePointerCanceled(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    ResetPointerHelper(args);
}

void WebView2::HandlePointerCaptureLost(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    ResetPointerHelper(args);
}

void WebView2::ResetPointerHelper(const winrt::PointerRoutedEventArgs& args)
{
    winrt::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };

    if (m_isPointerOver)
    {
        m_isPointerOver = false;
        winrt::CoreWindow::GetForCurrentThread().PointerCursor(m_oldCursor);
        m_oldCursor = nullptr;
    }

    if (deviceType == winrt::PointerDeviceType::Mouse)
    {
        m_hasMouseCapture = false;
        m_isLeftMouseButtonPressed = false;
        m_isMiddleMouseButtonPressed = false;
        m_isRightMouseButtonPressed = false;
        m_isXButton1Pressed = false;
        m_isXButton2Pressed = false;
    }
    else if (deviceType == winrt::PointerDeviceType::Touch)
    {
        // Get pointer id for handling multi-touch capture
        winrt::PointerPoint logicalPointerPoint{ args.GetCurrentPoint(*this) };
        int32_t id = logicalPointerPoint.PointerId();
        if (m_hasTouchCapture.count(id) > 0)
        {
            m_hasTouchCapture.erase(id);
        }
    }
    else if (deviceType == winrt::PointerDeviceType::Pen)
    {
        m_hasPenCapture = false;
    }
}

bool WebView2::ShouldNavigate(const winrt::Uri& uri)
{
    return uri != nullptr && uri.RawUri() != m_stopNavigateOnUriChanged;
}

winrt::IAsyncAction WebView2::OnSourceChanged(winrt::Uri providedUri)
{
    // Trigger / wait for CWV2 creation if one isn't yet ready
    if (!m_coreWebView)
    {
        // Creation request when one is already in flight (either explicit or implicit)
        if (m_creationInProgressAsync)
        {
            MUX_ASSERT(m_isExplicitCreationInProgress || m_isImplicitCreationInProgress);

            // If the in-flight creation is explicit (EnsureCWV2()), navigate to new source when that ends
            // Otherwise it's implicit (another Source set), we are done; first Source will use updated Source value
            if (m_isExplicitCreationInProgress)
            {
                // If EnsureCWV2() call is in flight, wait for it to complete then set Source
                co_await *m_creationInProgressAsync;
            }
            else if (m_isImplicitCreationInProgress)
            {
                // If Set Source is in flight, nothing to do - it will apply latest source when it completes
                co_return;
            }
        }
        // First-time creation request
        else
        {
            m_isImplicitCreationInProgress = true;
            co_await CreateCoreObjects();
        }
    }

    if (m_coreWebView)
    {
        // TODO_WebView2: Should we use RawUri() instead of AbsoluteUri()?
        // Try to apply latest source (could have changed during the co_await's above)
        if (m_isClosed)
        {
            throw winrt::hresult_error(RO_E_CLOSED, s_error_wv2_closed);
        }
        const auto updatedUri = this->Source();
        if (!updatedUri.Equals(providedUri) && ShouldNavigate(updatedUri))
        {
            m_coreWebView.Navigate(updatedUri.AbsoluteUri());
        }
        else
        {
            m_coreWebView.Navigate(providedUri.AbsoluteUri());
        }
    }

    co_return;
}

HWND WebView2::GetHostHwnd() noexcept
{
    if (!m_xamlHostHwnd)
    {
        winrt::CoreWindow coreWindow = winrt::CoreWindow::GetForCurrentThread();
        if (coreWindow)
        {
            auto coreWindowInterop = coreWindow.as<ICoreWindowInterop>();
            winrt::check_hresult(coreWindowInterop->get_WindowHandle(&m_xamlHostHwnd));
        }
        else
        {
            auto xamlRoot = this->XamlRoot();
            if (xamlRoot)
            {
#ifdef WINUI3
                com_ptr<IXamlRootNative> xamlRootNative = xamlRoot.as<IXamlRootNative>();
                winrt::check_hresult(xamlRootNative->get_HostWindow(&m_xamlHostHwnd));
#endif
            }
        }
    }

    return m_xamlHostHwnd;
}

HWND WebView2::GetActiveInputWindowHwnd() noexcept
{
    if (!m_inputWindowHwnd)
    {
        auto inputWindowHwnd = m_fnGetFocus();
        if (!inputWindowHwnd)
        {
            winrt::check_hresult(HRESULT_FROM_WIN32(::GetLastError()));
        }
        MUX_ASSERT(inputWindowHwnd != m_xamlHostHwnd); // Focused XAML host window cannot be set as input hwnd
        m_inputWindowHwnd = inputWindowHwnd;
    }
    return m_inputWindowHwnd;
}

void WebView2::RegisterCoreEventHandlers()
{
    m_coreNavigationStartingRevoker = m_coreWebView.NavigationStarting(winrt::auto_revoke, {
        [this](auto const&, winrt::CoreWebView2NavigationStartingEventArgs const& args)
        {
            // Update Uri without navigation
            UpdateSourceInternal();
            FireNavigationStarting(args);
        }});

    m_coreSourceChangedRevoker = m_coreWebView.SourceChanged(winrt::auto_revoke, {
        [this](auto const&, winrt::CoreWebView2SourceChangedEventArgs const& args)
        {
            // Update Uri without navigation
            UpdateSourceInternal();
        }});

    m_coreNavigationCompletedRevoker = m_coreWebView.NavigationCompleted(winrt::auto_revoke, {
        [this](auto const&, winrt::CoreWebView2NavigationCompletedEventArgs const& args)
        {
            FireNavigationCompleted(args);
        }});

    m_coreWebMessageReceivedRevoker = m_coreWebView.WebMessageReceived(winrt::auto_revoke, {
        [this](auto const&, winrt::CoreWebView2WebMessageReceivedEventArgs const& args)
        {
            // Fire the MUXC side NavigationCompleted event when the CoreWebView2 event is received.
            FireWebMessageReceived(args);
        }});

    m_coreMoveFocusRequestedRevoker = m_coreWebViewController.MoveFocusRequested(winrt::auto_revoke, {
        [this](auto const&, const winrt::CoreWebView2MoveFocusRequestedEventArgs& args)
        {
            winrt::CoreWebView2MoveFocusReason moveFocusRequestedReason{ args.Reason() };

            if (moveFocusRequestedReason == winrt::CoreWebView2MoveFocusReason::Next ||
                moveFocusRequestedReason == winrt::CoreWebView2MoveFocusReason::Previous)
            {
                winrt::FocusNavigationDirection xamlDirection{ moveFocusRequestedReason == winrt::CoreWebView2MoveFocusReason::Next ?
                                                                   winrt::FocusNavigationDirection::Next : winrt::FocusNavigationDirection::Previous };
                winrt::FindNextElementOptions findNextElementOptions;

                auto contentRoot = [this]()
                {
                    if (auto thisXamlRoot = try_as<winrt::IUIElement10>())
                    {
                        if (auto xamlRoot = thisXamlRoot.XamlRoot())
                        {
                            return xamlRoot.Content();
                        }
                    }
                    return winrt::Window::Current().Content();
                }();

                if (contentRoot)
                {
                    findNextElementOptions.SearchRoot(contentRoot);
                    winrt::DependencyObject nextElement = [=]() {
                        if (SharedHelpers::Is19H1OrHigher())
                        {
                            return winrt::FocusManager::FindNextElement(xamlDirection, findNextElementOptions);
                        }
                        else
                        {
                            return winrt::FocusManager::FindNextElement(xamlDirection);
                        }
                    }();
                    if (nextElement)
                    {
                        // If Anaheim is also losing focus via something other than TAB (web LostFocus event fired)
                        // and the TAB handling is arriving later (eg due to longer MOJO delay), skip manually moving Xaml Focus to next element.
                        if (m_webHasFocus)
                        {
                            const auto _ = [=]() {
                                if (SharedHelpers::Is19H1OrHigher())
                                {
                                    return winrt::FocusManager::TryMoveFocusAsync(xamlDirection, findNextElementOptions);
                                }
                                else
                                {
                                    return winrt::FocusManager::TryMoveFocusAsync(xamlDirection);
                                }
                            }();
                            m_webHasFocus = false;
                        }

                        args.Handled(TRUE);
                    }
                }

                // If nextElement is null, focus is maintained in Anaheim by not marking Handled.
            }
        }});

    m_coreLostFocusRevoker = m_coreWebViewController.LostFocus(winrt::auto_revoke, {
        [this](auto const&, auto const&)
        {
            // Unset our tracking of Anaheim focus when it is lost via something other than TAB navigation.
            m_webHasFocus = false;
        }});

    m_coreProcessFailedRevoker = m_coreWebView.ProcessFailed(winrt::auto_revoke, {
        [this](auto const&, winrt::CoreWebView2ProcessFailedEventArgs const& args)
        {
            winrt::CoreWebView2ProcessFailedKind coreProcessFailedKind{ args.ProcessFailedKind() };
            if (coreProcessFailedKind == winrt::CoreWebView2ProcessFailedKind::BrowserProcessExited)
            {
                m_isCoreFailure_BrowserExited_State = true;

                // CoreWebView2 takes care of clearing the event handlers when closing the host,
                // but we still need to reset the event tokens
                UnregisterCoreEventHandlers();

                // Null these out so we can't try to use them anymore
                m_coreWebViewCompositionController = nullptr;
                m_coreWebViewController = nullptr;
                m_coreWebView = nullptr;
                ResetProperties();
            }

            FireCoreProcessFailedEvent(args);
        }});

    m_cursorChangedRevoker = m_coreWebViewCompositionController.CursorChanged(winrt::auto_revoke, {
        [this](auto const& controller, auto const& obj)
        {
            m_requestedCursor = controller.Cursor();

            UpdateCoreWindowCursor();
        }});
}

winrt::IAsyncAction WebView2::CreateCoreObjects()
{
    MUX_ASSERT((m_isImplicitCreationInProgress && !m_isExplicitCreationInProgress) ||
               (!m_isImplicitCreationInProgress && m_isExplicitCreationInProgress));

    if (m_isClosed)
    {
        throw winrt::hresult_error(RO_E_CLOSED, s_error_wv2_closed);
    }
    else
    {
        m_creationInProgressAsync = std::make_unique<AsyncWebViewOperations>();
        RegisterXamlHandlers();

        // We are about to attempt re-creation of the environment,
        // so clear any previous 'Missing Anaheim Warning'
        Content(nullptr);

        if (!m_isCoreFailure_BrowserExited_State)
        {
            // Normally we always need a new environment, the exception being when Anaheim process failed and we get to reuse the existing one
            co_await CreateCoreEnvironment();
        }

        if (m_coreWebViewEnvironment)
        {
            co_await CreateCoreWebViewFromEnvironment(GetHostHwnd());

            // Do initialization including rendering setup.
            // Try this now but defer to Loaded event if it hasn't fired yet.
            TryCompleteInitialization();
        }
        else if (m_shouldShowMissingAnaheimWarning)
        {
            CreateMissingAnaheimWarning();
        }
    }

   co_return;
}

winrt::IAsyncAction WebView2::CreateCoreEnvironment() noexcept
{
    hstring browserInstall;
    hstring userDataFolder;

    if (!m_options)
    {
        // NOTE: To enable Anaheim logging, add:  --enable-logging=stderr --v=1
        m_options = winrt::CoreWebView2EnvironmentOptions();
        m_options.AdditionalBrowserArguments(L"--enable-features=msEmbeddedBrowserVisualHosting");

        auto applicationLanguagesList = winrt::ApplicationLanguages::Languages();
        if (applicationLanguagesList.Size() > 0)
        {
            m_options.Language(applicationLanguagesList.GetAt(0));
        }
    }

    try
    {
        m_coreWebViewEnvironment = co_await winrt::CoreWebView2Environment::CreateWithOptionsAsync(browserInstall, userDataFolder, m_options);
    }
    catch (winrt::hresult_error e)
    {
        winrt::hresult hr = e.code().value;
        m_shouldShowMissingAnaheimWarning = hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        FireCoreWebView2Initialized(hr);
    }

    co_return;
}

winrt::IAsyncAction WebView2::CreateCoreWebViewFromEnvironment(HWND hwndParent)
{
    if (!hwndParent)
    {
        hwndParent = EnsureTemporaryHostHwnd();
    }

    try
    {
        auto windowRef = winrt::CoreWebView2ControllerWindowReference::CreateFromWindowHandle(reinterpret_cast<UINT64>(hwndParent));
        m_coreWebViewCompositionController = co_await m_coreWebViewEnvironment.CreateCoreWebView2CompositionControllerAsync(windowRef);
        m_coreWebViewController = m_coreWebViewCompositionController.as<winrt::CoreWebView2Controller>();
        m_coreWebViewController.ShouldDetectMonitorScaleChanges(false);
        m_coreWebView = m_coreWebViewController.CoreWebView2();
        m_everHadCoreWebView = true;

        RegisterCoreEventHandlers();
        // Creation is considered complete at this point, however rendering and accessibility
        // hookup will only run after we get Loaded event (see TryCompleteInitialization())
        FireCoreWebView2Initialized(winrt::hresult(S_OK));
    }
    catch (winrt::hresult_error e)
    {
        winrt::hresult hr = e.code().value;
        FireCoreWebView2Initialized(hr);
    }

    m_isImplicitCreationInProgress = false;
    m_isExplicitCreationInProgress = false;
    m_creationInProgressAsync->StopWaiting();
    m_creationInProgressAsync.reset();

    co_return;
}

void WebView2::UpdateDefaultBackgroundColor()
{
    auto appResources = winrt::Application::Current().Resources();
    winrt::IInspectable backgroundColorAsI = (m_accessibilitySettings && m_accessibilitySettings.HighContrast()) ?
        appResources.TryLookup(box_value(L"SystemColorWindowColor")) :
        appResources.TryLookup(box_value(L"SolidBackgroundFillColorBase"));

    winrt::Color backgroundColor = winrt::unbox_value_or<winrt::Color>(backgroundColorAsI, winrt::Color{});
#ifdef WINUI3
    if (m_systemVisualBridge) { m_systemVisualBridge.BackgroundColor(backgroundColor); }
#endif
}

HWND WebView2::EnsureTemporaryHostHwnd()
{
    // If we don't know the parent yet, either use the CoreWindow as the parent,
    // or if we don't have one, create a dummy hwnd to be the temporary parent.
    // Using a dummy parent all the time won't work, since we can't reparent the
    // browser from a Non-ShellManaged Hwnd (dummy) to a ShellManaged one (CoreWindow).
    winrt::CoreWindow coreWindow = winrt::CoreWindow::GetForCurrentThread();
    if (coreWindow)
    {
        auto coreWindowInterop = coreWindow.as<ICoreWindowInterop>();
        winrt::check_hresult(coreWindowInterop->get_WindowHandle(&m_tempHostHwnd));
    }
    else
    {
        // Register the window class.
        const wchar_t CLASS_NAME[] = L"WEBVIEW2_TEMP_PARENT";
        HINSTANCE hInstance = ::GetModuleHandleW(NULL);
        WNDCLASSW wc = { };
        wc.lpfnWndProc = m_fnDefWindowProcW;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        m_fnRegisterClassW(&wc);

        m_tempHostHwnd = m_fnCreateWindowExW(
            0,
            CLASS_NAME,                     // Window class
            L"Webview2 Temporary Parent",   // Window text
            WS_OVERLAPPED,                  // Window style
            0, 0, 0, 0,
            NULL,       // Parent window
            NULL,       // Menu
            hInstance,  // Instance handle
            NULL        // Additional application data
        );
    }
    return m_tempHostHwnd;
}

void WebView2::CreateMissingAnaheimWarning()
{
    auto warning = winrt::TextBlock();
    warning.Text(ResourceAccessor::GetLocalizedStringResource(SR_WarningSuitableWebView2NotFound));
    warning.Inlines().Append(winrt::LineBreak());
    auto linkText = winrt::Run();
    linkText.Text(ResourceAccessor::GetLocalizedStringResource(SR_DownloadWebView2Runtime));
    auto hyperlink = winrt::Hyperlink();
    hyperlink.Inlines().Append(linkText);
    auto url = winrt::Uri(L"https://aka.ms/winui3/webview2download/");
    hyperlink.NavigateUri(url);
    warning.Inlines().Append(hyperlink);
    Content(warning);
}

// We could have a child TextBlock (see CreateMissingAnaheimWarning), make sure it is visited by Measure pass.
winrt::Size WebView2::MeasureOverride(winrt::Size const& availableSize)
{
    //if (auto child = Content().try_as<winrt::FrameworkElement>())
    //{
    //    Content().Measure(availableSize);
    //}

    return __super::MeasureOverride(availableSize);
}

// We could have a child TextBlock (see CreateMissingAnaheimWarning), make sure it is visited by Arrange pass.
winrt::Size WebView2::ArrangeOverride(winrt::Size const& finalSize)
{
    //if (auto child = Content().try_as<winrt::FrameworkElement>())
    //{
    //    Content().Arrange(winrt::Rect{ winrt::Point{0,0}, finalSize });
    //}

    return __super::ArrangeOverride(finalSize);
}

void WebView2::FillPointerPenInfo(const winrt::PointerPoint& inputPt, winrt::CoreWebView2PointerInfo outputPt)
{
    winrt::PointerPointProperties inputProperties{ inputPt.Properties() };
    UINT32 outputPt_penFlags{PEN_FLAG_NONE};

    if (inputProperties.IsBarrelButtonPressed())
    {
        outputPt_penFlags |= PEN_FLAG_BARREL;
    }

    if (inputProperties.IsInverted())
    {
        outputPt_penFlags |= PEN_FLAG_INVERTED;
    }

    if (inputProperties.IsEraser())
    {
        outputPt_penFlags |= PEN_FLAG_ERASER;
    }

    outputPt.PenFlags(outputPt_penFlags);

    UINT32 outputPt_penMask = PEN_MASK_PRESSURE | PEN_MASK_ROTATION | PEN_MASK_TILT_X | PEN_MASK_TILT_Y;
    outputPt.PenMask(outputPt_penMask);

    UINT32 outputPt_penPressure = static_cast<UINT32>(inputProperties.Pressure()* 1024);
    outputPt.PenPressure(outputPt_penPressure);

    UINT32 outputPt_penRotation = static_cast<UINT32>(inputProperties.Twist());
    outputPt.PenRotation(outputPt_penRotation);

    INT32 outputPt_penTiltX = static_cast<INT32>(inputProperties.XTilt());
    outputPt.PenTiltX(outputPt_penTiltX);

    INT32 outputPt_penTiltY = static_cast<INT32>(inputProperties.YTilt());
    outputPt.PenTiltY(outputPt_penTiltY);
}

void WebView2::FillPointerTouchInfo(const winrt::PointerPoint& inputPt, winrt::CoreWebView2PointerInfo outputPt)
{
    winrt::PointerPointProperties inputProperties{ inputPt.Properties() };

    outputPt.TouchFlags(TOUCH_FLAG_NONE);

    UINT32 outputPt_touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
    outputPt.TouchMask(outputPt_touchMask);

    //TOUCH CONTACT
    float width = inputProperties.ContactRect().Width * m_rasterizationScale;
    float height = inputProperties.ContactRect().Height * m_rasterizationScale;
    float leftVal = inputProperties.ContactRect().X * m_rasterizationScale;
    float topVal = inputProperties.ContactRect().Y * m_rasterizationScale;

    winrt::Windows::Foundation::Rect outputPt_touchContact(static_cast<float>(leftVal), static_cast<float>(topVal), static_cast<float>(width), static_cast<float>(height));
    outputPt.TouchContact(outputPt_touchContact);

    //TOUCH CONTACT RAW
    float widthRaw = inputProperties.ContactRectRaw().Width * m_rasterizationScale;
    float heightRaw = inputProperties.ContactRectRaw().Height * m_rasterizationScale;
    float leftValRaw = inputProperties.ContactRectRaw().X * m_rasterizationScale;
    float topValRaw = inputProperties.ContactRectRaw().Y * m_rasterizationScale;

    winrt::Windows::Foundation::Rect outputPt_touchContactRaw(static_cast<float>(leftValRaw), static_cast<float>(topValRaw), static_cast<float>(widthRaw), static_cast<float>(heightRaw));
    outputPt.TouchContactRaw(outputPt_touchContactRaw);

    UINT32 outputPt_touchOrientation = static_cast<UINT32>(inputProperties.Orientation());
    outputPt.TouchOrientation(outputPt_touchOrientation);

    UINT32 outputPt_touchPressure = static_cast<UINT32>(inputProperties.Pressure() * 1024);
    outputPt.TouchPressure(outputPt_touchPressure);
}

void WebView2::FillPointerInfo(const winrt::PointerPoint& inputPt, winrt::CoreWebView2PointerInfo outputPt, const winrt::PointerRoutedEventArgs& args)
{
    winrt::PointerPointProperties inputProperties{ inputPt.Properties() };

    //DEVICE TYPE
    winrt::PointerDeviceType deviceType{ inputPt.PointerDevice().PointerDeviceType() };

    if (deviceType == winrt::PointerDeviceType::Pen)
    {
        outputPt.PointerKind(PT_PEN);
    }
    else if (deviceType == winrt::PointerDeviceType::Touch)
    {
        outputPt.PointerKind(PT_TOUCH);
    }

    outputPt.PointerId(args.Pointer().PointerId());

    outputPt.FrameId(inputPt.FrameId());

    //POINTER FLAGS
    UINT32 outputPt_pointerFlags{POINTER_FLAG_NONE};

    if (inputProperties.IsInRange())
    {
        outputPt_pointerFlags |= POINTER_FLAG_INRANGE;
    }

    if (deviceType == winrt::PointerDeviceType::Touch)
    {
        if (inputPt.IsInContact())
        {
            outputPt_pointerFlags |= POINTER_FLAG_INCONTACT;
            outputPt_pointerFlags |= POINTER_FLAG_FIRSTBUTTON;
        }

        if (inputProperties.PointerUpdateKind() == winrt::PointerUpdateKind::LeftButtonPressed)
        {
            outputPt_pointerFlags |= POINTER_FLAG_NEW;
        }
    }

    if (deviceType == winrt::PointerDeviceType::Pen)
    {
        if (inputPt.IsInContact())
        {
            outputPt_pointerFlags |= POINTER_FLAG_INCONTACT;

            if (!inputProperties.IsBarrelButtonPressed())
            {
                outputPt_pointerFlags |= POINTER_FLAG_FIRSTBUTTON;
            }

            else
            {
                outputPt_pointerFlags |= POINTER_FLAG_SECONDBUTTON;
            }
        } // POINTER_FLAG_NEW is currently omitted for pen input
    }

    if (inputProperties.IsPrimary())
    {
        outputPt_pointerFlags |= POINTER_FLAG_PRIMARY;
    }

    if (inputProperties.TouchConfidence())
    {
        outputPt_pointerFlags |= POINTER_FLAG_CONFIDENCE;
    }

    if (inputProperties.IsCanceled())
    {
        outputPt_pointerFlags |= POINTER_FLAG_CANCELED;
    }

    if (inputProperties.PointerUpdateKind() == winrt::PointerUpdateKind::LeftButtonPressed)
    {
        outputPt_pointerFlags |= POINTER_FLAG_DOWN;
    }

    if (inputProperties.PointerUpdateKind() == winrt::PointerUpdateKind::Other)
    {
        outputPt_pointerFlags |= POINTER_FLAG_UPDATE;
    }

    if (inputProperties.PointerUpdateKind() == winrt::PointerUpdateKind::LeftButtonReleased)
    {
        outputPt_pointerFlags |= POINTER_FLAG_UP;
    }

    outputPt.PointerFlags(outputPt_pointerFlags);

    winrt::Point outputPt_pointerPixelLocation(static_cast<float>(m_rasterizationScale * (inputPt.Position().X)), static_cast<float>(m_rasterizationScale * (inputPt.Position().Y)));
    outputPt.PixelLocation(outputPt_pointerPixelLocation);

    //HIMETRIC LOCATION (task 30544057 exists to finish this)
    //auto himetricScale = 26.4583; //1 hiMetric = 0.037795280352161 PX
    //winrt::Point outputPt_pointerHimetricLocation(static_cast<float>(inputPt.Position().X), static_cast<float>(inputPt.Position().Y));
    //outputPt->HimetricLocation(outputPt_pointerHimetricLocation);

    winrt::Point outputPt_pointerRawPixelLocation(static_cast<float>(m_rasterizationScale * (inputPt.RawPosition().X)), static_cast<float>(m_rasterizationScale * (inputPt.RawPosition().Y)));
    outputPt.PixelLocationRaw(outputPt_pointerRawPixelLocation);

    //RAW HIMETRIC LOCATION
    //winrt::Point outputPt_pointerRawHimetricLocation = { static_cast<float>(inputPt.RawPosition().X), static_cast<float>(inputPt.RawPosition().Y) };
    //outputPt.HimetricLocationRaw(outputPt_pointerRawHimetricLocation);

    UINT32 outputPoint_pointerTime = static_cast<UINT32>(inputPt.Timestamp()/1000); //microsecond to millisecond conversion(for tick count)
    outputPt.Time(outputPoint_pointerTime);

    auto outputPoint_pointerHistoryCount = static_cast<UINT32>(args.GetIntermediatePoints(*this).Size());
    outputPt.HistoryCount(outputPoint_pointerHistoryCount);

    //PERFORMANCE COUNT
    LARGE_INTEGER lpFrequency{};
    bool res = QueryPerformanceFrequency(&lpFrequency);
    if (res)
    {
        auto scale = 1000000;
        auto frequency = lpFrequency.QuadPart;
        auto outputPoint_pointerPerformanceCount = (inputPt.Timestamp() * frequency) / scale;
        outputPt.PerformanceCount(outputPoint_pointerPerformanceCount);
    }

    auto outputPoint_pointerButtonChangeKind = static_cast<INT32>(inputProperties.PointerUpdateKind());
    outputPt.ButtonChangeKind(outputPoint_pointerButtonChangeKind);
}

void WebView2::UpdateCoreWindowCursor()
{
    if (m_isPointerOver)
    {
        winrt::CoreWindow::GetForCurrentThread().PointerCursor(m_requestedCursor);
    }
}

void WebView2::OnXamlPointerMessage(
    UINT message,
    const winrt::PointerRoutedEventArgs& args) noexcept
{
    // Set Handled to prevent ancestor actions such as ScrollViewer taking focus on PointerPressed/PointerReleased.
    args.Handled(true);

    if (!m_coreWebView || !m_coreWebViewCompositionController)
    {
        // returning only because one can click within webview2 element even before it gets loaded
        // in such scenarios, the input gets ignored
        return;
    }

    winrt::PointerPoint logicalPointerPoint{ args.GetCurrentPoint(*this) };
    winrt::Windows::Foundation::Point logicalPoint{ logicalPointerPoint.Position() };
    winrt::Windows::Foundation::Point physicalPoint{ logicalPoint.X * m_rasterizationScale, logicalPoint.Y * m_rasterizationScale };
    winrt::Windows::Devices::Input::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };

   if (deviceType == winrt::Windows::Devices::Input::PointerDeviceType::Mouse)
    {
        if (message == WM_MOUSELEAVE)
        {
            m_coreWebViewCompositionController.SendMouseInput(
                winrt::CoreWebView2MouseEventKind{static_cast<winrt::CoreWebView2MouseEventKind>(message)},
                winrt::CoreWebView2MouseEventVirtualKeys{static_cast<winrt::CoreWebView2MouseEventVirtualKeys>(0)},
                0,
                winrt::Point{0, 0});
        }
        else
        {
            const WPARAM l_param = WebView2Utility::PackIntoWin32StylePointerArgs_lparam(message, args, physicalPoint);
            const LPARAM w_param = WebView2Utility::PackIntoWin32StyleMouseArgs_wparam(message, args, logicalPointerPoint);

            POINT coords_win32;
            POINTSTOPOINT(coords_win32, l_param);
            winrt::Point coords{ static_cast<float>(coords_win32.x), static_cast<float>(coords_win32.y) };

            // mouse data is nonzero for mouse wheel scrolling and XBUTTON events
            DWORD mouse_data = 0;
            if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
            {
                mouse_data = GET_WHEEL_DELTA_WPARAM(w_param);
            }
            else if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP || message == WM_XBUTTONDBLCLK)
            {
                mouse_data = GET_XBUTTON_WPARAM(w_param);
            }

            m_coreWebViewCompositionController.SendMouseInput(
                winrt::CoreWebView2MouseEventKind{ static_cast<winrt::CoreWebView2MouseEventKind>(message) },
                winrt::CoreWebView2MouseEventVirtualKeys{ static_cast<winrt::CoreWebView2MouseEventVirtualKeys>(GET_KEYSTATE_WPARAM(w_param)) },
                mouse_data,
                coords);
        }
    }
    else if ((deviceType == winrt::Windows::Devices::Input::PointerDeviceType::Touch) ||
             (deviceType == winrt::Windows::Devices::Input::PointerDeviceType::Pen))
    {
        const winrt::PointerPoint inputPt{ args.GetCurrentPoint(*this) };
        winrt::CoreWebView2PointerInfo outputPt = m_coreWebViewEnvironment.CreateCoreWebView2PointerInfo();

        //PEN INPUT
        if (deviceType == winrt::PointerDeviceType::Pen)
        {
            FillPointerPenInfo(inputPt, outputPt);
        }

        //TOUCH INPUT
        if (deviceType == winrt::PointerDeviceType::Touch)
        {
            FillPointerTouchInfo(inputPt, outputPt);
        }

        //GENERAL POINTER INPUT
        FillPointerInfo(inputPt, outputPt, args);

        m_coreWebViewCompositionController.SendPointerInput(winrt::CoreWebView2PointerEventKind{ static_cast<winrt::CoreWebView2PointerEventKind>(message) }, outputPt);
    }
}

// The transform is not available in matrix form outside core windows so needed
// information about the transformation needs to be reconstructed by applying
// the transform directly to a known set of points.
// It is assumed that no shear transform is applied and currently rotation is not supported.
winrt::float4x4 WebView2::GetMatrixFromTransform() {
    // Calculate transformation assuming 2D only.
    // Calculate transformed values
    auto generalTransform = TransformToVisual(nullptr);
    winrt::Point initialOrigin = winrt::Point(0, 0);
    winrt::Point translatedOrigin = generalTransform.TransformPoint(initialOrigin);

    winrt::float4x4 outputMatrix{};

    // Assign rotation
    outputMatrix.m12 = 0.0f;
    outputMatrix.m13 = 0.0f;
    outputMatrix.m21 = 0.0f;
    outputMatrix.m23 = 0.0f;
    outputMatrix.m31 = 0.0f;
    outputMatrix.m32 = 0.0f;

    // Assign offsets/translation
    // This should be the global physical pixel offset to the top left corner of the XAML HWND.
    outputMatrix.m41 = (translatedOrigin.X * m_rasterizationScale); // X offset
    outputMatrix.m42 = (translatedOrigin.Y * m_rasterizationScale); // Y offset
    outputMatrix.m43 = 0.0f; // Z offset

    // Assign scale values
    // These values will just be 1.0 because Anaheim is getting their values in physical pixels,
    // so they don't need to do any extra unscaling.
    outputMatrix.m11 = 1.0f; // X Scale
    outputMatrix.m22 = 1.0f; // Y Scale
    outputMatrix.m33 = 1.0f; // Z scale

    // Set to 0 (3D coordinate transform values)
    outputMatrix.m14 = 0.0f;
    outputMatrix.m24 = 0.0f;
    outputMatrix.m34 = 0.0f;
    // Set to 1 to maintain non-zero det.
    outputMatrix.m44 = 1.0f;

    return outputMatrix;
}

void WebView2::ResetMouseInputState()
{
    m_isLeftMouseButtonPressed = false;
    m_isMiddleMouseButtonPressed = false;
    m_isRightMouseButtonPressed = false;
    m_isXButton1Pressed = false;
    m_isXButton2Pressed = false;
}

void WebView2::FireNavigationStarting(const winrt::CoreWebView2NavigationStartingEventArgs& args)
{
    // Fire new event on MUX side
    m_navigationStartingEventSource(*this, args);
}

// Event fired when navigation has occurred in the WebView2.
void WebView2::FireNavigationCompleted(const winrt::CoreWebView2NavigationCompletedEventArgs& args)
{
    // UpdateSource to keep coherence between WebView2 and CoreWebView2.
    UpdateSourceInternal();

    // Fire new event on MUXC side
    m_navigationCompletedEventSource(*this, args);
}

// MUXC side event for WebMessageReceived
// Registering for this event will allow for a notification to be received from an Anaheim side script.
void WebView2::FireWebMessageReceived(const winrt::CoreWebView2WebMessageReceivedEventArgs& args)
{
    m_webMessageReceivedEventSource(*this, args);
}

// Updates the Source without navigating when the NavigationStarting event, SourceChanged event,
// or NavigationCompleted events are fired from the core webview.
void WebView2::UpdateSourceInternal()
{
    // Update Source to keep coherence between WebView2 and CoreWebView2.
    winrt::hstring newUri{m_coreWebView.Source()};
    m_stopNavigateOnUriChanged = newUri;
    Source(winrt::Uri(newUri));
    m_stopNavigateOnUriChanged.clear();

    // Update CanGoBack/Forward to be consistent with core webview post navigation.
    if (m_coreWebView)
    {
        SetCanGoBack(m_coreWebView.CanGoBack());
        SetCanGoForward(m_coreWebView.CanGoForward());
    }
}

winrt::CoreWebView2 WebView2::CoreWebView2()
{
    return m_coreWebView;
}

void WebView2::FireCoreProcessFailedEvent(const winrt::CoreWebView2ProcessFailedEventArgs& args)
{
    // Fire new event on MUXC side
    m_coreProcessFailedEventSource(*this, args);
}

void WebView2::FireCoreWebView2Initialized(winrt::hresult exception)
{
    auto eventArgs = winrt::make_self<CoreWebView2InitializedEventArgs>(exception);
    m_coreWebView2InitializedEventSource(*this, *eventArgs);
}

void WebView2::HandleGotFocus(const winrt::Windows::Foundation::IInspectable&, const winrt::RoutedEventArgs&) noexcept
{
    if (m_coreWebView && m_xamlFocusChangeInfo.m_isPending)
    {
        // In current CoreWebView2 API, MoveFocus is not expected to fail, so set m_webHasFocus here rather than
        // in asynchronous (MOJO/cross-proc) CoreWebView2 GotFocus event.
        m_webHasFocus = true;
        m_coreWebViewController.MoveFocus(m_xamlFocusChangeInfo.m_storedMoveFocusReason);
        m_xamlFocusChangeInfo.m_isPending = false;
    }
}

void WebView2::HandleGettingFocus(const winrt::Windows::Foundation::IInspectable&, const winrt::GettingFocusEventArgs& args) noexcept
{
    if (m_coreWebView)
    {
        winrt::CoreWebView2MoveFocusReason moveFocusReason{winrt::CoreWebView2MoveFocusReason::Programmatic};

        if (args.InputDevice() == winrt::FocusInputDeviceKind::Keyboard)
        {
            if (args.Direction() == winrt::FocusNavigationDirection::Next)
            {
                moveFocusReason = winrt::CoreWebView2MoveFocusReason::Next;
            }
            else if (args.Direction() == winrt::FocusNavigationDirection::Previous)
            {
                moveFocusReason = winrt::CoreWebView2MoveFocusReason::Previous;
            }
        }

        m_xamlFocusChangeInfo.m_storedMoveFocusReason = moveFocusReason;
        m_xamlFocusChangeInfo.m_isPending = true;
    }
}


// Since WebView takes HWND focus (via OnGotFocus -> MoveFocus) Xaml assumes
// focus was lost for an external reason. When the next unhandled TAB KeyDown
// reaches the XamlRoot element, Xaml's FocusManager will try to move focus to the next
// Xaml control and force HWND focus back to itself, popping Xaml focus out of the
// WebView2 control. We mark TAB handled in our KeyDown handler so that it is ignored
// by XamlRoot's tab processing.
void WebView2::HandleKeyDown(const winrt::Windows::Foundation::IInspectable&, const winrt::KeyRoutedEventArgs& e)
{
    if (e.Key() == winrt::VirtualKey::Tab)
    {
        e.Handled(true);
    }
}

// When Win32 HWND focus is switched to InputWindow, VK_TAB's processed by Xaml's CoreWindow
// hosting accelerator key handling do not get dispatched to the child InputWindow.
// Send CoreWebView2 the missing Tab/KeyDown so that tab handling occurs in Anaheim.
void WebView2::HandleAcceleratorKeyActivated(const winrt::Windows::UI::Core::CoreDispatcher&, const winrt::AcceleratorKeyEventArgs& args) noexcept
{
    if (args.VirtualKey() == winrt::VirtualKey::Tab &&
        args.EventType() == winrt::CoreAcceleratorKeyEventType::KeyDown &&
        m_webHasFocus &&
        args.Handled())
    {
        const UINT message = WM_KEYDOWN;
        const WPARAM wparam = VK_TAB;
        const LPARAM lparam = MAKELPARAM(0x0001, 0x000f);  // flags copied from matching WM_KEYDOWN

        LRESULT result = m_fnSendMessageW(GetActiveInputWindowHwnd(), message, wparam, lparam);
        if (!result)
        {
            winrt::check_hresult(HRESULT_FROM_WIN32(::GetLastError()));
        }
    }
}

void WebView2::RegisterXamlHandlers()
{
    m_gettingFocusRevoker = GettingFocus(winrt::auto_revoke, { this, &WebView2::HandleGettingFocus });
    m_gotFocusRevoker = GotFocus(winrt::auto_revoke, { this, &WebView2::HandleGotFocus });
    
    m_pointerPressedRevoker = PointerPressed(winrt::auto_revoke, { this, &WebView2::HandlePointerPressed });
    m_pointerReleasedRevoker = PointerReleased(winrt::auto_revoke, { this, &WebView2::HandlePointerReleased });
    m_pointerMovedRevoker = PointerMoved(winrt::auto_revoke, { this, &WebView2::HandlePointerMoved });
    m_pointerWheelChangedRevoker = PointerWheelChanged(winrt::auto_revoke, { this, &WebView2::HandlePointerWheelChanged });
    m_pointerExitedRevoker = PointerExited(winrt::auto_revoke, { this, &WebView2::HandlePointerExited });
    m_pointerEnteredRevoker = PointerEntered(winrt::auto_revoke, { this, &WebView2::HandlePointerEntered });
    m_pointerCanceledRevoker = PointerCanceled(winrt::auto_revoke, { this, &WebView2::HandlePointerCanceled });
    m_pointerCaptureLostRevoker = PointerCaptureLost(winrt::auto_revoke, { this, &WebView2::HandlePointerCaptureLost });
    m_keyDownRevoker = KeyDown(winrt::auto_revoke, { this, &WebView2::HandleKeyDown });

    // TODO: We do not have direct analogue for AcceleratorKeyActivated with DispatcherQueue in Islands/ win32. Please refer Task# 30013704 for  more details.
    if (auto coreWindow = winrt::CoreWindow::GetForCurrentThread())
    {
        m_acceleratorKeyActivatedRevoker = Dispatcher().AcceleratorKeyActivated(winrt::auto_revoke, { this, &WebView2::HandleAcceleratorKeyActivated });
    }

    m_sizeChangedRevoker = SizeChanged(winrt::auto_revoke, { this, &WebView2::HandleSizeChanged });
}

winrt::AutomationPeer WebView2::OnCreateAutomationPeer()
{
    return winrt::make<WebView2AutomationPeer>(*this);
}

winrt::IUnknown WebView2::GetWebView2Provider()
{
    winrt::com_ptr<IUnknown> provider;
    if (m_coreWebViewCompositionController)
    {
        auto coreWebView2CompositionControllerInterop = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop>();
        winrt::check_hresult(coreWebView2CompositionControllerInterop->get_UIAProvider(provider.put()));
    }
    return provider.as<winrt::IUnknown>();
}

winrt::IUnknown WebView2::GetProviderForHwnd(HWND hwnd)
{
    winrt::com_ptr<IUnknown> provider;
    if (m_coreWebViewEnvironment)
    {
        // If there is no provider for the given HWND, CoreWebview2 will return UIA_E_ELEMENTNOTAVAILABLE,
        // and we should just return an empty provider
        auto coreWebView2EnvironmentInterop = m_coreWebViewEnvironment.as<ICoreWebView2EnvironmentInterop>();
        winrt::hresult hr = coreWebView2EnvironmentInterop->GetProviderForHwnd(hwnd, provider.put());
        if (hr != UIA_E_ELEMENTNOTAVAILABLE)
        {
            winrt::check_hresult(hr);
        }
    }
    return provider.as<winrt::IUnknown>();
}

winrt::IAsyncAction WebView2::EnsureCoreWebView2Async()
{
    // If CWV2 exists already, return immediately/synchronously
    if (m_coreWebView)
    {
        MUX_ASSERT(m_coreWebViewEnvironment && m_coreWebViewController);
        co_return;
    }

    // If CWV2 is being created, return when the pending creation completes
    if (m_creationInProgressAsync)
    {
        MUX_ASSERT(m_isExplicitCreationInProgress || m_isImplicitCreationInProgress);
        co_await *m_creationInProgressAsync;
    }

    // Otherwise, kick off a new CWV2 creation
    else
    {
        m_isExplicitCreationInProgress = true;
        co_await CreateCoreObjects();
    }

    MUX_ASSERT(!m_isImplicitCreationInProgress && !m_isExplicitCreationInProgress);
    co_return;
}

void WebView2::DisconnectFromRootVisualTarget()
{
    if (m_coreWebViewCompositionController)
    {
        auto coreWebView2CompositionControllerInterop = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop>();
        winrt::check_hresult(coreWebView2CompositionControllerInterop->put_RootVisualTarget(nullptr));
    }
}

void WebView2::TryCompleteInitialization()
{
    // If proper Anaheim not present, no further initialization is necessary
    if (m_shouldShowMissingAnaheimWarning)
    {
        return;
    }

    // If called directly after CWV2 creation completes, it's possible Xaml WV2 Loaded event did not yet fire.
    // Skip  work for now - this will be called again when WV2 Loaded fires.
    if (!SafeIsLoaded())
    {
        return;
    }

    // If called from Loaded handler, it's possible CWV2 is not ready yet.
    // Skip work for now - this will be called again when CWV2 is ready.
    if (!m_coreWebView)
    {
        return;
    }

    HWND prevParentWindow = m_xamlHostHwnd;
    m_xamlHostHwnd = nullptr;
    HWND newParentWindow = GetHostHwnd();
    UpdateParentWindow(newParentWindow);

    XamlRootChangedHelper(true /* forceUpdate */);
    if (auto thisXamlRoot = try_as<winrt::IUIElement10>())
    {
        auto xamlRoot = thisXamlRoot.XamlRoot();
        m_xamlRootChangedRevoker = RegisterXamlRootChanged(xamlRoot,
            [this](auto const& /*sender*/, auto const& /*args*/)
        {
            HandleXamlRootChanged();
        });
    }
    else
    {
        m_windowVisibilityChangedRevoker = winrt::Window::Current().VisibilityChanged(winrt::auto_revoke,
            [this](auto&&...)
        {
            HandleXamlRootChanged();
        });
    }


    if (!m_actualThemeChangedRevoker)
    {
        m_actualThemeChangedRevoker = this->ActualThemeChanged(winrt::auto_revoke,
            [this](auto&&, auto&&) { UpdateDefaultBackgroundColor(); });
    }

    if (!m_accessibilitySettings)
    {
        m_accessibilitySettings = winrt::AccessibilitySettings();
    }

    // TODO_WebView2: Currently, AccessibilitySettings.HighContrastChanged does not work without a core window, and throws
    // an ElementNotFound exception. VS default behavior is to break on exceptions, regardless of whether we catch it below.
    // This is not a good experience for developers, since this happens on every WebView2 creation, and it feels like an error
    // even though it's expected.
    // We should avoid this altogether on desktop by not registering for the HighContrastChanged event, since for now it
    // will never be raised. Once Task #24777629 is fixed, we can remove the coreWindow check.
    winrt::CoreWindow coreWindow = winrt::CoreWindow::GetForCurrentThread();
    if (!m_highContrastChangedRevoker && coreWindow)
    {
        m_highContrastChangedRevoker = m_accessibilitySettings.HighContrastChanged(winrt::auto_revoke,
            [this](auto&&, auto&&) { UpdateDefaultBackgroundColor(); });
    }

#ifdef WINUI3
    if (!m_systemVisualBridge)
    {
        winrt::Compositor compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();
        m_systemVisualBridge = winrt::ExpSystemVisualBridge::Create(compositor);
    }
#else
    if (!m_visual)
    {
        m_visual = winrt::Window::Current().Compositor().CreateSpriteVisual();
    }
#endif
    UpdateDefaultBackgroundColor();

    SetCoreWebViewAndVisualSize(static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight()));
#ifdef WINUI3
    winrt::ElementCompositionPreview::SetElementChildVisual(*this, m_systemVisualBridge.BridgeVisual());
#else
    winrt::ElementCompositionPreview::SetElementChildVisual(*this, m_visual);
#endif

#ifdef WINUI3
    winrt::com_ptr<IDCompositionTarget> sharedTarget = m_systemVisualBridge.as<IDCompositionTarget>();
    auto coreWebView2CompositionControllerInterop = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop>();
    winrt::check_hresult(coreWebView2CompositionControllerInterop->put_RootVisualTarget(sharedTarget.get()));
#else
    auto coreWebView2CompositionControllerInterop = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop>();
    winrt::check_hresult(coreWebView2CompositionControllerInterop->put_RootVisualTarget(m_visual.as<::IUnknown>().get()));
#endif


    // If we were recreating the webview after a core process failure, indicate that we have now recovered
    m_isCoreFailure_BrowserExited_State = false;
}

winrt::IAsyncOperation<winrt::hstring> WebView2::ExecuteScriptAsync(winrt::hstring javascriptCode)
{
    // TODO_WebView2: Consider recreating the core webview here if its process had failed,
    // allowing the app to use this API to recover from the error.

    winrt::hstring returnedValue{}; // initialized with default value
    if (m_coreWebView)
    {
        returnedValue = co_await m_coreWebView.ExecuteScriptAsync(javascriptCode);
    }
    else
    {
        throw winrt::hresult_illegal_method_call(std::wstring(L"ExecuteScriptAsync(): ").append(s_error_cwv2_not_present));
    }

    co_return returnedValue;
}

void WebView2::UpdateParentWindow(HWND newParentWindow)
{
    if (m_tempHostHwnd && m_coreWebViewController)
    {
        auto windowRef = winrt::CoreWebView2ControllerWindowReference::CreateFromWindowHandle(reinterpret_cast<UINT64>(newParentWindow));

        // Reparent webview host
        m_coreWebViewController.ParentWindow(windowRef);
        // TODO_WebView2Islands: currently m_tempHostHwnd is always the CoreWindow and as such
        // does not need to be destroyed, but it will in the future if it's a dummy window.
        m_tempHostHwnd = nullptr;
    }
}

bool WebView2::SafeIsLoaded()
{
    if (auto loaded = try_as<winrt::IFrameworkElement7>())
    {
        return loaded.IsLoaded();
    }

    return m_loaded;
}

void WebView2::OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    m_loaded = true; // Track for downlevel OS

    // OnLoaded and OnUnloaded are fired from XAML asynchronously, and unfortunately they could
    // be out of order.  If the element is removed from tree A and added to tree B, we could get
    // a Loaded event for tree B *before* we see the Unloaded event for tree A.  To handle this:
    //  * When we get a Loaded/Unloaded event, check the IsLoaded property. If it doesn't match
    //      the event we're in, nothing needs to be done since the other handler took care of it.
    //  * When we see a Loaded event when we have been or are already loaded, remove the
    //      XamlRootChanged event handler for the old tree and bind to the new one.

    // If we're not loaded, there's nothing for us to do since Unloaded took care of everything
    if (!SafeIsLoaded())
    {
        return;
    }

    TryCompleteInitialization();

    if (winrt::VisualTreeHelper::GetChildrenCount(*this) > 0)
    {
        if (auto contentPresenter = winrt::VisualTreeHelper::GetChild(*this, 0).try_as<winrt::ContentPresenter>())
        {
            contentPresenter.Background(Background());
            contentPresenter.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
            contentPresenter.VerticalAlignment(winrt::VerticalAlignment::Stretch);
            contentPresenter.HorizontalContentAlignment(winrt::HorizontalAlignment::Stretch);
            contentPresenter.VerticalContentAlignment(winrt::VerticalAlignment::Stretch);
        }
    }
}

void WebView2::OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    m_loaded = false;

    // Note that we always check the IsLoaded property before doing unloading actions, since we
    // could get an Unloaded event for an old tree A *after* a Loaded event for a new tree B.
    // See comment in WebView2::OnLoaded
    if (SafeIsLoaded())
    {
        return;
    }

    UpdateRenderedSubscriptionAndVisibility();

    m_xamlRootChangedRevoker.revoke();
    m_windowVisibilityChangedRevoker.revoke();

    DisconnectFromRootVisualTarget();
}

void WebView2::Reload()
{
    if (m_coreWebView)
    {
        m_coreWebView.Reload();
    }
    else
    {
        if (m_everHadCoreWebView)
        {
            throw winrt::hresult_illegal_method_call(std::wstring(L"Reload(): ").append(s_error_cwv2_not_present_closed));
        }
        else
        {
            throw winrt::hresult_illegal_method_call(std::wstring(L"Reload(): ").append(s_error_cwv2_not_present));
        }
    }
}

void WebView2::NavigateToString(winrt::hstring htmlContent)
{
    if (m_coreWebView)
    {
        m_coreWebView.NavigateToString(htmlContent);
    }
    else
    {
        throw winrt::hresult_illegal_method_call(std::wstring(L"NavigateToString(): ").append(s_error_cwv2_not_present));
    }
}

void WebView2::GoBack()
{
    if (m_coreWebView && CanGoBack())
    {
        m_coreWebView.GoBack();
    }
}

void WebView2::GoForward()
{
    if (m_coreWebView && CanGoForward())
    {
        m_coreWebView.GoForward();
    }
}

void WebView2::XamlRootChangedHelper(bool forceUpdate)
{
    auto [scale, hostVisibility] = [this]() {
        if (auto thisXamlRoot = try_as<winrt::IUIElement10>())
        {
            auto xamlRoot = thisXamlRoot.XamlRoot();
            if (xamlRoot)
            {
                auto scale = static_cast<float>(xamlRoot.RasterizationScale());
                bool hostVisibility = xamlRoot.IsHostVisible();

                return std::make_tuple(scale, hostVisibility);
            }
        }

        auto rawPixelsPerViewPixel = winrt::DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel();

        return std::make_tuple((float)rawPixelsPerViewPixel, winrt::Window::Current().Visible());
    }();


    if (forceUpdate || (scale != m_rasterizationScale))
    {
        m_rasterizationScale = scale;
        m_isHostVisible = hostVisibility; // If we did forceUpdate we'll want to update host visibility here too
        if (m_coreWebViewController)
        {
            m_coreWebViewController.RasterizationScale(static_cast<double>(scale));
        }
        SetCoreWebViewAndVisualSize(static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight()));
        CheckAndUpdateWebViewPosition();
        UpdateRenderedSubscriptionAndVisibility();
    }
    else if (hostVisibility != m_isHostVisible)
    {
        m_isHostVisible = hostVisibility;
        CheckAndUpdateVisibility();
    }
}

void WebView2::HandleXamlRootChanged()
{
    XamlRootChangedHelper(false /* forceUpdate */);
}

void WebView2::HandleSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& args)
{
    SetCoreWebViewAndVisualSize(args.NewSize().Width, args.NewSize().Height);
}

void WebView2::HandleRendered(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    if (m_coreWebView)
    {
        // Check if the position of the WebView inside the app has changed
        CheckAndUpdateWebViewPosition();
        // Check if the position of the window itself has changed
        CheckAndUpdateWindowPosition();
        // Check if the visibility property of a parent element has changed
        CheckAndUpdateVisibility();
    }
}

void WebView2::CheckAndUpdateWebViewPosition()
{
    if (!m_coreWebViewController) return;

    // Check if the position of the WebView within the window has changed
    auto transform = TransformToVisual(nullptr);
    auto topLeft = transform.TransformPoint(winrt::Point(0, 0));

    auto scaledTopLeftX = ceil(topLeft.X * m_rasterizationScale);
    auto scaledTopLeftY = ceil(topLeft.Y * m_rasterizationScale);

    if (scaledTopLeftX != m_webViewScaledPosition.X || scaledTopLeftY != m_webViewScaledPosition.Y)
    {
        m_webViewScaledPosition.X = scaledTopLeftX;
        m_webViewScaledPosition.Y = scaledTopLeftY;
    }

    m_webViewScaledSize.X = ceil(static_cast<float>(ActualWidth()) * m_rasterizationScale);
    m_webViewScaledSize.Y = ceil(static_cast<float>(ActualHeight()) * m_rasterizationScale);

    // We create the Bounds using X, Y, width, and height
    m_coreWebViewController.Bounds({
        (m_webViewScaledPosition.X),
        (m_webViewScaledPosition.Y),
        (m_webViewScaledSize.X),
        (m_webViewScaledSize.Y) });
}

void WebView2::SetCoreWebViewAndVisualSize(const float width, const float height)
{
#ifdef WINUI3
    if (!m_coreWebView && !m_systemVisualBridge) return;
#endif

    if (m_coreWebView)
    {
        CheckAndUpdateWebViewPosition();
    }

    // The CoreWebView2 visuals hosted under the bridge visual are already scaled for the rasterization scale.
    // To keep them from being scaled again from the scale above the WebView2 element, we need to apply
    // an inverse scale on the bridge visual. Since the inverse scale will reduce the size of the bridge visual, we
    // need to scale up the size by the rasterization scale to compensate.

#ifdef WINUI3
    if (m_systemVisualBridge)
    {
        winrt::Visual systemVisualBridgeVisual = m_systemVisualBridge.BridgeVisual();
        winrt::float2 newSize = winrt::float2(width * m_rasterizationScale, height * m_rasterizationScale);
        winrt::float3 newScale = winrt::float3(1.0f/m_rasterizationScale, 1.0f/m_rasterizationScale, 1.0);

        systemVisualBridgeVisual.Size(newSize);
        systemVisualBridgeVisual.Scale(newScale);
    }
#else
    if (m_visual)
    {
        winrt::float2 newSize = winrt::float2(width * m_rasterizationScale, height * m_rasterizationScale);
        winrt::float3 newScale = winrt::float3(1.0f / m_rasterizationScale, 1.0f / m_rasterizationScale, 1.0);

        m_visual.Size(newSize);
        m_visual.Scale(newScale);
    }
#endif
}

void WebView2::CheckAndUpdateWindowPosition()
{
    auto hostWindow = GetHostHwnd();
    if (!hostWindow)
    {
        return;
    }

    POINT windowPosition = { 0, 0 };
    m_fnClientToScreen(hostWindow, &windowPosition);
    if (m_hostWindowPosition.x != windowPosition.x || m_hostWindowPosition.y != windowPosition.y)
    {
        m_hostWindowPosition.x = windowPosition.x;
        m_hostWindowPosition.y = windowPosition.y;

        if (m_coreWebViewController)
        {
            m_coreWebViewController.NotifyParentWindowPositionChanged();
        }
    }
}

void WebView2::CheckAndUpdateVisibility(bool force)
{
    // Keep booleans in this order to prevent doing expensive tree walk if we don't have to.
    bool currentVisibility = this->Visibility() == winrt::Visibility::Visible &&
                             SafeIsLoaded() &&
                             this->m_isHostVisible &&
                             AreAllAncestorsVisible();
    if (m_isVisible != currentVisibility || force)
    {
        m_isVisible = currentVisibility;

        UpdateCoreWebViewVisibility();
    }
}

// When we hide the CoreWebView too early, we would see a flash caused by SystemVisualBridge's BackgroundColor being displayed.
// To resolve this, we delay the call to hide CoreWV if the WebView is being hidden. 
void WebView2::UpdateCoreWebViewVisibility()
{
    auto strongThis = get_strong();
    auto updateCoreWebViewVisibilityAction = [strongThis]()
    {
        if (strongThis->m_coreWebViewController)
        {
            strongThis->m_coreWebViewController.IsVisible(strongThis->m_isVisible);
        }
    };

    if (!m_isVisible && this->m_isHostVisible)
    {
        SharedHelpers::ScheduleActionAfterWait(updateCoreWebViewVisibilityAction, 200);
    }
    else
    {
        if (m_coreWebViewController)
        {
            m_coreWebViewController.IsVisible(m_isVisible);
        }
    }
}

bool WebView2::AreAllAncestorsVisible()
{
    bool allAncestorsVisible = true;
    winrt::DependencyObject parentAsDO = this->Parent().as<winrt::DependencyObject>();
    while (parentAsDO != nullptr)
    {
        winrt::IUIElement parentAsUIE = parentAsDO.as<winrt::IUIElement>();
        winrt::Visibility parentVisibility = parentAsUIE.Visibility();
        if  (parentVisibility == winrt::Visibility::Collapsed)
        {
            allAncestorsVisible = false;
            break;
        }
        parentAsDO = winrt::VisualTreeHelper::GetParent(parentAsDO);
    }

    return allAncestorsVisible;
}

void WebView2::UpdateRenderedSubscriptionAndVisibility()
{
    // The Rendered subscription is turned off for better performance when this element is hidden, or not loaded.
    // However, when this element is effectively hidden due to an ancestor being hidden, we should still subscribe --
    // otherwise, if the ancestor becomes visible again, we won't have the check in HandleRendered to inform us.
    if (SafeIsLoaded() && this->Visibility() == winrt::Visibility::Visible)
    {
        if (SharedHelpers::IsRS4OrHigher())
        {
            if (!m_renderedRevoker)
            {
                m_renderedRevoker = winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendered(
                    winrt::auto_revoke, { this, &WebView2::HandleRendered });
            }
        }
        else
        {
            if (!m_layoutUpdatedRevoker)
            {
                m_layoutUpdatedRevoker = LayoutUpdated(winrt::auto_revoke, [this](auto&&...) {
                    HandleRendered(nullptr, nullptr);
                });
            }
        }
    }
    else
    {
        m_renderedRevoker.revoke();
        m_layoutUpdatedRevoker.revoke();
    }
    CheckAndUpdateVisibility(true);
}

void WebView2::SetCanGoForward(bool canGoForward)
{
    m_canGoPropSetInternally = true;
    auto canGoPropSetInternally = gsl::finally([this]()
    {
        m_canGoPropSetInternally = false;
    });

    CanGoForward(canGoForward);
}

void WebView2::SetCanGoBack(bool canGoBack)
{
    m_canGoPropSetInternally = true;
    auto canGoPropSetInternally = gsl::finally([this]()
    {
        m_canGoPropSetInternally = false;
    });

    CanGoBack(canGoBack);
}

// Do not reset Source property as it be useful to app developer for restoring state
void WebView2::ResetProperties()
{
    SetCanGoForward(false);
    SetCanGoBack(false);
}
