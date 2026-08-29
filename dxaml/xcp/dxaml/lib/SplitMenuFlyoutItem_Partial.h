// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuFlyout.g.h"
#include "SplitMenuFlyoutItem.g.h"
#include "CascadingMenuHelper.h"
#include "MenuFlyoutKeyPressProcess.h"

namespace DirectUI
{
    class ButtonBase;

    PARTIAL_CLASS(SplitMenuFlyoutItem)
    {
        friend class SplitMenuFlyoutItemAutomationPeer;

        friend class KeyPress::MenuFlyout;

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
        
        // Called by MenuFlyoutPresenter to indicate upward navigation focus
        void SetFocusComingFromUpwardNavigation() { m_focusComingFromUpwardNavigation = true; }

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
        SplitMenuFlyoutItem();

        _Check_return_ HRESULT PrepareState() override;
        _Check_return_ HRESULT DisconnectFrameworkPeerCore() override;

        IFACEMETHOD(OnApplyTemplate)() override;

        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* args) override;

        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* args) override;

        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* args) override;

        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs* args) override;

        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* args) override;

        IFACEMETHOD(OnKeyUp)(
            _In_ xaml_input::IKeyRoutedEventArgs* args) override;

        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

        // Handle the custom property changed event and call the OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        _Check_return_ HRESULT OnIsEnabledChanged(
            _In_ DirectUI::IsEnabledChangedEventArgs* args) override;

        _Check_return_ HRESULT OnVisibilityChanged() override;

        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

    private:
        static _Check_return_ HRESULT SetPresenterStyle(
            _In_ xaml_controls::IControl* pPresenter,
            _In_opt_ xaml::IStyle* pStyle);
        
        // Apply SubMenuItemStyle to all items in the collection
        _Check_return_ HRESULT ApplySubMenuItemStyleToItems();
        
        // Apply SubMenuItemStyle to a single item (checks if style is already set locally)
        _Check_return_ HRESULT ApplySubMenuItemStyleToItem(
            _In_ xaml_controls::IMenuFlyoutItemBase* pItem,
            _In_opt_ xaml::IStyle* pStyle);

        _Check_return_ HRESULT RefreshItemsSource();

        _Check_return_ HRESULT HookTemplate();
        _Check_return_ HRESULT UnhookTemplate();
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
        _Check_return_ HRESULT EnsureCloseExistingSubItems();

    private:
        // Template part names
        static const WCHAR c_primaryButtonName[];
        static const WCHAR c_secondaryButtonName[];

        // Collection of the sub menu item
        TrackerPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> m_tpItems;

        // Popup for the SplitMenuFlyoutItem
        TrackerPtr<xaml_primitives::IPopup> m_tpPopup;

        // Presenter for the SplitMenuFlyoutItem
        TrackerPtr<xaml_controls::IControl> m_tpPresenter;

        // In Threshold, MenuFlyout uses the MenuPopupThemeTransition.
        TrackerPtr<xaml_animation::ITransition> m_tpMenuPopupThemeTransition;

        // Event pointer for the Loaded event
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epLoadedHandler;

        // Event pointer for the size changed on the SplitMenuFlyoutItem's presenter
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epPresenterSizeChangedHandler;

        // Helper to which to delegate cascading menu functionality.
        TrackerPtr<CascadingMenuHelper> m_menuHelper;

        // Weak reference the parent that owns the menu that this item belongs to.
        ctl::WeakRefPtr m_wrParentOwner;

        // Template parts
        TrackerPtr<xaml_primitives::IButtonBase> m_tpPrimaryButton;
        TrackerPtr<xaml_primitives::IButtonBase> m_tpSecondaryButton;

        // Event handlers for template parts
        _Check_return_ HRESULT OnPrimaryButtonPointerEntered(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* args);
        _Check_return_ HRESULT OnPrimaryButtonPointerExited(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* args);
        _Check_return_ HRESULT OnSecondaryButtonPointerEntered(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* args);
        _Check_return_ HRESULT OnSecondaryButtonPointerExited(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* args);
        _Check_return_ HRESULT OnPrimaryButtonClick(_In_ IInspectable* sender, _In_ xaml::IRoutedEventArgs* args);
        _Check_return_ HRESULT OnSecondaryButtonClick(_In_ IInspectable* sender, _In_ xaml::IRoutedEventArgs* args);

        // Helper methods for focus management
        _Check_return_ HRESULT SetButtonFocus(_In_ bool focusSecondaryButton, _In_ xaml::FocusState focusState);
        _Check_return_ HRESULT ResetButtonStates();
        _Check_return_ HRESULT HasButtonFocus(_In_ const TrackerPtr<xaml_primitives::IButtonBase>& button, _Out_ bool* hasFocus);
        
        // Helper methods to check button focus state directly from buttons
        _Check_return_ HRESULT HasPrimaryButtonFocus(_Out_ bool* hasFocus);
        _Check_return_ HRESULT HasSecondaryButtonFocus(_Out_ bool* hasFocus);

        // Helper method for setting secondary button's automation name
        _Check_return_ HRESULT SetSecondaryButtonAutomationName(bool isSubMenuOpen);

        ctl::EventPtr<UIElementPointerEnteredEventCallback> m_epPrimaryButtonPointerEnteredHandler;
        ctl::EventPtr<UIElementPointerExitedEventCallback> m_epPrimaryButtonPointerExitedHandler;
        ctl::EventPtr<UIElementPointerEnteredEventCallback> m_epSecondaryButtonPointerEnteredHandler;
        ctl::EventPtr<UIElementPointerExitedEventCallback> m_epSecondaryButtonPointerExitedHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epPrimaryButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epSecondaryButtonClickHandler;
        ctl::EventPtr<UIElementGotFocusEventCallback> m_epPrimaryButtonGotFocusHandler;
        ctl::EventPtr<UIElementLostFocusEventCallback> m_epPrimaryButtonLostFocusHandler;
        ctl::EventPtr<UIElementGotFocusEventCallback> m_epSecondaryButtonGotFocusHandler;
        ctl::EventPtr<UIElementLostFocusEventCallback> m_epSecondaryButtonLostFocusHandler;

        bool m_itemsSourceRefreshPending{ false };
        bool m_focusComingFromSubmenu{ false };
        bool m_focusComingFromUpwardNavigation{ false };
        bool m_hasSecondaryButtonCustomAutomationName{ false };
    };
}
