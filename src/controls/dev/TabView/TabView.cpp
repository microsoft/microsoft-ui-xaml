// Copyright (c) Microsoft Corporation. All rights reserved.
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
#include "velocity.h"
#include <algorithm>
#include <Windowsx.h>
#include <winuser.h>

// This needs to be here instead of in CppWinRTIncludes.h because it includes Microsoft.UI.h,
// which contains definitions of a few types like WindowId that conflict with their definitions
// in Microsoft.UI.Content.h.
#include <winrt\Microsoft.UI.Interop.h>

static constexpr double c_tabMinimumWidth = 48.0;
static constexpr double c_tabMaximumWidth = 200.0;

static constexpr wstring_view c_tabViewItemMinWidthName{ L"TabViewItemMinWidth"sv };
static constexpr wstring_view c_tabViewItemMaxWidthName{ L"TabViewItemMaxWidth"sv };

// TODO: what is the right number and should this be customizable?
static constexpr double c_scrollAmount = 50.0;

// Change to 'true' to turn on debugging outputs in Output window
bool TabViewTrace::s_IsDebugOutputEnabled{ false };
bool TabViewTrace::s_IsVerboseDebugOutputEnabled{ false };

std::list<winrt::weak_ref<winrt::TabView>> TabView::s_tabViewWithTearOutList;
HANDLE TabView::s_tabWithTearOutListMutex = ::CreateMutexW(nullptr, FALSE, nullptr);

TabViewTabTearOutWindowRequestedEventArgs::TabViewTabTearOutWindowRequestedEventArgs(winrt::IInspectable const& item, winrt::UIElement const& tab)
    : m_items({ item })
    , m_tabs({ tab })
{
}

TabViewTabTearOutRequestedEventArgs::TabViewTabTearOutRequestedEventArgs(winrt::IInspectable const& item, winrt::UIElement const& tab)
    : m_items({ item })
    , m_tabs({ tab })
{
}

TabViewExternalTornOutTabsDroppingEventArgs::TabViewExternalTornOutTabsDroppingEventArgs(winrt::IInspectable const& item, winrt::UIElement const& tab, int dropIndex)
    : m_items({ item })
    , m_tabs({ tab })
    , m_dropIndex(dropIndex)
{
}

TabViewExternalTornOutTabsDroppedEventArgs::TabViewExternalTornOutTabsDroppedEventArgs(winrt::IInspectable const& item, winrt::UIElement const& tab, int dropIndex)
    : m_items({ item })
    , m_tabs({ tab })
    , m_dropIndex(dropIndex)
{
}

TabView::TabView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TabView);

    auto items = winrt::make<Vector<winrt::IInspectable, MakeVectorParam<VectorFlag::Observable>()>>();
    SetValue(s_TabItemsProperty, items);

    SetDefaultStyleKey(this);

    Loaded({ this, &TabView::OnLoaded });
    Unloaded({ this, &TabView::OnUnloaded });

    winrt::KeyboardAccelerator ctrlf4Accel;
    ctrlf4Accel.Key(winrt::VirtualKey::F4);
    ctrlf4Accel.Modifiers(winrt::VirtualKeyModifiers::Control);
    ctrlf4Accel.Invoked({ this, &TabView::OnCtrlF4Invoked });
    ctrlf4Accel.ScopeOwner(*this);
    KeyboardAccelerators().Append(ctrlf4Accel);
    
    m_tabCloseButtonTooltipText = ResourceAccessor::GetLocalizedStringResource(SR_TabViewCloseButtonTooltipWithKA);

    winrt::KeyboardAccelerator ctrlTabAccel;
    ctrlTabAccel.Key(winrt::VirtualKey::Tab);
    ctrlTabAccel.Modifiers(winrt::VirtualKeyModifiers::Control);
    ctrlTabAccel.Invoked({ this, &TabView::OnCtrlTabInvoked });
    ctrlTabAccel.ScopeOwner(*this);
    KeyboardAccelerators().Append(ctrlTabAccel);

    winrt::KeyboardAccelerator ctrlShiftTabAccel;
    ctrlShiftTabAccel.Key(winrt::VirtualKey::Tab);
    ctrlShiftTabAccel.Modifiers(winrt::VirtualKeyModifiers::Control | winrt::VirtualKeyModifiers::Shift);
    ctrlShiftTabAccel.Invoked({ this, &TabView::OnCtrlShiftTabInvoked });
    ctrlShiftTabAccel.ScopeOwner(*this);
    KeyboardAccelerators().Append(ctrlShiftTabAccel);
}

TabView::~TabView()
{
    if (m_inputNonClientPointerSource)
    {
        m_inputNonClientPointerSource.EnteringMoveSize(m_enteringMoveSizeToken);
        m_inputNonClientPointerSource.EnteredMoveSize(m_enteredMoveSizeToken);
        m_inputNonClientPointerSource.WindowRectChanging(m_windowRectChangingToken);
        m_inputNonClientPointerSource.ExitedMoveSize(m_exitedMoveSizeToken);
    }
}

void TabView::OnApplyTemplate()
{
    UnhookEventsAndClearFields();

    m_isItemBeingDragged = false;
    m_isItemDraggedOver = false;
    m_expandedWidthForDragOver.reset();

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
        m_tabStripPointerEnteredRevoker = containerGrid.PointerEntered(winrt::auto_revoke, { this,&TabView::OnTabStripPointerEntered });
    }

    m_listView.set([this, controlProtected]() {
        auto listView = GetTemplateChildT<winrt::ListView>(L"TabListView", controlProtected);
        if (listView)
        {
            m_listViewLoadedRevoker = listView.Loaded(winrt::auto_revoke, { this, &TabView::OnListViewLoaded });
            m_listViewSelectionChangedRevoker = listView.SelectionChanged(winrt::auto_revoke, { this, &TabView::OnListViewSelectionChanged });
            m_listViewSizeChangedRevoker = listView.SizeChanged(winrt::auto_revoke, { this, &TabView::OnListViewSizeChanged });

            m_listViewDragItemsStartingRevoker = listView.DragItemsStarting(winrt::auto_revoke, { this, &TabView::OnListViewDragItemsStarting });
            m_listViewDragItemsCompletedRevoker = listView.DragItemsCompleted(winrt::auto_revoke, { this, &TabView::OnListViewDragItemsCompleted });
            m_listViewDragOverRevoker = listView.DragOver(winrt::auto_revoke, { this, &TabView::OnListViewDragOver });

            m_listViewDropRevoker = AddRoutedEventHandler<RoutedEventType::Drop>(
                listView,
                { this, &TabView::OnListViewDrop },
                true /* handledEventsToo */);
            m_listViewDragEnterRevoker = AddRoutedEventHandler<RoutedEventType::DragEnter>(
                listView,
                { this, &TabView::OnListViewDragEnter },
                true /* handledEventsToo */);
            m_listViewDragLeaveRevoker = AddRoutedEventHandler<RoutedEventType::DragLeave>(
                listView,
                { this, &TabView::OnListViewDragLeave },
                true /* handledEventsToo */);

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
            m_addButtonKeyDownRevoker = addButton.KeyDown(winrt::auto_revoke, { this, &TabView::OnAddButtonKeyDown });
        }
        return addButton;
    }());

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
                        options.SearchRoot(this->XamlRoot().Content());
                        const auto next = winrt::FocusManager::FindNextElement(direction, options);
                        
                        args.TrySetNewFocusedElement(next);
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

    UpdateBottomBorderLineVisualStates();
}

