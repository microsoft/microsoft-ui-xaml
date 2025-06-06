﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include <corewindow.h>
#include "dcomp.h"
#include "InvokableCallbackHelper.h"
#include "math.h"
#include "Microsoft.UI.Xaml.xamlroot.h"
#include "ResourceAccessor.h"
#include "RuntimeProfiler.h"
#include "UIAutomationCore.h"
#include "UIAutomationCoreApi.h"
#include "WebView2.h"
#include "WebView2AutomationPeer.h"
#include <WebView2Interop.h>
#include "WebView2Utility.h"
#include "Windows.Globalization.h"
#include <wrl\event.h>
#include "MuxcTraceLogging.h"

using namespace Microsoft::WRL;

static constexpr wstring_view s_error_wv2_closed{ L"Cannot create CoreWebView2 for Closed WebView2 element."sv };
static constexpr wstring_view s_error_cwv2_not_present{ L"Failed because a valid CoreWebView2 is not present. Make sure one was created, for example by calling EnsureCoreWebView2Async() API."sv };

// White matches CoreWebView2Controller.DefaultBackgroundColor
const winrt::Color WebView2::sc_controllerDefaultBackgroundColor{ 255, 255, 255, 255 };

// When the Xaml window is closed (eg by user clicking 'x'), CoreWebView2
// (associated with child "Input HWND") is notified ahead of Xaml WebView2
// resulting in a window of time when calls into the CoreWebView2 APIs
// fail with ERROR_INVALID_STATE. The helper functions below are available
// to detect and gracefully handle this situation.

// Checks CoreWebView2Controller status, use at start of simple/short functions
bool WebView2::CoreWebView2HasInvalidState()
{
    if (m_coreWebViewController)
    {
        try { bool isVisible = m_coreWebViewController.IsVisible(); }
        catch (winrt::hresult_error e)
        {
            if (e.code().value == HRESULT_FROM_WIN32(ERROR_INVALID_STATE))
            {
                return true;
            }
        }
    }
    return false;
}

// Wraps calls to Core Objects and swallows ERROR_INVALID_STATE, use around synchronous CWV2 API calls made frequently
// (Note: for async CWV2 calls needing the check, use below code directly for simplicity)
template <typename Delegate>
void WebView2::CoreWebView2RunIgnoreInvalidStateSync(Delegate delegate)
{
    try
    {
        delegate();
    }
    catch (winrt::hresult_error e)
    {
        if (e.code().value != HRESULT_FROM_WIN32(ERROR_INVALID_STATE))
        {
            throw;
        }
    }
}


WebView2::WebView2()
{
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
}

void WebView2::OnVisibilityPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/)
{
    UpdateRenderedSubscriptionAndVisibility();
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
    m_renderedRevoker.revoke();

    if (m_tempHostHwnd && !winrt::CoreWindow::GetForCurrentThread())
    {
        DestroyWindow(m_tempHostHwnd);
        m_tempHostHwnd = nullptr;
    }

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

    UnregisterXamlEventHandlers();

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
    else if (property == s_DefaultBackgroundColorProperty)
    {
        SetControllerDefaultBackgroundColor();
    }
}

void WebView2::HandlePointerPressed(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    // Chromium handles WM_MOUSEXXX for mouse, WM_POINTERXXX for touch
    winrt::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };
    winrt::Microsoft::UI::Input::PointerPoint pointerPoint{ args.GetCurrentPoint(*this) };
    UINT message = 0x0;

    if (deviceType == winrt::PointerDeviceType::Mouse)
    {
        // WebView takes mouse capture to avoid missing pointer released events that occur outside of the element that
        // end pointer pressed state inside the webview. Example, scrollbar is being used and mouse is moved out
        // of webview bounds before being released, the webview will miss the released event and upon reentry into
        // the webview, the mouse will still cause the scrollbar to move as if selected.
        m_hasMouseCapture = this->CapturePointer(args.Pointer());

        winrt::PointerPointProperties properties{ pointerPoint.Properties() };
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
    winrt::PointerPoint pointerPoint{ args.GetCurrentPoint(*this) };
    winrt::PointerPointProperties properties{ pointerPoint.Properties() };
    UINT message;

    if (deviceType == winrt::PointerDeviceType::Mouse)
    {
        message = properties.IsHorizontalMouseWheel() ? WM_MOUSEHWHEEL : WM_MOUSEWHEEL;
    }
    else
    {
        message = WM_POINTERWHEEL;
    }
    OnXamlPointerMessage(message, args);
}

