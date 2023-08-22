// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <winrt\Microsoft.Web.WebView2.Core.h>

namespace winrt
{
    using namespace Microsoft::Web::WebView2::Core;
}

#include "CoreWebView2InitializedEventArgs.g.h"

#include "DispatcherHelper.h"

#include "WebView2.g.h"
#include "WebView2.properties.h"
#pragma warning( push )
#pragma warning( disable : 28251 26812)   // warning C28251: Inconsistent annotation for function: this instance has an error
#include <webview2.h>
#include <WebView2EnvironmentOptions.h>
#pragma warning( pop )

// for making async/await possible
struct AsyncWebViewOperations final : public Awaitable
{
    AsyncWebViewOperations() = default;

    void StopWaiting()
    {
        // async operation completed, stop all awaits
        CompleteAwaits();
    }
};

class CoreWebView2InitializedEventArgs :
    public winrt::implementation::CoreWebView2InitializedEventArgsT<CoreWebView2InitializedEventArgs>
{
public:
    CoreWebView2InitializedEventArgs(winrt::hresult Exception) : m_exception(Exception) {};

    void Exception(winrt::hresult exception) { m_exception = exception; };
    winrt::hresult Exception() { return m_exception; }
private:
    winrt::hresult m_exception{};
};

template <typename D, typename T, typename ... I>
struct __declspec(empty_bases)DeriveFromContentControlHelper_base : winrt::Windows::UI::Xaml::Controls::ContentControlT<D, winrt::default_interface<T>, winrt::composable, I...>
{
    using composable = T;
    using class_type = typename T;

    operator class_type() const noexcept
    {
        return static_cast<winrt::IInspectable>(*this).as<class_type>();
    }

    hstring GetRuntimeClassName() const
    {
        return hstring{ winrt::name_of<T>() };
    }
};

class WebView2 :
    public ReferenceTracker<WebView2, DeriveFromContentControlHelper_base, winrt::WebView2>,
    public WebView2Properties
{

public:

    WebView2();
    virtual ~WebView2();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    // FrameworkElement overrides
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

    winrt::IUnknown GetWebView2Provider();
    winrt::IUnknown GetProviderForHwnd(HWND hwnd);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

    void UpdateCoreWebViewScale();

    winrt::IAsyncAction EnsureCoreWebView2Async();
    winrt::IAsyncOperation<winrt::hstring> ExecuteScriptAsync(winrt::hstring javascriptCode);
    void Reload();
    void NavigateToString(winrt::hstring htmlContent);
    void GoBack();
    void GoForward();
    void Close();

    winrt::CoreWebView2 CoreWebView2();     // Getter for CoreWebView2 property (read-only)

    winrt::Rect GetBoundingRectangle();

private:
    bool ShouldNavigate(const winrt::Uri& uri);
    winrt::IAsyncAction OnSourceChanged(winrt::Uri providedUri);
    void OnXamlPointerMessage(UINT message, const winrt::PointerRoutedEventArgs& args) noexcept;
    void FillPointerPenInfo(const winrt::PointerPoint& inputPt, winrt::CoreWebView2PointerInfo outputPt);
    void FillPointerTouchInfo(const winrt::PointerPoint& inputPt, winrt::CoreWebView2PointerInfo outputPt);
    void FillPointerInfo(const winrt::PointerPoint& inputPt, winrt::CoreWebView2PointerInfo outputPt, const winrt::PointerRoutedEventArgs& args);
    uint32_t GetPointerFlags(const winrt::PointerPoint& inputPt);
    winrt::Rect ScaleRectToPhysicalPixels(winrt::Rect inputRect);
    winrt::Point ScalePointToPhysicalPixels(winrt::Point inputPoint);

    void ResetMouseInputState();
    void OnManipulationModePropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/);
    void OnVisibilityPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/);

    void FireNavigationStarting(const winrt::CoreWebView2NavigationStartingEventArgs& args);
    void FireNavigationCompleted(const winrt::CoreWebView2NavigationCompletedEventArgs& args);
    void FireWebMessageReceived(const winrt::CoreWebView2WebMessageReceivedEventArgs& args);
    void UpdateSourceInternal();
    void FireCoreProcessFailedEvent(const winrt::CoreWebView2ProcessFailedEventArgs& args);
    void FireCoreWebView2Initialized(winrt::hresult exception);

    void UpdateDefaultVisualBackgroundColor();

    HWND GetHostHwnd() noexcept;
    HWND GetActiveInputWindowHwnd() noexcept;
    HWND EnsureTemporaryHostHwnd();
    void UpdateParentWindow(HWND newParentWindow);

    winrt::IAsyncAction CreateCoreObjects();
    winrt::IAsyncAction CreateCoreEnvironment() noexcept;
    winrt::IAsyncAction CreateCoreWebViewFromEnvironment(HWND hwndParent);
    void CreateMissingAnaheimWarning();

    void RegisterXamlEventHandlers();
    void UnregisterXamlEventHandlers();
    void RegisterCoreEventHandlers();
    void UnregisterCoreEventHandlers();

    void HandlePointerPressed(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args);
    void HandlePointerReleased(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args);
    void HandlePointerMoved(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args);
    void HandlePointerWheelChanged(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args);
    void HandlePointerExited(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args);
    void HandlePointerEntered(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args);
    void HandlePointerCanceled(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args);
    void HandlePointerCaptureLost(const winrt::Windows::Foundation::IInspectable&, const winrt::PointerRoutedEventArgs& args);
    void HandleKeyDown(const winrt::Windows::Foundation::IInspectable&, const winrt::KeyRoutedEventArgs& e);
    void HandleGettingFocus(const winrt::Windows::Foundation::IInspectable&, const winrt::GettingFocusEventArgs& args) noexcept;
    void HandleGotFocus(const winrt::Windows::Foundation::IInspectable&, const winrt::RoutedEventArgs&);
    void MoveFocusIntoCoreWebView(winrt::CoreWebView2MoveFocusReason reason);
    void HandleAcceleratorKeyActivated(const winrt::Windows::UI::Core::CoreDispatcher&, const winrt::AcceleratorKeyEventArgs& args) noexcept;
    void HandleXamlRootChanged();
    void HandleSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& args);
    void HandleRendered(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/);

    void XamlRootChangedHelper(bool forceUpdate);
    void TryCompleteInitialization();
    void DisconnectFromRootVisualTarget();
    void CreateAndSetVisual();
    void AddChildPanel();

    void CheckAndUpdateWebViewPosition();
    void CheckAndUpdateWindowPosition();
    void CheckAndUpdateVisibility(bool force = false);
    void UpdateCoreWebViewVisibility();
    bool AreAllAncestorsVisible();
    void SetCoreWebViewAndVisualSize(const float width, const float height);
    void UpdateRenderedSubscriptionAndVisibility();

    void SetCanGoForward(bool canGoForward);
    void SetCanGoBack(bool canGoBack);
    void ResetProperties();
    void CloseInternal(bool inShutdownPath);

    winrt::AccessibilitySettings GetAccessibilitySettings();
    winrt::UISettings GetUISettings();

    bool SafeIsLoaded();

    void UpdateCoreWindowCursor();

