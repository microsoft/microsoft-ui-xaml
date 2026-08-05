// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarFlyoutItem_Partial.cpp.
// Structure/logic preserved 1:1. WRL plumbing translated to C++/WinRT; ButtonBase/Control/UIElement
// members (IsEnabled/IsPressed/IsPointerOver/AddHandler/Click/Content/Focus) are called directly via
// the generated require<> injection. Cross-item radio-group access uses winrt::get_self.

#include "pch.h"
#include "common.h"
#include "InkToolbarFlyoutItem.h"
#include "InkToolbarFlyoutItemAutomationPeer.h"
#include "ButtonManager.h"
#include "InkToolbar.h"
#include "InkToolbarToolButton.h"
#include "InkToolbarMenuButton.h"
#include "InkToolbarEraserButton.h"
#include "InkToolbarStencilButton.h"

InkToolbarFlyoutItem::~InkToolbarFlyoutItem()
{
    try
    {
        auto self = try_as<winrt::UIElement>();
        if (!self)
        {
            return;
        }

        if (m_itemPointerPressedEventHandler) { self.RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_itemPointerPressedEventHandler); }
        if (m_itemPointerEnteredEventHandler) { self.RemoveHandler(winrt::UIElement::PointerEnteredEvent(), m_itemPointerEnteredEventHandler); }
        if (m_itemPointerExitedEventHandler) { self.RemoveHandler(winrt::UIElement::PointerExitedEvent(), m_itemPointerExitedEventHandler); }
        if (m_itemPointerReleasedEventHandler) { self.RemoveHandler(winrt::UIElement::PointerReleasedEvent(), m_itemPointerReleasedEventHandler); }
        if (m_itemKeyDownEventHandler) { self.RemoveHandler(winrt::UIElement::KeyDownEvent(), m_itemKeyDownEventHandler); }
        if (m_itemKeyUpEventHandler) { self.RemoveHandler(winrt::UIElement::KeyUpEvent(), m_itemKeyUpEventHandler); }

        if (auto buttonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>())
        {
            buttonBase.Click(m_clickRegistrationToken);
        }
    }
    catch (...)
    {
    }
}

winrt::AutomationPeer InkToolbarFlyoutItem::OnCreateAutomationPeer()
{
    return winrt::make<InkToolbarFlyoutItemAutomationPeer>(*this);
}

// This method is only supposed to be called once per item so the event handlers aren't hooked more than once.
void InkToolbarFlyoutItem::ConfigureItemEvents(winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase const& parentButton)
{
    m_parentButton = winrt::make_weak(parentButton);

    auto self = try_as<winrt::UIElement>();

    // Click.
    if (auto buttonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>())
    {
        m_clickRegistrationToken = buttonBase.Click(
            winrt::RoutedEventHandler([this](winrt::IInspectable const& s, winrt::RoutedEventArgs const& e) { OnItemClick(s, e); }));
    }

    // Keyboard events (handledEventsToo = true).
    m_itemKeyDownEventHandler = winrt::box_value(winrt::KeyEventHandler([this](winrt::IInspectable const& s, winrt::KeyRoutedEventArgs const& e) { OnItemKeyDown(s, e); }));
    self.AddHandler(winrt::UIElement::KeyDownEvent(), m_itemKeyDownEventHandler, true);
    m_itemKeyUpEventHandler = winrt::box_value(winrt::KeyEventHandler([this](winrt::IInspectable const& s, winrt::KeyRoutedEventArgs const& e) { OnItemKeyUp(s, e); }));
    self.AddHandler(winrt::UIElement::KeyUpEvent(), m_itemKeyUpEventHandler, true);

    // Pointer events (handledEventsToo = true).
    m_itemPointerEnteredEventHandler = winrt::box_value(winrt::PointerEventHandler([this](winrt::IInspectable const& s, winrt::PointerRoutedEventArgs const& e) { OnItemPointerEntered(s, e); }));
    self.AddHandler(winrt::UIElement::PointerEnteredEvent(), m_itemPointerEnteredEventHandler, true);
    m_itemPointerExitedEventHandler = winrt::box_value(winrt::PointerEventHandler([this](winrt::IInspectable const& s, winrt::PointerRoutedEventArgs const& e) { OnItemPointerExited(s, e); }));
    self.AddHandler(winrt::UIElement::PointerExitedEvent(), m_itemPointerExitedEventHandler, true);
    m_itemPointerPressedEventHandler = winrt::box_value(winrt::PointerEventHandler([this](winrt::IInspectable const& s, winrt::PointerRoutedEventArgs const& e) { OnItemPointerPressed(s, e); }));
    self.AddHandler(winrt::UIElement::PointerPressedEvent(), m_itemPointerPressedEventHandler, true);
    m_itemPointerReleasedEventHandler = winrt::box_value(winrt::PointerEventHandler([this](winrt::IInspectable const& s, winrt::PointerRoutedEventArgs const& e) { OnItemPointerReleased(s, e); }));
    self.AddHandler(winrt::UIElement::PointerReleasedEvent(), m_itemPointerReleasedEventHandler, true);
}

