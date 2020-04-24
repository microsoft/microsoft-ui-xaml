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
#include "Utils.h"
#include "TraceLogging.h"
#include "NavigationViewItemRevokers.h"
#include "IndexPath.h"
#include "InspectingDataSource.h"
#include "NavigationViewAutomationPeer.h"
#include "StackLayout.h"
#include "ItemsRepeater.h"
#include "ElementFactoryGetArgs.h"
#include "ElementFactoryRecycleArgs.h"
#include <ItemsRepeater.common.h>
#include "NavigationViewItemExpandingEventArgs.h"
#include "NavigationViewItemCollapsedEventArgs.h"

// General items
static constexpr auto c_togglePaneButtonName = L"TogglePaneButton"sv;
static constexpr auto c_paneTitleHolderFrameworkElement = L"PaneTitleHolder"sv;
static constexpr auto c_paneTitleFrameworkElement = L"PaneTitleTextBlock"sv;
static constexpr auto c_rootSplitViewName = L"RootSplitView"sv;
static constexpr auto c_menuItemsHost = L"MenuItemsHost"sv;
static constexpr auto c_settingsName = L"SettingsNavPaneItem"sv;
static constexpr auto c_settingsNameTopNav = L"SettingsTopNavPaneItem"sv;
static constexpr auto c_selectionIndicatorName = L"SelectionIndicator"sv;
static constexpr auto c_paneContentGridName = L"PaneContentGrid"sv;
static constexpr auto c_rootGridName = L"RootGrid"sv;
static constexpr auto c_contentGridName = L"ContentGrid"sv;
static constexpr auto c_searchButtonName = L"PaneAutoSuggestButton"sv;
static constexpr auto c_paneToggleButtonIconGridColumnName = L"PaneToggleButtonIconWidthColumn"sv;
static constexpr auto c_togglePaneTopPadding = L"TogglePaneTopPadding"sv;
static constexpr auto c_contentPaneTopPadding = L"ContentPaneTopPadding"sv;
static constexpr auto c_contentLeftPadding = L"ContentLeftPadding"sv;
static constexpr auto c_navViewBackButton = L"NavigationViewBackButton"sv;
static constexpr auto c_navViewBackButtonToolTip = L"NavigationViewBackButtonToolTip"sv;
static constexpr auto c_navViewCloseButton = L"NavigationViewCloseButton"sv;
static constexpr auto c_navViewCloseButtonToolTip = L"NavigationViewCloseButtonToolTip"sv;
static constexpr auto c_paneShadowReceiverCanvas = L"PaneShadowReceiver"sv;
static constexpr auto c_flyoutRootGrid = L"FlyoutRootGrid"sv;

// DisplayMode Top specific items
static constexpr auto c_topNavMenuItemsHost = L"TopNavMenuItemsHost"sv;
static constexpr auto c_topNavOverflowButton = L"TopNavOverflowButton"sv;
static constexpr auto c_topNavMenuItemsOverflowHost = L"TopNavMenuItemsOverflowHost"sv;
static constexpr auto c_topNavGrid = L"TopNavGrid"sv;
static constexpr auto c_topNavContentOverlayAreaGrid = L"TopNavContentOverlayAreaGrid"sv;
static constexpr auto c_leftNavPaneAutoSuggestBoxPresenter = L"PaneAutoSuggestBoxPresenter"sv;
static constexpr auto c_topNavPaneAutoSuggestBoxPresenter = L"TopPaneAutoSuggestBoxPresenter"sv;
static constexpr auto c_paneTitlePresenter = L"PaneTitlePresenter"sv;

// DisplayMode Left specific items
static constexpr auto c_leftNavFooterContentBorder = L"FooterContentBorder"sv;
static constexpr auto c_leftNavPaneHeaderContentBorder = L"PaneHeaderContentBorder"sv;
static constexpr auto c_leftNavPaneCustomContentBorder = L"PaneCustomContentBorder"sv;

static constexpr auto c_paneHeaderOnTopPane = L"PaneHeaderOnTopPane"sv;
static constexpr auto c_paneTitleOnTopPane = L"PaneTitleOnTopPane"sv;
static constexpr auto c_paneCustomContentOnTopPane = L"PaneCustomContentOnTopPane"sv;
static constexpr auto c_paneFooterOnTopPane = L"PaneFooterOnTopPane"sv;
static constexpr auto c_paneHeaderCloseButtonColumn = L"PaneHeaderCloseButtonColumn"sv;
static constexpr auto c_paneHeaderToggleButtonColumn = L"PaneHeaderToggleButtonColumn"sv;
static constexpr auto c_paneHeaderContentBorderRow = L"PaneHeaderContentBorderRow"sv;

static constexpr int c_backButtonHeight = 40;
static constexpr int c_backButtonWidth = 40;
static constexpr int c_paneToggleButtonHeight = 40;
static constexpr int c_paneToggleButtonWidth = 40;
static constexpr int c_toggleButtonHeightWhenShouldPreserveNavigationViewRS3Behavior = 56;
static constexpr int c_backButtonRowDefinition = 1;
static constexpr float c_paneElevationTranslationZ = 32;

constexpr int s_itemNotFound{ -1 };

static winrt::Size c_infSize{ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };

NavigationView::~NavigationView()
{
    UnhookEventsAndClearFields(true);
}

// IUIElement / IUIElementOverridesHelper
winrt::AutomationPeer NavigationView::OnCreateAutomationPeer()
{
    return winrt::make<NavigationViewAutomationPeer>(*this);
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

    m_paneSearchButtonClickRevoker.revoke();
    m_paneSearchButton.set(nullptr);

    m_paneHeaderOnTopPane.set(nullptr);
    m_paneTitleOnTopPane.set(nullptr);

    m_paneTitleHolderFrameworkElementSizeChangedRevoker.revoke();
    m_paneTitleHolderFrameworkElement.set(nullptr);

    m_paneTitleFrameworkElement.set(nullptr);
    m_paneTitlePresenter.set(nullptr);

    m_paneHeaderCloseButtonColumn.set(nullptr);
    m_paneHeaderToggleButtonColumn.set(nullptr);
    m_paneHeaderContentBorderRow.set(nullptr);

    m_leftNavItemsRepeaterElementPreparedRevoker.revoke();
    m_leftNavItemsRepeaterElementClearingRevoker.revoke();
    m_leftNavRepeaterLoadedRevoker.revoke();
    m_leftNavRepeaterGettingFocusRevoker.revoke();
    m_leftNavRepeater.set(nullptr);

    m_topNavItemsRepeaterElementPreparedRevoker.revoke();
    m_topNavItemsRepeaterElementClearingRevoker.revoke();
    m_topNavRepeaterLoadedRevoker.revoke();
    m_topNavRepeaterGettingFocusRevoker.revoke();
    m_topNavRepeater.set(nullptr);

    m_topNavOverflowItemsRepeaterElementPreparedRevoker.revoke();
    m_topNavOverflowItemsRepeaterElementClearingRevoker.revoke();
    m_topNavRepeaterOverflowView.set(nullptr);

    if (isFromDestructor)
    {
        m_selectionChangedRevoker.revoke();
    }
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
    Loaded({ this, &NavigationView::OnLoaded });

    m_selectionModel.SingleSelect(true);
    m_selectionChangedRevoker = m_selectionModel.SelectionChanged(winrt::auto_revoke, { this, &NavigationView::OnSelectionModelSelectionChanged });
    m_childrenRequestedRevoker = m_selectionModel.ChildrenRequested(winrt::auto_revoke, { this, &NavigationView::OnSelectionModelChildrenRequested });

    m_navigationViewItemsFactory = winrt::make_self<NavigationViewItemsFactory>();

    s_NavigationViewItemRevokersProperty =
        InitializeDependencyProperty(
            L"NavigationViewItemRevokers",
            winrt::name_of<winrt::IInspectable>(),
            winrt::name_of<winrt::NavigationViewItem>(),
            true /* isAttached */,
            nullptr /* defaultValue */);
}

void NavigationView::OnSelectionModelChildrenRequested(const winrt::SelectionModel& selectionModel, const winrt::SelectionModelChildrenRequestedEventArgs& e)
{
    if (auto nvi = e.Source().try_as<winrt::NavigationViewItem>())
    {
        e.Children(GetChildren(nvi));
    }
    else if (auto const children = GetChildrenForItemInIndexPath(e.SourceIndex(), true /*forceRealize*/))
    {
        e.Children(children);
    }
}

void NavigationView::OnSelectionModelSelectionChanged(const winrt::SelectionModel& selectionModel, const winrt::SelectionModelSelectionChangedEventArgs& e)
{
    auto selectedItem = selectionModel.SelectedItem();

    // Ignore this callback if:
    // 1. the SelectedItem property of NavigationView is already set to the item
    //    being passed in this callback. This is because the item has already been selected
    //    via API and we are just updating the m_selectionModel state to accurately reflect the new selection.
    // 2. Template has not been applied yet. SelectionModel's selectedIndex state will get properly updated
    //    after the repeater finishes loading.
    // TODO: Update SelectedItem comparison to work for the exact same item datasource scenario
    if (m_shouldIgnoreNextSelectionChange || selectedItem == SelectedItem() || !m_appliedTemplate)
    {
        return;
    }

    bool setSelectedItem = true;
    auto const selectedIndex = selectionModel.SelectedIndex();
    if (IsTopNavigationView())
    {
        // If selectedIndex does not exist, means item is being deselected through API
        auto isInOverflow = (selectedIndex && selectedIndex.GetSize() > 0) ? !m_topDataProvider.IsItemInPrimaryList(selectedIndex.GetAt(0)) : false;
        if (isInOverflow)
        {
            // We only want to close the overflow flyout and move the item on selection if it is a leaf node
            auto const itemShouldBeMoved = [selectedIndex, this]()
            {
                if (auto const selectedContainer = GetContainerForIndexPath(selectedIndex))
                {
                    if (auto const selectedNVI = selectedContainer.try_as<winrt::NavigationViewItem>())
                    {
                        if (DoesNavigationViewItemHaveChildren(selectedNVI))
                        {
                            return false;
                        }
                    }
                }
                return true;
            }();

            if (itemShouldBeMoved)
            {
                SelectandMoveOverflowItem(selectedItem, selectedIndex, true /*closeFlyout*/);
                setSelectedItem = false;
            }
            else
            {
                m_moveTopNavOverflowItemOnFlyoutClose = true;
            }
        } 
    }

    if (setSelectedItem)
    {
        SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(selectedItem);
    }
}

void NavigationView::SelectandMoveOverflowItem(winrt::IInspectable const& selectedItem, winrt::IndexPath const& selectedIndex, bool closeFlyout)
{
    // SelectOverflowItem is moving data in/out of overflow.
    auto scopeGuard = gsl::finally([this]()
        {
            m_selectionChangeFromOverflowMenu = false;
        });
    m_selectionChangeFromOverflowMenu = true;

    if (closeFlyout)
    {
        CloseTopNavigationViewFlyout();
    }

    if (!IsSelectionSuppressed(selectedItem))
    {
        SelectOverflowItem(selectedItem, selectedIndex);
    }
}

// We only need to close the flyout if the selected item is a leaf node
void NavigationView::CloseFlyoutIfRequired(const winrt::NavigationViewItem& selectedItem)
{
    auto const selectedIndex = m_selectionModel.SelectedIndex();
    bool isInModeWithFlyout = [this]()
    {
        if (auto splitView = m_rootSplitView.get())
        {
            // Check if the pane is closed and if the splitview is in either compact mode.
            auto splitViewDisplayMode = splitView.DisplayMode();
            return (!splitView.IsPaneOpen() && (splitViewDisplayMode == winrt::SplitViewDisplayMode::CompactOverlay || splitViewDisplayMode == winrt::SplitViewDisplayMode::CompactInline)) ||
                    PaneDisplayMode() == winrt::NavigationViewPaneDisplayMode::Top;
        }
        return false;
    }();

    if (isInModeWithFlyout && selectedIndex && !DoesNavigationViewItemHaveChildren(selectedItem))
    {
        // Item selected is a leaf node, find top level parent and close flyout
        if (auto const rootItem = GetContainerForIndex(selectedIndex.GetAt(0)))
        {
            if (auto const nvi = rootItem.try_as<winrt::NavigationViewItem>())
            {
                auto const nviImpl = winrt::get_self<NavigationViewItem>(nvi);
                if (nviImpl->ShouldRepeaterShowInFlyout())
                {
                    nvi.IsExpanded(false);
                }
            }
        }
    }
}

