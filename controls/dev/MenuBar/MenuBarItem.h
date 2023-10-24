// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuBarItem.g.h"
#include "MenuBarItem.properties.h"

enum class FlyoutLocation
{
    Left,
    Right
};

class MenuBarItem :
    public ReferenceTracker<MenuBarItem, winrt::implementation::MenuBarItemT>,
    public MenuBarItemProperties
{
public:
    MenuBarItem();
    virtual ~MenuBarItem();

    void OnApplyTemplate();


    void AddPassThroughElement(const winrt::DependencyObject& element);
    void ShowMenuFlyout();
    void CloseMenuFlyout();
    bool IsFlyoutOpen();
    void Invoke();

public:
    // IUIElement / IUIElementOverridesHelper
    winrt::AutomationPeer OnCreateAutomationPeer();

private:

    void PopulateContent();
    void AttachEventHandlers();
    void DetachEventHandlers(bool useSafeGet = false);
    void OpenFlyoutFrom(FlyoutLocation location);
    void MoveFocusTo(FlyoutLocation location);

    void OnVisualPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void UpdateVisualStates();

    // Input handlers
    void OnMenuBarItemPointerEntered(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnMenuBarItemPointerPressed(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnMenuBarItemKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args);
    void OnPresenterKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args);
    void OnItemsVectorChanged(winrt::Collections::IObservableVector<winrt::MenuFlyoutItemBase> const& sender, winrt::Collections::IVectorChangedEventArgs const& e);
    void OnMenuBarItemAccessKeyInvoked(winrt::IInspectable const& sender, winrt::AccessKeyInvokedEventArgs const& args);

    // Flyout event handlers
    void OnFlyoutClosed(winrt::IInspectable const& sender, winrt::IInspectable const& args);
    void OnFlyoutOpening(winrt::IInspectable const& sender, winrt::IInspectable const& args);

    // Variables
    tracker_ref<winrt::Button> m_button{ this };
    tracker_ref<winrt::MenuBarItemFlyout> m_flyout{ this };
    weak_ref<winrt::DependencyObject> m_passThroughElement;
    weak_ref<winrt::MenuBar> m_menuBar;
    bool m_isFlyoutOpen{ false };

    // Event tokens and handlers
    winrt::UIElement::KeyDown_revoker m_presenterKeyDownRevoker{};
    winrt::FlyoutBase::Closed_revoker m_flyoutClosedRevoker{};
    winrt::FlyoutBase::Opening_revoker m_flyoutOpeningRevoker{};
    winrt::UIElement::PointerEntered_revoker m_pointerEnteredRevoker{};
    winrt::UIElement::AccessKeyInvoked_revoker m_accessKeyInvokedRevoker{};

    RoutedEventHandler_revoker m_onMenuBarItemPointerPressedRevoker{};
    RoutedEventHandler_revoker m_onMenuBarItemKeyDownRevoker{};

    PropertyChanged_revoker m_pressedRevoker{};
    PropertyChanged_revoker m_pointerOverRevoker{};
};
