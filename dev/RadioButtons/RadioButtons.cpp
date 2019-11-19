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

    m_selectionModel = winrt::SelectionModel{};
    m_selectionModel.SingleSelect(true);
    m_selectionModel.Source(GetItemsSource());
    m_selectionChangedRevoker = m_selectionModel.SelectionChanged(winrt::auto_revoke, { this, &RadioButtons::OnSelectionChanged });

    auto const items = winrt::make<Vector<winrt::IInspectable>>();
    SetValue(s_ItemsProperty, items);

    SetDefaultStyleKey(this);

    // Override normal up/down/left/right behavior -- down should always go to the next item and up to the previous.
    // left and right should be spacial but contained to the RadioButtons control. We have to attach to PreviewKeyDown
    // because RadioButton has a key down handler for up and down that gets called before we can intercept. Issue #1634.
    PreviewKeyDown({ this, &RadioButtons::OnChildPreviewKeyDown });
    GettingFocus({ this, &RadioButtons::OnGettingFocus });
    GotFocus({ this, &RadioButtons::OnChildGotFocus });
    KeyDown({ this, &RadioButtons::KeyDownHandler });
    KeyUp({ this, &RadioButtons::KeyUpHandler });

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
    auto const inputDevice = args.InputDevice();
    if (inputDevice == winrt::FocusInputDeviceKind::Keyboard || inputDevice == winrt::FocusInputDeviceKind::GameController)
    {
        if (auto const selectedIndex = m_selectionModel.SelectedIndex())
        {
            if (selectedIndex.GetSize() == 1)
            {
                if (auto const repeater = m_repeater.get())
                {
                    if (auto const oldFocusedElement = args.OldFocusedElement())
                    {
                        if (auto const oldFocusedElementAsUIElement = oldFocusedElement.try_as<winrt::UIElement>())
                        {
                            auto const oldFocusedRepeaterIndex = repeater.GetElementIndex(oldFocusedElementAsUIElement);
                            auto const repeaterItemCount = [repeater]()
                            {
                                if (auto const itemsSourceView = repeater.ItemsSourceView())
                                {
                                    return itemsSourceView.Count();
                                }
                                return 0;
                            }();
                            // If focus is coming from outside the repeater, put focus on the selected item.
                            if (oldFocusedRepeaterIndex < 0)
                            {
                                if (auto const selectedItem = repeater.TryGetElement(selectedIndex.GetAt(0)))
                                {
                                    args.TrySetNewFocusedElement(selectedItem);
                                }
                            }
                            // If focus is coming from the first element to the last element (or last to first), via keyboard we know (suspect)
                            // that RadioButton has interfered and we want to correct. 
                            else if (oldFocusedRepeaterIndex == 0 ||
                                     oldFocusedRepeaterIndex == repeaterItemCount - 1)
                            {
                                if (auto const newFocusedElement = args.NewFocusedElement())
                                {
                                    if (auto const newFocusedElementAsUIElement = newFocusedElement.try_as<winrt::UIElement>())
                                    {
                                        auto const newFocusedRepeaterIndex = repeater.GetElementIndex(newFocusedElementAsUIElement);
                                        if ((oldFocusedRepeaterIndex == 0 && newFocusedRepeaterIndex == repeaterItemCount - 1) ||
                                            (oldFocusedRepeaterIndex == repeaterItemCount - 1 && newFocusedRepeaterIndex == 0))
                                        {
                                            args.TrySetNewFocusedElement(oldFocusedElement);
                                        }
                                    }
                                }
                            }
                        }
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

        UpdateSelectedIndex();
        UpdateSelectedItem();
        UpdateMaximumColumns();
        OnRepeaterCollectionChanged(nullptr, nullptr);
    }
}

void RadioButtons::KeyDownHandler(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Control)
    {
        m_isControlDown = true;
    }
}

void RadioButtons::KeyUpHandler(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Control)
    {
        m_isControlDown = false;
    }
}

void RadioButtons::OnChildPreviewKeyDown(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args)
{
    switch (args.Key())
    {
    case winrt::VirtualKey::Down:
        if (MoveFocusNext())
        {
            args.Handled(true);
        }
        else if (args.OriginalKey() == winrt::VirtualKey::GamepadDPadDown)
        {
            args.Handled(winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Next));
        }
        break;
    case winrt::VirtualKey::Up:
        if (MoveFocusPrevious())
        {
            args.Handled(true);
        }
        else if (args.OriginalKey() == winrt::VirtualKey::GamepadDPadUp)
        {
            args.Handled(winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Previous));
        }
        break;
    case winrt::VirtualKey::Right:
        if (args.OriginalKey() != winrt::VirtualKey::GamepadDPadRight)
        {
            args.Handled(winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Right, GetFindNextElementOptions()));
        }
        else
        {
            args.Handled(winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Right));
        }
        break;
    case winrt::VirtualKey::Left:
        if (args.OriginalKey() != winrt::VirtualKey::GamepadDPadLeft)
        {
            args.Handled(winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Left, GetFindNextElementOptions()));
        }
        else
        {
            args.Handled(winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Left));
        }
        break;
    }
}

