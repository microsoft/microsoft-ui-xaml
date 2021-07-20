﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TabView.h"
#include "TabViewItem.h"
#include "TabViewAutomationPeer.h"
#include "DoubleUtil.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "SharedHelpers.h"
#include <Vector.h>

static constexpr double c_tabMinimumWidth = 48.0;
static constexpr double c_tabMaximumWidth = 200.0;

static constexpr wstring_view c_tabViewItemMinWidthName{ L"TabViewItemMinWidth"sv };
static constexpr wstring_view c_tabViewItemMaxWidthName{ L"TabViewItemMaxWidth"sv };

// TODO: what is the right number and should this be customizable?
static constexpr double c_scrollAmount = 50.0;

TabView::TabView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TabView);

    auto items = winrt::make<Vector<winrt::IInspectable, MakeVectorParam<VectorFlag::Observable>()>>();
    SetValue(s_TabItemsProperty, items);

    SetDefaultStyleKey(this);

    Loaded({ this, &TabView::OnLoaded });

    // KeyboardAccelerator is only available on RS3+
    if (SharedHelpers::IsRS3OrHigher())
    {
        winrt::KeyboardAccelerator ctrlf4Accel;
        ctrlf4Accel.Key(winrt::VirtualKey::F4);
        ctrlf4Accel.Modifiers(winrt::VirtualKeyModifiers::Control);
        ctrlf4Accel.Invoked({ this, &TabView::OnCtrlF4Invoked });
        ctrlf4Accel.ScopeOwner(*this);
        KeyboardAccelerators().Append(ctrlf4Accel);

        m_tabCloseButtonTooltipText = ResourceAccessor::GetLocalizedStringResource(SR_TabViewCloseButtonTooltipWithKA);
    }
    else
    {
        m_tabCloseButtonTooltipText = ResourceAccessor::GetLocalizedStringResource(SR_TabViewCloseButtonTooltip);
    }

    // Ctrl+Tab as a KeyboardAccelerator only works on 19H1+
    if (SharedHelpers::Is19H1OrHigher())
    {
        winrt::KeyboardAccelerator ctrlTabAccel;
        ctrlTabAccel.Key(winrt::VirtualKey::Tab);
        ctrlTabAccel.Modifiers(winrt::VirtualKeyModifiers::Control);
        ctrlTabAccel.Invoked({ this, &TabView::OnCtrlTabInvoked });
        ctrlTabAccel.ScopeOwner(*this);
        KeyboardAccelerators().Append(ctrlTabAccel);

        const winrt::KeyboardAccelerator ctrlShiftTabAccel;
        ctrlShiftTabAccel.Key(winrt::VirtualKey::Tab);
        ctrlShiftTabAccel.Modifiers(winrt::VirtualKeyModifiers::Control | winrt::VirtualKeyModifiers::Shift);
        ctrlShiftTabAccel.Invoked({ this, &TabView::OnCtrlShiftTabInvoked });
        ctrlShiftTabAccel.ScopeOwner(*this);
        KeyboardAccelerators().Append(ctrlShiftTabAccel);
    }
}

