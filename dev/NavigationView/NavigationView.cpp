// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "NavigationView.h"
#include "Vector.h"
#include "BindableVector.h"
#include "NavigationViewDisplayModeChangedEventArgs.h"
#include "NavigationViewPaneClosingEventArgs.h"
#include "NavigationViewBackRequestedEventArgs.h"
#include "ResourceAccessor.h"
#include "CppWinRTHelpers.h"
#include "NavigationViewItem.h"
#include "NavigationViewSelectionChangedEventArgs.h"
#include "NavigationViewItemInvokedEventArgs.h"
#include "RuntimeProfiler.h"
#include "NavigationViewList.h"
#include "Utils.h"
#include "TraceLogging.h"

static constexpr auto c_togglePaneButtonName = L"TogglePaneButton"sv;
static constexpr auto c_paneTitleTextBlock = L"PaneTitleTextBlock"sv;
static constexpr auto c_rootSplitViewName = L"RootSplitView"sv;
static constexpr auto c_menuItemsHost = L"MenuItemsHost"sv;
static constexpr auto c_settingsName = L"SettingsNavPaneItem"sv;
static constexpr auto c_settingsNameTopNav = L"SettingsTopNavPaneItem"sv;
static constexpr auto c_selectionIndicatorName = L"SelectionIndicator"sv;
static constexpr auto c_paneContentGridName = L"PaneContentGrid"sv;
static constexpr auto c_rootGridName = L"RootGrid"sv;
static constexpr auto c_contentGridName = L"ContentGrid"sv;
static constexpr auto c_searchButtonName = L"PaneAutoSuggestButton"sv;
static constexpr auto c_togglePaneTopPadding = L"TogglePaneTopPadding"sv;
static constexpr auto c_contentPaneTopPadding = L"ContentPaneTopPadding"sv;
static constexpr auto c_headerContent = L"HeaderContent"sv;
static constexpr auto c_navViewBackButton = L"NavigationViewBackButton"sv;
static constexpr auto c_navViewBackButtonToolTip = L"NavigationViewBackButtonToolTip"sv;
static constexpr auto c_buttonHolderGrid = L"ButtonHolderGrid"sv;

static constexpr auto c_topNavMenuItemsHost = L"TopNavMenuItemsHost"sv;
static constexpr auto c_topNavOverflowButton = L"TopNavOverflowButton"sv;
static constexpr auto c_topNavMenuItemsOverflowHost = L"TopNavMenuItemsOverflowHost"sv;
static constexpr auto c_topNavGrid = L"TopNavGrid"sv;
static constexpr auto c_topNavContentOverlayAreaGrid = L"TopNavContentOverlayAreaGrid"sv;
static constexpr auto c_leftNavPaneAutoSuggestBoxPresenter = L"PaneAutoSuggestBoxPresenter"sv;
static constexpr auto c_topNavPaneAutoSuggestBoxPresenter = L"TopPaneAutoSuggestBoxPresenter"sv;

static constexpr auto c_leftNavFooterContentBorder = L"FooterContentBorder"sv;
static constexpr auto c_leftNavPaneHeaderContentBorder = L"PaneHeaderContentBorder"sv;
static constexpr auto c_leftNavPaneCustomContentBorder = L"PaneCustomContentBorder"sv;

static constexpr auto c_paneHeaderOnTopPane = L"PaneHeaderOnTopPane"sv;
static constexpr auto c_paneCustomContentOnTopPane = L"PaneCustomContentOnTopPane"sv;
static constexpr auto c_paneFooterOnTopPane = L"PaneFooterOnTopPane"sv;

static constexpr int c_backButtonHeight = 44;
static constexpr int c_backButtonWidth = 48;
static constexpr int c_backButtonPaneButtonMargin = 8;
static constexpr int c_paneToggleButtonWidth = 48;
static constexpr int c_toggleButtonHeightWhenShouldPreserveNavigationViewRS3Behavior = 56;
static constexpr int c_backButtonRowDefinition = 1;
static constexpr float c_paneElevationTranslationZ = 32;

// A tricky to help to stop layout cycle. As we know, we may have this:
// 1 .. first time invalid measure, normal case because of virtualization
// 2 .. data update before next invalid measure
// 3 .. possible layout cycle. a buffer
// so 4 is selected for threshold.
constexpr int s_measureOnInitStep2CountThreshold{ 4 };

static winrt::Size c_infSize{ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };

using namespace std::chrono_literals;

NavigationView::~NavigationView()
{
    UnhookEventsAndClearFields(true);
}

void NavigationView::UnhookEventsAndClearFields(bool isFromDestructor)
{
    m_titleBarMetricsChangedRevoker.revoke();
    m_titleBarIsVisibleChangedRevoker.revoke();
    m_paneToggleButtonClickRevoker.revoke();

    m_settingsItemTappedRevoker.revoke();
    m_settingsItemKeyDownRevoker.revoke();
    m_settingsItemKeyUpRevoker.revoke();
    m_settingsItem.set(nullptr);

    m_leftNavListViewSelectionChangedRevoker.revoke();
    m_leftNavListViewItemClickRevoker.revoke();
    m_leftNavListViewLoadedRevoker.revoke();       
    m_leftNavListView.set(nullptr);

    m_leftNavListViewSelectionChangedRevoker.revoke();
    m_leftNavListViewItemClickRevoker.revoke();
    m_leftNavListViewLoadedRevoker.revoke();      
    m_leftNavListView.set(nullptr);

    m_topNavListViewSelectionChangedRevoker.revoke();
    m_topNavListViewItemClickRevoker.revoke();
    m_topNavListViewLoadedRevoker.revoke();
    m_topNavListView.set(nullptr);

    m_topNavListOverflowViewSelectionChangedRevoker.revoke();
    m_topNavListOverflowView.set(nullptr);

    m_paneSearchButtonClickRevoker.revoke();
    m_paneSearchButton.set(nullptr);

    m_buttonHolderGrid.set(nullptr);
}

NavigationView::NavigationView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_NavigationView);
    SetValue(s_TemplateSettingsProperty, winrt::make<::NavigationViewTemplateSettings>());
    SetDefaultStyleKey(this);

    SizeChanged({ this, &NavigationView::OnSizeChanged });
    auto items = winrt::make<Vector<winrt::IInspectable>>();
    SetValue(s_MenuItemsProperty, items);

    auto weakThis = get_weak();
    m_topDataProvider.OnRawDataChanged(
        [weakThis](const winrt::NotifyCollectionChangedEventArgs& args)
        {
            if (auto target = weakThis.get())
            {
                target->OnTopNavDataSourceChanged(args);
            }
        });

    Unloaded({ this, &NavigationView::OnUnloaded });

    m_rootNode.set(winrt::TreeViewNode());
}

void NavigationView::OnApplyTemplate()
{
    // Stop update anything because of PropertyChange during OnApplyTemplate. Update them all together at the end of this function
    m_appliedTemplate = false;

    UnhookEventsAndClearFields();

    winrt::IControlProtected controlProtected = *this;

    // Register for changes in title bar layout
    winrt::CoreApplicationViewTitleBar coreTitleBar = winrt::CoreApplication::GetCurrentView().TitleBar();
    if (coreTitleBar)
    {
        m_coreTitleBar.set(coreTitleBar);
        m_titleBarMetricsChangedRevoker = coreTitleBar.LayoutMetricsChanged(winrt::auto_revoke, { this, &NavigationView::OnTitleBarMetricsChanged });
        m_titleBarIsVisibleChangedRevoker = coreTitleBar.IsVisibleChanged(winrt::auto_revoke, { this, &NavigationView::OnTitleBarIsVisibleChanged });
        m_headerContent.set(GetTemplateChildT<winrt::FrameworkElement>(c_headerContent, controlProtected));

        if (ShouldPreserveNavigationViewRS4Behavior())
        {
            m_togglePaneTopPadding.set(GetTemplateChildT<winrt::FrameworkElement>(c_togglePaneTopPadding, controlProtected));
            m_contentPaneTopPadding.set(GetTemplateChildT<winrt::FrameworkElement>(c_contentPaneTopPadding, controlProtected));
        }
    }

    // Set up the pane toggle button click handler
    if (auto paneToggleButton = GetTemplateChildT<winrt::Button>(c_togglePaneButtonName, controlProtected))
    {
        m_paneToggleButton.set(paneToggleButton);
        m_paneToggleButtonClickRevoker = paneToggleButton.Click(winrt::auto_revoke, { this, &NavigationView::OnPaneToggleButtonClick });
            
        SetPaneToggleButtonAutomationName();

        if (SharedHelpers::IsRS3OrHigher())
        {
            winrt::KeyboardAccelerator keyboardAccelerator;
            keyboardAccelerator.Key(winrt::VirtualKey::Back);
            keyboardAccelerator.Modifiers(winrt::VirtualKeyModifiers::Windows);
            paneToggleButton.KeyboardAccelerators().Append(keyboardAccelerator);
        }
    }

    if (auto leftNavPaneHeaderContentBorder = GetTemplateChildT<winrt::ContentControl>(c_leftNavPaneHeaderContentBorder, controlProtected))
    {
        m_leftNavPaneHeaderContentBorder.set(leftNavPaneHeaderContentBorder);
    }

    if (auto leftNavPaneCustomContentBorder = GetTemplateChildT<winrt::ContentControl>(c_leftNavPaneCustomContentBorder, controlProtected))
    {
        m_leftNavPaneCustomContentBorder.set(leftNavPaneCustomContentBorder);
    }

    if (auto leftNavFooterContentBorder = GetTemplateChildT<winrt::ContentControl>(c_leftNavFooterContentBorder, controlProtected))
    {
        m_leftNavFooterContentBorder.set(leftNavFooterContentBorder);
    }

    if (auto paneHeaderOnTopPane = GetTemplateChildT<winrt::ContentControl>(c_paneHeaderOnTopPane, controlProtected))
    {
        m_paneHeaderOnTopPane.set(paneHeaderOnTopPane);
    }

    if (auto paneCustomContentOnTopPane = GetTemplateChildT<winrt::ContentControl>(c_paneCustomContentOnTopPane, controlProtected))
    {
        m_paneCustomContentOnTopPane.set(paneCustomContentOnTopPane);
    }

    if (auto paneFooterOnTopPane = GetTemplateChildT<winrt::ContentControl>(c_paneFooterOnTopPane, controlProtected))
    {
        m_paneFooterOnTopPane.set(paneFooterOnTopPane);
    }

    if (auto textBlock = GetTemplateChildT<winrt::TextBlock>(c_paneTitleTextBlock, controlProtected))
    {
        m_paneTitleTextBlock.set(textBlock);
        UpdatePaneTitleMargins();
    }
    
    // Get a pointer to the root SplitView
    if (auto splitView = GetTemplateChildT<winrt::SplitView>(c_rootSplitViewName, controlProtected))
    {
        m_rootSplitView.set(splitView);
        m_splitViewIsPaneOpenChangedRevoker = RegisterPropertyChanged(splitView,
            winrt::SplitView::IsPaneOpenProperty(), 
            { this, &NavigationView::OnSplitViewClosedCompactChanged });

        m_splitViewDisplayModeChangedRevoker = RegisterPropertyChanged(splitView,
            winrt::SplitView::DisplayModeProperty(), 
            { this, &NavigationView::OnSplitViewClosedCompactChanged });

        if (SharedHelpers::IsRS3OrHigher()) // These events are new to RS3/v5 API
        {
            m_splitViewPaneClosedRevoker = splitView.PaneClosed(winrt::auto_revoke, { this, &NavigationView::OnSplitViewPaneClosed });
            m_splitViewPaneClosingRevoker = splitView.PaneClosing(winrt::auto_revoke, { this, &NavigationView::OnSplitViewPaneClosing });
            m_splitViewPaneOpenedRevoker = splitView.PaneOpened(winrt::auto_revoke, { this, &NavigationView::OnSplitViewPaneOpened });
            m_splitViewPaneOpeningRevoker = splitView.PaneOpening(winrt::auto_revoke, { this, &NavigationView::OnSplitViewPaneOpening });
        }

        UpdateIsClosedCompact();
    }

    // Change code to NOT do this if we're in top nav mode, to prevent it from being realized:
    if (auto leftNavListView = GetTemplateChildT<winrt::ListView>(c_menuItemsHost, controlProtected))
    {
        m_leftNavListView.set(leftNavListView);

        m_leftNavListViewLoadedRevoker = leftNavListView.Loaded(winrt::auto_revoke, { this, &NavigationView::OnLoaded });

        m_leftNavListViewSelectionChangedRevoker = leftNavListView.SelectionChanged(winrt::auto_revoke, { this, &NavigationView::OnSelectionChanged });
        m_leftNavListViewItemClickRevoker = leftNavListView.ItemClick(winrt::auto_revoke, { this, &NavigationView::OnItemClick });

        SetNavigationViewListPosition(leftNavListView, NavigationViewListPosition::LeftNav);

        // Set up the ViewModel for the List
        auto leftNavigationViewList = leftNavListView.try_as<NavigationViewList>();
        leftNavigationViewList->ListViewModel(winrt::make_self<ViewModel>());
        auto viewModel = leftNavigationViewList->ListViewModel();
        viewModel->IsContentMode(true);
        viewModel->PrepareView(m_rootNode.get());
        viewModel->SetOwningList(leftNavListView);
        leftNavigationViewList->ItemsSource(*viewModel.get());
        //viewModel->NodeExpanding({ this, &TreeView::OnNodeExpanding });
        //viewModel->NodeCollapsed({ this, &TreeView::OnNodeCollapsed });
    }

    // Change code to NOT do this if we're in left nav mode, to prevent it from being realized:
    if (auto topNavListView = GetTemplateChildT<winrt::ListView>(c_topNavMenuItemsHost, controlProtected))
    {
        m_topNavListView.set(topNavListView);

        m_topNavListViewLoadedRevoker = topNavListView.Loaded(winrt::auto_revoke, { this, &NavigationView::OnLoaded });

        m_topNavListViewSelectionChangedRevoker = topNavListView.SelectionChanged(winrt::auto_revoke, { this, &NavigationView::OnSelectionChanged });
        m_topNavListViewItemClickRevoker = topNavListView.ItemClick(winrt::auto_revoke, { this, &NavigationView::OnItemClick });

        SetNavigationViewListPosition(topNavListView, NavigationViewListPosition::TopPrimary);
    }

    // Change code to NOT do this if we're in left nav mode, to prevent it from being realized:
    if (auto topNavListOverflowView = GetTemplateChildT<winrt::ListView>(c_topNavMenuItemsOverflowHost, controlProtected))
    {
        m_topNavListOverflowView.set(topNavListOverflowView);
        m_topNavListOverflowViewSelectionChangedRevoker = topNavListOverflowView.SelectionChanged(winrt::auto_revoke, { this, &NavigationView::OnOverflowItemSelectionChanged });

        SetNavigationViewListPosition(topNavListOverflowView, NavigationViewListPosition::TopOverflow);
    }

    if (auto topNavOverflowButton = GetTemplateChildT<winrt::Button>(c_topNavOverflowButton, controlProtected))
    {
        m_topNavOverflowButton.set(topNavOverflowButton);
        winrt::AutomationProperties::SetName(topNavOverflowButton, ResourceAccessor::GetLocalizedStringResource(SR_NavigationOverflowButtonText));
        topNavOverflowButton.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_NavigationOverflowButtonText)));
        auto visual = winrt::ElementCompositionPreview::GetElementVisual(topNavOverflowButton);
        CreateAndAttachHeaderAnimation(visual);

#ifdef USE_INSIDER_SDK
        if (winrt::IFlyoutBase6 topNavOverflowButtonAsFlyoutBase6 = topNavOverflowButton.Flyout())
        {
            topNavOverflowButtonAsFlyoutBase6.ShouldConstrainToRootBounds(false);
        }