winrt::FindNextElementOptions RadioButtons::GetFindNextElementOptions()
{
    auto const findNextElementOptions = winrt::FindNextElementOptions{};
    findNextElementOptions.SearchRoot(*this);
    return findNextElementOptions;
}

// Selection follows focus unless control key is held down.
void RadioButtons::OnChildGotFocus(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    if (!m_isControlDown)
    {
        if (auto const repeater = m_repeater.get())
        {
            if (auto const originalSourceAsUIE = args.OriginalSource().as<winrt::UIElement>())
            {
                auto const index = repeater.GetElementIndex(originalSourceAsUIE);
                if (index >= 0)
                {
                    m_selectionModel.Select(index);
                }
            }
        }
    }
}

void RadioButtons::OnRepeaterElementPrepared(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementPreparedEventArgs& args)
{
    if (auto const element = args.Element())
    {
        if (auto const toggleButton = element.try_as<winrt::ToggleButton>())
        {
            auto childHandlers = winrt::make_self<ChildHandlers>();
            childHandlers->checkedRevoker = toggleButton.Checked(winrt::auto_revoke, { this, &RadioButtons::OnChildChecked });
            childHandlers->uncheckedRevoker = toggleButton.Unchecked(winrt::auto_revoke, { this, &RadioButtons::OnChildUnchecked });
                
            toggleButton.SetValue(s_childHandlersProperty, childHandlers.as<winrt::IInspectable>());
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
    if (auto const element = args.Element())
    {
        element.SetValue(s_childHandlersProperty, nullptr);
    }
}

void RadioButtons::OnRepeaterElementIndexChanged(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    if (auto const element = args.Element())
    {
        element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(args.NewIndex() + 1));
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
                if (auto const radioButton = repeater.TryGetElement(index))
                {
                    radioButton.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(count));
                }
            }
        }
    }
}

void RadioButtons::OnSelectionChanged(const winrt::IInspectable&, const winrt::SelectionModelSelectionChangedEventArgs& args)
{
    auto const currentIndex = m_selectionModel.SelectedIndex();
    auto const repeater = m_repeater.get();

    if (repeater)
    {
        if (auto const previousIndices = args.PreviousSelectedIndices())
        {
            for (auto const previousIndex : previousIndices)
            {
                if (previousIndex &&
                    (!currentIndex || previousIndex.CompareTo(currentIndex) != 0) &&
                    previousIndex.GetSize() == 1)
                {
                    if (auto const item = repeater.TryGetElement(previousIndex.GetAt(0)))
                    {
                        if (auto const itemAsToggleButton = item.try_as<winrt::ToggleButton>())
                        {
                            itemAsToggleButton.IsChecked(false);
                        }
                    }
                }
            }
        }
    }

    if (currentIndex && currentIndex.GetSize() == 1)
    {
        auto const index = currentIndex.GetAt(0);
        if (repeater)
        {
            if (auto const item = repeater.TryGetElement(index))
            {
                if (auto const itemAsToggleButton = item.try_as<winrt::ToggleButton>())
                {
                    itemAsToggleButton.IsChecked(true);
                }
            }
        }
        UpdateSelectionDPs(index);
    }
    else
    {
        UpdateSelectionDPs(-1);
    }
}

void RadioButtons::UpdateSelectionDPs(const int newIndex)
{
    if (auto const repeater = m_repeater.get())
    {
        auto const oldSelectedItemsRepeaterIndex = [selectedItem = SelectedItem(), repeater]()
        {
            if (selectedItem)
            {
                return repeater.GetElementIndex(selectedItem.try_as<winrt::UIElement>());
            }
            return -1;
        }();
        if (SelectedIndex() != newIndex || oldSelectedItemsRepeaterIndex != newIndex)
        {
            SelectedIndex(newIndex);
            auto const eventArgs = [this, newIndex, repeater]()
            {
                auto const previousSelectedItem = SelectedItem();
                if (auto const newSelectedItem = repeater.TryGetElement(newIndex))
                {
                    SelectedItem(newSelectedItem);
                    return winrt::SelectionChangedEventArgs({ previousSelectedItem }, { newSelectedItem });
                }
                SelectedItem(nullptr);
                return winrt::SelectionChangedEventArgs({ previousSelectedItem }, { nullptr });
            }();
            m_selectionChangedEventSource(*this, eventArgs);
        }
    }
}