void TabView::OnApplyTemplate()
{
    UnhookEventsAndClearFields();

    winrt::IControlProtected controlProtected{ *this };

    m_tabContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(L"TabContentPresenter", controlProtected));
    m_rightContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(L"RightContentPresenter", controlProtected));

    m_leftContentColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"LeftContentColumn", controlProtected));
    m_tabColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"TabColumn", controlProtected));
    m_addButtonColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"AddButtonColumn", controlProtected));
    m_rightContentColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"RightContentColumn", controlProtected));

    if (const auto& containerGrid = GetTemplateChildT<winrt::Grid>(L"TabContainerGrid", controlProtected))
    {
        m_tabContainerGrid.set(containerGrid);
        m_tabStripPointerExitedRevoker = containerGrid.PointerExited(winrt::auto_revoke, { this,&TabView::OnTabStripPointerExited });
    }

    if (!SharedHelpers::Is21H1OrHigher())
    {
        m_shadowReceiver.set(GetTemplateChildT<winrt::Grid>(L"ShadowReceiver", controlProtected));
    }

    m_listView.set([this, controlProtected]() {
        auto listView = GetTemplateChildT<winrt::ListView>(L"TabListView", controlProtected);
        if (listView)
        {
            m_listViewLoadedRevoker = listView.Loaded(winrt::auto_revoke, { this, &TabView::OnListViewLoaded });
            m_listViewSelectionChangedRevoker = listView.SelectionChanged(winrt::auto_revoke, { this, &TabView::OnListViewSelectionChanged });

            m_listViewDragItemsStartingRevoker = listView.DragItemsStarting(winrt::auto_revoke, { this, &TabView::OnListViewDragItemsStarting });
            m_listViewDragItemsCompletedRevoker = listView.DragItemsCompleted(winrt::auto_revoke, { this, &TabView::OnListViewDragItemsCompleted });
            m_listViewDragOverRevoker = listView.DragOver(winrt::auto_revoke, { this, &TabView::OnListViewDragOver });
            m_listViewDropRevoker = listView.Drop(winrt::auto_revoke, { this, &TabView::OnListViewDrop });

            m_listViewGettingFocusRevoker = listView.GettingFocus(winrt::auto_revoke, { this, &TabView::OnListViewGettingFocus });

            m_listViewCanReorderItemsPropertyChangedRevoker = RegisterPropertyChanged(listView, winrt::ListViewBase::CanReorderItemsProperty(), { this, &TabView::OnListViewDraggingPropertyChanged });
            m_listViewAllowDropPropertyChangedRevoker = RegisterPropertyChanged(listView, winrt::UIElement::AllowDropProperty(), { this, &TabView::OnListViewDraggingPropertyChanged });
        }
        return listView;
    }());

    m_addButton.set([this, controlProtected]() {
        auto addButton = GetTemplateChildT<winrt::Button>(L"AddButton", controlProtected);
        if (addButton)
        {
            // Do localization for the add button
            if (winrt::AutomationProperties::GetName(addButton).empty())
            {
                auto addButtonName = ResourceAccessor::GetLocalizedStringResource(SR_TabViewAddButtonName);
                winrt::AutomationProperties::SetName(addButton, addButtonName);
            }

            auto toolTip = winrt::ToolTipService::GetToolTip(addButton);
            if (!toolTip)
            {
                winrt::ToolTip tooltip = winrt::ToolTip();
                tooltip.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_TabViewAddButtonTooltip)));
                winrt::ToolTipService::SetToolTip(addButton, tooltip);
            }

            m_addButtonClickRevoker = addButton.Click(winrt::auto_revoke, { this, &TabView::OnAddButtonClick });
        }
        return addButton;
    }());

    if (SharedHelpers::IsThemeShadowAvailable())
    {
        if (!SharedHelpers::Is21H1OrHigher())
        {
            if (auto shadowCaster = GetTemplateChildT<winrt::Grid>(L"ShadowCaster", controlProtected))
            {
                winrt::ThemeShadow shadow;
                shadow.Receivers().Append(GetShadowReceiver());

                double shadowDepth = unbox_value<double>(SharedHelpers::FindInApplicationResources(c_tabViewShadowDepthName, box_value(c_tabShadowDepth)));

                const auto currentTranslation = shadowCaster.Translation();
                const auto translation = winrt::float3{ currentTranslation.x, currentTranslation.y, (float)shadowDepth };
                shadowCaster.Translation(translation);

                shadowCaster.Shadow(shadow);
            }
        }
    }

    UpdateListViewItemContainerTransitions();
}


void TabView::SetTabSeparatorOpacity(int index, int opacityValue)
{
    if (const auto tvi = ContainerFromIndex(index).try_as<winrt::TabViewItem>())
    {
        // The reason we set the opacity directly instead of using VisualState
        // is because we want to hide the separator on hover/pressed
        // but the tab adjacent on the left to the selected tab
        // must hide the tab separator at all times.
        // It causes two visual states to modify the same property
        // what leads to undesired behaviour.
        if (const auto tabSeparator = tvi.GetTemplateChild(L"TabSeparator").try_as<winrt::FrameworkElement>())
        {
            tabSeparator.Opacity(opacityValue);
        }
    }
}


void TabView::SetTabSeparatorOpacity(int index)
{
    const auto selectedIndex = SelectedIndex();

    // If Tab is adjacent on the left to selected one or
    // it is selected tab - we hide the tabSeparator.
    if (index == selectedIndex || index + 1 == selectedIndex)
    {
        SetTabSeparatorOpacity(index, 0);
    }
    else
    {
        SetTabSeparatorOpacity(index, 1);
    }
}


void TabView::OnListViewDraggingPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    UpdateListViewItemContainerTransitions();
}

void TabView::OnListViewGettingFocus(const winrt::IInspectable& sender, const winrt::GettingFocusEventArgs& args)
{
    // TabViewItems overlap each other by one pixel in order to get the desired visuals for the separator.
    // This causes problems with 2d focus navigation. Because the items overlap, pressing Down or Up from a
    // TabViewItem navigates to the overlapping item which is not desired.
    //
    // To resolve this issue, we detect the case where Up or Down focus navigation moves from one TabViewItem
    // to another.
    // How we handle it, depends on the input device.
    // For GamePad, we want to move focus to something in the direction of movement (other than the overlapping item)
    // For Keyboard, we cancel the focus movement.

    const auto direction = args.Direction();
    if (direction == winrt::FocusNavigationDirection::Up || direction == winrt::FocusNavigationDirection::Down)
    {
        auto oldItem = args.OldFocusedElement().try_as<winrt::TabViewItem>();
        auto newItem = args.NewFocusedElement().try_as<winrt::TabViewItem>();
        if (oldItem && newItem)
        {
            if (auto&& listView = m_listView.get())
            {
                bool oldItemIsFromThisTabView = listView.IndexFromContainer(oldItem) != -1;
                bool newItemIsFromThisTabView = listView.IndexFromContainer(newItem) != -1;
                if (oldItemIsFromThisTabView && newItemIsFromThisTabView)
                {
                    const auto inputDevice = args.InputDevice();
                    if (inputDevice == winrt::FocusInputDeviceKind::GameController)
                    {
                        const auto listViewBoundsLocal = winrt::Rect{ 0, 0, static_cast<float>(listView.ActualWidth()), static_cast<float>(listView.ActualHeight()) };
                        const auto listViewBounds = listView.TransformToVisual(nullptr).TransformBounds(listViewBoundsLocal);
                        const winrt::FindNextElementOptions options;
                        options.ExclusionRect(listViewBounds);
                        const auto next = winrt::FocusManager::FindNextElement(direction, options);
                        if (const auto args2 = args.try_as<winrt::IGettingFocusEventArgs2>())
                        {
                            args2.TrySetNewFocusedElement(next);
                        }
                        else
                        {
                            // Without TrySetNewFocusedElement, we cannot set focus while it is changing.
                            m_dispatcherHelper.RunAsync([next]()
                            {
                                SetFocus(next, winrt::FocusState::Programmatic);
                            });
                        }
                        args.Handled(true);
                    }
                    else
                    {
                        args.Cancel(true);
                        args.Handled(true);
                    }
                }
            }
        }
    }
}