void WebView2::HandlePointerExited(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    winrt::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };
    UINT message;

    if (deviceType == winrt::PointerDeviceType::Mouse)
    {
        if (m_hasMouseCapture)
        {
            // WM_MOUSELEAVE should not fire while we have capture, since doing so
            // will cancel capture in the WebView.
            m_isMouseLeaveNeeded = true;
            return;
        }

        message = WM_MOUSELEAVE;
        ResetMouseInputState();
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

    if (deviceType != winrt::PointerDeviceType::Mouse) //mouse does not have an equivalent pointer_entered event, so only handling pen/touch
    {
        OnXamlPointerMessage(WM_POINTERENTER, args);
    }
    else
    {
        MUX_ASSERT(deviceType == winrt::PointerDeviceType::Mouse);
        // If there was a queued MouseLeave, clear it now that the mouse has re-entered.
        m_isMouseLeaveNeeded = false;
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

    if (deviceType == winrt::PointerDeviceType::Mouse)
    {
        m_hasMouseCapture = false;
        ResetMouseInputState();
        if (m_isMouseLeaveNeeded)
        {
            m_isMouseLeaveNeeded = false;
            SendMouseLeave();
        }
    }
    else if (deviceType == winrt::PointerDeviceType::Touch)
    {
        // Get pointer id for handling multi-touch capture
        winrt::Microsoft::UI::Input::PointerPoint logicalPointerPoint{ args.GetCurrentPoint(*this) };
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

void WebView2::SendMouseLeave()
{
    if (!m_coreWebView || !m_coreWebViewCompositionController || CoreWebView2HasInvalidState())
    {
        // nothing to forward input to
        return;
    }

    m_coreWebViewCompositionController.SendMouseInput(
        winrt::CoreWebView2MouseEventKind{ static_cast<winrt::CoreWebView2MouseEventKind>(WM_MOUSELEAVE) },
        winrt::CoreWebView2MouseEventVirtualKeys{ static_cast<winrt::CoreWebView2MouseEventVirtualKeys>(0) },
        0,
        winrt::Point{ 0, 0 });
}

bool WebView2::ShouldNavigate(const winrt::Uri& uri)
{
    return uri != nullptr && uri.RawUri() != m_stopNavigateOnUriChanged;
}

winrt::IAsyncAction WebView2::OnSourceChanged(winrt::Uri providedUri)
{
    auto strongThis = get_strong(); // ensure object lifetime during coroutines

    // Trigger / wait for CWV2 creation if one isn't yet ready
    if (!m_coreWebView)
    {
        // Creation request when one is already in flight (either explicit or implicit)
        if (m_creationInProgressAsync)
        {
            MUX_ASSERT(m_isExplicitCreationInProgress || m_isImplicitCreationInProgress);

            // If the in-flight creation is explicit (EnsureCWV2*Async()), navigate to new source when that ends
            // Otherwise it's implicit (another Source set), we are done; first Source will use updated Source value
            if (m_isExplicitCreationInProgress)
            {
                // If EnsureCWV2*Async() call is in flight, wait for it to complete then set Source
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
        auto xamlRoot = this->XamlRoot();
        if (xamlRoot)
        {
            com_ptr<IXamlRootNative> xamlRootNative = xamlRoot.as<IXamlRootNative>();
            winrt::check_hresult(xamlRootNative->get_HostWindow(&m_xamlHostHwnd));
        }
    }

    return m_xamlHostHwnd;
}

void WebView2::RegisterCoreEventHandlers()
{
    // Previously we used get_weak() here, but we found the potential to hit a 
    // refcounting problem where in some scenarios the outer object gets
    // an extra Release() in this process.
    auto weakThis {winrt::make_weak(static_cast<winrt::WebView2>(*this))};
    m_coreNavigationStartingRevoker = m_coreWebView.NavigationStarting(winrt::auto_revoke, {
        [weakThis](auto const&, winrt::CoreWebView2NavigationStartingEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                // Update Uri without navigation
                rawThis->UpdateSourceInternal();
                rawThis->FireNavigationStarting(args);
            }
        } });

    m_coreSourceChangedRevoker = m_coreWebView.SourceChanged(winrt::auto_revoke, {
        [weakThis](auto const&, winrt::CoreWebView2SourceChangedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                rawThis->UpdateSourceInternal();
            }
        } });

    m_coreNavigationCompletedRevoker = m_coreWebView.NavigationCompleted(winrt::auto_revoke, {
        [weakThis](auto const&, winrt::CoreWebView2NavigationCompletedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                rawThis->FireNavigationCompleted(args);
            }
        } });

    m_coreWebMessageReceivedRevoker = m_coreWebView.WebMessageReceived(winrt::auto_revoke, {
        [weakThis](auto const&, winrt::CoreWebView2WebMessageReceivedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                rawThis->FireWebMessageReceived(args);
            }
        } });

    m_coreMoveFocusRequestedRevoker = m_coreWebViewController.MoveFocusRequested(winrt::auto_revoke, {
        [weakThis](auto const&, const winrt::CoreWebView2MoveFocusRequestedEventArgs& args)
        {
            if (auto strongThis = weakThis.get())
            {
                WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                winrt::CoreWebView2MoveFocusReason moveFocusRequestedReason{ args.Reason() };

                if (moveFocusRequestedReason == winrt::CoreWebView2MoveFocusReason::Next ||
                    moveFocusRequestedReason == winrt::CoreWebView2MoveFocusReason::Previous)
                {
                    winrt::XamlRoot xamlRoot = rawThis->XamlRoot();
                    if (xamlRoot)
                    {
                        winrt::FocusNavigationDirection xamlDirection = moveFocusRequestedReason == winrt::CoreWebView2MoveFocusReason::Next
                            ? winrt::FocusNavigationDirection::Next 
                            : winrt::FocusNavigationDirection::Previous;
                        
                        winrt::FindNextElementOptions findNextElementOptions;
                        findNextElementOptions.SearchRoot(xamlRoot.Content());

                        winrt::DependencyObject nextElement = winrt::FocusManager::FindNextElement(xamlDirection, findNextElementOptions);
                        winrt::DependencyObject focusedElement = winrt::FocusManager::GetFocusedElement(xamlRoot).try_as<winrt::DependencyObject>();

                        if (nextElement && nextElement != focusedElement)
                        {
                            // TODO_WebView2: We should check TryMoveFocusAsync() result before returning since FindNextElement()
                            //                only finds the next focusable element, but does not guarantee that we can
                            //                actually focus on it (eg it could be empty/in the process of loading).
                            //                Waiting on this result via winrt/cpp coroutine results in the WINRT_ASSERT
                            //                warning about a blocking wait on UI Thread.
                            //
                            //                We will address this scenario properly as part of:
                            //                Task 23157748: WebView2 should implement IInternalCoreWindowFocus (use Xaml's unified focus model)
                            //
                            //                For now, we'll assign the return value of TryMoveFocusAsync() to a dummy variable
                            //                in order to avoid the code analysis error C26444:
                            //
                            //                https://docs.microsoft.com/en-us/cpp/code-quality/c26444?view=vs-2019

                            // If core webview is also losing focus via something other than TAB (web LostFocus event fired)
                            // and the TAB handling is arriving later (eg due to longer MOJO delay), skip manually moving Xaml Focus to next element.
                            winrt::DependencyObject thisElement = rawThis->try_as<winrt::DependencyObject>();
                            if (thisElement == focusedElement)
                            {
                                // Move focus to the next XAML element
                                const auto _ = winrt::FocusManager::TryMoveFocusAsync(xamlDirection, findNextElementOptions);
                            }
                        }
                        else
                        {
                            // Handle the case where there is no "next" focusable XAML element (WebView2 is first/last/only element), 
                            // which we are in if FindNextElement() returns either null or the (already focused) WebView2. The appropriate 
                            // behavior here is to cycle focus inside the webview, "wrapping around" to the other end (regardless of
                            //  WebView2.KeyboardNavigationMode).To achieve this, manually Call MoveFocus() in the specified direction.
                            rawThis->MoveFocusIntoCoreWebView2(moveFocusRequestedReason);
                        }
                    }
                }

                // Always mark the args handled to prevent CoreWebView2Controller's "default behavior" which breaks XAML focus expectations (by explicitly changing  HWND focus)
                // More info: https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2movefocusrequestedeventargs?view=webview2-winrt-1.0.2535.41#handled
                args.Handled(TRUE);

            }
        } });

    m_coreProcessFailedRevoker = m_coreWebView.ProcessFailed(winrt::auto_revoke, {
        [weakThis](auto const&, const winrt::CoreWebView2ProcessFailedEventArgs& args)
        {
            if (auto strongThis = weakThis.get())
            {
                WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                // Moving focus out of WebView2
                winrt::FocusNavigationDirection xamlDirection{ winrt::FocusNavigationDirection::Next };
                winrt::FindNextElementOptions findNextElementOptions;
                winrt::XamlRoot xamlRoot = rawThis->XamlRoot();

                if (xamlRoot)
                {
                    findNextElementOptions.SearchRoot(xamlRoot.Content());
                    winrt::DependencyObject nextElement = winrt::FocusManager::FindNextElement(xamlDirection, findNextElementOptions);
                    if (nextElement)
                    {
                        const auto _ = winrt::FocusManager::TryMoveFocusAsync(xamlDirection, findNextElementOptions);
                    }
                }

                winrt::CoreWebView2ProcessFailedKind coreProcessFailedKind{ args.ProcessFailedKind() };
                if (coreProcessFailedKind == winrt::CoreWebView2ProcessFailedKind::BrowserProcessExited)
                {
                    rawThis->m_isCoreFailure_BrowserExited_State = true;

                    // CoreWebView2 takes care of clearing the event handlers when closing the host,
                    // but we still need to reset the event tokens
                    rawThis->UnregisterCoreEventHandlers();

                    // Null these out so we can't try to use them anymore
                    rawThis->m_coreWebViewCompositionController = nullptr;
                    rawThis->m_coreWebViewController = nullptr;
                    rawThis->m_coreWebView = nullptr;
                    rawThis->ResetProperties();
                }

                rawThis->FireCoreProcessFailedEvent(args);
            }
        } });

    m_cursorChangedRevoker = m_coreWebViewCompositionController.CursorChanged(winrt::auto_revoke, {
        [weakThis](auto const& controller, auto const& obj)
        {
            if (auto strongThis = weakThis.get())
            {
                WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                winrt::CoreCursor coreCursor = controller.Cursor();
                winrt::IInputSystemCursor inputSystemCursor;

                switch (coreCursor.Type())
                {
                    default:
                    case winrt::CoreCursorType::Arrow:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::Arrow);
                        break;
                    case winrt::CoreCursorType::Cross:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::Cross);
                        break;
                    case winrt::CoreCursorType::Hand:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::Hand);
                        break;
                    case winrt::CoreCursorType::Help:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::Help);
                        break;
                    case winrt::CoreCursorType::IBeam:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::IBeam);
                        break;
                    case winrt::CoreCursorType::Person:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::Person);
                        break;
                    case winrt::CoreCursorType::Pin:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::Pin);
                        break;
                    case winrt::CoreCursorType::SizeAll:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::SizeAll);
                        break;
                    case winrt::CoreCursorType::SizeNortheastSouthwest:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::SizeNortheastSouthwest);
                        break;
                    case winrt::CoreCursorType::SizeNorthwestSoutheast:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::SizeNorthwestSoutheast);
                        break;
                    case winrt::CoreCursorType::SizeNorthSouth:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::SizeNorthSouth);
                        break;
                    case winrt::CoreCursorType::SizeWestEast:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::SizeWestEast);
                        break;
                    case winrt::CoreCursorType::UniversalNo:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::UniversalNo);
                        break;
                    case winrt::CoreCursorType::Wait:
                        inputSystemCursor = winrt::InputSystemCursor::Create(winrt::InputSystemCursorShape::Wait);
                        break;
                }

                winrt::InputCursor inputCursorToSet = inputSystemCursor.as<winrt::InputCursor>();
                rawThis->ProtectedCursor(inputCursorToSet);
            }
        } });
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
    }

    if (m_coreWebViewCompositionController)
    {
        m_cursorChangedRevoker.revoke();
    }
}