void RadioButtons::OnChildChecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&)
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const senderAsUIE = sender.as<winrt::UIElement>())
        {
            m_selectionModel.Select(repeater.GetElementIndex(senderAsUIE));
        }
    }
}

void RadioButtons::OnChildUnchecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&)
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const senderAsUIE = sender.as<winrt::UIElement>())
        {
            m_selectionModel.Deselect(repeater.GetElementIndex(senderAsUIE));
        }
    }
}

bool RadioButtons::MoveFocusNext()
{
    return MoveFocus(1, MissStrategy::next);
}

bool RadioButtons::MoveFocusPrevious()
{
    return MoveFocus(-1, MissStrategy::previous);
}

bool RadioButtons::MoveFocus(int initialIndexIncrement, MissStrategy missStrategy)
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const focusedElement = winrt::FocusManager::GetFocusedElement().try_as<winrt::UIElement>())
        {
            auto focusedIndex = repeater.GetElementIndex(focusedElement);
            
            if (focusedIndex >= 0)
            {
                focusedIndex += initialIndexIncrement;
                auto const itemCount = repeater.ItemsSourceView().Count();
                while (focusedIndex >= 0 && focusedIndex < itemCount)
                {
                    if (auto const item = repeater.TryGetElement(focusedIndex))
                    {
                        if (auto const itemAsControl = item.try_as<winrt::IControl>())
                        {
                            if (itemAsControl.IsEnabled() && itemAsControl.IsTabStop())
                            {
                                itemAsControl.Focus(winrt::Windows::UI::Xaml::FocusState::Keyboard);
                                return true;
                            }
                        }
                    }
                    focusedIndex += initialIndexIncrement;
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
    else if (property == s_MaximumColumnsProperty)
    {
        UpdateMaximumColumns();
    }
    else if (property == s_SelectedIndexProperty)
    {
        UpdateSelectedIndex();
    }
    else if (property == s_SelectedItemProperty)
    {
        UpdateSelectedItem();
    }
}

void RadioButtons::UpdateItemsSource()
{
    if (auto const repeater = m_repeater.get())
    {
        m_itemsSourceChanged.revoke();
        auto const source = GetItemsSource();

        if (auto const item = repeater.TryGetElement(SelectedIndex()))
        {
            if (auto const itemAsToggleButton = item.try_as<winrt::ToggleButton>())
            {
                itemAsToggleButton.IsChecked(false);
            }
        }
        m_selectionModel.ClearSelection();
        repeater.ItemsSource(source);
        m_selectionModel.Source(source);

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

void RadioButtons::UpdateMaximumColumns()
{
    auto const maxColumns = MaximumColumns();
    if (maxColumns <= 0)
    {
        throw winrt::hresult_invalid_argument();
    }

    if (auto const repeater = m_repeater.get())
    {
        if (auto const layout = repeater.Layout())
        {
            if (auto const customlayout = layout.try_as<winrt::ColumnMajorUniformToLargestGridLayout>())
            {
                if (customlayout.MaximumColumns() != maxColumns)
                {
                    customlayout.MaximumColumns(maxColumns);
                }
            }
        }
    }
}

void RadioButtons::UpdateSelectedItem()
{
    if (auto const selectedItem = SelectedItem())
    {
        if (auto const repeater = m_repeater.get())
        {
            if (auto const itemsSourceView = repeater.ItemsSourceView())
            {
                if (auto const selectedItemAsUIElement = selectedItem.try_as<winrt::UIElement>())
                {
                    m_selectionModel.Select(repeater.GetElementIndex(selectedItemAsUIElement));
                }
            }
        }
    }
    else
    {
        m_selectionModel.ClearSelection();
    }
}

void RadioButtons::UpdateSelectedIndex()
{
    m_selectionModel.Select(SelectedIndex());
}

winrt::UIElement RadioButtons::ContainerFromItem(winrt::IInspectable const& item)
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const itemTemplate = repeater.ItemTemplate())
        {
            if (auto const itemFactory = itemTemplate.try_as<winrt::RadioButtonsElementFactory>())
            {
                return itemFactory.ContainerFromItem(item);
            }
        }
    }
    return nullptr;
}

winrt::UIElement RadioButtons::ContainerFromIndex(int index)
{
    if (auto const repeater = m_repeater.get())
    {
        return repeater.TryGetElement(index);
    }
    return nullptr;
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

ColumnMajorUniformToLargestGridLayout* RadioButtons::GetLayout()
{
    if (auto const repeater = m_repeater.get())
    {
        if (auto const layout = repeater.Layout())
        {
            if (auto const customLayout = layout.try_as<winrt::ColumnMajorUniformToLargestGridLayout>())
            {
                return winrt::get_self<ColumnMajorUniformToLargestGridLayout>(customLayout);
            }
        }
    }
    return nullptr;
}