void TabView::OnSelectedIndexPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // We update previous selected and adjacent on the left tab
    // as well as current selected and adjacent on the left tab
    // to show/hide tabSeparator accordingly.
    UpdateSelectedIndex();
    SetTabSeparatorOpacity(winrt::unbox_value<int>(args.OldValue()));
    SetTabSeparatorOpacity(winrt::unbox_value<int>(args.OldValue()) - 1);
    SetTabSeparatorOpacity(SelectedIndex() - 1);
    SetTabSeparatorOpacity(SelectedIndex());
}

void TabView::OnSelectedItemPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateSelectedItem();
}

void TabView::OnTabItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateListViewItemContainerTransitions();
}

void TabView::UpdateListViewItemContainerTransitions()
{
    if (TabItemsSource())
    {
        if (auto&& listView = m_listView.get())
        {
            if (listView.CanReorderItems() && listView.AllowDrop())
            {
                // Remove all the AddDeleteThemeTransition/ContentThemeTransition instances in the inner ListView's ItemContainerTransitions
                // collection to avoid attempting to reparent a tab's content while it is still parented during a tab reordering user gesture.
                // This is only required when:
                //  - the TabViewItem' contents are databound to UIElements (this condition is not being checked below though).
                //  - System animations turned on (this condition is not being checked below though to maximize behavior consistency).
                //  - TabViewItem reordering is turned on.
                // With all those conditions met, the databound UIElements are still parented to the old item container as the tab is being dropped in
                // its new location. Without animations, the old item container is already put into the recycling pool and picked as the new container.
                // Its ContentControl.Content is kept unchanged and no reparenting is attempted.
                // Because the default ItemContainerTransitions collection is defined in the TabViewListView style, all ListView instances share the same
                // collection by default. Thus to avoid one TabView affecting all other ones, a new ItemContainerTransitions collection is created
                // when the original one contains an AddDeleteThemeTransition or ContentThemeTransition instance.
                bool transitionCollectionHasAddDeleteOrContentThemeTransition = [listView]()
                {
                    if (auto itemContainerTransitions = listView.ItemContainerTransitions())
                    {
                        for (auto&& transition : itemContainerTransitions)
                        {
                            if (transition &&
                                (transition.try_as<winrt::AddDeleteThemeTransition>() || transition.try_as<winrt::ContentThemeTransition>()))
                            {
                                return true;
                            }
                        }
                    }
                    return false;
                }();

                if (transitionCollectionHasAddDeleteOrContentThemeTransition)
                {
                    auto const newItemContainerTransitions = winrt::TransitionCollection();
                    auto const oldItemContainerTransitions = listView.ItemContainerTransitions();

                    for (auto&& transition : oldItemContainerTransitions)
                    {
                        if (transition)
                        {
                            if (transition.try_as<winrt::AddDeleteThemeTransition>() || transition.try_as<winrt::ContentThemeTransition>())
                            {
                                continue;
                            }
                            newItemContainerTransitions.Append(transition);
                        }
                    }

                    listView.ItemContainerTransitions(newItemContainerTransitions);
                }
            }
        }
    }
}

void TabView::UnhookEventsAndClearFields()
{
    m_listViewLoadedRevoker.revoke();
    m_listViewSelectionChangedRevoker.revoke();
    m_listViewDragItemsStartingRevoker.revoke();
    m_listViewDragItemsCompletedRevoker.revoke();
    m_listViewDragOverRevoker.revoke();
    m_listViewDropRevoker.revoke();
    m_listViewGettingFocusRevoker.revoke();
    m_listViewCanReorderItemsPropertyChangedRevoker.revoke();
    m_listViewAllowDropPropertyChangedRevoker.revoke();
    m_addButtonClickRevoker.revoke();
    m_itemsPresenterSizeChangedRevoker.revoke();
    m_tabStripPointerExitedRevoker.revoke();
    m_scrollViewerLoadedRevoker.revoke();
    m_scrollViewerViewChangedRevoker.revoke();
    m_scrollDecreaseClickRevoker.revoke();
    m_scrollIncreaseClickRevoker.revoke();

    m_tabContentPresenter.set(nullptr);
    m_rightContentPresenter.set(nullptr);
    m_leftContentColumn.set(nullptr);
    m_tabColumn.set(nullptr);
    m_addButtonColumn.set(nullptr);
    m_rightContentColumn.set(nullptr);
    m_tabContainerGrid.set(nullptr);
    m_shadowReceiver.set(nullptr);
    m_listView.set(nullptr);
    m_addButton.set(nullptr);
    m_itemsPresenter.set(nullptr);
    m_scrollViewer.set(nullptr);
    m_scrollDecreaseButton.set(nullptr);
    m_scrollIncreaseButton.set(nullptr);
}