void NavigationView::OnApplyTemplate()
{
    // Stop update anything because of PropertyChange during OnApplyTemplate. Update them all together at the end of this function
    m_appliedTemplate = false;

    UnhookEventsAndClearFields();

    winrt::IControlProtected controlProtected = *this;

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

    m_leftNavPaneHeaderContentBorder.set(GetTemplateChildT<winrt::ContentControl>(c_leftNavPaneHeaderContentBorder, controlProtected));
    m_leftNavPaneCustomContentBorder.set(GetTemplateChildT<winrt::ContentControl>(c_leftNavPaneCustomContentBorder, controlProtected));
    m_leftNavFooterContentBorder.set(GetTemplateChildT<winrt::ContentControl>(c_leftNavFooterContentBorder, controlProtected));
    m_paneHeaderOnTopPane.set(GetTemplateChildT<winrt::ContentControl>(c_paneHeaderOnTopPane, controlProtected));
    m_paneTitleOnTopPane.set(GetTemplateChildT<winrt::ContentControl>(c_paneTitleOnTopPane, controlProtected));
    m_paneCustomContentOnTopPane.set(GetTemplateChildT<winrt::ContentControl>(c_paneCustomContentOnTopPane, controlProtected));
    m_paneFooterOnTopPane.set(GetTemplateChildT<winrt::ContentControl>(c_paneFooterOnTopPane, controlProtected));

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

    m_topNavGrid.set(GetTemplateChildT<winrt::Grid>(c_topNavGrid, controlProtected));

    // Change code to NOT do this if we're in top nav mode, to prevent it from being realized:
    if (auto leftNavRepeater = GetTemplateChildT<winrt::ItemsRepeater>(c_menuItemsHost, controlProtected))
    {
        m_leftNavRepeater.set(leftNavRepeater);

        // API is currently in preview, so setting this via code.
        // Disabling virtualization for now because of https://github.com/microsoft/microsoft-ui-xaml/issues/2095
        if (auto stackLayout = leftNavRepeater.Layout().try_as<winrt::StackLayout>())
        {
            auto stackLayoutImpl = winrt::get_self<StackLayout>(stackLayout);
            stackLayoutImpl->DisableVirtualization(true);
        }

        m_leftNavItemsRepeaterElementPreparedRevoker = leftNavRepeater.ElementPrepared(winrt::auto_revoke, { this, &NavigationView::OnRepeaterElementPrepared });
        m_leftNavItemsRepeaterElementClearingRevoker = leftNavRepeater.ElementClearing(winrt::auto_revoke, { this, &NavigationView::OnRepeaterElementClearing });
        m_leftNavRepeaterGettingFocusRevoker = leftNavRepeater.GettingFocus(winrt::auto_revoke, { this, &NavigationView::OnRepeaterGettingFocus });

        m_leftNavRepeaterLoadedRevoker = leftNavRepeater.Loaded(winrt::auto_revoke, { this, &NavigationView::OnRepeaterLoaded });

        leftNavRepeater.ItemTemplate(*m_navigationViewItemsFactory);
    }

    // Change code to NOT do this if we're in left nav mode, to prevent it from being realized:
    if (auto topNavRepeater = GetTemplateChildT<winrt::ItemsRepeater>(c_topNavMenuItemsHost, controlProtected))
    {
        m_topNavRepeater.set(topNavRepeater);

        // API is currently in preview, so setting this via code
        if (auto stackLayout = topNavRepeater.Layout().try_as<winrt::StackLayout>())
        {
            auto stackLayoutImpl = winrt::get_self<StackLayout>(stackLayout);
            stackLayoutImpl->DisableVirtualization(true);
        }

        m_topNavItemsRepeaterElementPreparedRevoker = topNavRepeater.ElementPrepared(winrt::auto_revoke, { this, &NavigationView::OnRepeaterElementPrepared });
        m_topNavItemsRepeaterElementClearingRevoker = topNavRepeater.ElementClearing(winrt::auto_revoke, { this, &NavigationView::OnRepeaterElementClearing });
        m_topNavRepeaterGettingFocusRevoker = topNavRepeater.GettingFocus(winrt::auto_revoke, { this, &NavigationView::OnRepeaterGettingFocus });

        m_topNavRepeaterLoadedRevoker = topNavRepeater.Loaded(winrt::auto_revoke, { this, &NavigationView::OnRepeaterLoaded });

        topNavRepeater.ItemTemplate(*m_navigationViewItemsFactory);
    }

    // Change code to NOT do this if we're in left nav mode, to prevent it from being realized:
    if (auto topNavListOverflowRepeater = GetTemplateChildT<winrt::ItemsRepeater>(c_topNavMenuItemsOverflowHost, controlProtected))
    {
        m_topNavRepeaterOverflowView.set(topNavListOverflowRepeater);

        // API is currently in preview, so setting this via code.
        // Disabling virtualization for now because of https://github.com/microsoft/microsoft-ui-xaml/issues/2095
        if (auto stackLayout = topNavListOverflowRepeater.Layout().try_as<winrt::StackLayout>())
        {
            auto stackLayoutImpl = winrt::get_self<StackLayout>(stackLayout);
            stackLayoutImpl->DisableVirtualization(true);
        }

        m_topNavOverflowItemsRepeaterElementPreparedRevoker = topNavListOverflowRepeater.ElementPrepared(winrt::auto_revoke, { this, &NavigationView::OnRepeaterElementPrepared });
        m_topNavOverflowItemsRepeaterElementClearingRevoker = topNavListOverflowRepeater.ElementClearing(winrt::auto_revoke, { this, &NavigationView::OnRepeaterElementClearing });

        topNavListOverflowRepeater.ItemTemplate(*m_navigationViewItemsFactory);
    }

    if (auto topNavOverflowButton = GetTemplateChildT<winrt::Button>(c_topNavOverflowButton, controlProtected))
    {
        m_topNavOverflowButton.set(topNavOverflowButton);
        winrt::AutomationProperties::SetName(topNavOverflowButton, ResourceAccessor::GetLocalizedStringResource(SR_NavigationOverflowButtonText));
        topNavOverflowButton.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_NavigationOverflowButtonText)));
        auto visual = winrt::ElementCompositionPreview::GetElementVisual(topNavOverflowButton);
        CreateAndAttachHeaderAnimation(visual);

        if (auto const flyoutBase = topNavOverflowButton.Flyout())
        {
            if (winrt::IFlyoutBase6 topNavOverflowButtonAsFlyoutBase6 = flyoutBase)
            {
                topNavOverflowButtonAsFlyoutBase6.ShouldConstrainToRootBounds(false);
            }
            m_flyoutClosingRevoker = flyoutBase.Closing(winrt::auto_revoke, { this, &NavigationView::OnFlyoutClosing });
        }
    }

    m_topNavContentOverlayAreaGrid.set(GetTemplateChildT<winrt::Border>(c_topNavContentOverlayAreaGrid, controlProtected));
    m_leftNavPaneAutoSuggestBoxPresenter.set(GetTemplateChildT<winrt::ContentControl>(c_leftNavPaneAutoSuggestBoxPresenter, controlProtected));
    m_topNavPaneAutoSuggestBoxPresenter.set(GetTemplateChildT<winrt::ContentControl>(c_topNavPaneAutoSuggestBoxPresenter, controlProtected));

    // Get pointer to the pane content area, for use in the selection indicator animation
    m_paneContentGrid.set(GetTemplateChildT<winrt::UIElement>(c_paneContentGridName, controlProtected));

    m_contentLeftPadding.set(GetTemplateChildT<winrt::FrameworkElement>(c_contentLeftPadding, controlProtected));

    m_paneHeaderCloseButtonColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(c_paneHeaderCloseButtonColumn, controlProtected));
    m_paneHeaderToggleButtonColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(c_paneHeaderToggleButtonColumn, controlProtected));
    m_paneHeaderContentBorderRow.set(GetTemplateChildT<winrt::RowDefinition>(c_paneHeaderContentBorderRow, controlProtected));
    m_paneTitleFrameworkElement.set(GetTemplateChildT<winrt::FrameworkElement>(c_paneTitleFrameworkElement, controlProtected));
    m_paneTitlePresenter.set(GetTemplateChildT<winrt::ContentControl>(c_paneTitlePresenter, controlProtected));

    if (auto paneTitleHolderFrameworkElement = GetTemplateChildT<winrt::FrameworkElement>(c_paneTitleHolderFrameworkElement, controlProtected))
    {
        m_paneTitleHolderFrameworkElement.set(paneTitleHolderFrameworkElement);
        m_paneTitleHolderFrameworkElementSizeChangedRevoker = paneTitleHolderFrameworkElement.SizeChanged(winrt::auto_revoke, { this, &NavigationView::OnPaneTitleHolderSizeChanged });
    }

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

    // Register for changes in title bar layout
    if (auto coreTitleBar = winrt::CoreApplication::GetCurrentView().TitleBar())
    {
        m_coreTitleBar.set(coreTitleBar);
        m_titleBarMetricsChangedRevoker = coreTitleBar.LayoutMetricsChanged(winrt::auto_revoke, { this, &NavigationView::OnTitleBarMetricsChanged });
        m_titleBarIsVisibleChangedRevoker = coreTitleBar.IsVisibleChanged(winrt::auto_revoke, { this, &NavigationView::OnTitleBarIsVisibleChanged });

        if (ShouldPreserveNavigationViewRS4Behavior())
        {
            m_togglePaneTopPadding.set(GetTemplateChildT<winrt::FrameworkElement>(c_togglePaneTopPadding, controlProtected));
            m_contentPaneTopPadding.set(GetTemplateChildT<winrt::FrameworkElement>(c_contentPaneTopPadding, controlProtected));
        }
    }

    if (auto backButtonToolTip = GetTemplateChildT<winrt::ToolTip>(c_navViewBackButtonToolTip, controlProtected))
    {
        winrt::hstring navigationBackButtonToolTip = ResourceAccessor::GetLocalizedStringResource(SR_NavigationBackButtonToolTip);
        backButtonToolTip.Content(box_value(navigationBackButtonToolTip));
    }

    if (auto closeButton = GetTemplateChildT<winrt::Button>(c_navViewCloseButton, controlProtected))
    {
        m_closeButton.set(closeButton);
        m_closeButtonClickedRevoker = closeButton.Click(winrt::auto_revoke, { this, &NavigationView::OnPaneToggleButtonClick });

        winrt::hstring navigationName = ResourceAccessor::GetLocalizedStringResource(SR_NavigationCloseButtonName);
        winrt::AutomationProperties::SetName(closeButton, navigationName);
    }

    if (auto closeButtonToolTip = GetTemplateChildT<winrt::ToolTip>(c_navViewCloseButtonToolTip, controlProtected))
    {
        winrt::hstring navigationCloseButtonToolTip = ResourceAccessor::GetLocalizedStringResource(SR_NavigationButtonOpenName);
        closeButtonToolTip.Content(box_value(navigationCloseButtonToolTip));
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

    m_accessKeyInvokedRevoker = AccessKeyInvoked(winrt::auto_revoke, { this, &NavigationView::OnAccessKeyInvoked });

    UpdatePaneShadow();

    m_appliedTemplate = true;

    // Do initial setup
    UpdatePaneDisplayMode();
    UpdateHeaderVisibility();
    UpdatePaneTitleFrameworkElementParents();
    UpdateTitleBarPadding();
    UpdatePaneTabFocusNavigation();
    UpdateBackAndCloseButtonsVisibility();
    UpdateSingleSelectionFollowsFocusTemplateSetting();
    UpdateNavigationViewUseSystemVisual();
    UpdatePaneVisibility();
    UpdateVisualState();
    UpdatePaneTitleMargins();
}

void NavigationView::UpdateRepeaterItemsSource(bool forceSelectionModelUpdate)
{
    auto const itemsSource = [this]()
    {
        if (auto const menuItemsSource = MenuItemsSource())
        {
            return menuItemsSource;
        }
        UpdateSelectionForMenuItems();
        return MenuItems().as<winrt::IInspectable>();
    }();

    // Selection Model has same representation of data regardless
    // of pane mode, so only update if the ItemsSource data itself
    // has changed.
    if (forceSelectionModelUpdate)
    {
        m_selectionModel.Source(itemsSource);
    }

    if (IsTopNavigationView())
    {
        UpdateLeftRepeaterItemSource(nullptr);
        UpdateTopNavRepeatersItemSource(itemsSource);
        InvalidateTopNavPrimaryLayout();
        SyncSettingsSelectionState();
    }
    else
    {
        UpdateTopNavRepeatersItemSource(nullptr);
        UpdateLeftRepeaterItemSource(itemsSource);
    }
}

void NavigationView::UpdateLeftRepeaterItemSource(const winrt::IInspectable& items)
{
    UpdateItemsRepeaterItemsSource(m_leftNavRepeater.get(), items);
}

void NavigationView::UpdateTopNavRepeatersItemSource(const winrt::IInspectable& items)
{
    // Change data source and setup vectors
    m_topDataProvider.SetDataSource(items);

    // rebinding
    if (items)
    {
        UpdateItemsRepeaterItemsSource(m_topNavRepeater.get(), m_topDataProvider.GetPrimaryItems());
        UpdateItemsRepeaterItemsSource(m_topNavRepeaterOverflowView.get(), m_topDataProvider.GetOverflowItems());
    }
    else
    {
        UpdateItemsRepeaterItemsSource(m_topNavRepeater.get(), nullptr);
        UpdateItemsRepeaterItemsSource(m_topNavRepeaterOverflowView.get(), nullptr);
    }
}

void NavigationView::UpdateItemsRepeaterItemsSource(const winrt::ItemsRepeater& ir,
    const winrt::IInspectable& itemsSource)
{
    if (ir)
    {
        ir.ItemsSource(itemsSource);
    }
}

void NavigationView::OnFlyoutClosing(const winrt::IInspectable& sender, const winrt::FlyoutBaseClosingEventArgs& args)
{
    // If the user selected an parent item in the overflow flyout then the item has not been moved to top primary yet.
    // So we need to move it.
    if (m_moveTopNavOverflowItemOnFlyoutClose && !m_selectionChangeFromOverflowMenu)
    {
        m_moveTopNavOverflowItemOnFlyoutClose = false;

        auto const selectedIndex = m_selectionModel.SelectedIndex();
        if (selectedIndex.GetSize() > 0)
        {
            if (auto const firstContainer = GetContainerForIndex(selectedIndex.GetAt(0)))
            {
                if (auto const firstNVI = firstContainer.try_as<winrt::NavigationViewItem>())
                {
                    // We want to collapse the top level item before we move it
                    firstNVI.IsExpanded(false);
                }
            }

            SelectandMoveOverflowItem(SelectedItem(), selectedIndex, false);
        }
    }
}

void NavigationView::OnNavigationViewItemIsSelectedPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    if (auto const nvi = sender.try_as<winrt::NavigationViewItem>())
    {
        // Check whether the container that triggered this call back is the selected container
        bool isContainerSelectedInModel = IsContainerTheSelectedItemInTheSelectionModel(nvi);
        bool isSelectedInContainer = nvi.IsSelected();

        if (isSelectedInContainer && !isContainerSelectedInModel)
        {
            auto indexPath = GetIndexPathForContainer(nvi);
            UpdateSelectionModelSelection(indexPath);
        }
        else if (!isSelectedInContainer && isContainerSelectedInModel)
        {
            auto indexPath = GetIndexPathForContainer(nvi);
            auto indexPathFromModel = m_selectionModel.SelectedIndex();

            if (indexPathFromModel && indexPath.CompareTo(indexPathFromModel) == 0)
            {
                m_selectionModel.DeselectAt(indexPath);
            }
        }

        if (isSelectedInContainer)
        {
            nvi.IsChildSelected(false);
        }
    }
}

void NavigationView::OnNavigationViewItemExpandedPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    if (auto const nvi = sender.try_as<winrt::NavigationViewItem>())
    {
        if (nvi.IsExpanded())
        {
            RaiseExpandingEvent(nvi);
        }

        ShowHideChildrenItemsRepeater(nvi);

        if (!nvi.IsExpanded())
        {
            RaiseCollapsedEvent(nvi);
        }
    }
}

void NavigationView::RaiseItemInvokedForNavigationViewItem(const winrt::NavigationViewItem& nvi)
{
    winrt::IInspectable nextItem = nullptr;
    auto prevItem = SelectedItem();
    auto parentIR = GetParentItemsRepeaterForContainer(nvi);

    if (auto itemsSourceView = parentIR.ItemsSourceView())
    {
        auto inspectingDataSource = static_cast<InspectingDataSource*>(winrt::get_self<ItemsSourceView>(itemsSourceView));
        auto itemIndex = parentIR.GetElementIndex(nvi);
        nextItem = inspectingDataSource->GetAt(itemIndex);
    }

    // Determine the recommeded transition direction.
    // Any transitions other than `Default` only apply in top nav scenarios.
    auto recommendedDirection = [this, prevItem, nvi, parentIR]()
    {
        if (IsTopNavigationView() && nvi.SelectsOnInvoked())
        {
            bool isInOverflow = parentIR == m_topNavRepeaterOverflowView.get();
            if (isInOverflow)
            {
                return NavigationRecommendedTransitionDirection::FromOverflow;
            }
            else if (prevItem)
            {
                return GetRecommendedTransitionDirection(NavigationViewItemBaseOrSettingsContentFromData(prevItem), nvi);
            }
        }
        return NavigationRecommendedTransitionDirection::Default;
    }();

    RaiseItemInvoked(nextItem, false /*isSettings*/, nvi, recommendedDirection);
}

void NavigationView::OnNavigationViewItemInvoked(const winrt::NavigationViewItem& nvi)
{
    m_shouldRaiseItemInvokedAfterSelection = true;

    auto scopeGuard = gsl::finally([this]()
    {
        m_shouldRaiseItemInvokedAfterSelection = false;
    });

    auto selectedItem = SelectedItem();
    bool updateSelection = m_selectionModel && nvi.SelectsOnInvoked();
    if (updateSelection)
    {
        auto ip = GetIndexPathForContainer(nvi);
        UpdateSelectionModelSelection(ip);
    }

    // Item was invoked but already selected, so raise event here.
    if (selectedItem == SelectedItem())
    {
        RaiseItemInvokedForNavigationViewItem(nvi);
    }

    ToggleIsExpandedNavigationViewItem(nvi);
    ClosePaneIfNeccessaryAfterItemIsClicked(nvi);

    if (updateSelection)
    {
        CloseFlyoutIfRequired(nvi);
    }
}