winrt::IAsyncAction WebView2::CreateCoreObjects(winrt::CoreWebView2Environment environment, winrt::CoreWebView2ControllerOptions controllerOptions)
{
    MUX_ASSERT((m_isImplicitCreationInProgress && !m_isExplicitCreationInProgress) ||
               (!m_isImplicitCreationInProgress && m_isExplicitCreationInProgress));

    auto strongThis = get_strong(); // ensure object lifetime during coroutines

    if (m_isClosed)
    {
        throw winrt::hresult_error(RO_E_CLOSED, s_error_wv2_closed);
    }
    else
    {
        XamlTelemetry::WebView2_CreateCoreObjects(true, reinterpret_cast<uint64_t>(this));

        m_creationInProgressAsync = std::make_unique<AsyncWebViewOperations>();
        RegisterXamlEventHandlers();

        // We are about to attempt re-creation of the environment,
        // so clear any previous 'Missing Anaheim Warning'
        Children().Clear();

        // Normally we always need a new environment, the exception being when Anaheim process failed and we get to reuse the existing one
        if (!m_isCoreFailure_BrowserExited_State)
        {
            m_coreWebViewEnvironment = environment ? environment : co_await CreateDefaultCoreEnvironment();
        }

        if (m_coreWebViewEnvironment)
        {
            co_await CreateCoreWebViewFromEnvironment(GetHostHwnd(), controllerOptions);

            // Do initialization including rendering setup.
            // Try this now but defer to Loaded event if it hasn't fired yet.
            TryCompleteInitialization();
        }
        else if (m_shouldShowMissingAnaheimWarning)
        {
            CreateMissingAnaheimWarning();
        }

        XamlTelemetry::WebView2_CreateCoreObjects(false, reinterpret_cast<uint64_t>(this));
    }

    co_return;
}

winrt::IAsyncOperation<winrt::CoreWebView2Environment> WebView2::CreateDefaultCoreEnvironment() noexcept
{
    hstring browserInstall;
    hstring userDataFolder;
    winrt::CoreWebView2Environment returnedValue = nullptr;

    auto strongThis = get_strong(); // ensure object lifetime during coroutines

    // NOTE: To enable Anaheim logging, add: environmentOptions.AdditionalBrowserArguments(L"--enable-logging=stderr --v=1");
    winrt::CoreWebView2EnvironmentOptions environmentOptions =  winrt::CoreWebView2EnvironmentOptions();

    auto applicationLanguagesList = winrt::ApplicationLanguages::Languages();
    if (applicationLanguagesList.Size() > 0)
    {
        environmentOptions.Language(applicationLanguagesList.GetAt(0));
    }

    try
    {
        returnedValue = co_await winrt::CoreWebView2Environment::CreateWithOptionsAsync(browserInstall, userDataFolder, environmentOptions);
    }
    catch (winrt::hresult_error e)
    {
        winrt::hresult hr = e.code().value;
        m_shouldShowMissingAnaheimWarning = hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        FireCoreWebView2Initialized(hr);
    }

    co_return returnedValue;
}