void TabView::OnTabWidthModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateTabWidths();

    // Switch the visual states of all tab items to the correct TabViewWidthMode
    for (auto&& item : TabItems())
    {
        auto const tvi = [item, this]()
        {
            if (auto tabViewItem = item.try_as<TabViewItem>())
            {
                return tabViewItem;
            }
            return ContainerFromItem(item).try_as<TabViewItem>();
        }();

        if (tvi)
        {
            tvi->OnTabViewWidthModeChanged(TabWidthMode());
        }
    }
}

void TabView::OnCloseButtonOverlayModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    // Switch the visual states of all tab items to to the correct closebutton overlay mode
    for (auto&& item : TabItems())
    {
        auto const tvi = [item, this]()
        {
            if (auto tabViewItem = item.try_as<TabViewItem>())
            {
                return tabViewItem;
            }
            return ContainerFromItem(item).try_as<TabViewItem>();
        }();

        if (tvi)
        {
            tvi->OnCloseButtonOverlayModeChanged(CloseButtonOverlayMode());
        }
    }
}

void TabView::OnAddButtonClick(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    m_addTabButtonClickEventSource(*this, args);
}

winrt::AutomationPeer TabView::OnCreateAutomationPeer()
{
    return winrt::make<TabViewAutomationPeer>(*this);
}

void TabView::OnLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    UpdateTabContent();
}

void TabView::OnListViewLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    if (auto&& listView = m_listView.get())
    {
        // Now that ListView exists, we can start using its Items collection.
        if (auto const lvItems = listView.Items())
        {
            if (!listView.ItemsSource())
            {
                // copy the list, because clearing lvItems may also clear TabItems
                winrt::IVector<winrt::IInspectable> const itemList{ winrt::single_threaded_vector<winrt::IInspectable>() };

                for (auto const item : TabItems())
                {
                    itemList.Append(item);
                }

                lvItems.Clear();

                for (auto const item : itemList)
                {
                    // App put items in our Items collection; copy them over to ListView.Items
                    if (item)
                    {
                        lvItems.Append(item);
                    }
                }
            }
            TabItems(lvItems);
        }

        if (ReadLocalValue(s_SelectedItemProperty) != winrt::DependencyProperty::UnsetValue())
        {
            UpdateSelectedItem();
        }
        else
        {
            // If SelectedItem wasn't set, default to selecting the first tab
            UpdateSelectedIndex();
        }

        SelectedIndex(listView.SelectedIndex());
        SelectedItem(listView.SelectedItem());

        // Find TabsItemsPresenter and listen for SizeChanged
        m_itemsPresenter.set([this, listView]() {
            auto itemsPresenter = SharedHelpers::FindInVisualTreeByName(listView, L"TabsItemsPresenter").as<winrt::ItemsPresenter>();
            if (itemsPresenter)
            {
                m_itemsPresenterSizeChangedRevoker = itemsPresenter.SizeChanged(winrt::auto_revoke, { this, &TabView::OnItemsPresenterSizeChanged });
            }
            return itemsPresenter;
            }());

        auto scrollViewer = SharedHelpers::FindInVisualTreeByName(listView, L"ScrollViewer").as<winrt::FxScrollViewer>();
        m_scrollViewer.set(scrollViewer);
        if (scrollViewer)
        {
            if (SharedHelpers::IsIsLoadedAvailable() && scrollViewer.IsLoaded())
            {
                // This scenario occurs reliably for Terminal in XAML islands
                OnScrollViewerLoaded(nullptr, nullptr);
            }
            else
            {
                m_scrollViewerLoadedRevoker = scrollViewer.Loaded(winrt::auto_revoke, { this, &TabView::OnScrollViewerLoaded });
            }
        }
    }
}

void TabView::OnTabStripPointerExited(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (m_updateTabWidthOnPointerLeave)
    {
        auto scopeGuard = gsl::finally([this]()
        {
            m_updateTabWidthOnPointerLeave = false;
        });
        UpdateTabWidths();
    }
}

void TabView::OnScrollViewerLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    if (auto&& scrollViewer = m_scrollViewer.get())
    {
        m_scrollDecreaseButton.set([this, scrollViewer]() {
            const auto decreaseButton = SharedHelpers::FindInVisualTreeByName(scrollViewer, L"ScrollDecreaseButton").as<winrt::RepeatButton>();
            if (decreaseButton)
            {
                // Do localization for the scroll decrease button
                const auto toolTip = winrt::ToolTipService::GetToolTip(decreaseButton);
                if (!toolTip)
                {
                    const auto tooltip = winrt::ToolTip();
                    tooltip.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_TabViewScrollDecreaseButtonTooltip)));
                    winrt::ToolTipService::SetToolTip(decreaseButton, tooltip);
                }

                m_scrollDecreaseClickRevoker = decreaseButton.Click(winrt::auto_revoke, { this, &TabView::OnScrollDecreaseClick });
            }
            return decreaseButton;
        }());

        m_scrollIncreaseButton.set([this, scrollViewer]() {
            const auto increaseButton = SharedHelpers::FindInVisualTreeByName(scrollViewer, L"ScrollIncreaseButton").as<winrt::RepeatButton>();
            if (increaseButton)
            {
                // Do localization for the scroll increase button
                const auto toolTip = winrt::ToolTipService::GetToolTip(increaseButton);
                if (!toolTip)
                {
                    const auto tooltip = winrt::ToolTip();
                    tooltip.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_TabViewScrollIncreaseButtonTooltip)));
                    winrt::ToolTipService::SetToolTip(increaseButton, tooltip);
                }

                m_scrollIncreaseClickRevoker = increaseButton.Click(winrt::auto_revoke, { this, &TabView::OnScrollIncreaseClick });
            }
            return increaseButton;
        }());

        m_scrollViewerViewChangedRevoker = scrollViewer.ViewChanged(winrt::auto_revoke, { this, &TabView::OnScrollViewerViewChanged });
    }

    UpdateTabWidths();
}

