// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class AppBar;
    enum AppBarMode;
    enum AppBarTabPriority;

    interface __declspec(uuid("8d279331-de11-4267-abb8-911099ddc5c4")) IApplicationBarService : public IUnknown
    {
        virtual _Check_return_ HRESULT ClearCaches() = 0;

        virtual _Check_return_ HRESULT RegisterApplicationBar(_In_ AppBar* pApplicationBar, _In_ AppBarMode mode) = 0;

        virtual _Check_return_ HRESULT UnregisterApplicationBar(_In_ AppBar* pApplicationBar) = 0;

        virtual _Check_return_ HRESULT OnBoundsChanged(_In_ BOOLEAN inputPaneChange = FALSE) = 0;

        virtual _Check_return_ HRESULT OpenApplicationBar(_In_ AppBar* pAppBar, _In_ AppBarMode mode) = 0;

        virtual _Check_return_ HRESULT CloseApplicationBar(_In_ AppBar* pAppBar, _In_ AppBarMode mode) = 0;

        virtual _Check_return_ HRESULT HandleApplicationBarClosedDisplayModeChange(_In_ AppBar* pAppBar, _In_ AppBarMode mode) = 0;

        virtual _Check_return_ HRESULT CloseAllNonStickyAppBars() = 0;
        virtual _Check_return_ HRESULT CloseAllNonStickyAppBars(_Out_ bool* isAnyAppBarClosed) = 0;

        virtual _Check_return_ HRESULT UpdateDismissLayer() = 0;

        virtual _Check_return_ HRESULT ToggleApplicationBars() = 0;

        virtual _Check_return_ HRESULT SaveCurrentFocusedElement(_In_ AppBar* pAppBar) = 0;

        virtual _Check_return_ HRESULT FocusSavedElement(_In_ AppBar* pApplicationBar) = 0;

        virtual _Check_return_ HRESULT ProcessTabStopOverride(
            _In_opt_ DependencyObject* pFocusedElement,
            _In_opt_ DependencyObject* pCandidateTabStopElement,
            _In_ BOOLEAN isBackward,
            _Outptr_ DependencyObject** ppNewTabStop,
            _Out_ BOOLEAN* pIsTabStopOverridden) = 0;

        virtual _Check_return_ HRESULT FocusApplicationBar(
            _In_ AppBar* pAppBar,
            _In_ xaml::FocusState focusState) = 0;

        virtual void SetFocusReturnState(_In_ xaml::FocusState focusState) = 0;

        virtual void ResetFocusReturnState() = 0;

        virtual _Check_return_ HRESULT GetAppBarStatus(
            _Out_ bool* pIsTopOpen,
            _Out_ bool* pIsTopSticky,
            _Out_ XFLOAT* pTopWidth,
            _Out_ XFLOAT* pTopHeight,
            _Out_ bool* pIsBottomOpen,
            _Out_ bool* pIsBottomSticky,
            _Out_ XFLOAT* pBottomWidth,
            _Out_ XFLOAT* pBottomHeight) = 0;

        virtual _Check_return_ HRESULT ProcessToggleApplicationBarsFromMouseRightTapped() = 0;

        virtual _Check_return_ HRESULT GetTopAndBottomAppBars(
            _Outptr_ AppBar** ppTopAppBar,
            _Outptr_ AppBar** ppBottomAppBar) = 0;

        virtual _Check_return_ HRESULT GetTopAndBottomOpenAppBars(
            _Outptr_ AppBar** ppTopAppBar,
            _Outptr_ AppBar** ppBottomAppBar,
            _Out_ BOOLEAN* pIsAnyLightDismiss) = 0;

        virtual _Check_return_ HRESULT GetFirstFocusableElementFromAppBars(
            _In_opt_ AppBar* pTopAppBar,
            _In_opt_ AppBar* pBottomAppBar,
            _In_ AppBarTabPriority tabPriority,
            _In_ BOOLEAN startFromEnd,
            _Outptr_ CDependencyObject **ppNewTabStop) = 0;

    }; // interface IApplicationBarService

    class ApplicationBarServiceStatics
    {
    public:
        static _Check_return_ HRESULT GetAppBarStatus(
            _In_ CDependencyObject* object,
            _Out_ bool* pIsTopOpen,
            _Out_ bool* pIsTopSticky,
            _Out_ XFLOAT* pTopWidth,
            _Out_ XFLOAT* pTopHeight,
            _Out_ bool* pIsBottomOpen,
            _Out_ bool* pIsBottomSticky,
            _Out_ XFLOAT* pBottomWidth,
            _Out_ XFLOAT* pBottomHeight);

        static _Check_return_ HRESULT ProcessToggleApplicationBarsFromMouseRightTapped(_In_ IInspectable* xamlRootInspectable);

    }; // class ApplicationBarServiceStatics

} // namespace DirectUI
