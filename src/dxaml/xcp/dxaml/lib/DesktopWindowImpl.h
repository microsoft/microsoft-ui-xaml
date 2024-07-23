// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

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
        IFACEMETHOD(remove_Activated)(_In_ EventRegistrationToken tToken) override;
        IFACEMETHOD(add_Closed)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_Closed)(_In_ EventRegistrationToken tToken) override;
        IFACEMETHOD(add_SizeChanged)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*> * pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_SizeChanged)(_In_ EventRegistrationToken tToken) override;
        IFACEMETHOD(add_VisibilityChanged)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_VisibilityChanged)(_In_ EventRegistrationToken tToken) override;
        _Check_return_ HRESULT ActivateImpl() override;
        _Check_return_ HRESULT CloseImpl() override;

        _Check_return_ HRESULT get_CompositorImpl(_Outptr_result_maybenull_ WUComp::ICompositor** compositor) override;

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
        EventRegistrationToken m_takeFocusRequestedEventToken;
        GUID m_lastTakeFocusRequestCorrelationId;
        HWND m_lastFocusedWindowHandle = NULL;
        HWND m_positioningBridgeWindowHandle = NULL;
        wrl::ComPtr<ixp::IIslandInputSitePartner> m_islandInputSite = nullptr;
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

