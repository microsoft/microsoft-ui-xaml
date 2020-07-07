// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemPresenter.h"
#include "NavigationViewItem.h"
#include "SharedHelpers.h"

static constexpr auto c_contentGrid = L"PresenterContentRootGrid"sv;
static constexpr auto c_expandCollapseChevron = L"ExpandCollapseChevron"sv;
static constexpr auto c_expandCollapseRotateExpandedStoryboard = L"ExpandCollapseRotateExpandedStoryboard"sv;
static constexpr auto c_expandCollapseRotateCollapsedStoryboard = L"ExpandCollapseRotateCollapsedStoryboard"sv;

static constexpr auto c_iconBoxColumnDefinitionName = L"IconColumn"sv;

NavigationViewItemPresenter::NavigationViewItemPresenter()
{
    SetDefaultStyleKey(this);
}

void NavigationViewItemPresenter::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected = *this;

    // Retrieve pointers to stable controls 
    m_helper.Init(*this);

    if (auto contentGrid = GetTemplateChildT<winrt::Grid>(c_contentGrid, *this))
    {
        m_contentGrid.set(contentGrid);
    }

    if (auto navigationViewItem = GetNavigationViewItem())
    {
        if (auto const expandCollapseChevron = GetTemplateChildT<winrt::Grid>(c_expandCollapseChevron, *this))
        {
            m_expandCollapseChevron.set(expandCollapseChevron);
            m_expandCollapseChevronTappedToken = expandCollapseChevron.Tapped({ navigationViewItem, &NavigationViewItem::OnExpandCollapseChevronTapped });
        }
        navigationViewItem->UpdateVisualStateNoTransition();


        // We probably switched displaymode, so restore width now, otherwise the next time we will restore is when the CompactPaneLength changes
        if(const auto& navigationView = navigationViewItem->GetNavigationView())
        {
            if (navigationView.PaneDisplayMode() != winrt::NavigationViewPaneDisplayMode::Top)
            {
                UpdateCompactPaneLength(m_compactPaneLengthValue, true);
            }
        }
    }

    m_chevronExpandedStoryboard.set(GetTemplateChildT<winrt::Storyboard>(c_expandCollapseRotateExpandedStoryboard, *this));
    m_chevronCollapsedStoryboard.set(GetTemplateChildT<winrt::Storyboard>(c_expandCollapseRotateCollapsedStoryboard, *this));

    UpdateMargin();
}

void NavigationViewItemPresenter::RotateExpandCollapseChevron(bool isExpanded)
{
    if (isExpanded)
    {
        if (auto const openStoryboard = m_chevronExpandedStoryboard.get())
        {
            openStoryboard.Begin();
        }
    }
    else
    {
        if (auto const closedStoryboard = m_chevronCollapsedStoryboard.get())
        {
            closedStoryboard.Begin();
        }
    }
}

winrt::UIElement NavigationViewItemPresenter::GetSelectionIndicator()
{
    return m_helper.GetSelectionIndicator();  
}

bool NavigationViewItemPresenter::GoToElementStateCore(winrt::hstring const& state, bool useTransitions)
{
    // GoToElementStateCore: Update visualstate for itself.
    // VisualStateManager::GoToState: update visualstate for it's first child.

    // If NavigationViewItemPresenter is used, two sets of VisualStateGroups are supported. One set is help to switch the style and it's NavigationViewItemPresenter itself and defined in NavigationViewItem
    // Another set is defined in style for NavigationViewItemPresenter.
    // OnLeftNavigation, OnTopNavigationPrimary, OnTopNavigationOverflow only apply to itself.
    if (state == c_OnLeftNavigation || state == c_OnLeftNavigationReveal || state == c_OnTopNavigationPrimary
        || state == c_OnTopNavigationPrimaryReveal || state == c_OnTopNavigationOverflow)
    {
        return __super::GoToElementStateCore(state, useTransitions);
    }
    return winrt::VisualStateManager::GoToState(*this, state, useTransitions);
}

NavigationViewItem* NavigationViewItemPresenter::GetNavigationViewItem()
{
    NavigationViewItem* navigationViewItem = nullptr;

    winrt::DependencyObject obj = operator winrt::DependencyObject();

    if (auto item = SharedHelpers::GetAncestorOfType<winrt::NavigationViewItem>(winrt::VisualTreeHelper::GetParent(obj)))
    {
        navigationViewItem = winrt::get_self<NavigationViewItem>(item);
    }
    return navigationViewItem;
}

void NavigationViewItemPresenter::UpdateContentLeftIndentation(double leftIndentation)
{
    m_leftIndentation = leftIndentation;
    UpdateMargin();
}

void NavigationViewItemPresenter::UpdateMargin()
{
    if (auto const grid = m_contentGrid.get())
    {
        auto const oldGridMargin = grid.Margin();
        grid.Margin({ m_leftIndentation, oldGridMargin.Top, oldGridMargin.Right, oldGridMargin.Bottom });
    }
}

void NavigationViewItemPresenter::UpdateCompactPaneLength(double compactPaneLength, bool shouldUpdate)
{
    m_compactPaneLengthValue = compactPaneLength;
    if (shouldUpdate)
    {
        if (auto iconGridColumn = GetTemplateChildT<winrt::ColumnDefinition>(c_iconBoxColumnDefinitionName, *this))
        {
            auto gridLength = iconGridColumn.Width();
            gridLength.Value = compactPaneLength;
            iconGridColumn.Width(gridLength);
        }
    }
}

void NavigationViewItemPresenter::UpdateClosedCompactVisualState(bool isTopLevelItem, bool isClosedCompact)
{
    // We increased the ContentPresenter margin to align it visually with the expand/collapse chevron. This updated margin is even applied when the
    // NavigationView is in a visual state where no expand/collapse chevrons are shown, leading to more content being cut off than necessary.
    // This is the case for top-level items when the NavigationView is in a compact mode and the NavigationView pane is closed. To keep the original
    // cutoff visual experience intact, we restore  the original ContentPresenter margin for such top-level items only (children shown in a flyout
    // will use the updated margin).
    const auto stateName = isClosedCompact && isTopLevelItem
        ? L"ClosedCompactAndTopLevelItem"
        : L"NotClosedCompactAndTopLevelItem";

    winrt::VisualStateManager::GoToState(*this, stateName, false /*useTransitions*/);
}
