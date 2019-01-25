// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class NavigationViewItem;
class NavigationViewList;
struct bringintoview_event_revoker;

#include "NavigationViewTemplateSettings.h"
#include "NavigationViewItem.h"
#include "NavigationView.g.h"
#include "TopNavigationViewDataProvider.h"
#include "NavigationViewHelper.h"
#include "NavigationView.properties.h"
#include "TreeViewNode.h"
#include "ViewModel.h"

enum class TopNavigationViewLayoutState
{
    InitStep1 = 0, // Move all data to primary
    InitStep2, // Realized virtualization items
    InitStep3, // Waiting for moving data to overflow
    Normal,
    Overflow,
    OverflowNoChange // InvalidateMeasure but not move any items. It happens when we have enough information 
                     // to swap an navigationviewitem to overflow, InvalidateMeasure is only used to update
                     // SelectionIndicate. Otherwise FindSelectionIndicator may be nullptr for the overflow item
};

enum class NavigationRecommendedTransitionDirection
{
    FromOverflow, // mapping to SlideNavigationTransitionInfo FromLeft
    FromLeft, // SlideNavigationTransitionInfo
    FromRight, // SlideNavigationTransitionInfo
    Default // Currently it's mapping to EntranceNavigationTransitionInfo and is subject to change.
};

class NavigationView :
    public ReferenceTracker<NavigationView, winrt::implementation::NavigationViewT>,
    public NavigationViewProperties
{
public:
    NavigationView();
    virtual ~NavigationView();

    // IFrameworkElementOverrides
    void OnApplyTemplate();

#pragma region IFrameworkElementOverridesHelper
    // IFrameworkElementOverrides (unoverridden methods provided by FrameworkElementOverridesHelper)
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
#pragma endregion

    winrt::IInspectable MenuItemFromContainer(winrt::DependencyObject const& container);
    winrt::DependencyObject ContainerFromMenuItem(winrt::IInspectable const& item);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&  args);
    void OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

    void OnSettingsInvoked();

    winrt::UIElement FindSelectionIndicator(const winrt::IInspectable& item);

    static void CreateAndAttachHeaderAnimation(winrt::Visual visual);

    void OnKeyDown(winrt::KeyRoutedEventArgs const& args);

    bool IsFullScreenOrTabletMode();

    void OnTopNavDataSourceChanged(winrt::NotifyCollectionChangedEventArgs const& args);

    int GetNavigationViewItemCountInPrimaryList();
    int GetNavigationViewItemCountInTopNav();
    winrt::SplitView GetSplitView();
    TopNavigationViewDataProvider& GetTopDataProvider() { return m_topDataProvider; };
    winrt::ListView LeftNavListView() { return m_leftNavListView.get(); };
    void TopNavigationViewItemContentChanged();

    void CoerceToGreaterThanZero(double& value);

    void Expand(winrt::NavigationViewItem const& value);
    void Collapse(winrt::NavigationViewItem const& value);

    winrt::NavigationViewItem GetLastExpandedItem();

