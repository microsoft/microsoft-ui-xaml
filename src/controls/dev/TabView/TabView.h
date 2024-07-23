// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TabView.g.h"
#include "TabView.properties.h"
#include "TabViewTabCloseRequestedEventArgs.g.h"
#include "TabViewTabDroppedOutsideEventArgs.g.h"
#include "TabViewTabDragStartingEventArgs.g.h"
#include "TabViewTabDragCompletedEventArgs.g.h"
#include "TabViewTabTearOutWindowRequestedEventArgs.g.h"
#include "TabViewTabTearOutRequestedEventArgs.g.h"
#include "TabViewExternalTornOutTabsDroppingEventArgs.g.h"
#include "TabViewExternalTornOutTabsDroppedEventArgs.g.h"
#include "TabViewTrace.h"

#include <wil/resource.h>

class TabViewTabCloseRequestedEventArgs :
    public winrt::implementation::TabViewTabCloseRequestedEventArgsT<TabViewTabCloseRequestedEventArgs>
{
public:
    TabViewTabCloseRequestedEventArgs(winrt::IInspectable const& item, winrt::TabViewItem tab) : m_item(item), m_tab(tab) {}

    winrt::IInspectable Item() { return m_item; }
    winrt::TabViewItem Tab() { return m_tab; }

private:
    winrt::IInspectable m_item{};
    winrt::TabViewItem m_tab{};
};


class TabViewTabDroppedOutsideEventArgs :
    public winrt::implementation::TabViewTabDroppedOutsideEventArgsT<TabViewTabDroppedOutsideEventArgs>
{
public:
    TabViewTabDroppedOutsideEventArgs(winrt::IInspectable const& item, winrt::TabViewItem tab) : m_item(item), m_tab(tab) {}

    winrt::IInspectable Item() { return m_item; }
    winrt::TabViewItem Tab() { return m_tab; }

private:
    winrt::IInspectable m_item{};
    winrt::TabViewItem m_tab{};
};

class TabViewTabDragStartingEventArgs :
    public winrt::implementation::TabViewTabDragStartingEventArgsT<TabViewTabDragStartingEventArgs>
{
public:
    TabViewTabDragStartingEventArgs(winrt::DragItemsStartingEventArgs const& args, winrt::IInspectable const& item, winrt::TabViewItem tab) : m_args(args), m_item(item), m_tab(tab) {}

    bool Cancel() { return m_args.Cancel(); }
    void Cancel(bool value) { m_args.Cancel(value); }
    winrt::DataPackage Data() { return m_args.Data(); }
    winrt::IInspectable Item() { return m_item; }
    winrt::TabViewItem Tab() { return m_tab; }

private:
    winrt::DragItemsStartingEventArgs m_args{};
    winrt::IInspectable m_item{};
    winrt::TabViewItem m_tab{};
};

class TabViewTabDragCompletedEventArgs :
    public winrt::implementation::TabViewTabDragCompletedEventArgsT<TabViewTabDragCompletedEventArgs>
{
public:
    TabViewTabDragCompletedEventArgs(winrt::DragItemsCompletedEventArgs const& args, winrt::IInspectable const& item, winrt::TabViewItem tab) : m_args(args), m_item(item), m_tab(tab) {}

    winrt::DataPackageOperation DropResult() { return m_args.DropResult(); }
    winrt::IInspectable Item() { return m_item; }
    winrt::TabViewItem Tab() { return m_tab; }

private:
    winrt::DragItemsCompletedEventArgs m_args{ nullptr };
    winrt::IInspectable m_item{};
    winrt::TabViewItem m_tab{};
};

// We need to make a copy of our arrays in event arg types because in managed code, they're cleaned up after access.
// In other words, if we provide a pointer to the arrays stored on the args type, they will be clobbered after being accessed once.
template <class T>
winrt::com_array<T> CopyArray(winrt::com_array<T> const& a)
{
    return std::move(winrt::com_array<T>{ a.begin(), a.end()});
}

class TabViewTabTearOutWindowRequestedEventArgs :
    public winrt::implementation::TabViewTabTearOutWindowRequestedEventArgsT<TabViewTabTearOutWindowRequestedEventArgs>
{
public:
    TabViewTabTearOutWindowRequestedEventArgs(winrt::IInspectable const& item, winrt::UIElement const& tab);

    winrt::com_array<winrt::IInspectable> Items() const { return CopyArray(m_items); }
    winrt::com_array<winrt::UIElement> Tabs() const { return CopyArray(m_tabs); }

    winrt::Microsoft::UI::WindowId NewWindowId() const { return m_newWindowId; }
    void NewWindowId(winrt::Microsoft::UI::WindowId const& value) { m_newWindowId = value; }

private:
    winrt::com_array<winrt::IInspectable> m_items;
    winrt::com_array<winrt::UIElement> m_tabs;

    winrt::Microsoft::UI::WindowId m_newWindowId{};
};

