// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RadioButtons.h"
#include "Vector.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "RadioButtonsTestHooks.h"

RadioButtons::RadioButtons()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RadioButtons);

    auto const items = winrt::make<Vector<winrt::IInspectable>>();
    SetValue(s_ItemsProperty, items);

    SetDefaultStyleKey(this);

    // Override normal up/down/left/right behavior -- down should always go to the next item and up to the previous.
    // left and right should be spacial but contained to the RadioButtons control. We have to attach to PreviewKeyDown
    // because RadioButton has a key down handler for up and down that gets called before we can intercept. Issue #1634.
    if (auto const thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
    {
        thisAsIUIElement7.PreviewKeyDown({ this, &RadioButtons::OnChildPreviewKeyDown });
    }

    AccessKeyInvoked({ this, &RadioButtons::OnAccessKeyInvoked });
    GettingFocus({ this, &RadioButtons::OnGettingFocus });

    m_radioButtonsElementFactory = winrt::make_self<RadioButtonsElementFactory>();

    // RadioButtons adds handlers to its child radio button elements' checked and unchecked events.
    // To ensure proper lifetime management we create revokers for these elements and attach
    // the revokers to the child radio button via this attached property.  This way, if/when the child
    // is cleaned up we will automatically revoke the handler.
    s_childHandlersProperty =
        InitializeDependencyProperty(
            s_childHandlersPropertyName,
            winrt::name_of<ChildHandlers>(),
            winrt::name_of<winrt::RadioButtons>(),
            true /* isAttached */,
            nullptr,
            nullptr);
}

void RadioButtons::OnApplyTemplate()
{
    const winrt::IControlProtected controlProtected{ *this };

    m_repeater.set([this, controlProtected]() {
        if (auto const repeater = GetTemplateChildT<winrt::ItemsRepeater>(s_repeaterName, controlProtected))
        {
            repeater.ItemTemplate(*m_radioButtonsElementFactory);

            m_repeaterElementPreparedRevoker = repeater.ElementPrepared(winrt::auto_revoke, { this, &RadioButtons::OnRepeaterElementPrepared });
            m_repeaterElementClearingRevoker = repeater.ElementClearing(winrt::auto_revoke, { this, &RadioButtons::OnRepeaterElementClearing });
            m_repeaterElementIndexChangedRevoker = repeater.ElementIndexChanged(winrt::auto_revoke, { this, &RadioButtons::OnRepeaterElementIndexChanged });
            m_repeaterLoadedRevoker = repeater.Loaded(winrt::auto_revoke, { this, &RadioButtons::OnRepeaterLoaded });
            return repeater;
        }
        return static_cast<winrt::ItemsRepeater>(nullptr);
    }());

    UpdateItemsSource();
}

// When focus comes from outside the RadioButtons control we will put focus on the selected radio button.
void RadioButtons::OnGettingFocus(const winrt::IInspectable&, const winrt::GettingFocusEventArgs& args)
{
    if (auto const repeater = m_repeater.get())
    {
        auto const inputDevice = args.InputDevice();
        if (inputDevice == winrt::FocusInputDeviceKind::Keyboard)
        {
            // If focus is coming from outside the repeater, put focus on the selected item.
            auto const oldFocusedElement = args.OldFocusedElement();
            if (!IsRepeaterOwnedElement(oldFocusedElement, repeater))
            {
                if (auto const selectedItem = repeater.TryGetElement(m_selectedIndex))
                {
                    if (auto const argsAsIGettingFocusEventArgs2 = args.try_as<winrt::IGettingFocusEventArgs2>())
                    {
                        if (args.TrySetNewFocusedElement(selectedItem))
                        {
                            args.Handled(true);
                        }
                    }
                }
            }

            // Focus was already in the repeater: On RS3+ Selection follows focus unless control is held down.
            else if (SharedHelpers::IsRS3OrHigher() &&
                (winrt::Window::Current().CoreWindow().GetKeyState(winrt::VirtualKey::Control) &
                    winrt::CoreVirtualKeyStates::Down) != winrt::CoreVirtualKeyStates::Down)
            {
                if (auto const newFocusedElementAsUIE = args.NewFocusedElement().as<winrt::UIElement>())
                {
                    int index;
                    if (TryGetRepeaterElementIndex(newFocusedElementAsUIE, repeater, index))
                    {
                        Select(index);
                        args.Handled(true);
                    }
                }
            }
        }
    }
}

