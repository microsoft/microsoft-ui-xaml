// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Window.g.h"
#include <WindowImpl.h>
#include <Microsoft.UI.Xaml.Window.h>

namespace DirectUI
{
    PARTIAL_CLASS(Window),
         public IWindowNative
    {

        BEGIN_INTERFACE_MAP(Window, WindowGenerated)
            INTERFACE_ENTRY(Window, IWindowNative)
        END_INTERFACE_MAP(Window, WindowGenerated)

    public:
        // public Window API: IWindow
        Window();
        _Check_return_ HRESULT get_BoundsImpl(_Out_ wf::Rect* pValue);
        _Check_return_ HRESULT get_VisibleImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT get_ContentImpl(_Outptr_result_maybenull_ xaml::IUIElement** pValue);
        _Check_return_ HRESULT put_ContentImpl(_In_opt_ xaml::IUIElement* value);
        _Check_return_ HRESULT get_CoreWindowImpl(_Outptr_result_maybenull_ wuc::ICoreWindow** pValue);
        _Check_return_ HRESULT get_DispatcherImpl(_Outptr_result_maybenull_ wuc::ICoreDispatcher** pValue);
        _Check_return_ HRESULT get_TitleImpl(_Out_ HSTRING* pValue);
        _Check_return_ HRESULT put_TitleImpl(_In_opt_ HSTRING value);
        IFACEMETHOD(add_Activated)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_Activated)(_In_ EventRegistrationToken tToken) override;
        IFACEMETHOD(add_Closed)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_Closed)(_In_ EventRegistrationToken tToken) override;
        IFACEMETHOD(add_SizeChanged)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*> * pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_SizeChanged)(_In_ EventRegistrationToken tToken) override;
        IFACEMETHOD(add_VisibilityChanged)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_VisibilityChanged)(_In_ EventRegistrationToken tToken) override;
        _Check_return_ HRESULT ActivateImpl();
        _Check_return_ HRESULT CloseImpl();

        _Check_return_ HRESULT get_ExtendsContentIntoTitleBarImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_ExtendsContentIntoTitleBarImpl(_In_ BOOLEAN value);
        _Check_return_ HRESULT SetTitleBarImpl(_In_opt_ xaml::IUIElement* pTitleBar);

        _Check_return_ HRESULT get_DispatcherQueueImpl(_Outptr_result_maybenull_ msy::IDispatcherQueue** ppValue);
        _Check_return_ HRESULT get_CompositorImpl(_Outptr_result_maybenull_ WUComp::ICompositor** compositor);
        _Check_return_ HRESULT get_AppWindowImpl(_Outptr_result_maybenull_ ixp::IAppWindow** ppValue);

        _Check_return_ HRESULT get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop** iSystemBackdrop);
        _Check_return_ HRESULT put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop* iSystemBackdrop);

        // IWindowPrivate
        _Check_return_ HRESULT get_TransparentBackgroundImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_TransparentBackgroundImpl(_In_ BOOLEAN value);
        _Check_return_ HRESULT ShowImpl();
        _Check_return_ HRESULT HideImpl();
        _Check_return_ HRESULT MoveWindowImpl(_In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height);
        _Check_return_ HRESULT SetAtlasSizeHintImpl(UINT width, UINT height);
        _Check_return_ HRESULT ReleaseGraphicsDeviceOnSuspendImpl(BOOLEAN enable);
        _Check_return_ HRESULT SetAtlasRequestCallbackImpl(_In_opt_ xaml::IAtlasRequestCallback* callback);
        _Check_return_ HRESULT SetSynchronizationInfoImpl(UINT64 compSyncObject, UINT64 commitResizeWindow);
        _Check_return_ HRESULT GetWindowContentBoundsForElementImpl(_In_ xaml::IDependencyObject* element, _Out_ wf::Rect* rect);

        // pubic Window API: IWindowNative
        IFACEMETHOD(get_WindowHandle)(_Out_ HWND* pValue) override;
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        // Windows::UI::Composition::ICompositionSupportsSystemBackdrop implementation
        IFACEMETHOD(get_SystemBackdrop)(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush) override;
        IFACEMETHOD(put_SystemBackdrop)(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush) override;

        // --------------------------------------------------
        // internal static methods
        // --------------------------------------------------
        static _Check_return_ HRESULT Create(_In_ DXamlCore *pDXamlCore, _Outptr_ Window** ppWindow);
        static _Check_return_ HRESULT GetContentRootBounds(_In_ CDependencyObject* pObject, _Out_ XRECTF* pContentRootBounds);
        static _Check_return_ HRESULT GetContentRootLayoutBounds(_In_ CDependencyObject* pObject, _Out_ XRECTF* pContentRootBounds);
        static _Check_return_ HRESULT GetRootScrollViewer(_Outptr_result_maybenull_ CDependencyObject** ppRootScrollViewer);
        static bool AtlasRequest(uint32_t width, uint32_t height, PixelFormat pixelFormat);

        static _Check_return_ HRESULT ShrinkApplicationViewVisibleBounds(_In_ Window* pCurrentWindow, _Inout_ wf::Rect* pApplicationViewVisibleBounds);

        // --------------------------------------------------
        // internal instance methods
        // --------------------------------------------------
        bool HasBounds();
        _Check_return_ HRESULT SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow);
        _Check_return_ HRESULT GetVisibleBounds(_In_ bool ignoreIHM, _In_ bool inDesktopCoordinates, _Out_ wf::Rect* pValue);
        _Check_return_ HRESULT GetRootScrollViewer(_Outptr_result_maybenull_ xaml_controls::IScrollViewer** pRootScrollViewer);
        _Check_return_ HRESULT OnWindowSizeChanged();
        WindowType::Enum GetWindowType();
        _Check_return_ HRESULT GetLayoutBounds(_Out_  wf::Rect* pLayoutBounds);
        _Check_return_ HRESULT GetLayoutBounds(_Out_  XRECTF* pLayoutBoundsRect);
        DXamlCore* GetDXamlCore();
        void SetDXamlCore(_In_opt_ DXamlCore* pDXamlCore);
        _Check_return_ HRESULT EnsureInitializedForIslands();
        bool ShouldShrinkApplicationViewVisibleBounds() const;

        ctl::ComPtr<xaml::IAtlasRequestCallback> GetAtlasRequestCallback();
        _Check_return_ HRESULT ProcessMessage(_In_ MSG& msg, _Out_ bool* pHandled);
        DirectUI::WindowImpl* GetWindowImpl();
        void UnPeg();

        // --------------------------------------------------
        // DXamlTestHooks
        // --------------------------------------------------
        UINT32 GetWidthOverride();
        void SetWidthOverride(UINT32 width);
        UINT32 GetHeightOverride();
        void SetHeightOverride(UINT32 height);
        wf::Rect GetLayoutBoundsOverrides();
        void SetLayoutBoundsOverrides(const wf::Rect& rect);
        wf::Rect& GetVisibleBoundsOverrides();
        void SetVisibleBoundsOverrides(const wf::Rect& rect);
        void SetHasSizeOverrides(bool hasSizeOverrides);
        void SetShrinkApplicationViewVisibleBounds(bool enabled);

    private:
        std::shared_ptr<WindowImpl> m_spWindowImpl;
        // Flag is used to balance peg/ unpeg DesktopWindow only.
        bool m_peggedForHWNDLifetime{ false };
    };
}

