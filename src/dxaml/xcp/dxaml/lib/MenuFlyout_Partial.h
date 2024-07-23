// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuFlyout.g.h"
#include "Rectangle.g.h"
#include "Storyboard.g.h"
#include "MenuPopupThemeTransition.g.h"
#include "FlyoutBase.g.h"

// This the fallback is used if we fail to retrieve a value from the MenuShowDelay RegKey
static const XINT32 DefaultMenuShowDelay = 400; // in milliseconds

namespace DirectUI
{
    class Popup;

    PARTIAL_CLASS(MenuFlyout)
    {
    public:
        _Check_return_ IFACEMETHOD(ShowAt)(_In_ xaml::IFrameworkElement* pPlacementTarget) override;

        DirectUI::InputDeviceType GetInputDeviceTypeUsedToOpen() const
        {
            return m_inputDeviceTypeUsedToOpen;
        }

        _Check_return_ HRESULT ShowAtImpl(
            _In_opt_ xaml::IUIElement* pTargetElement,
            wf::Point point);

        static _Check_return_ HRESULT PreparePopupThemeTransitionsAndShadows(
            _In_ Popup* popup,
            double closedRatioConstant,
            UINT depth,
            _Outptr_ xaml_animation::ITransition** transition);

        // Callback for ShowAt() from core layer
        static _Check_return_ HRESULT ShowAtStatic(
            _In_ CMenuFlyout* pCoreMenuFlyout,
            _In_ CUIElement* pCoreTarget,
            wf::Point point);

        _Check_return_ HRESULT OnProcessKeyboardAcceleratorsImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs) final;

        // IMenu implementation
        _Check_return_ HRESULT get_ParentMenuImpl(_Outptr_result_maybenull_ xaml_controls::IMenu** ppValue);
        _Check_return_ HRESULT put_ParentMenuImpl(_In_opt_ xaml_controls::IMenu* pValue);

        _Check_return_ HRESULT CloseImpl();

        _Check_return_ HRESULT QueueRefreshItemsSource();

    private:
        TrackerPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> m_tpItems;

        // In Threshold, MenuFlyout uses the MenuPopupThemeTransition.
        TrackerPtr<xaml_animation::ITransition> m_tpMenuPopupThemeTransition;

        DirectUI::InputDeviceType m_inputDeviceTypeUsedToOpen;

        ctl::WeakRefPtr m_wrParentMenu;

        bool m_openWindowed{ true };
        bool m_itemsSourceRefreshPending{ false };

    protected:
        MenuFlyout();

        // Prepares object's state
        _Check_return_ HRESULT PrepareState() override;

        // Handle the release of the core object
        _Check_return_ HRESULT DisconnectFrameworkPeerCore() override;

        // Handle the custom property changed event and call the
        // OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(const PropertyChangedParams& args) override;

        IFACEMETHOD(CreatePresenter)(_Outptr_ xaml_controls::IControl** ppReturnValue) override;

        _Check_return_ HRESULT ShowAtCore(
            _In_ xaml::IFrameworkElement* pPlacementTarget,
            _Out_ bool& openDelayed) override;

        // Raise opening event.
        _Check_return_ HRESULT OnOpening() override;

        _Check_return_ HRESULT OnClosing(bool* cancel) override;

        _Check_return_ HRESULT OnClosed() override;

        _Check_return_ HRESULT PreparePopupTheme(
            _In_ Popup* pPopup,
            MajorPlacementMode placementMode,
            _In_ xaml::IFrameworkElement* pPlacementTarget) override;

        _Check_return_ HRESULT UpdatePresenterVisualState(
            FlyoutBase::MajorPlacementMode placement) override;

        _Check_return_ HRESULT AutoAdjustPlacement(
            _Inout_ FlyoutBase::MajorPlacementMode* pPlacement) override;

    private:
        _Check_return_ HRESULT UpdatePresenterVisualState(
            FlyoutBase::MajorPlacementMode placement,
            BOOLEAN doForceTransitions);

        _Check_return_ HRESULT CacheInputDeviceTypeUsedToOpen(
            _In_ CUIElement *pTargetElement);

        _Check_return_ HRESULT CloseSubMenu();

        _Check_return_ HRESULT RefreshItemsSource();
    };
}