void RadioButtons::OnRepeaterLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto const repeater = m_repeater.get())
    {
        if (m_testHooksEnabled)
        {
            AttachToLayoutChanged();
        }

        m_blockSelecting = false;
        if (SelectedIndex() == -1 && SelectedItem())
        {
            UpdateSelectedItem();
        }
        else
        {
            UpdateSelectedIndex();
        }

        OnRepeaterCollectionChanged(nullptr, nullptr);
    }
}

void RadioButtons::OnChildPreviewKeyDown(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args)
{
    switch (args.Key())
    {
    case winrt::VirtualKey::Down:
        if (MoveFocusNext())
        {
            return args.Handled(true);
        }
        else if (args.OriginalKey() == winrt::VirtualKey::GamepadDPadDown)
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Next))
            {
                return args.Handled(true);
            }
        }
        args.Handled(HandleEdgeCaseFocus(false, args.OriginalSource()));
        break;
    case winrt::VirtualKey::Up:
        if (MoveFocusPrevious())
        {
            return args.Handled(true);
        }
        else if (args.OriginalKey() == winrt::VirtualKey::GamepadDPadUp)
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Previous))
            {
                return args.Handled(true);
            }
        }
        args.Handled(HandleEdgeCaseFocus(true, args.OriginalSource()));
        break;
    case winrt::VirtualKey::Right:
        if (args.OriginalKey() != winrt::VirtualKey::GamepadDPadRight)
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Right, GetFindNextElementOptions()))
            {
                return args.Handled(true);
            }
        }
        else
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Right))
            {
                return args.Handled(true);
            }
        }
        args.Handled(HandleEdgeCaseFocus(false, args.OriginalSource()));
        break;

    case winrt::VirtualKey::Left:
        if (args.OriginalKey() != winrt::VirtualKey::GamepadDPadLeft)
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Left, GetFindNextElementOptions()))
            {
                return args.Handled(true);
            }
        }
        else
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Left))
            {
                return args.Handled(true);
            }
        }
        args.Handled(HandleEdgeCaseFocus(true, args.OriginalSource()));
        break;
    }
}

void RadioButtons::OnAccessKeyInvoked(const winrt::UIElement&, const winrt::AccessKeyInvokedEventArgs& args)
{
    // If RadioButtons is an AccessKeyScope then we do not want to handle the access
    // key invoked event because the user has (probably) set up access keys for the
    // RadioButton elements.
    if (!IsAccessKeyScope())
    {
        if (m_selectedIndex)
        {
            if (auto const repeater = m_repeater.get())
            {
                if (auto const selectedItem = TryGetRadioButtonAsUIElement(m_selectedIndex, repeater))
                {
                    if (auto const selectedItemAsControl = selectedItem.try_as<winrt::Control>())
                    {
                        return args.Handled(selectedItemAsControl.Focus(winrt::FocusState::Programmatic));
                    }
                    
                }
            }
        }

        // If we don't have a selected index, focus the RadioButton's which under normal
        // circumstances will put focus on the first radio button.
        args.Handled(this->Focus(winrt::FocusState::Programmatic));
    }
}

// If we haven't handled the key yet and the original source was the first(for up and left)
// or last(for down and right) element in the repeater we need to handle the key so
// RadioButton doesn't, which would result in the behavior.
bool RadioButtons::HandleEdgeCaseFocus(bool first, const winrt::IInspectable& source)
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const sourceAsUIElement = source.try_as<winrt::UIElement>())
        {
            auto const index = [first, repeater]()
            {
                if (first)
                {
                    return 0;
                }
                if (auto const itemsSourceView = repeater.ItemsSourceView())
                {
                    return itemsSourceView.Count() - 1;
                }
                return -1;
            }();

            int sourceRepeaterElementIndex;
            if (TryGetRepeaterElementIndex(sourceAsUIElement, repeater, sourceRepeaterElementIndex)
                && sourceRepeaterElementIndex == index)
            {
                return true;
            }
        }
    }
    return false;
}

winrt::FindNextElementOptions RadioButtons::GetFindNextElementOptions()
{
    auto const findNextElementOptions = winrt::FindNextElementOptions{};
    findNextElementOptions.SearchRoot(*this);
    return findNextElementOptions;
}