void TabView::UpdateTabBottomBorderLineVisualStates()
{
    const int numItems = static_cast<int>(TabItems().Size());
    const int selectedIndex = SelectedIndex();

    for (int i = 0; i < numItems; i++)
    {
        auto state = L"NormalBottomBorderLine";
        if (m_isItemBeingDragged)
        {
            state = L"NoBottomBorderLine";
        }
        else if (selectedIndex != -1)
        {
            if (i == selectedIndex)
            {
                state = L"NoBottomBorderLine";
            }
            else if (i == selectedIndex - 1)
            {
                state = L"LeftOfSelectedTab";
            }
            else if (i == selectedIndex + 1)
            {
                state = L"RightOfSelectedTab";
            }
        }

        if (const auto tvi = ContainerFromIndex(i).try_as<winrt::Control>())
        {
            winrt::VisualStateManager::GoToState(tvi, state, false /*useTransitions*/);
        }
    }
}

void TabView::UpdateBottomBorderLineVisualStates()
{
    // Update border line on all tabs
    UpdateTabBottomBorderLineVisualStates();

    // Update border lines on the TabView
    winrt::VisualStateManager::GoToState(*this, m_isItemBeingDragged ? L"SingleBottomBorderLine" : L"NormalBottomBorderLine", false /*useTransitions*/);

    // Update border lines in the inner TabViewListView
    if (const auto lv = m_listView.get())
    {
        winrt::VisualStateManager::GoToState(lv, m_isItemBeingDragged ? L"NoBottomBorderLine" : L"NormalBottomBorderLine", false /*useTransitions*/);
    }

    // Update border lines in the ScrollViewer
    if (const auto scroller = m_scrollViewer.get())
    {
        winrt::VisualStateManager::GoToState(scroller, m_isItemBeingDragged ? L"NoBottomBorderLine" : L"NormalBottomBorderLine", false /*useTransitions*/);
    }
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

void TabView::OnCanTearOutTabsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateTabViewWithTearOutList();
    AttachMoveSizeLoopEvents();
    UpdateNonClientRegion();
}

void TabView::UnhookEventsAndClearFields()
{
    m_listViewLoadedRevoker.revoke();
    m_listViewSelectionChangedRevoker.revoke();
    m_listViewDragItemsStartingRevoker.revoke();
    m_listViewDragItemsCompletedRevoker.revoke();
    m_listViewDragOverRevoker.revoke();
    m_listViewGettingFocusRevoker.revoke();
    m_listViewCanReorderItemsPropertyChangedRevoker.revoke();
    m_listViewAllowDropPropertyChangedRevoker.revoke();
    m_addButtonClickRevoker.revoke();
    m_itemsPresenterSizeChangedRevoker.revoke();
    m_tabStripPointerExitedRevoker.revoke();
    m_tabStripPointerEnteredRevoker.revoke();
    m_scrollViewerLoadedRevoker.revoke();
    m_scrollViewerViewChangedRevoker.revoke();
    m_scrollDecreaseClickRevoker.revoke();
    m_scrollIncreaseClickRevoker.revoke();
    m_addButtonKeyDownRevoker.revoke();

    m_tabContentPresenter.set(nullptr);
    m_rightContentPresenter.set(nullptr);
    m_leftContentColumn.set(nullptr);
    m_tabColumn.set(nullptr);
    m_addButtonColumn.set(nullptr);
    m_rightContentColumn.set(nullptr);
    m_tabContainerGrid.set(nullptr);
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
    UpdateTabViewWithTearOutList();
    AttachMoveSizeLoopEvents();
    UpdateNonClientRegion();
}

void TabView::OnUnloaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    UpdateTabViewWithTearOutList();
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
            if (scrollViewer.IsLoaded())
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

    UpdateBottomBorderLineVisualStates();
    UpdateNonClientRegion();
}

void TabView::OnTabStripPointerExited(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    m_pointerInTabstrip = false;
    if (m_updateTabWidthOnPointerLeave)
    {
        auto scopeGuard = gsl::finally([this]()
        {
            m_updateTabWidthOnPointerLeave = false;
        });
        UpdateTabWidths();
    }
}

void TabView::OnTabStripPointerEntered(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    m_pointerInTabstrip = true;
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
        // Make sure that the selected tab is fully in view and not cut off
        BringSelectedTabIntoView();
    }
}

void TabView::BringSelectedTabIntoView()
{
    if (SelectedItem())
    {
        auto tvi = SelectedItem().try_as<winrt::TabViewItem>();
        if (!tvi)
        {
            tvi = ContainerFromItem(SelectedItem()).try_as<winrt::TabViewItem>();
        }
        if (tvi)
        {
            winrt::get_self<TabViewItem>(tvi)->StartBringTabIntoView();
        }
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
            if (TabWidthMode() == winrt::TabViewWidthMode::Equal)
            {
                if (!m_pointerInTabstrip || args.Index() == TabItems().Size())
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

    UpdateBottomBorderLineVisualStates();
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

void TabView::OnListViewSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args)
{
    UpdateNonClientRegion();
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
            if (auto tabItem = ContainerFromIndex(i).try_as<winrt::TabViewItem>())
            {
                if (tabItem.Content() == item)
                {
                    tab = tabItem;
                    break;
                }
            }
        }
    }

    return tab;
}

void TabView::OnListViewDragItemsStarting(const winrt::IInspectable& sender, const winrt::DragItemsStartingEventArgs& args)
{
    m_isItemBeingDragged = true;

    auto item = args.Items().GetAt(0);
    auto tab = FindTabViewItemFromDragItem(item);
    auto myArgs = winrt::make_self<TabViewTabDragStartingEventArgs>(args, item, tab);

    m_tabDragStartingEventSource(*this, *myArgs);

    UpdateBottomBorderLineVisualStates();
}

void TabView::OnListViewDragOver(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    m_tabStripDragOverEventSource(*this, args);
}

void TabView::OnListViewDrop(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    if (!args.Handled())
    {
        m_tabStripDropEventSource(*this, args);
    }

    UpdateIsItemDraggedOver(false);
}

void TabView::OnListViewDragEnter(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    // DragEnter can occur when we're dragging an item from within this TabView,
    // which will be handled internally.  In that case, we don't want to do anything here.
    for (auto&& tabItem : TabItems())
    {
        if (auto tabViewItem = ContainerFromItem(tabItem).try_as<winrt::TabViewItem>())
        {
            if (winrt::get_self<TabViewItem>(tabViewItem)->IsBeingDragged())
            {
                return;
            }
        }
    }

    UpdateIsItemDraggedOver(true);
}

void TabView::OnListViewDragLeave(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    UpdateIsItemDraggedOver(false);
}

