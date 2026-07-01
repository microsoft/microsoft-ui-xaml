// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <optional>
#include <memory>

#include "Window.g.h"
#include "ContentManager.h"
#include <WindowImpl.h>
#include <DesktopWindowXamlSource_partial.h>
#include "CWindowChrome.h"
#include "BaseWindow.h"


namespace DirectUI
{
    class DesktopWindowImpl : public WindowImpl, public BaseWindow<DesktopWindowImpl>
    {

    public:
        DesktopWindowImpl(Window* parentWindow);
        ~DesktopWindowImpl() override;

        // ---------------------------------------------------------------
        // public Window API: IWindow, IWindow3, IWindow4
        // ---------------------------------------------------------------
        _Check_return_ HRESULT get_BoundsImpl(_Out_ wf::Rect* pValue) override;
        _Check_return_ HRESULT get_VisibleImpl(_Out_ BOOLEAN* pValue) override;
        _Check_return_ HRESULT get_ContentImpl(_Outptr_result_maybenull_ xaml::IUIElement** pValue) override;
        _Check_return_ HRESULT put_ContentImpl(_In_opt_ xaml::IUIElement* value) override;
        _Check_return_ HRESULT get_CoreWindowImpl(_Outptr_result_maybenull_ wuc::ICoreWindow** pValue) override;
        _Check_return_ HRESULT get_DispatcherImpl(_Outptr_result_maybenull_ wuc::ICoreDispatcher** pValue) override;
        _Check_return_ HRESULT get_TitleImpl(_Out_ HSTRING* pValue) override;
        _Check_return_ HRESULT put_TitleImpl(_In_opt_ HSTRING value) override;
        IFACEMETHOD(add_Activated)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_Activated)(EventRegistrationToken tToken) override;
        IFACEMETHOD(add_Closed)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_Closed)(EventRegistrationToken tToken) override;
        IFACEMETHOD(add_SizeChanged)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*> * pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_SizeChanged)(EventRegistrationToken tToken) override;
        IFACEMETHOD(add_VisibilityChanged)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_VisibilityChanged)(EventRegistrationToken tToken) override;
        _Check_return_ HRESULT ActivateImpl() override;
        _Check_return_ HRESULT CloseImpl() override;

        _Check_return_ HRESULT get_CompositorImpl(_Outptr_result_maybenull_ WUComp::ICompositor** compositor) override;
        _Check_return_ HRESULT get_WidthImpl(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_WidthImpl(DOUBLE value) override;
        _Check_return_ HRESULT get_HeightImpl(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_HeightImpl(DOUBLE value) override;

        _Check_return_ HRESULT get_ExtendsContentIntoTitleBarImpl(_Out_ BOOLEAN* pValue) final;
        _Check_return_ HRESULT put_ExtendsContentIntoTitleBarImpl(_In_ BOOLEAN value) final;
        _Check_return_ HRESULT SetTitleBarImpl(_In_ xaml::IUIElement* pTitleBar) final;
        _Check_return_ HRESULT get_AppWindowImpl(_Outptr_result_maybenull_ ixp::IAppWindow** ppValue) final;

        _Check_return_ HRESULT get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop** systemBackdrop) override;
        _Check_return_ HRESULT put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop* systemBackdrop) override;

        _Check_return_ HRESULT SetXamlIslandRootBackground(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush, xaml::Media::ISystemBackdrop* systemBackdrop);

        // Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop implementation
        IFACEMETHOD(get_SystemBackdrop)(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush) override;
        IFACEMETHOD(put_SystemBackdrop)(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush) override;

        // IWindowNative
        _Check_return_ IFACEMETHOD(get_WindowHandle)(_Out_ HWND* pValue) override;

        // -------------------------------------------------
        // IWindowPrivate
        // -------------------------------------------------
        _Check_return_ HRESULT get_TransparentBackgroundImpl(_Out_ BOOLEAN* pValue) override;
        _Check_return_ HRESULT put_TransparentBackgroundImpl(_In_ BOOLEAN value) override;
        _Check_return_ HRESULT ShowImpl() override;
        _Check_return_ HRESULT HideImpl() override;
        _Check_return_ HRESULT MoveWindowImpl(_In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height) override;
        _Check_return_ HRESULT SetAtlasSizeHintImpl(UINT width, UINT height) override;
        _Check_return_ HRESULT ReleaseGraphicsDeviceOnSuspendImpl(BOOLEAN enable) override;
        _Check_return_ HRESULT SetAtlasRequestCallbackImpl(_In_opt_ xaml::IAtlasRequestCallback* callback) override;
        _Check_return_ HRESULT GetWindowContentBoundsForElementImpl(_In_ xaml::IDependencyObject* element, _Out_ wf::Rect* rect) override;

        // --------------------------------------------------
        // internal instance methods
        // --------------------------------------------------
        bool HasBounds() override;
        _Check_return_ HRESULT SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow) override;
        _Check_return_ HRESULT GetVisibleBounds(_In_ bool ignoreIHM, _In_ bool inDesktopCoordinates, _Out_ wf::Rect* pValue) override;
        _Check_return_ HRESULT GetRootScrollViewer(_Outptr_result_maybenull_ xaml_controls::IScrollViewer** pRootScrollViewer) override;
        _Check_return_ HRESULT OnWindowSizeChanged() override;
        WindowType::Enum GetWindowType() override;
        _Check_return_ HRESULT GetLayoutBounds(_Out_  wf::Rect* pLayoutBounds) override;
        _Check_return_ HRESULT GetLayoutBounds(_Out_  XRECTF* pLayoutBoundsRect) override;
        DXamlCore* GetDXamlCore() override;
        void SetDXamlCore(_In_opt_ DXamlCore* pDXamlCore) override;
        _Check_return_ HRESULT EnsureInitializedForIslands() override;
        bool ShouldShrinkApplicationViewVisibleBounds() const override;

        ctl::ComPtr<xaml::IAtlasRequestCallback> GetAtlasRequestCallback() override;

        // --------------------------------------------------
        // DXamlTestHooks
        // --------------------------------------------------
        UINT32 GetWidthOverride() override;
        void SetWidthOverride(UINT32 width) override;
        UINT32 GetHeightOverride() override;
        void SetHeightOverride(UINT32 height) override;
        wf::Rect GetLayoutBoundsOverrides() override;
        void SetLayoutBoundsOverrides(const wf::Rect& rect) override;
        wf::Rect& GetVisibleBoundsOverrides() override;
        void SetVisibleBoundsOverrides(const wf::Rect& rect) override;
        void SetHasSizeOverrides(bool hasSizeOverrides) override;
        void SetShrinkApplicationViewVisibleBounds(bool enabled) override;

    public:
        LRESULT OnMessage(
            UINT const message,
            WPARAM const wparam,
            LPARAM const lparam) noexcept override final;
        void OnCreate() noexcept override final;
        HWND GetPositioningBridgeWindowHandle() const {return m_positioningBridgeWindowHandle;}
        void MakeWindowRTL();

    private:
        LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
        _Check_return_ HRESULT OnSizeChanged(WPARAM wParam, LPARAM lParam);
        _Check_return_ HRESULT OnMoved(WPARAM wParam, LPARAM lParam);
        _Check_return_ HRESULT OnActivate(WPARAM wParam, LPARAM lParam);
        void OpenSystemMenu(const int cursorX, const int cursorY) const noexcept;
        _Check_return_ HRESULT OnNonClientRegionButtonUp(WPARAM wParam, const int cursorX, const int cursorY);
        _Check_return_ HRESULT OnClosed();

        void OnSetFocus();
        void RegisterDesktopWindowClass();
        void CreateDesktopWindow();
        _Check_return_ HRESULT RaiseWindowSizeChangedEvent();
        _Check_return_ HRESULT RaiseWindowActivatedEvent(_In_ const xaml::WindowActivationState state);
        _Check_return_ HRESULT RaiseWindowVisibilityChangedEvent(_In_ const BOOLEAN visible);

        // --------------------------------------------------
        // Window sizing support
        // --------------------------------------------------
        // Shared getter helper: resolves the client size (in DIPs) the Width/Height getters report
        // when no per-axis value is pending - the maximized/minimized restored size, the tracked
        // restored size while in a non-sizing presenter, or otherwise the live size.
        _Check_return_ HRESULT GetEffectiveClientSizeInDips(_Out_ wf::Size* pValue);
        // Shared setter helper: records that the app opted into Width/Height, then either applies the
        // requested client size now or remembers it (pending) to apply on first activation / when a
        // sizing-capable presenter returns. Only the axis with a value is affected.
        _Check_return_ HRESULT ApplyOrDeferClientSizeInDips(std::optional<double> width, std::optional<double> height);
        _Check_return_ HRESULT SetRestoredClientSizeInDips(std::optional<double> width, std::optional<double> height);
        _Check_return_ HRESULT TryGetRestoredClientSizeInDips(_Out_ bool* pUseRestoredClientSize, _Out_ wf::Size* pValue);
        _Check_return_ HRESULT GetRestoredChromeSizeInPixels(_Out_ SIZE* pChromeSize);
        _Check_return_ HRESULT ValidateWidthHeightValue(DOUBLE value);
        // Only Default and Overlapped presenters support Window.Width/Height sizing.
        bool AppWindowPresenterSupportsSizing();
        // True when the live window currently represents its restored geometry: a sizing-capable
        // presenter (Default/Overlapped) and neither maximized nor minimized. In any other state the
        // live window rect is the maximized / minimized / full-screen / compact rect, not the
        // restored one. Both the restored-size capture and the ExtendsContentIntoTitleBar size
        // preservation only act when the window is in this state.
        bool IsInOverlappedRestoredState();
        // True once the app has set Width or Height at least once (opted into the feature).
        bool HasExplicitClientSize() const { return m_hasExplicitClientSize; }
        // Records the current client size as the window's restored size, but only when the live
        // window actually represents that restored size (see IsInOverlappedRestoredState). Invoked
        // from a deferred re-evaluation (ScheduleUpdateLastRestoredClientSize) so the presenter state
        // is settled (a presenter change resizes the window before AppWindow reports the change to us).
        void UpdateLastRestoredClientSize();
        // Schedules a deferred re-evaluation of the restored client size onto the dispatcher
        // queue (rather than a posted window message, so apps watching this window's messages don't
        // see it), coalescing the burst of WM_SIZE messages a drag produces via m_restoredSizeReevalPosted.
        void ScheduleUpdateLastRestoredClientSize();
        // Applies any Width/Height requested before the window was first shown
        // (e.g. from XAML markup), or while a non-sizing presenter was active. Called
        // on first activation and on presenter changes (OnAppWindowChanged).
        _Check_return_ HRESULT ApplyPendingClientSize();
        // Handles AppWindow.Changed so a Width/Height remembered while a non-sizing presenter
        // (FullScreen/CompactOverlay) was active is applied once we return to Default/Overlapped.
        _Check_return_ HRESULT OnAppWindowChanged(_In_ ixp::IAppWindow* sender, _In_ ixp::IAppWindowChangedEventArgs* args);
        
        void ResizeWindowToDesktopWindowXamlSourceWindowDimensions(WPARAM wParam, LPARAM lParam);
        void RepositionWindowToDesktopWindowXamlSourceWindowDimensions(WPARAM wParam, LPARAM lParam);
        void Shutdown();
        static _Check_return_ Window* GetMUXWindowFromHwnd(_In_ DXamlCore *dxamlCore, _In_ const HWND& hwnd);
        _Check_return_ HRESULT CheckIsWindowClosed();
        _Check_return_ HRESULT OnDesktopWindowXamlSourceTakeFocusRequested(_In_ xaml::Hosting::IDesktopWindowXamlSource* desktopWindowXamlSource, _In_ xaml::Hosting::IDesktopWindowXamlSourceTakeFocusRequestedEventArgs* args);
        _Check_return_ HRESULT RestoreFocus(_Outptr_ xaml_hosting::IXamlSourceFocusNavigationResult** ppEventSource);
        void SetFocusToContentIsland();

        // ------------------------------------
        //     Desktop-specific State
        // ------------------------------------
        bool m_bIsClosed = false;
        bool m_bIsClosing = false;
        Window* m_dxamlWindowInstance = nullptr;
        DXamlCore* m_dxamlCoreNoRef = nullptr;
        bool m_bMinimizedOrHidden = false;
        bool m_bInitialWindowActivation = true;

        // --------------------------------------------------
        // Window sizing support
        // --------------------------------------------------

        // True once the app has set Width or Height at least once.
        bool m_hasExplicitClientSize = false;

        // When the app has set Width/Height, but we haven't been able to honor them yet,
        // we store them here.
        std::optional<double> m_pendingClientWidthDips;
        std::optional<double> m_pendingClientHeightDips;

        // The window's last known restored client and chrome size in DIPs - the size it has,
        // or returns to, when it isn't maximized, minimized, or in a non-sizing presenter.
        std::optional<wf::Size> m_lastRestoredClientSizeDips;
        std::optional<wf::Size> m_lastRestoredChromeDips;

        // We've scheduled an update of the restored client size onto the dispatcher queue
        bool m_restoredSizeUpdateScheduled = false;

        // True while the user is interactively moving/resizing the window.
        bool m_inSizeMove = false;

        std::shared_ptr<bool> m_isWindowAlive = std::make_shared<bool>(true);

        EventRegistrationToken m_takeFocusRequestedEventToken;
        EventRegistrationToken m_appWindowChangedToken{};
        GUID m_lastTakeFocusRequestCorrelationId;
        HWND m_lastFocusedWindowHandle = NULL;
        HWND m_positioningBridgeWindowHandle = NULL;
        wrl::ComPtr<InputSiteHelper::IIslandInputSite> m_islandInputSite = nullptr;
        ctl::ComPtr<ixp::IAppWindowStatics> m_appWindowStatics;

        // We use ::GetClientRect to report the window bounds, but that returns 0x0 if the window is minimized. In
        // that case we'll cache the most recently reported bounds and return that.
        wf::Rect m_cachedWindowBounds {0};

        // We also get a WM_SIZE message whenever the window is minimized or restored. In those cases, we don't want
        // to raise a Window.SizeChanged event to match behavior on WPF and system Xaml. So cache the size of the last
        // Window.SizeChanged that we reported, and only raise SizeChanged if we have a different size. Combined with
        // using cached window bounds when minimized, we won't see a different size when the window is minimized or
        // restored.
        wf::Size m_previousWindowSizeChangedSize {0};

        // ------------------------------------
        //           Event Sources
        // ------------------------------------
        CFTMEventSource<wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*>, xaml::IWindow, xaml::IWindowSizeChangedEventArgs> m_sizeChangedEventSource;
        CFTMEventSource<wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>, xaml::IWindow, xaml::IWindowActivatedEventArgs> m_activatedEventSource;
        CFTMEventSource<wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>, xaml::IWindow, xaml::IWindowEventArgs> m_closedEventSource;
        CFTMEventSource<wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>, xaml::IWindow, xaml::IWindowVisibilityChangedEventArgs> m_visibilityChangedEventSource;

        // ------------------------------------
        //          Private Data
        // ------------------------------------
        ctl::ComPtr<xaml::IAtlasRequestCallback> m_atlasRequestCallback;
        ctl::ComPtr<DirectUI::WindowChrome> m_windowChrome;
        ctl::ComPtr<DirectUI::DesktopWindowXamlSource> m_desktopWindowXamlSource;
    };
}