void TabView::OnScrollViewerViewChanged(winrt::IInspectable const& sender, winrt::ScrollViewerViewChangedEventArgs const& args)
{
    UpdateScrollViewerDecreaseAndIncreaseButtonsViewState();
}

void TabView::UpdateScrollViewerDecreaseAndIncreaseButtonsViewState()
{
    if (auto&& scrollViewer = m_scrollViewer.get())
    {
        auto&& decreaseButton = m_scrollDecreaseButton.get();
        auto&& increaseButton = m_scrollIncreaseButton.get();

        constexpr auto minThreshold = 0.1;
        const auto horizontalOffset = scrollViewer.HorizontalOffset();
        const auto scrollableWidth = scrollViewer.ScrollableWidth();

        if (abs(horizontalOffset - scrollableWidth) < minThreshold)
        {
            if (decreaseButton)
            {
                decreaseButton.IsEnabled(true);
            }
            if (increaseButton)
            {
                increaseButton.IsEnabled(false);
            }
        }
        else if (abs(horizontalOffset) < minThreshold)
        {
            if (decreaseButton)
            {
                decreaseButton.IsEnabled(false);
            }
            if (increaseButton)
            {
                increaseButton.IsEnabled(true);
            }
        }
        else
        {
            if (decreaseButton)
            {
                decreaseButton.IsEnabled(true);
            }
            if (increaseButton)
            {
                increaseButton.IsEnabled(true);
            }
        }
    }
}

void TabView::OnItemsPresenterSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args)
{
    if (!m_updateTabWidthOnPointerLeave)
    {
        // Presenter size didn't change because of item being removed, so update manually
        UpdateScrollViewerDecreaseAndIncreaseButtonsViewState();
        UpdateTabWidths();
    }
}

void TabView::OnItemsChanged(winrt::IInspectable const& item)
{
    if (auto args = item.as<winrt::IVectorChangedEventArgs>())
    {
        m_tabItemsChangedEventSource(*this, args);

        int numItems = static_cast<int>(TabItems().Size());
        const auto listViewInnerSelectedIndex = m_listView.get().SelectedIndex();
        auto selectedIndex = SelectedIndex();
        
        if (selectedIndex != listViewInnerSelectedIndex && listViewInnerSelectedIndex != -1)
        {
            SelectedIndex(listViewInnerSelectedIndex);
            selectedIndex = listViewInnerSelectedIndex;
        }

        if (args.CollectionChange() == winrt::CollectionChange::ItemRemoved)
        {
            m_updateTabWidthOnPointerLeave = true;
            if (numItems > 0)
            {
                // SelectedIndex might also already be -1
                if (selectedIndex == -1 || selectedIndex == static_cast<int32_t>(args.Index()))
                {
                    // Find the closest tab to select instead.
                    int startIndex = static_cast<int>(args.Index());
                    if (startIndex >= numItems)
                    {
                        startIndex = numItems - 1;
                    }
                    int index = startIndex;

                    do
                    {
                        const auto nextItem = ContainerFromIndex(index).as<winrt::ListViewItem>();

                        if (nextItem && nextItem.IsEnabled() && nextItem.Visibility() == winrt::Visibility::Visible)
                        {
                            SelectedItem(TabItems().GetAt(index));
                            break;
                        }

                        // try the next item
                        index++;
                        if (index >= numItems)
                        {
                            index = 0;
                        }
                    } while (index != startIndex);
                }

            }
            // Last item removed, update sizes
            // The index of the last element is "Size() - 1", but in TabItems, it is already removed.
            if (TabWidthMode() == winrt::TabViewWidthMode::Equal)
            {
                m_updateTabWidthOnPointerLeave = true;
                if (args.Index() == TabItems().Size())
                {
                    UpdateTabWidths(true, false);
                }
            }
        }
        else
        {
            UpdateTabWidths();
            SetTabSeparatorOpacity(numItems - 1);
        }
    }

}

void TabView::OnListViewSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args)
{

    
    if (auto&& listView = m_listView.get())
    {
        SelectedIndex(listView.SelectedIndex());
        SelectedItem(listView.SelectedItem());
    }

    UpdateTabContent();

    m_selectionChangedEventSource(*this, args);
}