#endif
    }

    if (auto topNavGrid = GetTemplateChildT<winrt::Grid>(c_topNavGrid, controlProtected))
    {
        m_topNavGrid.set(topNavGrid);
    }

    if (auto topNavContentOverlayAreaGrid = GetTemplateChildT<winrt::Border>(c_topNavContentOverlayAreaGrid, controlProtected))
    {
        m_topNavContentOverlayAreaGrid.set(topNavContentOverlayAreaGrid);
    }

    if (auto leftNavSearchContentControl = GetTemplateChildT<winrt::ContentControl>(c_leftNavPaneAutoSuggestBoxPresenter, controlProtected))
    {
        m_leftNavPaneAutoSuggestBoxPresenter.set(leftNavSearchContentControl);
    }

    if (auto topNavSearchContentControl = GetTemplateChildT<winrt::ContentControl>(c_topNavPaneAutoSuggestBoxPresenter, controlProtected))
    {
        m_topNavPaneAutoSuggestBoxPresenter.set(topNavSearchContentControl);
    }

    // Get pointer to the pane content area, for use in the selection indicator animation
    m_paneContentGrid.set(GetTemplateChildT<winrt::UIElement>(c_paneContentGridName, controlProtected));

    // Set automation name on search button
    if (auto button = GetTemplateChildT<winrt::Button>(c_searchButtonName, controlProtected))
    {
        m_paneSearchButton.set(button);
        m_paneSearchButtonClickRevoker = button.Click(winrt::auto_revoke, { this, &NavigationView::OnPaneSearchButtonClick });

        auto searchButtonName = ResourceAccessor::GetLocalizedStringResource(SR_NavigationViewSearchButtonName);
        winrt::AutomationProperties::SetName(button, searchButtonName);
        auto toolTip = winrt::ToolTip();
        toolTip.Content(box_value(searchButtonName));
        winrt::ToolTipService::SetToolTip(button, toolTip);
    }

    if (auto backButton = GetTemplateChildT<winrt::Button>(c_navViewBackButton, controlProtected))
    {
        m_backButton.set(backButton);
        m_backButtonClickedRevoker = backButton.Click(winrt::auto_revoke, { this, &NavigationView::OnBackButtonClicked });
        
        winrt::hstring navigationName = ResourceAccessor::GetLocalizedStringResource(SR_NavigationBackButtonName);
        winrt::AutomationProperties::SetName(backButton, navigationName);
    }

    if (auto backButtonToolTip = GetTemplateChildT<winrt::ToolTip>(c_navViewBackButtonToolTip, controlProtected))
    {
        winrt::hstring navigationBackButtonToolTip = ResourceAccessor::GetLocalizedStringResource(SR_NavigationBackButtonToolTip);
        backButtonToolTip.Content(box_value(navigationBackButtonToolTip));
    }

    if (SharedHelpers::IsRS2OrHigher())
    {
        // Get hold of the outermost grid and enable XYKeyboardNavigationMode
        // However, we only want this to work in the content pane + the hamburger button (which is not inside the splitview)
        // so disable it on the grid in the content area of the SplitView
        if (auto rootGrid = GetTemplateChildT<winrt::Grid>(c_rootGridName, controlProtected))
        {
            rootGrid.XYFocusKeyboardNavigation(winrt::XYFocusKeyboardNavigationMode::Enabled);
        }

        if (auto contentGrid = GetTemplateChildT<winrt::Grid>(c_contentGridName, controlProtected))
        {
            contentGrid.XYFocusKeyboardNavigation(winrt::XYFocusKeyboardNavigationMode::Disabled);
        }
    }

    // Since RS5, SingleSelectionFollowsFocus is set by XAML other than by code
    if (SharedHelpers::IsRS1OrHigher() && ShouldPreserveNavigationViewRS4Behavior() && m_leftNavListView)
    {
        m_leftNavListView.get().SingleSelectionFollowsFocus(false);
    }

    m_accessKeyInvokedRevoker = AccessKeyInvoked(winrt::auto_revoke, { this, &NavigationView::OnAccessKeyInvoked });

    if (SharedHelpers::IsThemeShadowAvailable())
    {
#ifdef USE_INTERNAL_SDK
        if (auto splitView = m_rootSplitView.get())
        {
            if (auto contentRoot = splitView.Content())
            {
                if (auto paneRoot = splitView.Pane())
                {
                    winrt::ThemeShadow shadow;
                    shadow.Receivers().Append(contentRoot);
                    paneRoot.Shadow(shadow);
                }
            }
        }
#endif
    }

    m_appliedTemplate = true;

    // Do initial setup
    UpdatePaneDisplayMode();    
    UpdateHeaderVisibility();
    UpdateTitleBarPadding();
    UpdatePaneTabFocusNavigation();
    UpdateBackButtonVisibility();
    UpdateSingleSelectionFollowsFocusTemplateSetting();
    UpdateNavigationViewUseSystemVisual();
    PropagateNavigationViewAsParent();
    UpdateVisualState();
}

// Hook up the Settings Item Invoked event listener
void NavigationView::CreateAndHookEventsToSettings(std::wstring_view settingsName)
{
    winrt::IControlProtected controlProtected = *this;
    auto settingsItem = GetTemplateChildT<winrt::NavigationViewItem>(settingsName, controlProtected);
    if (settingsItem && settingsItem != m_settingsItem.get())
     {
        // If the old settings item is selected, move the selection to the new one.
        auto selectedItem = SelectedItem();
        bool shouldSelectSetting = selectedItem && IsSettingsItem(selectedItem);

        if (shouldSelectSetting)
        { 
            SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(nullptr);
        }

        m_settingsItemTappedRevoker.revoke();
        m_settingsItemKeyDownRevoker.revoke();
        m_settingsItemKeyUpRevoker.revoke();

        m_settingsItem.set(settingsItem);
        m_settingsItemTappedRevoker = settingsItem.Tapped(winrt::auto_revoke, { this, &NavigationView::OnSettingsTapped });
        m_settingsItemKeyDownRevoker = settingsItem.KeyDown(winrt::auto_revoke, { this, &NavigationView::OnSettingsKeyDown });
        m_settingsItemKeyUpRevoker = settingsItem.KeyUp(winrt::auto_revoke, { this, &NavigationView::OnSettingsKeyUp });

        // Do localization for settings item label and Automation Name
        auto localizedSettingsName = ResourceAccessor::GetLocalizedStringResource(SR_SettingsButtonName);
        winrt::AutomationProperties::SetName(settingsItem, localizedSettingsName);
        UpdateSettingsItemToolTip();

        // Add the name only in case of horizontal nav
        if (!IsTopNavigationView())
        {
            settingsItem.Content(box_value(localizedSettingsName));
        }

        // hook up SettingsItem
        SetValue(s_SettingsItemProperty, settingsItem);

        if (shouldSelectSetting)
        { 
            SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(m_settingsItem.get());
        }
    }
}

// Unlike other control, NavigationView only move items into/out of overflow on MeasureOverride. 
// and the actual measure is done by __super::MeasureOverride.
// We can't move items in LayoutUpdated or OnLoaded, otherwise it would trig another MeasureOverride.
// Because of Items Container restriction, apps may crash if we move the same item out of overflow, 
// and then move it back to overflow in the same measureoveride(busy, unlink failure, in transition...).
// TopNavigationViewLayoutState is used to guarantee above will not happen
// 
// Because of ItemsStackPanel and overflow, we need to run MeasureOverride multiple times. RequestInvalidateMeasureOnNextLayoutUpdate is helping with this.  
// Here is a typical scenario:
//  MeasureOverride(RequestInvalidateMeasureOnNextLayoutUpdate and register LayoutUpdated) -> LayoutUpdated(unregister LayoutUpdated) -> InvalidMeasure 
//   -> Another MeasureOverride(register LayoutUpdated) -> LayoutUpdated(unregister LayoutUpdated) -> Done
winrt::Size NavigationView::MeasureOverride(winrt::Size const& availableSize)
{
    if (!ShouldIgnoreMeasureOverride())
    {
        auto scopeGuard = gsl::finally([this]()
        {
            m_shouldIgnoreOverflowItemSelectionChange = false;
            m_shouldIgnoreNextSelectionChange = false;
        });
        m_shouldIgnoreOverflowItemSelectionChange = true;
        m_shouldIgnoreNextSelectionChange = true;

        if (IsTopNavigationView() && IsTopPrimaryListVisible())
        {
            if (availableSize.Width == std::numeric_limits<float>::infinity())
            {
                // We have infinite space, so move all items to primary list
                m_topDataProvider.MoveAllItemsToPrimaryList();
            }
            else
            {
                HandleTopNavigationMeasureOverride(availableSize);

                if (m_topNavigationMode != TopNavigationViewLayoutState::Normal && m_topNavigationMode != TopNavigationViewLayoutState::Overflow)
                {
                    RequestInvalidateMeasureOnNextLayoutUpdate();
                }
#ifdef DEBUG
                if (m_topDataProvider.Size() > 0)
                {
                    // We should always have at least one item in primary.
                    MUX_ASSERT(m_topDataProvider.GetPrimaryItems().Size() > 0);
                }
#endif // DEBUG
            }
        }

        m_layoutUpdatedToken.revoke();
        m_layoutUpdatedToken = LayoutUpdated(winrt::auto_revoke, { this, &NavigationView::OnLayoutUpdated });
    }
    else
    {
        RequestInvalidateMeasureOnNextLayoutUpdate();
    }
    return __super::MeasureOverride(availableSize);
}

void NavigationView::OnLayoutUpdated(const winrt::IInspectable& sender, const winrt::IInspectable& e)
{
    // We only need to handle once after MeasureOverride, so revoke the token.
    m_layoutUpdatedToken.revoke();

    if (m_shouldInvalidateMeasureOnNextLayoutUpdate)
    {
        m_shouldInvalidateMeasureOnNextLayoutUpdate = false;
        InvalidateMeasure();
    }
    else
    {
        // For some unknown reason, ListView may not always selected a item on the first time when we update the datasource.
        // If it's not selected, we re-selected it.
        auto selectedItem = SelectedItem();
        if (selectedItem)
        {
            auto container = NavigationViewItemOrSettingsContentFromData(selectedItem);
            if (container && !container.IsSelected() && container.SelectsOnInvoked())
            {
                container.IsSelected(true);

            }
        }

        // In topnav, when an item in overflow menu is clicked, the animation is delayed because that item is not move to primary list yet.
        // And it depends on LayoutUpdated to re-play the animation. m_lastSelectedItemPendingAnimationInTopNav is the last selected overflow item.
        if (auto lastSelectedItemInTopNav = m_lastSelectedItemPendingAnimationInTopNav.get())
        {
            AnimateSelectionChanged(lastSelectedItemInTopNav, selectedItem);
        }
        else
        {
            AnimateSelectionChanged(nullptr, selectedItem);
        }
    }
}

void NavigationView::OnSizeChanged(winrt::IInspectable const& /*sender*/, winrt::SizeChangedEventArgs const& args)
{
    auto width = args.NewSize().Width;
    UpdateAdaptiveLayout(width);
    UpdateTitleBarPadding();
    UpdateBackButtonVisibility();
    UpdatePaneTitleMargins();
}

// forceSetDisplayMode: On first call to SetDisplayMode, force setting to initial values
void NavigationView::UpdateAdaptiveLayout(double width, bool forceSetDisplayMode)
{
    // In top nav, there is no adaptive pane layout
    if (IsTopNavigationView())
    {
        return;
    }

    if (!m_rootSplitView)
    {
        return;
    }

    // If we decide we want it to animate open/closed when you resize the
    // window we'll have to change how we figure out the initial state
    // instead of this:
    m_initialListSizeStateSet = false; // see UpdateIsClosedCompact()

    winrt::NavigationViewDisplayMode displayMode = winrt::NavigationViewDisplayMode::Compact;

    auto paneDisplayMode = PaneDisplayMode();
    if (paneDisplayMode == winrt::NavigationViewPaneDisplayMode::Auto)
    {
        if (width >= ExpandedModeThresholdWidth())
        {
            displayMode = winrt::NavigationViewDisplayMode::Expanded;
        }
        else if (width < CompactModeThresholdWidth())
        {
            displayMode = winrt::NavigationViewDisplayMode::Minimal;
        }
    }
    else if (paneDisplayMode == winrt::NavigationViewPaneDisplayMode::Left)
    {
        displayMode = winrt::NavigationViewDisplayMode::Expanded;
    }
    else if (paneDisplayMode == winrt::NavigationViewPaneDisplayMode::LeftCompact)
    {
        displayMode = winrt::NavigationViewDisplayMode::Compact;
    }
    else if (paneDisplayMode == winrt::NavigationViewPaneDisplayMode::LeftMinimal)
    {
        displayMode = winrt::NavigationViewDisplayMode::Minimal;
    }
    else
    {
        MUX_FAIL_FAST();
    }


    SetDisplayMode(displayMode, forceSetDisplayMode);

    if (displayMode == winrt::NavigationViewDisplayMode::Expanded)
    {
        if (!m_wasForceClosed)
        {
            OpenPane();
        }
    }     
}

void NavigationView::OnPaneToggleButtonClick(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    if (IsPaneOpen())
    {
        m_wasForceClosed = true;
        ClosePane();
    }
    else
    {
        m_wasForceClosed = false;
        OpenPane();
    }
}

void NavigationView::OnPaneSearchButtonClick(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    m_wasForceClosed = false;
    OpenPane();

    if (auto autoSuggestBox = AutoSuggestBox())
    {
        autoSuggestBox.Focus(winrt::FocusState::Keyboard);
    }
}

void NavigationView::OpenPane()
{
    auto scopeGuard = gsl::finally([this]()
    {
        m_isOpenPaneForInteraction = false;
    });
    m_isOpenPaneForInteraction = true;
    IsPaneOpen(true);
}

// Call this when you want an uncancellable close
void NavigationView::ClosePane()
{
    auto scopeGuard = gsl::finally([this]()
    {
        m_isOpenPaneForInteraction = false;
    });
    m_isOpenPaneForInteraction = true;
    IsPaneOpen(false); // the SplitView is two-way bound to this value 
}

// Call this when NavigationView itself is going to trigger a close
// where you will stop the close if the cancel is triggered
bool NavigationView::AttemptClosePaneLightly()
{
    bool pendingPaneClosingCancel = false;

    if (SharedHelpers::IsRS3OrHigher())
    {
        auto eventArgs = winrt::make_self<NavigationViewPaneClosingEventArgs>();
        m_paneClosingEventSource(*this, *eventArgs);
        pendingPaneClosingCancel = eventArgs->Cancel();
    }

    if (!pendingPaneClosingCancel || m_wasForceClosed)
    {
        m_blockNextClosingEvent = true;
        ClosePane();
        return true;
    }

    return false;
}

void NavigationView::OnSplitViewClosedCompactChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    if (args == winrt::SplitView::IsPaneOpenProperty() ||
        args == winrt::SplitView::DisplayModeProperty())
    {
        UpdateIsClosedCompact();
    }
}

void NavigationView::OnSplitViewPaneClosed(const winrt::DependencyObject& /*sender*/, const winrt::IInspectable& obj)
{
    m_paneClosedEventSource(*this, nullptr);
}

void NavigationView::OnSplitViewPaneClosing(const winrt::DependencyObject& /*sender*/, const winrt::SplitViewPaneClosingEventArgs& args)
{
    bool pendingPaneClosingCancel = false;
    if (m_paneClosingEventSource)
    {
        if (!m_blockNextClosingEvent) // If this is true, we already sent one out "manually" and don't need to forward SplitView's event
        {
            auto eventArgs = winrt::make_self<NavigationViewPaneClosingEventArgs>();
            eventArgs->SplitViewClosingArgs(args);
            m_paneClosingEventSource(*this, *eventArgs);
            pendingPaneClosingCancel = eventArgs->Cancel();
        }
        else
        {
            m_blockNextClosingEvent = false;
        }
    }

    if (!pendingPaneClosingCancel) // will be set in above event!
    {
        if (auto splitView = m_rootSplitView.get())
        {
            if (auto paneList = m_leftNavListView)
            {
                if (splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactOverlay || splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactInline)
                {
                    // See UpdateIsClosedCompact 'RS3+ animation timing enhancement' for explanation:
                    winrt::VisualStateManager::GoToState(*this, L"ListSizeCompact", true /*useTransitions*/);
                    UpdatePaneToggleSize();
                }
            }
        }
    }
}

void NavigationView::OnSplitViewPaneOpened(const winrt::DependencyObject& /*sender*/, const winrt::IInspectable& obj)
{
    m_paneOpenedEventSource(*this, nullptr);
}

void NavigationView::OnSplitViewPaneOpening(const winrt::DependencyObject& /*sender*/, const winrt::IInspectable& obj)
{
    if (m_leftNavListView)
    {
        // See UpdateIsClosedCompact 'RS3+ animation timing enhancement' for explanation:
        winrt::VisualStateManager::GoToState(*this, L"ListSizeFull", true /*useTransitions*/);
    }

    m_paneOpeningEventSource(*this, nullptr);
}

void NavigationView::UpdateIsClosedCompact()
{
    if (auto splitView = m_rootSplitView.get())
    {
        // Check if the pane is closed and if the splitview is in either compact mode.
        auto splitViewDisplayMode = splitView.DisplayMode();
        m_isClosedCompact = !splitView.IsPaneOpen() && (splitViewDisplayMode == winrt::SplitViewDisplayMode::CompactOverlay || splitViewDisplayMode == winrt::SplitViewDisplayMode::CompactInline);
        winrt::VisualStateManager::GoToState(*this, m_isClosedCompact ? L"ClosedCompact" : L"NotClosedCompact", true /*useTransitions*/);

        // Set the initial state of the list size
        if (!m_initialListSizeStateSet)
        {
            m_initialListSizeStateSet = true;
            winrt::VisualStateManager::GoToState(*this, m_isClosedCompact ? L"ListSizeCompact" : L"ListSizeFull", true /*useTransitions*/);
        }
        else if (!SharedHelpers::IsRS3OrHigher()) // Do any changes that would otherwise happen on opening/closing for RS2 and earlier:
        {
            // RS3+ animation timing enhancement:
            // Pre-RS3, we didn't have the full suite of Closed, Closing, Opened,
            // Opening events on SplitView. So when doing open/closed operations,
            // we have to do them immediately. Just one example: on RS2 when you
            // close the pane, the PaneTitle will disappear *immediately* which
            // looks janky. But on RS4, it'll have its visibility set after the
            // closed event fires.
            winrt::VisualStateManager::GoToState(*this, m_isClosedCompact ? L"ListSizeCompact" : L"ListSizeFull", true /*useTransitions*/);
        }

        UpdateTitleBarPadding();
        UpdateBackButtonVisibility();
        UpdatePaneTitleMargins();
        UpdatePaneToggleSize();
    }
}

