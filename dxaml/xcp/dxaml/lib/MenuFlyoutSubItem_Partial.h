// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a MenuFlyoutSubItem.
//  Implements a MenFlyoutSubItem that supports the cascade menu style and
//  the sub menu interaction between the menu item and the MenuFlyout.

#pragma once

#include "MenuFlyout.g.h"
#include "MenuFlyoutSubItem.g.h"
#include "CascadingMenuHelper.h"

namespace DirectUI
{
    PARTIAL_CLASS(MenuFlyoutSubItem)
    {
        friend class MenuFlyoutSubItemAutomationPeer;

    public:
        Popup* GetPopup()
        {
            return m_tpPopup.Cast<Popup>();
        }

        Control* GetMenuFlyoutPresenter()
        {
            return m_tpPresenter.Cast<Control>();
        }

        _Check_return_ HRESULT Open();
        _Check_return_ HRESULT Close();

        // ISubMenuOwner implementation
        _Check_return_ HRESULT get_IsSubMenuOpenImpl(_Out_ BOOLEAN* pValue);

        _Check_return_ HRESULT get_IsSubMenuPositionedAbsolutelyImpl(_Out_ BOOLEAN* pValue)
        {
            *pValue = TRUE;
            return S_OK;
        }

        _Check_return_ HRESULT get_ParentOwnerImpl(_Outptr_ xaml_controls::ISubMenuOwner** ppValue);
        _Check_return_ HRESULT put_ParentOwnerImpl(_In_ xaml_controls::ISubMenuOwner* pValue);

        _Check_return_ HRESULT SetSubMenuDirectionImpl(BOOLEAN isSubMenuDirectionUp);
        _Check_return_ HRESULT PrepareSubMenuImpl();
        _Check_return_ HRESULT OpenSubMenuImpl(wf::Point position);
        _Check_return_ HRESULT PositionSubMenuImpl(wf::Point position);
        _Check_return_ HRESULT ClosePeerSubMenusImpl();
        _Check_return_ HRESULT CloseSubMenuImpl();
        _Check_return_ HRESULT CloseSubMenuTreeImpl();
        _Check_return_ HRESULT DelayCloseSubMenuImpl();
        _Check_return_ HRESULT CancelCloseSubMenuImpl();
        _Check_return_ HRESULT RaiseAutomationPeerExpandCollapseImpl(_In_ BOOLEAN isOpen);

        _Check_return_ HRESULT QueueRefreshItemsSource();

    protected:
        MenuFlyoutSubItem();

        IFACEMETHOD(OnApplyTemplate)() override;

        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* args) override;

        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* args) override;

        IFACEMETHOD(OnPointerPressed)(
            _In_ xaml_input::IPointerRoutedEventArgs* args) override;

        IFACEMETHOD(OnPointerReleased)(
            _In_ xaml_input::IPointerRoutedEventArgs* args) override;

        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* args) override;

        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs* args) override;

        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* args) override;

        IFACEMETHOD(OnKeyUp)(
            _In_ xaml_input::IKeyRoutedEventArgs* args) override;

        _Check_return_ HRESULT PrepareState() override;

        _Check_return_ HRESULT DisconnectFrameworkPeerCore() override;

        _Check_return_ HRESULT ChangeVisualState(
            _In_ bool bUseTransitions) override;

        _Check_return_ HRESULT OnIsEnabledChanged(
            _In_ DirectUI::IsEnabledChangedEventArgs* args) override;

        _Check_return_ HRESULT OnVisibilityChanged() override;

        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;
        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

    private:
        _Check_return_ HRESULT CreateSubPresenter(
            _Outptr_ xaml_controls::IControl** ppReturnValue);

        _Check_return_ HRESULT EnsurePopupAndPresenter();

        _Check_return_ HRESULT UpdateParentOwner(
            _In_opt_ MenuFlyoutPresenter* parentMenuFlyoutPresenter);

        _Check_return_ HRESULT SetIsOpen(_In_ BOOLEAN isOpen);

        _Check_return_ HRESULT ClearStateFlags();

        _Check_return_ HRESULT OnPresenterSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* args) noexcept;

        _Check_return_ HRESULT ForwardPresenterProperties(
            _In_ MenuFlyout* pOwnerMenuFlyout,
            _In_ MenuFlyoutPresenter* pParentMenuFlyoutPresenter,
            _In_ MenuFlyoutPresenter* pSubMenuFlyoutPresenter);

        _Check_return_ HRESULT ForwardSystemBackdropToPopup(_In_ MenuFlyout* pOwnerMenuFlyout);

        _Check_return_ HRESULT get_IsOpen(_Out_ BOOLEAN *pIsOpen);

        // Ensure that any currently open MenuFlyoutSubItems are closed
        _Check_return_ HRESULT EnsureCloseExistingSubItems();

        _Check_return_ HRESULT RefreshItemsSource();

    private:
        // Collection of the sub menu item
        TrackerPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> m_tpItems;

        // Popup for the MenuFlyoutSubItem
        TrackerPtr<xaml_primitives::IPopup> m_tpPopup;

        // Presenter for the MenuFlyoutSubItem
        TrackerPtr<xaml_controls::IControl> m_tpPresenter;

        // In Threshold, MenuFlyout uses the MenuPopupThemeTransition.
        TrackerPtr<xaml_animation::ITransition> m_tpMenuPopupThemeTransition;

        // Event pointer for the Loaded event
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epLoadedHandler;

        // Event pointer for the size changed on the MenuFlyoutSubItem's presenter
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epPresenterSizeChangedHandler;

        // Helper to which to delegate cascading menu functionality.
        TrackerPtr<CascadingMenuHelper> m_menuHelper;

        // Weak reference the parent that owns the menu that this item belongs to.
        ctl::WeakRefPtr m_wrParentOwner;

        bool m_itemsSourceRefreshPending{ false };
    };
}
