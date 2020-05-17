// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class NavigationViewVisualStateDisplayMode
{
    Compact,
    Expanded,
    Minimal,
    MinimalWithBackButton
};

enum class NavigationViewRepeaterPosition
{
    LeftNav,
    TopPrimary,
    TopOverflow,
    LeftFooter,
    TopFooter
};

enum class NavigationViewPropagateTarget
{
    LeftListView,
    TopListView,
    OverflowListView,
    All
};

// Since RS5, a lot of functions in NavigationViewItem is moved to NavigationViewItemPresenter. So they both share some common codes.
// This class helps to initialize and maintain the status of SelectionIndicator and ToolTip
template<typename T>
class NavigationViewItemHelper
{
public:
    NavigationViewItemHelper(const ITrackerHandleManager* owner): m_owner(owner)
    {
    }

    winrt::UIElement GetSelectionIndicator() { return m_selectionIndicator.get(); }

    void Init(const winrt::IControlProtected & controlProtected)
    {
        m_selectionIndicator.set(GetTemplateChildT<winrt::UIElement>(c_selectionIndicatorName, controlProtected));
    }

private:
    const ITrackerHandleManager* m_owner;
    tracker_ref<winrt::UIElement> m_selectionIndicator{ m_owner };

    static constexpr wstring_view c_selectionIndicatorName = L"SelectionIndicator"sv;
};

static constexpr wstring_view c_OnLeftNavigationReveal = L"OnLeftNavigationReveal"sv;
static constexpr wstring_view c_OnLeftNavigation = L"OnLeftNavigation"sv;
static constexpr wstring_view c_OnTopNavigationPrimary = L"OnTopNavigationPrimary"sv;
static constexpr wstring_view c_OnTopNavigationPrimaryReveal = L"OnTopNavigationPrimaryReveal"sv;
static constexpr wstring_view c_OnTopNavigationOverflow = L"OnTopNavigationOverflow"sv;