void NavigationView::OnBackButtonClicked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    auto eventArgs = winrt::make_self<NavigationViewBackRequestedEventArgs>();
    m_backRequestedEventSource(*this, *eventArgs);
}

bool NavigationView::IsOverlay()
{
    if (auto splitView = m_rootSplitView.get())
    {
        return splitView.DisplayMode() == winrt::SplitViewDisplayMode::Overlay;
    }
    else
    {
        return false;
    }
}

bool NavigationView::IsLightDismissible()
{
    if (auto splitView = m_rootSplitView.get())
    {
        return splitView.DisplayMode() != winrt::SplitViewDisplayMode::Inline && splitView.DisplayMode() != winrt::SplitViewDisplayMode::CompactInline;
    }
    else
    {
        return false;
    }
}

bool NavigationView::ShouldShowBackButton()
{
    if (m_backButton && !ShouldPreserveNavigationViewRS3Behavior())
    {
        if (DisplayMode() == winrt::NavigationViewDisplayMode::Minimal && IsPaneOpen())
        {
            return false;
        }

        auto visibility = IsBackButtonVisible();
        return (visibility == winrt::NavigationViewBackButtonVisible::Visible || (visibility == winrt::NavigationViewBackButtonVisible::Auto && !SharedHelpers::IsOnXbox()));
    }

    return false;
}

// The automation name and tooltip for the pane toggle button changes depending on whether it is open or closed
// put the logic here as it will be called in a couple places
void NavigationView::SetPaneToggleButtonAutomationName()
{
    winrt::hstring navigationName;
    if (IsPaneOpen())
    {
        navigationName = ResourceAccessor::GetLocalizedStringResource(SR_NavigationButtonOpenName);
    }
    else
    {
        navigationName = ResourceAccessor::GetLocalizedStringResource(SR_NavigationButtonClosedName);
    }

    if (auto paneToggleButton = m_paneToggleButton.get())
    {
        winrt::AutomationProperties::SetName(paneToggleButton, navigationName);
        auto toolTip = winrt::ToolTip();
        toolTip.Content(box_value(navigationName));
        winrt::ToolTipService::SetToolTip(paneToggleButton, toolTip);
    }
}

void NavigationView::UpdateSettingsItemToolTip()
{
    if (auto settingsItem = m_settingsItem.get())
    {
        if (!IsTopNavigationView() && IsPaneOpen())
        {
            winrt::ToolTipService::SetToolTip(settingsItem, nullptr);
        }
        else
        {
            auto localizedSettingsName = ResourceAccessor::GetLocalizedStringResource(SR_SettingsButtonName);
            auto toolTip = winrt::ToolTip();
            toolTip.Content(box_value(localizedSettingsName));
            winrt::ToolTipService::SetToolTip(settingsItem, toolTip);
        }
    }
}

void NavigationView::OnSettingsTapped(const winrt::IInspectable& /*sender*/, const winrt::TappedRoutedEventArgs& /*args*/)
{
    OnSettingsInvoked();
}

void NavigationView::OnSettingsKeyDown(const winrt::IInspectable& /*sender*/, const winrt::KeyRoutedEventArgs& args)
{
    auto key = args.Key();

    // Because ListViewItem eats the events, we only get these keys on KeyDown.
    if (key == winrt::VirtualKey::Space ||
        key == winrt::VirtualKey::Enter)
    {
        args.Handled(true);
        OnSettingsInvoked();
    }
}

void NavigationView::OnSettingsKeyUp(const winrt::IInspectable& /*sender*/, const winrt::KeyRoutedEventArgs& args)
{
    if (!args.Handled())
    {
        // Because ListViewItem eats the events, we only get these keys on KeyUp.
        if (args.OriginalKey() == winrt::VirtualKey::GamepadA)
        {
            args.Handled(true);
            OnSettingsInvoked();
        }
    }
}

void NavigationView::OnSettingsInvoked()
{
    auto prevItem = SelectedItem();
    auto settingsItem = m_settingsItem.get();
    if (IsSettingsItem(prevItem))
    {
        RaiseItemInvoked(settingsItem, true /*isSettings*/);
    }
    else if (settingsItem)
    {
        SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(settingsItem);
    }
}

winrt::float2 c_frame1point1 = winrt::float2(0.9f, 0.1f);
winrt::float2 c_frame1point2 = winrt::float2(1.0f, 0.2f);
winrt::float2 c_frame2point1 = winrt::float2(0.1f, 0.9f);
winrt::float2 c_frame2point2 = winrt::float2(0.2f, 1.0f);

void NavigationView::AnimateSelectionChangedToItem(const winrt::IInspectable& selectedItem)
{
    if (selectedItem && !IsSelectionSuppressed(selectedItem))
    {
        AnimateSelectionChanged(nullptr /* prevItem */, selectedItem);
    }
}

// Please clear the field m_lastSelectedItemPendingAnimationInTopNav when calling this method to prevent garbage value and incorrect animation
// when the layout is invalidated as it's called in OnLayoutUpdated.
void NavigationView::AnimateSelectionChanged(const winrt::IInspectable& prevItem, const winrt::IInspectable& nextItem)
{  
    winrt::UIElement prevIndicator = FindSelectionIndicator(prevItem);
    winrt::UIElement nextIndicator = FindSelectionIndicator(nextItem);

    bool haveValidAnimation = false;
    // It's possible that AnimateSelectionChanged is called multiple times before the first animation is complete.
    // To have better user experience, if the selected target is the same, keep the first animation
    // If the selected target is not the same, abort the first animation and launch another animation.
    if (m_prevIndicator || m_nextIndicator) // There is ongoing animation
    {
        if (nextIndicator && m_nextIndicator.get() == nextIndicator) // animate to the same target, just wait for animation complete
        {
            if (prevIndicator && prevIndicator != m_prevIndicator.get())
            {
                ResetElementAnimationProperties(prevIndicator, 0.0f);
            }
            haveValidAnimation = true;
        } 
        else
        {
            // If the last animation is still playing, force it to complete.
            OnAnimationComplete(nullptr, nullptr);
        }
    }

    if (!haveValidAnimation)
    {
        winrt::UIElement paneContentGrid = m_paneContentGrid.get();

        if ((prevItem != nextItem) && paneContentGrid && prevIndicator && nextIndicator && SharedHelpers::IsAnimationsEnabled())
        {
            // Make sure both indicators are visible and in their original locations
            ResetElementAnimationProperties(prevIndicator, 1.0f);
            ResetElementAnimationProperties(nextIndicator, 1.0f);

            // get the item positions in the pane
            winrt::Point point = winrt::Point(0, 0);
            float prevPos;
            float nextPos;

            winrt::Point prevPosPoint = prevIndicator.TransformToVisual(paneContentGrid).TransformPoint(point);
            winrt::Point nextPosPoint = nextIndicator.TransformToVisual(paneContentGrid).TransformPoint(point);
            winrt::Size prevSize = prevIndicator.RenderSize();
            winrt::Size nextSize = nextIndicator.RenderSize();

            if (IsTopNavigationView())
            {
                prevPos = prevPosPoint.X;
                nextPos = nextPosPoint.X;
            }
            else
            {
                prevPos = prevPosPoint.Y;
                nextPos = nextPosPoint.Y;
            }

            winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
            winrt::CompositionScopedBatch scopedBatch = visual.Compositor().CreateScopedBatch(winrt::CompositionBatchTypes::Animation);

            // Play the animation on both the previous and next indicators
            PlayIndicatorAnimations(prevIndicator, 0, nextPos - prevPos, prevSize, nextSize, true);
            PlayIndicatorAnimations(nextIndicator, prevPos - nextPos, 0, prevSize, nextSize, false);

            scopedBatch.End();
            m_prevIndicator.set(prevIndicator);
            m_nextIndicator.set(nextIndicator);

            auto strongThis = get_strong();
            scopedBatch.Completed(
                [strongThis](auto sender, auto args)
            {
                strongThis->OnAnimationComplete(sender, args);
            });
        }
        else
        {
            // if all else fails, or if animations are turned off, attempt to correctly set the positions and opacities of the indicators.
            ResetElementAnimationProperties(prevIndicator, 0.0f);
            ResetElementAnimationProperties(nextIndicator, 1.0f);
        }

        if (m_lastSelectedItemPendingAnimationInTopNav.get())
        {
            // if nextItem && !nextIndicator, that means a item from topnav flyout is selected, and we delay the animation to LayoutUpdated.
            // nextIndicator is null because we have problem to get the selectionindicator since it's not in primary list yet.
            // Otherwise we already done the animation and clear m_lastSelectedItemPendingAnimationInTopNav.
            if (!(nextItem && !nextIndicator))
            {
                m_lastSelectedItemPendingAnimationInTopNav.set(nullptr);
            }
        }
    }
}

void NavigationView::PlayIndicatorAnimations(const winrt::UIElement& indicator, float from, float to, winrt::Size beginSize, winrt::Size endSize, bool isOutgoing)
{
    winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(indicator);
    winrt::Compositor comp = visual.Compositor();

    winrt::Size size = indicator.RenderSize();
    float dimension = IsTopNavigationView() ? size.Width : size.Height;

    float beginScale = 1.0f;
    float endScale = 1.0f;
    if (IsTopNavigationView() && fabs(size.Width) > 0.001f)
    {
        beginScale = beginSize.Width / size.Width;
        endScale = endSize.Width / size.Width;
    }

    winrt::StepEasingFunction singleStep = comp.CreateStepEasingFunction();
    singleStep.IsFinalStepSingleFrame(true);

    if (isOutgoing)
    {
        // fade the outgoing indicator so it looks nice when animating over the scroll area
        winrt::ScalarKeyFrameAnimation opacityAnim = comp.CreateScalarKeyFrameAnimation();
        opacityAnim.InsertKeyFrame(0.0f, 1.0);
        opacityAnim.InsertKeyFrame(0.333f, 1.0, singleStep);
        opacityAnim.InsertKeyFrame(1.0f, 0.0, comp.CreateCubicBezierEasingFunction(c_frame2point1, c_frame2point2));
        opacityAnim.Duration(600ms);

        visual.StartAnimation(L"Opacity", opacityAnim);
    }

    winrt::ScalarKeyFrameAnimation posAnim = comp.CreateScalarKeyFrameAnimation();
    posAnim.InsertKeyFrame(0.0f, from < to ? from : (from + (dimension * (beginScale - 1))));
    posAnim.InsertKeyFrame(0.333f, from < to ? (to + (dimension * (endScale - 1))) : to, singleStep);
    posAnim.Duration(600ms);

    winrt::ScalarKeyFrameAnimation scaleAnim = comp.CreateScalarKeyFrameAnimation();
    scaleAnim.InsertKeyFrame(0.0f, beginScale);
    scaleAnim.InsertKeyFrame(0.333f, abs(to - from) / dimension + (from < to ? endScale : beginScale), comp.CreateCubicBezierEasingFunction(c_frame1point1, c_frame1point2));
    scaleAnim.InsertKeyFrame(1.0f, endScale, comp.CreateCubicBezierEasingFunction(c_frame2point1, c_frame2point2));
    scaleAnim.Duration(600ms);

    winrt::ScalarKeyFrameAnimation centerAnim = comp.CreateScalarKeyFrameAnimation();
    centerAnim.InsertKeyFrame(0.0f, from < to ? 0.0f : dimension);
    centerAnim.InsertKeyFrame(1.0f, from < to ? dimension : 0.0f, singleStep);
    centerAnim.Duration(200ms);

    if (IsTopNavigationView())
    {
        visual.StartAnimation(L"Offset.X", posAnim);
        visual.StartAnimation(L"Scale.X", scaleAnim);
        visual.StartAnimation(L"CenterPoint.X", centerAnim);
    }
    else
    {
        visual.StartAnimation(L"Offset.Y", posAnim);
        visual.StartAnimation(L"Scale.Y", scaleAnim);
        visual.StartAnimation(L"CenterPoint.Y", centerAnim);
    }
}

void NavigationView::OnAnimationComplete(const winrt::IInspectable& /*sender*/, const winrt::CompositionBatchCompletedEventArgs& /*args*/)
{
    auto indicator = m_prevIndicator.get();
    ResetElementAnimationProperties(indicator, 0.0f);
    m_prevIndicator.set(nullptr);

    indicator = m_nextIndicator.get();
    ResetElementAnimationProperties(indicator, 1.0f);
    m_nextIndicator.set(nullptr);
}

void NavigationView::ResetElementAnimationProperties(const winrt::UIElement& element, float desiredOpacity)
{
    if (element)
    {
        element.Opacity(desiredOpacity);

        winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(element);

        if (visual)
        {
            visual.Offset(winrt::float3(0.0f, 0.0f, 0.0f));
            visual.Scale(winrt::float3(1.0f, 1.0f, 1.0f));
            visual.Opacity(desiredOpacity);
        }
    }
}

winrt::NavigationViewItemBase NavigationView::NavigationViewItemBaseOrSettingsContentFromData(const winrt::IInspectable& data)
{
    return GetContainerForData<winrt::NavigationViewItemBase>(data);
}

winrt::NavigationViewItem NavigationView::NavigationViewItemOrSettingsContentFromData(const winrt::IInspectable& data)
{
    return GetContainerForData<winrt::NavigationViewItem>(data);
}

bool NavigationView::IsSelectionSuppressed(const winrt::IInspectable& item)
{
    if (item)
    {
        if (auto nvi = NavigationViewItemOrSettingsContentFromData(item))
        {
            return !winrt::get_self<NavigationViewItem>(nvi)->SelectsOnInvoked();
        }
    }

    return false;
}

bool NavigationView::ShouldPreserveNavigationViewRS4Behavior()
{
    // Since RS5, we support topnav
    return !m_topNavGrid;
}

bool NavigationView::ShouldPreserveNavigationViewRS3Behavior()
{
    // Since RS4, we support backbutton
    return !m_backButton;
}

winrt::UIElement NavigationView::FindSelectionIndicator(const winrt::IInspectable& item)
{
    if (item)
    {
        if (auto nvi = NavigationViewItemOrSettingsContentFromData(item))
        {
            return winrt::get_self<NavigationViewItem>(nvi)->GetSelectionIndicator();
        }
    }

    return nullptr;
}

//SFF = SelectionFollowsFocus 
//SOI = SelectsOnInvoked
//
//                  !SFF&SOI     SFF&SOI     !SFF&&!SOI     SFF&&!SOI
//ItemInvoke        FIRE         FIRE        FIRE         FIRE
//SelectionChanged  FIRE         FIRE        DO NOT FIRE  DO NOT FIRE

//If OnItemClick
//  If SelectsOnInvoked and previous item == new item, raise OnItemInvoked(same item would not have select change event)
//  else let SelectionChanged to raise OnItemInvoked event
//If SelectionChanged, it changes SelectedItem -> OnPropertyChange -> ChangeSelection. On ChangeSelection:
//  If !SelectsOnInvoked for new item. Undo the selection.
//  If SelectsOnInvoked, raise OnItemInvoked(if not from API), then raise SelectionChanged.
void NavigationView::OnSelectionChanged(const winrt::IInspectable& /*sender*/, const winrt::SelectionChangedEventArgs& args)
{
    if (!m_shouldIgnoreNextSelectionChange)
    {
        winrt::IInspectable prevItem{ nullptr };
        winrt::IInspectable nextItem{ nullptr };

        if (args.RemovedItems().Size() > 0)
        {
            prevItem = args.RemovedItems().GetAt(0);
        }

        if (args.AddedItems().Size() > 0)
        {
            nextItem = args.AddedItems().GetAt(0);
        }

        if (prevItem && !nextItem && !IsSettingsItem(prevItem)) // try to unselect an item but it's not allowed
        {
            // Aways keep one item is selected except Settings

            // So you're wondering - wait if the menu was previously selected, how can
            // the removed item not be a NavigationViewItem? Well, if you say clear a
            // NavigationView of MenuItems() and replace it with MenuItemsSource() full
            // of strings, you may end up in this state which necessitates the following
            // check:
            if (auto itemAsNVI = prevItem.try_as<winrt::NavigationViewItem>())
            {
                itemAsNVI.IsSelected(true);
            }
        }
        else
        {
            SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(nextItem);
        }
    }
}