winrt::IAsyncAction WebView2::CreateCoreWebViewFromEnvironment(HWND hwndParent, winrt::CoreWebView2ControllerOptions controllerOptions)
{
    auto strongThis = get_strong(); // ensure object lifetime during coroutines

    if (!hwndParent)
    {
        hwndParent = EnsureTemporaryHostHwnd();
    }

    try
    {
        auto windowRef = winrt::CoreWebView2ControllerWindowReference::CreateFromWindowHandle(reinterpret_cast<UINT64>(hwndParent));
        // CreateCoreWebView2CompositionController(Async) creates a CompositionController and is in visual hosting mode.
        // Calling CreateCoreWebView2Controller would create a Controller and would be in windowed mode.
        if (!controllerOptions)
        {
            m_coreWebViewCompositionController = co_await m_coreWebViewEnvironment.CreateCoreWebView2CompositionControllerAsync(windowRef);
        }
        else
        {
            m_customCoreWebViewControllerOptions = controllerOptions;
            m_coreWebViewCompositionController = co_await m_coreWebViewEnvironment.CreateCoreWebView2CompositionControllerAsync(windowRef, controllerOptions);
        }
        m_coreWebViewController = m_coreWebViewCompositionController.as<winrt::CoreWebView2Controller>();
        m_coreWebViewController.DefaultBackgroundColor(this->DefaultBackgroundColor());
        m_coreWebViewController.ShouldDetectMonitorScaleChanges(false);
        m_coreWebView = m_coreWebViewController.CoreWebView2();

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

// Get a theme-appropriate Background color to use as BG for SystemVisualBridge.
// This helps avoid flicker when CoreWebView2 is intializing or shutting down.
winrt::Color WebView2::GetThemeBackgroundColor()
{
    bool isHighContrast = GetAccessibilitySettings().HighContrast();
    std::wstring helperBrushName = isHighContrast ? L"BrushForThemeBackgroundColor_HC" : L"BrushForThemeBackgroundColor";

    // Ensure helper SCB referencing appropriate color as a ThemeResource is added to WV2.Resources.
    // This is needed since programatic lookup of theme resources themselves does not account for active theme.
    if (!this->Resources().HasKey(box_value(helperBrushName)))
    {
        std::wstring resourceName = isHighContrast ? L"SystemColorWindowColor" : L"SolidBackgroundFillColorBase";
        winrt::SolidColorBrush helperBrush = winrt::XamlReader::Load(
            L"<SolidColorBrush xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
            L"xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
            L"x:Name='" + helperBrushName + L"' "
            L"Color='{ThemeResource " + resourceName + L"}'/>"
        ).as<winrt::SolidColorBrush>();

        this->Resources().Insert(box_value(helperBrushName), helperBrush);
    }

    // Read off the theme-appropriate BG color from helper brush resource
    return (this->Resources().Lookup(box_value(helperBrushName)).as<winrt::SolidColorBrush>()).Color();
}

void WebView2::UpdateDefaultVisualBackgroundColor()
{
    if (m_systemVisualBridge) { m_systemVisualBridge.BackgroundColor(GetThemeBackgroundColor()); }
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
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        RegisterClassW(&wc);

        m_tempHostHwnd = CreateWindowEx(
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
    Children().Append(warning);
}

// We could have a child TextBlock (see CreateMissingAnaheimWarning), make sure it is visited by Measure pass.
winrt::Size WebView2::MeasureOverride(winrt::Size const& availableSize)
{
    if (Children().Size() > 0)
    {
        Children().GetAt(0).Measure(availableSize);
    }

    return __super::MeasureOverride(availableSize);
}

// We could have a child TextBlock (see CreateMissingAnaheimWarning), make sure it is visited by Arrange pass.
winrt::Size WebView2::ArrangeOverride(winrt::Size const& finalSize)
{
    if (Children().Size() > 0)
    {
        Children().GetAt(0).Arrange(winrt::Rect{ winrt::Point{0,0}, finalSize });
    }

    return __super::ArrangeOverride(finalSize);
}

void WebView2::FillPointerPenInfo(const winrt::PointerPoint& inputPt, winrt::CoreWebView2PointerInfo outputPt)
{
    winrt::PointerPointProperties inputProperties{ inputPt.Properties() };

    UINT32 outputPt_penFlags{ PEN_FLAG_NONE };
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

    outputPt.PenMask(PEN_MASK_PRESSURE | PEN_MASK_ROTATION | PEN_MASK_TILT_X | PEN_MASK_TILT_Y);
    outputPt.PenPressure(static_cast<uint32_t>(inputProperties.Pressure() * 1024));
    outputPt.PenRotation(static_cast<uint32_t>(inputProperties.Twist()));
    outputPt.PenTiltX(static_cast<int32_t>(inputProperties.XTilt()));
    outputPt.PenTiltY(static_cast<int32_t>(inputProperties.YTilt()));
}

void WebView2::FillPointerTouchInfo(const winrt::PointerPoint& inputPt, winrt::CoreWebView2PointerInfo outputPt)
{
    winrt::PointerPointProperties inputProperties{ inputPt.Properties() };

    outputPt.TouchFlags(TOUCH_FLAG_NONE);
    outputPt.TouchMask(TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE);

    auto touchContact = ScaleRectToPhysicalPixels(inputProperties.ContactRect());
    outputPt.TouchContact(touchContact);
    outputPt.TouchContactRaw(touchContact);

    outputPt.TouchOrientation(static_cast<uint32_t>(inputProperties.Orientation()));
    outputPt.TouchPressure(static_cast<uint32_t>(inputProperties.Pressure() * 1024));
}

void WebView2::FillPointerInfo(const winrt::PointerPoint& inputPt, winrt::CoreWebView2PointerInfo outputPt, const winrt::PointerRoutedEventArgs& args)
{
    winrt::PointerPointProperties inputProperties{ inputPt.Properties() };

    winrt::PointerDeviceType deviceType{ inputPt.PointerDeviceType() };

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
    outputPt.PointerFlags(GetPointerFlags(inputPt));

    auto pixelLocation = ScalePointToPhysicalPixels(inputPt.Position());
    outputPt.PixelLocation(pixelLocation);
    outputPt.PixelLocationRaw(pixelLocation);

    // TODO Task 30544057 - Himetric location and raw himetric location

    outputPt.Time(static_cast<uint32_t>(inputPt.Timestamp() / 1000)); //microsecond to millisecond conversion (for tick count)
    outputPt.HistoryCount(args.GetIntermediatePoints(*this).Size());

    LARGE_INTEGER lpFrequency{};
    if (QueryPerformanceFrequency(&lpFrequency))
    {
        auto scale = 1000000;
        auto frequency = lpFrequency.QuadPart;
        outputPt.PerformanceCount((inputPt.Timestamp() * frequency) / scale);
    }

    outputPt.ButtonChangeKind(static_cast<int32_t>(inputProperties.PointerUpdateKind()));
}

uint32_t WebView2::GetPointerFlags(const winrt::PointerPoint& inputPt)
{
    winrt::PointerPointProperties inputProperties{ inputPt.Properties() };
    winrt::PointerDeviceType deviceType{ inputPt.PointerDeviceType() };
    uint32_t pointerFlags{ POINTER_FLAG_NONE };

    if (deviceType == winrt::PointerDeviceType::Touch)
    {
        if (inputPt.IsInContact())
        {
            pointerFlags |= POINTER_FLAG_INCONTACT;
            pointerFlags |= POINTER_FLAG_FIRSTBUTTON;
        }

        if (inputProperties.PointerUpdateKind() == winrt::PointerUpdateKind::LeftButtonPressed)
        {
            pointerFlags |= POINTER_FLAG_NEW;
        }
    }
    else if (deviceType == winrt::PointerDeviceType::Pen)
    {
        if (inputPt.IsInContact())
        {
            pointerFlags |= POINTER_FLAG_INCONTACT;

            if (!inputProperties.IsBarrelButtonPressed())
            {
                pointerFlags |= POINTER_FLAG_FIRSTBUTTON;
            }
            else
            {
                pointerFlags |= POINTER_FLAG_SECONDBUTTON;
            }
        } // POINTER_FLAG_NEW is currently omitted for pen input
    }

    if (inputProperties.IsInRange()) { pointerFlags |= POINTER_FLAG_INRANGE; }
    if (inputProperties.IsPrimary()) { pointerFlags |= POINTER_FLAG_PRIMARY; }
    if (inputProperties.IsCanceled()) { pointerFlags |= POINTER_FLAG_CANCELED; }
    if (inputProperties.TouchConfidence()) { pointerFlags |= POINTER_FLAG_CONFIDENCE; }
    if (inputProperties.PointerUpdateKind() == winrt::PointerUpdateKind::LeftButtonPressed) { pointerFlags |= POINTER_FLAG_DOWN; }
    if (inputProperties.PointerUpdateKind() == winrt::PointerUpdateKind::LeftButtonReleased) { pointerFlags |= POINTER_FLAG_UP; }
    if (inputProperties.PointerUpdateKind() == winrt::PointerUpdateKind::Other) { pointerFlags |= POINTER_FLAG_UPDATE; }

    return pointerFlags;
}

winrt::Rect WebView2::ScaleRectToPhysicalPixels(winrt::Rect inputRect)
{
    float xVal = inputRect.X * m_rasterizationScale;
    float yVal = inputRect.Y * m_rasterizationScale;
    float width = inputRect.Width * m_rasterizationScale;
    float height = inputRect.Height * m_rasterizationScale;

    return winrt::Rect(xVal, yVal, width, height);
}

winrt::Point WebView2::ScalePointToPhysicalPixels(winrt::Point inputPoint)
{
    return winrt::Point(inputPoint.X * m_rasterizationScale, inputPoint.Y * m_rasterizationScale);
}

void WebView2::OnXamlPointerMessage(UINT message, const winrt::PointerRoutedEventArgs& args) noexcept
{
    // Set Handled to prevent ancestor actions such as ScrollViewer taking focus on PointerPressed/PointerReleased.
    args.Handled(true);

    if (!m_coreWebView || !m_coreWebViewCompositionController || CoreWebView2HasInvalidState())
    {
        // nothing to forward input to
        return;
    }

    winrt::Microsoft::UI::Input::PointerPoint logicalPointerPoint{ args.GetCurrentPoint(*this) };
    winrt::Windows::Foundation::Point logicalPoint{ logicalPointerPoint.Position() };
    winrt::Windows::Foundation::Point physicalPoint = ScalePointToPhysicalPixels(logicalPoint);
    winrt::Microsoft::UI::Input::PointerDeviceType deviceType{ args.Pointer().PointerDeviceType() };

    if (deviceType == winrt::Microsoft::UI::Input::PointerDeviceType::Mouse)
    {
        if (message == WM_MOUSELEAVE)
        {
            m_coreWebViewCompositionController.SendMouseInput(
                winrt::CoreWebView2MouseEventKind{ static_cast<winrt::CoreWebView2MouseEventKind>(message) },
                winrt::CoreWebView2MouseEventVirtualKeys{ static_cast<winrt::CoreWebView2MouseEventVirtualKeys>(0) },
                0,
                winrt::Point{ 0, 0 });
        }
        else
        {
            const WPARAM l_param = WebView2Utility::PackIntoWin32StylePointerArgs_lparam(message, args, physicalPoint);
            const LPARAM w_param = WebView2Utility::PackIntoWin32StyleMouseArgs_wparam(message, args, logicalPointerPoint);

            POINT coords_win32{};
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
    else if ((deviceType == winrt::Microsoft::UI::Input::PointerDeviceType::Touch) ||
             (deviceType == winrt::Microsoft::UI::Input::PointerDeviceType::Pen))
    {
        const winrt::PointerPoint inputPt{ args.GetCurrentPoint(*this) };
        winrt::CoreWebView2PointerInfo outputPt = m_coreWebViewEnvironment.CreateCoreWebView2PointerInfo();

        if (deviceType == winrt::PointerDeviceType::Pen)
        {
            FillPointerPenInfo(inputPt, outputPt);
        }
        else if (deviceType == winrt::PointerDeviceType::Touch)
        {
            FillPointerTouchInfo(inputPt, outputPt);
        }

        FillPointerInfo(inputPt, outputPt, args);

        m_coreWebViewCompositionController.SendPointerInput(
            winrt::CoreWebView2PointerEventKind{ static_cast<winrt::CoreWebView2PointerEventKind>(message) },
            outputPt);
    }
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
    XamlTelemetry::WebView2_FireNavigationCompleted(reinterpret_cast<uint64_t>(this), !!m_navigationCompletedEventSource);

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
    if(CoreWebView2HasInvalidState()) { return; }

    // Update Source to keep coherence between WebView2 and CoreWebView2.
    winrt::hstring newUri{ m_coreWebView.Source() };
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

void WebView2::MoveFocusIntoCoreWebView2(winrt::CoreWebView2MoveFocusReason reason)
{
    if (m_coreWebView && m_coreWebViewController)
    {
        CoreWebView2RunIgnoreInvalidStateSync(
            [&]()
            {
                m_coreWebViewController.MoveFocus(reason);
            });
    }
}

void WebView2::HandleGotFocus(const winrt::Windows::Foundation::IInspectable&, const winrt::RoutedEventArgs&) noexcept
{
    if (m_coreWebView && m_xamlFocusChangeInfo.m_isPending)
    {
        MoveFocusIntoCoreWebView2(m_xamlFocusChangeInfo.m_storedMoveFocusReason);
        m_xamlFocusChangeInfo.m_isPending = false;
    }
}

void WebView2::HandleGettingFocus(const winrt::Windows::Foundation::IInspectable&, const winrt::GettingFocusEventArgs& args) noexcept
{
    winrt::CoreWebView2MoveFocusReason moveFocusReason{ winrt::CoreWebView2MoveFocusReason::Programmatic };

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

void WebView2::HandleLosingFocus(const winrt::Windows::Foundation::IInspectable&, const winrt::RoutedEventArgs&) noexcept
{
    m_xamlFocusChangeInfo.m_isPending = false;
}

void WebView2::RegisterXamlEventHandlers()
{
    m_gettingFocusRevoker = GettingFocus(winrt::auto_revoke, { this, &WebView2::HandleGettingFocus });
    m_gotFocusRevoker = GotFocus(winrt::auto_revoke, { this, &WebView2::HandleGotFocus });
    m_losingFocusRevoker = LosingFocus(winrt::auto_revoke, { this, &WebView2::HandleLosingFocus });
    m_pointerPressedRevoker = PointerPressed(winrt::auto_revoke, { this, &WebView2::HandlePointerPressed });
    m_pointerReleasedRevoker = PointerReleased(winrt::auto_revoke, { this, &WebView2::HandlePointerReleased });
    m_pointerMovedRevoker = PointerMoved(winrt::auto_revoke, { this, &WebView2::HandlePointerMoved });
    m_pointerWheelChangedRevoker = PointerWheelChanged(winrt::auto_revoke, { this, &WebView2::HandlePointerWheelChanged });
    m_pointerExitedRevoker = PointerExited(winrt::auto_revoke, { this, &WebView2::HandlePointerExited });
    m_pointerEnteredRevoker = PointerEntered(winrt::auto_revoke, { this, &WebView2::HandlePointerEntered });
    m_pointerCanceledRevoker = PointerCanceled(winrt::auto_revoke, { this, &WebView2::HandlePointerCanceled });
    m_pointerCaptureLostRevoker = PointerCaptureLost(winrt::auto_revoke, { this, &WebView2::HandlePointerCaptureLost });

    m_sizeChangedRevoker = SizeChanged(winrt::auto_revoke, { this, &WebView2::HandleSizeChanged });
}

void WebView2::UnregisterXamlEventHandlers()
{
    m_gettingFocusRevoker.revoke();
    m_gotFocusRevoker.revoke();
    m_losingFocusRevoker.revoke();
    m_pointerPressedRevoker.revoke();
    m_pointerReleasedRevoker.revoke();
    m_pointerMovedRevoker.revoke();
    m_pointerWheelChangedRevoker.revoke();
    m_pointerExitedRevoker.revoke();
    m_pointerEnteredRevoker.revoke();
    m_pointerCanceledRevoker.revoke();
    m_pointerCaptureLostRevoker.revoke();

    m_sizeChangedRevoker.revoke();
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
        CoreWebView2RunIgnoreInvalidStateSync(
            [&]()
            {
                auto coreWebView2CompositionControllerInterop = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop>();
                winrt::check_hresult(coreWebView2CompositionControllerInterop->get_AutomationProvider(provider.put()));
            });
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
        CoreWebView2RunIgnoreInvalidStateSync(
            [&]()
            {
                auto coreWebView2EnvironmentInterop = m_coreWebViewEnvironment.as<ICoreWebView2EnvironmentInterop>();
                winrt::hresult hr = coreWebView2EnvironmentInterop->GetAutomationProviderForWindow(hwnd, provider.put());
                if (hr != UIA_E_ELEMENTNOTAVAILABLE)
                {
                    winrt::check_hresult(hr);
                }
            });
    }
    return provider.as<winrt::IUnknown>();
}

winrt::IAsyncAction WebView2::EnsureCoreWebView2Async()
{
    co_await EnsureCoreWebView2Async(nullptr, nullptr);
}

winrt::IAsyncAction WebView2::EnsureCoreWebView2Async(winrt::CoreWebView2Environment environment)
{
    co_await EnsureCoreWebView2Async(environment, nullptr);
}

winrt::IAsyncAction WebView2::EnsureCoreWebView2Async(winrt::CoreWebView2Environment environment, winrt::CoreWebView2ControllerOptions controllerOptions)
{
    auto strongThis = get_strong(); // ensure object lifetime during coroutines

    if (m_isClosed)
    {
        throw winrt::hresult_error(RO_E_CLOSED, s_error_wv2_closed);
    }

    // If CWV2 exists already, return immediately provided that current call to EnsureCWV2Async is compatible with original creation
    // Specifically, if current call has:
    //    a. Null args       : Always compatible  [i.e. E() == E(nullptr) == E(nullptr, nullptr))]
    //    b. Non-null arg(s) : Compatible iff called with same args as call that created current core objecs.
    //                         Note comparison is via reference equality for both args.
    if (m_coreWebView)
    {
        if (environment != nullptr || controllerOptions != nullptr)
        {
             if (m_coreWebViewEnvironment != environment)
             {
                throw winrt::hresult_invalid_argument(
                    L"WebView2 was already initialized with a different CoreWebView2Environment. "
                    L"Check to see if the Source property was already set or EnsureCoreWebView2Async was previously called with different values."
                    );
             }
             if (m_customCoreWebViewControllerOptions != controllerOptions)
             {
                throw winrt::hresult_invalid_argument(
                    L"WebView2 was already initialized with different CoreWebView2ControllerOptions. "
                    L"Check to see if the Source property was already set or EnsureCoreWebView2Async was previously called with different values."
                    );
             }
        }

        // Synchronous return - existing core objects are compatible
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
        if (environment != nullptr)
        {
            m_isExplicitEnvironment = true;
        }
        if (controllerOptions != nullptr)
        {
            m_isExplicitControllerOptions = true;
        }
        co_await CreateCoreObjects(environment, controllerOptions);
    }

    MUX_ASSERT(!m_isImplicitCreationInProgress && !m_isExplicitCreationInProgress);
    co_return;
}

void WebView2::DisconnectFromRootVisualTarget()
{
    if(CoreWebView2HasInvalidState()) { return; }

    if (m_coreWebViewCompositionController)
    {
        auto coreWebView2CompositionControllerInterop = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop>();
        winrt::check_hresult(coreWebView2CompositionControllerInterop->put_RootVisualTarget(nullptr));
    }
}

winrt::AccessibilitySettings WebView2::GetAccessibilitySettings()
{
    if (!m_accessibilitySettings)
    {
        m_accessibilitySettings = winrt::AccessibilitySettings();
    }

    return m_accessibilitySettings;
}

struct WebView2_TryCompleteInitialization_RAII : public SimpleXamlStartStopEvent_RAII
{
public:
    WebView2_TryCompleteInitialization_RAII(uint64_t objectPointer) : SimpleXamlStartStopEvent_RAII(objectPointer)
        { XamlTelemetry::WebView2_TryCompleteInitialization(true, m_objectPointer); }
    ~WebView2_TryCompleteInitialization_RAII()
        { XamlTelemetry::WebView2_TryCompleteInitialization(false, m_objectPointer); }
};

void WebView2::TryCompleteInitialization()
{
    // If proper Anaheim not present, no further initialization is necessary
    if (m_shouldShowMissingAnaheimWarning)
    {
        return;
    }

    // If called directly after CWV2 creation completes, it's possible Xaml WV2 Loaded event did not yet fire.
    // Skip  work for now - this will be called again when WV2 Loaded fires.
    if (!this->IsLoaded())
    {
        return;
    }

    // If called from Loaded handler, it's possible CWV2 is not ready yet.
    // Skip work for now - this will be called again when CWV2 is ready.
    if (!m_coreWebView)
    {
        return;
    }

    if(CoreWebView2HasInvalidState())
    {
        return;
    }

    WebView2_TryCompleteInitialization_RAII etwEvents(reinterpret_cast<uint64_t>(this));

    // In a desktop scenario, we may have created the CoreWebView2 with a dummy hwnd as its parent
    // (see EnsureTemporaryHostHwnd()), in which case we need to update to use the real parent here.
    // If we used a CoreWindow parent, that hwnd has not changed. The CoreWebView2 does not allow us to switch
    // from using the CoreWindow as the parent to the XamlRoot.
    winrt::CoreWindow coreWindow = winrt::CoreWindow::GetForCurrentThread();
    if (!coreWindow)
    {
        HWND prevParentWindow = m_xamlHostHwnd;
        m_xamlHostHwnd = nullptr;
        HWND newParentWindow = GetHostHwnd();
        UpdateParentWindow(newParentWindow);
    }

    XamlRootChangedHelper(true /* forceUpdate */);
    // Previously we used get_weak() here, but we found the potential to hit a 
    // refcounting problem where in some scenarios the outer object gets
    // an extra Release() in this process.
    auto weakThis {winrt::make_weak(static_cast<winrt::WebView2>(*this))};
    m_xamlRootChangedRevoker = this->XamlRoot().Changed(winrt::auto_revoke,
        [weakThis](auto const& /*sender*/, auto const& /*args*/)
        {
            if (auto strongThis = weakThis.get())
            {
                WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                rawThis->HandleXamlRootChanged();
            }
        });

    if (!m_actualThemeChangedRevoker)
    {
        m_actualThemeChangedRevoker = this->ActualThemeChanged(winrt::auto_revoke,
            [this](auto&&, auto&&) { UpdateDefaultVisualBackgroundColor(); });
    }

    // TODO_WebView2: Currently, AccessibilitySettings.HighContrastChanged does not work without a core window, and throws
    // an ElementNotFound exception. VS default behavior is to break on exceptions, regardless of whether we catch it below.
    // This is not a good experience for developers, since this happens on every WebView2 creation, and it feels like an error
    // even though it's expected.
    // We should avoid this altogether on desktop by not registering for the HighContrastChanged event, since for now it
    // will never be raised. Once Task #24777629 is fixed, we can remove the coreWindow check.
    if (!m_highContrastChangedRevoker && coreWindow)
    {
        m_highContrastChangedRevoker = GetAccessibilitySettings().HighContrastChanged(winrt::auto_revoke,
            [this](auto&&, auto&&) { UpdateDefaultVisualBackgroundColor(); });
    }

    if (!m_textScaleChangedRevoker)
    {
        m_textScaleChangedRevoker = GetUISettings().TextScaleFactorChanged(winrt::auto_revoke,
            [weakThis](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.get())
                {
                    WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                    // OnTextScaleFactorChanged happens in non-UI thread, use dispatcher to call UpdateCoreWebViewScale in UI thread.
                    rawThis->m_dispatcherHelper.RunAsync([weakThis]()
                        {
                            if (auto strongThis = weakThis.get())
                            {
                                WebView2* rawThis = winrt::get_self<WebView2>(strongThis);
                                rawThis->UpdateCoreWebViewScale();
                            }
                        });
                }
            });
    }

    m_dragEnterRevoker = this->DragEnter(winrt::auto_revoke, { this, &WebView2::OnDragEnter });
    m_dragOverRevoker = this->DragOver(winrt::auto_revoke, { this, &WebView2::OnDragOver });
    m_dragLeaveRevoker = this->DragLeave(winrt::auto_revoke, { this, &WebView2::OnDragLeave });
    m_dropRevoker = this->Drop(winrt::auto_revoke, { this, &WebView2::OnDrop });

    CreateAndSetVisual();

    // If we were recreating the webview after a core process failure, indicate that we have now recovered
    m_isCoreFailure_BrowserExited_State = false;

    if (const auto contentIslandEnvironment = this->XamlRoot().ContentIslandEnvironment())
    {
        const auto appWindowId = contentIslandEnvironment.AppWindowId();
        if(m_appWindow)
        {
            m_appWindow.Changed(m_appWindowPositionChangedToken);
        }
        m_appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(appWindowId);
        m_appWindowPositionChangedToken = m_appWindow.Changed({this, &WebView2::OnAppWindowPositionChanged});
    }

    //HandleGotFocus will give focus to the CWV2 if we're still pending to give it focus
    const winrt::Windows::Foundation::IInspectable emptyIInspectable;
    const winrt::RoutedEventArgs emptyArgs;
    HandleGotFocus(emptyIInspectable, emptyArgs);
}

void WebView2::OnAppWindowPositionChanged(const winrt::Microsoft::UI::Windowing::AppWindow& sender, const winrt::Microsoft::UI::Windowing::AppWindowChangedEventArgs& args)
{
    if (m_coreWebViewController && args.DidPositionChange())
    {
        m_coreWebViewController.NotifyParentWindowPositionChanged();
    }
}

winrt::DataPackageOperation WebView2::DataPackageOperationFromDropEffect(DWORD dropEffect)
{
    winrt::DataPackageOperation dataPackageOperation = winrt::DataPackageOperation::None;
    if ((dropEffect & DROPEFFECT_COPY) == DROPEFFECT_COPY)
    {
        dataPackageOperation |= winrt::DataPackageOperation::Copy;
    }
    if ((dropEffect & DROPEFFECT_LINK) == DROPEFFECT_LINK)
    {
        dataPackageOperation |= winrt::DataPackageOperation::Link;
    }
    if ((dropEffect & DROPEFFECT_MOVE) == DROPEFFECT_MOVE)
    {
        dataPackageOperation |= winrt::DataPackageOperation::Move;
    }

    return dataPackageOperation;
}

void WebView2::ConvertXamlArgsToOle32Args(const winrt::DragEventArgs& args, DWORD& keyState, POINT& cursorPosition, DWORD& dropEffect, IDataObject** dataObject)
{
    keyState = 0;
    cursorPosition = {0};
    dropEffect = DROPEFFECT_NONE;

    // Get the drag drop modifiers and convert to key state
    winrt::DragDropModifiers dragDropModifiers = args.Modifiers();
    if ((dragDropModifiers & winrt::DragDropModifiers::LeftButton) ==
        winrt::DragDropModifiers::LeftButton)
    {
        keyState |= MK_LBUTTON;
    }
    if ((dragDropModifiers & winrt::DragDropModifiers::MiddleButton) ==
        winrt::DragDropModifiers::MiddleButton)
    {
        keyState |= MK_MBUTTON;
    }
    if ((dragDropModifiers & winrt::DragDrop::DragDropModifiers::RightButton) ==
        winrt::DragDrop::DragDropModifiers::RightButton)
    {
        keyState |= MK_RBUTTON;
    }
    if ((dragDropModifiers & winrt::DragDrop::DragDropModifiers::Shift) ==
        winrt::DragDrop::DragDropModifiers::Shift)
    {
        keyState |= MK_SHIFT;
    }
    if ((dragDropModifiers & winrt::DragDrop::DragDropModifiers::Control) ==
        winrt::DragDrop::DragDropModifiers::Control)
    {
        keyState |= MK_CONTROL;
    }
    if ((dragDropModifiers & winrt::DragDrop::DragDropModifiers::Alt) ==
        winrt::DragDrop::DragDropModifiers::Alt)
    {
        keyState |= MK_ALT;
    }

    // point parameter must be modified to include the WebView's offset and be
    // in the WebView's client coordinates (Similar to how SendMouseInput works).
    winrt::Point cursorPosition_winrt{args.GetPosition(*this /* relativeTo */)};
    cursorPosition = {static_cast<LONG>(cursorPosition_winrt.X * m_rasterizationScale), static_cast<LONG>(cursorPosition_winrt.Y * m_rasterizationScale)};

    // Map source's AllowedOperations to dropEffect
    winrt::DataPackageOperation allowedOperations = args.AllowedOperations();
    if ((allowedOperations & winrt::DataPackageOperation::Copy) ==
        winrt::DataPackageOperation::Copy)
    {
        dropEffect |= DROPEFFECT_COPY;
    }
    if ((allowedOperations & winrt::DataPackageOperation::Link) ==
        winrt::DataPackageOperation::Link)
    {
        dropEffect |= DROPEFFECT_LINK;
    }
    if ((allowedOperations & winrt::DataPackageOperation::Move) ==
        winrt::DataPackageOperation::Move)
    {
        dropEffect |= DROPEFFECT_MOVE;
    }

    // Retrieve the IDataPackage
    if (dataObject)
    {
        args.DataView().as(IID_IDataObject, (void**)dataObject);
    }
}


void WebView2::OnDragEnter(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    DWORD keyState;
    POINT cursorPosition;
    DWORD dropEffect;
    winrt::com_ptr<IDataObject> dataObject;

    ConvertXamlArgsToOle32Args(args, keyState, cursorPosition, dropEffect, dataObject.put());

    auto coreWebViewCompositionControllerInterop2 = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop2>();
    winrt::check_hresult(coreWebViewCompositionControllerInterop2->DragEnter(dataObject.get(), keyState, cursorPosition, &dropEffect));

    // Set the 'accepted' potential drop operation (chosen by DragEnter based on provided 'allowed operations')
    // This ensures Drop handler gets called for the element if appropriate
    args.AcceptedOperation(DataPackageOperationFromDropEffect(dropEffect));
}

void WebView2::OnDragOver(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    DWORD keyState;
    POINT cursorPosition;
    DWORD dropEffect;

    ConvertXamlArgsToOle32Args(args, keyState, cursorPosition, dropEffect, nullptr);

    auto coreWebViewCompositionControllerInterop2 = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop2>();
    winrt::check_hresult(coreWebViewCompositionControllerInterop2->DragOver(keyState, cursorPosition, &dropEffect));

    // Set the 'accepted' potential drop operation
    args.AcceptedOperation(DataPackageOperationFromDropEffect(dropEffect));
}

void WebView2::OnDragLeave(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    auto coreWebViewCompositionControllerInterop2 = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop2>();
    winrt::check_hresult(coreWebViewCompositionControllerInterop2->DragLeave());
}

void WebView2::OnDrop(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    DWORD keyState;
    POINT cursorPosition;
    DWORD dropEffect;
    winrt::com_ptr<IDataObject> dataObject;

    ConvertXamlArgsToOle32Args(args, keyState, cursorPosition, dropEffect, dataObject.put());

    auto coreWebViewCompositionControllerInterop2 = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop2>();
    winrt::check_hresult(coreWebViewCompositionControllerInterop2->Drop(dataObject.get(), keyState, cursorPosition, &dropEffect));

    // Reflect the actual operation realized by drop in args.AcceptedOperation
    args.AcceptedOperation(DataPackageOperationFromDropEffect(dropEffect));
}

void WebView2::CreateAndSetVisual()
{
    if (!m_systemVisualBridge)
    {
        winrt::Compositor compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();
        m_systemVisualBridge = winrt::ContentExternalOutputLink::Create(compositor);
    }
    UpdateDefaultVisualBackgroundColor();

    SetCoreWebViewAndVisualSize(static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight()));
    winrt::ElementCompositionPreview::SetElementChildVisual(*this, m_systemVisualBridge.PlacementVisual());

    winrt::com_ptr<IDCompositionTarget> sharedTarget = m_systemVisualBridge.as<IDCompositionTarget>();
    auto coreWebView2CompositionControllerInterop = m_coreWebViewCompositionController.as<ICoreWebView2CompositionControllerInterop>();
    winrt::check_hresult(coreWebView2CompositionControllerInterop->put_RootVisualTarget(sharedTarget.get()));
}

winrt::IAsyncOperation<winrt::hstring> WebView2::ExecuteScriptAsync(winrt::hstring javascriptCode)
{
    // TODO_WebView2: Consider recreating the core webview here if its process had failed,
    // allowing the app to use this API to recover from the error.
    auto strongThis = get_strong(); // ensure object lifetime during coroutines

    winrt::hstring returnedValue{}; // initialized with default value
    if (m_coreWebView)
    {
        try
        {
            returnedValue = co_await m_coreWebView.ExecuteScriptAsync(javascriptCode);
        }
        catch (winrt::hresult_error e)
        {
            if (e.code().value != HRESULT_FROM_WIN32(ERROR_INVALID_STATE))
            {
                throw;
            }
        }
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

        DestroyWindow(m_tempHostHwnd);
        m_tempHostHwnd = nullptr;
    }
}

void WebView2::OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    // OnLoaded and OnUnloaded are fired from XAML asynchronously, and unfortunately they could
    // be out of order.  If the element is removed from tree A and added to tree B, we could get
    // a Loaded event for tree B *before* we see the Unloaded event for tree A.  To handle this:
    //  * When we get a Loaded/Unloaded event, check the IsLoaded property. If it doesn't match
    //      the event we're in, nothing needs to be done since the other handler took care of it.
    //  * When we see a Loaded event when we have been or are already loaded, remove the
    //      XamlRootChanged event handler for the old tree and bind to the new one.

    // If we're not loaded, there's nothing for us to do since Unloaded took care of everything
    if (!this->IsLoaded())
    {
        return;
    }

    TryCompleteInitialization();
}

void WebView2::OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    // Note that we always check the IsLoaded property before doing unloading actions, since we
    // could get an Unloaded event for an old tree A *after* a Loaded event for a new tree B.
    // See comment in WebView2::OnLoaded
    if (this->IsLoaded())
    {
        return;
    }

    UpdateRenderedSubscriptionAndVisibility();

    m_xamlRootChangedRevoker.revoke();
    if (m_appWindow)
    {
        m_appWindow.Changed(m_appWindowPositionChangedToken);
        m_appWindow = nullptr;
    }

    DisconnectFromRootVisualTarget();
}

void WebView2::Reload()
{
    if (m_coreWebView)
    {
        CoreWebView2RunIgnoreInvalidStateSync(
            [&]()
            {
                m_coreWebView.Reload();
            });
    }
    else
    {
        throw winrt::hresult_illegal_method_call(std::wstring(L"Reload(): ").append(s_error_cwv2_not_present));
    }
}

void WebView2::UpdateCoreWebViewScale()
{
    if (m_coreWebViewController)
    {
        auto textScaleFactor = m_uiSettings.TextScaleFactor();
        m_coreWebViewController.RasterizationScale(static_cast<double>(m_rasterizationScale * textScaleFactor));
    }
}

void WebView2::NavigateToString(winrt::hstring htmlContent)
{
    if (m_coreWebView)
    {
        CoreWebView2RunIgnoreInvalidStateSync(
            [&]()
            {
                m_coreWebView.NavigateToString(htmlContent);
            });
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
        CoreWebView2RunIgnoreInvalidStateSync(
            [&]()
            {
                m_coreWebView.GoBack();
            });
    }
}

void WebView2::GoForward()
{
    if (m_coreWebView && CanGoForward())
    {
        CoreWebView2RunIgnoreInvalidStateSync(
            [&]()
            {
                m_coreWebView.GoForward();
            });
    }
}

void WebView2::XamlRootChangedHelper(bool forceUpdate)
{
    if(CoreWebView2HasInvalidState()) { return; }

    auto xamlRoot = this->XamlRoot();
    if (xamlRoot)
    {
        auto scale = static_cast<float>(xamlRoot.RasterizationScale());
        bool hostVisibility = xamlRoot.IsHostVisible();
        if (forceUpdate || (scale != m_rasterizationScale))
        {
            m_rasterizationScale = scale;
            m_isHostVisible = hostVisibility; // If we did forceUpdate we'll want to update host visibility here too
            UpdateCoreWebViewScale();
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
    if (!m_coreWebViewController)
    {
        return;
    }

    // Skip this work if WebView2 has just been removed from the tree - otherwise the CWV2.Bounds update could cause a flicker.
    //
    // After WebView2 is removed from the tree, this handler gets run one more time during the frame's render pass
    // (WebView2::HandleRendered()). The removed element's ActualWidth or ActualHeight could now evaluate to zero
    // (if Width or Height weren't explicitly set), causing 0-sized Bounds to get applied below and clear the web content,
    // producing a flicker that last until DComp Commit for this frame is processed by Lifted & OS Compositors.
    if (!this->IsLoaded())
    {
        return;
    }

    // Check if the position of the WebView2 within the window has changed
    bool changed = false;
    auto transform = TransformToVisual(nullptr);
    auto topLeft = transform.TransformPoint(winrt::Point(0, 0));

    auto scaledTopLeftX = ceil(topLeft.X * m_rasterizationScale);
    auto scaledTopLeftY = ceil(topLeft.Y * m_rasterizationScale);

    if (scaledTopLeftX != m_webViewScaledPosition.X || scaledTopLeftY != m_webViewScaledPosition.Y)
    {
        m_webViewScaledPosition.X = scaledTopLeftX;
        m_webViewScaledPosition.Y = scaledTopLeftY;
        changed = true;
    }

    auto scaledSizeX = ceil(static_cast<float>(ActualWidth()) * m_rasterizationScale);
    auto scaledSizeY = ceil(static_cast<float>(ActualHeight()) * m_rasterizationScale);
    if (scaledSizeX != m_webViewScaledSize.X || scaledSizeY != m_webViewScaledSize.Y)
    {
        m_webViewScaledSize.X = scaledSizeX;
        m_webViewScaledSize.Y = scaledSizeY;
        changed = true;
    }

    if (changed)
    {
        // We create the Bounds using X, Y, width, and height
        CoreWebView2RunIgnoreInvalidStateSync(
            [&]()
            {
                m_coreWebViewController.Bounds({
                    (m_webViewScaledPosition.X),
                    (m_webViewScaledPosition.Y),
                    (m_webViewScaledSize.X),
                    (m_webViewScaledSize.Y) });
            });
    }
}

void WebView2::SetCoreWebViewAndVisualSize(const float width, const float height)
{
    if (!m_coreWebView && !m_systemVisualBridge) return;

    if (m_coreWebView)
    {
        CheckAndUpdateWebViewPosition();
    }

    // The CoreWebView2 visuals hosted under the bridge visual are already scaled for the rasterization scale.
    // To keep them from being scaled again from the scale above the WebView2 element, we need to apply
    // an inverse scale on the bridge visual. Since the inverse scale will reduce the size of the bridge visual, we
    // need to scale up the size by the rasterization scale to compensate.

    if (m_systemVisualBridge)
    {
        winrt::Visual systemVisualPlacementVisual = m_systemVisualBridge.PlacementVisual();
        winrt::float2 newSize = winrt::float2(width * m_rasterizationScale, height * m_rasterizationScale);
        winrt::float3 newScale = winrt::float3(1.0f / m_rasterizationScale, 1.0f / m_rasterizationScale, 1.0);

        systemVisualPlacementVisual.Size(newSize);
        systemVisualPlacementVisual.Scale(newScale);
    }
}

void WebView2::CheckAndUpdateWindowPosition()
{
    auto hostWindow = GetHostHwnd();
    if (!m_coreWebViewController || !::IsWindow(hostWindow))
    {
        return;
    }

    POINT windowPosition = { 0, 0 };
    ::ClientToScreen(hostWindow, &windowPosition);
    if (m_hostWindowPosition.x != windowPosition.x || m_hostWindowPosition.y != windowPosition.y)
    {
        m_hostWindowPosition.x = windowPosition.x;
        m_hostWindowPosition.y = windowPosition.y;
        CoreWebView2RunIgnoreInvalidStateSync(
            [&]()
            {
                m_coreWebViewController.NotifyParentWindowPositionChanged();
            });
    }
 }

void WebView2::CheckAndUpdateVisibility(bool force)
{
    // Keep booleans in this order to prevent doing expensive tree walk if we don't have to.
    bool currentVisibility = this->Visibility() == winrt::Visibility::Visible &&
                             this->IsLoaded() &&
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
    auto strongThis = get_strong(); // ensure object lifetime during coroutines
    auto updateCoreWebViewVisibilityAction = [&, strongThis]()
    {
        if (strongThis->m_coreWebViewController)
        {
            CoreWebView2RunIgnoreInvalidStateSync(
                [&]()
                {
                    strongThis->m_coreWebViewController.IsVisible(strongThis->m_isVisible);
                });
        }
    };

    if (!m_isVisible && m_isHostVisible)
    {
        SharedHelpers::ScheduleActionAfterWait(updateCoreWebViewVisibilityAction, 200);
    }
    else
    {
        if (m_coreWebViewController)
        {
            CoreWebView2RunIgnoreInvalidStateSync(
                [&]()
                {
                    m_coreWebViewController.IsVisible(m_isVisible);
                });
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
        if (parentVisibility == winrt::Visibility::Collapsed)
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
    if (this->IsLoaded() && this->Visibility() == winrt::Visibility::Visible)
    {
        if (!m_renderedRevoker)
        {
            m_renderedRevoker = winrt::Microsoft::UI::Xaml::Media::CompositionTarget::Rendered(
                winrt::auto_revoke, { this, &WebView2::HandleRendered });
        }
    }
    else
    {
        if (m_renderedRevoker)
        {
            m_renderedRevoker.revoke();
        }
    }
    CheckAndUpdateVisibility(true);
}

void WebView2::SetControllerDefaultBackgroundColor()
{
    if(CoreWebView2HasInvalidState()) { return; }

    if (m_coreWebViewController)
    {
        m_coreWebViewController.DefaultBackgroundColor(this->DefaultBackgroundColor());
    }
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

HWND WebView2::GetComponentHwnd()
{
    HWND parentWindow = GetHostHwnd();
    HWND childWindow = FindWindowExA(
        parentWindow,          // hwndParent
        nullptr,
        "Chrome_WidgetWin_0",  // window class name
        nullptr);

    auto peer = OnCreateAutomationPeer().as<IAutomationPeerHwndInterop>();

    while (childWindow)
    {
        bool foundHwnd = false;
        // Check for the correct peer, because there will be more than one child hwnd if there is more than one WebView2.
        winrt::check_hresult(peer->IsCorrectPeerForHwnd(childWindow, &foundHwnd));
        if (foundHwnd)
        {
            return childWindow;
        }
        else
        {
            childWindow = GetWindow(childWindow, GW_HWNDNEXT);
        }
    }

    return nullptr;
}

winrt::UISettings WebView2::GetUISettings()
{
    if (m_uiSettings == nullptr)
    {
        m_uiSettings = winrt::UISettings();
    }
    return m_uiSettings;
}
