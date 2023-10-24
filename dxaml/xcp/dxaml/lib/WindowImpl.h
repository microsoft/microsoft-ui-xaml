// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Window.g.h"
#include "JupiterWindow.h" // Required for WindowType

namespace DirectUI
{
    class WindowImpl
    {
        public:
            // ---------------------------------------------------------------
            // public Window API: IWindow
            // ---------------------------------------------------------------
            virtual _Check_return_ HRESULT get_BoundsImpl(_Out_ wf::Rect* pValue) = 0;
            virtual _Check_return_ HRESULT get_VisibleImpl(_Out_ BOOLEAN* pValue) = 0;
            virtual _Check_return_ HRESULT get_ContentImpl(_Outptr_result_maybenull_ xaml::IUIElement** pValue) = 0;
            virtual _Check_return_ HRESULT put_ContentImpl(_In_opt_ xaml::IUIElement* value) = 0;
            virtual _Check_return_ HRESULT get_CoreWindowImpl(_Outptr_result_maybenull_ wuc::ICoreWindow** pValue) = 0;
            virtual _Check_return_ HRESULT get_DispatcherImpl(_Outptr_result_maybenull_ wuc::ICoreDispatcher** pValue) = 0;
            virtual _Check_return_ HRESULT get_TitleImpl(_Out_ HSTRING* pValue) = 0;
            virtual _Check_return_ HRESULT put_TitleImpl(_In_opt_ HSTRING value) = 0;
            IFACEMETHOD(add_Activated)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken)  = 0;
            IFACEMETHOD(remove_Activated)(_In_ EventRegistrationToken tToken) = 0;
            IFACEMETHOD(add_Closed)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) = 0;
            IFACEMETHOD(remove_Closed)(_In_ EventRegistrationToken tToken) = 0;
            IFACEMETHOD(add_SizeChanged)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*> * pValue, _Out_ EventRegistrationToken* ptToken) = 0;
            IFACEMETHOD(remove_SizeChanged)(_In_ EventRegistrationToken tToken) = 0;
            IFACEMETHOD(add_VisibilityChanged)(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken)  = 0;
            IFACEMETHOD(remove_VisibilityChanged)(_In_ EventRegistrationToken tToken) = 0;
            virtual _Check_return_ HRESULT ActivateImpl() = 0;
            virtual _Check_return_ HRESULT CloseImpl() = 0;

            virtual _Check_return_ HRESULT get_ExtendsContentIntoTitleBarImpl(_Out_ BOOLEAN* pValue) = 0;
            virtual _Check_return_ HRESULT put_ExtendsContentIntoTitleBarImpl(_In_ BOOLEAN value) = 0;
            virtual _Check_return_ HRESULT SetTitleBarImpl(_In_ xaml::IUIElement* pTitleBar) = 0;

            virtual _Check_return_ HRESULT get_CompositorImpl(_Outptr_result_maybenull_ WUComp::ICompositor** compositor) = 0;
            virtual _Check_return_ HRESULT get_AppWindowImpl(_Outptr_result_maybenull_ ixp::IAppWindow** ppValue) = 0;

            virtual _Check_return_ HRESULT get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop** systemBackdrop) = 0;
            virtual _Check_return_ HRESULT put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop* systemBackdrop) = 0;

            // IWindowNative
            IFACEMETHOD(get_WindowHandle)(_Out_ HWND* pValue) = 0;

            // Windows::UI::Composition::ICompositionSupportsSystemBackdrop implementation
            IFACEMETHOD(get_SystemBackdrop)(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush) = 0;
            IFACEMETHOD(put_SystemBackdrop)(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush) = 0;

            // -------------------------------------------------
            // IWindowPrivate
            // -------------------------------------------------
            virtual _Check_return_ HRESULT get_TransparentBackgroundImpl(_Out_ BOOLEAN* pValue) = 0;
            virtual _Check_return_ HRESULT put_TransparentBackgroundImpl(_In_ BOOLEAN value) = 0;
            virtual _Check_return_ HRESULT ShowImpl() = 0;
            virtual _Check_return_ HRESULT HideImpl() = 0;
            virtual _Check_return_ HRESULT MoveWindowImpl(_In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height) = 0;
            virtual _Check_return_ HRESULT SetAtlasSizeHintImpl(UINT width, UINT height) = 0;
            virtual _Check_return_ HRESULT ReleaseGraphicsDeviceOnSuspendImpl(BOOLEAN enable) = 0;
            virtual _Check_return_ HRESULT SetAtlasRequestCallbackImpl(_In_opt_ xaml::IAtlasRequestCallback* callback) = 0;
            virtual _Check_return_ HRESULT GetWindowContentBoundsForElementImpl(_In_ xaml::IDependencyObject* element, _Out_ wf::Rect* rect) = 0;

            // --------------------------------------------------
            // internal instance methods
            // --------------------------------------------------
            virtual bool HasBounds() = 0;
            virtual _Check_return_ HRESULT SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow) = 0;
            virtual _Check_return_ HRESULT GetVisibleBounds(_In_ bool ignoreIHM, _In_ bool inDesktopCoordinates, _Out_ wf::Rect* pValue) = 0;
            virtual _Check_return_ HRESULT GetRootScrollViewer(_Outptr_result_maybenull_ xaml_controls::IScrollViewer** pRootScrollViewer) = 0;
            virtual _Check_return_ HRESULT OnWindowSizeChanged() = 0;
            virtual WindowType::Enum GetWindowType() = 0;
            virtual _Check_return_ HRESULT GetLayoutBounds(_Out_  wf::Rect* pLayoutBounds) = 0;
            virtual _Check_return_ HRESULT GetLayoutBounds(_Out_  XRECTF* pLayoutBoundsRect) = 0;
            virtual DXamlCore* GetDXamlCore() = 0;
            virtual void SetDXamlCore(_In_opt_ DXamlCore* pDXamlCore) = 0;
            virtual _Check_return_ HRESULT EnsureInitializedForIslands() = 0;
            virtual bool ShouldShrinkApplicationViewVisibleBounds() const = 0;

            virtual ctl::ComPtr<xaml::IAtlasRequestCallback> GetAtlasRequestCallback() = 0;

            // --------------------------------------------------
            // DXamlTestHooks
            // --------------------------------------------------
            virtual UINT32 GetWidthOverride() = 0;
            virtual void SetWidthOverride(UINT32 width) = 0;
            virtual UINT32 GetHeightOverride() = 0;
            virtual void SetHeightOverride(UINT32 height) = 0;
            virtual wf::Rect GetLayoutBoundsOverrides() = 0;
            virtual void SetLayoutBoundsOverrides(const wf::Rect& rect) = 0;
            virtual wf::Rect& GetVisibleBoundsOverrides() = 0;
            virtual void SetVisibleBoundsOverrides(const wf::Rect& rect) = 0;
            virtual void SetHasSizeOverrides(bool hasSizeOverrides) = 0;
            virtual void SetShrinkApplicationViewVisibleBounds(bool enabled) = 0;

    };
}