void InkToolbarFlyoutItem::SetRadioGroupName(winrt::hstring const& radioGroupName)
{
    m_radioGroupName = radioGroupName;
}

// Returns the content text.
winrt::hstring InkToolbarFlyoutItem::TryGetTextContent()
{
    auto contentControl = try_as<winrt::ContentControl>();
    if (!contentControl)
    {
        return {};
    }

    auto content = contentControl.Content();
    if (!content)
    {
        return {};
    }

    if (auto contentAsPanel = content.try_as<winrt::Panel>())
    {
        // Composite item: find the item name text block (child index 1).
        auto children = contentAsPanel.Children();
        if (children && children.Size() > 1)
        {
            if (auto buttonNameAsTextBlock = children.GetAt(1).try_as<winrt::TextBlock>())
            {
                return buttonNameAsTextBlock.Text();
            }
        }
        return {};
    }

    // Regular item with plain text content.
    return winrt::unbox_value_or<winrt::hstring>(content, L"");
}

void InkToolbarFlyoutItem::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    if (args.Property() == winrt::InkToolbarFlyoutItem::IsCheckedProperty())
    {
        OnIsCheckedChanged(args);
    }
    // KindProperty: no-op.
}

void InkToolbarFlyoutItem::OnApplyTemplate()
{
    UpdateStates(false);
}

// Called by InkToolbarFlyoutItemAutomationPeer; performs the default operation for touch/narrator.
void InkToolbarFlyoutItem::OnInvoked()
{
    ItemAction();
    UpdateStates(false);

    auto parent = m_parentButton.get();
    if (!parent)
    {
        return;
    }

    if (auto toolButton = parent.try_as<winrt::InkToolbarToolButton>())
    {
        if (parent.try_as<winrt::InkToolbarEraserButton>())
        {
            if (auto inkToolbar = winrt::get_self<InkToolbarToolButton>(toolButton)->GetParentInkToolbar())
            {
                winrt::get_self<InkToolbar>(inkToolbar)->OnEraserL3ItemsClicked(*this, nullptr);
            }
        }
    }
    else if (auto menuButton = parent.try_as<winrt::InkToolbarMenuButton>())
    {
        if (parent.try_as<winrt::InkToolbarStencilButton>())
        {
            if (auto inkToolbar = winrt::get_self<InkToolbarMenuButton>(menuButton)->GetParentInkToolbar())
            {
                winrt::get_self<InkToolbar>(inkToolbar)->OnStencilL3ItemsClicked(*this, nullptr);
            }
        }
    }
}

void InkToolbarFlyoutItem::OnItemClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
    ItemAction();
    UpdateStates(false);
}

void InkToolbarFlyoutItem::OnItemPointerEntered(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UpdateStates(false);
    args.Handled(true);
}

void InkToolbarFlyoutItem::OnItemPointerExited(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UpdateStates(false);
    args.Handled(true);
}

void InkToolbarFlyoutItem::OnItemPointerPressed(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UpdateStates(false);
    args.Handled(true);
}