winrt::TabViewItem TabView::FindTabViewItemFromDragItem(const winrt::IInspectable& item)
{
    auto tab = ContainerFromItem(item).try_as<winrt::TabViewItem>();

    if (!tab)
    {
        if (auto fe = item.try_as<winrt::FrameworkElement>())
        {
            tab = winrt::VisualTreeHelper::GetParent(fe).try_as<winrt::TabViewItem>();
        }
    }

    if (!tab)
    {
        // This is a fallback scenario for tabs without a data context
        auto numItems = static_cast<int>(TabItems().Size());
        for (int i = 0; i < numItems; i++)
        {
            auto tabItem = ContainerFromIndex(i).try_as<winrt::TabViewItem>();
            if (tabItem.Content() == item)
            {
                tab = tabItem;
                break;
            }
        }
    }

    return tab;
}

void TabView::OnListViewDragItemsStarting(const winrt::IInspectable& sender, const winrt::DragItemsStartingEventArgs& args)
{
    auto item = args.Items().GetAt(0);
    auto tab = FindTabViewItemFromDragItem(item);
    auto myArgs = winrt::make_self<TabViewTabDragStartingEventArgs>(args, item, tab);

    m_tabDragStartingEventSource(*this, *myArgs);
}

void TabView::OnListViewDragOver(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    m_tabStripDragOverEventSource(*this, args);
}

void TabView::OnListViewDrop(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    m_tabStripDropEventSource(*this, args);
}

void TabView::OnListViewDragItemsCompleted(const winrt::IInspectable& sender, const winrt::DragItemsCompletedEventArgs& args)
{
    auto item = args.Items().GetAt(0);
    auto tab = FindTabViewItemFromDragItem(item);
    auto myArgs = winrt::make_self<TabViewTabDragCompletedEventArgs>(args, item, tab);

    m_tabDragCompletedEventSource(*this, *myArgs);

    // None means it's outside of the tab strip area
    if (args.DropResult() == winrt::DataPackageOperation::None)
    {
        auto tabDroppedArgs = winrt::make_self<TabViewTabDroppedOutsideEventArgs>(item, tab);
        m_tabDroppedOutsideEventSource(*this, *tabDroppedArgs);
    }
}

void TabView::UpdateTabContent()
{
    if (auto&& tabContentPresenter = m_tabContentPresenter.get())
    {
        if (!SelectedItem())
        {
            tabContentPresenter.Content(nullptr);
            tabContentPresenter.ContentTemplate(nullptr);
            tabContentPresenter.ContentTemplateSelector(nullptr);
        }
        else
        {
            auto tvi = SelectedItem().try_as<winrt::TabViewItem>();
            if (!tvi)
            {
                tvi = ContainerFromItem(SelectedItem()).try_as<winrt::TabViewItem>();
            }

            if (tvi)
            {
                // If the focus was in the old tab content, we will lose focus when it is removed from the visual tree.
                // We should move the focus to the new tab content.
                // The new tab content is not available at the time of the LosingFocus event, so we need to
                // move focus later.
                bool shouldMoveFocusToNewTab = false;
                auto revoker = tabContentPresenter.LosingFocus(winrt::auto_revoke, [&shouldMoveFocusToNewTab](const winrt::IInspectable&, const winrt::LosingFocusEventArgs& args)
                {
                    shouldMoveFocusToNewTab = true;
                });

                tabContentPresenter.Content(tvi.Content());
                tabContentPresenter.ContentTemplate(tvi.ContentTemplate());
                tabContentPresenter.ContentTemplateSelector(tvi.ContentTemplateSelector());

                // It is not ideal to call UpdateLayout here, but it is necessary to ensure that the ContentPresenter has expanded its content
                // into the live visual tree.
                tabContentPresenter.UpdateLayout();

                if (shouldMoveFocusToNewTab)
                {
                    auto focusable = winrt::FocusManager::FindFirstFocusableElement(tabContentPresenter);
                    if (!focusable)
                    {
                        // If there is nothing focusable in the new tab, just move focus to the TabViewItem itself.
                        focusable = tvi;
                    }

                    if (focusable)
                    {
                        SetFocus(focusable, winrt::FocusState::Programmatic);
                    }
                }
            }
        }
    }
}

void TabView::RequestCloseTab(winrt::TabViewItem const& container, bool updateTabWidths)
{
    if (auto&& listView = m_listView.get())
    {
        auto args = winrt::make_self<TabViewTabCloseRequestedEventArgs>(listView.ItemFromContainer(container), container);

        m_tabCloseRequestedEventSource(*this, *args);

        if (auto internalTabViewItem = winrt::get_self<TabViewItem>(container))
        {
            internalTabViewItem->RaiseRequestClose(*args);
        }
    }
    UpdateTabWidths(updateTabWidths);
}

void TabView::OnScrollDecreaseClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto&& scrollViewer = m_scrollViewer.get())
    {
        scrollViewer.ChangeView(std::max(0.0, scrollViewer.HorizontalOffset() - c_scrollAmount), nullptr, nullptr);
    }
}

void TabView::OnScrollIncreaseClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto&& scrollViewer = m_scrollViewer.get())
    {
        scrollViewer.ChangeView(std::min(scrollViewer.ScrollableWidth(), scrollViewer.HorizontalOffset() + c_scrollAmount), nullptr, nullptr);
    }
}