bool NavigationView::IsRootItemsRepeater(const winrt::DependencyObject& element)
{
    if (element)
    {
        return (element == m_topNavRepeater.get() ||
            element == m_leftNavRepeater.get() ||
            element == m_topNavRepeaterOverflowView.get());
    }
    return false;
}

bool NavigationView::IsRootGridOfFlyout(const winrt::DependencyObject& element)
{
    if (auto grid = element.try_as<winrt::Grid>())
    {
        return grid.Name() == c_flyoutRootGrid;
    }
    return false;
}

winrt::ItemsRepeater NavigationView::GetParentItemsRepeaterForContainer(const winrt::NavigationViewItemBase& nvib)
{
    if (auto parent = winrt::VisualTreeHelper::GetParent(nvib))
    {
        if (auto parentIR = parent.try_as<winrt::ItemsRepeater>())
        {
            return parentIR;
        }
    }
    return nullptr;
}

winrt::NavigationViewItem NavigationView::GetParentNavigationViewItemForContainer(const winrt::NavigationViewItemBase& nvib)
{
    // TODO: This scenario does not find parent items when in a flyout, which causes problems if item if first loaded
    // straight in the flyout. Fix. This logic can be merged with the 'GetIndexPathForContainer' logic below.
    winrt::DependencyObject parent = GetParentItemsRepeaterForContainer(nvib);
    if (!IsRootItemsRepeater(parent))
    {
        while (parent)
        {
            parent = winrt::VisualTreeHelper::GetParent(parent);
            if (auto const nvi = parent.try_as<winrt::NavigationViewItem>())
            {
                return nvi;
            }
        }
    }
    return nullptr;
}

winrt::IndexPath NavigationView::GetIndexPathForContainer(const winrt::NavigationViewItemBase& nvib)
{
    auto path = std::vector<int>();

    winrt::DependencyObject child = nvib;
    auto parent = winrt::VisualTreeHelper::GetParent(child);
    if (!parent)
    {
        return IndexPath::CreateFromIndices(path);
    }

    // Search through VisualTree for a root itemsrepeater
    while (parent && !IsRootItemsRepeater(parent) && !IsRootGridOfFlyout(parent))
    {
        if (auto parentIR = parent.try_as<winrt::ItemsRepeater>())
        {
            if (auto childElement = child.try_as<winrt::UIElement>())
            {
                path.insert(path.begin(), parentIR.GetElementIndex(childElement));
            }
        }
        child = parent;
        parent = winrt::VisualTreeHelper::GetParent(parent);
    }

    // If the item is in a flyout, then we need to final index of its parent
    if (IsRootGridOfFlyout(parent))
    {
        if (auto const nvi = m_lastItemExpandedIntoFlyout.get())
        {
            child = nvi;
            parent = IsTopNavigationView() ? m_topNavRepeater.get() : m_leftNavRepeater.get();
        }
    }

    // If item is in one of the disconnected ItemRepeaters, account for that in IndexPath calculations
    if (parent == m_topNavRepeaterOverflowView.get())
    {
        // Convert index of selected item in overflow to index in datasource
        auto containerIndex = m_topNavRepeaterOverflowView.get().GetElementIndex(child.try_as<winrt::UIElement>());
        auto item = m_topDataProvider.GetOverflowItems().GetAt(containerIndex);
        auto indexAtRoot = m_topDataProvider.IndexOf(item);
        path.insert(path.begin(), indexAtRoot);
    }
    else if (parent == m_topNavRepeater.get())
    {
        // Convert index of selected item in overflow to index in datasource
        auto containerIndex = m_topNavRepeater.get().GetElementIndex(child.try_as<winrt::UIElement>());
        auto item = m_topDataProvider.GetPrimaryItems().GetAt(containerIndex);
        auto indexAtRoot = m_topDataProvider.IndexOf(item);
        path.insert(path.begin(), indexAtRoot);
    }
    else if (auto parentIR = parent.try_as<winrt::ItemsRepeater>())
    {
        path.insert(path.begin(), parentIR.GetElementIndex(child.try_as<winrt::UIElement>()));
    }

    return IndexPath::CreateFromIndices(path);
}

void NavigationView::OnRepeaterElementPrepared(const winrt::ItemsRepeater& ir, const winrt::ItemsRepeaterElementPreparedEventArgs& args)
{
    // This validation is only relevant outside of the Windows build where WUXC and MUXC have distinct types.
    // Certain items are disallowed in a NavigationView's items list. Check for them.
    if (args.Element().try_as<winrt::Windows::UI::Xaml::Controls::NavigationViewItemBase>())
    {
        throw winrt::hresult_invalid_argument(L"MenuItems contains a Windows.UI.Xaml.Controls.NavigationViewItem. This control requires that the NavigationViewItems be of type Microsoft.UI.Xaml.Controls.NavigationViewItem.");
    }

    if (auto nvib = args.Element().try_as<winrt::NavigationViewItemBase>())
    {
        auto nvibImpl = winrt::get_self<NavigationViewItemBase>(nvib);
        nvibImpl->SetNavigationViewParent(*this);
        nvibImpl->IsTopLevelItem(IsTopLevelItem(nvib));

        // Visual state info propagation
        auto position = [this, ir]()
        {
            if (IsTopNavigationView())
            {
                if (ir == m_topNavRepeater.get())
                {
                    return NavigationViewRepeaterPosition::TopPrimary;
                }
                return NavigationViewRepeaterPosition::TopOverflow;
            }
            return NavigationViewRepeaterPosition::LeftNav;
        }();
        nvibImpl->Position(position);

        if (auto const parentNVI = GetParentNavigationViewItemForContainer(nvib))
        {
            auto const parentNVIImpl = winrt::get_self<NavigationViewItem>(parentNVI);
            auto itemDepth = parentNVIImpl->ShouldRepeaterShowInFlyout() ? 0 : parentNVIImpl->Depth() + 1;
            nvibImpl->Depth(itemDepth);
        }
        else
        {
            nvibImpl->Depth(0);
        }

        // Apply any custom container styling
        ApplyCustomMenuItemContainerStyling(nvib, ir, args.Index());

        if (auto nvi = args.Element().try_as<winrt::NavigationViewItem>())
        {
            // Propagate depth to children items if they exist
            auto childDepth = [position, nvibImpl]()
            {
                if (position == NavigationViewRepeaterPosition::TopPrimary)
                {
                    return 0;
                }
                return nvibImpl->Depth() + 1;

            }();
            winrt::get_self<NavigationViewItem>(nvi)->PropagateDepthToChildren(childDepth);

            if (ir != m_topNavRepeaterOverflowView.get())
            {
                nvibImpl->UseSystemFocusVisuals(ShouldShowFocusVisual());
            }

            // Register for item events
            auto nviRevokers = winrt::make_self<NavigationViewItemRevokers>();
            nviRevokers->tappedRevoker = nvi.Tapped(winrt::auto_revoke, { this, &NavigationView::OnNavigationViewItemTapped });
            nviRevokers->keyDownRevoker = nvi.KeyDown(winrt::auto_revoke, { this, &NavigationView::OnNavigationViewItemKeyDown });
            nviRevokers->keyUpRevoker = nvi.KeyUp(winrt::auto_revoke, { this, &NavigationView::OnNavigationViewItemKeyUp });
            nviRevokers->gotFocusRevoker = nvi.GotFocus(winrt::auto_revoke, { this, &NavigationView::OnNavigationViewItemOnGotFocus });
            nviRevokers->isSelectedRevoker = RegisterPropertyChanged(nvi, winrt::NavigationViewItemBase::IsSelectedProperty(), { this, &NavigationView::OnNavigationViewItemIsSelectedPropertyChanged });
            nviRevokers->isExpandedRevoker = RegisterPropertyChanged(nvi, winrt::NavigationViewItem::IsExpandedProperty(), { this, &NavigationView::OnNavigationViewItemExpandedPropertyChanged });
            nvi.SetValue(s_NavigationViewItemRevokersProperty, nviRevokers.as<winrt::IInspectable>());
        }
    }
}

void NavigationView::ApplyCustomMenuItemContainerStyling(const winrt::NavigationViewItemBase& nvib, const winrt::ItemsRepeater& ir, int index)
{
    if (auto menuItemContainerStyle = MenuItemContainerStyle())
    {
        nvib.Style(menuItemContainerStyle);
    }
    else if (auto menuItemContainerStyleSelector = MenuItemContainerStyleSelector())
    {
        if (auto itemsSourceView = ir.ItemsSourceView())
        {
            if (auto item = itemsSourceView.GetAt(index))
            {
                if (auto selectedStyle = menuItemContainerStyleSelector.SelectStyle(item, nvib))
                {
                    nvib.Style(selectedStyle);
                }
            }
        }
    }
}

void NavigationView::OnRepeaterElementClearing(const winrt::ItemsRepeater& ir, const winrt::ItemsRepeaterElementClearingEventArgs& args)
{
    if (auto nvib = args.Element().try_as<winrt::NavigationViewItemBase>())
    {
        auto const nvibImpl = winrt::get_self<NavigationViewItemBase>(nvib);
        nvibImpl->Depth(0);
        nvibImpl->IsTopLevelItem(false);
        if (auto nvi = nvib.try_as<winrt::NavigationViewItem>())
        {
            // Revoke all the events that we were listing to on the item
            nvi.SetValue(s_NavigationViewItemRevokersProperty, nullptr);
        }
    }
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
            auto scopeGuard = gsl::finally([this]()
            {
                    m_shouldIgnoreNextSelectionChangeBecauseSettingsRestore = false;
            });
            m_shouldIgnoreNextSelectionChangeBecauseSettingsRestore = true;
            SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(nullptr);
        }

        m_settingsItemTappedRevoker.revoke();
        m_settingsItemKeyDownRevoker.revoke();
        m_settingsItemKeyUpRevoker.revoke();

        m_settingsItem.set(settingsItem);
        m_settingsItemTappedRevoker = settingsItem.Tapped(winrt::auto_revoke, { this, &NavigationView::OnNavigationViewItemTapped });
        m_settingsItemKeyDownRevoker = settingsItem.KeyDown(winrt::auto_revoke, { this, &NavigationView::OnNavigationViewItemKeyDown });
        m_settingsItemKeyUpRevoker = settingsItem.KeyUp(winrt::auto_revoke, { this, &NavigationView::OnNavigationViewItemKeyUp });

        auto nvibImpl = winrt::get_self<NavigationViewItem>(settingsItem);
        nvibImpl->SetNavigationViewParent(*this);

        // Do localization for settings item label and Automation Name
        auto localizedSettingsName = ResourceAccessor::GetLocalizedStringResource(SR_SettingsButtonName);
        winrt::AutomationProperties::SetName(settingsItem, localizedSettingsName);
        settingsItem.Tag(box_value(localizedSettingsName));

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
            auto scopeGuard = gsl::finally([this]()
            {
                    m_shouldIgnoreNextSelectionChangeBecauseSettingsRestore = false;
            });
            m_shouldIgnoreNextSelectionChangeBecauseSettingsRestore = true;
            SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(m_settingsItem.get());
        }
    }
}

winrt::Size NavigationView::MeasureOverride(winrt::Size const& availableSize)
{
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

    return __super::MeasureOverride(availableSize);
}

void NavigationView::OnLayoutUpdated(const winrt::IInspectable& sender, const winrt::IInspectable& e)
{
    // We only need to handle once after MeasureOverride, so revoke the token.
    m_layoutUpdatedToken.revoke();

    // In topnav, when an item in overflow menu is clicked, the animation is delayed because that item is not move to primary list yet.
    // And it depends on LayoutUpdated to re-play the animation. m_lastSelectedItemPendingAnimationInTopNav is the last selected overflow item.
    if (auto lastSelectedItemInTopNav = m_lastSelectedItemPendingAnimationInTopNav.get())
    {
        m_lastSelectedItemPendingAnimationInTopNav.set(nullptr);
        AnimateSelectionChanged(lastSelectedItemInTopNav);
    }
}

void NavigationView::OnSizeChanged(winrt::IInspectable const& /*sender*/, winrt::SizeChangedEventArgs const& args)
{
    auto width = args.NewSize().Width;
    UpdateAdaptiveLayout(width);
    UpdateTitleBarPadding();
    UpdateBackAndCloseButtonsVisibility();
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

    if (!forceSetDisplayMode && m_InitialNonForcedModeUpdate) {
        if (displayMode == winrt::NavigationViewDisplayMode::Minimal ||
            displayMode == winrt::NavigationViewDisplayMode::Compact) {
            ClosePane();
        }
        m_InitialNonForcedModeUpdate = false;
    }

    auto previousMode = DisplayMode();
    SetDisplayMode(displayMode, forceSetDisplayMode);

    if (displayMode == winrt::NavigationViewDisplayMode::Expanded && IsPaneVisible())
    {
        if (!m_wasForceClosed)
        {
            OpenPane();
        }
    }

    if (previousMode == winrt::NavigationViewDisplayMode::Expanded
        && displayMode == winrt::NavigationViewDisplayMode::Compact)
    {
        m_initialListSizeStateSet = false;
        ClosePane();
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

void NavigationView::OnPaneTitleHolderSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& /*args*/)
{
    UpdateBackAndCloseButtonsVisibility();
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
    CollapseMenuItemsInRepeater(m_leftNavRepeater.get());
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
            if (auto paneList = m_leftNavRepeater.get())
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
    if (m_leftNavRepeater)
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
        UpdateBackAndCloseButtonsVisibility();
        UpdatePaneTitleMargins();
        UpdatePaneToggleSize();
    }
}

void NavigationView::UpdatePaneButtonsWidths()
{
    auto newButtonWidths = [this]()
    {
        if (DisplayMode() == winrt::NavigationViewDisplayMode::Minimal)
        {
            return static_cast<double>(c_paneToggleButtonWidth);
        }
        return CompactPaneLength();
    }();

    if (auto&& backButton = m_backButton.get())
    {
        backButton.Width(newButtonWidths);
    }
    if (auto&& paneToggleButton = m_paneToggleButton.get())
    {
        paneToggleButton.MinWidth(newButtonWidths);
        if (const auto iconGridColumnElement = paneToggleButton.GetTemplateChild(c_paneToggleButtonIconGridColumnName))
        {
            if (const auto paneToggleButtonIconColumn = iconGridColumnElement.try_as<winrt::ColumnDefinition>())
            {
                auto width = paneToggleButtonIconColumn.Width();
                width.Value = newButtonWidths;
                paneToggleButtonIconColumn.Width(width);
            }
        }
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

        return ShouldShowBackOrCloseButton();
    }

    return false;
}

bool NavigationView::ShouldShowCloseButton()
{
    if (m_backButton && !ShouldPreserveNavigationViewRS3Behavior() && m_closeButton)
    {
        if (!IsPaneOpen())
        {
            return false;
        }

        auto paneDisplayMode = PaneDisplayMode();

        if (paneDisplayMode != winrt::NavigationViewPaneDisplayMode::LeftMinimal &&
            (paneDisplayMode != winrt::NavigationViewPaneDisplayMode::Auto || DisplayMode() != winrt::NavigationViewDisplayMode::Minimal))
        {
            return false;
        }

        return ShouldShowBackOrCloseButton();
    }

    return false;
}

