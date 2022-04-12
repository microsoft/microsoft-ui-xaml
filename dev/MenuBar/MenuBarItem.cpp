// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MenuBar.h"
#include "MenuBarItem.h"
#include "MenuBarItemFlyout.h"
#include "Vector.h"
#include "VectorIterator.h"
#include "MenuBarItemAutomationPeer.h"

MenuBarItem::MenuBarItem()
{   
    SetDefaultStyleKey(this);

    auto items = winrt::make<Vector<winrt::MenuFlyoutItemBase>>();
    auto observableVector = items.try_as<winrt::IObservableVector<winrt::MenuFlyoutItemBase>>();
    observableVector.VectorChanged({ this, &MenuBarItem::OnItemsVectorChanged });
    SetValue(s_ItemsProperty, items);
}

MenuBarItem::~MenuBarItem()
{
    DetachEventHandlers(true);
}

// IUIElement / IUIElementOverridesHelper
winrt::AutomationPeer MenuBarItem::OnCreateAutomationPeer()
{
    return winrt::make<MenuBarItemAutomationPeer>(*this);
}

// IFramework Override
void MenuBarItem::OnApplyTemplate()
{
    m_button.set(GetTemplateChildT<winrt::Button>(L"ContentButton", *this));

    auto menuBar = SharedHelpers::GetAncestorOfType<winrt::MenuBar>(winrt::VisualTreeHelper::GetParent(*this));
    if (menuBar)
    {
        m_menuBar = winrt::make_weak(menuBar);
        // Ask parent MenuBar for its root to enable pass through
        winrt::get_self<MenuBar>(menuBar)->RequestPassThroughElement(*this);
    }

    PopulateContent();
    DetachEventHandlers();
    AttachEventHandlers();
}

void MenuBarItem::PopulateContent()
{
    // Create flyout
    winrt::MenuBarItemFlyout flyout;

    for (winrt::MenuFlyoutItemBase const& flyoutItem : Items())
    {
        flyout.Items().Append(flyoutItem);
    }

    flyout.Placement(winrt::FlyoutPlacementMode::Bottom);

    if (m_passThroughElement)
    {
        if (winrt::IFlyoutBase3 flyoutBase3 = flyout)
        {
            flyout.OverlayInputPassThroughElement(m_passThroughElement.get());
        }
    }
    m_flyout.set(flyout);

    if (auto button = m_button.get())
    {
        button.IsAccessKeyScope(true);
        button.ContextFlyout(flyout);
    }
}

void MenuBarItem::AttachEventHandlers()
{
    if (auto button = m_button.get())
    {
        m_pressedRevoker = RegisterPropertyChanged(button, winrt::ButtonBase::IsPressedProperty(), { this, &MenuBarItem::OnVisualPropertyChanged });
        m_pointerOverRevoker = RegisterPropertyChanged(button, winrt::ButtonBase::IsPointerOverProperty(), { this, &MenuBarItem::OnVisualPropertyChanged });
    }

    m_onMenuBarItemPointerPressedRevoker = AddRoutedEventHandler<RoutedEventType::PointerPressed>(
        *this,
        [this](auto const& sender, auto const& args)
        {
            OnMenuBarItemPointerPressed(sender, args);
        },
        true /*handledEventsToo*/
    );

    m_onMenuBarItemKeyDownRevoker = AddRoutedEventHandler<RoutedEventType::KeyDown>(
        *this,
        [this](auto const& sender, auto const& args)
        {
            OnMenuBarItemKeyDown(sender, args);
        },
        true /*handledEventsToo*/
     );

    if (auto flyout = m_flyout.get())
    {
        m_flyoutClosedRevoker = flyout.Closed(winrt::auto_revoke, { this, &MenuBarItem::OnFlyoutClosed });
        m_flyoutOpeningRevoker = flyout.Opening(winrt::auto_revoke, { this, &MenuBarItem::OnFlyoutOpening });
    }

    m_pointerEnteredRevoker = PointerEntered(winrt::auto_revoke, { this, &MenuBarItem::OnMenuBarItemPointerEntered });

    m_accessKeyInvokedRevoker = AccessKeyInvoked(winrt::auto_revoke, { this, &MenuBarItem::OnMenuBarItemAccessKeyInvoked });
}