winrt::Size TabView::MeasureOverride(winrt::Size const& availableSize)
{
    if (previousAvailableSize.Width != availableSize.Width)
    {
        previousAvailableSize = availableSize;
        UpdateTabWidths();
    }

    return __super::MeasureOverride(availableSize);
}

void TabView::UpdateTabWidths(bool shouldUpdateWidths, bool fillAllAvailableSpace)
{
    double tabWidth = std::numeric_limits<double>::quiet_NaN();

    if (auto&& tabGrid = m_tabContainerGrid.get())
    {
        // Add up width taken by custom content and + button
        double widthTaken = 0.0;
        if (auto&& leftContentColumn = m_leftContentColumn.get())
        {
            widthTaken += leftContentColumn.ActualWidth();
        }
        if (auto&& addButtonColumn = m_addButtonColumn.get())
        {
            widthTaken += addButtonColumn.ActualWidth();
        }
        if (auto&& rightContentColumn = m_rightContentColumn.get())
        {
            if (auto&& rightContentPresenter = m_rightContentPresenter.get())
            {
                const winrt::Size rightContentSize = rightContentPresenter.DesiredSize();
                rightContentColumn.MinWidth(rightContentSize.Width);
                widthTaken += rightContentSize.Width;
            }
        }

        if (auto&& tabColumn = m_tabColumn.get())
        {
            // Note: can be infinite
            const auto availableWidth = previousAvailableSize.Width - widthTaken;

            // Size can be 0 when window is first created; in that case, skip calculations; we'll get a new size soon
            if (availableWidth > 0)
            {
                if (TabWidthMode() == winrt::TabViewWidthMode::Equal)
                {

                    auto const minTabWidth = unbox_value<double>(SharedHelpers::FindInApplicationResources(c_tabViewItemMinWidthName, box_value(c_tabMinimumWidth)));
                    auto const maxTabWidth = unbox_value<double>(SharedHelpers::FindInApplicationResources(c_tabViewItemMaxWidthName, box_value(c_tabMaximumWidth)));

                    // If we should fill all of the available space, use scrollviewer dimensions
                    auto const padding = Padding();
                    if (fillAllAvailableSpace)
                    {
                        // Calculate the proportional width of each tab given the width of the ScrollViewer.
                        auto const tabWidthForScroller = (availableWidth - (padding.Left + padding.Right)) / (double)(TabItems().Size());
                        tabWidth = std::clamp(tabWidthForScroller, minTabWidth, maxTabWidth);
                    }
                    else
                    {
                        double availableTabViewSpace = (tabColumn.ActualWidth() - (padding.Left + padding.Right));
                        if (const auto increaseButton = m_scrollIncreaseButton.get())
                        {
                            if (increaseButton.Visibility() == winrt::Visibility::Visible)
                            {
                                availableTabViewSpace -= increaseButton.ActualWidth();
                            }
                        }

                        if (const auto decreaseButton = m_scrollDecreaseButton.get())
                        {
                            if (decreaseButton.Visibility() == winrt::Visibility::Visible)
                            {
                                availableTabViewSpace -= decreaseButton.ActualWidth();
                            }
                        }

                        // Use current size to update items to fill the currently occupied space
                        auto const tabWidthUnclamped = availableTabViewSpace / (double)(TabItems().Size());
                        tabWidth = std::clamp(tabWidthUnclamped, minTabWidth, maxTabWidth);
                    }


                    // Size tab column to needed size
                    tabColumn.MaxWidth(availableWidth);
                    auto requiredWidth = tabWidth * TabItems().Size();
                    if (requiredWidth > availableWidth - (padding.Left + padding.Right))
                    {
                        tabColumn.Width(winrt::GridLengthHelper::FromPixels(availableWidth));
                        if (auto&& listview = m_listView.get())
                        {
                            winrt::FxScrollViewer::SetHorizontalScrollBarVisibility(listview, winrt::Windows::UI::Xaml::Controls::ScrollBarVisibility::Visible);
                            UpdateScrollViewerDecreaseAndIncreaseButtonsViewState();
                        }
                    }
                    else
                    {
                        tabColumn.Width(winrt::GridLengthHelper::FromValueAndType(1.0, winrt::GridUnitType::Auto));
                        if (auto&& listview = m_listView.get())
                        {
                            if (shouldUpdateWidths && fillAllAvailableSpace)
                            {
                                winrt::FxScrollViewer::SetHorizontalScrollBarVisibility(listview, winrt::Windows::UI::Xaml::Controls::ScrollBarVisibility::Hidden);
                            }
                            else
                            {
                                if (auto&& decreaseButton = m_scrollDecreaseButton.get())
                                {
                                    decreaseButton.IsEnabled(false);
                                }
                                if (auto&& increaseButton = m_scrollIncreaseButton.get())
                                {
                                    increaseButton.IsEnabled(false);
                                }
                            }
                        }
                    }
                }
                else
                {
                    // Case: TabWidthMode "Compact" or "FitToContent"
                    tabColumn.MaxWidth(availableWidth);
                    tabColumn.Width(winrt::GridLengthHelper::FromValueAndType(1.0, winrt::GridUnitType::Auto));
                    if (auto&& listview = m_listView.get())
                    {
                        listview.MaxWidth(availableWidth);

                        // Calculate if the scroll buttons should be visible.
                        if (auto&& itemsPresenter = m_itemsPresenter.get())
                        {
                            const auto visible = itemsPresenter.ActualWidth() > availableWidth;
                            winrt::FxScrollViewer::SetHorizontalScrollBarVisibility(listview, visible
                                ? winrt::Windows::UI::Xaml::Controls::ScrollBarVisibility::Visible
                                : winrt::Windows::UI::Xaml::Controls::ScrollBarVisibility::Hidden);
                            if (visible)
                            {
                                UpdateScrollViewerDecreaseAndIncreaseButtonsViewState();
                            }
                        }
                    }
                }
            }
        }
    }


    if (shouldUpdateWidths || TabWidthMode() != winrt::TabViewWidthMode::Equal)
    {
        for (auto item : TabItems())
        {
            // Set the calculated width on each tab.
            auto tvi = item.try_as<winrt::TabViewItem>();
            if (!tvi)
            {
                tvi = ContainerFromItem(item).as<winrt::TabViewItem>();
            }

            if (tvi)
            {
                tvi.Width(tabWidth);
            }
        }
    }
}