private:
    struct XamlFocusChangeInfo
    {
        winrt::CoreWebView2MoveFocusReason m_storedMoveFocusReason{};

        // Reason gets set in GettingFocus handler, but the focus change can still be cancelled or redirected
        bool m_isPending{};
    };

    winrt::CoreWebView2EnvironmentOptions m_options{ nullptr };
    winrt::CoreWebView2Environment m_coreWebViewEnvironment{ nullptr };
    winrt::CoreWebView2Controller m_coreWebViewController{ nullptr };
    winrt::CoreWebView2CompositionController m_coreWebViewCompositionController{ nullptr };
    winrt::CoreWebView2 m_coreWebView{ nullptr };
    bool m_everHadCoreWebView{};

    bool m_isLeftMouseButtonPressed{};
    bool m_isMiddleMouseButtonPressed{};
    bool m_isRightMouseButtonPressed{};
    bool m_isXButton1Pressed{};
    bool m_isXButton2Pressed{};
    bool m_hasMouseCapture{};
    std::map<int32_t, bool> m_hasTouchCapture{};
    bool m_hasPenCapture{};
    HWND m_xamlHostHwnd{ nullptr };
    HWND m_tempHostHwnd{ nullptr };
    HWND m_inputWindowHwnd{ nullptr };
    winrt::hstring m_stopNavigateOnUriChanged{};
    bool m_canGoPropSetInternally{};
    winrt::Windows::UI::Composition::SpriteVisual m_visual{ nullptr };

    winrt::UIElement::GettingFocus_revoker m_gettingFocusRevoker;
    winrt::UIElement::GotFocus_revoker m_gotFocusRevoker;
    winrt::UIElement::PointerPressed_revoker m_pointerPressedRevoker;
    winrt::UIElement::PointerReleased_revoker m_pointerReleasedRevoker;
    winrt::UIElement::PointerMoved_revoker m_pointerMovedRevoker;
    winrt::UIElement::PointerWheelChanged_revoker m_pointerWheelChangedRevoker;
    winrt::UIElement::PointerExited_revoker m_pointerExitedRevoker;
    winrt::UIElement::PointerEntered_revoker m_pointerEnteredRevoker;
    winrt::UIElement::PointerCanceled_revoker m_pointerCanceledRevoker;
    winrt::UIElement::PointerCaptureLost_revoker m_pointerCaptureLostRevoker;
    winrt::UIElement::KeyDown_revoker m_keyDownRevoker;
    winrt::CoreDispatcher::AcceleratorKeyActivated_revoker m_acceleratorKeyActivatedRevoker;
    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};
    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_sizeChangedRevoker;
    winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendered_revoker m_renderedRevoker{};
    winrt::FrameworkElement::LayoutUpdated_revoker m_layoutUpdatedRevoker{};
    XamlRootChanged_revoker m_xamlRootChangedRevoker{};
    winrt::event_token m_manipulationModeChangedToken{};
    winrt::event_token m_visibilityChangedToken{};
    winrt::Window::VisibilityChanged_revoker m_windowVisibilityChangedRevoker{};

    winrt::CoreWebView2::NavigationStarting_revoker m_coreNavigationStartingRevoker;
    winrt::CoreWebView2::SourceChanged_revoker m_coreSourceChangedRevoker;
    winrt::CoreWebView2::NavigationCompleted_revoker m_coreNavigationCompletedRevoker;
    winrt::CoreWebView2::WebMessageReceived_revoker m_coreWebMessageReceivedRevoker;
    winrt::CoreWebView2::ProcessFailed_revoker m_coreProcessFailedRevoker;
    winrt::CoreWebView2Controller::MoveFocusRequested_revoker m_coreMoveFocusRequestedRevoker;
    winrt::CoreWebView2Controller::LostFocus_revoker m_coreLostFocusRevoker;
    winrt::CoreWebView2CompositionController::CursorChanged_revoker m_cursorChangedRevoker;

    winrt::FrameworkElement::ActualThemeChanged_revoker m_actualThemeChangedRevoker{};
    winrt::AccessibilitySettings m_accessibilitySettings;
    winrt::AccessibilitySettings::HighContrastChanged_revoker m_highContrastChangedRevoker{};
    winrt::UISettings::TextScaleFactorChanged_revoker m_textScaleChangedRevoker{};

    // Pointer handling for CoreWindow
    void ResetPointerHelper(const winrt::PointerRoutedEventArgs& args);
    bool m_isPointerOver{};
    winrt::CoreCursor m_oldCursor{ nullptr };

    XamlFocusChangeInfo m_xamlFocusChangeInfo{};

    // Tracks when Edge thinks it has focus.
    //
    // Xaml's CoreWindow hosting code swallows WM_KEYDOWN messages for VK_TAB that are expected to go to InputWindow HWND.
    // When true, fill in this missing event manually via SendMessage so that TAB's can be processed in Edge.
    bool m_webHasFocus{};

    bool m_isVisible{};
    bool m_isHostVisible{};
    bool m_shouldShowMissingAnaheimWarning{};

    bool m_loaded{};

    bool m_isCoreFailure_BrowserExited_State{};    // True after Edge ProcessFailed event w/ CORE_WEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED
    bool m_isClosed{};    // True after WebView2::Close() has been called - no core objects can be created

    bool m_isImplicitCreationInProgress{};    // True while we are creating CWV2 due to Source property being set
    bool m_isExplicitCreationInProgress{};    // True while we are creating CWV2 due to EnsureCoreWebView2Async() being called
    std::unique_ptr<AsyncWebViewOperations> m_creationInProgressAsync{ nullptr };    // Awaitable object for any currently active creation. There should be only one active operation at a time.

    float m_rasterizationScale{};
    // The last known WebView rect position, scaled for DPI
    winrt::Point m_webViewScaledPosition{};
    // Last known WebView rect size, scaled for DPI
    winrt::Point m_webViewScaledSize{};
    // Last known position of the host window
    POINT m_hostWindowPosition{};

    DispatcherHelper m_dispatcherHelper{ *this };

    winrt::UISettings m_uiSettings;

    // Manually delay load functions to avoid WACK exceptions.
    decltype(&ClientToScreen) m_fnClientToScreen;
    decltype(&SendMessageW) m_fnSendMessageW;
    decltype(&CreateWindowExW) m_fnCreateWindowExW;
    decltype(&DefWindowProcW) m_fnDefWindowProcW;
    decltype(&GetFocus) m_fnGetFocus;
    decltype(&RegisterClassW) m_fnRegisterClassW;
    decltype(&DestroyWindow) m_fnDestroyWindow;
};