void RadioButtons::OnRepeaterElementPrepared(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementPreparedEventArgs& args)
{
    if (auto const element = TryGetRadioButtonAsUIElement(args.Element()))
    {
        if (auto const elementAsToggle = element.try_as<winrt::ToggleButton>())
        {
            auto childHandlers = winrt::make_self<ChildHandlers>();
            childHandlers->checkedRevoker = elementAsToggle.Checked(winrt::auto_revoke, { this, &RadioButtons::OnChildChecked });
            childHandlers->uncheckedRevoker = elementAsToggle.Unchecked(winrt::auto_revoke, { this, &RadioButtons::OnChildUnchecked });

            elementAsToggle.SetValue(s_childHandlersProperty, childHandlers.as<winrt::IInspectable>());

            // If the developer adds a checked radio button to the collection, update selection to this item.
            if (SharedHelpers::IsTrue(elementAsToggle.IsChecked()))
            {
                Select(args.Index());
            }
        }
        
        if (auto const repeater = m_repeater.get())
        {
            if (auto const itemSourceView = repeater.ItemsSourceView())
            {
                element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(args.Index() + 1));
                element.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(itemSourceView.Count()));
            }
        }
    }
}

void RadioButtons::OnRepeaterElementClearing(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementClearingEventArgs& args)
{
    if (auto const element = TryGetRadioButtonAsUIElement(args.Element()))
    {
        if (auto const elementAsToggle = element.try_as<winrt::ToggleButton>())
        {
            elementAsToggle.SetValue(s_childHandlersProperty, nullptr);

            // If the removed element was the selected one, update selection to -1
            if (SharedHelpers::IsTrue(elementAsToggle.IsChecked()))
            {
                Select(-1);
            }
        }
    }
}

void RadioButtons::OnRepeaterElementIndexChanged(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    if (auto const element = TryGetRadioButtonAsUIElement(args.Element()))
    {
        element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(args.NewIndex() + 1));

        if (auto const elementAsToggle = element.try_as<winrt::ToggleButton>())
        {
            // When the selected item's index changes, update selection to match
            if (SharedHelpers::IsTrue(elementAsToggle.IsChecked()))
            {
                Select(args.NewIndex());
            }
        }
    }
}

void RadioButtons::OnRepeaterCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const itemSourceView = repeater.ItemsSourceView())
        {
            auto const count = itemSourceView.Count();
            for (auto index = 0; index < count; index++)
            {
                if (auto const element = TryGetRadioButtonAsUIElement(index, repeater))
                {
                    element.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(count));
                }
            }
        }
    }
}

void RadioButtons::Select(int index)
{
    if(!m_blockSelecting && !m_currentlySelecting && m_selectedIndex != index)
    {
        // Calling Select updates the checked state on the radio button being selected
        // and the radio button being unselected, as well as updates the SelectedIndex
        // and SelectedItem DP. All of these things would cause Select to be called so
        // we'll prevent reentrency with this m_currentlySelecting boolean.
        auto clearSelecting = gsl::finally([this]()
            {
                m_currentlySelecting = false;
            });
        m_currentlySelecting = true;

        auto const previousSelectedIndex = m_selectedIndex;
        m_selectedIndex = index;

        auto const newSelectedItem = GetDataAtIndex(m_selectedIndex, true);
        auto const previousSelectedItem = GetDataAtIndex(previousSelectedIndex, false);

        SelectedIndex(m_selectedIndex);
        SelectedItem(newSelectedItem);

        auto const previousSelectedItems = winrt::make<Vector<winrt::IInspectable>>();
        if (previousSelectedItem)
        {
            previousSelectedItems.Append(previousSelectedItem);
        }

        auto const newSelectedItems = winrt::make<Vector<winrt::IInspectable>>();
        if (newSelectedItem)
        {
            newSelectedItems.Append(newSelectedItem);
        }

        m_selectionChangedEventSource(*this, winrt::SelectionChangedEventArgs(previousSelectedItems, newSelectedItems));
    }
}