void InkToolbarFlyoutItem::OnItemPointerReleased(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UpdateStates(false);
    args.Handled(true);
}

void InkToolbarFlyoutItem::OnItemKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    auto key = args.Key();

    winrt::InkToolbarFlyoutItem itemToFocus{ nullptr };
    bool isHandled = false;

    switch (key)
    {
    case winrt::Windows::System::VirtualKey::Enter:
    case winrt::Windows::System::VirtualKey::Space:
        // Enter and Space are ignored because a Click event is sent by ButtonBase on KeyUp.
        break;

    case winrt::Windows::System::VirtualKey::Down:
    case winrt::Windows::System::VirtualKey::Right:
        itemToFocus = GetRelativeItem(RelativeItem::Next, *this, true);
        isHandled = true;
        break;

    case winrt::Windows::System::VirtualKey::Up:
    case winrt::Windows::System::VirtualKey::Left:
        itemToFocus = GetRelativeItem(RelativeItem::Previous, *this, true);
        isHandled = true;
        break;

    default:
        break;
    }

    if (itemToFocus)
    {
        if (auto itemAsControl = itemToFocus.try_as<winrt::Control>())
        {
            if (itemAsControl.IsEnabled())
            {
                itemAsControl.Focus(winrt::FocusState::Programmatic);
            }
        }
    }

    UpdateStates(false);
    args.Handled(isHandled);
}

void InkToolbarFlyoutItem::OnItemKeyUp(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
    UpdateStates(false);
}

// React to a 'Click'-like action.
void InkToolbarFlyoutItem::ItemAction()
{
    switch (Kind())
    {
    case winrt::InkToolbarFlyoutItemKind::Simple:
        // Simple item; not setting the checked state.
        break;

    case winrt::InkToolbarFlyoutItemKind::Check:
        ToggleCheckedState();
        break;

    case winrt::InkToolbarFlyoutItemKind::Radio:
        IsChecked(true);
        ForAllOthersInGroup(*this, [](winrt::InkToolbarFlyoutItem const& item)
        {
            item.IsChecked(false);
            return true;
        });
        break;

    case winrt::InkToolbarFlyoutItemKind::RadioCheck:
        if (ToggleCheckedState())
        {
            ForAllOthersInGroup(*this, [](winrt::InkToolbarFlyoutItem const& item)
            {
                item.IsChecked(false);
                return true;
            });
        }
        break;

    default:
        throw winrt::hresult_invalid_argument(L"Unknown InkToolbarFlyoutItemKind");
    }
}