void NavigationView::OnOverflowItemSelectionChanged(const winrt::IInspectable& /*sender*/, const winrt::SelectionChangedEventArgs& args)
{   
    // SelectOverflowItem is moving data in/out of overflow. it caused another round of OnOverflowItemSelectionChanged
    // also in MeasureOverride, it may raise OnOverflowItemSelectionChanged.
    // Ignore it if it's m_isHandleOverflowItemClick or m_isMeasureOverriding;
    if (!m_shouldIgnoreNextMeasureOverride && !m_shouldIgnoreOverflowItemSelectionChange)
    {
        auto scopeGuard = gsl::finally([this]()
        {
            m_shouldIgnoreNextMeasureOverride = false;
            m_selectionChangeFromOverflowMenu = false;
        });
        m_shouldIgnoreNextMeasureOverride = true;
        m_selectionChangeFromOverflowMenu = true;

        if (args.AddedItems().Size() > 0)
        {
            auto nextItem = args.AddedItems().GetAt(0);
            if (nextItem)
            {
                CloseTopNavigationViewFlyout();

                if (!IsSelectionSuppressed(nextItem))
                {
                    SelectOverflowItem(nextItem);
                }
                else
                {
                    RaiseItemInvoked(nextItem, false /*isSettings*/);
                }
            }           
        }
    }
}

void NavigationView::RaiseSelectionChangedEvent(winrt::IInspectable const& nextItem, bool isSettingsItem, NavigationRecommendedTransitionDirection recommendedDirection)
{
    auto eventArgs = winrt::make_self<NavigationViewSelectionChangedEventArgs>();
    eventArgs->SelectedItem(nextItem);
    eventArgs->IsSettingsSelected(isSettingsItem);
    if (auto container = NavigationViewItemBaseOrSettingsContentFromData(nextItem))
    {
        eventArgs->SelectedItemContainer(container);
    }
    eventArgs->RecommendedNavigationTransitionInfo(CreateNavigationTransitionInfo(recommendedDirection));
    m_selectionChangedEventSource(*this, *eventArgs);
}
 
// SelectedItem change can be invoked by API or user's action like clicking. if it's not from API, m_shouldRaiseInvokeItemInSelectionChange would be true
// If nextItem is selectionsuppressed, we should undo the selection. We didn't undo it OnSelectionChange because we want change by API has the same undo logic.
void NavigationView::ChangeSelection(const winrt::IInspectable& prevItem, const winrt::IInspectable& nextItem)
{
    auto nextActualItem = nextItem;
    if (!m_shouldIgnoreNextSelectionChange)
    {
        auto scopeGuard = gsl::finally([this]()
        {
            m_shouldIgnoreNextSelectionChange = false;
        });
        m_shouldIgnoreNextSelectionChange = true;

        bool isSettingsItem = IsSettingsItem(nextActualItem);

        bool isSelectionSuppressed = IsSelectionSuppressed(nextActualItem);
        if (isSelectionSuppressed)
        {
            UndoSelectionAndRevertSelectionTo(prevItem, nextActualItem);
            
            // Undo only happened when customer clicked a selectionsuppressed item. 
            // To simplify the logic, OnItemClick didn't raise the event and it's been delayed to here.
            RaiseItemInvoked(nextActualItem, isSettingsItem);

            auto container = NavigationViewItemOrSettingsContentFromData(nextActualItem);
            if (container)
            {
                ToggleIsExpanded(container);
            }
        }
        else
        {
            // Other transition other than default only apply to topnav
            // when clicking overflow on topnav, transition is from bottom
            // otherwise if prevItem is on left side of nextActualItem, transition is from left
            //           if prevItem is on right side of nextActualItem, transition is from right
            // click on Settings item is considered Default
            NavigationRecommendedTransitionDirection recommendedDirection = NavigationRecommendedTransitionDirection::Default;
            if (IsTopNavigationView())
            {
                if (m_selectionChangeFromOverflowMenu)
                {
                    recommendedDirection = NavigationRecommendedTransitionDirection::FromOverflow;
                }
                else if (!isSettingsItem && prevItem && nextActualItem)
                {
                    recommendedDirection = GetRecommendedTransitionDirection(NavigationViewItemBaseOrSettingsContentFromData(prevItem),
                        NavigationViewItemBaseOrSettingsContentFromData(nextActualItem));
                }
            }

            // Bug 17850504, Customer may use NavigationViewItem.IsSelected in ItemInvoke or SelectionChanged Event.
            // To keep the logic the same as RS4, ItemInvoke is before unselect the old item
            // And SelectionChanged is after we selected the new item.
            {
                if (m_shouldRaiseInvokeItemInSelectionChange)
                {
                    RaiseItemInvoked(nextActualItem, isSettingsItem, nullptr/*container*/, recommendedDirection);

                    // In current implementation, when customer clicked a NavigationViewItem, ListView raised ItemInvoke, and we ignored it
                    // then ListView raised SelectionChange event. And NavigationView listen to this event and raise ItemInvoked, and then SelectionChanged.
                    // This caused a problem that if customer changed SelectedItem in ItemInvoked, ListView.SelectionChanged event doesn't know about it.
                    // So need to see make nextActualItem the same as SelectedItem.
                    auto selectedItem = SelectedItem();
                    if (nextActualItem != selectedItem)
                    {
                        auto invokedItem = nextActualItem;
                        nextActualItem = selectedItem;
                        isSettingsItem = IsSettingsItem(nextActualItem);
                        recommendedDirection = NavigationRecommendedTransitionDirection::Default;

                        // Customer set SelectedItem to null in ItemInvoked event, so we unselect the old selectedItem.
                        if (invokedItem && !nextActualItem)
                        {
                            UnselectPrevItem(invokedItem, nextActualItem);
                        }
                    }
                }
                UnselectPrevItem(prevItem, nextActualItem);

                ChangeSelectStatusForItem(nextActualItem, true /*selected*/);
                RaiseSelectionChangedEvent(nextActualItem, isSettingsItem, recommendedDirection);
            }

            UpdateIsChildSelected(prevItem, nextActualItem);
            auto container = NavigationViewItemOrSettingsContentFromData(nextActualItem);
            if (container)
            {
                // Simply Expanding and Collapsing should never change the selection state of the NavigationView itself (aka the 'SelectedItem' property).
                // In the case where the parent items are selectable, that has already been taken care of before this gets executed.
                auto scopeGuard = gsl::finally([this]()
                {
                    m_shouldIgnoreNextSelectionChange = false;
                });
                m_shouldIgnoreNextSelectionChange = true;
                ToggleIsExpanded(container);
            }

            AnimateSelectionChanged(prevItem, nextActualItem);

            if (IsPaneOpen() && DisplayMode() != winrt::NavigationViewDisplayMode::Expanded)
            {
                ClosePane();
            }
        }
    }
}

void NavigationView::UpdateIsChildSelected(winrt::IInspectable const& prevItem, winrt::IInspectable const& nextItem)
{
    auto lv = GetActiveListView().try_as<winrt::NavigationViewList>();
    auto viewModel = winrt::get_self<NavigationViewList>(lv)->ListViewModel();

    if (prevItem)
    {
        winrt::TreeViewNode prevItemNode{ nullptr };

        if (auto container = lv.ContainerFromItem(prevItem))
        {
            prevItemNode = NodeFromContainer(container);
        }
        else
        {
            prevItemNode = NodeFromPreviouslySelectedItem(prevItem);
        }

        if (auto nodeParent = prevItemNode.Parent())
        {
            viewModel->UpdateSelection(nodeParent, TreeNodeSelectionState::UnSelected);
            viewModel->NotifyContainerOfSelectionChange(nodeParent, TreeNodeSelectionState::UnSelected);
        }
    }

    if (nextItem)
    {
        // The next item being selected must be in the listview
        if (auto container = lv.ContainerFromItem(nextItem))
        {
            auto nextItemNode = NodeFromContainer(container);
            if (auto nodeParent = nextItemNode.Parent())
            {
                viewModel->UpdateSelection(nodeParent, TreeNodeSelectionState::PartialSelected);
                viewModel->NotifyContainerOfSelectionChange(nodeParent, TreeNodeSelectionState::PartialSelected);
            }
        }
    }
}

// This search function is utilizing the assumption that the item we are searching for in the node tree is in the process of being unselected.
// This means that it's ancestors are still in the TreeNodeSelectionState::PartialSelected state.
winrt::TreeViewNode NavigationView::NodeFromPreviouslySelectedItem(winrt::IInspectable const& item)
{
    auto nodeList = RootNodes();

    bool foundItem = false;
    while (!foundItem && nodeList && nodeList.Size() > 0)
    {
        for (uint32_t i = 0; i < nodeList.Size(); i++)
        {
            auto node = nodeList.GetAt(i);

            if (winrt::get_self<TreeViewNode>(node)->Content() == item)
            {
                return node;
            }

            if (winrt::get_self<TreeViewNode>(node)->SelectionState() == TreeNodeSelectionState::PartialSelected)
            {
                nodeList = winrt::get_self<TreeViewNode>(node)->Children();
                break;
            }
        }
    }
    return nullptr;
}


void NavigationView::OnItemClick(const winrt::IInspectable& /*sender*/, const winrt::ItemClickEventArgs& args)
{
    auto clickedItem = args.ClickedItem();

    //TODO: Update container retrieval logic to work with popup listviews
    auto itemContainer = GetContainerForClickedItem(clickedItem);

    auto selectedItem = SelectedItem();

    // TODO: There is bug in the above method of retrieving an item container when using databinding.
    //       For now, retrieving container by bypassing the buggy method.
    // Explanation:
    //      The container retrieval workaround in 'GetContainerForClickedItem' does not work in a
    //      databinding scenario. 'NavigationViewItemBaseOrSettingsContentFromData' doesn't work
    //      in this function in a markup scenario. So we first try the ListView API to retrieve
    //      a container and if that fails, we use the workaround.
    //auto itemContainerForExpanding = NavigationViewItemBaseOrSettingsContentFromData(clickedItem);
    //if (!itemContainerForExpanding)
    //{
    //    itemContainerForExpanding = itemContainer;
    //}

    // If SelectsOnInvoked and previous item(selected item) == new item(clicked item), raise OnItemClicked (same item would not have selectchange event)
    // Others would be invoked by SelectionChanged. Please see ChangeSelection for more details.
    //
    // args.ClickedItem itself is the content of ListViewItem, so it can't be compared directly with SelectedItem or do IsSelectionSuppressed
    // We workaround this by compare the selectedItem.content with clickeditem by DoesSelectedItemContainContent.
    // If selecteditem.content == item, selecteditem is used to deduce the selectionsuppressed flag
    if (!m_shouldIgnoreNextSelectionChange && DoesSelectedItemContainContent(clickedItem, itemContainer) && !IsSelectionSuppressed(selectedItem))
    {
        auto containterContent = itemContainer.Content();
        RaiseItemInvoked(selectedItem, false /*isSettings*/, itemContainer);
        if (auto nviExpanding = itemContainer.try_as<winrt::NavigationViewItem>())
        {
            ToggleIsExpanded(nviExpanding);
        }
    }
}

void NavigationView::ToggleIsExpanded(winrt::NavigationViewItem const& item)
{
    if (item)
    {
        bool hasChildren = (item.MenuItems().Size() > 0 ||
                            item.MenuItemsSource() ||
                            item.HasUnrealizedChildren());
        if (hasChildren)
        {
            auto isItemBeingExpanded = !item.IsExpanded();
            if (isItemBeingExpanded)
            {
                RaiseIsExpanding(item);
            }

            item.IsExpanded(isItemBeingExpanded);

            if (!isItemBeingExpanded)
            {
                RaiseCollapsed(item);
            }
        }
    }
}


void NavigationView::RaiseIsExpanding(winrt::NavigationViewItemBase const& item)
{

}

void NavigationView::RaiseCollapsed(winrt::NavigationViewItemBase const& item)
{

}

void NavigationView::RaiseItemInvoked(winrt::IInspectable const& item,
    bool isSettings,
    winrt::NavigationViewItemBase const& container,
    NavigationRecommendedTransitionDirection recommendedDirection)
{
    auto invokedItem = item;
    auto invokedContainer = container;

    auto eventArgs = winrt::make_self<NavigationViewItemInvokedEventArgs>();

    if (container)
    {
        invokedItem = container.Content();
    }
    else
    {
        // InvokedItem is container for Settings, but Content of item for other ListViewItem
        if (!isSettings)
        {
            if (auto containerFromData = NavigationViewItemBaseOrSettingsContentFromData(item))
            {
                invokedItem = containerFromData.Content();
                invokedContainer = containerFromData;
            }
        }
        else
        {            
            MUX_ASSERT(item);
            invokedContainer = item.try_as<winrt::NavigationViewItemBase>();
            MUX_ASSERT(invokedContainer);
        }
    }
    eventArgs->InvokedItem(invokedItem);
    eventArgs->InvokedItemContainer(invokedContainer);
    eventArgs->IsSettingsInvoked(isSettings);
    eventArgs->RecommendedNavigationTransitionInfo(CreateNavigationTransitionInfo(recommendedDirection));
    m_itemInvokedEventSource(*this, *eventArgs);
}

// forceSetDisplayMode: On first call to SetDisplayMode, force setting to initial values
void NavigationView::SetDisplayMode(const winrt::NavigationViewDisplayMode& displayMode, bool forceSetDisplayMode)
{
    if (forceSetDisplayMode || DisplayMode() != displayMode)
    {
        UpdateVisualStateForDisplayModeGroup(displayMode);

        // Update header visibility based on what the new display mode will be
        UpdateHeaderVisibility(displayMode);

        UpdatePaneTabFocusNavigation();

        UpdatePaneToggleSize();

        RaiseDisplayModeChanged(displayMode);
    }
}

// To support TopNavigationView, DisplayModeGroup in visualstate(We call it VisualStateDisplayMode) is decoupled with DisplayMode.
// The VisualStateDisplayMode is the combination of TopNavigationView, DisplayMode, PaneDisplayMode.
// Here is the mapping:
//    TopNav -> Minimal
//    PaneDisplayMode::Left || (PaneDisplayMode::Auto && DisplayMode::Expanded) -> Expanded
//    PaneDisplayMode::LeftCompact || (PaneDisplayMode::Auto && DisplayMode::Compact) -> Compact
//    Map others to Minimal or MinimalWithBackButton 
NavigationViewVisualStateDisplayMode NavigationView::GetVisualStateDisplayMode(const winrt::NavigationViewDisplayMode& displayMode)
{
    auto paneDisplayMode = PaneDisplayMode();

    if (IsTopNavigationView())
    {
        return NavigationViewVisualStateDisplayMode::Minimal;
    }

    if (paneDisplayMode == winrt::NavigationViewPaneDisplayMode::Left ||
        (paneDisplayMode == winrt::NavigationViewPaneDisplayMode::Auto && displayMode == winrt::NavigationViewDisplayMode::Expanded))
    {
        return NavigationViewVisualStateDisplayMode::Expanded;
    }

    if (paneDisplayMode == winrt::NavigationViewPaneDisplayMode::LeftCompact ||
        (paneDisplayMode == winrt::NavigationViewPaneDisplayMode::Auto && displayMode == winrt::NavigationViewDisplayMode::Compact))
    {
        return NavigationViewVisualStateDisplayMode::Compact;
    }

    // In minimal mode, when the NavView is closed, the HeaderContent doesn't have
    // its own dedicated space, and must 'share' the top of the NavView with the 
    // pane toggle button ('hamburger' button) and the back button.
    if (ShouldShowBackButton())
    {
        return NavigationViewVisualStateDisplayMode::MinimalWithBackButton;
    }
    else
    {
        return NavigationViewVisualStateDisplayMode::Minimal;
    }
}

void NavigationView::UpdateVisualStateForDisplayModeGroup(const winrt::NavigationViewDisplayMode& displayMode)
{
    if (auto splitView = m_rootSplitView.get())
    {
        auto visualStateDisplayMode = GetVisualStateDisplayMode(displayMode);
        auto visualStateName = L"";
        auto splitViewDisplayMode = winrt::SplitViewDisplayMode::Overlay;
        auto visualStateNameMinimal = L"Minimal";

        switch (visualStateDisplayMode)
        {
        case NavigationViewVisualStateDisplayMode::MinimalWithBackButton:
            visualStateName = L"MinimalWithBackButton";
            splitViewDisplayMode = winrt::SplitViewDisplayMode::Overlay;
            break;
        case NavigationViewVisualStateDisplayMode::Minimal:
            visualStateName = visualStateNameMinimal;
            splitViewDisplayMode = winrt::SplitViewDisplayMode::Overlay;
            break;
        case NavigationViewVisualStateDisplayMode::Compact:
            visualStateName = L"Compact";
            splitViewDisplayMode = winrt::SplitViewDisplayMode::CompactOverlay;
            break;
        case NavigationViewVisualStateDisplayMode::Expanded:
            visualStateName = L"Expanded";
            splitViewDisplayMode = winrt::SplitViewDisplayMode::CompactInline;
            break;
        }

        // When the pane is made invisible we need to collapse the pane part of the SplitView
        if (!IsPaneVisible())
        {
            splitViewDisplayMode = winrt::SplitViewDisplayMode::CompactOverlay;
        }

        auto handled = false;
        if (visualStateName == visualStateNameMinimal && IsTopNavigationView())
        {
            // TopNavigationMinimal is introduced since RS6. We need to fallback to Minimal if customer re-template RS5 NavigationView.
            handled = winrt::VisualStateManager::GoToState(*this, L"TopNavigationMinimal", false /*useTransitions*/);
        }
        if (!handled)
        {
            winrt::VisualStateManager::GoToState(*this, visualStateName, false /*useTransitions*/);
        }
        splitView.DisplayMode(splitViewDisplayMode);
    }
}