void TabView::OnListViewDragItemsCompleted(const winrt::IInspectable& sender, const winrt::DragItemsCompletedEventArgs& args)
{
    m_isItemBeingDragged = false;

    // Selection may have changed during drag if dragged outside, so we update SelectedIndex again.
    if (auto&& listView = m_listView.get())
    {
        SelectedIndex(listView.SelectedIndex());
        SelectedItem(listView.SelectedItem());

        BringSelectedTabIntoView();
    }

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

    UpdateBottomBorderLineVisualStates();
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
    // If the tab being closed is the currently focused tab, we'll move focus to the next tab
    // when the tab closes.
    bool tabIsFocused = false;
    auto focusedElement{ winrt::FocusManager::GetFocusedElement() ? winrt::FocusManager::GetFocusedElement().try_as<winrt::DependencyObject>() : nullptr };

    while (focusedElement)
    {
        if (focusedElement == container)
        {
            tabIsFocused = true;
            break;
        }

        focusedElement = winrt::VisualTreeHelper::GetParent(focusedElement);
    }

    winrt::UIElement::LosingFocus_revoker losingFocusRevoker{};

    if (tabIsFocused)
    {
        // If the tab specified both is focused and loses focus, then we'll move focus to an adjacent focusable tab, if one exists.
        losingFocusRevoker = container.LosingFocus(winrt::auto_revoke, [&](const winrt::IInspectable&, const winrt::LosingFocusEventArgs& args)
            {
                if (!args.Cancel() && !args.Handled())
                {
                    int focusedIndex = IndexFromContainer(container);
                    winrt::DependencyObject newFocusedElement{ nullptr };

                    for (int i = focusedIndex + 1; i < GetItemCount(); i++)
                    {
                        auto candidateElement = ContainerFromIndex(i);

                        if (IsFocusable(candidateElement))
                        {
                            newFocusedElement = candidateElement;
                            break;
                        }
                    }

                    if (!newFocusedElement)
                    {
                        for (int i = focusedIndex - 1; i >= 0; i--)
                        {
                            auto candidateElement = ContainerFromIndex(i);

                            if (IsFocusable(candidateElement))
                            {
                                newFocusedElement = candidateElement;
                                break;
                            }
                        }
                    }

                    if (!newFocusedElement)
                    {
                        newFocusedElement = m_addButton.get();
                    }

                    args.Handled(args.TrySetNewFocusedElement(newFocusedElement));
                }
            });
    }

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
    if (m_previousAvailableSize.Width != availableSize.Width)
    {
        m_previousAvailableSize = availableSize;
        UpdateTabWidths();
    }

    return __super::MeasureOverride(availableSize);
}