winrt::IInspectable RadioButtons::GetDataAtIndex(int index, bool containerIsChecked)
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const element = TryGetRadioButtonAsUIElement(index, repeater))
        {
            if (auto const elementAsToggle = element.try_as<winrt::ToggleButton>())
            {
                elementAsToggle.IsChecked(containerIsChecked);
            }
        }

        if (index >= 0)
        {
            if (auto const itemsSourceView = repeater.ItemsSourceView())
            {
                if (index < itemsSourceView.Count())
                {
                    return itemsSourceView.GetAt(index);
                }
            }
        }
    }
    return static_cast<winrt::IInspectable>(nullptr);
}

void RadioButtons::OnChildUnchecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&)
{
    if (!m_currentlySelecting)
    {
        if (auto const repeater = m_repeater.get())
        {
            if (auto const senderAsUIE = sender.as<winrt::UIElement>())
            {
                int index;
                if (TryGetRepeaterElementIndex(senderAsUIE, repeater, index))
                {
                    if (m_selectedIndex == index)
                    {
                        Select(-1);
                    }
                }
            }
        }
    }
}

void RadioButtons::OnChildChecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&)
{
    if (!m_currentlySelecting)
    {
        if (auto const repeater = m_repeater.get())
        {
            if (auto const senderAsUIE = sender.as<winrt::UIElement>())
            {
                int index;
                if (TryGetRepeaterElementIndex(senderAsUIE, repeater, index))
                {
                    Select(index);
                }
            }
        }
    }
}

bool RadioButtons::MoveFocusNext()
{
    return MoveFocus(1);
}

bool RadioButtons::MoveFocusPrevious()
{
    return MoveFocus(-1);
}

bool RadioButtons::MoveFocus(int indexIncrement)
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const focusedElement = winrt::FocusManager::GetFocusedElement().try_as<winrt::UIElement>())
        {
            int focusedIndex;
            if (TryGetRepeaterElementIndex(focusedElement, repeater, focusedIndex)
                && focusedIndex >= 0)
            {
                focusedIndex += indexIncrement;
                auto const itemCount = repeater.ItemsSourceView().Count();
                while (focusedIndex >= 0 && focusedIndex < itemCount)
                {
                    if (auto const element = TryGetRadioButtonAsUIElement(focusedIndex, repeater))
                    {
                        if (auto const elementAsControl = element.try_as<winrt::Control>())
                        {
                            if (elementAsControl.Focus(winrt::FocusState::Programmatic))
                            {
                                return true;
                            }
                        }
                        
                    }
                    focusedIndex += indexIncrement;
                }
            }
        }
    }
    return false;
}

void RadioButtons::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_ItemsProperty || property == s_ItemsSourceProperty)
    {
        UpdateItemsSource();
    }
    else if (property == s_SelectedIndexProperty)
    {
        UpdateSelectedIndex();
    }
    else if (property == s_SelectedItemProperty)
    {
        UpdateSelectedItem();
    }
    else if (property == s_ItemTemplateProperty)
    {
        UpdateItemTemplate();
    }
}

winrt::UIElement RadioButtons::ContainerFromIndex(int index)
{
    if (auto const repeater = m_repeater.get())
    {
        // We return the actual RadioButton here as the parent grid wrapper is just an implementation detail
        // and conceptually, the RadioButton control is the container.
        return TryGetRadioButtonAsUIElement(index, repeater);
    }
    return nullptr;
}

void RadioButtons::UpdateItemsSource()
{
    Select(-1);
    m_itemsSourceChanged.revoke();
    if (auto const repeater = m_repeater.get())
    {
        repeater.ItemsSource(GetItemsSource());

        if (auto const itemsSourceView = repeater.ItemsSourceView())
        {
            m_itemsSourceChanged = itemsSourceView.CollectionChanged(winrt::auto_revoke, { this, &RadioButtons::OnRepeaterCollectionChanged });
        }
    }
}

winrt::IInspectable RadioButtons::GetItemsSource()
{
    if (auto const itemsSource = ItemsSource())
    {
        return itemsSource;
    }
    else
    {
        return Items();
    }
}

void RadioButtons::UpdateSelectedIndex()
{
    if (!m_currentlySelecting)
    {
        Select(SelectedIndex());
    }
}

void RadioButtons::UpdateSelectedItem()
{
    if (!m_currentlySelecting)
    {
        if (auto const repeater = m_repeater.get())
        {
            if (auto const itemsSourceView = repeater.ItemsSourceView())
            {
                Select(itemsSourceView.IndexOf(SelectedItem()));
            }
        }
    }
}