bool NavigationView::ShouldShowBackOrCloseButton()
{
    auto visibility = IsBackButtonVisible();
    return (visibility == winrt::NavigationViewBackButtonVisible::Visible || (visibility == winrt::NavigationViewBackButtonVisible::Auto && !SharedHelpers::IsOnXbox()));
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

// Updates the PaneTitleHolder.Visibility and PaneTitleTextBlock.Parent properties based on the PaneDisplayMode, PaneTitle and IsPaneToggleButtonVisible properties.
void NavigationView::UpdatePaneTitleFrameworkElementParents()
{
    if (auto&& paneTitleHolderFrameworkElement = m_paneTitleHolderFrameworkElement.get())
    {
        auto isPaneToggleButtonVisible = IsPaneToggleButtonVisible();
        auto isTopNavigationView = IsTopNavigationView();

        paneTitleHolderFrameworkElement.Visibility(
            (isPaneToggleButtonVisible ||
                isTopNavigationView ||
                PaneTitle().size() == 0 ||
                (PaneDisplayMode() == winrt::NavigationViewPaneDisplayMode::LeftMinimal && !IsPaneOpen())) ?
            winrt::Visibility::Collapsed : winrt::Visibility::Visible);

        if (auto&& paneTitleFrameworkElement = m_paneTitleFrameworkElement.get())
        {
            const auto first = SetPaneTitleFrameworkElementParent(m_paneToggleButton.get(), paneTitleFrameworkElement, isTopNavigationView || !isPaneToggleButtonVisible);
            const auto second = SetPaneTitleFrameworkElementParent(m_paneTitlePresenter.get(), paneTitleFrameworkElement, isTopNavigationView || isPaneToggleButtonVisible);
            const auto third = SetPaneTitleFrameworkElementParent(m_paneTitleOnTopPane.get(), paneTitleFrameworkElement, !isTopNavigationView || isPaneToggleButtonVisible);
            first ? first() : second ? second() : third ? third() : []() {}();
        }
    }
}

std::function<void()> NavigationView::SetPaneTitleFrameworkElementParent(const winrt::ContentControl& parent, const winrt::FrameworkElement& paneTitle, bool shouldNotContainPaneTitle)
{
    if (parent)
    {
        if ((parent.Content() == paneTitle) == shouldNotContainPaneTitle)
        {
            if (shouldNotContainPaneTitle)
            {
                parent.Content(nullptr);
            }
            else
            {
                return [parent, paneTitle]() { parent.Content(paneTitle); };
            }
        }
    }
    return nullptr;
}

winrt::float2 c_frame1point1 = winrt::float2(0.9f, 0.1f);
winrt::float2 c_frame1point2 = winrt::float2(1.0f, 0.2f);
winrt::float2 c_frame2point1 = winrt::float2(0.1f, 0.9f);
winrt::float2 c_frame2point2 = winrt::float2(0.2f, 1.0f);

void NavigationView::AnimateSelectionChangedToItem(const winrt::IInspectable& selectedItem)
{
    if (selectedItem && !IsSelectionSuppressed(selectedItem))
    {
        AnimateSelectionChanged(selectedItem);
    }
}

// Please clear the field m_lastSelectedItemPendingAnimationInTopNav when calling this method to prevent garbage value and incorrect animation
// when the layout is invalidated as it's called in OnLayoutUpdated.
void NavigationView::AnimateSelectionChanged(const winrt::IInspectable& nextItem)
{
    // If we are delaying animation due to item movement in top nav overflow, dont do anything
    if (m_lastSelectedItemPendingAnimationInTopNav)
    {
        return;
    }

    winrt::UIElement prevIndicator = m_activeIndicator.get();
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

        if ((prevIndicator != nextIndicator) && paneContentGrid && prevIndicator && nextIndicator && SharedHelpers::IsAnimationsEnabled())
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

            bool areElementsAtSameDepth = false;
            if (IsTopNavigationView())
            {
                prevPos = prevPosPoint.X;
                nextPos = nextPosPoint.X;
                areElementsAtSameDepth = prevPosPoint.Y == nextPosPoint.Y;
            }
            else
            {
                prevPos = prevPosPoint.Y;
                nextPos = nextPosPoint.Y;
                areElementsAtSameDepth = prevPosPoint.X == nextPosPoint.X;
            }

            winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
            winrt::CompositionScopedBatch scopedBatch = visual.Compositor().CreateScopedBatch(winrt::CompositionBatchTypes::Animation);

            if (!areElementsAtSameDepth)
            {
                bool isNextBelow = prevPosPoint.Y < nextPosPoint.Y;
                prevIndicator.RenderSize().Height > prevIndicator.RenderSize().Width ?
                    PlayIndicatorNonSameLevelAnimations(prevIndicator, true, isNextBelow ? false : true) :
                    PlayIndicatorNonSameLevelTopPrimaryAnimation(prevIndicator, true);

                nextIndicator.RenderSize().Height > nextIndicator.RenderSize().Width ?
                    PlayIndicatorNonSameLevelAnimations(nextIndicator, false, isNextBelow ? true : false) :
                    PlayIndicatorNonSameLevelTopPrimaryAnimation(nextIndicator, false);

            }
            else
            {

                float outgoingEndPosition = static_cast<float>(nextPos - prevPos);
                float incomingStartPosition = static_cast<float>(prevPos - nextPos);

                // Play the animation on both the previous and next indicators
                PlayIndicatorAnimations(prevIndicator,
                    0,
                    outgoingEndPosition,
                    prevSize,
                    nextSize,
                    true);
                PlayIndicatorAnimations(nextIndicator,
                    incomingStartPosition,
                    0,
                    prevSize,
                    nextSize,
                    false);
            }

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

        m_activeIndicator.set(nextIndicator);
    }
}

void NavigationView::PlayIndicatorNonSameLevelAnimations(const winrt::UIElement& indicator, bool isOutgoing, bool fromTop)
{
    winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(indicator);
    winrt::Compositor comp = visual.Compositor();

    // Determine scaling of indicator (whether it is appearing or dissapearing)
    float beginScale = isOutgoing ? 1.0f : 0.0f;
    float endScale = isOutgoing ? 0.0f : 1.0f;
    winrt::ScalarKeyFrameAnimation scaleAnim = comp.CreateScalarKeyFrameAnimation();
    scaleAnim.InsertKeyFrame(0.0f, beginScale);
    scaleAnim.InsertKeyFrame(1.0f, endScale);
    scaleAnim.Duration(600ms);

    // Determine where the indicator is animating from/to
    winrt::Size size = indicator.RenderSize();
    float dimension = IsTopNavigationView() ? size.Width : size.Height;
    float newCenter = fromTop ? 0.0f : dimension;
    auto indicatorCenterPoint = visual.CenterPoint();
    indicatorCenterPoint.y = newCenter;
    visual.CenterPoint(indicatorCenterPoint);

    visual.StartAnimation(L"Scale.Y", scaleAnim);
}


void NavigationView::PlayIndicatorNonSameLevelTopPrimaryAnimation(const winrt::UIElement& indicator, bool isOutgoing)
{
    winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(indicator);
    winrt::Compositor comp = visual.Compositor();

    // Determine scaling of indicator (whether it is appearing or dissapearing)
    float beginScale = isOutgoing ? 1.0f : 0.0f;
    float endScale = isOutgoing ? 0.0f : 1.0f;
    winrt::ScalarKeyFrameAnimation scaleAnim = comp.CreateScalarKeyFrameAnimation();
    scaleAnim.InsertKeyFrame(0.0f, beginScale);
    scaleAnim.InsertKeyFrame(1.0f, endScale);
    scaleAnim.Duration(600ms);

    // Determine where the indicator is animating from/to
    winrt::Size size = indicator.RenderSize();
    float newCenter = size.Width /2;
    auto indicatorCenterPoint = visual.CenterPoint();
    indicatorCenterPoint.y = newCenter;
    visual.CenterPoint(indicatorCenterPoint);

    visual.StartAnimation(L"Scale.X", scaleAnim);
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
        if (winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(element))
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
        if (auto const container = NavigationViewItemOrSettingsContentFromData(item))
        {
            return winrt::get_self<NavigationViewItem>(container)->GetSelectionIndicator();
        }
    }
    return nullptr;
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
    // Selection changed event was requested to be ignored by settings item restoration, so let's do that
    if (m_shouldIgnoreNextSelectionChangeBecauseSettingsRestore) {
        return;
    }

    bool isSettingsItem = IsSettingsItem(nextItem);

    if (IsSelectionSuppressed(nextItem))
    {
        // This should not be a common codepath. Only happens if customer passes a 'selectionsuppressed' item via API.
        UndoSelectionAndRevertSelectionTo(prevItem, nextItem);
        RaiseItemInvoked(nextItem, isSettingsItem);
    }
    else
    {
        // TODO: The raising of this event can probably can be moved where the settings invocation is first handled.
        // Need to raise ItemInvoked for when the settings item gets invoked
        if (isSettingsItem)
        {
            RaiseItemInvoked(nextItem, isSettingsItem);
        }

        // Other transition other than default only apply to topnav
        // when clicking overflow on topnav, transition is from bottom
        // otherwise if prevItem is on left side of nextActualItem, transition is from left
        //           if prevItem is on right side of nextActualItem, transition is from right
        // click on Settings item is considered Default
        auto recommendedDirection = [this, prevItem, nextItem, isSettingsItem]()
        {
            if (IsTopNavigationView())
            {
                if (m_selectionChangeFromOverflowMenu)
                {
                    return NavigationRecommendedTransitionDirection::FromOverflow;
                }
                else if (!isSettingsItem && prevItem && nextItem)
                {
                    return GetRecommendedTransitionDirection(NavigationViewItemBaseOrSettingsContentFromData(prevItem),
                        NavigationViewItemBaseOrSettingsContentFromData(nextItem));
                }
            }
            return NavigationRecommendedTransitionDirection::Default;
        }();

        // Bug 17850504, Customer may use NavigationViewItem.IsSelected in ItemInvoke or SelectionChanged Event.
        // To keep the logic the same as RS4, ItemInvoke is before unselect the old item
        // And SelectionChanged is after we selected the new item.
        const auto selectedItem = SelectedItem();
        if (m_shouldRaiseItemInvokedAfterSelection)
        {
            RaiseItemInvoked(NavigationViewItemOrSettingsContentFromData(nextItem), isSettingsItem);
        }
        // Selection was modified inside ItemInvoked, skip everything here!
        if (selectedItem != SelectedItem())
        {
            return;
        }
        UnselectPrevItem(prevItem, nextItem);
        ChangeSelectStatusForItem(nextItem, true /*selected*/);

        RaiseSelectionChangedEvent(nextItem, isSettingsItem, recommendedDirection);
        AnimateSelectionChanged(nextItem);

        if (auto const nvi = NavigationViewItemOrSettingsContentFromData(nextItem))
        {
            ClosePaneIfNeccessaryAfterItemIsClicked(nvi);
        }    
    }
}

void NavigationView::UpdateSelectionModelSelection(const winrt::IndexPath& ip)
{
    auto const prevIndexPath = m_selectionModel.SelectedIndex();
    m_selectionModel.SelectAt(ip);
    UpdateIsChildSelected(prevIndexPath, ip);  
}

void NavigationView::UpdateIsChildSelected(const winrt::IndexPath& prevIP, const winrt::IndexPath& nextIP)
{ 
    if (prevIP && prevIP.GetSize() > 0)
    {
        UpdateIsChildSelectedForIndexPath(prevIP, false /*isChildSelected*/);
    }
     
    if (nextIP && nextIP.GetSize() > 0)
    {
        UpdateIsChildSelectedForIndexPath(nextIP, true /*isChildSelected*/);
    }
}

void NavigationView::UpdateIsChildSelectedForIndexPath(const winrt::IndexPath& ip, bool isChildSelected)
{
    // Update the isChildSelected property for every container on the IndexPath (with the exception of the actual container pointed to by the indexpath)
    auto container = GetContainerForIndex(ip.GetAt(0));
    auto index = 1;
    while (container)
    {
        if (auto const nvi = container.try_as<winrt::NavigationViewItem>())
        {
            nvi.IsChildSelected(isChildSelected);
            if (auto const nextIR = winrt::get_self<NavigationViewItem>(nvi)->GetRepeater())
            {
                if (index < ip.GetSize() - 1)
                {
                    container = nextIR.TryGetElement(ip.GetAt(index));
                    index++;
                    continue;
                }
            }
        }
        container = nullptr;
    }
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
    // Need to keep the VisualStateGroup "DisplayModeGroup" updated even if the actual
    // display mode is not changed. This is due to the fact that there can be a transition between
    // 'Minimal' and 'MinimalWithBackButton'.
    UpdateVisualStateForDisplayModeGroup(displayMode);

    if (forceSetDisplayMode || DisplayMode() != displayMode)
    {
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
    // When the NavView is open, the close button is taking space instead of the back button.
    if (ShouldShowBackButton() || ShouldShowCloseButton())
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
            // TopNavigationMinimal was introduced in 19H1. We need to fallback to Minimal if the customer uses an older template.
            handled = winrt::VisualStateManager::GoToState(*this, L"TopNavigationMinimal", false /*useTransitions*/);
        }
        if (!handled)
        {
            winrt::VisualStateManager::GoToState(*this, visualStateName, false /*useTransitions*/);
        }
        splitView.DisplayMode(splitViewDisplayMode);
    }
}

void NavigationView::OnNavigationViewItemTapped(const winrt::IInspectable& sender, const winrt::TappedRoutedEventArgs& args)
{
    if (auto nvi = sender.try_as<winrt::NavigationViewItem>())
    {
        if (IsSettingsItem(nvi))
        {
            OnSettingsInvoked();
        }
        else
        {
            OnNavigationViewItemInvoked(nvi);
        }
        nvi.Focus(winrt::FocusState::Pointer);
        args.Handled(true);
    }
}

void NavigationView::OnNavigationViewItemKeyUp(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    // Because ListViewItem eats the events, we only get these keys on KeyUp.
    if (args.OriginalKey() == winrt::VirtualKey::GamepadA)
    {
        if (auto nvi = sender.try_as<winrt::NavigationViewItem>())
        {
            HandleKeyEventForNavigationViewItem(nvi, args);
        }
    }
}

void NavigationView::OnNavigationViewItemKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    if (auto nvi = sender.try_as<winrt::NavigationViewItem>())
    {
        HandleKeyEventForNavigationViewItem(nvi, args);
    }
}

void NavigationView::HandleKeyEventForNavigationViewItem(const winrt::NavigationViewItem& nvi, const winrt::KeyRoutedEventArgs& args)
{
    auto key = args.Key();
    if (IsSettingsItem(nvi))
    {
        // Because ListViewItem eats the events, we only get these keys on KeyDown.
        if (key == winrt::VirtualKey::Space ||
            key == winrt::VirtualKey::Enter)
        {
            args.Handled(true);
            OnSettingsInvoked();
        }
    }
    else
    {
        switch (key)
        {
        case winrt::VirtualKey::Enter:
        case winrt::VirtualKey::Space:
            args.Handled(true);
            OnNavigationViewItemInvoked(nvi);
            break;
        case winrt::VirtualKey::Home:
            args.Handled(true);
            KeyboardFocusFirstItemFromItem(nvi);
            break;
        case winrt::VirtualKey::End:
            args.Handled(true);
            KeyboardFocusLastItemFromItem(nvi);
            break;
        }
    }
}