private:
    bool ShouldIgnoreMeasureOverride();
    bool NeedTopPaddingForRS5OrHigher(winrt::CoreApplicationViewTitleBar const& coreTitleBar);
    void OnAccessKeyInvoked(winrt::IInspectable const& sender, winrt::AccessKeyInvokedEventArgs const& args);
    winrt::NavigationTransitionInfo CreateNavigationTransitionInfo(NavigationRecommendedTransitionDirection recommendedTransitionDirection);
    NavigationRecommendedTransitionDirection GetRecommendedTransitionDirection(winrt::DependencyObject const& prev, winrt::DependencyObject const& next);
    winrt::NavigationViewItemBase GetContainerForClickedItem(winrt::IInspectable const& itemData);
    inline NavigationViewTemplateSettings* GetTemplateSettings();
    inline bool IsNavigationViewListSingleSelectionFollowsFocus();
    inline void UpdateSingleSelectionFollowsFocusTemplateSetting();
    void OnSelectedItemPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(winrt::IInspectable const& item);
    bool DoesSelectedItemContainContent(winrt::IInspectable const& item, winrt::NavigationViewItemBase const& itemContainer);
    void ChangeSelectStatusForItem(winrt::IInspectable const& item, bool selected);
    bool IsSettingsItem(winrt::IInspectable const& item);
    void UnselectPrevItem(winrt::IInspectable const& prevItem, winrt::IInspectable const& nextItem);
    void UndoSelectionAndRevertSelectionTo(winrt::IInspectable const& prevSelectedItem, winrt::IInspectable const& nextItem);
    void CloseTopNavigationViewFlyout();
    void UpdateVisualState(bool useTransitions = false);
    void UpdateVisualStateForOverflowButton();
    void UpdateLeftNavigationOnlyVisualState(bool useTransitions);
    void SetNavigationViewListPosition(winrt::ListView& listView, NavigationViewListPosition position);
    void UpdateNavigationViewUseSystemVisual();
    void PropagateNavigationViewAsParent();
    void PropagateChangeToNavigationViewLists(NavigationViewPropagateTarget target, std::function<void(NavigationViewList*)> const& function);
    void PropagateChangeToNavigationViewList(winrt::ListView const& listView, std::function<void(NavigationViewList*)> const& function);

    void InvalidateTopNavPrimaryLayout();
    // Measure functions for top navigation   
    float MeasureTopNavigationViewDesiredWidth(winrt::Size const& availableSize);
    float MeasureTopNavMenuItemsHostDesiredWidth(winrt::Size const& availableSize);
    float GetTopNavigationViewActualWidth();
    bool IsTopNavigationFirstMeasure();

    void RequestInvalidateMeasureOnNextLayoutUpdate();
    bool HasTopNavigationViewItemNotInPrimaryList();
    void HandleTopNavigationMeasureOverride(winrt::Size const& availableSize);
    void HandleTopNavigationMeasureOverrideNormal(const winrt::Windows::Foundation::Size & availableSize);
    void HandleTopNavigationMeasureOverrideOverflow(const winrt::Windows::Foundation::Size & availableSize);
    void ContinueHandleTopNavigationMeasureOverride(TopNavigationViewLayoutState nextMode, const winrt::Windows::Foundation::Size & availableSize);
    void HandleTopNavigationMeasureOverrideStep3(winrt::Size const& availableSize);
    void SetOverflowButtonVisibility(winrt::Visibility const& visibility);
    void SetTopNavigationViewNextMode(TopNavigationViewLayoutState nextMode);
    void SelectOverflowItem(winrt::IInspectable const& item);

    void ShrinkTopNavigationSize(float desiredWidth, winrt::Size const& availableSize);

    std::vector<int> FindMovableItemsRecoverToPrimaryList(float availableWidth, std::vector<int> const& includeItems);
    std::vector<int> FindMovableItemsToBeRemovedFromPrimaryList(float widthToBeRemoved, std::vector<int> const& excludeItems);
    std::vector<int> FindMovableItemsBeyondAvailableWidth(float availableWidth);
    void KeepAtLeastOneItemInPrimaryList(std::vector<int> itemInPrimaryToBeRemoved, bool shouldKeepFirst);
    void UpdateTopNavigationWidthCache();

    int GetSelectedItemIndex();

    bool BumperNavigation(int offset);

    bool IsTopNavigationView();
    bool IsTopPrimaryListVisible();

    void CreateAndHookEventsToSettings(std::wstring_view settingsName);
    void OnIsPaneOpenChanged();
    void UpdateHeaderVisibility();
    void UpdateHeaderVisibility(winrt::NavigationViewDisplayMode displayMode);
    void UpdatePaneToggleButtonVisibility();
    void UpdatePaneDisplayMode();
    void UpdatePaneDisplayMode(winrt::NavigationViewPaneDisplayMode oldDisplayMode, winrt::NavigationViewPaneDisplayMode newDisplayMode);
    void UpdatePaneVisibility();
    void UpdateContentBindingsForPaneDisplayMode();
    void UpdateSelectedItem();
    void UpdatePaneTabFocusNavigation();
    void UpdatePaneToggleSize();
    void UpdateBackButtonVisibility();
    void UpdatePaneTitleMargins();
    void UpdateLeftNavListViewItemSource(const winrt::IInspectable& items);
    void UpdateTopNavListViewItemSource(const winrt::IInspectable& items);
    void UpdateListViewItemsSource(const winrt::ListView& listView, const winrt::IInspectable& itemsSource);
    void UpdateListViewItemSource();

    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnLayoutUpdated(const winrt::IInspectable& sender, const winrt::IInspectable& e);
    void UpdateAdaptiveLayout(double width, bool forceSetDisplayMode = false);
    void SetDisplayMode(const winrt::NavigationViewDisplayMode& displayMode, bool forceSetDisplayMode = false);
   
    NavigationViewVisualStateDisplayMode GetVisualStateDisplayMode(const winrt::NavigationViewDisplayMode& displayMode);
    void UpdateVisualStateForDisplayModeGroup(const winrt::NavigationViewDisplayMode& displayMode);

    // Event Handlers
    void OnPaneToggleButtonClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnSettingsTapped(const winrt::IInspectable& sender, const winrt::TappedRoutedEventArgs& args);
    void OnSettingsKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnSettingsKeyUp(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnPaneSearchButtonClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    void OnItemClick(const winrt::IInspectable& sender, const winrt::ItemClickEventArgs& args);
    void RaiseItemInvoked(winrt::IInspectable const& item, 
        bool isSettings, 
        winrt::NavigationViewItemBase const& container = nullptr, 
        NavigationRecommendedTransitionDirection recommendedDirection = NavigationRecommendedTransitionDirection::Default);

    void OnSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args);
    void OnOverflowItemSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args);
    void RaiseSelectionChangedEvent(winrt::IInspectable const& nextItem, 
        bool isSettingsItem,
        NavigationRecommendedTransitionDirection recommendedDirection = NavigationRecommendedTransitionDirection::Default);
    void ChangeSelection(const winrt::IInspectable& prevItem, const winrt::IInspectable& nextItem);

    void OnTitleBarMetricsChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnTitleBarIsVisibleChanged(const winrt::CoreApplicationViewTitleBar& sender, const winrt::IInspectable& args);
    void UpdateTitleBarPadding();

    void RaiseDisplayModeChanged(const winrt::NavigationViewDisplayMode& displayMode);
    void AnimateSelectionChanged(const winrt::IInspectable& lastItem, const winrt::IInspectable& currentItem);
    void AnimateSelectionChangedToItem(const winrt::IInspectable& selectedItem);
    void PlayIndicatorAnimations(const winrt::UIElement& indicator, float yFrom, float yTo, winrt::Size beginSize, winrt::Size endSize, bool isOutgoing);
    void OnAnimationComplete(const winrt::IInspectable& sender, const winrt::CompositionBatchCompletedEventArgs& args);
    void ResetElementAnimationProperties(const winrt::UIElement& element, float desiredOpacity);
    winrt::NavigationViewItem NavigationViewItemOrSettingsContentFromData(const winrt::IInspectable& data);
    winrt::NavigationViewItemBase NavigationViewItemBaseOrSettingsContentFromData(const winrt::IInspectable& data);

    void RaiseIsExpanding(winrt::NavigationViewItemBase const& item);
    void RaiseCollapsed(winrt::NavigationViewItemBase const& item);

    // Cache these objects for the view as they are expensive to query via GetForCurrentView() calls.
    winrt::ViewManagement::ApplicationView m_applicationView{ nullptr };
    winrt::ViewManagement::UIViewSettings m_uiViewSettings{ nullptr };

    template<typename T> T GetContainerForData(const winrt::IInspectable& data)
    {
        if (!data)
        {
            return nullptr;
        }

        if (auto nvi = data.try_as<T>())
        {
            return nvi;
        }

        if (auto lv = IsTopNavigationView() ? m_topNavListView.get() : m_leftNavListView.get())
        {
            if (auto itemContainer = lv.ContainerFromItem(data))
            {
                return itemContainer.try_as<T>();
            }
        }

        if (auto settingsItem = m_settingsItem.get())
        {
            if (settingsItem == data || settingsItem.Content() == data)
            {
                return settingsItem.try_as<T>();
            }
        }

        return nullptr;
    }

    void OpenPane();
    void ClosePane();
    bool AttemptClosePaneLightly();
    void ClosePaneLightly();
    void SetPaneToggleButtonAutomationName();
    void SwapPaneHeaderContent(tracker_ref<winrt::ContentControl> newParent, tracker_ref<winrt::ContentControl> oldParent, winrt::hstring const& propertyPathName);
    void UpdateSettingsItemToolTip();

    void OnSplitViewClosedCompactChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnSplitViewPaneClosed(const winrt::DependencyObject& sender, const winrt::IInspectable& obj);
    void OnSplitViewPaneClosing(const winrt::DependencyObject& sender, const winrt::SplitViewPaneClosingEventArgs& args);
    void OnSplitViewPaneOpened(const winrt::DependencyObject& sender, const winrt::IInspectable& obj);
    void OnSplitViewPaneOpening(const winrt::DependencyObject& sender, const winrt::IInspectable& obj);
    void UpdateIsClosedCompact();

    void OnBackButtonClicked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    bool IsOverlay();
    bool IsLightDismissible();
    bool ShouldShowBackButton();

    void UnhookEventsAndClearFields(bool isFromDestructor = false);

    bool IsSelectionSuppressed(const winrt::IInspectable& item);
    
    bool ShouldPreserveNavigationViewRS4Behavior();
    bool ShouldPreserveNavigationViewRS3Behavior();

    // Visual components
    tracker_ref<winrt::Button> m_paneToggleButton{ this };
    tracker_ref<winrt::SplitView> m_rootSplitView{ this };
    tracker_ref<winrt::NavigationViewItem> m_settingsItem{ this };
    tracker_ref<winrt::UIElement> m_paneContentGrid{ this };
    tracker_ref<winrt::Button> m_paneSearchButton{ this };
    tracker_ref<winrt::Button> m_backButton{ this };
    tracker_ref<winrt::TextBlock> m_paneTitleTextBlock{ this };
    tracker_ref<winrt::Grid> m_buttonHolderGrid{ this };
    tracker_ref<winrt::ListView> m_leftNavListView{ this };
    tracker_ref<winrt::ListView> m_topNavListView{ this };
    tracker_ref<winrt::Button> m_topNavOverflowButton{ this };
    tracker_ref<winrt::ListView> m_topNavListOverflowView{ this };
    tracker_ref<winrt::Grid> m_topNavGrid{ this };
    tracker_ref<winrt::Border> m_topNavContentOverlayAreaGrid{ this };

    tracker_ref<winrt::UIElement> m_prevIndicator{ this };
    tracker_ref<winrt::UIElement> m_nextIndicator{ this };

    tracker_ref<winrt::FrameworkElement> m_togglePaneTopPadding{ this };
    tracker_ref<winrt::FrameworkElement> m_contentPaneTopPadding{ this };
    tracker_ref<winrt::FrameworkElement> m_topPadding{ this };
    tracker_ref<winrt::FrameworkElement> m_headerContent{ this };

    tracker_ref<winrt::CoreApplicationViewTitleBar> m_coreTitleBar{ this };

    tracker_ref<winrt::ContentControl> m_leftNavPaneAutoSuggestBoxPresenter{ this };
    tracker_ref<winrt::ContentControl> m_topNavPaneAutoSuggestBoxPresenter{ this };

    tracker_ref<winrt::ContentControl> m_leftNavPaneHeaderContentBorder{ this };
    tracker_ref<winrt::ContentControl> m_leftNavPaneCustomContentBorder{ this };
    tracker_ref<winrt::ContentControl> m_leftNavFooterContentBorder{ this };

    tracker_ref<winrt::ContentControl> m_paneHeaderOnTopPane{ this };
    tracker_ref<winrt::ContentControl> m_paneCustomContentOnTopPane{ this };
    tracker_ref<winrt::ContentControl> m_paneFooterOnTopPane{ this };
    
    int m_indexOfLastSelectedItemInTopNav{ 0 };
    tracker_ref<winrt::IInspectable> m_lastSelectedItemPendingAnimationInTopNav{ this };
    std::vector<int> m_itemsRemovedFromMenuFlyout{};

    // Event Tokens
    winrt::Button::Click_revoker m_paneToggleButtonClickRevoker{};
    winrt::UIElement::Tapped_revoker m_settingsItemTappedRevoker{};
    winrt::UIElement::KeyDown_revoker m_settingsItemKeyDownRevoker{};
    winrt::UIElement::KeyUp_revoker m_settingsItemKeyUpRevoker{};
    winrt::Button::Click_revoker m_paneSearchButtonClickRevoker{};
    winrt::CoreApplicationViewTitleBar::LayoutMetricsChanged_revoker m_titleBarMetricsChangedRevoker{};
    winrt::CoreApplicationViewTitleBar::IsVisibleChanged_revoker m_titleBarIsVisibleChangedRevoker{};
    winrt::Button::Click_revoker m_backButtonClickedRevoker{};
    winrt::ListView::ItemClick_revoker m_leftNavListViewItemClickRevoker{};
    winrt::ListView::Loaded_revoker m_leftNavListViewLoadedRevoker{};
    winrt::ListView::SelectionChanged_revoker m_leftNavListViewSelectionChangedRevoker{};
    winrt::ListView::ItemClick_revoker m_topNavListViewItemClickRevoker{};
    winrt::ListView::Loaded_revoker m_topNavListViewLoadedRevoker{};
    winrt::ListView::SelectionChanged_revoker m_topNavListViewSelectionChangedRevoker{};
    winrt::ListView::SelectionChanged_revoker m_topNavListOverflowViewSelectionChangedRevoker{};
    PropertyChanged_revoker m_splitViewIsPaneOpenChangedRevoker{};
    PropertyChanged_revoker m_splitViewDisplayModeChangedRevoker{};
    winrt::SplitView::PaneClosed_revoker m_splitViewPaneClosedRevoker{};
    winrt::SplitView::PaneClosing_revoker m_splitViewPaneClosingRevoker{};
    winrt::SplitView::PaneOpened_revoker m_splitViewPaneOpenedRevoker{};
    winrt::SplitView::PaneOpening_revoker m_splitViewPaneOpeningRevoker{};
    winrt::FrameworkElement::LayoutUpdated_revoker m_layoutUpdatedToken{};
    winrt::UIElement::AccessKeyInvoked_revoker m_accessKeyInvokedRevoker{};

    bool m_wasForceClosed{ false };
    bool m_isClosedCompact{ false };
    bool m_blockNextClosingEvent{ false };
    bool m_initialListSizeStateSet{ false };

    TopNavigationViewDataProvider m_topDataProvider{ this };

    // Hierarchical Nav Additions

    tracker_ref<winrt::TreeViewNode> m_rootNode{ this };

    winrt::IVector<winrt::TreeViewNode> RootNodes();

    void SyncRootNodesWithItemsSource(winrt::IInspectable const& items);

    winrt::TreeViewNode NodeFromContainer(winrt::DependencyObject const& container);
    winrt::DependencyObject ContainerFromNode(winrt::TreeViewNode const& node);
    winrt::TreeViewNode NodeFromPreviouslySelectedItem(winrt::IInspectable const& item);

    void ToggleIsExpanded(winrt::NavigationViewItem const& item);
    void UpdateIsChildSelected(winrt::IInspectable const& prevItem, winrt::IInspectable const& nextItem);
    winrt::ListView GetActiveListView();

    bool m_appliedTemplate{ false };

    // flag is used to stop recursive call. eg:
    // Customer select an item from SelectedItem property->ChangeSelection update ListView->LIstView raise OnSelectChange(we want stop here)->change property do do animation again.
    // Customer clicked listview->listview raised OnSelectChange->SelectedItem property changed->ChangeSelection->Undo the selection by SelectedItem(prevItem) (we want it stop here)->ChangeSelection again ->...
    bool m_shouldIgnoreNextSelectionChange{ false };
   
    // If SelectedItem is set by API, ItemInvoked should not be raised. 
    bool m_shouldRaiseInvokeItemInSelectionChange{ false };

    // Because virtualization for ItemsStackPanel, not all containers are realized. It request another round of MeasureOverride
    bool m_shouldInvalidateMeasureOnNextLayoutUpdate{ false };

    // during measuring, we should ignore SelectChange in overflow, otherwise it enters deadloop.
    bool m_shouldIgnoreOverflowItemSelectionChange{ false };

    // when exchanging items between overflow and primary, it cause selectionchange. and then item invoked, and may cause MeasureOverride like customer changed something.
    bool m_shouldIgnoreNextMeasureOverride{ false };

    // A flag to track that the selectionchange is caused by selection a item in topnav overflow menu
    bool m_selectionChangeFromOverflowMenu{ false };

    TopNavigationViewLayoutState m_topNavigationMode{ TopNavigationViewLayoutState::InitStep1 };

    // A threshold to stop recovery from overflow to normal happens immediately on resize.
    float m_topNavigationRecoveryGracePeriodWidth{ 5.f };

    // Avoid layout cycle on InitStep2
    int m_measureOnInitStep2Count{ 0 };

    // There are three ways to change IsPaneOpen:
    // 1, customer call IsPaneOpen=true/false directly or nav.IsPaneOpen is binding with a variable and the value is changed.
    // 2, customer click ToggleButton or splitView.IsPaneOpen->nav.IsPaneOpen changed because of window resize
    // 3, customer changed PaneDisplayMode.
    // 2 and 3 are internal implementation and will call by ClosePane/OpenPane. the flag is to indicate 1 if it's false
    bool m_isOpenPaneForInteraction{ false };
};

