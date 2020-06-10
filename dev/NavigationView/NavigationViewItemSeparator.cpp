// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemSeparator.h"
#include "Utils.h"

#include "NavigationViewItemSeparator.properties.cpp"

static constexpr auto c_rootGrid = L"NavigationViewItemSeparatorRootGrid"sv;

NavigationViewItemSeparator::NavigationViewItemSeparator()
{
    SetDefaultStyleKey(this);
}

void NavigationViewItemSeparator::UpdateVisualState(bool useTransitions)
{
    if (m_appliedTemplate)
    {
        static auto groupName = L"NavigationSeparatorLineStates"sv;
        auto stateName = (Position() != NavigationViewRepeaterPosition::TopPrimary)
            ? m_isClosedCompact
                ? L"HorizontalLineCompact"sv
                : L"HorizontalLine"sv
            : L"VerticalLine"sv;

        VisualStateUtil::GoToStateIfGroupExists(*this, groupName, stateName, false /*useTransitions*/);
    }
}

void NavigationViewItemSeparator::OnApplyTemplate()
{
    // Stop UpdateVisualState before template is applied. Otherwise the visual may not the same as we expect
    m_appliedTemplate = false;
    NavigationViewItemBase::OnApplyTemplate();

    if (auto rootGrid = GetTemplateChildT<winrt::Grid>(c_rootGrid, *this))
    {
        m_rootGrid.set(rootGrid);
    }

    if (auto splitView = GetSplitView())
    {
        m_splitViewIsPaneOpenChangedRevoker = RegisterPropertyChanged(splitView,
            winrt::SplitView::IsPaneOpenProperty(), { this, &NavigationViewItemSeparator::OnSplitViewPropertyChanged });
        m_splitViewDisplayModeChangedRevoker = RegisterPropertyChanged(splitView,
            winrt::SplitView::DisplayModeProperty(), { this, &NavigationViewItemSeparator::OnSplitViewPropertyChanged });

        UpdateIsClosedCompact(false);
    }

    m_appliedTemplate = true;
    UpdateVisualState(false /*useTransition*/);
    UpdateItemIndentation();
}

void NavigationViewItemSeparator::OnNavigationViewItemBaseDepthChanged()
{
    UpdateItemIndentation();
}

void NavigationViewItemSeparator::OnNavigationViewItemBasePositionChanged()
{
    UpdateVisualState(false /*useTransition*/);
}

void NavigationViewItemSeparator::OnSplitViewPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/)
{
    UpdateIsClosedCompact(true);
}

void NavigationViewItemSeparator::UpdateItemIndentation()
{
    // Update item indentation based on its depth
    if (auto const rootGrid = m_rootGrid.get())
    {
        auto const oldMargin = rootGrid.Margin();
        auto newLeftMargin = Depth() * c_itemIndentation;
        rootGrid.Margin({ static_cast<double>(newLeftMargin), oldMargin.Top, oldMargin.Right, oldMargin.Bottom });
    }
}

void NavigationViewItemSeparator::UpdateIsClosedCompact(bool updateVisualState)
{
    if (auto splitView = GetSplitView())
    {
        // Check if the pane is closed and if the splitview is in either compact mode
        m_isClosedCompact = !splitView.IsPaneOpen()
            && (splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactOverlay || splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactInline);

        if (updateVisualState)
        {
            UpdateVisualState(false /*useTransition*/);
        }
    }
}