class TabViewTabTearOutRequestedEventArgs :
    public winrt::implementation::TabViewTabTearOutRequestedEventArgsT<TabViewTabTearOutRequestedEventArgs>
{
public:
    TabViewTabTearOutRequestedEventArgs(winrt::IInspectable const& item, winrt::UIElement const& tab);

    winrt::com_array<winrt::IInspectable> Items() const { return CopyArray(m_items); }
    winrt::com_array<winrt::UIElement> Tabs() const { return CopyArray(m_tabs); }

    winrt::Microsoft::UI::WindowId NewWindowId() const { return m_newWindowId; }

private:
    winrt::com_array<winrt::IInspectable> m_items;
    winrt::com_array<winrt::UIElement> m_tabs;

    winrt::Microsoft::UI::WindowId m_newWindowId{};
};

class TabViewExternalTornOutTabsDroppingEventArgs :
    public winrt::implementation::TabViewExternalTornOutTabsDroppingEventArgsT<TabViewExternalTornOutTabsDroppingEventArgs>
{
public:
    TabViewExternalTornOutTabsDroppingEventArgs(winrt::IInspectable const& item, winrt::UIElement const& tab, int dropIndex);

    winrt::com_array<winrt::IInspectable> Items() const { return CopyArray(m_items); }
    winrt::com_array<winrt::UIElement> Tabs() const { return CopyArray(m_tabs); }

    int DropIndex() const { return m_dropIndex; }
    bool AllowDrop() const { return m_allowDrop; }
    void AllowDrop(bool value) { m_allowDrop = value; }

private:
    winrt::com_array<winrt::IInspectable> m_items;
    winrt::com_array<winrt::UIElement> m_tabs;

    int m_dropIndex{ -1 };
    bool m_allowDrop{ false };
};

class TabViewExternalTornOutTabsDroppedEventArgs :
    public winrt::implementation::TabViewExternalTornOutTabsDroppedEventArgsT<TabViewExternalTornOutTabsDroppedEventArgs>
{
public:
    TabViewExternalTornOutTabsDroppedEventArgs(winrt::IInspectable const& item, winrt::UIElement const& tab, int dropIndex);

    winrt::com_array<winrt::IInspectable> Items() const { return CopyArray(m_items); }
    winrt::com_array<winrt::UIElement> Tabs() const { return CopyArray(m_tabs); }

    int DropIndex() const { return m_dropIndex; }

private:
    winrt::com_array<winrt::IInspectable> m_items;
    winrt::com_array<winrt::UIElement> m_tabs;

    int m_dropIndex{ -1 };
};

template<class T>
class MutexLockedResource
{
public:
    MutexLockedResource(HANDLE mutex, T* resource)
    {
        m_mutex = mutex;
        m_resource = resource;

        ::WaitForSingleObject(m_mutex, INFINITE);
    }

    ~MutexLockedResource()
    {
        ::ReleaseMutex(m_mutex);
    }

    T& operator*()
    {
        return *m_resource;
    }

    T* operator->()
    {
        return m_resource;
    }

private:
    HANDLE m_mutex;
    T* m_resource;
};

enum class TabTearOutDraggingState
{
    Idle,
    DraggingTabWithinTabView,
    DraggingTornOutTab,
};