// Find the relative item in the same flyout. Return the anchor itself if none is found.
winrt::InkToolbarFlyoutItem InkToolbarFlyoutItem::GetRelativeItem(
    RelativeItem relative,
    winrt::InkToolbarFlyoutItem const& anchor,
    bool shouldWrapAround)
{
    std::vector<winrt::InkToolbarFlyoutItem> items;
    ForAllInFlyout([&](winrt::InkToolbarFlyoutItem const& item)
    {
        items.push_back(item);
        return true;
    });

    winrt::InkToolbarFlyoutItem relativeItem = anchor;

    if (items.size() > 1)
    {
        auto found = std::find(items.cbegin(), items.cend(), anchor);
        if (items.cend() != found)
        {
            size_t foundIndex = found - items.cbegin();

            switch (relative)
            {
            case RelativeItem::Next:
                if (foundIndex < (items.size() - 1))
                {
                    relativeItem = items[foundIndex + 1];
                }
                else
                {
                    relativeItem = shouldWrapAround ? items.front() : items.back();
                }
                break;

            case RelativeItem::Previous:
                if (foundIndex > 0)
                {
                    relativeItem = items[foundIndex - 1];
                }
                else
                {
                    relativeItem = shouldWrapAround ? items.back() : items.front();
                }
                break;

            case RelativeItem::RadioGroupNext:
            case RelativeItem::RadioGroupPrevious:
            {
                auto anchorRadioGroupName = winrt::get_self<InkToolbarFlyoutItem>(anchor)->RadioGroupName();
                auto anchorKind = anchor.Kind();

                for (unsigned i = 0; i < items.size(); ++i)
                {
                    int searchIndex = 0;
                    winrt::InkToolbarFlyoutItem targetItem{ nullptr };

                    if (RelativeItem::RadioGroupNext == relative)
                    {
                        searchIndex = static_cast<int>(foundIndex) + static_cast<int>(i) + 1;
                        if (searchIndex <= static_cast<int>(items.size() - 1))
                        {
                            targetItem = items[searchIndex];
                        }
                        else if (shouldWrapAround)
                        {
                            targetItem = items[searchIndex % items.size()];
                        }
                        else
                        {
                            targetItem = anchor;
                            break;
                        }
                    }
                    else
                    {
                        searchIndex = static_cast<int>(foundIndex) - static_cast<int>(i) - 1;
                        if (searchIndex >= 0)
                        {
                            targetItem = items[searchIndex];
                        }
                        else if (shouldWrapAround)
                        {
                            targetItem = items[(searchIndex + items.size()) % items.size()];
                        }
                        else
                        {
                            targetItem = anchor;
                            break;
                        }
                    }

                    if (targetItem.Kind() != anchorKind)
                    {
                        // Not the same kind of items, continue searching.
                        continue;
                    }

                    auto targetRadioGroupName = winrt::get_self<InkToolbarFlyoutItem>(targetItem)->RadioGroupName();
                    if (anchorRadioGroupName == targetRadioGroupName)
                    {
                        // Found an item in the same radio group; return it if visible.
                        if (targetItem.as<winrt::UIElement>().Visibility() == winrt::Visibility::Visible)
                        {
                            relativeItem = targetItem;
                            break;
                        }
                    }
                }
                break;
            }

            default:
                break;
            }
        }
    }

    return relativeItem;
}

void InkToolbarFlyoutItem::OnIsCheckedChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    bool oldValue = winrt::unbox_value_or<bool>(args.OldValue(), false);
    bool newValue = winrt::unbox_value_or<bool>(args.NewValue(), false);

    if (newValue == oldValue)
    {
        return;
    }

    UpdateStates(false);

    if (newValue)
    {
        auto kind = Kind();
        // Ensure other items in the radio group are unchecked when checked programmatically.
        if (kind == winrt::InkToolbarFlyoutItemKind::Radio || kind == winrt::InkToolbarFlyoutItemKind::RadioCheck)
        {
            ForAllOthersInGroup(*this, [](winrt::InkToolbarFlyoutItem const& item)
            {
                item.IsChecked(false);
                return true;
            });
        }

        m_checkedEventSource(*this, nullptr);
    }
    else
    {
        m_uncheckedEventSource(*this, nullptr);
    }
}

// Loop through all flyout items in the flyout. The function returns bool to indicate whether to continue.
void InkToolbarFlyoutItem::ForAllInFlyout(std::function<bool(winrt::InkToolbarFlyoutItem const& item)> function)
{
    auto parentButton = m_parentButton.get();
    if (!parentButton)
    {
        return;
    }

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(parentButton, flyoutContent))
    {
        int count = winrt::VisualTreeHelper::GetChildrenCount(flyoutContent);
        for (int i = 0; i < count; ++i)
        {
            if (auto child = winrt::VisualTreeHelper::GetChild(flyoutContent, i).try_as<winrt::InkToolbarFlyoutItem>())
            {
                if (!function(child))
                {
                    break;
                }
            }
        }
    }
}

// Loop through all flyout items from the same radio group in the same flyout.
void InkToolbarFlyoutItem::ForAllInGroup(
    winrt::InkToolbarFlyoutItem const& anchor,
    std::function<bool(winrt::InkToolbarFlyoutItem const& item)> function)
{
    ForAllInFlyout([&](winrt::InkToolbarFlyoutItem const& child)
    {
        bool shouldContinue = true;

        if (anchor == child)
        {
            shouldContinue = function(child);
        }
        else
        {
            // Radio group names are per item kind; different kinds are different groups.
            if (anchor.Kind() != child.Kind())
            {
                return shouldContinue;
            }

            auto anchorRadioGroupName = winrt::get_self<InkToolbarFlyoutItem>(anchor)->RadioGroupName();
            auto childRadioGroupName = winrt::get_self<InkToolbarFlyoutItem>(child)->RadioGroupName();

            if (anchorRadioGroupName == childRadioGroupName)
            {
                shouldContinue = function(child);
            }
        }

        return shouldContinue;
    });
}