void MenuBarItem::DetachEventHandlers(bool useSafeGet)
{
    m_pressedRevoker.revoke();
    m_pointerOverRevoker.revoke();

    m_flyoutClosedRevoker.revoke();
    m_flyoutOpeningRevoker.revoke();

    m_onMenuBarItemPointerPressedRevoker.revoke();
    m_onMenuBarItemKeyDownRevoker.revoke();
}

// Event Handlers
void MenuBarItem::OnMenuBarItemPointerEntered(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    if (auto menuBar = m_menuBar.get())
    {
        const auto flyoutOpen = (winrt::get_self<MenuBar>(menuBar)->IsFlyoutOpen());
        if (flyoutOpen)
        {
            ShowMenuFlyout();
        }
    }
}

void MenuBarItem::OnMenuBarItemPointerPressed(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    if (auto menuBar = m_menuBar.get())
    {
        const auto flyoutOpen = (winrt::get_self<MenuBar>(menuBar)->IsFlyoutOpen());
        if (!flyoutOpen)
        {
            ShowMenuFlyout();
        }
    }
}

void MenuBarItem::OnMenuBarItemKeyDown( winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    const auto key = args.Key();
    if (key == winrt::VirtualKey::Down
        || key == winrt::VirtualKey::Enter
        || key == winrt::VirtualKey::Space)
    {
        ShowMenuFlyout();
    }
    else if (key == winrt::VirtualKey::Right)
    {
        if (FlowDirection() == winrt::FlowDirection::RightToLeft)
        {
            MoveFocusTo(FlyoutLocation::Left);
        }
        else
        {
            MoveFocusTo(FlyoutLocation::Right);
        }
        args.Handled(TRUE);
    }
    else if (key == winrt::VirtualKey::Left)
    {
        if (FlowDirection() == winrt::FlowDirection::RightToLeft)
        {
            MoveFocusTo(FlyoutLocation::Right);
        }
        else
        {
            MoveFocusTo(FlyoutLocation::Left);
        }
        args.Handled(TRUE);
    }
}

void MenuBarItem::OnPresenterKeyDown( winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    // If the event came from a MenuFlyoutSubItem it means right/left arrow will open it, so we should not handle them to not override default behaviour
    if (auto const& subitem = args.OriginalSource().try_as<winrt::MenuFlyoutSubItem>())
    {
        if (subitem.Items().GetAt(0))
        {
            return;
        }
    }

    const auto key = args.Key();
    if (key == winrt::VirtualKey::Right)
    {
        if (FlowDirection() == winrt::FlowDirection::RightToLeft)
        {
            OpenFlyoutFrom(FlyoutLocation::Left);
        }
        else
        {
            OpenFlyoutFrom(FlyoutLocation::Right);
        }
    }
    else if (key == winrt::VirtualKey::Left)
    {
        if (FlowDirection() == winrt::FlowDirection::RightToLeft)
        {
            OpenFlyoutFrom(FlyoutLocation::Right);
        }
        else
        {
            OpenFlyoutFrom(FlyoutLocation::Left);
        }
    }
}

void MenuBarItem::OnItemsVectorChanged(winrt::Collections::IObservableVector<winrt::MenuFlyoutItemBase> const& sender, winrt::Collections::IVectorChangedEventArgs const& e)
{
    if (auto flyout = m_flyout.safe_get())
    {
        const auto index = e.Index();
        switch (e.CollectionChange())
        {
        case winrt::Collections::CollectionChange::ItemInserted:
            flyout.Items().InsertAt(index, Items().GetAt(index));
            break;
        case winrt::Collections::CollectionChange::ItemRemoved:
            flyout.Items().RemoveAt(index);
            break;
        default:
            break;
        }
    }
}

void MenuBarItem::OnMenuBarItemAccessKeyInvoked(winrt::IInspectable const& sender, winrt::AccessKeyInvokedEventArgs const& args)
{
    ShowMenuFlyout();
    args.Handled(true);
}