void NavigationView::OnKeyDown(winrt::KeyRoutedEventArgs const& e)
{
    auto eventArgs = e;
    auto key = eventArgs.Key();

    bool handled = false;

    switch (key)
    {
    case winrt::VirtualKey::GamepadView:
        if (!IsPaneOpen())
        {
            OpenPane();
            handled = true;
        }
        break;
    case winrt::VirtualKey::GoBack:
    case winrt::VirtualKey::XButton1:
        if (IsPaneOpen() && IsLightDismissible())
        {
            handled = AttemptClosePaneLightly();
        }
        break;
    case winrt::VirtualKey::GamepadLeftShoulder:
        handled = BumperNavigation(-1);
        break;
    case winrt::VirtualKey::GamepadRightShoulder:
        handled = BumperNavigation(1);
        break;
    case winrt::VirtualKey::Left:
        auto altState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Menu);
        bool isAltPressed = (altState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
        
        if (isAltPressed && IsPaneOpen() && IsLightDismissible())
        {
            handled = AttemptClosePaneLightly();
        }

        break;
    }

    eventArgs.Handled(handled);

    __super::OnKeyDown(e);
}

bool NavigationView::BumperNavigation(int offset)
{
    // By passing an offset indicating direction (ideally 1 or -1, meaning right or left respectively)
    // we'll try to move focus to an item. We won't be moving focus to items in the overflow menu and this won't
    // work on left navigation, only dealing with the top primary list here and only with items that don't have
    // !SelectsOnInvoked set to true. If !SelectsOnInvoked is true, we'll skip the item and try focusing on the next one
    // that meets the conditions, in the same direction.
    auto shoulderNavigationEnabledParamValue = ShoulderNavigationEnabled();
    auto shoulderNavigationForcedDisabled = (shoulderNavigationEnabledParamValue == winrt::NavigationViewShoulderNavigationEnabled::Never);

    if (!IsTopNavigationView() 
        || !IsNavigationViewListSingleSelectionFollowsFocus() 
        || shoulderNavigationForcedDisabled)
    {
        return false;
    }

    auto shoulderNavigationSelectionFollowsFocusEnabled = (SelectionFollowsFocus() == winrt::NavigationViewSelectionFollowsFocus::Enabled
        && shoulderNavigationEnabledParamValue == winrt::NavigationViewShoulderNavigationEnabled::WhenSelectionFollowsFocus);

    auto shoulderNavigationEnabled = (shoulderNavigationSelectionFollowsFocusEnabled
        || shoulderNavigationEnabledParamValue == winrt::NavigationViewShoulderNavigationEnabled::Always);

    if (!shoulderNavigationEnabled)
    {
        return false;
    }

    auto item = SelectedItem();

    if (item)
    {
        if (auto nvi = NavigationViewItemOrSettingsContentFromData(item))
        {
            auto index = m_topDataProvider.IndexOf(item, PrimaryList);

            if (index >= 0)
            {
                auto topNavListView = m_topNavListView.get();
                auto itemsList = topNavListView.Items();
                auto topPrimaryListSize = m_topDataProvider.GetPrimaryListSize();
                index += offset;

                while (index > -1 && index < topPrimaryListSize)
                {
                    auto newItem = itemsList.GetAt(index);
                    if (auto newNavViewItem = newItem.try_as<winrt::NavigationViewItem>())
                    {
                        // This is done to skip Separators or other items that are not NavigationViewItems
                        if (winrt::get_self<NavigationViewItem>(newNavViewItem)->SelectsOnInvoked())
                        {
                            topNavListView.SelectedItem(newItem);
                            return true;
                        }
                    }

                    index += offset;
                }
            }
        }
    }

    return false;
}

winrt::IInspectable NavigationView::MenuItemFromContainer(winrt::DependencyObject const& container)
{
    if (auto nvi = container)
    {
        if (IsTopNavigationView())
        {
            winrt::IInspectable item{ nullptr };
            // Search topnav first, if not found, search overflow
            if (auto lv = m_topNavListView.get())
            {
                item = lv.ItemFromContainer(nvi);
                if (item)
                {
                    return item;
                }
            }

            if (auto lv = m_topNavListOverflowView.get())
            {
                item = lv.ItemFromContainer(nvi);
            }
            return item;
        }
        else
        {
            if (auto lv = m_leftNavListView.get())
            {
                auto item = lv.ItemFromContainer(nvi);
                return item;
            }
        }
    }

    return nullptr;
}

winrt::DependencyObject NavigationView::ContainerFromMenuItem(winrt::IInspectable const& item)
{
    if (auto data = item)
    {
        return NavigationViewItemBaseOrSettingsContentFromData(item);
    }

    return nullptr;
}

void NavigationView::OnTopNavDataSourceChanged(winrt::NotifyCollectionChangedEventArgs const& args)
{
    CloseTopNavigationViewFlyout();
    
    // Assume that raw data doesn't change very often for navigationview.
    // So here is a simple implementation and for each data item change, it request a layout change
    // update this in the future if there is performance problem

    // If it's InitStep1, it means that we didn't start the layout yet.
    if (m_topNavigationMode != TopNavigationViewLayoutState::InitStep1)
    {
        {
            auto scopeGuard = gsl::finally([this]()
            {
                m_shouldIgnoreOverflowItemSelectionChange = false;
            });
            m_shouldIgnoreOverflowItemSelectionChange = true;
            m_topDataProvider.MoveAllItemsToPrimaryList();
        }
        SetTopNavigationViewNextMode(TopNavigationViewLayoutState::InitStep2);
        InvalidateTopNavPrimaryLayout();
    }

    m_indexOfLastSelectedItemInTopNav = 0;
    m_lastSelectedItemPendingAnimationInTopNav.set(nullptr);
    m_itemsRemovedFromMenuFlyout.clear();
}

int NavigationView::GetNavigationViewItemCountInPrimaryList()
{
    return m_topDataProvider.GetNavigationViewItemCountInPrimaryList();
}

int NavigationView::GetNavigationViewItemCountInTopNav()
{
    return m_topDataProvider.GetNavigationViewItemCountInTopNav();
}

winrt::SplitView NavigationView::GetSplitView()
{
    return m_rootSplitView.get();
}

void NavigationView::TopNavigationViewItemContentChanged()
{
    if (m_appliedTemplate)
    {
        if (ShouldIgnoreMeasureOverride())
        {
            RequestInvalidateMeasureOnNextLayoutUpdate();
        }
        else
        {
            InvalidateMeasure();
        }
    }
}

void NavigationView::OnAccessKeyInvoked(winrt::IInspectable const& sender, winrt::AccessKeyInvokedEventArgs const& args)
{
    if (args.Handled())
    {
        return;
    }

    // For topnav, invoke Morebutton, otherwise togglebutton
    auto button = IsTopNavigationView() ? m_topNavOverflowButton.get() : m_paneToggleButton.get();
    if (button)
    {
        if (auto peer = winrt::FrameworkElementAutomationPeer::FromElement(button).try_as<winrt::ButtonAutomationPeer>())
        {
            peer.Invoke();
            args.Handled(true);
        }
    }
}

winrt::NavigationTransitionInfo NavigationView::CreateNavigationTransitionInfo(NavigationRecommendedTransitionDirection recommendedTransitionDirection)
{
    // In current implementation, if click is from overflow item, just recommend FromRight Slide animation.
    if (recommendedTransitionDirection == NavigationRecommendedTransitionDirection::FromOverflow)
    {
        recommendedTransitionDirection = NavigationRecommendedTransitionDirection::FromRight;
    }

    if ((recommendedTransitionDirection == NavigationRecommendedTransitionDirection::FromLeft
            || recommendedTransitionDirection == NavigationRecommendedTransitionDirection::FromRight)
        && SharedHelpers::IsRS5OrHigher())
    {
        winrt::SlideNavigationTransitionInfo sliderNav;
        winrt::SlideNavigationTransitionEffect effect =
            recommendedTransitionDirection == NavigationRecommendedTransitionDirection::FromRight ?
            winrt::SlideNavigationTransitionEffect::FromRight :
            winrt::SlideNavigationTransitionEffect::FromLeft;
        // PR 1895355: Bug 17724768: Remove Side-to-Side navigation transition velocity key
        // https://microsoft.visualstudio.com/_git/os/commit/7d58531e69bc8ad1761cff938d8db25f6fb6a841
        // We want to use Effect, but it's not in all os of rs5. as a workaround, we only apply effect to the os which is already remove velocity key.
        if (auto sliderNav2 = sliderNav.try_as<winrt::ISlideNavigationTransitionInfo2>())
        {
            sliderNav.Effect(effect);
        }
        return sliderNav;
    } 
    else
    {
        winrt::EntranceNavigationTransitionInfo defaultInfo;
        return defaultInfo;
    }
}

NavigationRecommendedTransitionDirection NavigationView::GetRecommendedTransitionDirection(winrt::DependencyObject const& prev, winrt::DependencyObject const& next)
{
    auto recommendedTransitionDirection = NavigationRecommendedTransitionDirection::Default;
    if (auto topNavListView = m_topNavListView.get())
    {
        MUX_ASSERT(prev && next);
        auto prevIndex = topNavListView.IndexFromContainer(prev);
        auto nextIndex = topNavListView.IndexFromContainer(next);
        if (prevIndex == -1 || nextIndex == -1)
        {
            // One item is settings, so have problem to get the index
            recommendedTransitionDirection = NavigationRecommendedTransitionDirection::Default;
        }
        else if (prevIndex < nextIndex)
        {
            recommendedTransitionDirection = NavigationRecommendedTransitionDirection::FromRight;
        }
        else if (prevIndex > nextIndex)
        {
            recommendedTransitionDirection = NavigationRecommendedTransitionDirection::FromLeft;
        }
    }
    return recommendedTransitionDirection;
}

winrt::NavigationViewItemBase NavigationView::GetContainerForClickedItem(winrt::IInspectable const& itemData)
{
    // ListViewBase::OnItemClick raises ItemClicked event, but it doesn't provide the container of a item
    // If it's an virtualized panel like ItemsStackPanel, IsItemItsOwnContainer is called before raise the event in ListViewBase::OnItemClick.
    // Here we assume the LastItemCalledInIsItemItsOwnContainerOverride is the container.
    winrt::NavigationViewItemBase container{ nullptr };
    auto listView = IsTopNavigationView() ? m_topNavListView.get() : m_leftNavListView.get();
    MUX_ASSERT(listView);

    if (auto navListView = listView.try_as<winrt::NavigationViewList>())
    {
        container = winrt::get_self<NavigationViewList>(navListView)->GetLastItemCalledInIsItemItsOwnContainerOverride();
    }

    // Most likely we didn't use ItemStackPanel. but we still try to see if we can find a matched container.
    if (!container && itemData)
    {
        container = listView.ContainerFromItem(itemData).try_as<winrt::NavigationViewItemBase>();
    }

    MUX_ASSERT(container);
    return container;
}

NavigationViewTemplateSettings* NavigationView::GetTemplateSettings()
{
    return winrt::get_self<NavigationViewTemplateSettings>(TemplateSettings());
}

bool NavigationView::IsNavigationViewListSingleSelectionFollowsFocus()
{
    return (SelectionFollowsFocus() == winrt::NavigationViewSelectionFollowsFocus::Enabled);
}

void NavigationView::UpdateSingleSelectionFollowsFocusTemplateSetting()
{
    GetTemplateSettings()->SingleSelectionFollowsFocus(IsNavigationViewListSingleSelectionFollowsFocus());
}

void NavigationView::OnSelectedItemPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto newItem = args.NewValue();
    ChangeSelection(args.OldValue(), newItem);

    if (m_appliedTemplate && IsTopNavigationView())
    {
        // In above ChangeSelection function, m_shouldIgnoreNextSelectionChange is set to true first and then set to false when leaving the function scope. 
        // When customer select an item by API, SelectionChanged event is raised in ChangeSelection and customer may change the layout.
        // MeasureOverride is executed but it did nothing since m_shouldIgnoreNextSelectionChange is true in ChangeSelection function.
        // InvalidateMeasure to make MeasureOverride happen again
        bool measureOverrideDidNothing = m_shouldInvalidateMeasureOnNextLayoutUpdate && !m_layoutUpdatedToken;
            
        if (measureOverrideDidNothing ||
            (newItem && m_topDataProvider.IndexOf(newItem) != -1 && m_topDataProvider.IndexOf(newItem, PrimaryList) == -1)) // selection is in overflow
        {
            InvalidateTopNavPrimaryLayout();
        }
    }
}

void NavigationView::SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(winrt::IInspectable const& item)
{
    // SelectedItem can be set by API or be clicking/selecting ListViewItem or by clicking on settings
    // We should not raise ItemInvoke if SelectedItem is changed by API.
    // If isChangingSelection, this function is called in an inner loop and it should be called from API, so don't change m_shouldRaiseInvokeItemInSelectionChange
    // Otherwise, it's not from API and expect ItemInvoke when selectionchanged.

    bool isChangingSelection = m_shouldIgnoreNextSelectionChange;
    
    if (!isChangingSelection)
    {
        m_shouldRaiseInvokeItemInSelectionChange = true;
    }

    if (IsTopNavigationView())
    {
        bool shouldAnimateToSelectedItemFromFlyout = true;

        // if the last item selected is going to be removed, i.e. added to the menu flyout, then don't animate.
        for (std::vector<int>::iterator it = m_itemsRemovedFromMenuFlyout.begin(); it != m_itemsRemovedFromMenuFlyout.end(); ++it) 
        {
            if (*it == m_indexOfLastSelectedItemInTopNav)
            {
                shouldAnimateToSelectedItemFromFlyout = false;
                break;
            }
        }

        if (shouldAnimateToSelectedItemFromFlyout)
        {
            m_lastSelectedItemPendingAnimationInTopNav.set(SelectedItem());
        }
        else
        {
            m_lastSelectedItemPendingAnimationInTopNav.set(nullptr);
        }

        m_indexOfLastSelectedItemInTopNav = m_topDataProvider.IndexOf(item); // for the next time we animate
    }

    SelectedItem(item);
    if (!isChangingSelection)
    {
        m_shouldRaiseInvokeItemInSelectionChange = false;
    }
}

bool NavigationView::DoesSelectedItemContainContent(winrt::IInspectable const& item, winrt::NavigationViewItemBase const& itemContainer)
{
    // If item and selected item has same container, it would be selected item
    bool isSelectedItem = false;
    auto selectedItem = SelectedItem();
    if (selectedItem && (item || itemContainer))
    {
        if (item && item == selectedItem)
        {
            isSelectedItem = true;
        }
        else if (auto selectItemContainer = selectedItem.try_as<winrt::NavigationViewItemBase>()) //SelectedItem itself is a container
        {
            isSelectedItem = selectItemContainer == itemContainer;
        }
        else // selectedItem itself is data
        {
            auto selectedItemContainer = NavigationViewItemBaseOrSettingsContentFromData(selectedItem);
            if (selectedItemContainer && itemContainer)
            {
                isSelectedItem = selectedItemContainer == itemContainer;
            }
        }
    }
    return isSelectedItem;
}

void NavigationView::ChangeSelectStatusForItem(winrt::IInspectable const& item, bool selected)
{
    if (auto container = NavigationViewItemOrSettingsContentFromData(item))
    {
        // If we unselect an item, ListView doesn't tolerate setting the SelectedItem to nullptr. 
        // Instead we remove IsSelected from the item itself, and it make ListView to unselect it.
        // If we select an item, we follow the unselect to simplify the code.
        container.IsSelected(selected);
    }
 }

bool NavigationView::IsSettingsItem(winrt::IInspectable const& item)
{
    bool isSettingsItem = false;
    if (item)
    {
        if (auto settingItem = m_settingsItem.get())
        {
            isSettingsItem = (settingItem == item) || (settingItem.Content() == item);
        }
    }
    return isSettingsItem;
}

