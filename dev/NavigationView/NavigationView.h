// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class NavigationViewItem;
struct bringintoview_event_revoker;

#include "NavigationViewTemplateSettings.h"
#include "NavigationViewItem.h"
#include "NavigationView.g.h"
#include "TopNavigationViewDataProvider.h"
#include "NavigationViewHelper.h"
#include "NavigationView.properties.h"
#include "NavigationViewItemsFactory.h"

enum class TopNavigationViewLayoutState
{
    Uninitialized = 0,
    Initialized
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

    // IUIElement / IUIElementOverridesHelper
    winrt::AutomationPeer OnCreateAutomationPeer();

    winrt::IInspectable MenuItemFromContainer(winrt::DependencyObject const& container);
    winrt::DependencyObject ContainerFromMenuItem(winrt::IInspectable const& item);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&  args);
    void OnRepeaterLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

    void OnSettingsInvoked();

    winrt::UIElement FindSelectionIndicator(const winrt::IInspectable& item);

    static void CreateAndAttachHeaderAnimation(const winrt::Visual& visual);

    void OnPreviewKeyDown(winrt::KeyRoutedEventArgs const& args);
    void OnKeyDown(winrt::KeyRoutedEventArgs const& args);

    bool IsFullScreenOrTabletMode();

    void OnTopNavDataSourceChanged(winrt::NotifyCollectionChangedEventArgs const& args);

    int GetNavigationViewItemCountInPrimaryList();
    int GetNavigationViewItemCountInTopNav();
    winrt::SplitView GetSplitView();
    TopNavigationViewDataProvider& GetTopDataProvider() { return m_topDataProvider; };
    void TopNavigationViewItemContentChanged();

    void CoerceToGreaterThanZero(double& value);

    void OnRepeaterElementPrepared(const winrt::ItemsRepeater& ir, const winrt::ItemsRepeaterElementPreparedEventArgs& args);
    void OnRepeaterElementClearing(const winrt::ItemsRepeater& ir, const winrt::ItemsRepeaterElementClearingEventArgs& args);

    com_ptr<NavigationViewItemsFactory> GetNavigationViewMenuItemsFactory() { return m_navigationViewMenuItemsFactory; };
    com_ptr<NavigationViewItemsFactory> GetNavigationViewFooterItemsFactory() { return m_navigationViewFooterItemsFactory; };

    // Used in AutomationPeer
    winrt::ItemsRepeater LeftNavRepeater();
    winrt::NavigationViewItem GetSelectedContainer();
    winrt::ItemsRepeater GetParentItemsRepeaterForContainer(const winrt::NavigationViewItemBase& nvib);
    winrt::ItemsRepeater GetParentRootItemsRepeaterForContainer(const winrt::NavigationViewItemBase& nvib); 
    winrt::IndexPath GetIndexPathForContainer(const winrt::NavigationViewItemBase& nvib);

    // Hierarchical related functions
    void Expand(const winrt::NavigationViewItem& item);
    void Collapse(const winrt::NavigationViewItem& item);

    // Selection handling functions
    void OnNavigationViewItemInvoked(const winrt::NavigationViewItem& nvi);

