// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Window.g.h"
#include "JupiterWindow.h"
#include "ContentManager.h"
#include <WindowImpl.h>

namespace DirectUI
{
    class CoreWindowWrapper;

    class UWPWindowImpl : public WindowImpl
    {

    public:
        // ---------------------------------------------------------------
        // UWPWindowImpl
        // ---------------------------------------------------------------
        UWPWindowImpl(Window* owningWindow);
        ~UWPWindowImpl();

        // ---------------------------------------------------------------
        // public Window API: IWindow
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

        _Check_return_ HRESULT get_ExtendsContentIntoTitleBarImpl(_Out_ BOOLEAN* pValue) final;
        _Check_return_ HRESULT put_ExtendsContentIntoTitleBarImpl(_In_ BOOLEAN value) final;
        _Check_return_ HRESULT SetTitleBarImpl(_In_ xaml::IUIElement* pTitleBar) final;

        _Check_return_ HRESULT get_CompositorImpl(_Outptr_result_maybenull_ WUComp::ICompositor** compositor) override;
        _Check_return_ HRESULT get_AppWindowImpl(_Outptr_result_maybenull_ ixp::IAppWindow** ppValue) final;

        _Check_return_ HRESULT get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop** systemBackdrop) override;
        _Check_return_ HRESULT put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop* systemBackdrop) override;

        // Windows::UI::Composition::ICompositionSupportsSystemBackdrop implementation
        IFACEMETHOD(get_SystemBackdrop)(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush) override;
        IFACEMETHOD(put_SystemBackdrop)(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush) override;

        // pubic Window API: IWindowNative
        IFACEMETHOD(get_WindowHandle)(_Out_ HWND* pValue) override;

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

    private:

        _Check_return_ HRESULT OnCoreWindowSizeChanged(_In_ wuc::ICoreWindow* pSender, _In_ wuc::IWindowSizeChangedEventArgs* pArgs);
        _Check_return_ HRESULT OnApplicationViewVisibleBoundsChanged(_In_ wuv::IApplicationView* pSender, _In_ IInspectable* pArgs);
        _Check_return_ HRESULT OnCoreWindowVisibleBoundsChanged(_In_ wuc::ICoreWindow* pSender, _In_ IInspectable* pArgs);
        _Check_return_ HRESULT OnCoreWindowClosed(_In_ wuc::ICoreWindow* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT UpdateApplicationBarServiceBounds();

        ContentManager m_contentManager;
        CoreWindowWrapper* m_pCoreWindowWrapper;
        EventRegistrationToken m_tokAppViewVisibleBoundsChanged;
        EventRegistrationToken m_tokCoreWindowSizeChanged;
        EventRegistrationToken m_tokCoreWindowVisibleBoundsChanged;
        EventRegistrationToken m_tokCoreWindowClosed;
        bool m_coreWindowClosed;
        CFTMEventSource<wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*>, xaml::IWindow, xaml::IWindowSizeChangedEventArgs> m_SizeChangedEventSource;
        wf::Rect m_layoutBounds{};

        ctl::ComPtr<wuv::IApplicationView2> m_spApplicationView2;
        BOOLEAN m_useApplicationView = TRUE;

        ctl::ComPtr<IUnknown> m_spTitleBarDCompVisual; // Reference to the DComp visual used as a custom title bar.
        ctl::WeakRefPtr m_wrTitleBarUIElement; // Weak reference to the UIElement used as a custom title bar.

        ctl::ComPtr<xaml::IAtlasRequestCallback> m_atlasRequestCallback;

        DXamlCore* m_pDXamlCoreNoRef;

        // Data for xaml test hooks
        BOOLEAN m_hasSizeOverrides;
        UINT32 m_widthOverride;
        UINT32 m_heightOverride;
        bool m_shrinkApplicationViewVisibleBounds;
        wf::Rect m_layoutBoundsOverrides = {};
        wf::Rect m_visibleBoundsOverrides = {};

        // NoRef pointer to WinRT Window object that holds an instance of UWPWindowImpl
        Window* m_parentWindow;
    };
}