class TabView :
    public ReferenceTracker<TabView, winrt::implementation::TabViewT>,
    public TabViewProperties
{

public:
    TabView();
    ~TabView();

    // IFrameworkElement
    void OnApplyTemplate();
    winrt::Size MeasureOverride(winrt::Size const& availableSize);

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    // From ListView
    winrt::DependencyObject ContainerFromItem(winrt::IInspectable const& item);
    winrt::DependencyObject ContainerFromIndex(int index);
    int IndexFromContainer(winrt::DependencyObject const& container);
    winrt::IInspectable ItemFromContainer(winrt::DependencyObject const& container);

    // Internal
    void OnCloseButtonOverlayModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnTabItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnTabWidthModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnSelectedIndexPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnSelectedItemPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnCanTearOutTabsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnItemsChanged(winrt::IInspectable const& item);
    void UpdateTabContent();

    void RequestCloseTab(winrt::TabViewItem const& item,bool updateTabWidths);

    winrt::hstring GetTabCloseButtonTooltipText() { return m_tabCloseButtonTooltipText; }
    void SetTabSeparatorOpacity(int index, int opacityValue);
    void SetTabSeparatorOpacity(int index);

    bool MoveFocus(bool moveForward);

    void UpdateTabWidths(bool shouldUpdateWidths = true, bool fillAllAvailableSpace = true);

    int GetItemCount();

    void UpdateTabViewWithTearOutList();
    void AttachMoveSizeLoopEvents();

    static MutexLockedResource<std::list<winrt::weak_ref<winrt::TabView>>> GetTabViewWithTearOutList();

    winrt::InputNonClientPointerSource const& GetInputNonClientPointerSource();
    winrt::ContentCoordinateConverter const& GetAppWindowCoordinateConverter();

private:
    void OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnUnloaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnScrollViewerLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnAddButtonClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnScrollDecreaseClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnScrollIncreaseClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnScrollViewerViewChanged(winrt::IInspectable const& sender, winrt::ScrollViewerViewChangedEventArgs const& args);
    void OnItemsPresenterSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);

    void OnListViewLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnTabStripPointerExited(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnTabStripPointerEntered(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnListViewSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args);
    void OnListViewSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);

    void OnListViewDragItemsStarting(const winrt::IInspectable& sender, const winrt::DragItemsStartingEventArgs& args);
    void OnListViewDragItemsCompleted(const winrt::IInspectable& sender, const winrt::DragItemsCompletedEventArgs& args);
    void OnListViewDragOver(const winrt::IInspectable& sender, const winrt::DragEventArgs& args);
    void OnListViewDrop(const winrt::IInspectable& sender, const winrt::DragEventArgs& args);

    void OnListViewDragEnter(const winrt::IInspectable& sender, const winrt::DragEventArgs& args);
    void OnListViewDragLeave(const winrt::IInspectable& sender, const winrt::DragEventArgs& args);

    void OnCtrlF4Invoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args);
    void OnCtrlTabInvoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args);
    void OnCtrlShiftTabInvoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args);

    void OnAddButtonKeyDown(const winrt::IInspectable& sender, winrt::KeyRoutedEventArgs const& args);

    bool RequestCloseCurrentTab();
    bool MoveSelection(bool moveForward);
    void BringSelectedTabIntoView();

    void UpdateSelectedItem();
    void UpdateSelectedIndex();

    void UpdateScrollViewerDecreaseAndIncreaseButtonsViewState();
    void UpdateListViewItemContainerTransitions();

    void UnhookEventsAndClearFields();

    void OnListViewDraggingPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnListViewGettingFocus(const winrt::IInspectable& sender, const winrt::GettingFocusEventArgs& args);

    void UpdateBottomBorderLineVisualStates();
    void UpdateTabBottomBorderLineVisualStates();

    winrt::TabViewItem FindTabViewItemFromDragItem(const winrt::IInspectable& item);

    static bool IsFocusable(winrt::DependencyObject const& object, bool checkTabStop = false);

    void UpdateIsItemDraggedOver(bool isItemDraggedOver);

    void OnEnteringMoveSize(const winrt::InputNonClientPointerSource& sender, const winrt::EnteringMoveSizeEventArgs& args);
    void OnEnteredMoveSize(const winrt::InputNonClientPointerSource& sender, const winrt::EnteredMoveSizeEventArgs& args);
    void OnWindowRectChanging(const winrt::InputNonClientPointerSource& sender, const winrt::WindowRectChangingEventArgs& args);
    void OnExitedMoveSize(const winrt::InputNonClientPointerSource& sender, const winrt::ExitedMoveSizeEventArgs& args);

    winrt::TabViewItem GetTabAtPoint(const winrt::Point& point);
    void PopulateTabViewList();

    void DragTabWithinTabView(const winrt::WindowRectChangingEventArgs& args);
    void UpdateTabIndex(winrt::TabViewItem const& tabBeingDragged, winrt::Point const& pointerPosition);
    void TearOutTab(winrt::TabViewItem const& tabBeingDragged, winrt::Point const& pointerPosition);
    void DragTornOutTab(const winrt::WindowRectChangingEventArgs& args);
    int GetTabInsertionIndex(winrt::TabView const& otherTabView, winrt::PointInt32 const& screenPosition);

    void UpdateNonClientRegion();
    winrt::WindowId GetAppWindowId();

    static std::list<winrt::weak_ref<winrt::TabView>> s_tabViewWithTearOutList;
    static HANDLE s_tabWithTearOutListMutex;

    bool m_updateTabWidthOnPointerLeave{ false };
    bool m_pointerInTabstrip{ false };

    tracker_ref<winrt::ColumnDefinition> m_leftContentColumn{ this };
    tracker_ref<winrt::ColumnDefinition> m_tabColumn{ this };
    tracker_ref<winrt::ColumnDefinition> m_addButtonColumn{ this };
    tracker_ref<winrt::ColumnDefinition> m_rightContentColumn{ this };

    tracker_ref<winrt::ListView> m_listView{ this };
    tracker_ref<winrt::ContentPresenter> m_tabContentPresenter{ this };
    tracker_ref<winrt::ContentPresenter> m_rightContentPresenter{ this };
    tracker_ref<winrt::Grid> m_tabContainerGrid{ this };
    tracker_ref<winrt::FxScrollViewer> m_scrollViewer{ this };
    tracker_ref<winrt::RepeatButton> m_scrollDecreaseButton{ this };
    tracker_ref<winrt::RepeatButton> m_scrollIncreaseButton{ this };
    tracker_ref<winrt::Button> m_addButton{ this };
    tracker_ref<winrt::ItemsPresenter> m_itemsPresenter{ this };

    winrt::ListView::Loaded_revoker m_listViewLoadedRevoker{};
    winrt::ListView::PointerExited_revoker m_tabStripPointerExitedRevoker{};
    winrt::ListView::PointerEntered_revoker m_tabStripPointerEnteredRevoker{};
    winrt::Selector::SelectionChanged_revoker m_listViewSelectionChangedRevoker{};
    winrt::UIElement::GettingFocus_revoker m_listViewGettingFocusRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_listViewSizeChangedRevoker{};

    PropertyChanged_revoker m_listViewCanReorderItemsPropertyChangedRevoker{};
    PropertyChanged_revoker m_listViewAllowDropPropertyChangedRevoker{};

    winrt::ListView::DragItemsStarting_revoker m_listViewDragItemsStartingRevoker{};
    winrt::ListView::DragItemsCompleted_revoker m_listViewDragItemsCompletedRevoker{};
    winrt::UIElement::DragOver_revoker m_listViewDragOverRevoker{};

    RoutedEventHandler_revoker m_listViewDropRevoker{};
    RoutedEventHandler_revoker m_listViewDragEnterRevoker{};
    RoutedEventHandler_revoker m_listViewDragLeaveRevoker{};

    winrt::FxScrollViewer::Loaded_revoker m_scrollViewerLoadedRevoker{};
    winrt::FxScrollViewer::ViewChanged_revoker m_scrollViewerViewChangedRevoker{};

    winrt::Button::Click_revoker m_addButtonClickRevoker{};

    winrt::RepeatButton::Click_revoker m_scrollDecreaseClickRevoker{};
    winrt::RepeatButton::Click_revoker m_scrollIncreaseClickRevoker{};

    winrt::Button::KeyDown_revoker m_addButtonKeyDownRevoker{};
    
    winrt::ItemsPresenter::SizeChanged_revoker m_itemsPresenterSizeChangedRevoker{};

    winrt::hstring m_tabCloseButtonTooltipText{};

    winrt::Size m_previousAvailableSize{};

    bool m_isItemBeingDragged{ false };
    bool m_isItemDraggedOver{ false };
    std::optional<double> m_expandedWidthForDragOver{};

    winrt::event_token m_enteringMoveSizeToken{};
    winrt::event_token m_enteredMoveSizeToken{};
    winrt::event_token m_windowRectChangingToken{};
    winrt::event_token m_exitedMoveSizeToken{};

    bool m_isInTabTearOutLoop{ false };
    winrt::AppWindow m_tabTearOutNewAppWindow{ nullptr };
    winrt::IInspectable m_dataItemBeingDragged{ nullptr };
    winrt::TabViewItem m_tabBeingDragged{ nullptr };
    winrt::TabView m_tabViewContainingTabBeingDragged{ nullptr };
    winrt::TabView m_tabViewInNewAppWindow{ nullptr };
    winrt::Point m_originalTabBeingDraggedPoint{};
    winrt::Point m_dragPositionOffset{};
    TabTearOutDraggingState m_tabTearOutDraggingState{ TabTearOutDraggingState::Idle };
    winrt::PointInt32 m_tabTearOutInitialPosition{};

    winrt::RectInt32 m_nonClientRegion{};
    bool m_nonClientRegionSet{ false };

    std::vector<std::tuple<winrt::RectInt32, winrt::TabView>> m_tabViewBoundsTuples;

    winrt::WindowId m_lastAppWindowId{};
    winrt::InputNonClientPointerSource m_inputNonClientPointerSource{ nullptr };
    winrt::ContentCoordinateConverter m_appWindowCoordinateConverter{ nullptr };
};