private:

    // Selection handling functions
    void OnNavigationViewItemIsSelectedPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnSelectionModelSelectionChanged(const winrt::SelectionModel& selectionModel, const winrt::SelectionModelSelectionChangedEventArgs& e);
    void OnSelectionModelChildrenRequested(const winrt::SelectionModel& selectionModel, const winrt::SelectionModelChildrenRequestedEventArgs& e);
    void OnSelectedItemPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void ChangeSelection(const winrt::IInspectable& prevItem, const winrt::IInspectable& nextItem);
    void UpdateSelectionModelSelection(const winrt::IndexPath& ip);

    // Item/container info functions
    int GetIndexFromItem(const winrt::ItemsRepeater& ir, const winrt::IInspectable& data);
    static winrt::IInspectable GetItemFromIndex(const winrt::ItemsRepeater& ir, int index);
    winrt::IndexPath GetIndexPathOfItem(const winrt::IInspectable& data);
    winrt::UIElement GetContainerForIndex(int index, bool inFooter);
    winrt::NavigationViewItemBase GetContainerForIndexPath(const winrt::IndexPath& ip, bool lastVisible = false);
    winrt::NavigationViewItemBase GetContainerForIndexPath(const winrt::UIElement& firstContainer, const winrt::IndexPath& ip, bool lastVisible);
    winrt::IInspectable GetChildrenForItemInIndexPath(const winrt::IndexPath& ip, bool forceRealize = false);
    winrt::IInspectable GetChildrenForItemInIndexPath(const winrt::UIElement& firstContainer, const winrt::IndexPath& ip, bool forceRealize = false);
    winrt::UIElement SearchEntireTreeForContainer(const winrt::ItemsRepeater& rootRepeater, const winrt::IInspectable& data);
    winrt::IndexPath SearchEntireTreeForIndexPath(const winrt::ItemsRepeater& rootRepeater, const winrt::IInspectable& data, bool isFooterRepeater);
    winrt::IndexPath SearchEntireTreeForIndexPath(const winrt::NavigationViewItem& parentContainer, const winrt::IInspectable& data, const winrt::IndexPath& ip);

    winrt::ItemsRepeater GetChildRepeaterForIndexPath(const winrt::IndexPath& ip);
    winrt::NavigationViewItem GetParentNavigationViewItemForContainer(const winrt::NavigationViewItemBase& nvib);
    bool IsContainerTheSelectedItemInTheSelectionModel(const winrt::NavigationViewItemBase& nvib);
    int GetContainerCountInRepeater(const winrt::ItemsRepeater& ir);
    bool DoesRepeaterHaveRealizedContainers(const winrt::ItemsRepeater& ir);
    bool IsSettingsItem(winrt::IInspectable const& item);
    bool IsSelectionSuppressed(const winrt::IInspectable& item);
    bool DoesNavigationViewItemHaveChildren(const winrt::NavigationViewItem& nvi);
    bool IsTopLevelItem(const winrt::NavigationViewItemBase& nvib);
    winrt::IInspectable GetChildren(const winrt::NavigationViewItem& nvi);

    // Hierarchy related functions
    void ToggleIsExpandedNavigationViewItem(const winrt::NavigationViewItem& nvi);
    void ChangeIsExpandedNavigationViewItem(const winrt::NavigationViewItem& nvi, bool isExpanded);
    void ShowHideChildrenItemsRepeater(const winrt::NavigationViewItem& nvi);
    winrt::NavigationViewItem FindLowestLevelContainerToDisplaySelectionIndicator();
    void UpdateIsChildSelectedForIndexPath(const winrt::IndexPath& ip, bool isChildSelected);
    void UpdateIsChildSelected(const winrt::IndexPath& prevIP, const winrt::IndexPath& nextIP);
    void CollapseTopLevelMenuItems(winrt::NavigationViewPaneDisplayMode oldDisplayMode);
    void CollapseMenuItemsInRepeater(const winrt::ItemsRepeater& ir);
    void RaiseExpandingEvent(const winrt::NavigationViewItemBase& container);
    void RaiseCollapsedEvent(const winrt::NavigationViewItemBase& container);
    void CloseFlyoutIfRequired(const winrt::NavigationViewItem& selectedItem);

    // Force realization functions
    winrt::NavigationViewItemBase ResolveContainerForItem(const winrt::IInspectable& item, int index);
    void RecycleContainer(const winrt::UIElement& container);

    void OnFlyoutClosing(const winrt::IInspectable& sender, const winrt::FlyoutBaseClosingEventArgs& args);

    void ClosePaneIfNeccessaryAfterItemIsClicked(const winrt::NavigationViewItem& selectedItem);
    bool NeedTopPaddingForRS5OrHigher(winrt::CoreApplicationViewTitleBar const& coreTitleBar);
    void OnAccessKeyInvoked(winrt::IInspectable const& sender, winrt::AccessKeyInvokedEventArgs const& args);
    winrt::NavigationTransitionInfo CreateNavigationTransitionInfo(NavigationRecommendedTransitionDirection recommendedTransitionDirection);
    NavigationRecommendedTransitionDirection GetRecommendedTransitionDirection(winrt::DependencyObject const& prev, winrt::DependencyObject const& next);
    inline NavigationViewTemplateSettings* GetTemplateSettings();
    inline bool IsNavigationViewListSingleSelectionFollowsFocus();
    inline void UpdateSingleSelectionFollowsFocusTemplateSetting();
    void OnMenuItemsSourceCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnFooterItemsSourceCollectionChanged(const winrt::IInspectable &, const winrt::IInspectable &);
    void OnOverflowItemsSourceCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(winrt::IInspectable const& item);
    void ChangeSelectStatusForItem(winrt::IInspectable const& item, bool selected);
    void UnselectPrevItem(winrt::IInspectable const& prevItem, winrt::IInspectable const& nextItem);
    void UndoSelectionAndRevertSelectionTo(winrt::IInspectable const& prevSelectedItem, winrt::IInspectable const& nextItem);
    void CloseTopNavigationViewFlyout();
    void UpdateVisualState(bool useTransitions = false);
    void UpdateVisualStateForOverflowButton();
    void UpdateLeftNavigationOnlyVisualState(bool useTransitions);
    void UpdatePaneShadow();
    void UpdateNavigationViewItemsFactories();
    void SyncItemTemplates();
    bool IsRootGridOfFlyout(const winrt::DependencyObject& element);
    bool IsRootItemsRepeater(const winrt::DependencyObject& element);
    void RaiseItemInvoked(winrt::IInspectable const& item,
        bool isSettings,
        winrt::NavigationViewItemBase const& container = nullptr,
        NavigationRecommendedTransitionDirection recommendedDirection = NavigationRecommendedTransitionDirection::Default);
    void RaiseItemInvokedForNavigationViewItem(const winrt::NavigationViewItem& nvi);
    void HandleKeyEventForNavigationViewItem(const winrt::NavigationViewItem& nvi, const winrt::KeyRoutedEventArgs& args);

    // This property is attached to the NavigationViewItems that are being
    // displayed by the repeaters in this control. It is used to keep track of the
    // revokers for NavigationViewItem events and allows them to get revoked when
    // the item gets cleaned up
    GlobalDependencyProperty s_NavigationViewItemRevokersProperty{ nullptr };

    void InvalidateTopNavPrimaryLayout();
    // Measure functions for top navigation   
    float MeasureTopNavigationViewDesiredWidth(winrt::Size const& availableSize);
    float MeasureTopNavMenuItemsHostDesiredWidth(winrt::Size const& availableSize);
    float GetTopNavigationViewActualWidth();

    bool HasTopNavigationViewItemNotInPrimaryList();
    void HandleTopNavigationMeasureOverride(winrt::Size const& availableSize);
    void HandleTopNavigationMeasureOverrideNormal(const winrt::Windows::Foundation::Size & availableSize);
    void HandleTopNavigationMeasureOverrideOverflow(const winrt::Windows::Foundation::Size & availableSize);
    void SetOverflowButtonVisibility(winrt::Visibility const& visibility);
    void SelectOverflowItem(winrt::IInspectable const& item, winrt::IndexPath const& ip);
    void SelectandMoveOverflowItem(winrt::IInspectable const& selectedItem, winrt::IndexPath const& selectedIndex, bool closeFlyout);

    void ResetAndRearrangeTopNavItems(winrt::Size const& availableSize);
    void ArrangeTopNavItems(winrt::Size const& availableSize);

    void ShrinkTopNavigationSize(float desiredWidth, winrt::Size const& availableSize);

    std::vector<int> FindMovableItemsRecoverToPrimaryList(float availableWidth, std::vector<int> const& includeItems);
    std::vector<int> FindMovableItemsToBeRemovedFromPrimaryList(float widthToBeRemoved, std::vector<int> const& excludeItems);
    std::vector<int> FindMovableItemsBeyondAvailableWidth(float availableWidth);
    void KeepAtLeastOneItemInPrimaryList(std::vector<int>& itemInPrimaryToBeRemoved, bool shouldKeepFirst);
    void UpdateTopNavigationWidthCache();

    int GetSelectedItemIndex();
    double GetPaneToggleButtonWidth();
    double GetPaneToggleButtonHeight();

    bool BumperNavigation(int offset);
    bool SelectSelectableItemWithOffset(int startIndex, int offset, winrt::ItemsRepeater const& repeater, int repeaterCollectionSize);

    bool IsTopNavigationView();
    bool IsTopPrimaryListVisible();

    void CreateAndHookEventsToSettings();
    void OnIsPaneOpenChanged();
    void UpdatePaneButtonsWidths();
    void UpdateHeaderVisibility();
    void UpdateHeaderVisibility(winrt::NavigationViewDisplayMode displayMode);
    void UpdatePaneToggleButtonVisibility();
    void UpdatePaneDisplayMode();
    void UpdatePaneDisplayMode(winrt::NavigationViewPaneDisplayMode oldDisplayMode, winrt::NavigationViewPaneDisplayMode newDisplayMode);
    void UpdatePaneVisibility();
    void UpdateContentBindingsForPaneDisplayMode();
    void UpdatePaneTabFocusNavigation();
    void UpdatePaneToggleSize();
    void UpdateBackAndCloseButtonsVisibility();
    void UpdatePaneTitleMargins();
    void UpdateLeftRepeaterItemSource(const winrt::IInspectable& items);
    void UpdateTopNavRepeatersItemSource(const winrt::IInspectable& items);
    void UpdateTopNavPrimaryRepeaterItemsSource(const winrt::IInspectable& items);
    void UpdateTopNavOverflowRepeaterItemsSource(const winrt::IInspectable& items);
    static void UpdateItemsRepeaterItemsSource(const winrt::ItemsRepeater& listView, const winrt::IInspectable& itemsSource);
    void UpdateSelectionForMenuItems();
    bool UpdateSelectedItemFromMenuItems(const winrt::impl::com_ref<winrt::IVector<winrt::IInspectable>>& menuItems, bool foundFirstSelected = false);
    bool m_InitialNonForcedModeUpdate{ true };

    void UpdateRepeaterItemsSource(bool forceSelectionModelUpdate);
    void UpdateFooterRepeaterItemsSource(bool sourceCollectionReseted, bool sourceCollectionChanged);

    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnItemsContainerSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnLayoutUpdated(const winrt::IInspectable& sender, const winrt::IInspectable& e);
    void UpdateAdaptiveLayout(double width, bool forceSetDisplayMode = false);
    void UpdatePaneLayout();
    void SetDisplayMode(const winrt::NavigationViewDisplayMode& displayMode, bool forceSetDisplayMode = false);
   
    NavigationViewVisualStateDisplayMode GetVisualStateDisplayMode(const winrt::NavigationViewDisplayMode& displayMode);
    void UpdateVisualStateForDisplayModeGroup(const winrt::NavigationViewDisplayMode& displayMode);

    // Event Handlers
    void OnPaneToggleButtonClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnPaneSearchButtonClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnPaneTitleHolderSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);

    void OnNavigationViewItemTapped(const winrt::IInspectable& sender, const winrt::TappedRoutedEventArgs& args);
    void OnNavigationViewItemKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnRepeaterGettingFocus(const winrt::IInspectable& sender, const winrt::GettingFocusEventArgs& args);
    void OnNavigationViewItemOnGotFocus(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& e);
    void OnNavigationViewItemExpandedPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);

    void RaiseSelectionChangedEvent(winrt::IInspectable const& nextItem, 
        bool isSettingsItem,
        NavigationRecommendedTransitionDirection recommendedDirection = NavigationRecommendedTransitionDirection::Default);

    void OnTitleBarMetricsChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnTitleBarIsVisibleChanged(const winrt::CoreApplicationViewTitleBar& sender, const winrt::IInspectable& args);
    void UpdateTitleBarPadding();

    void OnAutoSuggestBoxSuggestionChosen(const winrt::AutoSuggestBox& sender, const winrt::Windows::UI::Xaml::Controls::AutoSuggestBoxSuggestionChosenEventArgs& args);

    void RaiseDisplayModeChanged(const winrt::NavigationViewDisplayMode& displayMode);
    void AnimateSelectionChanged(const winrt::IInspectable& currentItem);
    void AnimateSelectionChangedToItem(const winrt::IInspectable& selectedItem);
    void PlayIndicatorAnimations(const winrt::UIElement& indicator, float yFrom, float yTo, winrt::Size beginSize, winrt::Size endSize, bool isOutgoing);
    void PlayIndicatorNonSameLevelAnimations(const winrt::UIElement& indicator, bool isOutgoing, bool fromTop);
    void PlayIndicatorNonSameLevelTopPrimaryAnimation(const winrt::UIElement& indicator, bool isOutgoing);
    void OnAnimationComplete(const winrt::IInspectable& sender, const winrt::CompositionBatchCompletedEventArgs& args);
    void ResetElementAnimationProperties(const winrt::UIElement& element, float desiredOpacity);
    winrt::NavigationViewItem NavigationViewItemOrSettingsContentFromData(const winrt::IInspectable& data);
    winrt::NavigationViewItemBase NavigationViewItemBaseOrSettingsContentFromData(const winrt::IInspectable& data);

    // Cache these objects for the view as they are expensive to query via GetForCurrentView() calls.
    winrt::ViewManagement::ApplicationView m_applicationView{ nullptr };
    winrt::ViewManagement::UIViewSettings m_uiViewSettings{ nullptr };

    template<typename T> T GetContainerForData(const winrt::IInspectable& data);

    void OpenPane();
    void ClosePane();
    bool AttemptClosePaneLightly();
    void SetPaneToggleButtonAutomationName();
    void SwapPaneHeaderContent(tracker_ref<winrt::ContentControl> newParent, tracker_ref<winrt::ContentControl> oldParent, winrt::hstring const& propertyPathName);
    void UpdateSettingsItemToolTip();
    void UpdatePaneTitleFrameworkElementParents();
    std::function<void ()> SetPaneTitleFrameworkElementParent(const winrt::ContentControl& parent, const winrt::FrameworkElement& paneTitle, bool shouldNotContainPaneTitle);

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
    bool ShouldShowCloseButton();
    bool ShouldShowBackOrCloseButton();

    void UnhookEventsAndClearFields(bool isFromDestructor = false);
    
    bool ShouldPreserveNavigationViewRS4Behavior();
    bool ShouldPreserveNavigationViewRS3Behavior();

    bool NeedRearrangeOfTopElementsAfterOverflowSelectionChanged(int selectedOriginalIndex);
    void KeyboardFocusFirstItemFromItem(const winrt::NavigationViewItemBase& nvib);
    void KeyboardFocusLastItemFromItem(const winrt::NavigationViewItemBase& nvib);
    void FocusNextDownItem(const winrt::NavigationViewItem& nvi, const winrt::KeyRoutedEventArgs& args);
    void FocusNextUpItem(const winrt::NavigationViewItem& nvi, const winrt::KeyRoutedEventArgs& args);
    void ApplyCustomMenuItemContainerStyling(const winrt::NavigationViewItemBase& nvib, const winrt::ItemsRepeater& ir, int index);

    com_ptr<NavigationViewItemsFactory> m_navigationViewMenuItemsFactory{ nullptr };
    com_ptr<NavigationViewItemsFactory> m_navigationViewFooterItemsFactory{ nullptr };

    void SetDropShadow();
    void UnsetDropShadow();
    void ShadowCasterEaseOutStoryboard_Completed(const winrt::Grid& shadowCaster);

    // Visual components
    tracker_ref<winrt::Button> m_paneToggleButton{ this };
    tracker_ref<winrt::SplitView> m_rootSplitView{ this };
    tracker_ref<winrt::NavigationViewItem> m_settingsItem{ this };
    tracker_ref<winrt::RowDefinition> m_itemsContainerRow{ this };
    tracker_ref<winrt::FrameworkElement> m_menuItemsScrollViewer{ this };
    tracker_ref<winrt::FrameworkElement> m_footerItemsScrollViewer{ this };
    tracker_ref<winrt::UIElement> m_paneContentGrid{ this };
    tracker_ref<winrt::ColumnDefinition> m_paneToggleButtonIconGridColumn{ this };
    tracker_ref<winrt::FrameworkElement> m_paneTitleHolderFrameworkElement{ this };
    tracker_ref<winrt::FrameworkElement> m_paneTitleFrameworkElement{ this };
    tracker_ref<winrt::FrameworkElement> m_visualItemsSeparator{ this };
    tracker_ref<winrt::Button> m_paneSearchButton{ this };
    tracker_ref<winrt::Button> m_backButton{ this };
    tracker_ref<winrt::Button> m_closeButton{ this };
    tracker_ref<winrt::ItemsRepeater> m_leftNavRepeater{ this };
    tracker_ref<winrt::ItemsRepeater> m_topNavRepeater{ this };
    tracker_ref<winrt::ItemsRepeater> m_leftNavFooterMenuRepeater{ this };
    tracker_ref<winrt::ItemsRepeater> m_topNavFooterMenuRepeater{ this };
    tracker_ref<winrt::Button> m_topNavOverflowButton{ this };
    tracker_ref<winrt::ItemsRepeater> m_topNavRepeaterOverflowView{ this };
    tracker_ref<winrt::Grid> m_topNavGrid{ this };
    tracker_ref<winrt::Border> m_topNavContentOverlayAreaGrid{ this };
    tracker_ref<winrt::Grid> m_shadowCaster{ this };
    tracker_ref<winrt::Storyboard> m_shadowCasterEaseInStoryboard{ this };
    tracker_ref<winrt::Storyboard> m_shadowCasterSmallPaneEaseInStoryboard{ this };
    tracker_ref<winrt::Storyboard> m_shadowCasterEaseOutStoryboard{ this };

    // Indicator animations
    tracker_ref<winrt::UIElement> m_prevIndicator{ this };
    tracker_ref<winrt::UIElement> m_nextIndicator{ this };
    tracker_ref<winrt::UIElement> m_activeIndicator{ this };
    tracker_ref<winrt::IInspectable> m_lastSelectedItemPendingAnimationInTopNav{ this };

    tracker_ref<winrt::FrameworkElement> m_togglePaneTopPadding{ this };
    tracker_ref<winrt::FrameworkElement> m_contentPaneTopPadding{ this };
    tracker_ref<winrt::FrameworkElement> m_contentLeftPadding{ this };

    tracker_ref<winrt::CoreApplicationViewTitleBar> m_coreTitleBar{ this };

    tracker_ref<winrt::ContentControl> m_leftNavPaneAutoSuggestBoxPresenter{ this };
    tracker_ref<winrt::ContentControl> m_topNavPaneAutoSuggestBoxPresenter{ this };

    tracker_ref<winrt::ContentControl> m_leftNavPaneHeaderContentBorder{ this };
    tracker_ref<winrt::ContentControl> m_leftNavPaneCustomContentBorder{ this };
    tracker_ref<winrt::ContentControl> m_leftNavFooterContentBorder{ this };

    tracker_ref<winrt::ContentControl> m_paneHeaderOnTopPane{ this };
    tracker_ref<winrt::ContentControl> m_paneTitleOnTopPane{ this };
    tracker_ref<winrt::ContentControl> m_paneCustomContentOnTopPane{ this };
    tracker_ref<winrt::ContentControl> m_paneFooterOnTopPane{ this };
    tracker_ref<winrt::ContentControl> m_paneTitlePresenter{ this };

    tracker_ref<winrt::ColumnDefinition> m_paneHeaderCloseButtonColumn{ this };
    tracker_ref<winrt::ColumnDefinition> m_paneHeaderToggleButtonColumn{ this };
    tracker_ref<winrt::RowDefinition> m_paneHeaderContentBorderRow{ this };

    tracker_ref<winrt::NavigationViewItem> m_lastItemExpandedIntoFlyout{ this };

    // Event Tokens
    winrt::Button::Click_revoker m_paneToggleButtonClickRevoker{};
    winrt::Button::Click_revoker m_paneSearchButtonClickRevoker{};
    winrt::CoreApplicationViewTitleBar::LayoutMetricsChanged_revoker m_titleBarMetricsChangedRevoker{};
    winrt::CoreApplicationViewTitleBar::IsVisibleChanged_revoker m_titleBarIsVisibleChangedRevoker{};
    winrt::Button::Click_revoker m_backButtonClickedRevoker{};
    winrt::Button::Click_revoker m_closeButtonClickedRevoker{};
    PropertyChanged_revoker m_splitViewIsPaneOpenChangedRevoker{};
    PropertyChanged_revoker m_splitViewDisplayModeChangedRevoker{};
    winrt::SplitView::PaneClosed_revoker m_splitViewPaneClosedRevoker{};
    winrt::SplitView::PaneClosing_revoker m_splitViewPaneClosingRevoker{};
    winrt::SplitView::PaneOpened_revoker m_splitViewPaneOpenedRevoker{};
    winrt::SplitView::PaneOpening_revoker m_splitViewPaneOpeningRevoker{};
    winrt::FrameworkElement::LayoutUpdated_revoker m_layoutUpdatedToken{};
    winrt::UIElement::AccessKeyInvoked_revoker m_accessKeyInvokedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_paneTitleHolderFrameworkElementSizeChangedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_itemsContainerSizeChangedRevoker{};
    winrt::AutoSuggestBox::SuggestionChosen_revoker m_autoSuggestBoxSuggestionChosenRevoker{};

    winrt::ItemsRepeater::ElementPrepared_revoker m_leftNavItemsRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_leftNavItemsRepeaterElementClearingRevoker{};
    winrt::ItemsRepeater::Loaded_revoker m_leftNavRepeaterLoadedRevoker{};
    winrt::ItemsRepeater::GettingFocus_revoker m_leftNavRepeaterGettingFocusRevoker{};

    winrt::ItemsRepeater::ElementPrepared_revoker m_topNavItemsRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_topNavItemsRepeaterElementClearingRevoker{};
    winrt::ItemsRepeater::Loaded_revoker m_topNavRepeaterLoadedRevoker{};
    winrt::ItemsRepeater::GettingFocus_revoker m_topNavRepeaterGettingFocusRevoker{};

    winrt::ItemsRepeater::ElementPrepared_revoker m_leftNavFooterMenuItemsRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_leftNavFooterMenuItemsRepeaterElementClearingRevoker{};
    winrt::ItemsRepeater::Loaded_revoker m_leftNavFooterMenuRepeaterLoadedRevoker{};
    winrt::ItemsRepeater::GettingFocus_revoker m_leftNavFooterMenuRepeaterGettingFocusRevoker{};

    winrt::ItemsRepeater::ElementPrepared_revoker m_topNavFooterMenuItemsRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_topNavFooterMenuItemsRepeaterElementClearingRevoker{};
    winrt::ItemsRepeater::Loaded_revoker m_topNavFooterMenuRepeaterLoadedRevoker{};
    winrt::ItemsRepeater::GettingFocus_revoker m_topNavFooterMenuRepeaterGettingFocusRevoker{};

    winrt::ItemsRepeater::ElementPrepared_revoker m_topNavOverflowItemsRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_topNavOverflowItemsRepeaterElementClearingRevoker{};

    winrt::SelectionModel::SelectionChanged_revoker m_selectionChangedRevoker{};
    winrt::SelectionModel::ChildrenRequested_revoker m_childrenRequestedRevoker{};

    winrt::ItemsSourceView::CollectionChanged_revoker m_menuItemsCollectionChangedRevoker{};
    winrt::ItemsSourceView::CollectionChanged_revoker m_footerItemsCollectionChangedRevoker{};

    winrt::ItemsSourceView::CollectionChanged_revoker m_topNavOverflowItemsCollectionChangedRevoker{};

    winrt::FlyoutBase::Closing_revoker m_flyoutClosingRevoker{};

    winrt::Storyboard::Completed_revoker m_shadowCasterEaseOutStoryboardRevoker{};

    bool m_wasForceClosed{ false };
    bool m_isClosedCompact{ false };
    bool m_blockNextClosingEvent{ false };
    bool m_initialListSizeStateSet{ false };

    TopNavigationViewDataProvider m_topDataProvider{ this };

    winrt::SelectionModel m_selectionModel{};
    winrt::IVector<winrt::IInspectable> m_selectionModelSource{};

    winrt::ItemsSourceView m_menuItemsSource{ nullptr };
    winrt::ItemsSourceView m_footerItemsSource{ nullptr };

    bool m_appliedTemplate{ false };

    // Identifies whenever a call is the result of OnApplyTemplate
    bool m_fromOnApplyTemplate{ false };

    // Used to defer updating the SplitView displaymode property
    bool m_updateVisualStateForDisplayModeFromOnLoaded{ false };

    // flag is used to stop recursive call. eg:
    // Customer select an item from SelectedItem property->ChangeSelection update ListView->LIstView raise OnSelectChange(we want stop here)->change property do do animation again.
    // Customer clicked listview->listview raised OnSelectChange->SelectedItem property changed->ChangeSelection->Undo the selection by SelectedItem(prevItem) (we want it stop here)->ChangeSelection again ->...
    bool m_shouldIgnoreNextSelectionChange{ false };
    // A flag to track that the selectionchange is caused by selection a item in topnav overflow menu
    bool m_selectionChangeFromOverflowMenu{ false };
    // Flag indicating whether selection change should raise item invoked. This is needed to be able to raise ItemInvoked before SelectionChanged while SelectedItem should point to the clicked item
    bool m_shouldRaiseItemInvokedAfterSelection{ false };

    TopNavigationViewLayoutState m_topNavigationMode{ TopNavigationViewLayoutState::Uninitialized };

    // A threshold to stop recovery from overflow to normal happens immediately on resize.
    float m_topNavigationRecoveryGracePeriodWidth{ 5.f };

    // There are three ways to change IsPaneOpen:
    // 1, customer call IsPaneOpen=true/false directly or nav.IsPaneOpen is binding with a variable and the value is changed.
    // 2, customer click ToggleButton or splitView.IsPaneOpen->nav.IsPaneOpen changed because of window resize
    // 3, customer changed PaneDisplayMode.
    // 2 and 3 are internal implementation and will call by ClosePane/OpenPane. the flag is to indicate 1 if it's false
    bool m_isOpenPaneForInteraction{ false };

    bool m_moveTopNavOverflowItemOnFlyoutClose{ false };

    bool m_shouldIgnoreUIASelectionRaiseAsExpandCollapseWillRaise{ false };

    bool m_OrientationChangedPendingAnimation{ false };

    bool m_TabKeyPrecedesFocusChange{ false };
};