void TabView::UpdateSelectedItem()
{
    if (auto&& listView = m_listView.get())
    {
        listView.SelectedItem(SelectedItem());
    }
}

void TabView::UpdateSelectedIndex()
{
    if (auto&& listView = m_listView.get())
    {
        const auto selectedIndex = SelectedIndex();
        // Ensure that the selected index is within range of the items
        if (selectedIndex < static_cast<int>(listView.Items().Size()))
        {
            listView.SelectedIndex(selectedIndex);
        }
    }
}

winrt::DependencyObject TabView::ContainerFromItem(winrt::IInspectable const& item)
{
    if (auto&& listView = m_listView.get())
    {
        return listView.ContainerFromItem(item);
    }
    return nullptr;
}

winrt::DependencyObject TabView::ContainerFromIndex(int index)
{
    if (auto&& listView = m_listView.get())
    {
        return listView.ContainerFromIndex(index);
    }
    return nullptr;
}

int TabView::IndexFromContainer(winrt::DependencyObject const& container)
{
    if (auto&& listView = m_listView.get())
    {
        return listView.IndexFromContainer(container);
    }
    return -1;
}
winrt::IInspectable TabView::ItemFromContainer(winrt::DependencyObject const& container)
{
    if (auto&& listView = m_listView.get())
    {
        return listView.ItemFromContainer(container);
    }
    return nullptr;
}

int TabView::GetItemCount()
{
    if (auto itemssource = TabItemsSource())
    {
        if (auto iterable = itemssource.try_as<winrt::IIterable<winrt::IInspectable>>())
        {
            int i = 1;
            auto iter = iterable.First();
            while (iter.MoveNext())
            {
                i++;
            }
            return i;
        }
        return 0;
    }
    else
    {
        return static_cast<int>(TabItems().Size());
    }
}

bool TabView::SelectNextTab(int increment)
{
    bool handled = false;
    const int itemsSize = GetItemCount();
    if (itemsSize > 1)
    {
        auto index = SelectedIndex();
        index = (index + increment + itemsSize) % itemsSize;
        SelectedIndex(index);
        handled = true;
    }
    return handled;
}

bool TabView::RequestCloseCurrentTab()
{
    bool handled = false;
    if (auto selectedTab = SelectedItem().try_as<winrt::TabViewItem>())
    {
        if (selectedTab.IsClosable())
        {
            // Close the tab on ctrl + F4
            RequestCloseTab(selectedTab, true);
            handled = true;
        }
    }

    return handled;
}

void TabView::OnKeyDown(winrt::KeyRoutedEventArgs const& args)
{
    if (auto coreWindow = winrt::CoreWindow::GetForCurrentThread())
    {
        if (args.Key() == winrt::VirtualKey::F4)
        {
            // Handle Ctrl+F4 on RS2 and lower
            // On RS3+, it is handled by a KeyboardAccelerator
            if (!SharedHelpers::IsRS3OrHigher())
            {
                const auto isCtrlDown = (coreWindow.GetKeyState(winrt::VirtualKey::Control) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
                if (isCtrlDown)
                {
                    args.Handled(RequestCloseCurrentTab());
                }
            }
        }
        else if (args.Key() == winrt::VirtualKey::Tab)
        {
            // Handle Ctrl+Tab/Ctrl+Shift+Tab on RS5 and lower
            // On 19H1+, it is handled by a KeyboardAccelerator
            if (!SharedHelpers::Is19H1OrHigher())
            {
                const auto isCtrlDown = (coreWindow.GetKeyState(winrt::VirtualKey::Control) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
                const auto isShiftDown = (coreWindow.GetKeyState(winrt::VirtualKey::Shift) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

                if (isCtrlDown && !isShiftDown)
                {
                    args.Handled(SelectNextTab(1));
                }
                else if (isCtrlDown && isShiftDown)
                {
                    args.Handled(SelectNextTab(-1));
                }
            }
        }
    }
}

void TabView::OnCtrlF4Invoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args)
{
    args.Handled(RequestCloseCurrentTab());
}

void TabView::OnCtrlTabInvoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args)
{
    args.Handled(SelectNextTab(1));
}

void TabView::OnCtrlShiftTabInvoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args)
{
    args.Handled(SelectNextTab(-1));
}
