// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Popup.g.h"

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    class __declspec(novtable) CascadingMenuHelper : public ctl::WeakReferenceSource
    {
    public:
        CascadingMenuHelper();

        ~CascadingMenuHelper() override;

        _Check_return_ HRESULT Initialize(
            _In_ xaml::IFrameworkElement* owner);

        _Check_return_ HRESULT SetSubMenuPresenter(
            _In_ xaml::IFrameworkElement* subMenuPresenter);

        _Check_return_ HRESULT OpenSubMenu();

        _Check_return_ HRESULT CloseSubMenu();

        _Check_return_ HRESULT CloseChildSubMenus();

        _Check_return_ HRESULT DelayCloseSubMenu();

        _Check_return_ HRESULT CancelCloseSubMenu();

        _Check_return_ HRESULT ClearStateFlags();

        _Check_return_ HRESULT OnApplyTemplate();

        _Check_return_ HRESULT OnPointerEntered(
            _In_ xaml_input::IPointerRoutedEventArgs* args);

        _Check_return_ HRESULT OnPointerExited(
            _In_ xaml_input::IPointerRoutedEventArgs* args,
            bool parentIsSubMenu);

        _Check_return_ HRESULT OnPointerPressed(
            _In_ xaml_input::IPointerRoutedEventArgs* args);

        _Check_return_ HRESULT OnPointerReleased(
            _In_ xaml_input::IPointerRoutedEventArgs* args);

        _Check_return_ HRESULT OnGotFocus(
            _In_ xaml::IRoutedEventArgs* args);

        _Check_return_ HRESULT OnLostFocus(
            _In_ xaml::IRoutedEventArgs* args);

        _Check_return_ HRESULT OnKeyDown(
            _In_ xaml_input::IKeyRoutedEventArgs* args);

        _Check_return_ HRESULT OnKeyUp(
            _In_ xaml_input::IKeyRoutedEventArgs* args);

        _Check_return_ HRESULT OnIsEnabledChanged(
            _In_ DirectUI::IsEnabledChangedEventArgs* args);

        _Check_return_ HRESULT OnVisibilityChanged();

        _Check_return_ HRESULT OnPresenterSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* args,
            _In_ Popup* popup);

        _Check_return_ HRESULT IsDelayCloseTimerRunning(
            _Out_ BOOLEAN* pValue);

        bool IsPointerOver() { return m_isPointerOver; }
        bool IsPressed() { return m_isPressed; }

    private:
        _Check_return_ HRESULT GetPositionAndDirection(
            float presenterWidth,
            float presenterHeight,
            _In_ Popup* popup,
            _Out_ wf::Point& subMenuPosition,
            _Out_ bool* isSubMenuDirectionUp,
            _Out_ bool* positionAndDirectionSet);

        _Check_return_ HRESULT GetPositionAndDirection(
            float presenterWidth,
            float presenterHeight,
            const wf::Rect& availableBounds,
            const wf::Point& ownerPosition,
            _Out_ wf::Point& subMenuPosition,
            _Out_ bool* isSubMenuDirectionUp,
            _Out_ bool* positionAndDirectionSet);
            
        // Creates a DispatcherTimer for delaying showing the sub menu flyout
        _Check_return_ HRESULT EnsureDelayOpenMenuTimer();

        // Creates a DispatcherTimer for delaying hiding the sub menu flyout
        _Check_return_ HRESULT EnsureDelayCloseMenuTimer();

        // Handler for the Tick event on delay open timer.
        _Check_return_ HRESULT DelayOpenMenuTimerTickHandler(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2);

        // Handler for the Tick event on the delay close timer.
        _Check_return_ HRESULT DelayCloseMenuTimerTickHandler(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2);

        // Ensure that any currently open cascading menus are closed
        _Check_return_ HRESULT EnsureCloseExistingSubItems();

        _Check_return_ HRESULT UpdateOwnerVisualState();

    private:
        // The overlapped menu pixels between the main main menu presenter and the sub presenter
        static constexpr UINT m_subMenuOverlapPixels = 4;

        INT32 m_subMenuShowDelay;

        // Owner of the cascading menu
        ctl::WeakRefPtr m_wpOwner;

        // Presenter of the sub-menu
        ctl::WeakRefPtr m_wpSubMenuPresenter;

        // Event pointer for the Loaded event
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_loadedHandler;

        // Dispatcher timer to delay showing the sub menu flyout
        TrackerPtr<xaml::IDispatcherTimer> m_delayOpenMenuTimer;

        // Dispatcher timer to delay hiding the sub menu flyout
        TrackerPtr<xaml::IDispatcherTimer> m_delayCloseMenuTimer;

        // Indicate the pointer is over the cascading menu owner
        bool m_isPointerOver : 1;

        // Indicate the pointer is pressed on the cascading menu owner
        bool m_isPressed : 1;
    };
}