// Menu Flyout actions
void MenuBarItem::ShowMenuFlyout()
{
    if (Items().Size() != 0)
    {
        if (auto button = m_button.get())
        {
            const auto width = static_cast<float>(button.ActualWidth());
            const auto height = static_cast<float>(button.ActualHeight());

            if (SharedHelpers::IsFlyoutShowOptionsAvailable())
            {
                // Sets an exclusion rect over the button that generates the flyout so that even if the menu opens upwards
                // (which is the default in touch mode) it doesn't cover the menu bar button.
                winrt::FlyoutShowOptions options{};
                options.Position(winrt::Point(0, height));
                options.Placement(winrt::FlyoutPlacementMode::Bottom);
                options.ExclusionRect(winrt::Rect(0, 0, width, height));
                m_flyout.get().ShowAt(button, options);
            }
            else
            {
                m_flyout.get().ShowAt(button, winrt::Point(0, height));
            }

            // Attach keyboard event handler
            auto presenter = winrt::get_self<MenuBarItemFlyout>(m_flyout.get())->m_presenter.get();
            m_presenterKeyDownRevoker = presenter.KeyDown(winrt::auto_revoke, { this,  &MenuBarItem::OnPresenterKeyDown });
        }
    }
}

void MenuBarItem::CloseMenuFlyout()
{
    m_flyout.get().Hide();
}

void MenuBarItem::OpenFlyoutFrom(FlyoutLocation location)
{
    if (auto menuBar = m_menuBar.get())
    {
        uint32_t index = 0;
        menuBar.Items().IndexOf(*this, index);
        CloseMenuFlyout();
        if (location == FlyoutLocation::Left)
        {
            winrt::get_self<MenuBarItem>(menuBar.Items().GetAt(((index - 1) + menuBar.Items().Size()) % menuBar.Items().Size()))->ShowMenuFlyout();
        }
        else
        {
            winrt::get_self<MenuBarItem>(menuBar.Items().GetAt((index + 1) % menuBar.Items().Size()))->ShowMenuFlyout();
        }
    }
}

void MenuBarItem::MoveFocusTo(FlyoutLocation location)
{
    if (auto menuBar = m_menuBar.get())
    {
        uint32_t index = 0;
        menuBar.Items().IndexOf(*this, index);
        if (location == FlyoutLocation::Left)
        {
            winrt::get_self<MenuBarItem>(menuBar.Items().GetAt(((index - 1) + menuBar.Items().Size()) % menuBar.Items().Size()))->Focus(winrt::FocusState::Programmatic);
        }
        else
        {
            winrt::get_self<MenuBarItem>(menuBar.Items().GetAt((index + 1) % menuBar.Items().Size()))->Focus(winrt::FocusState::Programmatic);
        }
    }
}

void MenuBarItem::AddPassThroughElement(const winrt::DependencyObject& element)
{
    m_passThroughElement = winrt::make_weak(element);
}

bool MenuBarItem::IsFlyoutOpen()
{
    return m_isFlyoutOpen;
}

void MenuBarItem::Invoke()
{
    if (IsFlyoutOpen())
    {
        CloseMenuFlyout();
    }
    else
    {
        ShowMenuFlyout();
    }
}

// Menu Flyout Events
void MenuBarItem::OnFlyoutClosed( winrt::IInspectable const& sender, winrt::IInspectable const& args)
{
    m_isFlyoutOpen = false;

    if (auto menuBar = m_menuBar.get())
    {
        winrt::get_self<MenuBar>(menuBar)->IsFlyoutOpen(false);
    }

    UpdateVisualStates();
}

void MenuBarItem::OnFlyoutOpening( winrt::IInspectable const& sender, winrt::IInspectable const& args)
{
    Focus(winrt::FocusState::Programmatic);

    m_isFlyoutOpen = true;

    if (auto menuBar = m_menuBar.get())
    {
        winrt::get_self<MenuBar>(menuBar)->IsFlyoutOpen(true);
    }

    UpdateVisualStates();
}

void MenuBarItem::OnVisualPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    UpdateVisualStates();
}

void MenuBarItem::UpdateVisualStates()
{
    if (auto button = m_button.get())
    {
        if (button.IsPressed())
        {
            winrt::VisualStateManager::GoToState(*this, L"Pressed", false);
        }
        else if (button.IsPointerOver())
        {
            winrt::VisualStateManager::GoToState(*this, L"PointerOver", false);
        }
        else
        {
            if (m_isFlyoutOpen)
            {
                winrt::VisualStateManager::GoToState(*this, L"Selected", false);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, L"Normal", false);
            }
        }
    }
}
