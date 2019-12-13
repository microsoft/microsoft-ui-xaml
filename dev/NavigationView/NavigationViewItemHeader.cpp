// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemHeader.h"
#include "NavigationView.h"

#include "NavigationViewItemHeader.properties.cpp"

NavigationViewItemHeader::NavigationViewItemHeader()
{
    SetDefaultStyleKey(this);
}

void NavigationViewItemHeader::OnApplyTemplate()
{
    if (auto splitView = GetSplitView())
    {
        m_splitViewIsPaneOpenChangedRevoker = RegisterPropertyChanged(splitView,
            winrt::SplitView::IsPaneOpenProperty(), { this, &NavigationViewItemHeader::OnSplitViewPropertyChanged });
        m_splitViewDisplayModeChangedRevoker = RegisterPropertyChanged(splitView,
            winrt::SplitView::DisplayModeProperty(), { this, &NavigationViewItemHeader::OnSplitViewPropertyChanged });

        UpdateIsClosedCompact();
    }

    UpdateVisualState(false /*useTransitions*/);

    auto visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    NavigationView::CreateAndAttachHeaderAnimation(visual);
}

void NavigationViewItemHeader::OnSplitViewPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    if (args == winrt::SplitView::IsPaneOpenProperty() ||
        args == winrt::SplitView::DisplayModeProperty())
    {
        UpdateIsClosedCompact();
    }
}

void NavigationViewItemHeader::UpdateIsClosedCompact()
{
    if (auto splitView = GetSplitView())
    {
        // Check if the pane is closed and if the splitview is in either compact mode.
        m_isClosedCompact = !splitView.IsPaneOpen() && (splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactOverlay || splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactInline);
        UpdateVisualState(true /*useTransitions*/);
    }
}

void NavigationViewItemHeader::UpdateVisualState(bool useTransitions)
{
    winrt::VisualStateManager::GoToState(*this, m_isClosedCompact ? L"HeaderTextCollapsed" : L"HeaderTextVisible", useTransitions);
}