void NavigationView::KeyboardFocusFirstItemFromItem(const winrt::NavigationViewItemBase& nvib)
{
    auto const firstElement = [this, nvib]()
    {
        if (IsTopNavigationView())
        {
            if (IsContainerInOverflow(nvib))
            {
                return m_topNavRepeaterOverflowView.get().TryGetElement(0);
            }
            else
            {
                return m_topNavRepeater.get().TryGetElement(0);
            }
        }
        return m_leftNavRepeater.get().TryGetElement(0);
    }();

    if (auto controlFirst = firstElement.try_as<winrt::Control>())
    {
        controlFirst.Focus(winrt::FocusState::Keyboard);
    }
}

void NavigationView::KeyboardFocusLastItemFromItem(const winrt::NavigationViewItemBase& nvib)
{
    auto const ir = [this, nvib]()
    {
        if (IsTopNavigationView())
        {
            if (IsContainerInOverflow(nvib))
            {
                return m_topNavRepeaterOverflowView.get();
            }
            else
            {
                return m_topNavRepeater.get();
            }
        }
        else
        {
            return m_leftNavRepeater.get();
        }
    }();

    if (auto itemsSourceView = ir.ItemsSourceView())
    {
        auto lastIndex = itemsSourceView.Count() - 1;
        if (auto lastElement = ir.TryGetElement(lastIndex))
        {
            if (auto controlLast = lastElement.try_as<winrt::Control>())
            {
                controlLast.Focus(winrt::FocusState::Programmatic);
            }
        }
    }
}

void NavigationView::OnRepeaterGettingFocus(const winrt::IInspectable& sender, const winrt::GettingFocusEventArgs& args)
{
    if (args.InputDevice() == winrt::FocusInputDeviceKind::Keyboard)
    {
        if (auto const oldFocusedElement = args.OldFocusedElement())
        {
            auto const oldElementParent = winrt::VisualTreeHelper::GetParent(oldFocusedElement);
            auto const rootRepeater = [this]()
            {
                if (IsTopNavigationView())
                {
                    return m_topNavRepeater.get();
                }
                return m_leftNavRepeater.get();
            }();
            // If focus is coming from outside the root repeater, put focus on last focused item
            if (rootRepeater != oldElementParent)
            {
                if (auto const argsAsIGettingFocusEventArgs2 = args.try_as<winrt::IGettingFocusEventArgs2>())
                {
                    if (auto const lastFocusedNvi = rootRepeater.TryGetElement(m_indexOfLastFocusedItem))
                    {
                        if (argsAsIGettingFocusEventArgs2.TrySetNewFocusedElement(lastFocusedNvi))
                        {
                            args.Handled(true);
                        }
                    }
                }
            }
        }
    }
}