void NavigationView::UnselectPrevItem(winrt::IInspectable const& prevItem, winrt::IInspectable const& nextItem)
{
    // ListView already handled unselect by itself if ListView raise SelectChanged by itself.
    // We only need to handle unselect when:
    // 1, select from setting to listviewitem or null
    // 2, select from listviewitem to setting
    // 3, select from listviewitem to null from API.
    if (prevItem && prevItem != nextItem)
    {
        // TODO: Add Check for where above selection case 2 & 3 start from listviewitem that is hidden
        if (IsSettingsItem(prevItem) || (nextItem && IsSettingsItem(nextItem)) || !nextItem)
        {
            ChangeSelectStatusForItem(prevItem, false /*selected*/);
        }
    }
}

void NavigationView::UndoSelectionAndRevertSelectionTo(winrt::IInspectable const& prevSelectedItem, winrt::IInspectable const& nextItem)
{    
    winrt::IInspectable selectedItem{ nullptr };
    if (prevSelectedItem)
    {
        if (IsSelectionSuppressed(prevSelectedItem))
        {
            AnimateSelectionChanged(prevSelectedItem, nullptr);
        }
        else
        {
            // In the case of hierarchical nav view, there is a possibility that the previously selected item
            // is hidden (hence not in the listview). This 'if' clause guarantees that the items that needs to be unselected
            // gets unselected.
            if (nextItem)
            {
                ChangeSelectStatusForItem(nextItem, false /*selected*/);
            }
            ChangeSelectStatusForItem(prevSelectedItem, true /*selected*/);
            AnimateSelectionChangedToItem(prevSelectedItem);
            selectedItem = prevSelectedItem;
        }
    }
    else
    {
        // Bug 18033309, A SelectsOnInvoked=false item is clicked, if we don't unselect it from listview, the second click will not raise ItemClicked
        // because listview doesn't raise SelectionChange.
        ChangeSelectStatusForItem(nextItem, false /*selected*/);
    }
    SelectedItem(selectedItem);
}

void NavigationView::CloseTopNavigationViewFlyout()
{
    if (auto button = m_topNavOverflowButton.get())
    {   
        if (auto flyout = button.Flyout())
        { 
            flyout.Hide();
        }
    }
}

void NavigationView::UpdateVisualState(bool useTransitions)
{
    if (m_appliedTemplate)
    {
        auto box = AutoSuggestBox();
        winrt::VisualStateManager::GoToState(*this, box ? L"AutoSuggestBoxVisible" : L"AutoSuggestBoxCollapsed", false /*useTransitions*/);

        bool isVisible = IsSettingsVisible();
        winrt::VisualStateManager::GoToState(*this, isVisible ? L"SettingsVisible" : L"SettingsCollapsed", false /*useTransitions*/);

        if (IsTopNavigationView())
        {
            UpdateVisualStateForOverflowButton();
        }
        else
        {
            UpdateLeftNavigationOnlyVisualState(useTransitions);
        }
    }
}

void NavigationView::UpdateVisualStateForOverflowButton()
{
    auto state = (OverflowLabelMode() == winrt::NavigationViewOverflowLabelMode::MoreLabel)?
        L"OverflowButtonWithLabel":
        L"OverflowButtonNoLabel";
    winrt::VisualStateManager::GoToState(*this, state, false /* useTransitions*/);
}

void NavigationView::UpdateLeftNavigationOnlyVisualState(bool useTransitions)
{
    bool isToggleButtonVisible = IsPaneToggleButtonVisible();
    winrt::VisualStateManager::GoToState(*this, isToggleButtonVisible ? L"TogglePaneButtonVisible" : L"TogglePaneButtonCollapsed", false /*useTransitions*/);
}

void NavigationView::UpdateNavigationViewUseSystemVisual()
{
    if (SharedHelpers::IsRS1OrHigher() && !ShouldPreserveNavigationViewRS4Behavior() && m_appliedTemplate)
    {
        auto showFocusVisual = SelectionFollowsFocus() == winrt::NavigationViewSelectionFollowsFocus::Disabled;

        PropagateChangeToNavigationViewLists(NavigationViewPropagateTarget::LeftListView,
            [showFocusVisual](NavigationViewList* list)
        {
            list->SetShowFocusVisual(showFocusVisual);
        }
        );

        PropagateChangeToNavigationViewLists(NavigationViewPropagateTarget::TopListView,
            [showFocusVisual](NavigationViewList* list)
        {
            list->SetShowFocusVisual(showFocusVisual);
        }
        );
    }
}

void NavigationView::SetNavigationViewListPosition(winrt::ListView& listView, NavigationViewListPosition position)
{
    if (listView)
    {
        if (auto navigationViewList = listView.try_as<winrt::NavigationViewList>())
        {
            winrt::get_self<NavigationViewList>(navigationViewList)->SetNavigationViewListPosition(position);
        }
    }
}

void NavigationView::PropagateNavigationViewAsParent()
{    
    PropagateChangeToNavigationViewLists(NavigationViewPropagateTarget::All,
        [this](NavigationViewList* list)
            {
                list->SetNavigationViewParent(*this);
            }
        );
}

void NavigationView::PropagateChangeToNavigationViewLists(NavigationViewPropagateTarget target, std::function<void(NavigationViewList*)> const& function)
{
    if (NavigationViewPropagateTarget::LeftListView == target || 
        NavigationViewPropagateTarget::All == target)
    {
        PropagateChangeToNavigationViewList(m_leftNavListView.get(), function);
    }
    if (NavigationViewPropagateTarget::TopListView == target ||
        NavigationViewPropagateTarget::All == target)
    {
        PropagateChangeToNavigationViewList(m_topNavListView.get(), function);
    }
    if (NavigationViewPropagateTarget::OverflowListView == target ||
        NavigationViewPropagateTarget::All == target)
    {
        PropagateChangeToNavigationViewList(m_topNavListOverflowView.get(), function);
    }
}

void NavigationView::PropagateChangeToNavigationViewList(winrt::ListView const& listView, std::function<void(NavigationViewList*)> const& function)
{
    if (listView)
    {
        if (auto navigationViewList = listView.try_as<winrt::NavigationViewList>())
        {
            auto container = winrt::get_self<NavigationViewList>(navigationViewList);
            function(container);
        }
    }
}

void NavigationView::InvalidateTopNavPrimaryLayout()
{
    if (m_appliedTemplate && IsTopNavigationView())
    {
        InvalidateMeasure();
    }
}

float NavigationView::MeasureTopNavigationViewDesiredWidth(winrt::Size const& availableSize)
{
    float width = 0.0;
    width += LayoutUtils::MeasureAndGetDesiredWidthFor(m_buttonHolderGrid.get(), availableSize);
    width += LayoutUtils::MeasureAndGetDesiredWidthFor(m_topNavGrid.get(), availableSize);
    return width;
}

float NavigationView::MeasureTopNavMenuItemsHostDesiredWidth(winrt::Size const& availableSize)
{
    return LayoutUtils::MeasureAndGetDesiredWidthFor(m_topNavListView.get(), availableSize);
}

float NavigationView::GetTopNavigationViewActualWidth()
{
    double width = 0.0;
    width += LayoutUtils::GetActualWidthFor(m_buttonHolderGrid.get());
    width += LayoutUtils::GetActualWidthFor(m_topNavGrid.get());
    MUX_ASSERT(width < std::numeric_limits<float>::max());
    return static_cast<float>(width);
}

bool NavigationView::IsTopNavigationFirstMeasure()
{
    // ItemsStackPanel have two round of measure. the first measure only measure the first child, then provide a roughly estimation
    // second measure would initialize the containers.
    bool firstMeasure = false;
    if (auto listView = m_topNavListView.get())
    {
        int size = m_topDataProvider.GetPrimaryListSize();
        if (size > 1)
        {
            auto container = listView.ContainerFromIndex(1);
            firstMeasure = !container;
        }
    }
    return firstMeasure;
}

void NavigationView::RequestInvalidateMeasureOnNextLayoutUpdate()
{
    m_shouldInvalidateMeasureOnNextLayoutUpdate = true;
}

bool NavigationView::HasTopNavigationViewItemNotInPrimaryList()
{
    return m_topDataProvider.GetPrimaryListSize() != m_topDataProvider.Size();
}

void NavigationView::HandleTopNavigationMeasureOverride(winrt::Size const& availableSize)
{
    auto mode = m_topNavigationMode; // mode is for debugging because m_topNavigationMode is changing but we don't want to loss it in the stack
    switch (mode)
    {
    case TopNavigationViewLayoutState::InitStep1: // Move all data to primary
        if (HasTopNavigationViewItemNotInPrimaryList())
        {
            m_topDataProvider.MoveAllItemsToPrimaryList();
        }
        else
        {
             ContinueHandleTopNavigationMeasureOverride(TopNavigationViewLayoutState::InitStep2, availableSize);
        }
        break;
    case TopNavigationViewLayoutState::InitStep2: // Realized virtualization items
        {
            // Bug 18196691: For some reason(eg: customer hide topnav grid or it's parent from code directly), 
            // The 2nd item may never been realized. and it will enter into a layout_cycle.
            // For performance reason, we don't go through the visualtree to determine if ListView is actually visible or not
            // m_measureOnInitStep2Count is used to avoid the cycle

            // In our test environment, m_measureOnInitStep2Count should <= 2 since we didn't hide anything from code
            // so the assert count is different from s_measureOnInitStep2CountThreshold 
            MUX_ASSERT(m_measureOnInitStep2Count <= 2);

            if (m_measureOnInitStep2Count >= s_measureOnInitStep2CountThreshold || !IsTopNavigationFirstMeasure())
            {
                m_measureOnInitStep2Count = 0;
                ContinueHandleTopNavigationMeasureOverride(TopNavigationViewLayoutState::InitStep3, availableSize);
            }
            else
            {
                m_measureOnInitStep2Count++;
            }
        }
        break;

    case TopNavigationViewLayoutState::InitStep3: // Waiting for moving data to overflow
        HandleTopNavigationMeasureOverrideStep3(availableSize);
        break;
    case TopNavigationViewLayoutState::Normal:
        HandleTopNavigationMeasureOverrideNormal(availableSize);
        break;
    case TopNavigationViewLayoutState::Overflow:
        HandleTopNavigationMeasureOverrideOverflow(availableSize);
        break;
    case TopNavigationViewLayoutState::OverflowNoChange:
        SetTopNavigationViewNextMode(TopNavigationViewLayoutState::Overflow);
        break;
    }
}

void NavigationView::HandleTopNavigationMeasureOverrideNormal(const winrt::Windows::Foundation::Size & availableSize)
{
    auto desiredWidth = MeasureTopNavigationViewDesiredWidth(c_infSize);
    if (desiredWidth > availableSize.Width)
    {
        ContinueHandleTopNavigationMeasureOverride(TopNavigationViewLayoutState::InitStep3, availableSize);
    }
}

void NavigationView::HandleTopNavigationMeasureOverrideOverflow(const winrt::Windows::Foundation::Size & availableSize)
{
    auto desiredWidth = MeasureTopNavigationViewDesiredWidth(c_infSize);
    if (desiredWidth > availableSize.Width)
    {
        ShrinkTopNavigationSize(desiredWidth, availableSize);
    }
    else if (desiredWidth < availableSize.Width)
    {
        auto fullyRecoverWidth = m_topDataProvider.WidthRequiredToRecoveryAllItemsToPrimary();
        if (availableSize.Width >= desiredWidth + fullyRecoverWidth + m_topNavigationRecoveryGracePeriodWidth)
        {
            // It's possible to recover from Overflow to Normal state, so we restart the MeasureOverride from first step
            ContinueHandleTopNavigationMeasureOverride(TopNavigationViewLayoutState::InitStep1, availableSize);
        }
        else
        {
            m_topDataProvider.InvalidWidthCacheIfOverflowItemContentChanged();

            auto movableItems = FindMovableItemsRecoverToPrimaryList(availableSize.Width- desiredWidth, {}/*includeItems*/);
            m_topDataProvider.MoveItemsToPrimaryList(movableItems);
            if (m_topDataProvider.HasInvalidWidth(movableItems))
            {
                m_topDataProvider.ResetAttachedData(); // allow every item to be recovered in next MeasureOverride
                RequestInvalidateMeasureOnNextLayoutUpdate();
            }
        }
    }
}

void NavigationView::ContinueHandleTopNavigationMeasureOverride(TopNavigationViewLayoutState nextMode, const winrt::Size & availableSize)
{
    SetTopNavigationViewNextMode(nextMode);
    HandleTopNavigationMeasureOverride(availableSize);
}

void NavigationView::HandleTopNavigationMeasureOverrideStep3(winrt::Size const& availableSize)
{
    SetOverflowButtonVisibility(winrt::Visibility::Collapsed);
    auto desiredWidth = MeasureTopNavigationViewDesiredWidth(c_infSize);
    if (desiredWidth < availableSize.Width)
    {
        ContinueHandleTopNavigationMeasureOverride(TopNavigationViewLayoutState::Normal, availableSize);
    }
    else
    {
        // overflow
        SetOverflowButtonVisibility(winrt::Visibility::Visible);
        auto desiredWidthForOverflowButton = MeasureTopNavigationViewDesiredWidth(c_infSize);
        
        MUX_ASSERT(desiredWidthForOverflowButton >= desiredWidth);
        m_topDataProvider.OverflowButtonWidth(desiredWidthForOverflowButton - desiredWidth);

        ShrinkTopNavigationSize(desiredWidthForOverflowButton, availableSize);
    }
}

void NavigationView::SetOverflowButtonVisibility(winrt::Visibility const& visibility)
{ 
    if (visibility != TemplateSettings().OverflowButtonVisibility())
    {
       GetTemplateSettings()->OverflowButtonVisibility(visibility);
    }
}

void NavigationView::SetTopNavigationViewNextMode(TopNavigationViewLayoutState nextMode)
{
    m_topNavigationMode = nextMode;
}

void NavigationView::SelectOverflowItem(winrt::IInspectable const& item)
{
    // Calculate selected overflow item size.
    auto selectedOverflowItemIndex = m_topDataProvider.IndexOf(item);
    MUX_ASSERT(selectedOverflowItemIndex != -1);
    auto selectedOverflowItemWidth = m_topDataProvider.GetWidthForItem(selectedOverflowItemIndex);
 
    bool needInvalidMeasure = !m_topDataProvider.IsValidWidthForItem(selectedOverflowItemIndex);

    if (!needInvalidMeasure)
    {
        //
        auto actualWidth = GetTopNavigationViewActualWidth();
        auto desiredWidth = MeasureTopNavigationViewDesiredWidth(c_infSize);
        MUX_ASSERT(desiredWidth <= actualWidth);

        // Calculate selected item size
        auto selectedItemIndex = -1;
        auto selectedItemWidth = 0.f;
        if (auto selectedItem = SelectedItem())
        {
            selectedItemIndex = m_topDataProvider.IndexOf(selectedItem);
            if (selectedItemIndex != -1)
            {
                selectedItemWidth = m_topDataProvider.GetWidthForItem(selectedItemIndex);
            }
        }

        auto widthAtLeastToBeRemoved = desiredWidth + selectedOverflowItemWidth - actualWidth;

        // calculate items to be removed from primary because a overflow item is selected. 
        // SelectedItem is assumed to be removed from primary first, then added it back if it should not be removed
        auto itemsToBeRemoved = FindMovableItemsToBeRemovedFromPrimaryList(widthAtLeastToBeRemoved, { } /*excludeItems*/);
        m_itemsRemovedFromMenuFlyout = itemsToBeRemoved;
        
        // calculate the size to be removed
        auto toBeRemovedItemWidth = m_topDataProvider.CalculateWidthForItems(itemsToBeRemoved);

        auto widthAvailableToRecover = toBeRemovedItemWidth - widthAtLeastToBeRemoved;
        auto itemsToBeAdded = FindMovableItemsRecoverToPrimaryList(widthAvailableToRecover, { selectedOverflowItemIndex }/*includeItems*/);

        CollectionHelper::unique_push_back(itemsToBeAdded, selectedOverflowItemIndex);

        if (m_topDataProvider.HasInvalidWidth(itemsToBeAdded))
        {
            needInvalidMeasure = true;
        }
        else
        {
            // Exchange items between Primary and Overflow
            {
                auto scopeGuard = gsl::finally([this]()
                {
                    m_shouldIgnoreNextSelectionChange = false;
                });
                m_shouldIgnoreNextSelectionChange = true;

                m_topDataProvider.MoveItemsToPrimaryList(itemsToBeAdded);
                m_topDataProvider.MoveItemsOutOfPrimaryList(itemsToBeRemoved);
            }
            SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(item);

            SetTopNavigationViewNextMode(TopNavigationViewLayoutState::OverflowNoChange);
            InvalidateMeasure();
        }
    }

    if (needInvalidMeasure || m_shouldInvalidateMeasureOnNextLayoutUpdate)
    {
        // not all items have known width, need to redo the layout
        m_topDataProvider.MoveAllItemsToPrimaryList();
        SetTopNavigationViewNextMode(TopNavigationViewLayoutState::InitStep2);
        SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(item);
        InvalidateTopNavPrimaryLayout();  
    }
}