winrt::UIElement RadioButtons::TryGetRadioButtonAsUIElement(const winrt::UIElement& repaterElement)
{
    if (auto const elementAsPanel = repaterElement.try_as<winrt::Panel>())
    {
        if (elementAsPanel.Children().Size() >= 1)
        {
            return elementAsPanel.Children().GetAt(0).try_as<winrt::UIElement>();
        }
    }

    return nullptr;
}

winrt::UIElement RadioButtons::TryGetRadioButtonAsUIElement(int repeaterElementIndex, const winrt::ItemsRepeater& repeater)
{
    if (auto const repeaterElement = repeater.TryGetElement(repeaterElementIndex))
    {
        return TryGetRadioButtonAsUIElement(repeaterElement);
    }

    return nullptr;
}

winrt::UIElement RadioButtons::TryGetRepeaterElement(const winrt::UIElement& radioButton)
{
    return winrt::VisualTreeHelper::GetParent(radioButton).try_as<winrt::UIElement>();
}

bool RadioButtons::TryGetRepeaterElementIndex(const winrt::UIElement& radioButton, const winrt::ItemsRepeater& repeater, int& index)
{
    if (auto const repeaterElement = TryGetRepeaterElement(radioButton))
    {
        index = repeater.GetElementIndex(repeaterElement);
        return true;
    }

    return false;
}

bool RadioButtons::IsRepeaterOwnedElement(const winrt::DependencyObject& element, const winrt::ItemsRepeater& repeater)
{
    if (auto const elementAsUIE = element.try_as<winrt::UIElement>())
    {
        if (auto const repeaterElement = TryGetRepeaterElement(elementAsUIE))
        {
            return repeater == winrt::VisualTreeHelper::GetParent(repeaterElement);
        }
    }

    return false;
}

void RadioButtons::UpdateItemTemplate()
{
    m_radioButtonsElementFactory->UserElementFactory(ItemTemplate());
}

// Test Hooks helpers, only function when m_testHooksEnabled == true
void RadioButtons::SetTestHooksEnabled(bool enabled)
{
    if (m_testHooksEnabled != enabled)
    {
        m_testHooksEnabled = enabled;
        if (enabled)
        {
            AttachToLayoutChanged();
        }
        else
        {
            DetatchFromLayoutChanged();
        }
    }
}

RadioButtons::~RadioButtons()
{
    if (m_layoutChangedToken)
    {
        if (auto const layout = GetLayout())
        {
            layout->LayoutChanged(m_layoutChangedToken);
            m_layoutChangedToken = { 0 };
        }
    }
}

void RadioButtons::OnLayoutChanged(const winrt::ColumnMajorUniformToLargestGridLayout&, const winrt::IInspectable&)
{
    RadioButtonsTestHooks::NotifyLayoutChanged(*this);
}

int RadioButtons::GetRows()
{
    if (auto const layout = GetLayout())
    {
        return layout->GetRows();
    }
    return -1;
}

int RadioButtons::GetColumns()
{
    if (auto const layout = GetLayout())
    {
        return layout->GetColumns();
    }
    return -1;
}

int RadioButtons::GetLargerColumns()
{
    if (auto const layout = GetLayout())
    {
        return layout->GetLargerColumns();
    }
    return -1;
}

void RadioButtons::AttachToLayoutChanged()
{
    if (auto const layout = GetLayout())
    {
        layout->SetTestHooksEnabled(true);
        m_layoutChangedToken = layout->LayoutChanged({ this, &RadioButtons::OnLayoutChanged });
    }
}

void RadioButtons::DetatchFromLayoutChanged()
{
    if (auto const layout = GetLayout())
    {
        layout->SetTestHooksEnabled(false);
        layout->LayoutChanged(m_layoutChangedToken);
        m_layoutChangedToken = { 0 };
    }
}

com_ptr<ColumnMajorUniformToLargestGridLayout> RadioButtons::GetLayout()
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const layout = repeater.Layout())
        {
            if (auto const customLayout = layout.try_as<winrt::ColumnMajorUniformToLargestGridLayout>())
            {
                return winrt::get_self<ColumnMajorUniformToLargestGridLayout>(customLayout)->get_strong();
            }
        }
    }
    return nullptr;
}