void NavigationView::OnNavigationViewItemOnGotFocus(const winrt::IInspectable& sender, winrt::RoutedEventArgs const& e)
{
    if (auto nvi = sender.try_as<winrt::NavigationViewItem>())
    {
        // Store index of last focused item in order to get proper focus behavior.
        // No need to keep track of last focused item if the item is in the overflow menu.
        m_indexOfLastFocusedItem = [this, nvi]()
        {
            if (IsTopNavigationView())
            {
                return m_topNavRepeater.get().GetElementIndex(nvi);;
            }
            return m_leftNavRepeater.get().GetElementIndex(nvi);
        }();

        // Achieve selection follows focus behavior
        if (IsNavigationViewListSingleSelectionFollowsFocus())
        {
            if (nvi.SelectsOnInvoked())
            {
                if (IsTopNavigationView())
                {
                    if (auto parentIR = GetParentItemsRepeaterForContainer(nvi))
                    {
                        if (parentIR != m_topNavRepeaterOverflowView.get())
                        {
                            OnNavigationViewItemInvoked(nvi);
                        }
                    }
                }
                else
                {
                    OnNavigationViewItemInvoked(nvi);
                }
            }
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

void NavigationView::OnKeyDown(winrt::KeyRoutedEventArgs const& e)
{
    const auto& eventArgs = e;
    auto key = eventArgs.Key();

    bool handled = false;

    switch (key)
    {
    case winrt::VirtualKey::GamepadView:
        if (!IsPaneOpen() && !IsTopNavigationView())
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
            auto index = m_topDataProvider.IndexOf(item, NavigationViewSplitVectorID::PrimaryList);

            if (index >= 0)
            {
                if (auto&& topNavRepeater = m_topNavRepeater.get())
                {
                    auto topPrimaryListSize = m_topDataProvider.GetPrimaryListSize();
                    index += offset;

                    while (index > -1 && index < topPrimaryListSize)
                    {
                        auto newItem = topNavRepeater.TryGetElement(index);
                        if (auto newNavViewItem = newItem.try_as<winrt::NavigationViewItem>())
                        {
                            // This is done to skip Separators or other items that are not NavigationViewItems
                            if (winrt::get_self<NavigationViewItem>(newNavViewItem)->SelectsOnInvoked())
                            {
                                newNavViewItem.IsSelected(true);
                                return true;
                            }
                        }

                        index += offset;
                    }
                }
            }
        }
    }

    return false;
}

winrt::IInspectable NavigationView::MenuItemFromContainer(winrt::DependencyObject const& container)
{
    if (container)
    {
        if (auto const nvib = container.try_as<winrt::NavigationViewItemBase>())
        {
            if (auto const parentRepeater = GetParentItemsRepeaterForContainer(nvib))
            {
                auto const containerIndex = parentRepeater.GetElementIndex(nvib);
                if (containerIndex >= 0)
                {
                    return GetItemFromIndex(parentRepeater, containerIndex);
                }
            }
        }
    }
    return nullptr;
}

winrt::DependencyObject NavigationView::ContainerFromMenuItem(winrt::IInspectable const& item)
{
    if (const auto& data = item)
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

    // If it's Uninitialized, it means that we didn't start the layout yet.
    if (m_topNavigationMode != TopNavigationViewLayoutState::Uninitialized)
    {
        m_topDataProvider.MoveAllItemsToPrimaryList();
    }

    m_lastSelectedItemPendingAnimationInTopNav.set(nullptr);
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
        m_topDataProvider.InvalidWidthCache();
        InvalidateMeasure();
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
    if (auto ir = m_topNavRepeater.get())
    {
        auto prevIndex = prev ? ir.GetElementIndex(prev.try_as<winrt::UIElement>()) : s_itemNotFound;
        auto nextIndex = next ? ir.GetElementIndex(next.try_as<winrt::UIElement>()) : s_itemNotFound;
        if (prevIndex == s_itemNotFound || nextIndex == s_itemNotFound)
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

    const auto newItem = args.NewValue();
    const auto oldItem = args.OldValue();

    ChangeSelection(oldItem, newItem);

    // When we do not raise a "SelectItemChanged" event, the selection does not get animated.
    // To prevent faulty visual states, we will animate that here
    // Since we only do this for the settings item, check if the old item is our settings item
    if (oldItem != newItem && m_shouldIgnoreNextSelectionChangeBecauseSettingsRestore)
    {
        ChangeSelectStatusForItem(oldItem, false /*selected*/);
        ChangeSelectStatusForItem(newItem, true /*selected*/);
        AnimateSelectionChanged(newItem);
    }

    if (m_appliedTemplate && IsTopNavigationView())
    {
        if (!m_layoutUpdatedToken ||
            (newItem && m_topDataProvider.IndexOf(newItem) != s_itemNotFound && m_topDataProvider.IndexOf(newItem, NavigationViewSplitVectorID::PrimaryList) == s_itemNotFound)) // selection is in overflow
        {
            InvalidateTopNavPrimaryLayout();
        }
    }
}

void NavigationView::SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(winrt::IInspectable const& item)
{
    SelectedItem(item);
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
    else if (selected)
    {
        // If we are selecting an item and have not found a realized container for it,
        // we may need to manually resolve a container for this in order to update the
        // SelectionModel's selected IndexPath.
        auto const ip = GetIndexPathOfItem(item);
        if (ip && ip.GetSize() > 0)
        {
            // The SelectedItem property has already been updated. So we want to block any logic from executing
            // in the SelectionModel selection changed callback.
            auto scopeGuard = gsl::finally([this]()
                {
                    m_shouldIgnoreNextSelectionChange = false;
                });
            m_shouldIgnoreNextSelectionChange = true;
            UpdateSelectionModelSelection(ip);
        }
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
    if (prevItem && prevItem != nextItem)
    {
        auto scopeGuard = gsl::finally([this, setIgnoreNextSelectionChangeToFalse = !m_shouldIgnoreNextSelectionChange]()
        {
            if (setIgnoreNextSelectionChangeToFalse)
            {
                m_shouldIgnoreNextSelectionChange = false;
            }
        });
        m_shouldIgnoreNextSelectionChange = true;
        ChangeSelectStatusForItem(prevItem, false /*selected*/);
    }
}

void NavigationView::UndoSelectionAndRevertSelectionTo(winrt::IInspectable const& prevSelectedItem, winrt::IInspectable const& nextItem)
{
    winrt::IInspectable selectedItem{ nullptr };
    if (prevSelectedItem)
    {
        if (IsSelectionSuppressed(prevSelectedItem))
        {
            AnimateSelectionChanged(nullptr);
        }
        else
        {
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
    auto state = (OverflowLabelMode() == winrt::NavigationViewOverflowLabelMode::MoreLabel) ?
        L"OverflowButtonWithLabel" :
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
        PropagateShowFocusVisualToAllNavigationViewItemsInRepeater(m_leftNavRepeater.get(), ShouldShowFocusVisual());
        PropagateShowFocusVisualToAllNavigationViewItemsInRepeater(m_topNavRepeater.get(), ShouldShowFocusVisual());
    }
}

bool NavigationView::ShouldShowFocusVisual()
{
    return SelectionFollowsFocus() == winrt::NavigationViewSelectionFollowsFocus::Disabled;
}

void NavigationView::PropagateShowFocusVisualToAllNavigationViewItemsInRepeater(winrt::ItemsRepeater const& ir, bool showFocusVisual)
{
    if (ir)
    {
        if (auto itemsSourceView = ir.ItemsSourceView())
        {
            auto numberOfItems = itemsSourceView.Count();
            for (int i = 0; i < numberOfItems; i++)
            {
                if (auto nvib = ir.TryGetElement(i))
                {
                    if (auto nvi = nvib.try_as<winrt::NavigationViewItem>())
                    {
                        auto nviImpl = winrt::get_self<NavigationViewItem>(nvi);
                        nviImpl->UseSystemFocusVisuals(showFocusVisual);
                    }
                }

            }
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
    return LayoutUtils::MeasureAndGetDesiredWidthFor(m_topNavGrid.get(), availableSize);
}

float NavigationView::MeasureTopNavMenuItemsHostDesiredWidth(winrt::Size const& availableSize)
{
    return LayoutUtils::MeasureAndGetDesiredWidthFor(m_topNavRepeater.get(), availableSize);
}

float NavigationView::GetTopNavigationViewActualWidth()
{
    double width = LayoutUtils::GetActualWidthFor(m_topNavGrid.get());
    MUX_ASSERT(width < std::numeric_limits<float>::max());
    return static_cast<float>(width);
}

bool NavigationView::HasTopNavigationViewItemNotInPrimaryList()
{
    return m_topDataProvider.GetPrimaryListSize() != m_topDataProvider.Size();
}

void NavigationView::ResetAndRearrangeTopNavItems(winrt::Size const& availableSize)
{
    if (HasTopNavigationViewItemNotInPrimaryList())
    {
        m_topDataProvider.MoveAllItemsToPrimaryList();
    }
    ArrangeTopNavItems(availableSize);
}

void NavigationView::HandleTopNavigationMeasureOverride(winrt::Size const& availableSize)
{
    // Determine if TopNav is in Overflow
    if (HasTopNavigationViewItemNotInPrimaryList())
    {
        HandleTopNavigationMeasureOverrideOverflow(availableSize);
    }
    else
    {
        HandleTopNavigationMeasureOverrideNormal(availableSize);
    }

    if (m_topNavigationMode == TopNavigationViewLayoutState::Uninitialized)
    {
        m_topNavigationMode = TopNavigationViewLayoutState::Initialized;
    }
}

void NavigationView::HandleTopNavigationMeasureOverrideNormal(const winrt::Windows::Foundation::Size& availableSize)
{
    auto desiredWidth = MeasureTopNavigationViewDesiredWidth(c_infSize);
    if (desiredWidth > availableSize.Width)
    {
        ResetAndRearrangeTopNavItems(availableSize);
    }
}

void NavigationView::HandleTopNavigationMeasureOverrideOverflow(const winrt::Windows::Foundation::Size& availableSize)
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
            ResetAndRearrangeTopNavItems(availableSize);
        }
        else
        {
            auto movableItems = FindMovableItemsRecoverToPrimaryList(availableSize.Width - desiredWidth, {}/*includeItems*/);
            m_topDataProvider.MoveItemsToPrimaryList(movableItems);
        }
    }
}

void NavigationView::ArrangeTopNavItems(winrt::Size const& availableSize)
{
    SetOverflowButtonVisibility(winrt::Visibility::Collapsed);
    auto desiredWidth = MeasureTopNavigationViewDesiredWidth(c_infSize);
    if (!(desiredWidth < availableSize.Width))
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

void NavigationView::SelectOverflowItem(winrt::IInspectable const& item, winrt::IndexPath const& ip)
{

    auto const itemBeingMoved = [item, ip, this]()
    {
        if (ip.GetSize() > 1)
        {
            return GetItemFromIndex(m_topNavRepeaterOverflowView.get(), m_topDataProvider.ConvertOriginalIndexToIndex(ip.GetAt(0)));
        }
        return item;
    }();

    // Calculate selected overflow item size.
    auto selectedOverflowItemIndex = m_topDataProvider.IndexOf(itemBeingMoved);
    MUX_ASSERT(selectedOverflowItemIndex != s_itemNotFound);
    auto selectedOverflowItemWidth = m_topDataProvider.GetWidthForItem(selectedOverflowItemIndex);

    bool needInvalidMeasure = !m_topDataProvider.IsValidWidthForItem(selectedOverflowItemIndex);

    if (!needInvalidMeasure)
    {
        auto actualWidth = GetTopNavigationViewActualWidth();
        auto desiredWidth = MeasureTopNavigationViewDesiredWidth(c_infSize);
        MUX_ASSERT(desiredWidth <= actualWidth);

        // Calculate selected item size
        auto selectedItemIndex = s_itemNotFound;
        auto selectedItemWidth = 0.f;
        if (auto selectedItem = SelectedItem())
        {
            selectedItemIndex = m_topDataProvider.IndexOf(selectedItem);
            if (selectedItemIndex != s_itemNotFound)
            {
                selectedItemWidth = m_topDataProvider.GetWidthForItem(selectedItemIndex);
            }
        }

        auto widthAtLeastToBeRemoved = desiredWidth + selectedOverflowItemWidth - actualWidth;

        // calculate items to be removed from primary because a overflow item is selected. 
        // SelectedItem is assumed to be removed from primary first, then added it back if it should not be removed
        auto itemsToBeRemoved = FindMovableItemsToBeRemovedFromPrimaryList(widthAtLeastToBeRemoved, { } /*excludeItems*/);

        // calculate the size to be removed
        auto toBeRemovedItemWidth = m_topDataProvider.CalculateWidthForItems(itemsToBeRemoved);

        auto widthAvailableToRecover = toBeRemovedItemWidth - widthAtLeastToBeRemoved;
        auto itemsToBeAdded = FindMovableItemsRecoverToPrimaryList(widthAvailableToRecover, { selectedOverflowItemIndex }/*includeItems*/);

        CollectionHelper::unique_push_back(itemsToBeAdded, selectedOverflowItemIndex);

        // Keep track of the item being moved in order to know where to animate selection indicator
        m_lastSelectedItemPendingAnimationInTopNav.set(itemBeingMoved);
        if (ip && ip.GetSize() > 0)
        {
            for (std::vector<int>::iterator it = itemsToBeRemoved.begin(); it != itemsToBeRemoved.end(); ++it)
            {
                if (*it == ip.GetAt(0))
                {
                    if (auto const indicator = m_activeIndicator.get())
                    {
                        // If the previously selected item is being moved into overflow, hide its indicator
                        // as we will no longer need to animate from its location.
                        AnimateSelectionChanged(nullptr);
                    }
                    break;
                }
            }
        }

        if (m_topDataProvider.HasInvalidWidth(itemsToBeAdded))
        {
            needInvalidMeasure = true;
        }
        else
        {
            // Exchange items between Primary and Overflow
            {
                m_topDataProvider.MoveItemsToPrimaryList(itemsToBeAdded);
                m_topDataProvider.MoveItemsOutOfPrimaryList(itemsToBeRemoved);
            }

            if (NeedRearrangeOfTopElementsAfterOverflowSelectionChanged(selectedOverflowItemIndex))
            {
                needInvalidMeasure = true;
            }

            if (!needInvalidMeasure)
            {
                SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(item);
                InvalidateMeasure();
            }
        }
    }

    // TODO: Verify that this is no longer needed and delete
    if (needInvalidMeasure)
    {
        // not all items have known width, need to redo the layout
        m_topDataProvider.MoveAllItemsToPrimaryList();
        SetSelectedItemAndExpectItemInvokeWhenSelectionChangedIfNotInvokedFromAPI(item);
        InvalidateTopNavPrimaryLayout();
    }
}

bool NavigationView::NeedRearrangeOfTopElementsAfterOverflowSelectionChanged(int selectedOriginalIndex)
{
    bool needRearrange = false;

    auto primaryList = m_topDataProvider.GetPrimaryItems();
    auto primaryListSize = primaryList.Size();
    auto indexInPrimary = m_topDataProvider.ConvertOriginalIndexToIndex(selectedOriginalIndex);
    // We need to verify that through various overflow selection combinations, the primary
    // items have not been put into a state of non-logical item layout (aka not in proper sequence).
    // To verify this, if the newly selected item has items following it in the primary items:
    // - we verify that they are meant to follow the selected item as specified in the original order
    // - we verify that the preceding item is meant to directly precede the selected item in the original order
    // If these two conditions are not met, we move all items to the primary list and trigger a re-arrangement of the items.
    if (indexInPrimary < static_cast<int>(primaryListSize - 1))
    {
        auto nextIndexInPrimary = indexInPrimary + 1;
        auto nextIndexInOriginal = selectedOriginalIndex + 1;
        auto prevIndexInOriginal = selectedOriginalIndex - 1;

        // Check whether item preceding the selected is not directly preceding
        // in the original.
        if (indexInPrimary > 0)
        {
            std::vector<int> prevIndexInVector;
            prevIndexInVector.push_back(nextIndexInPrimary - 1);
            auto prevOriginalIndexOfPrevPrimaryItem = m_topDataProvider.ConvertPrimaryIndexToIndex(prevIndexInVector);
            if (prevOriginalIndexOfPrevPrimaryItem.at(0) != prevIndexInOriginal)
            {
                needRearrange = true;
            }
        }


        // Check whether items following the selected item are out of order
        while (!needRearrange && nextIndexInPrimary < static_cast<int>(primaryListSize))
        {
            std::vector<int> nextIndexInVector;
            nextIndexInVector.push_back(nextIndexInPrimary);
            auto originalIndex = m_topDataProvider.ConvertPrimaryIndexToIndex(nextIndexInVector);
            if (nextIndexInOriginal != originalIndex.at(0))
            {
                needRearrange = true;
                break;
            }
            nextIndexInPrimary++;
            nextIndexInOriginal++;
        }
    }

    return needRearrange;
}

void NavigationView::ShrinkTopNavigationSize(float desiredWidth, winrt::Size const& availableSize)
{
    UpdateTopNavigationWidthCache();

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
    if (auto ir = m_topNavRepeater.get())
    {
        int selectedItemIndexInPrimary = m_topDataProvider.IndexOf(SelectedItem(), NavigationViewSplitVectorID::PrimaryList);
        int size = m_topDataProvider.GetPrimaryListSize();

        float requiredWidth = 0;

        for (int i = 0; i < size; i++)
        {
            if (i != selectedItemIndexInPrimary)
            {
                bool shouldMove = true;
                if (requiredWidth <= availableWidth)
                {
                    auto container = ir.TryGetElement(i);
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

double NavigationView::GetPaneToggleButtonWidth()
{
    return unbox_value<double>(SharedHelpers::FindInApplicationResources(L"PaneToggleButtonWidth", box_value(c_paneToggleButtonWidth)));
}

double NavigationView::GetPaneToggleButtonHeight()
{
    return unbox_value<double>(SharedHelpers::FindInApplicationResources(L"PaneToggleButtonHeight", box_value(c_paneToggleButtonHeight)));
}

void NavigationView::UpdateTopNavigationWidthCache()
{
    int size = m_topDataProvider.GetPrimaryListSize();
    if (auto ir = m_topNavRepeater.get())
    {
        for (int i = 0; i < size; i++)
        {
            auto container = ir.TryGetElement(i);
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
    return m_topNavRepeater && (TemplateSettings().TopPaneVisibility() == winrt::Visibility::Visible);
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
        UpdateVisualStateForDisplayModeGroup(DisplayMode());
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
        UpdatePaneTitleFrameworkElementParents();
        UpdateBackAndCloseButtonsVisibility();
        UpdatePaneToggleSize();
    }
    else if (property == s_IsBackButtonVisibleProperty)
    {
        UpdateBackAndCloseButtonsVisibility();
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
        UpdateRepeaterItemsSource(true /*forceSelectionModelUpdate*/);
    }
    else if (property == s_MenuItemsProperty)
    {
        UpdateRepeaterItemsSource(true /*forceSelectionModelUpdate*/);
    }
    else if (property == s_PaneDisplayModeProperty)
    {
        // m_wasForceClosed is set to true because ToggleButton is clicked and Pane is closed.
        // When PaneDisplayMode is changed, reset the force flag to make the Pane can be opened automatically again.
        m_wasForceClosed = false;

        CollapseTopLevelMenuItems(auto_unbox(args.OldValue()));
        UpdatePaneToggleButtonVisibility();
        UpdatePaneDisplayMode(auto_unbox(args.OldValue()), auto_unbox(args.NewValue()));
        UpdatePaneTitleFrameworkElementParents();
        UpdatePaneVisibility();
        UpdateVisualState();
        UpdatePaneButtonsWidths();
    }
    else if (property == s_IsPaneVisibleProperty)
    {
        UpdatePaneVisibility();
        UpdateVisualStateForDisplayModeGroup(DisplayMode());

        // When NavView is in expaneded mode with fixed window size, setting IsPaneVisible to false doesn't closes the pane
        // We manually close/open it for this case
        if (!IsPaneVisible() && IsPaneOpen())
        {
            ClosePane();
        }

        if (IsPaneVisible() && DisplayMode() == winrt::NavigationViewDisplayMode::Expanded && !IsPaneOpen())
        {
            OpenPane();
        }
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
        UpdatePaneTitleFrameworkElementParents();
        UpdateBackAndCloseButtonsVisibility();
        UpdatePaneToggleButtonVisibility();
        UpdateVisualState();
    }
    else if (property == s_IsSettingsVisibleProperty)
    {
        UpdateVisualState();
    }
    else if (property == s_CompactPaneLengthProperty)
    {
        // Need to update receiver margins when CompactPaneLength changes
        UpdatePaneShadow();

        // Update pane-button-grid width when pane is closed and we are not in minimal
        UpdatePaneButtonsWidths();
    }
    else if (property == s_IsTitleBarAutoPaddingEnabledProperty)
    {
        UpdateTitleBarPadding();
    }
    else if (property == s_MenuItemTemplateProperty ||
        property == s_MenuItemTemplateSelectorProperty)
    {
        SyncItemTemplates();
    }
}

void NavigationView::UpdateNavigationViewItemsFactory()
{
    winrt::IInspectable newItemTemplate = MenuItemTemplate();
    if (!newItemTemplate)
    {
        newItemTemplate = MenuItemTemplateSelector();
    }
    m_navigationViewItemsFactory->UserElementFactory(newItemTemplate);
}

void NavigationView::SyncItemTemplates()
{
    UpdateNavigationViewItemsFactory();
}

void NavigationView::OnRepeaterLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    if (auto item = SelectedItem())
    {
        if (!IsSelectionSuppressed(item))
        {
            if (auto navViewItem = NavigationViewItemOrSettingsContentFromData(item))
            {
                navViewItem.IsSelected(true);
            }
        }
        AnimateSelectionChanged(item);
    }
}

// If app is .net app, the lifetime of NavigationView maybe depends on garbage collection.
// Unlike other revoker, TitleBar is in global space and we need to stop receiving changed event when it's unloaded.
// So we do hook it in Loaded and Unhook it in Unloaded
void NavigationView::OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    m_titleBarMetricsChangedRevoker.revoke();
    m_titleBarIsVisibleChangedRevoker.revoke();
}

void NavigationView::OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    if (auto coreTitleBar = m_coreTitleBar.get())
    {
        m_titleBarMetricsChangedRevoker = coreTitleBar.LayoutMetricsChanged(winrt::auto_revoke, { this, &NavigationView::OnTitleBarMetricsChanged });
        m_titleBarIsVisibleChangedRevoker = coreTitleBar.IsVisibleChanged(winrt::auto_revoke, { this, &NavigationView::OnTitleBarIsVisibleChanged });
    }
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
    UpdatePaneTitleFrameworkElementParents();

    if (SharedHelpers::IsThemeShadowAvailable())
    {
        if (auto splitView = m_rootSplitView.get())
        {
            auto displayMode = splitView.DisplayMode();
            auto isOverlay = displayMode == winrt::SplitViewDisplayMode::Overlay || displayMode == winrt::SplitViewDisplayMode::CompactOverlay;
            if (auto paneRoot = splitView.Pane())
            {
                auto currentTranslation = paneRoot.Translation();
                auto translation = winrt::float3{ currentTranslation.x, currentTranslation.y, IsPaneOpen() && isOverlay ? c_paneElevationTranslationZ : 0.0f };
                paneRoot.Translation(translation);
            }
        }
    }
    UpdatePaneButtonsWidths();
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
    UpdateRepeaterItemsSource(false /*forceSelectionModelUpdate*/);
}

void NavigationView::UpdatePaneDisplayMode(winrt::NavigationViewPaneDisplayMode oldDisplayMode, winrt::NavigationViewPaneDisplayMode newDisplayMode)
{
    if (!m_appliedTemplate)
    {
        return;
    }

    UpdatePaneDisplayMode();

    // For better user experience, We help customer to Open/Close Pane automatically when we switch between LeftMinimal <-> Left.
    // From other navigation PaneDisplayMode to LeftMinimal, we expect pane is closed.
    // From LeftMinimal to Left, it is expected the pane is open. For other configurations, this seems counterintuitive.
    // See #1702 and #1787
    if (!IsTopNavigationView())
    {
        if (IsPaneOpen())
        {
            if (newDisplayMode == winrt::NavigationViewPaneDisplayMode::LeftMinimal)
            {
                ClosePane();
            }
        }
        else
        {
            if (oldDisplayMode == winrt::NavigationViewPaneDisplayMode::LeftMinimal
                && newDisplayMode == winrt::NavigationViewPaneDisplayMode::Left)
            {
                OpenPane();
            }
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

        SharedHelpers::SetBinding(propertyPathName, newParent, winrt::ContentControl::ContentProperty());
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

        SharedHelpers::SetBinding(L"AutoSuggestBox", autoSuggestBoxContentControl, winrt::ContentControl::ContentProperty());
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
    // Ignore AlwaysShowHeader property in case DisplayMode is Minimal and it's not Top NavigationView
    bool showHeader = AlwaysShowHeader() || (!IsTopNavigationView() && displayMode == winrt::NavigationViewDisplayMode::Minimal);

    // Like bug 17517627, Customer like WallPaper Studio 10 expects a HeaderContent visual even if Header() is null. 
    // App crashes when they have dependency on that visual, but the crash is not directly state that it's a header problem.   
    // NavigationView doesn't use quirk, but we determine the version by themeresource.
    // As a workaround, we 'quirk' it for RS4 or before release. if it's RS4 or before, HeaderVisible is not related to Header().
    // If theme resource is RS5 or later, we will not show header if header is null.
    if (SharedHelpers::IsRS5OrHigher())
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
            double width = GetPaneToggleButtonWidth();
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
                    togglePaneButtonWidth = OpenPaneLength() - ((ShouldShowBackButton() || ShouldShowCloseButton()) ? c_backButtonWidth : 0);
                }
                else if (!(splitView.DisplayMode() == winrt::SplitViewDisplayMode::Overlay && !IsPaneOpen()))
                {
                    width = OpenPaneLength();
                    togglePaneButtonWidth = OpenPaneLength();
                }
            }

            if (auto toggleButton = m_paneToggleButton.get())
            {
                toggleButton.Width(togglePaneButtonWidth);
            }
        }
    }
}

void NavigationView::UpdateBackAndCloseButtonsVisibility()
{
    if (!m_appliedTemplate)
    {
        return;
    }

    auto shouldShowBackButton = ShouldShowBackButton();
    auto backButtonVisibility = Util::VisibilityFromBool(shouldShowBackButton);
    auto visualStateDisplayMode = GetVisualStateDisplayMode(DisplayMode());
    bool useLeftPaddingForBackOrCloseButton =
        (visualStateDisplayMode == NavigationViewVisualStateDisplayMode::Minimal && !IsTopNavigationView()) ||
        visualStateDisplayMode == NavigationViewVisualStateDisplayMode::MinimalWithBackButton;
    double leftPaddingForBackOrCloseButton = 0.0;
    double paneHeaderPaddingForToggleButton = 0.0;
    double paneHeaderPaddingForCloseButton = 0.0;
    double paneHeaderContentBorderRowMinHeight = 0.0;

    GetTemplateSettings()->BackButtonVisibility(backButtonVisibility);

    if (m_paneToggleButton && IsPaneToggleButtonVisible())
    {
        paneHeaderContentBorderRowMinHeight = GetPaneToggleButtonHeight();
        paneHeaderPaddingForToggleButton = GetPaneToggleButtonWidth();

        if (useLeftPaddingForBackOrCloseButton)
        {
            leftPaddingForBackOrCloseButton = paneHeaderPaddingForToggleButton;
        }
    }

    if (auto backButton = m_backButton.get())
    {
        if (ShouldPreserveNavigationViewRS4Behavior())
        {
            backButton.Visibility(backButtonVisibility);
        }

        if (useLeftPaddingForBackOrCloseButton && backButtonVisibility == winrt::Visibility::Visible)
        {
            leftPaddingForBackOrCloseButton += backButton.Width();
        }
    }

    if (auto closeButton = m_closeButton.get())
    {
        auto closeButtonVisibility = Util::VisibilityFromBool(ShouldShowCloseButton());

        closeButton.Visibility(closeButtonVisibility);

        if (closeButtonVisibility == winrt::Visibility::Visible)
        {
            paneHeaderContentBorderRowMinHeight = std::max(paneHeaderContentBorderRowMinHeight, closeButton.Height());

            if (useLeftPaddingForBackOrCloseButton)
            {
                paneHeaderPaddingForCloseButton = closeButton.Width();
                leftPaddingForBackOrCloseButton += paneHeaderPaddingForCloseButton;
            }
        }
    }

    if (auto contentLeftPadding = m_contentLeftPadding.get())
    {
        contentLeftPadding.Width(leftPaddingForBackOrCloseButton);
    }

    if (auto paneHeaderToggleButtonColumn = m_paneHeaderToggleButtonColumn.get())
    {
        // Account for the PaneToggleButton's width in the PaneHeader's placement.
        paneHeaderToggleButtonColumn.Width(winrt::GridLengthHelper::FromValueAndType(paneHeaderPaddingForToggleButton, winrt::GridUnitType::Pixel));
    }

    if (auto paneHeaderCloseButtonColumn = m_paneHeaderCloseButtonColumn.get())
    {
        // Account for the CloseButton's width in the PaneHeader's placement.
        paneHeaderCloseButtonColumn.Width(winrt::GridLengthHelper::FromValueAndType(paneHeaderPaddingForCloseButton, winrt::GridUnitType::Pixel));
    }

    if (auto paneTitleHolderFrameworkElement = m_paneTitleHolderFrameworkElement.get())
    {
        if (paneHeaderContentBorderRowMinHeight == 0.00 && paneTitleHolderFrameworkElement.Visibility() == winrt::Visibility::Visible)
        {
            // Handling the case where the PaneTottleButton is collapsed and the PaneTitle's height needs to push the rest of the NavigationView's UI down.
            paneHeaderContentBorderRowMinHeight = paneTitleHolderFrameworkElement.ActualHeight();
        }
    }

    if (auto paneHeaderContentBorderRow = m_paneHeaderContentBorderRow.get())
    {
        paneHeaderContentBorderRow.MinHeight(paneHeaderContentBorderRowMinHeight);
    }

    if (auto paneContentGridAsUIE = m_paneContentGrid.get())
    {
        if (auto paneContentGrid = paneContentGridAsUIE.try_as<winrt::Grid>())
        {
            auto rowDefs = paneContentGrid.RowDefinitions();

            if (rowDefs.Size() >= c_backButtonRowDefinition)
            {
                auto rowDef = rowDefs.GetAt(c_backButtonRowDefinition);

                int backButtonRowHeight = 0;
                if (!IsOverlay() && shouldShowBackButton)
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
    }

    if (!ShouldPreserveNavigationViewRS4Behavior())
    {
        winrt::VisualStateManager::GoToState(*this, shouldShowBackButton ? L"BackButtonVisible" : L"BackButtonCollapsed", false /*useTransitions*/);
    }
    UpdateTitleBarPadding();
}

void NavigationView::UpdatePaneTitleMargins()
{
    if (ShouldPreserveNavigationViewRS4Behavior())
    {
        if (auto paneTitleFrameworkElement = m_paneTitleFrameworkElement.get())
        {
            double width = GetPaneToggleButtonWidth();

            if (ShouldShowBackButton() && IsOverlay())
            {
                width += c_backButtonWidth;
            }

            paneTitleFrameworkElement.Margin({ width, 0, 0, 0 }); // see "Hamburger title" on uni
        }
    }
}

void NavigationView::UpdateSelectionForMenuItems()
{
    // Allow customer to set selection by NavigationViewItem.IsSelected.
    // If there are more than two items are set IsSelected=true, the first one is actually selected.
    // If SelectedItem is set, IsSelected is ignored.
    //         <NavigationView.MenuItems>
    //              <NavigationViewItem Content = "Collection" IsSelected = "True" / >
    //         </NavigationView.MenuItems>
    if (!SelectedItem())
    {
        if (auto menuItems = MenuItems().try_as<winrt::IVector<winrt::IInspectable>>())
        {
            bool foundFirstSelected = false;
            for (auto menuItem : menuItems)
            {
                if (auto nvi = menuItem.try_as<winrt::NavigationViewItem>())
                {
                    if (nvi.IsSelected())
                    {
                        if (!foundFirstSelected)
                        {
                            auto scopeGuard = gsl::finally([this]()
                                {
                                    m_shouldIgnoreNextSelectionChange = false;
                                });
                            m_shouldIgnoreNextSelectionChange = true;
                            SelectedItem(nvi);
                            foundFirstSelected = true;
                        }
                        else
                        {
                            nvi.IsSelected(false);
                        }
                    }
                }
            }
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

void NavigationView::ClosePaneIfNeccessaryAfterItemIsClicked(const winrt::NavigationViewItem& selectedContainer)
{
    if (IsPaneOpen() && DisplayMode() != winrt::NavigationViewDisplayMode::Expanded && !DoesNavigationViewItemHaveChildren(selectedContainer))
    {
        ClosePane();
    }
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

        // Do not set a top padding when the IsTitleBarAutoPaddingEnabled property is set to False.
        if (IsTitleBarAutoPaddingEnabled())
        {
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
        }

        if (needsTopPadding)
        {
            // Only add extra padding if the NavView is the "root" of the app,
            // but not if the app is expanding into the titlebar
            winrt::UIElement root = winrt::Window::Current().Content();
            winrt::GeneralTransform gt = TransformToVisual(root);
            winrt::Point pos = gt.TransformPoint(winrt::Point());

            if (pos.Y == 0.0f)
            {
                topPadding = coreTitleBar.Height();
            }
        }

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

        auto paneTitleHolderFrameworkElement = m_paneTitleHolderFrameworkElement.get();
        auto paneToggleButton = m_paneToggleButton.get();

        bool setPaneTitleHolderFrameworkElementMargin = paneTitleHolderFrameworkElement && paneTitleHolderFrameworkElement.Visibility() == winrt::Visibility::Visible;
        bool setPaneToggleButtonMargin = !setPaneTitleHolderFrameworkElementMargin && paneToggleButton && paneToggleButton.Visibility() == winrt::Visibility::Visible;

        if (setPaneTitleHolderFrameworkElementMargin || setPaneToggleButtonMargin)
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
            else if (ShouldShowCloseButton() && IsOverlay())
            {
                thickness = winrt::ThicknessHelper::FromLengths(c_backButtonWidth, 0, 0, 0);
            }

            if (setPaneTitleHolderFrameworkElementMargin)
            {
                // The PaneHeader is hosted by PaneTitlePresenter and PaneTitleHolder.
                paneTitleHolderFrameworkElement.Margin(thickness);
            }
            else
            {
                // The PaneHeader is hosted by PaneToggleButton
                paneToggleButton.Margin(thickness);
            }
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

void NavigationView::SyncSettingsSelectionState()
{
    auto item = SelectedItem();
    auto settingsItem = m_settingsItem.get();
    if (settingsItem && item == settingsItem)
    {
        OnSettingsInvoked();
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
void NavigationView::CreateAndAttachHeaderAnimation(const winrt::Visual& visual)
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

void NavigationView::UpdatePaneShadow()
{
    if (SharedHelpers::IsThemeShadowAvailable())
    {
        winrt::Canvas shadowReceiver = GetTemplateChildT<winrt::Canvas>(c_paneShadowReceiverCanvas, *this);
        if (!shadowReceiver)
        {
            shadowReceiver = winrt::Canvas();
            shadowReceiver.Name(c_paneShadowReceiverCanvas);

            if (auto contentGrid = GetTemplateChildT<winrt::Grid>(c_contentGridName, *this))
            {
                contentGrid.SetRowSpan(shadowReceiver, contentGrid.RowDefinitions().Size());
                contentGrid.SetRow(shadowReceiver, 0);
                // Only register to columns if those are actually defined
                if (contentGrid.ColumnDefinitions().Size() > 0) {
                    contentGrid.SetColumn(shadowReceiver, 0);
                    contentGrid.SetColumnSpan(shadowReceiver, contentGrid.ColumnDefinitions().Size());
                }
                contentGrid.Children().Append(shadowReceiver);

                winrt::ThemeShadow shadow;
                shadow.Receivers().Append(shadowReceiver);
                if (auto splitView = m_rootSplitView.get())
                {
                    if (auto paneRoot = splitView.Pane())
                    {
                        if (auto paneRoot_uiElement10 = paneRoot.try_as<winrt::IUIElement10 >())
                        {
                            paneRoot_uiElement10.Shadow(shadow);
                        }
                    }
                }
            }
        }


        // Shadow will get clipped if casting on the splitView.Content directly
        // Creating a canvas with negative margins as receiver to allow shadow to be drawn outside the content grid 
        winrt::Thickness shadowReceiverMargin = { 0, -c_paneElevationTranslationZ, -c_paneElevationTranslationZ, -c_paneElevationTranslationZ };

        // Ensuring shadow is aligned to the left
        shadowReceiver.HorizontalAlignment(winrt::HorizontalAlignment::Left);

        // Ensure shadow is as wide as the pane when it is open
        shadowReceiver.Width(OpenPaneLength());
        shadowReceiver.Margin(shadowReceiverMargin);
    }
}

template<typename T> T NavigationView::GetContainerForData(const winrt::IInspectable& data)
{
    if (!data)
    {
        return nullptr;
    }

    if (auto nvi = data.try_as<T>())
    {
        return nvi;
    }

    if (auto settingsItem = m_settingsItem.get())
    {
        if (settingsItem == data || settingsItem.Content() == data)
        {
            return settingsItem.try_as<T>();
        }
    }

    // First conduct a basic top level search, which should succeed for a lot of scenarios.
    auto ir = IsTopNavigationView() ? m_topNavRepeater.get() : m_leftNavRepeater.get();
    auto itemIndex = GetIndexFromItem(ir, data);
    if (ir && itemIndex >= 0)
    {
        if (auto container = ir.TryGetElement(itemIndex))
        {
            return container.try_as<T>();
        }
    }

    // If unsuccessful, unfortunately we are going to have to search through the whole tree
    // TODO: Either fix or remove implementation for TopNav.
    // It may not be required due to top nav rarely having realized children in its default state.
    auto const repeater = IsTopNavigationView() ? m_topNavRepeater.get() : m_leftNavRepeater.get();
    if (auto const container = SearchEntireTreeForContainer(repeater, data))
    {
        return container.try_as<T>();
    }

    return nullptr;
}

winrt::UIElement NavigationView::SearchEntireTreeForContainer(const winrt::ItemsRepeater& rootRepeater, const winrt::IInspectable& data)
{
    // TODO: Temporary inefficient solution that results in unnecessary time complexity, fix.
    auto index = GetIndexFromItem(rootRepeater, data);
    if (index != -1)
    {
        return rootRepeater.TryGetElement(index);
    }

    for (int i = 0; i < GetContainerCountInRepeater(rootRepeater); i++)
    {
        if (auto const container = rootRepeater.TryGetElement(i))
        {
            if (auto const nvi = container.try_as<winrt::NavigationViewItem>())
            {
                if (auto const nviRepeater = winrt::get_self<NavigationViewItem>(nvi)->GetRepeater())
                {
                    if (auto const foundElement = SearchEntireTreeForContainer(nviRepeater, data))
                    {
                        return foundElement;
                    }
                }
            }
        }
    }
    return nullptr;
}

winrt::IndexPath NavigationView::SearchEntireTreeForIndexPath(const winrt::ItemsRepeater& rootRepeater, const winrt::IInspectable& data)
{
    for (int i = 0; i < GetContainerCountInRepeater(rootRepeater); i++)
    {
        if (auto const container = rootRepeater.TryGetElement(i))
        {
            if (auto const nvi = container.try_as<winrt::NavigationViewItem>())
            {
                auto const ip = winrt::make<IndexPath>(std::vector<int>({ i }));
                if (auto const indexPath = SearchEntireTreeForIndexPath(nvi, data, ip))
                {
                    return indexPath;
                }
            }
        }
    }
    return nullptr;
}

// There are two possibilities here if the passed in item has children. Either the children of the passed in container have already been realized,
// in which case we simply just iterate through the children containers, or they have not been realized yet and we have to iterate through the data
// and manually realize each item.
winrt::IndexPath NavigationView::SearchEntireTreeForIndexPath(const winrt::NavigationViewItem& parentContainer, const winrt::IInspectable& data, const winrt::IndexPath& ip)
{
    bool areChildrenRealized = false;
    if (auto const childrenRepeater = winrt::get_self<NavigationViewItem>(parentContainer)->GetRepeater())
    {
        if (DoesRepeaterHaveRealizedContainers(childrenRepeater))
        {
            areChildrenRealized = true;
            for (int i = 0; i < GetContainerCountInRepeater(childrenRepeater); i++)
            {
                if (auto const container = childrenRepeater.TryGetElement(i))
                {
                    if (auto const nvi = container.try_as<winrt::NavigationViewItem>())
                    {
                        auto const newIndexPath = winrt::get_self<IndexPath>(ip)->CloneWithChildIndex(i);
                        if (nvi.Content() == data)
                        {
                            return newIndexPath;
                        }
                        else
                        {
                            if (auto const foundIndexPath = SearchEntireTreeForIndexPath(nvi, data, newIndexPath))
                            {
                                return foundIndexPath;
                            }
                        }
                    }
                }
            }
        }
    }

    //If children are not realized, manually realize and search.
    if (!areChildrenRealized)
    {
        if (auto const childrenData = GetChildren(parentContainer))
        {
            // Get children data in an enumarable form
            auto newDataSource = childrenData.try_as<winrt::ItemsSourceView>();
            if (childrenData && !newDataSource)
            {
                newDataSource = winrt::ItemsSourceView(childrenData);
            }

            for (int i = 0; i < newDataSource.Count(); i++)
            {
                auto const newIndexPath = winrt::get_self<IndexPath>(ip)->CloneWithChildIndex(i);
                auto const childData = newDataSource.GetAt(i);
                if (childData == data)
                {
                    return newIndexPath;
                }
                else
                {
                    // Resolve databinding for item and search through that item's children
                    if (auto const nvib = ResolveContainerForItem(childData, i))
                    {
                        if (auto const nvi = nvib.try_as<winrt::NavigationViewItem>())
                        {
                            // Process x:bind
                            if (auto extension = CachedVisualTreeHelpers::GetDataTemplateComponent(nvi))
                            {
                                // Clear out old data. 
                                extension.Recycle();
                                int nextPhase = VirtualizationInfo::PhaseReachedEnd;
                                // Run Phase 0
                                extension.ProcessBindings(childData, i, 0 /* currentPhase */, nextPhase);

                                // TODO: If nextPhase is not -1, ProcessBinding for all the phases
                            }

                            if (auto const foundIndexPath = SearchEntireTreeForIndexPath(nvi, data, newIndexPath))
                            {
                                return foundIndexPath;
                            }

                            //TODO: Recycle container!
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

winrt::NavigationViewItemBase NavigationView::ResolveContainerForItem(const winrt::IInspectable& item, int index)
{
    auto const args = winrt::make_self<ElementFactoryGetArgs>();
    args->Data(item);
    args->Index(index);

    if (auto container = m_navigationViewItemsFactory.get()->GetElement(static_cast<winrt::ElementFactoryGetArgs>(*args)))
    {
        if (auto nvib = container.try_as<winrt::NavigationViewItemBase>())
        {
            return nvib;
        }
    }
    return nullptr;
}

void NavigationView::RecycleContainer(const winrt::UIElement& container)
{
    auto const args = winrt::make_self<ElementFactoryRecycleArgs>();
    args->Element(container);
    m_navigationViewItemsFactory.get()->RecycleElement(static_cast<winrt::ElementFactoryRecycleArgs>(*args));
}

int NavigationView::GetContainerCountInRepeater(const winrt::ItemsRepeater& ir)
{
    if (ir)
    {
        if (auto const repeaterItemSourceView = ir.ItemsSourceView())
        {
            return repeaterItemSourceView.Count();
        }
    }
    return -1;
}

bool NavigationView::DoesRepeaterHaveRealizedContainers(const winrt::ItemsRepeater& ir)
{
    if (ir)
    {
        if (ir.TryGetElement(0))
        {
            return true;
        }
    }
    return false;
}

int NavigationView::GetIndexFromItem(const winrt::ItemsRepeater& ir, const winrt::IInspectable& data)
{
    if (ir)
    {
        if (auto itemsSourceView = ir.ItemsSourceView())
        {
            return itemsSourceView.IndexOf(data);
        }
    }
    return -1;
}

winrt::IInspectable NavigationView::GetItemFromIndex(const winrt::ItemsRepeater& ir, int index)
{
    if (ir)
    {
        if (auto itemsSourceView = ir.ItemsSourceView())
        {
            return itemsSourceView.GetAt(index);
        }
    }
    return nullptr;
}

winrt::IndexPath NavigationView::GetIndexPathOfItem(const winrt::IInspectable& data)
{
    if (auto const nvib = data.try_as<winrt::NavigationViewItemBase>())
    {
        return GetIndexPathForContainer(nvib);
    }

    // In the databinding scenario, we need to conduct a search where we go through every item,
    // realizing it if necessary.
    if (IsTopNavigationView())
    {
        // First search through primary list
        if (auto const ip = SearchEntireTreeForIndexPath(m_topNavRepeater.get(), data))
        {
            return ip;
        }

        // If item was not located in primary list, search through overflow
        if (auto const ip = SearchEntireTreeForIndexPath(m_topNavRepeaterOverflowView.get(), data))
        {
            return ip;
        }
    }
    else
    {
        if (auto const ip = SearchEntireTreeForIndexPath(m_leftNavRepeater.get(), data))
        {
            return ip;
        }
    }

    return winrt::make<IndexPath>(std::vector<int>(0));
}

winrt::UIElement NavigationView::GetContainerForIndex(int index)
{
    if (IsTopNavigationView())
    {
        // Get the repeater that is presenting the first item
        auto ir = m_topDataProvider.IsItemInPrimaryList(index) ? m_topNavRepeater.get() : m_topNavRepeaterOverflowView.get();

        // Get the index of the first item in the repeater
        auto irIndex = m_topDataProvider.ConvertOriginalIndexToIndex(index);

        // Get the container of the first item
        if (auto const container = ir.TryGetElement(irIndex))
        {
            return container;
        }
    }
    else
    {
        if (auto const container = m_leftNavRepeater.get().TryGetElement(index))
        {
            return container;
        }
    }
    return nullptr;
}

winrt::NavigationViewItemBase NavigationView::GetContainerForIndexPath(const winrt::IndexPath& ip)
{
    if (ip && ip.GetSize() > 0)
    {
        if (auto const container = GetContainerForIndex(ip.GetAt(0)))
        {
            // TODO: Fix below for top flyout scenario once the flyout is introduced in the XAML.
            // We want to be able to retrieve containers for items that are in the flyout.
            // This will return nullptr if requesting children containers of
            // items in the primary list, or unrealized items in the overflow popup.
            // However this should not happen.
            return GetContainerForIndexPath(container, ip);
        }
    }
    return nullptr;
}

winrt::NavigationViewItemBase NavigationView::GetContainerForIndexPath(const winrt::UIElement& firstContainer, const winrt::IndexPath& ip)
{
    auto container = firstContainer;
    if (ip.GetSize() > 1)
    {
        for (int i = 1; i < ip.GetSize(); i++)
        {
            bool succeededGettingNextContainer = false;
            if (auto const nvi = container.try_as<winrt::NavigationViewItem>())
            {
                if (auto const nviRepeater = winrt::get_self<NavigationViewItem>(nvi)->GetRepeater())
                {
                    if (auto const nextContainer = nviRepeater.TryGetElement(ip.GetAt(i)))
                    {
                        container = nextContainer;
                        succeededGettingNextContainer = true;
                    }
                }
            }
            // If any of the above checks failed, it means something went wrong and we have an index for a non-existent repeater.
            if (!succeededGettingNextContainer)
            {
                return nullptr;
            }
        }
    }
    return container.try_as<winrt::NavigationViewItemBase>();
}

bool NavigationView::IsContainerTheSelectedItemInTheSelectionModel(const winrt::NavigationViewItemBase& nvib)
{
    if (auto selectedItem = m_selectionModel.SelectedItem())
    {
        auto selectedItemContainer = selectedItem.try_as<winrt::NavigationViewItemBase>();
        if (!selectedItemContainer)
        {
            selectedItemContainer = GetContainerForIndexPath(m_selectionModel.SelectedIndex());
        }

        return selectedItemContainer == nvib;
    }
    return false;
}

winrt::ItemsRepeater NavigationView::LeftNavRepeater()
{
    return m_leftNavRepeater.get();
}

bool NavigationView::IsContainerInOverflow(const winrt::NavigationViewItemBase& nvib)
{
    if (IsTopNavigationView())
    {
        auto parentIR = GetParentItemsRepeaterForContainer(nvib);
        if (parentIR == m_topNavRepeaterOverflowView.get())
        {
            return true;
        }
    }
    return false;
}

winrt::NavigationViewItem NavigationView::GetSelectedContainer()
{
    if (auto selectedItem = SelectedItem())
    {
        if (auto selectedItemContainer = selectedItem.try_as<winrt::NavigationViewItem>())
        {
            return selectedItemContainer;
        }
        else
        {
            return NavigationViewItemOrSettingsContentFromData(selectedItem);
        }
    }
    return nullptr;
}

void NavigationView::Expand(const winrt::NavigationViewItem& item)
{
    ChangeIsExpandedNavigationViewItem(item, true /*isExpanded*/);
}

void NavigationView::Collapse(const winrt::NavigationViewItem& item)
{
    ChangeIsExpandedNavigationViewItem(item, false /*isExpanded*/);
}

bool NavigationView::DoesNavigationViewItemHaveChildren(const winrt::NavigationViewItem& nvi)
{
    return nvi.MenuItems().Size() > 0 || nvi.MenuItemsSource() != nullptr || nvi.HasUnrealizedChildren();
}

void NavigationView::ToggleIsExpandedNavigationViewItem(const winrt::NavigationViewItem& nvi)
{
    ChangeIsExpandedNavigationViewItem(nvi, !nvi.IsExpanded());
}

void NavigationView::ChangeIsExpandedNavigationViewItem(const winrt::NavigationViewItem& nvi, bool isExpanded)
{
    if (DoesNavigationViewItemHaveChildren(nvi))
    {
        nvi.IsExpanded(isExpanded);
    }
}

winrt::NavigationViewItem NavigationView::FindLowestLevelContainerToDisplaySelectionIndicator()
{
    auto indexIntoIndex = 0;
    auto const selectedIndex = m_selectionModel.SelectedIndex();
    if (selectedIndex && selectedIndex.GetSize() > 0)
    {
        if (auto container = GetContainerForIndex(selectedIndex.GetAt(indexIntoIndex)))
        {
            if (auto nvi = container.try_as<winrt::NavigationViewItem>())
            {
                auto nviImpl = winrt::get_self<NavigationViewItem>(nvi);
                auto isRepeaterVisible = nviImpl->IsRepeaterVisible();
                while (nvi && isRepeaterVisible && !nvi.IsSelected() && nvi.IsChildSelected())
                {
                    indexIntoIndex++;
                    isRepeaterVisible = false;
                    if (auto const repeater = nviImpl->GetRepeater())
                    {
                        if (auto const childContainer = repeater.TryGetElement(selectedIndex.GetAt(indexIntoIndex)))
                        {
                            nvi = childContainer.try_as<winrt::NavigationViewItem>();
                            nviImpl = winrt::get_self<NavigationViewItem>(nvi);
                            isRepeaterVisible = nviImpl->IsRepeaterVisible();
                        }
                    }
                }
                return nvi;
            }
        }
    }
    return nullptr;
}

void NavigationView::ShowHideChildrenItemsRepeater(const winrt::NavigationViewItem& nvi)
{
    auto nviImpl = winrt::get_self<NavigationViewItem>(nvi);

    nviImpl->ShowHideChildren();

    if (nviImpl->ShouldRepeaterShowInFlyout())
    {
        nvi.IsExpanded() ? m_lastItemExpandedIntoFlyout.set(nvi) : m_lastItemExpandedIntoFlyout.set(nullptr);
    }

    // If SelectedItem is being hidden/shown, animate SelectionIndicator
    if (!nvi.IsSelected() && nvi.IsChildSelected())
    {
        if (!nviImpl->IsRepeaterVisible() && nvi.IsChildSelected())
        {
            AnimateSelectionChanged(nvi);
        }
        else
        {
            AnimateSelectionChanged(FindLowestLevelContainerToDisplaySelectionIndicator());
        }
    }

    nviImpl->RotateExpandCollapseChevron(nvi.IsExpanded());
}

winrt::IInspectable NavigationView::GetChildren(const winrt::NavigationViewItem& nvi)
{
    if (nvi.MenuItems().Size() > 0)
    {
        return nvi.MenuItems();
    }
    return nvi.MenuItemsSource();
}

winrt::ItemsRepeater NavigationView::GetChildRepeaterForIndexPath(const winrt::IndexPath& ip)
{
    if (auto const container = GetContainerForIndexPath(ip).try_as<winrt::NavigationViewItem>())
    {
        return winrt::get_self<NavigationViewItem>(container)->GetRepeater();
    }
    return nullptr;
}


winrt::IInspectable NavigationView::GetChildrenForItemInIndexPath(const winrt::IndexPath& ip, bool forceRealize)
{
    if (ip && ip.GetSize() > 0)
    {
        if (auto const container = GetContainerForIndex(ip.GetAt(0)))
        {
            return GetChildrenForItemInIndexPath(container, ip, forceRealize);
        }
    }
    return nullptr;
}

winrt::IInspectable NavigationView::GetChildrenForItemInIndexPath(const winrt::UIElement& firstContainer, const winrt::IndexPath& ip, bool forceRealize)
{
    auto container = firstContainer;
    bool shouldRecycleContainer = false;
    if (ip.GetSize() > 1)
    {
        for (int i = 1; i < ip.GetSize(); i++)
        {
            bool succeededGettingNextContainer = false;
            if (auto const nvi = container.try_as<winrt::NavigationViewItem>())
            {
                auto const nextContainerIndex = ip.GetAt(i);
                auto const nviRepeater = winrt::get_self<NavigationViewItem>(nvi)->GetRepeater();
                if (nviRepeater && DoesRepeaterHaveRealizedContainers(nviRepeater))
                {
                    if (auto const nextContainer = nviRepeater.TryGetElement(nextContainerIndex))
                    {
                        container = nextContainer;
                        succeededGettingNextContainer = true;
                    }
                }
                else if (forceRealize)
                {
                    if (auto const childrenData = GetChildren(nvi))
                    {
                        if (shouldRecycleContainer)
                        {
                            RecycleContainer(nvi);
                            shouldRecycleContainer = false;
                        }

                        // Get children data in an enumarable form
                        auto newDataSource = childrenData.try_as<winrt::ItemsSourceView>();
                        if (childrenData && !newDataSource)
                        {
                            newDataSource = winrt::ItemsSourceView(childrenData);
                        }

                        if (auto const data = newDataSource.GetAt(nextContainerIndex))
                        {
                            // Resolve databinding for item and search through that item's children
                            if (auto const nvib = ResolveContainerForItem(data, nextContainerIndex))
                            {
                                if (auto const nextContainer = nvib.try_as<winrt::NavigationViewItem>())
                                {
                                    // Process x:bind
                                    if (auto extension = CachedVisualTreeHelpers::GetDataTemplateComponent(nextContainer))
                                    {
                                        // Clear out old data. 
                                        extension.Recycle();
                                        int nextPhase = VirtualizationInfo::PhaseReachedEnd;
                                        // Run Phase 0
                                        extension.ProcessBindings(data, nextContainerIndex, 0 /* currentPhase */, nextPhase);

                                        // TODO: If nextPhase is not -1, ProcessBinding for all the phases
                                    }

                                    container = nextContainer;
                                    shouldRecycleContainer = true;
                                    succeededGettingNextContainer = true;
                                }
                            }
                        }
                    }
                }

            }
            // If any of the above checks failed, it means something went wrong and we have an index for a non-existent repeater.
            if (!succeededGettingNextContainer)
            {
                return nullptr;
            }
        }
    }

    if (auto const nvi = container.try_as<winrt::NavigationViewItem>())
    {
        auto const children = GetChildren(nvi);
        if (shouldRecycleContainer)
        {
            RecycleContainer(nvi);
        }
        return children;
    }

    return nullptr;
}

void NavigationView::CollapseTopLevelMenuItems(winrt::NavigationViewPaneDisplayMode oldDisplayMode)
{
    // We want to make sure only top level items are visible when switching pane modes
    if (oldDisplayMode == winrt::NavigationViewPaneDisplayMode::Top)
    {
        CollapseMenuItemsInRepeater(m_topNavRepeater.get());
        CollapseMenuItemsInRepeater(m_topNavRepeaterOverflowView.get());
    }
    else
    {
        CollapseMenuItemsInRepeater(m_leftNavRepeater.get());
    }
}

void NavigationView::CollapseMenuItemsInRepeater(const winrt::ItemsRepeater& ir)
{
    for (int index = 0; index < GetContainerCountInRepeater(ir); index++)
    {
        if (auto const element = ir.TryGetElement(index))
        {
            if (auto const nvi = element.try_as<winrt::NavigationViewItem>())
            {
                ChangeIsExpandedNavigationViewItem(nvi, false /*isExpanded*/);
            }
        }
    }
}

void NavigationView::RaiseExpandingEvent(const winrt::NavigationViewItemBase& container)
{
    auto eventArgs = winrt::make_self<NavigationViewItemExpandingEventArgs>(*this);
    eventArgs->ExpandingItemContainer(container);
    m_expandingEventSource(*this, *eventArgs);
}

void NavigationView::RaiseCollapsedEvent(const winrt::NavigationViewItemBase& container)
{
    auto eventArgs = winrt::make_self<NavigationViewItemCollapsedEventArgs>(*this);
    eventArgs->CollapsedItemContainer(container);
    m_collapsedEventSource(*this, *eventArgs);
}

bool NavigationView::IsTopLevelItem(const winrt::NavigationViewItemBase& nvib)
{
    return IsRootItemsRepeater(GetParentItemsRepeaterForContainer(nvib));
}