void NavigationView::ShrinkTopNavigationSize(float desiredWidth, winrt::Size const& availableSize)
{   
    UpdateTopNavigationWidthCache();
    SetTopNavigationViewNextMode(TopNavigationViewLayoutState::Overflow);

    auto selectedItemIndex = GetSelectedItemIndex();

    auto possibleWidthForPrimaryList = MeasureTopNavMenuItemsHostDesiredWidth(c_infSize) - (desiredWidth - availableSize.Width);
    if (possibleWidthForPrimaryList >= 0)
    {
        // Remove all items which is not visible except first item and selected item.
        auto itemToBeRemoved = FindMovableItemsBeyondAvailableWidth(possibleWidthForPrimaryList);
        // should keep at least one item in primary
        KeepAtLeastOneItemInPrimaryList(itemToBeRemoved, true/*shouldKeepFirst*/);
        m_topDataProvider.MoveItemsOutOfPrimaryList(itemToBeRemoved);
    }

    // measure again to make sure SelectedItem is realized
    desiredWidth = MeasureTopNavigationViewDesiredWidth(c_infSize);

    auto widthAtLeastToBeRemoved = desiredWidth - availableSize.Width;
    if (widthAtLeastToBeRemoved > 0)
    {
        auto itemToBeRemoved = FindMovableItemsToBeRemovedFromPrimaryList(widthAtLeastToBeRemoved, { selectedItemIndex });

        // At least one item is kept on primary list
        KeepAtLeastOneItemInPrimaryList(itemToBeRemoved, false/*shouldKeepFirst*/);
        
        // There should be no item is virtualized in this step
        MUX_ASSERT(!m_topDataProvider.HasInvalidWidth(itemToBeRemoved));
        m_topDataProvider.MoveItemsOutOfPrimaryList(itemToBeRemoved);
    }
}

std::vector<int> NavigationView::FindMovableItemsRecoverToPrimaryList(float availableWidth, std::vector<int> const& includeItems)
{
    std::vector<int> toBeMoved;

    auto size = m_topDataProvider.Size();
   
    // Included Items take high priority, all of them are included in recovery list
    for (auto index : includeItems)
    {
        auto width = m_topDataProvider.GetWidthForItem(index);
        toBeMoved.push_back(index);
        availableWidth -= width;
    }

    int i = 0;
    while (i < size && availableWidth > 0)
    {
        if (!m_topDataProvider.IsItemInPrimaryList(i) && !CollectionHelper::contains(includeItems, i))
        {
            auto width = m_topDataProvider.GetWidthForItem(i);
            if (availableWidth >= width)
            {
                toBeMoved.push_back(i);
                availableWidth -= width;
            }
            else
            {
                break;
            }
        }
        i++;
    }
    // Keep at one item is not in primary list. Two possible reason: 
    //  1, Most likely it's caused by m_topNavigationRecoveryGracePeriod
    //  2, virtualization and it doesn't have cached width
    if (i == size && !toBeMoved.empty())
    {
        toBeMoved.pop_back();
    }
    return toBeMoved;
}

std::vector<int> NavigationView::FindMovableItemsToBeRemovedFromPrimaryList(float widthAtLeastToBeRemoved, std::vector<int> const& excludeItems)
{
    std::vector<int> toBeMoved;

    int i = m_topDataProvider.Size() - 1;
    while (i >= 0 && widthAtLeastToBeRemoved > 0)
    {
        if (m_topDataProvider.IsItemInPrimaryList(i))
        {
            if (!CollectionHelper::contains(excludeItems, i))
            {
                auto width = m_topDataProvider.GetWidthForItem(i);
                toBeMoved.push_back(i);
                widthAtLeastToBeRemoved -= width;
            }
        }
        i--;
    }
    
    return toBeMoved;
}

std::vector<int> NavigationView::FindMovableItemsBeyondAvailableWidth(float availableWidth)
{
    std::vector<int> toBeMoved;
    if (auto listView = m_topNavListView.get())
    {
        int selectedItemIndexInPrimary = m_topDataProvider.IndexOf(SelectedItem(), PrimaryList);
        int size = m_topDataProvider.GetPrimaryListSize();

        float requiredWidth = 0;

        for (int i = 0; i<size; i++)
        {
            if (i != selectedItemIndexInPrimary)
            {
                bool shouldMove = true;
                if (requiredWidth <= availableWidth)
                {
                    auto container = listView.ContainerFromIndex(i);
                    if (container)
                    {
                        if (auto containerAsUIElement = container.try_as<winrt::UIElement>())
                        {
                            auto width = containerAsUIElement.DesiredSize().Width;
                            requiredWidth += width;
                            shouldMove = requiredWidth > availableWidth;
                        }
                    }
                    else
                    {
                        // item is virtualized but not realized.                    
                    }
                }

                if (shouldMove)
                {
                    toBeMoved.push_back(i);
                }
            }
        }
    }

    return m_topDataProvider.ConvertPrimaryIndexToIndex(toBeMoved);
}

void NavigationView::KeepAtLeastOneItemInPrimaryList(std::vector<int> itemInPrimaryToBeRemoved, bool shouldKeepFirst)
{
    if (!itemInPrimaryToBeRemoved.empty() && static_cast<int>(itemInPrimaryToBeRemoved.size()) == m_topDataProvider.GetPrimaryListSize())
    {
        if (shouldKeepFirst)
        {
            itemInPrimaryToBeRemoved.erase(itemInPrimaryToBeRemoved.begin());
        }
        else
        {
            itemInPrimaryToBeRemoved.pop_back();
        }
    }
}

int NavigationView::GetSelectedItemIndex()
{
    return m_topDataProvider.IndexOf(SelectedItem());
}

void NavigationView::UpdateTopNavigationWidthCache()
{
    int size = m_topDataProvider.GetPrimaryListSize();
    if (auto topNavigationView = m_topNavListView.get())
    {
        for (int i = 0; i < size; i++)
        {
            auto container = topNavigationView.ContainerFromIndex(i);
            if (container)
            {
                if (auto containerAsUIElement = container.try_as<winrt::UIElement>())
                {
                    auto width = containerAsUIElement.DesiredSize().Width;
                    m_topDataProvider.UpdateWidthForPrimaryItem(i, width);
                }
            }
            else
            {
                break;
            }
        }
    }
}

bool NavigationView::IsTopNavigationView()
{
    return PaneDisplayMode() == winrt::NavigationViewPaneDisplayMode::Top;
}

bool NavigationView::IsTopPrimaryListVisible()
{
    return m_topNavListView && (TemplateSettings().TopPaneVisibility() == winrt::Visibility::Visible);
}

void NavigationView::CoerceToGreaterThanZero(double& value)
{
    // Property coercion for OpenPaneLength, CompactPaneLength, CompactModeThresholdWidth, ExpandedModeThresholdWidth
    value = std::max(value, 0.0);
}

void NavigationView::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_IsPaneOpenProperty)
    {
        OnIsPaneOpenChanged();
    }
    else if (property == s_CompactModeThresholdWidthProperty ||
        property == s_ExpandedModeThresholdWidthProperty)
    {
        UpdateAdaptiveLayout(ActualWidth());
    }
    else if (property == s_AlwaysShowHeaderProperty || property == s_HeaderProperty)
    {
        UpdateHeaderVisibility();
    }
    else if (property == s_SelectedItemProperty)
    {
        OnSelectedItemPropertyChanged(args);
    }    
    else if (property == s_PaneTitleProperty)
    {
        UpdatePaneToggleSize();
    }
    else if (property == s_IsBackButtonVisibleProperty)
    {
        UpdateBackButtonVisibility();
        UpdateAdaptiveLayout(ActualWidth());
        if (IsTopNavigationView())
        {
            InvalidateTopNavPrimaryLayout();
        }
        
        if (g_IsTelemetryProviderEnabled && IsBackButtonVisible() == winrt::NavigationViewBackButtonVisible::Collapsed)
        {
            //  Explicitly disabling BackUI on NavigationView
            TraceLoggingWrite(
                g_hTelemetryProvider,  
                "NavigationView_DisableBackUI",
                TraceLoggingDescription("Developer explicitly disables the BackUI on NavigationView"));
        }
    }
    else if (property == s_MenuItemsSourceProperty)
    {
        UpdateListViewItemSource();
    }
    else if (property == s_MenuItemsProperty)
    {
        UpdateListViewItemSource();
    }
    else if (property == s_PaneDisplayModeProperty)
    {
        // m_wasForceClosed is set to true because ToggleButton is clicked and Pane is closed.
        // When PaneDisplayMode is changed, reset the force flag to make the Pane can be opened automatically again.
        m_wasForceClosed = false;

        UpdatePaneDisplayMode(auto_unbox(args.OldValue()), auto_unbox(args.NewValue()));
        UpdatePaneToggleButtonVisibility();
        UpdatePaneVisibility();
        UpdateVisualState();
    }
    else if (property == s_IsPaneVisibleProperty)
    {
        UpdatePaneVisibility();
        UpdateVisualStateForDisplayModeGroup(DisplayMode());
    }
    else if (property == s_OverflowLabelModeProperty)
    {
        if (m_appliedTemplate)
        {
            UpdateVisualStateForOverflowButton();
            InvalidateTopNavPrimaryLayout();
        }
    }   
    else if (property == s_AutoSuggestBoxProperty)
    {
        InvalidateTopNavPrimaryLayout();
    }
    else if (property == s_SelectionFollowsFocusProperty)
    {
        UpdateSingleSelectionFollowsFocusTemplateSetting();
        UpdateNavigationViewUseSystemVisual();
    }
    else if (property == s_IsPaneToggleButtonVisibleProperty)
    {
        UpdatePaneToggleButtonVisibility();
        UpdateVisualState();
    }
    else if (property == s_IsSettingsVisibleProperty)
    {
        UpdateVisualState();
    }        
}


void NavigationView::OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    if (auto item = SelectedItem())
    {
        if (!IsSelectionSuppressed(item))
        {
            // Work around for issue where NavigationViewItem doesn't report
            // its initial IsSelected state properly on RS2 and older builds.
            //
            // Without this, the visual state is proper, but the actual 
            // IsSelected reported by the NavigationViewItem is not.
            if (!SharedHelpers::IsRS3OrHigher())
            {
                if (auto navViewItem = item.try_as<winrt::NavigationViewItem>())
                {
                    navViewItem.IsSelected(true);
                }
            }
        }
        AnimateSelectionChanged(nullptr /* prevItem */, item);
    }
}

void NavigationView::OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UnhookEventsAndClearFields();
}

void NavigationView::OnIsPaneOpenChanged()
{
    auto isPaneOpen = IsPaneOpen();
    if (isPaneOpen && m_wasForceClosed)
    {
        m_wasForceClosed = false; // remove the pane open flag since Pane is opened.
    }
    else if (!m_isOpenPaneForInteraction && !isPaneOpen)
    {
        if (auto splitView = m_rootSplitView.get())
        {
            // splitview.IsPaneOpen and nav.IsPaneOpen is two way binding. There is possible change that SplitView.IsPaneOpen=false, then
            // nav.IsPaneOpen=false. We don't need to set force flag in this situation
            if (splitView.IsPaneOpen())
            {
                m_wasForceClosed = true;
            }
        }
    }
    SetPaneToggleButtonAutomationName();
    UpdatePaneTabFocusNavigation();
    UpdateSettingsItemToolTip();

    if (SharedHelpers::IsThemeShadowAvailable())
    {
#ifdef USE_INTERNAL_SDK
        if (auto splitView = m_rootSplitView.get())
        {
            if (auto paneRoot = splitView.Pane())
            {
                auto currentTranslation = paneRoot.Translation();
                auto translation = winrt::float3{ currentTranslation.x, currentTranslation.y, IsPaneOpen() ? c_paneElevationTranslationZ : 0.0f };
                paneRoot.Translation(translation);
            }
        }
#endif
    }
}

void NavigationView::UpdatePaneToggleButtonVisibility()
{
    auto visible = IsPaneToggleButtonVisible() && !IsTopNavigationView();
    GetTemplateSettings()->PaneToggleButtonVisibility(Util::VisibilityFromBool(visible));
}

void NavigationView::UpdatePaneDisplayMode()
{
    if (!m_appliedTemplate)
    {
        return; 
    }
    if (!IsTopNavigationView())
    {
        UpdateAdaptiveLayout(ActualWidth(), true /*forceSetDisplayMode*/);

        SwapPaneHeaderContent(m_leftNavPaneHeaderContentBorder, m_paneHeaderOnTopPane, L"PaneHeader");
        SwapPaneHeaderContent(m_leftNavPaneCustomContentBorder, m_paneCustomContentOnTopPane, L"PaneCustomContent");
        SwapPaneHeaderContent(m_leftNavFooterContentBorder, m_paneFooterOnTopPane, L"PaneFooter");

        CreateAndHookEventsToSettings(c_settingsName);

        if (winrt::IUIElement8 thisAsUIElement8 = *this)
        {
            if (auto paneToggleButton = m_paneToggleButton.get())
            {
                thisAsUIElement8.KeyTipTarget(paneToggleButton);
            }
        }
        
    }
    else
    {
        ClosePane();
        SetDisplayMode(winrt::NavigationViewDisplayMode::Minimal, true);

        SwapPaneHeaderContent(m_paneHeaderOnTopPane, m_leftNavPaneHeaderContentBorder, L"PaneHeader");
        SwapPaneHeaderContent(m_paneCustomContentOnTopPane, m_leftNavPaneCustomContentBorder, L"PaneCustomContent");
        SwapPaneHeaderContent(m_paneFooterOnTopPane, m_leftNavFooterContentBorder, L"PaneFooter");

        CreateAndHookEventsToSettings(c_settingsNameTopNav);

        if (winrt::IUIElement8 thisAsUIElement8 = *this)
        {
            if (auto topNavOverflowButton = m_topNavOverflowButton.get())
            {
                thisAsUIElement8.KeyTipTarget(topNavOverflowButton);
            }
        }
    }

    UpdateContentBindingsForPaneDisplayMode();
    UpdateListViewItemSource();
}

void NavigationView::UpdatePaneDisplayMode(winrt::NavigationViewPaneDisplayMode oldDisplayMode, winrt::NavigationViewPaneDisplayMode newDisplayMode)
{
    if (!m_appliedTemplate)
    {
        return;
    }

    UpdatePaneDisplayMode();

    // For better user experience, We help customer to Open/Close Pane automatically when we switch between LeftMinimal <-> other PaneDisplayMode.
    // From other navigation PaneDisplayMode to LeftMinimal, we expect pane is closed.
    // From LeftMinimal to other left PaneDisplayMode other than Auto, we expect Pane is opened.
    if (!IsTopNavigationView())
    {
        bool isPaneOpen = IsPaneOpen();
        if (newDisplayMode == winrt::NavigationViewPaneDisplayMode::LeftMinimal && isPaneOpen)
        {            
            ClosePane();
        }
        else if (oldDisplayMode == winrt::NavigationViewPaneDisplayMode::LeftMinimal &&
            !isPaneOpen &&
            newDisplayMode != winrt::NavigationViewPaneDisplayMode::Auto)
        {
            OpenPane();
        }
    }
}

void NavigationView::UpdatePaneVisibility()
{
    auto templateSettings = GetTemplateSettings();
    if (IsPaneVisible())
    {
        if (IsTopNavigationView())
        {
            templateSettings->LeftPaneVisibility(winrt::Visibility::Collapsed);
            templateSettings->TopPaneVisibility(winrt::Visibility::Visible);
        }
        else
        {
            templateSettings->TopPaneVisibility(winrt::Visibility::Collapsed);
            templateSettings->LeftPaneVisibility(winrt::Visibility::Visible);
        }

        winrt::VisualStateManager::GoToState(*this, L"PaneVisible", false /*useTransitions*/);
    }
    else
    {
        templateSettings->TopPaneVisibility(winrt::Visibility::Collapsed);
        templateSettings->LeftPaneVisibility(winrt::Visibility::Collapsed);

        winrt::VisualStateManager::GoToState(*this, L"PaneCollapsed", false /*useTransitions*/);
    }
}

void NavigationView::SwapPaneHeaderContent(tracker_ref<winrt::ContentControl> newParentTrackRef, tracker_ref<winrt::ContentControl> oldParentTrackRef, winrt::hstring const& propertyPathName)
{
    if (auto newParent = newParentTrackRef.get())
    {
        if (auto oldParent = oldParentTrackRef.get())
        {
            oldParent.ClearValue(winrt::ContentControl::ContentProperty());
        }

        auto binding = winrt::Binding();
        auto propertyPath = winrt::PropertyPath(propertyPathName);
        binding.Path(propertyPath);
        binding.Source(*this);
        winrt::BindingOperations::SetBinding(newParent, winrt::ContentControl::ContentProperty(), binding);
    }
}