// Loop through all items from the same radio group except the anchor itself.
void InkToolbarFlyoutItem::ForAllOthersInGroup(
    winrt::InkToolbarFlyoutItem const& anchor,
    std::function<bool(winrt::InkToolbarFlyoutItem const& item)> function)
{
    ForAllInGroup(*this, [&](winrt::InkToolbarFlyoutItem const& child)
    {
        bool shouldContinue = true;
        if (child != anchor)
        {
            shouldContinue = function(child);
        }
        return shouldContinue;
    });
}

// Toggles the Checked state; returns the new state.
bool InkToolbarFlyoutItem::ToggleCheckedState()
{
    bool newCheckedState = !IsChecked();
    IsChecked(newCheckedState);
    UpdateStates(false);
    return newCheckedState;
}

void InkToolbarFlyoutItem::GoToState(wchar_t const* stateString, bool useTransitions)
{
    UNREFERENCED_PARAMETER(useTransitions);
    // UWP hardcodes useTransitions=false in the underlying GoToState call; mirror that.
    winrt::VisualStateManager::GoToState(*this, stateString, false);
}

// Update the visual state of the item. We listen to all events that trigger a state change and update.
void InkToolbarFlyoutItem::UpdateStates(bool useTransitions)
{
    bool isEnabled = IsEnabled();
    bool isPressed = IsPressed();
    bool isPointerOver = IsPointerOver();
    bool isChecked = IsChecked();

    if (isChecked)
    {
        if (!isEnabled) { GoToState(L"CheckedDisabled", useTransitions); }
        else if (isPressed) { GoToState(L"CheckedPressed", useTransitions); }
        else if (isPointerOver) { GoToState(L"CheckedPointerOver", useTransitions); }
        else { GoToState(L"Checked", useTransitions); }
    }
    else
    {
        if (!isEnabled) { GoToState(L"Disabled", useTransitions); }
        else if (isPressed) { GoToState(L"Pressed", useTransitions); }
        else if (isPointerOver) { GoToState(L"PointerOver", useTransitions); }
        else { GoToState(L"Normal", useTransitions); }
    }
}

void InkToolbarFlyoutItem::UpdateVisualStatesForAllItems()
{
    ForAllInFlyout([&](winrt::InkToolbarFlyoutItem const& child)
    {
        winrt::get_self<InkToolbarFlyoutItem>(child)->UpdateStates(false);
        return true;
    });
}

// Returns the first visible item in the same radio group.
winrt::InkToolbarFlyoutItem InkToolbarFlyoutItem::GetFirstVisibleItemInGroup()
{
    winrt::InkToolbarFlyoutItem item{ nullptr };
    ForAllInGroup(*this, [&](winrt::InkToolbarFlyoutItem const& child)
    {
        if (child.as<winrt::UIElement>().Visibility() == winrt::Visibility::Collapsed)
        {
            return true;   // Continue.
        }
        item = child;
        return false;
    });
    return item;
}

bool InkToolbarFlyoutItem::IsAnySelectedInRadioGroup()
{
    bool isAnySelected = false;
    auto kind = Kind();

    if (kind == winrt::InkToolbarFlyoutItemKind::Radio || kind == winrt::InkToolbarFlyoutItemKind::RadioCheck)
    {
        ForAllInGroup(*this, [&isAnySelected](winrt::InkToolbarFlyoutItem const& child)
        {
            if (child.IsChecked())
            {
                isAnySelected = true;
            }
            // Don't continue once a selected item is found.
            return !isAnySelected;
        });
    }

    return isAnySelected;
}
