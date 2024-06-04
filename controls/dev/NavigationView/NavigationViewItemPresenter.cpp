// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemPresenter.h"
#include "NavigationViewItem.h"
#include "SharedHelpers.h"

static constexpr auto c_contentGrid = L"PresenterContentRootGrid"sv;
static constexpr auto c_infoBadgePresenter = L"InfoBadgePresenter"sv;
static constexpr auto c_expandCollapseChevron = L"ExpandCollapseChevron"sv;
static constexpr auto c_expandCollapseRotateExpandedStoryboard = L"ExpandCollapseRotateExpandedStoryboard"sv;
static constexpr auto c_expandCollapseRotateCollapsedStoryboard = L"ExpandCollapseRotateCollapsedStoryboard"sv;

NavigationViewItemPresenter::NavigationViewItemPresenter()
{
    SetValue(s_TemplateSettingsProperty, winrt::make<::NavigationViewItemPresenterTemplateSettings>());
    SetDefaultStyleKey(this);
}

void NavigationViewItemPresenter::UnhookEventsAndClearFields()
{
    m_expandCollapseChevronPointerPressedRevoker.revoke();
    m_expandCollapseChevronPointerReleasedRevoker.revoke();
    m_expandCollapseChevronPointerExitedRevoker.revoke();
    m_expandCollapseChevronPointerCanceledRevoker.revoke();
    m_expandCollapseChevronPointerCaptureLostRevoker.revoke();

    m_contentGrid.set(nullptr);
    m_infoBadgePresenter.set(nullptr);
    m_expandCollapseChevron.set(nullptr);
    m_chevronExpandedStoryboard.set(nullptr);
    m_chevronCollapsedStoryboard.set(nullptr);

    m_isChevronPressed = false;
}

void NavigationViewItemPresenter::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected = *this;

    // Retrieve pointers to stable controls 
    m_helper.Init(*this);

    UnhookEventsAndClearFields();

    if (auto contentGrid = GetTemplateChildT<winrt::Grid>(c_contentGrid, *this))
    {
        m_contentGrid.set(contentGrid);
    }

    m_infoBadgePresenter.set(GetTemplateChildT<winrt::ContentPresenter>(c_infoBadgePresenter, controlProtected));

    if (auto navigationViewItem = GetNavigationViewItem())
    {
        if (navigationViewItem->HasPotentialChildren())
        {
            LoadChevron();
        }
        navigationViewItem->UpdateVisualStateNoTransition();

        // We probably switched displaymode, so restore width now, otherwise the next time we will restore is when the CompactPaneLength changes
        if (const auto&& navigationView = navigationViewItem->GetNavigationView())
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

void NavigationViewItemPresenter::LoadChevron()
{
    if (!m_expandCollapseChevron)
    {
        if (auto navigationViewItem = GetNavigationViewItem())
        {
            if (auto const expandCollapseChevron = GetTemplateChildT<winrt::Grid>(c_expandCollapseChevron, *this))
            {
                m_expandCollapseChevron.set(expandCollapseChevron);

                m_expandCollapseChevronPointerPressedRevoker = expandCollapseChevron.PointerPressed(winrt::auto_revoke, { this, &NavigationViewItemPresenter::OnExpandCollapseChevronPointerPressed });
                m_expandCollapseChevronPointerReleasedRevoker = expandCollapseChevron.PointerReleased(winrt::auto_revoke, { this, &NavigationViewItemPresenter::OnExpandCollapseChevronPointerReleased });

                m_expandCollapseChevronPointerCanceledRevoker = AddRoutedEventHandler<RoutedEventType::PointerCanceled>(
                    *this,
                    { this, &NavigationViewItemPresenter::OnExpandCollapseChevronPointerCanceled },
                    true /*handledEventsToo*/);
                    
                m_expandCollapseChevronPointerExitedRevoker = AddRoutedEventHandler<RoutedEventType::PointerExited>(
                    *this,
                    { this, &NavigationViewItemPresenter::OnExpandCollapseChevronPointerExited },
                    true /*handledEventsToo*/);

                m_expandCollapseChevronPointerCaptureLostRevoker = AddRoutedEventHandler<RoutedEventType::PointerCaptureLost>(
                    *this,
                    { this, &NavigationViewItemPresenter::OnExpandCollapseChevronPointerCaptureLost },
                    true /*handledEventsToo*/);
            }
        }
    }
}


void NavigationViewItemPresenter::ResetTrackedPointerId()
{
    m_trackedPointerId = 0;
}

// Returns False when the provided pointer Id matches the currently tracked Id.
// When there is no currently tracked Id, sets the tracked Id to the provided Id and returns False.
// Returns True when the provided pointer Id does not match the currently tracked Id.
bool NavigationViewItemPresenter::IgnorePointerId(const winrt::PointerRoutedEventArgs& args)
{
    uint32_t pointerId = args.Pointer().PointerId();

    if (m_trackedPointerId == 0)
    {
        m_trackedPointerId = pointerId;
    }
    else if (m_trackedPointerId != pointerId)
    {
        return true;
    }
    return false;
}

void NavigationViewItemPresenter::OnExpandCollapseChevronPointerPressed(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    const bool ignorePointerId = IgnorePointerId(args);
    const auto pointerProperties = args.GetCurrentPoint(*this).Properties();
    if (ignorePointerId ||
        !pointerProperties.IsLeftButtonPressed() ||
        args.Handled())
    {
        // We are only interested in the primary action of the pointer device 
        // (e.g. left click of a mouse)
		// Despite the name, IsLeftButtonPressed covers the primary action regardless of device.
        return;
    }

    m_isChevronPressed = true;
    args.Handled(true);
}

void NavigationViewItemPresenter::OnExpandCollapseChevronPointerReleased(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    const auto navigationViewItem = GetNavigationViewItem();
    const auto pointerProperties = args.GetCurrentPoint(*this).Properties();
    if (!args.Handled() &&
        m_isChevronPressed &&
        pointerProperties.PointerUpdateKind() == winrt::PointerUpdateKind::LeftButtonReleased &&
        navigationViewItem)
    {
        navigationViewItem->OnExpandCollapseChevronPointerReleased();
        args.Handled(true);
    }

    m_isChevronPressed = false;
    ResetTrackedPointerId();
}

void NavigationViewItemPresenter::OnExpandCollapseChevronPointerCanceled(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    m_isChevronPressed = false;
    ResetTrackedPointerId();
}

void NavigationViewItemPresenter::OnExpandCollapseChevronPointerExited(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    m_isChevronPressed = false;
    ResetTrackedPointerId();
}

void NavigationViewItemPresenter::OnExpandCollapseChevronPointerCaptureLost(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    m_isChevronPressed = false;
    ResetTrackedPointerId();
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
        if (auto const infoBadgePresenter = m_infoBadgePresenter.get())
        {
            infoBadgePresenter.Content(nullptr);
        }
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
        const auto templateSettings = winrt::get_self<NavigationViewItemPresenterTemplateSettings>(TemplateSettings());
        const auto gridLength = compactPaneLength;

        templateSettings->IconWidth(gridLength);
        templateSettings->SmallerIconWidth(std::max(0.0, gridLength - 8));
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