void NavigationView::UpdateContentBindingsForPaneDisplayMode()
{
    winrt::UIElement autoSuggestBoxContentControl = nullptr;
    winrt::UIElement notControl = nullptr;
    if (!IsTopNavigationView())
    {
        autoSuggestBoxContentControl = m_leftNavPaneAutoSuggestBoxPresenter.get();
        notControl = m_topNavPaneAutoSuggestBoxPresenter.get();
    } 
    else
    {
        autoSuggestBoxContentControl = m_topNavPaneAutoSuggestBoxPresenter.get();
        notControl = m_leftNavPaneAutoSuggestBoxPresenter.get();
    }

    if (autoSuggestBoxContentControl)
    {
        if (notControl)
        {
            notControl.ClearValue(winrt::ContentControl::ContentProperty());
        }

        auto binding = winrt::Binding();
        auto propertyPath = winrt::PropertyPath(L"AutoSuggestBox");
        binding.Path(propertyPath);
        binding.Source(*this);
        winrt::BindingOperations::SetBinding(autoSuggestBoxContentControl, winrt::ContentControl::ContentProperty(), binding);
    }
}

void NavigationView::UpdateHeaderVisibility()
{
    if (!m_appliedTemplate)
    {
        return;
    }

    UpdateHeaderVisibility(DisplayMode());
}

void NavigationView::UpdateHeaderVisibility(winrt::NavigationViewDisplayMode displayMode)
{
    bool showHeader = AlwaysShowHeader() || displayMode == winrt::NavigationViewDisplayMode::Minimal;
    // Like bug 17517627, Customer like WallPaper Studio 10 expects a HeaderContent visual even if Header() is null. 
    // App crashes when they have dependency on that visual, but the crash is not directly state that it's a header problem.   
    // NavigationView doesn't use quirk, but we determine the version by themeresource.
    // As a workaround, we 'quirk' it for RS4 or before release. if it's RS4 or before, HeaderVisible is not related to Header().
    // If theme resource is RS5 or later, we will not show header if header is null.
    if (!ShouldPreserveNavigationViewRS4Behavior())
    {
        showHeader = Header() && showHeader;
    }
    winrt::VisualStateManager::GoToState(*this, showHeader ? L"HeaderVisible" : L"HeaderCollapsed", false /*useTransitions*/);
}

void NavigationView::UpdatePaneTabFocusNavigation()
{
    if (!m_appliedTemplate)
    {
        return;
    }

    if (SharedHelpers::IsRS2OrHigher())
    {
        winrt::KeyboardNavigationMode mode = winrt::KeyboardNavigationMode::Local;

        if (auto splitView = m_rootSplitView.get())
        {
            // If the pane is open in an overlay (light-dismiss) mode, trap keyboard focus inside the pane
            if (IsPaneOpen() && (splitView.DisplayMode() == winrt::SplitViewDisplayMode::Overlay || splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactOverlay))
            {
                mode = winrt::KeyboardNavigationMode::Cycle;
            }
        }

        if (auto paneContentGrid = m_paneContentGrid.get())
        {
            paneContentGrid.TabFocusNavigation(mode);
        }
    }
}

void NavigationView::UpdatePaneToggleSize()
{
    if (!ShouldPreserveNavigationViewRS3Behavior())
    {
        if (auto splitView = m_rootSplitView.get())
        {
            double width = c_paneToggleButtonWidth;

            auto resourceName = box_value(L"PaneToggleButtonWidth");
            if (winrt::Application::Current().Resources().HasKey(resourceName))
            {
                if (auto lookup = winrt::Application::Current().Resources().Lookup(resourceName))
                {
                    width = unbox_value<double>(lookup);
                }
            }

            double togglePaneButtonWidth = width;

            if (ShouldShowBackButton() && splitView.DisplayMode() == winrt::SplitViewDisplayMode::Overlay)
            {
                double backButtonWidth = c_backButtonWidth;
                if (auto backButton = m_backButton.get())
                {
                    backButtonWidth = backButton.Width();
                }

                width += backButtonWidth;
            }

            if (!m_isClosedCompact && PaneTitle().size() > 0)
            {
                if (splitView.DisplayMode() == winrt::SplitViewDisplayMode::Overlay && IsPaneOpen())
                {
                    width = OpenPaneLength();
                    togglePaneButtonWidth = OpenPaneLength() - (ShouldShowBackButton() ? c_backButtonWidth : 0);
                }
                else if (!(splitView.DisplayMode() == winrt::SplitViewDisplayMode::Overlay && !IsPaneOpen()))
                {
                    width = OpenPaneLength();
                    togglePaneButtonWidth = OpenPaneLength();
                }
            }

            if (auto buttonHolderGrid = m_buttonHolderGrid.get())
            {
                buttonHolderGrid.Width(width);
            }

            if (auto toggleButton = m_paneToggleButton.get())
            {
                toggleButton.Width(togglePaneButtonWidth);
            }
        }
    }
}

void NavigationView::UpdateBackButtonVisibility()
{
    if (!m_appliedTemplate)
    {
        return;
    }

    auto shouldShowBackButton = ShouldShowBackButton();
    auto visibility = Util::VisibilityFromBool(shouldShowBackButton);
    GetTemplateSettings()->BackButtonVisibility(visibility);
   
    if (auto backButton = m_backButton.get(); backButton && ShouldPreserveNavigationViewRS4Behavior())
    {
        backButton.Visibility(visibility);
    }

    if (auto paneContentGridAsUIE = m_paneContentGrid.get())
    {
        if (auto paneContentGrid = paneContentGridAsUIE.try_as<winrt::Grid>())
        {
            auto rowDefs = paneContentGrid.RowDefinitions();
            auto rowDef = rowDefs.GetAt(c_backButtonRowDefinition);

            int backButtonRowHeight = 0;
            if (!IsOverlay() && ShouldShowBackButton())
            {
                backButtonRowHeight = c_backButtonHeight;
            }
            else if (ShouldPreserveNavigationViewRS3Behavior())
            {
                // This row represented the height of the hamburger+margin in RS3 and prior
                backButtonRowHeight = c_toggleButtonHeightWhenShouldPreserveNavigationViewRS3Behavior;
            }

            auto length = winrt::GridLengthHelper::FromPixels(backButtonRowHeight);
            rowDef.Height(length);
        }
    }

    if (!ShouldPreserveNavigationViewRS4Behavior())
    {
        winrt::VisualStateManager::GoToState(*this, shouldShowBackButton ? L"BackButtonVisible" : L"BackButtonCollapsed", false /*useTransitions*/);
    }
    UpdateTitleBarPadding();
}

void NavigationView::UpdatePaneTitleMargins()
{
    if (auto textBlock = m_paneTitleTextBlock.get())
    {
        double width = 0;

        double buttonSize = c_paneToggleButtonWidth; // in case the resource lookup fails
        if (auto lookup = winrt::Application::Current().Resources().Lookup(box_value(L"PaneToggleButtonWidth")))
        {
            buttonSize = unbox_value<double>(lookup);
        }

        width += buttonSize;

        if (ShouldShowBackButton() && IsOverlay())
        {
            width += c_backButtonWidth;
        }

        textBlock.Margin({ width, 0, 0, 0 }); // see "Hamburger title" on uni
    }
}

void NavigationView::UpdateLeftNavListViewItemSource(const winrt::IInspectable& items)
{
    // unbinding Data from ListView
    UpdateListViewItemsSource(m_topNavListView.get(), nullptr);
    UpdateListViewItemsSource(m_topNavListOverflowView.get(), nullptr);

    SyncRootNodesWithItemsSource(items);
}

void NavigationView::UpdateTopNavListViewItemSource(const winrt::IInspectable& items)
{
    if (m_topDataProvider.ShouldChangeDataSource(items))
    {
        // unbinding Data from ListView
        UpdateListViewItemsSource(m_topNavListView.get(), nullptr);
        UpdateListViewItemsSource(m_topNavListOverflowView.get(), nullptr);

        // Change data source and setup vectors
        m_topDataProvider.SetDataSource(items);

        // rebinding
        if (items)
        {
            UpdateListViewItemsSource(m_topNavListView.get(), m_topDataProvider.GetPrimaryItems());
            UpdateListViewItemsSource(m_topNavListOverflowView.get(), m_topDataProvider.GetOverflowItems());
        }
        else
        {
            UpdateListViewItemsSource(m_topNavListView.get(), nullptr);
            UpdateListViewItemsSource(m_topNavListOverflowView.get(), nullptr);
        }
    }
}

void NavigationView::UpdateListViewItemSource()
{
    if (!m_appliedTemplate)
    {
        return;
    }

    auto dataSource = MenuItemsSource();
    if (!dataSource)
    {
        dataSource = MenuItems();
    }

    // Always unset the data source first from old ListView, then set data source for new ListView.
    if (IsTopNavigationView())
    {
        UpdateLeftNavListViewItemSource(nullptr);
        UpdateTopNavListViewItemSource(dataSource);
    }
    else
    {
        UpdateTopNavListViewItemSource(nullptr);
        UpdateLeftNavListViewItemSource(dataSource);
    }
 
    if (IsTopNavigationView())
    {
        InvalidateTopNavPrimaryLayout();
        UpdateSelectedItem();
    }
}

winrt::IVector<winrt::TreeViewNode> NavigationView::RootNodes()
{
    auto x = m_rootNode.get().Children();
    return x;
}

void NavigationView::SyncRootNodesWithItemsSource(winrt::IInspectable const& items)
{
    // All TreeViewNode should be set to 'IsContentMode = true' as we dont want to pass node objects to the list view
    winrt::get_self<TreeViewNode>(m_rootNode.get())->IsContentMode(true);
    winrt::get_self<TreeViewNode>(m_rootNode.get())->ItemsSource(items);

}

void NavigationView::UpdateListViewItemsSource(const winrt::ListView& listView, 
    const winrt::IInspectable& itemsSource)
{
    if (listView)
    {
        auto oldItemsSource = listView.ItemsSource();
        if (oldItemsSource != itemsSource)
        {
            listView.ItemsSource(itemsSource);
        }
    }
}

void NavigationView::OnTitleBarMetricsChanged(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    UpdateTitleBarPadding();
}

void NavigationView::OnTitleBarIsVisibleChanged(const winrt::CoreApplicationViewTitleBar& /*sender*/, const winrt::IInspectable& /*args*/)
{
    UpdateTitleBarPadding();
}

bool NavigationView::ShouldIgnoreMeasureOverride()
{
    return m_shouldIgnoreNextMeasureOverride || m_shouldIgnoreOverflowItemSelectionChange || m_shouldIgnoreNextSelectionChange;
}

bool NavigationView::NeedTopPaddingForRS5OrHigher(winrt::CoreApplicationViewTitleBar const& coreTitleBar)
{
    // Starting on RS5, we will be using the following IsVisible API together with ExtendViewIntoTitleBar
    // to decide whether to try to add top padding or not.
    // We don't add padding when in fullscreen or tablet mode.
    return coreTitleBar.IsVisible() && coreTitleBar.ExtendViewIntoTitleBar()
        && !IsFullScreenOrTabletMode();
}

void NavigationView::UpdateTitleBarPadding()
{
    if (!m_appliedTemplate)
    {
        return;
    }

    double topPadding = 0;
    if (auto coreTitleBar = m_coreTitleBar.get())
    {
        bool needsTopPadding = false;
        if (ShouldPreserveNavigationViewRS3Behavior())
        {
            needsTopPadding = true;
        }
        else if (ShouldPreserveNavigationViewRS4Behavior())
        {
            // For RS4 apps maintain the behavior that we shipped for RS4.
            // We keep this behavior for app compact purposes.
            needsTopPadding = !coreTitleBar.ExtendViewIntoTitleBar();
        }
        else
        {
            needsTopPadding = NeedTopPaddingForRS5OrHigher(coreTitleBar);
        }

        if (needsTopPadding)
        {
            topPadding = coreTitleBar.Height();

            // Only add extra padding if the NavView is the "root" of the app,
            // but not if the app is expanding into the titlebar
            winrt::UIElement root = winrt::Window::Current().Content();
            winrt::GeneralTransform gt = TransformToVisual(root);
            winrt::Point pos = gt.TransformPoint(winrt::Point(0.0f, 0.0f));
            if (pos.Y != 0.0f)
            {
                topPadding = 0.0;
            }

            auto backButtonVisibility = IsBackButtonVisible();
            if (ShouldPreserveNavigationViewRS4Behavior())
            {
                if (auto fe = m_togglePaneTopPadding.get())
                {
                    fe.Height(topPadding);
                }

                if (auto fe = m_contentPaneTopPadding.get())
                {
                    fe.Height(topPadding);
                }
            }
        }

        if (auto paneButton = m_paneToggleButton.get())
        {
            auto thickness = winrt::ThicknessHelper::FromLengths(0, 0, 0, 0);

            if (ShouldShowBackButton())
            {
                if (IsOverlay())
                {
                    thickness = winrt::ThicknessHelper::FromLengths(c_backButtonWidth, 0, 0, 0);
                }
                else
                {
                    thickness = winrt::ThicknessHelper::FromLengths(0, c_backButtonHeight, 0, 0);
                }
            }
            paneButton.Margin(thickness);
        }
    }

    if (auto templateSettings = TemplateSettings())
    {
        // 0.0 and 0.00000000 is not the same in double world. try to reduce the number of TopPadding update event. epsilon is 0.1 here.
        if (fabs(templateSettings.TopPadding() - topPadding) > 0.1)
        {
            GetTemplateSettings()->TopPadding(topPadding);
        }
    }
}

void NavigationView::UpdateSelectedItem()
{
    auto item = SelectedItem();
    auto settingsItem = m_settingsItem.get();
    if (settingsItem && item == settingsItem)
    {
        OnSettingsInvoked();
    }
    else
    {
        auto lv = m_leftNavListView.get();
        if (IsTopNavigationView())
        {
            lv = m_topNavListView.get();
        }

        if (lv)
        {
            lv.SelectedItem(item);
        }
    }
}

void NavigationView::RaiseDisplayModeChanged(const winrt::NavigationViewDisplayMode& displayMode)
{
    SetValue(s_DisplayModeProperty, box_value(displayMode));
    auto eventArgs = winrt::make_self<NavigationViewDisplayModeChangedEventArgs>();
    eventArgs->DisplayMode(displayMode);
    m_displayModeChangedEventSource(*this, *eventArgs);
}

// This method attaches the series of animations which are fired off dependent upon the amount 
// of space give and the length of the strings involved. It occurs upon re-rendering.
void NavigationView::CreateAndAttachHeaderAnimation(winrt::Visual visual) 
{
    auto compositor = visual.Compositor();
    auto cubicFunction = compositor.CreateCubicBezierEasingFunction({ 0.0f, 0.35f }, { 0.15f, 1.0f });
    auto moveAnimation = compositor.CreateVector3KeyFrameAnimation();
    moveAnimation.Target(L"Offset");
    moveAnimation.InsertExpressionKeyFrame(1.0f, L"this.FinalValue", cubicFunction);
    moveAnimation.Duration(200ms);

    auto collection = compositor.CreateImplicitAnimationCollection();
    collection.Insert(L"Offset", moveAnimation);
    visual.ImplicitAnimations(collection);
}

bool NavigationView::IsFullScreenOrTabletMode()
{
    // ApplicationView::GetForCurrentView() is an expensive call - make sure to cache the ApplicationView
    if (!m_applicationView)
    {
        m_applicationView = winrt::ViewManagement::ApplicationView::GetForCurrentView();
    }

    // UIViewSettings::GetForCurrentView() is an expensive call - make sure to cache the UIViewSettings
    if (!m_uiViewSettings)
    {
        m_uiViewSettings = winrt::ViewManagement::UIViewSettings::GetForCurrentView();
    }

    bool isFullScreenMode = m_applicationView.IsFullScreenMode();
    bool isTabletMode = m_uiViewSettings.UserInteractionMode() == winrt::ViewManagement::UserInteractionMode::Touch;

    return isFullScreenMode || isTabletMode;
}

void NavigationView::Expand(winrt::NavigationViewItem const& value)
{

}

void NavigationView::Collapse(winrt::NavigationViewItem const& value)
{

}

winrt::TreeViewNode NavigationView::NodeFromContainer(winrt::DependencyObject const& container)
{
    if (auto lv = GetActiveListView())
    {
        if (auto navListView = lv.try_as<winrt::NavigationViewList>())
        {
            return winrt::get_self<NavigationViewList>(navListView)->NodeFromContainer(container);
        }
    }
    return nullptr;
}

winrt::DependencyObject NavigationView::ContainerFromNode(winrt::TreeViewNode const& node)
{
    if (auto lv = GetActiveListView())
    {
        if (auto navListView = lv.try_as<winrt::NavigationViewList>())
        {
            return winrt::get_self<NavigationViewList>(navListView)->ContainerFromNode(node);
        }
    }
    return nullptr;
}

//TODO: Update to work with Overflow Popup
winrt::ListView NavigationView::GetActiveListView()
{
    return IsTopNavigationView() ? m_topNavListView.get() : m_leftNavListView.get();
}