void TabView::UpdateTabWidths(bool shouldUpdateWidths, bool fillAllAvailableSpace)
{
    auto const maxTabWidth = unbox_value<double>(SharedHelpers::FindInApplicationResources(c_tabViewItemMaxWidthName, box_value(c_tabMaximumWidth)));
    double tabWidth = std::numeric_limits<double>::quiet_NaN();
    uint32_t tabCount = TabItems().Size();

    // If an item is being dragged over this TabView, then we'll want to act like there's an extra item
    // when updating tab widths, which will create a hole into which the item can be dragged.
    if (m_isItemDraggedOver)
    {
        tabCount++;
    }

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
            const auto availableWidth = m_previousAvailableSize.Width - widthTaken;

            // Size can be 0 when window is first created; in that case, skip calculations; we'll get a new size soon
            if (availableWidth > 0)
            {
                if (TabWidthMode() == winrt::TabViewWidthMode::Equal)
                {
                    auto const minTabWidth = unbox_value<double>(SharedHelpers::FindInApplicationResources(c_tabViewItemMinWidthName, box_value(c_tabMinimumWidth)));

                    // If we should fill all of the available space, use scrollviewer dimensions
                    auto const padding = Padding();

                    double headerWidth = 0.0;
                    double footerWidth = 0.0;
                    if (auto&& itemsPresenter = m_itemsPresenter.get())
                    {
                        if (auto const header = itemsPresenter.Header().try_as<winrt::FrameworkElement>())
                        {
                            headerWidth = header.ActualWidth();
                        }
                        if (auto const footer = itemsPresenter.Footer().try_as<winrt::FrameworkElement>())
                        {
                            footerWidth = footer.ActualWidth();
                        }
                    }

                    if (fillAllAvailableSpace)
                    {
                        // Calculate the proportional width of each tab given the width of the ScrollViewer.
                        auto const tabWidthForScroller = (availableWidth - (padding.Left + padding.Right + headerWidth + footerWidth)) / (double)(tabCount);
                        tabWidth = std::clamp(tabWidthForScroller, minTabWidth, maxTabWidth);
                    }
                    else
                    {
                        double availableTabViewSpace = (tabColumn.ActualWidth() - (padding.Left + padding.Right + headerWidth + footerWidth));
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
                        auto const tabWidthUnclamped = availableTabViewSpace / (double)(tabCount);
                        tabWidth = std::clamp(tabWidthUnclamped, minTabWidth, maxTabWidth);
                    }


                    // Size tab column to needed size
                    tabColumn.MaxWidth(availableWidth + headerWidth + footerWidth);
                    auto requiredWidth = tabWidth * tabCount + headerWidth + footerWidth + padding.Left + padding.Right;
                    if (requiredWidth > availableWidth)
                    {
                        tabColumn.Width(winrt::GridLengthHelper::FromPixels(availableWidth));
                        if (auto&& listview = m_listView.get())
                        {
                            winrt::FxScrollViewer::SetHorizontalScrollBarVisibility(listview, winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility::Visible);
                            UpdateScrollViewerDecreaseAndIncreaseButtonsViewState();
                        }
                    }
                    else
                    {
                        // If we're dragging over the TabView, we need to set the width to a specific value,
                        // since we want it to be larger than the items actually in it in order to accommodate
                        // the item being dragged into the TabView.  Otherwise, we can just set its width to Auto.
                        tabColumn.Width(
                            m_isItemDraggedOver ?
                            winrt::GridLengthHelper::FromPixels(requiredWidth) :
                            winrt::GridLengthHelper::FromValueAndType(1.0, winrt::GridUnitType::Auto));

                        if (auto&& listview = m_listView.get())
                        {
                            if (shouldUpdateWidths && fillAllAvailableSpace)
                            {
                                winrt::FxScrollViewer::SetHorizontalScrollBarVisibility(listview, winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility::Hidden);
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
                    // Case: TabWidthMode "Compact" or "SizeToContent"
                    tabColumn.MaxWidth(availableWidth);

                    if (auto&& listview = m_listView.get())
                    {
                        // When an item is being dragged over, we need to reserve extra space for the potential new tab,
                        // so we can't rely on auto sizing in that case.  However, the ListView expands to the size of the column,
                        // so we need to store the value lest we keep expanding the width of the column every time we call this method.
                        if (m_isItemDraggedOver)
                        {
                            if (!m_expandedWidthForDragOver.has_value())
                            {
                                m_expandedWidthForDragOver = listview.ActualWidth() + maxTabWidth;
                            }

                            tabColumn.Width(winrt::GridLengthHelper::FromPixels(m_expandedWidthForDragOver.value()));
                        }
                        else
                        {
                            if (m_expandedWidthForDragOver.has_value())
                            {
                                m_expandedWidthForDragOver.reset();
                            }

                            tabColumn.Width(winrt::GridLengthHelper::FromValueAndType(1.0, winrt::GridUnitType::Auto));
                        }

                        listview.MaxWidth(availableWidth);

                        // Calculate if the scroll buttons should be visible.
                        if (auto&& itemsPresenter = m_itemsPresenter.get())
                        {
                            const auto visible = itemsPresenter.ActualWidth() > availableWidth;
                            winrt::FxScrollViewer::SetHorizontalScrollBarVisibility(listview, visible
                                ? winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility::Visible
                                : winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility::Hidden);
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
    if (auto itemsSource = TabItemsSource())
    {
        if (auto vector = itemsSource.try_as<winrt::IVector<winrt::IInspectable>>())
        {
            return static_cast<int>(vector.Size());
        }
        else if (auto iterable = itemsSource.try_as<winrt::IIterable<winrt::IInspectable>>())
        {
            int i = 0;

            for (auto const& o : iterable)
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

bool TabView::MoveFocus(bool moveForward)
{
    auto focusedControl = winrt::FocusManager::GetFocusedElement() ? winrt::FocusManager::GetFocusedElement().try_as<winrt::Control>() : nullptr;

    // If there's no focused control, then we have nothing to do.
    if (!focusedControl)
    {
        return false;
    }

    // Focus goes in this order:
    //
    //    Tab 1 -> Tab 1 close button -> Tab 2 -> Tab 2 close button -> ... -> Tab N -> Tab N close button -> Add tab button -> Tab 1
    //
    // Any element that's not focusable is skipped.
    //
    std::vector<winrt::Control> focusOrderList;

    for (int i = 0; i < GetItemCount(); i++)
    {
        if (auto tab = ContainerFromIndex(i).try_as<winrt::TabViewItem>())
        {
            if (IsFocusable(tab, false /* checkTabStop */))
            {
                focusOrderList.push_back(tab);

                if (auto closeButton = winrt::get_self<TabViewItem>(tab)->GetCloseButton())
                {
                    if (IsFocusable(closeButton, false /* checkTabStop */))
                    {
                        focusOrderList.push_back(closeButton);
                    }
                }
            }
        }
    }

    if (auto&& addButton = m_addButton.get())
    {
        if (IsFocusable(addButton, false /* checkTabStop */))
        {
            focusOrderList.push_back(addButton);
        }
    }

    auto position = std::find(focusOrderList.begin(), focusOrderList.end(), focusedControl);

    // The focused control is not in the focus order list - nothing for us to do here either.
    if (position == focusOrderList.end())
    {
        return false;
    }

    // At this point, we know that the focused control is indeed in the focus list, so we'll move focus to the next or previous control in the list.

    const int sourceIndex = static_cast<int>(position - focusOrderList.begin());
    const int listSize = static_cast<int>(focusOrderList.size());
    const int increment = moveForward ? 1 : -1;
    int nextIndex = sourceIndex + increment;

    if (nextIndex < 0)
    {
        nextIndex = listSize - 1;
    }
    else if (nextIndex >= listSize)
    {
        nextIndex = 0;
    }

    // We have to do a bit of a dance for the close buttons - we don't want users to be able to give them focus when tabbing through an app,
    // since we only want to tab into the TabView once and then tab out on the next tab press.  However, IsTabStop also controls keyboard
    // focusability in general - we can't give keyboard focus to a control with IsTabStop = false.  To work around this, we'll temporarily set
    // IsTabStop = true before calling Focus(), and then set it back to false if it was previously false.

    auto&& control = focusOrderList[nextIndex];
    const bool originalIsTabStop = control.IsTabStop();

    auto scopeGuard = gsl::finally([control, originalIsTabStop]()
        {
            control.IsTabStop(originalIsTabStop);
        });

    control.IsTabStop(true);

    // We checked focusability above, so we should never be in a situation where Focus() returns false.
    MUX_ASSERT(control.Focus(winrt::FocusState::Keyboard));
    return true;
}

bool TabView::MoveSelection(bool moveForward)
{
    const int originalIndex = SelectedIndex();
    const int increment = moveForward ? 1 : -1;
    int currentIndex = originalIndex + increment;
    const int itemCount = GetItemCount();

    while (currentIndex != originalIndex)
    {
        if (currentIndex < 0)
        {
            currentIndex = static_cast<int>(itemCount - 1);
        }
        else if (currentIndex >= itemCount)
        {
            currentIndex = 0;
        }

        if (IsFocusable(ContainerFromIndex(currentIndex)))
        {
            SelectedIndex(currentIndex);
            return true;
        }

        currentIndex += increment;
    }

    return false;
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

void TabView::OnCtrlF4Invoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args)
{
    args.Handled(RequestCloseCurrentTab());
}

void TabView::OnCtrlTabInvoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args)
{
    args.Handled(MoveSelection(true /* moveForward */));
}

void TabView::OnCtrlShiftTabInvoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args)
{
    args.Handled(MoveSelection(false /* moveForward */));
}

void TabView::OnAddButtonKeyDown(const winrt::IInspectable& sender, winrt::KeyRoutedEventArgs const& args)
{
    if (auto&& addButton = m_addButton.get())
    {
        if (args.Key() == winrt::VirtualKey::Right)
        {
            args.Handled(MoveFocus(addButton.FlowDirection() == winrt::FlowDirection::LeftToRight));
        }
        else if (args.Key() == winrt::VirtualKey::Left)
        {
            args.Handled(MoveFocus(addButton.FlowDirection() != winrt::FlowDirection::LeftToRight));
        }
    }
}

// Note that the parameter is a DependencyObject for convenience to allow us to call this on the return value of ContainerFromIndex.
// There are some non-control elements that can take focus - e.g. a hyperlink in a RichTextBlock - but those aren't relevant for our purposes here.
bool TabView::IsFocusable(winrt::DependencyObject const& object, bool checkTabStop)
{
    if (!object)
    {
        return false;
    }

    if (auto control = object.try_as<winrt::Control>())
    {
        return control &&
            control.Visibility() == winrt::Visibility::Visible &&
            (control.IsEnabled() || control.AllowFocusWhenDisabled()) &&
            (control.IsTabStop() || !checkTabStop);
    }
    else
    {
        return false;
    }
}

void TabView::UpdateIsItemDraggedOver(bool isItemDraggedOver)
{
    if (m_isItemDraggedOver != isItemDraggedOver)
    {
        m_isItemDraggedOver = isItemDraggedOver;
        UpdateTabWidths();
    }
}

void TabView::UpdateTabViewWithTearOutList()
{
    auto tabViewWithTearOutList = GetTabViewWithTearOutList();

    winrt::weak_ref<winrt::TabView> thisAsWeak = *this;
    auto existingIterator = std::find(tabViewWithTearOutList->begin(), tabViewWithTearOutList->end(), thisAsWeak);

    if (CanTearOutTabs() && IsLoaded() && existingIterator == std::end((*tabViewWithTearOutList)))
    {
        tabViewWithTearOutList->push_back(*this);
    }
    else if ((!CanTearOutTabs() || !IsLoaded()) && existingIterator != std::end((*tabViewWithTearOutList)))
    {
        tabViewWithTearOutList->erase(existingIterator);
    }
}

void TabView::AttachMoveSizeLoopEvents()
{
    if (CanTearOutTabs())
    {
        if (IsLoaded() && m_enteringMoveSizeToken.value == 0)
        {
            auto& nonClientPointerSource = GetInputNonClientPointerSource();

            m_enteringMoveSizeToken = nonClientPointerSource.EnteringMoveSize({ this, &TabView::OnEnteringMoveSize });
            m_enteredMoveSizeToken = nonClientPointerSource.EnteredMoveSize({ this, &TabView::OnEnteredMoveSize });
            m_windowRectChangingToken = nonClientPointerSource.WindowRectChanging({ this, &TabView::OnWindowRectChanging });
            m_exitedMoveSizeToken = nonClientPointerSource.ExitedMoveSize({ this, &TabView::OnExitedMoveSize });
        }
    }
    else if (m_inputNonClientPointerSource)
    {
        m_inputNonClientPointerSource.EnteringMoveSize(m_enteringMoveSizeToken);
        m_inputNonClientPointerSource.EnteredMoveSize(m_enteredMoveSizeToken);
        m_inputNonClientPointerSource.WindowRectChanging(m_windowRectChangingToken);
        m_inputNonClientPointerSource.ExitedMoveSize(m_exitedMoveSizeToken);

        m_enteringMoveSizeToken.value = 0;
        m_enteredMoveSizeToken.value = 0;
        m_windowRectChangingToken.value = 0;
        m_exitedMoveSizeToken.value = 0;
    }
}

//
// We initialize the tab tear-out state machine when we enter the move-size loop. The state machine has two states it can be in:
// either we're dragging a tab within a tab view, or we're dragging a tab that has been torn out of a tab view.
//
// If we start dragging a tab in a tab view with multiple tabs, then we'll start in the former state.  We'll raise the TabTearOutWindowRequested event,
// which prompts the app to create a new window to host the tab's data object.
// 
// If we start dragging a tab in a tab view where that is its only tab, then we'll start in the latter state.  We will *not* raise the TabTearOutWindowRequested event,
// because in this case, the window being dragged is the one that owns the tab view with a single tab.
//
// We update the state machine in the WindowRectChanging event.  See that method for a description of the state machine's functionality.
//

void TabView::OnEnteringMoveSize(const winrt::InputNonClientPointerSource& sender, const winrt::EnteringMoveSizeEventArgs& args)
{
    // We only perform tab tear-out when a move is being performed.
    if (args.MoveSizeOperation() != winrt::MoveSizeOperation::Move)
    {
        return;
    }

    auto pointInIslandCoords = XamlRoot().CoordinateConverter().ConvertScreenToLocal(args.PointerScreenPoint());
    auto tab = GetTabAtPoint(pointInIslandCoords);

    // If there was no tab at the point where the user clicked and dragged, then we have nothing to do.
    if (!tab)
    {
        return;
    }

    auto dataItem = ItemFromContainer(tab);

    m_isInTabTearOutLoop = true;
    m_tabBeingDragged = tab;
    m_dataItemBeingDragged = dataItem;
    m_tabViewContainingTabBeingDragged = *this;
    m_originalTabBeingDraggedPoint = m_tabBeingDragged.TransformToVisual(nullptr).TransformPoint({ 0, 0 });

    SelectedItem(m_dataItemBeingDragged);

    // We don't want to create a new window for tearing out if every tab is being torn out -
    // in that case, we just want to drag the window.
    if (GetItemCount() > 1)
    {
        auto windowRequestedArgs = winrt::make<TabViewTabTearOutWindowRequestedEventArgs>(dataItem, tab);
        m_tabTearOutWindowRequestedEventSource(*this, windowRequestedArgs);

        args.MoveSizeWindowId(windowRequestedArgs.NewWindowId());

        HWND newWindow = winrt::Microsoft::UI::GetWindowFromWindowId(windowRequestedArgs.NewWindowId());
        m_tabTearOutNewAppWindow = winrt::AppWindow::GetFromWindowId(windowRequestedArgs.NewWindowId());
        HWND currentWindow = winrt::Microsoft::UI::GetWindowFromWindowId(XamlRoot().ContentIslandEnvironment().AppWindowId());

        WINDOWPLACEMENT wp{};
        wp.length = sizeof(wp);
        GetWindowPlacement(currentWindow, &wp);

        // We'll position the new window to be hidden at the same position as the current window and with the restored size of the current window.
        RECT windowRect;
        GetWindowRect(currentWindow, &windowRect);
        SetWindowPos(
            newWindow,
            HWND_TOP,
            windowRect.left,
            windowRect.top,
            wp.rcNormalPosition.right - wp.rcNormalPosition.left,
            wp.rcNormalPosition.bottom - wp.rcNormalPosition.top,
            SWP_HIDEWINDOW);
    }
    else
    {
        m_tabTearOutNewAppWindow = winrt::AppWindow::GetFromWindowId(XamlRoot().ContentIslandEnvironment().AppWindowId());
    }
}

void TabView::OnEnteredMoveSize(const winrt::InputNonClientPointerSource& sender, const winrt::EnteredMoveSizeEventArgs& args)
{
    if (!m_isInTabTearOutLoop)
    {
        return;
    }

    MUX_ASSERT(CanTearOutTabs() && m_tabTearOutDraggingState == TabTearOutDraggingState::Idle);
    m_tabTearOutDraggingState = GetItemCount() > 1 ? TabTearOutDraggingState::DraggingTabWithinTabView : TabTearOutDraggingState::DraggingTornOutTab;
    m_tabTearOutInitialPosition = args.PointerScreenPoint();
    m_dragPositionOffset = {};

    // If we're starting in the state of dragging a torn out tab, let's populate the list of tab views and their bounds now.
    if (m_tabTearOutDraggingState == TabTearOutDraggingState::DraggingTornOutTab)
    {
        PopulateTabViewList();
    }
}

//
// The tab tear-out state machine proceeds as follows.
// 
// When dragging a tab within a tab view:
//   - If the tab is still within the bounds of the tab view, then we'll update its position in the item list based on where the user has dragged it -
//     e.g., if the user has dragged it more than 1/2 of the way across the width of the tab to the right, then we'll swap the positions of those two tabs
//     to keep the dragged tab underneath the user's pointer.
//   - If the tab is no longer within the bounds of the tab view, then we'll transition to the torn-out tab state.  We'll raise the TabTearOutRequested event,
//     which prompts the app to remove the tab's data object from the item list of the tab view it's being torn out from.  We'll then show the window created
//     in response to TabTearOutWindowRequested, which will now display the data object that has been torn out.
//
// When dragging a torn-out tab:
//   - If the tab is not over a tab view with CanTearOutTabs set to true, then we won't do anything, which will allow the window to be dragged as normal.
//   - If the tab is over a tab view with CanTearOutTabs set to true, then we'll raise the ExternalTornOutTabsDropping event, which allows the app
//     to decide whether it wants to allow the tab to be dropped into the tab view.  If it does, then we'll raise the ExternalTornOutTabsDropped event,
//     which prompts the app to move the tab's data object to the item list of the tab view in question, then hide the window being dragged,
//     and finally transition to the dragging within tab view state.
//
// The tab tear-out state concludes when the user releases the pointer.
//

void TabView::OnWindowRectChanging(const winrt::InputNonClientPointerSource& sender, const winrt::WindowRectChangingEventArgs& args)
{
    if (!m_isInTabTearOutLoop)
    {
        return;
    }

    switch (m_tabTearOutDraggingState)
    {
    case TabTearOutDraggingState::DraggingTabWithinTabView:
        DragTabWithinTabView(args);
        break;
    case TabTearOutDraggingState::DraggingTornOutTab:
        DragTornOutTab(args);
        break;
    }

    auto newWindowRect = args.NewWindowRect();
    newWindowRect.X -= static_cast<int32_t>(m_dragPositionOffset.X);
    newWindowRect.X -= static_cast<int32_t>(m_dragPositionOffset.Y);
    args.NewWindowRect(newWindowRect);
}

void TabView::DragTabWithinTabView(const winrt::WindowRectChangingEventArgs& args)
{
    auto pointInIslandCoords = m_tabViewContainingTabBeingDragged.XamlRoot().CoordinateConverter().ConvertScreenToLocal(args.PointerScreenPoint());
    auto tabBeingDragged = m_tabViewContainingTabBeingDragged.ContainerFromItem(m_dataItemBeingDragged).try_as<winrt::TabViewItem>();

    if (tabBeingDragged)
    {
        // We'll retrieve the bounds of the tab view in which we're dragging the tab, in order to be able to tell whether the tab has been dragged out of it.
        auto bounds = m_tabViewContainingTabBeingDragged.TransformToVisual(nullptr).TransformBounds({ 0, 0, static_cast<float>(m_tabViewContainingTabBeingDragged.ActualWidth()), static_cast<float>(m_tabViewContainingTabBeingDragged.ActualHeight()) });

        // We'll add a one-pixel margin to the bounds, since otherwise we could run into the situation where we immediately reattach after dragging out of a tab view,
        // depending on how sub-pixel rounding works out.
        bounds.X -= 1;
        bounds.Y -= 1;
        bounds.Width += 2;
        bounds.Height += 2;

        if (SharedHelpers::DoesRectContainPoint(bounds, pointInIslandCoords))
        {
            // If the tab view bounds contain the pointer point, then we'll update the index of the tab being dragged within its tab view.
            UpdateTabIndex(tabBeingDragged, pointInIslandCoords);
        }
        else
        {
            // Otherwise, we'll tear out the tab and show the window created to host the torn-out tab.
            TearOutTab(tabBeingDragged, pointInIslandCoords);
        }
    }
}

void TabView::UpdateTabIndex(winrt::TabViewItem const& tabBeingDragged, winrt::Point const& pointerPosition)
{
    auto tabViewImpl = winrt::get_self<TabView>(m_tabViewContainingTabBeingDragged);

    // We'll first figure out what tab is located at the position in question.  This may return null if, for example,
    // the user has dragged over the add-tab button, in which case we'll just early-out.
    if (auto tabAtPoint = tabViewImpl->GetTabAtPoint(pointerPosition))
    {
        // Now we'll retrieve the data item associated with that tab.  If it's the data item of the tab we're dragging,
        // then we know that the tab doesn't need to move - the pointer is still over the tab in question.
        // If it's *not* the data item of the tab we're dragging, then we'll swap the tab the pointer is over
        // with the tab we're dragging.
        auto dataItemAtPoint = tabViewImpl->ItemFromContainer(tabAtPoint);

        if (dataItemAtPoint != m_dataItemBeingDragged)
        {
            int newIndex = tabViewImpl->IndexFromContainer(tabAtPoint);

            // If this tab view has an items source set, we'll swap the items in the items source.
            // Otherwise, we'll swap the tab items themselves.
            if (auto tabItemsSource = m_tabViewContainingTabBeingDragged.TabItemsSource())
            {
                if (auto tabItemsSourceVector = tabItemsSource.try_as<winrt::IVector<IInspectable>>())
                {
                    tabItemsSourceVector.RemoveAt(tabViewImpl->IndexFromContainer(tabBeingDragged));
                    tabItemsSourceVector.InsertAt(newIndex, m_dataItemBeingDragged);
                }
            }
            else
            {
                m_tabViewContainingTabBeingDragged.TabItems().RemoveAt(tabViewImpl->IndexFromContainer(tabBeingDragged));
                m_tabViewContainingTabBeingDragged.TabItems().InsertAt(newIndex, tabBeingDragged);
            }

            // Finally, we'll re-select the tab being dragged, since it has changed positions.
            m_tabViewContainingTabBeingDragged.SelectedIndex(newIndex);
        }
    }
}

void TabView::TearOutTab(winrt::TabViewItem const& tabBeingDragged, winrt::Point const& pointerPosition)
{
    auto tabViewImpl = winrt::get_self<TabView>(m_tabViewContainingTabBeingDragged);

    // We'll first raise the TabTearOutRequested event, which prompts the app to move the torn-out tab data item from its current tab view to the one in the new window.
    tabViewImpl->m_tabTearOutRequestedEventSource(m_tabViewContainingTabBeingDragged, winrt::make<TabViewTabTearOutRequestedEventArgs>(m_dataItemBeingDragged, tabBeingDragged));

    // We're now dragging a torn out tab, so let's populate the list of tab views.
    m_tabTearOutDraggingState = TabTearOutDraggingState::DraggingTornOutTab;
    PopulateTabViewList();

    // Now we'll show the window.
    m_tabTearOutNewAppWindow.Show();

    if (m_tabViewContainingTabBeingDragged)
    {
        m_tabViewContainingTabBeingDragged.UpdateLayout();

        // We want to keep the tab under the user's pointer, so we'll subtract off the difference from the XAML position of the tab in the original window,
        // in order to ensure we position the window such that the tab in the new window is in the same position as the tab in the old window.
        auto containingTabPosition = m_tabViewContainingTabBeingDragged.ContainerFromIndex(m_tabViewContainingTabBeingDragged.SelectedIndex()).as<winrt::TabViewItem>().TransformToVisual(nullptr).TransformPoint({ 0, 0 });
        m_dragPositionOffset = { containingTabPosition.X - m_originalTabBeingDraggedPoint.X , containingTabPosition.Y - m_originalTabBeingDraggedPoint.Y };
    }
    else
    {
        m_dragPositionOffset = {};
    }
}

void TabView::DragTornOutTab(const winrt::WindowRectChangingEventArgs& args)
{
    // When we're dragging a torn-out tab, we want to check, as the window moves, whether the user has dragged the tab over a tab view that will allow it to be dropped into it.
    // We'll iterate through the list of tab views and their bounds and check each of their screen positions against the screen position of the pointer.
    for (auto iterator = m_tabViewBoundsTuples.begin(); iterator != m_tabViewBoundsTuples.end(); iterator++)
    {
        auto& otherTabViewScreenBounds = std::get<winrt::RectInt32>(*iterator);

        if (SharedHelpers::DoesRectContainPoint(otherTabViewScreenBounds, args.PointerScreenPoint()))
        {
            auto& otherTabView = std::get<winrt::TabView>(*iterator);

            // We'll check which index we need to insert the tab at.
            int insertionIndex = GetTabInsertionIndex(otherTabView, args.PointerScreenPoint());

            // If we got a valid index, we'll begin attempting to merge the tab into this tab view.
            if (insertionIndex >= 0)
            {
                // First, we'll raise the ExternalTornOutTabsDropping event, which asks the app whether this tab view should accept the tab being dropped into it.
                auto tabsDroppingArgs = winrt::make<TabViewExternalTornOutTabsDroppingEventArgs>(m_dataItemBeingDragged, m_tabBeingDragged, insertionIndex);

                auto otherTabViewImpl = winrt::get_self<TabView>(otherTabView);
                otherTabViewImpl->m_externalTornOutTabsDroppingEventSource(otherTabView, tabsDroppingArgs);

                // If the response was yes, then we'll raise the ExternalTornOutTabsDropped event, which prompts the app to actually move the tab's data item
                // to the new tab view; we'll flag the new tab view as the one containing the tab we're dragging; we'll move to the dragging tab within a tab view state;
                // and finally we'll then hide the torn-out tab window.
                if (tabsDroppingArgs.AllowDrop())
                {
                    // We're about to merge the tab into the other tab view, so we'll retrieve and save off the tab view that currently holds the tab being dragged.
                    // We'll need to remove it from the list of tab views with CanTearOutTabs set to true if its window is destroyed.
                    m_tabViewInNewAppWindow = m_tabViewContainingTabBeingDragged;

                    otherTabViewImpl->m_externalTornOutTabsDroppedEventSource(otherTabView, winrt::make<TabViewExternalTornOutTabsDroppedEventArgs>(m_dataItemBeingDragged, m_tabBeingDragged, insertionIndex));
                    otherTabView.SelectedItem(m_dataItemBeingDragged);

                    // If the other tab view's app window is a different app window than the one being dragged, bring it to the front beneath the one being dragged.
                    auto otherTabViewAppWindow = winrt::AppWindow::GetFromWindowId(otherTabView.XamlRoot().ContentIslandEnvironment().AppWindowId());
                    if (otherTabViewAppWindow.Id() != m_tabTearOutNewAppWindow.Id())
                    {
                        otherTabViewAppWindow.MoveInZOrderBelow(m_tabTearOutNewAppWindow.Id());
                    }

                    m_tabViewContainingTabBeingDragged = otherTabView;
                    m_dragPositionOffset = {};
                    m_tabTearOutDraggingState = TabTearOutDraggingState::DraggingTabWithinTabView;
                    m_tabTearOutNewAppWindow.Hide();

                    break;
                }
            }
        }
    }
}

int TabView::GetTabInsertionIndex(winrt::TabView const& otherTabView, winrt::PointInt32 const& screenPosition)
{
    int index = -1;

    // To get the insertion index, we'll first check what tab (if any) is beneath the screen position.
    auto otherTabViewImpl = winrt::get_self<TabView>(otherTabView);
    auto tab = otherTabViewImpl->GetTabAtPoint(otherTabView.XamlRoot().CoordinateConverter().ConvertScreenToLocal(screenPosition));

    if (tab)
    {
        // If there was a tab underneath the position, then we'll check whether the screen position is on its left side or its right side.
        // If it's on the left side, we'll set the insertion position to be before this tab. Otherwise, we'll set it to be after this tab.
        auto tabIndex = otherTabViewImpl->IndexFromContainer(tab);
        auto tabRect = otherTabView.XamlRoot().CoordinateConverter().ConvertLocalToScreen(tab.TransformToVisual(nullptr).TransformBounds(winrt::Rect{ 0, 0, static_cast<float>(tab.ActualWidth()), static_cast<float>(tab.ActualHeight()) }));

        if (screenPosition.X < tabRect.X + tabRect.Width / 2)
        {
            index = tabIndex;
        }
        else
        {
            index = tabIndex + 1;
        }
    }
    else if (otherTabViewImpl->GetItemCount() > 0)
    {
        // If there was no tab, under the cursor, then that suggests we want to insert the tab either at the very beginning or at the very end.
        // We'll first check whether the screen position is to the left of the bounds of the first tab.  If so, we'll set the insertion position
        // to be the start of the item list.
        auto firstTab = otherTabViewImpl->ContainerFromIndex(0).as<winrt::TabViewItem>();

        if (firstTab)
        {
            auto firstTabRect = otherTabView.XamlRoot().CoordinateConverter().ConvertLocalToScreen(firstTab.TransformToVisual(nullptr).TransformBounds(winrt::Rect{ 0, 0, static_cast<float>(firstTab.ActualWidth()), static_cast<float>(firstTab.ActualHeight()) }));

            if (screenPosition.X < firstTabRect.X)
            {
                index = 0;
            }
        }

        // If that wasn't the case, then next we'll check whether the screen position is to the right of the bounds of the last tab.
        // If so, we'll set the insertion position to be the end of the item list.
        if (index < 0)
        {
            auto lastTabIndex = otherTabViewImpl->GetItemCount() - 1;
            auto lastTab = otherTabViewImpl->ContainerFromIndex(lastTabIndex).as<winrt::TabViewItem>();

            if (lastTab)
            {
                auto lastTabRect = otherTabView.XamlRoot().CoordinateConverter().ConvertLocalToScreen(lastTab.TransformToVisual(nullptr).TransformBounds(winrt::Rect{ 0, 0, static_cast<float>(lastTab.ActualWidth()), static_cast<float>(lastTab.ActualHeight()) }));
                if (screenPosition.X > lastTabRect.X + lastTabRect.Width)
                {
                    index = otherTabViewImpl->GetItemCount();
                }
            }
        }
    }

    return index;
}

// When we exit the move-size loop, we'll reset the tab tear-out state machine to an idle state, and check the status of the window that was created.
// If the window is currently hidden, then the user has merged the torn out tab with another tab view, and the window is no longer needed.
// In that case, we'll queue the window for destruction.

void TabView::OnExitedMoveSize(const winrt::InputNonClientPointerSource& sender, const winrt::ExitedMoveSizeEventArgs& args)
{
    if (!m_isInTabTearOutLoop)
    {
        return;
    }

    m_tabTearOutDraggingState = TabTearOutDraggingState::Idle;

    if (!m_tabTearOutNewAppWindow.IsVisible())
    {
        // We're about to close the window containing the tab view that had been holding the tab view,
        // so we'll remove it from the list of tab views with CanTearOutTabs set to true
        // This will ensure that it's immediately removed from the list rather than waiting for the
        // WM_CLOSE message to be handled.
        if (m_tabViewInNewAppWindow)
        {
            auto tabViewWithTearOutList = GetTabViewWithTearOutList();
            winrt::weak_ref<winrt::TabView> windowTabViewAsWeak = m_tabViewInNewAppWindow;
            auto existingIterator = std::find(tabViewWithTearOutList->begin(), tabViewWithTearOutList->end(), windowTabViewAsWeak);

            if (existingIterator != std::end((*tabViewWithTearOutList)))
            {
                tabViewWithTearOutList->erase(existingIterator);
            }
        }

        PostMessageW(winrt::Microsoft::UI::GetWindowFromWindowId(m_tabTearOutNewAppWindow.Id()), WM_CLOSE, 0, 0);
    }
    else if (m_tabViewContainingTabBeingDragged)
    {
        // Otherwise, if the window is still open, let's update its tab view's non-client region.
        winrt::get_self<TabView>(m_tabViewContainingTabBeingDragged)->UpdateNonClientRegion();
    }

    // We'll also update this tab view's non-client region, now that it's stabilized.
    UpdateNonClientRegion();
    m_isInTabTearOutLoop = false;
}

winrt::TabViewItem TabView::GetTabAtPoint(const winrt::Point& point)
{
    // Convert the point to a point in the TabView's coordinate space
    // and then detect which TabViewItem is at that point.
    auto tabViewRect = TransformToVisual(nullptr).TransformBounds(winrt::Rect{ 0, 0, static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight()) });

    if (SharedHelpers::DoesRectContainPoint(tabViewRect, point))
    {
        auto tabCount = GetItemCount();
        for (int i = 0; i < tabCount; i++)
        {
            auto tab = ContainerFromIndex(i).try_as<winrt::TabViewItem>();
            if (tab)
            {
                auto tabRect = tab.TransformToVisual(nullptr).TransformBounds(winrt::Rect{ 0, 0, static_cast<float>(tab.ActualWidth()), static_cast<float>(tab.ActualHeight()) });
                if (SharedHelpers::DoesRectContainPoint(tabRect, point))
                {
                    return tab;
                }
            }
        }
    }

    return nullptr;
}

void TabView::PopulateTabViewList()
{
    // When we're dragging a torn-out tab, we want to check, as the window moves, whether the user has dragged the tab over a tab view that will allow it to be dropped into it.
    // We'll pre-fill a list of tab views and their screen bounds when we start dragging a torn-out tab, on the basis that they are unlikely to move while we're dragging the tab.
    m_tabViewBoundsTuples.clear();

    // We'll also track which tab view holds the torn-out tab.
    m_tabViewContainingTabBeingDragged = nullptr;

    auto tabViewWithTearOutList = TabView::GetTabViewWithTearOutList();

    auto iterator = tabViewWithTearOutList->begin();
    while (iterator != tabViewWithTearOutList->end())
    {
        auto otherTabView = (*iterator).get();

        if (!otherTabView)
        {
            iterator = tabViewWithTearOutList->erase(iterator);
        }
        else
        {
            // We only want to populate the tuple list with tab views that don't currently contain the item being dragged,
            // since this tuple list is used to detect tab views that the item being dragged can be dragged onto.
            bool otherTabViewContainsTab = false;
            uint32_t ignored;

            if (auto otherTabItemsSource = otherTabView.TabItemsSource())
            {
                if (auto tabItemsSourceVector = otherTabItemsSource.try_as<winrt::IVector<IInspectable>>())
                {
                    otherTabViewContainsTab = tabItemsSourceVector.IndexOf(m_dataItemBeingDragged, ignored);
                }
            }
            else
            {
                otherTabViewContainsTab = otherTabView.TabItems().IndexOf(m_tabBeingDragged, ignored);
            }

            if (otherTabViewContainsTab)
            {
                m_tabViewContainingTabBeingDragged = otherTabView;
            }
            else
            {
                auto otherTabViewXamlBounds = otherTabView.TransformToVisual(nullptr).TransformBounds(winrt::Rect(0, 0, static_cast<float>(otherTabView.ActualWidth()), static_cast<float>(otherTabView.ActualHeight())));
                auto otherTabViewScreenBounds = otherTabView.XamlRoot().CoordinateConverter().ConvertLocalToScreen(otherTabViewXamlBounds);

                m_tabViewBoundsTuples.push_back(std::make_tuple(otherTabViewScreenBounds, otherTabView));
            }

            iterator++;
        }
    }
}

// At the moment, all TabViews and windows are on the same thread.
// However, that won't always be the case, so to handle things
// when it isn't, we'll lock the accessing of the list of TabViews
// behind a mutex and require its acquisition to interact with the list.
MutexLockedResource<std::list<winrt::weak_ref<winrt::TabView>>> TabView::GetTabViewWithTearOutList()
{
    return MutexLockedResource(s_tabWithTearOutListMutex, &s_tabViewWithTearOutList);
}

winrt::InputNonClientPointerSource const& TabView::GetInputNonClientPointerSource()
{
    auto windowId = GetAppWindowId();

    if (!m_inputNonClientPointerSource && windowId.Value != 0)
    {
        m_inputNonClientPointerSource = winrt::InputNonClientPointerSource::GetForWindowId(windowId);
    }

    return m_inputNonClientPointerSource;
}

winrt::ContentCoordinateConverter const& TabView::GetAppWindowCoordinateConverter()
{
    auto windowId = GetAppWindowId();

    if (!m_appWindowCoordinateConverter && windowId.Value != 0)
    {
        m_appWindowCoordinateConverter = winrt::ContentCoordinateConverter::CreateForWindowId(windowId);
    }

    return m_appWindowCoordinateConverter;
}

void TabView::UpdateNonClientRegion()
{
    if (auto& nonClientPointerSource = GetInputNonClientPointerSource())
    {
        auto captionRects = nonClientPointerSource.GetRegionRects(winrt::NonClientRegionKind::Caption);

        // We need to preserve non-client caption regions set by components other than us,
        // so we'll keep around all caption regions except the one that we set.
        std::vector<winrt::RectInt32> captionRegions;
        std::for_each(captionRects.cbegin(), captionRects.cend(), [&](winrt::RectInt32 const& rect)
            {
                if (!m_nonClientRegionSet || rect == m_nonClientRegion)
                {
                    captionRegions.push_back(rect);
                }
            });

        if (CanTearOutTabs() && IsLoaded())
        {
            if (auto& listView = m_listView.get())
            {
                if (listView.IsLoaded())
                {
                    if (auto& appWindowCoordinateConverter = GetAppWindowCoordinateConverter())
                    {
                        auto listViewBounds = listView.TransformToVisual(nullptr).TransformBounds(winrt::Rect(0, 0, static_cast<float>(listView.ActualWidth()), static_cast<float>(listView.ActualHeight())));

                        if (listViewBounds.X < 0 || listViewBounds.Y < 0)
                        {
                            return;
                        }

                        // Non-client region rects need to be in the coordinate system of the owning app window, so we'll take our XAML island coordinates,
                        // convert them to screen coordinates, and then convert from there to app window coordinates.
                        auto appWindowListViewBounds = appWindowCoordinateConverter.ConvertScreenToLocal(XamlRoot().CoordinateConverter().ConvertLocalToScreen(listViewBounds));

                        m_nonClientRegion = {
                            static_cast<int32_t>(appWindowListViewBounds.X),
                            static_cast<int32_t>(appWindowListViewBounds.Y),
                            static_cast<int32_t>(appWindowListViewBounds.Width),
                            static_cast<int32_t>(appWindowListViewBounds.Height),
                        };

                        m_nonClientRegionSet = true;

                        captionRegions.push_back(m_nonClientRegion);
                    }
                }
            }
        }

        nonClientPointerSource.SetRegionRects(winrt::NonClientRegionKind::Caption, captionRegions);
    }
}

winrt::WindowId TabView::GetAppWindowId()
{
    winrt::WindowId appWindowId{};

    if (auto xamlRoot = XamlRoot())
    {
        if (auto contentIslandEnvironment = xamlRoot.ContentIslandEnvironment())
        {
            appWindowId = contentIslandEnvironment.AppWindowId();
        }
    }

    if (appWindowId.Value != m_lastAppWindowId.Value)
    {
        m_lastAppWindowId = appWindowId;

        m_inputNonClientPointerSource = nullptr;
        m_appWindowCoordinateConverter = nullptr;
    }

    return appWindowId;
}
