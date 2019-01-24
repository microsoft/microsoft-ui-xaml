// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>

#include "NavigationView.h"
#include "NavigationViewItem.h"
#include "NavigationViewItemAutomationPeer.h"
#include "Utils.h"
#include "NavigationViewList.h"


static constexpr wstring_view c_navigationViewItemPresenterName = L"NavigationViewItemPresenter"sv;


void NavigationViewItem::UpdateVisualStateNoTransition()
{
    UpdateVisualState(false /*useTransition*/);
}

void NavigationViewItem::OnNavigationViewListPositionChanged()
{
    UpdateVisualStateNoTransition();
}

NavigationViewItem::NavigationViewItem()
{
    SetDefaultStyleKey(this);

    auto items = winrt::make<Vector<winrt::IInspectable>>();
    SetValue(s_MenuItemsProperty, items);
}

NavigationViewItem::~NavigationViewItem()
{
}

void NavigationViewItem::OnApplyTemplate()
{
    // Stop UpdateVisualState before template is applied. Otherwise the visual may not the same as we expect
    m_appliedTemplate = false;
 
    __super::OnApplyTemplate();

    // Find selection indicator
    // Retrieve pointers to stable controls 
    winrt::IControlProtected controlProtected = *this;
    m_helper.Init(controlProtected);
    m_navigationViewItemPresenter.set(GetTemplateChildT<winrt::NavigationViewItemPresenter>(c_navigationViewItemPresenterName, controlProtected));

    m_toolTip.set(GetTemplateChildT<winrt::ToolTip>(L"ToolTip"sv, controlProtected));

    if (auto splitView = GetSplitView())
    {
        m_splitViewIsPaneOpenChangedRevoker = RegisterPropertyChanged(splitView,
            winrt::SplitView::IsPaneOpenProperty(), { this, &NavigationViewItem::OnSplitViewPropertyChanged });
        m_splitViewDisplayModeChangedRevoker = RegisterPropertyChanged(splitView,
            winrt::SplitView::DisplayModeProperty(), { this, &NavigationViewItem::OnSplitViewPropertyChanged });
        m_splitViewCompactPaneLengthChangedRevoker = RegisterPropertyChanged(splitView,
            winrt::SplitView::CompactPaneLengthProperty(), { this, &NavigationViewItem::OnSplitViewPropertyChanged });

        UpdateCompactPaneLength();
        UpdateIsClosedCompact();
    }

    winrt::get_self<NavigationViewItemPresenter>(m_navigationViewItemPresenter.get())->SetDepth(GetDepth());

    RegisterPropertyChangedCallback(winrt::SelectorItem::IsSelectedProperty(), { this, &NavigationViewItem::OnIsSelectedChanged });

    m_appliedTemplate = true;
    UpdateVisualStateNoTransition();

    auto visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    NavigationView::CreateAndAttachHeaderAnimation(visual);
}

void NavigationViewItem::UpdateItemDepth(int depth)
{
        SetDepth(depth);
        winrt::get_self<NavigationViewItemPresenter>(m_navigationViewItemPresenter.get())->SetDepth(depth);
}

winrt::UIElement NavigationViewItem::GetSelectionIndicator()
{
    auto selectIndicator = m_helper.GetSelectionIndicator();
    if (auto presenter = GetPresenter())
    {
        selectIndicator = presenter->GetSelectionIndicator();
    }
    return selectIndicator;
}

void NavigationViewItem::OnSplitViewPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    if (args == winrt::SplitView::CompactPaneLengthProperty())
    {
        UpdateCompactPaneLength();
    }
    else if (args == winrt::SplitView::IsPaneOpenProperty() ||
        args == winrt::SplitView::DisplayModeProperty())
    {
        UpdateIsClosedCompact();
    }
}

void NavigationViewItem::UpdateCompactPaneLength()
{
    if (auto splitView = GetSplitView())
    {
        SetValue(s_CompactPaneLengthProperty, winrt::PropertyValue::CreateDouble(splitView.CompactPaneLength()));
    }
}

void NavigationViewItem::UpdateIsClosedCompact()
{
    if (auto splitView = GetSplitView())
    {
        // Check if the pane is closed and if the splitview is in either compact mode.
        m_isClosedCompact = !splitView.IsPaneOpen() && (splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactOverlay || splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactInline);
        UpdateVisualState(true /*useTransitions*/);
    }
}

void NavigationViewItem::UpdateNavigationViewItemToolTip()
{
    auto toolTipContent = winrt::ToolTipService::GetToolTip(*this);
    
    // no custom tooltip, then use suggested tooltip
    if (!toolTipContent || toolTipContent == m_suggestedToolTipContent.get())
    {
        if (ShouldEnableToolTip())
        {
            winrt::ToolTipService::SetToolTip(*this, m_suggestedToolTipContent.get());
        }
        else
        {
            winrt::ToolTipService::SetToolTip(*this, nullptr);
        }
    }
}

void NavigationViewItem::SuggestedToolTipChanged(winrt::IInspectable const& newContent)
{
    auto potentialString = safe_try_cast<winrt::IPropertyValue>(newContent);
    bool stringableToolTip = (potentialString && potentialString.Type() == winrt::PropertyType::String);
    
    winrt::IInspectable newToolTipContent{ nullptr };
    if (stringableToolTip)
    {
        newToolTipContent = newContent;
    }

    // Both customer and NavigationViewItem can update ToolTipContent by winrt::ToolTipService::SetToolTip or XAML
    // If the ToolTipContent is not the same as m_suggestedToolTipContent, then it's set by customer.
    // Customer's ToolTip take high priority, and we never override Customer's ToolTip.
    auto toolTipContent = winrt::ToolTipService::GetToolTip(*this);
    if (auto oldToolTipContent = m_suggestedToolTipContent.get())
    {
        if (oldToolTipContent == toolTipContent)
        {
            winrt::ToolTipService::SetToolTip(*this, nullptr);
        }
    }

    m_suggestedToolTipContent.set(newToolTipContent);
}

void NavigationViewItem::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto property = args.Property();
    if (property == s_IconProperty)
    {
        UpdateVisualStateNoTransition();
    }

    if (auto node = TreeNode())
    {
        if (property == s_IsExpandedProperty)
        {
            UpdateIsExpanded(node);            
            UpdateSelectionIndicatorVisiblity();
        }
        else if (property == s_MenuItemsSourceProperty)
        {
            winrt::IInspectable value = args.NewValue();
            winrt::get_self<TreeViewNode>(node)->ItemsSource(value);
            //m_dispatcherHelper.RunAsync(
            //    [node, value]()
            //{
            //    winrt::get_self<TreeViewNode>(node)->ItemsSource(value);
            //});
        }
        else if (property == s_IsChildSelectedProperty)
        {
            // Update the corresponding node
            if (auto navViewList = GetNavigationViewList())
            {
                auto viewModel = winrt::get_self<NavigationViewList>(navViewList)->ListViewModel();
                TreeNodeSelectionState selectionState = IsChildSelected() ? TreeNodeSelectionState::PartialSelected : TreeNodeSelectionState::UnSelected;
                viewModel->UpdateSelection(node, selectionState);
            }
            UpdateSelectionIndicatorVisiblity();
        }
    }
}

winrt::TreeViewNode NavigationViewItem::TreeNode()
{
    if (auto navViewList = GetNavigationViewList())
    {
        return  winrt::get_self<NavigationViewList>(navViewList)->NodeFromContainer(*this);
    }

    return nullptr;
}

void NavigationViewItem::UpdateVisualStateForIconAndContent(bool showIcon, bool showContent)
{
    auto stateName = showIcon ? (showContent ? L"IconOnLeft": L"IconOnly") : L"ContentOnly"; 
    winrt::VisualStateManager::GoToState(*this, stateName, false /*useTransitions*/);
}

void NavigationViewItem::UpdateVisualStateForNavigationViewListPositionChange()
{
    auto position = Position();
    auto stateName = c_OnLeftNavigation;

    bool handled = false;
    if (position == NavigationViewListPosition::LeftNav)
    {
        if (SharedHelpers::IsRS4OrHigher() && winrt::Application::Current().FocusVisualKind() == winrt::FocusVisualKind::Reveal)
        {
            // OnLeftNavigationReveal is introduced in RS6. 
            // Will fallback to stateName for the customer who re-template rs5 NavigationViewItem
            if (winrt::VisualStateManager::GoToState(*this, c_OnLeftNavigationReveal, false /*useTransitions*/))
            {
                handled = true;
            }
        }
    }
    else if (position == NavigationViewListPosition::TopPrimary)
    {
        if (SharedHelpers::IsRS4OrHigher() && winrt::Application::Current().FocusVisualKind() == winrt::FocusVisualKind::Reveal)
        {
            stateName = c_OnTopNavigationPrimaryReveal;
        }
        else
        {
            stateName = c_OnTopNavigationPrimary;
        }
    }
    else if (position == NavigationViewListPosition::TopOverflow)
    {
        stateName = c_OnTopNavigationOverflow;
    }

    if (!handled)
    {
        winrt::VisualStateManager::GoToState(*this, stateName, false /*useTransitions*/);
    }
}

void NavigationViewItem::UpdateVisualStateForKeyboardFocusedState()
{
    auto focusState = L"KeyboardNormal";
    if (m_hasKeyboardFocus)
    {
        focusState = L"KeyboardFocused";
    }

    winrt::VisualStateManager::GoToState(*this, focusState, false /*useTransitions*/);
}

void NavigationViewItem::UpdateVisualStateForToolTip()
{
    // Since RS5, ToolTip apply to NavigationViewItem directly to make Keyboard focus has tooltip too.
    // If ToolTip TemplatePart is detected, fallback to old logic and apply ToolTip on TemplatePart.
    if (auto toolTip = m_toolTip.get())
    {
        auto shouldEnableToolTip = ShouldEnableToolTip();
        auto toolTipContent = m_suggestedToolTipContent.get();
        if (shouldEnableToolTip && toolTipContent)
        {
            toolTip.Content(toolTipContent);
            toolTip.IsEnabled(true);
        }
        else
        {
            toolTip.Content(nullptr);
            toolTip.IsEnabled(false);
        }
    }
    else
    {
        UpdateNavigationViewItemToolTip();
    }
}

void NavigationViewItem::UpdateVisualState(bool useTransitions)
{
    if (!m_appliedTemplate)
        return;

    UpdateVisualStateForNavigationViewListPositionChange();

    bool shouldShowIcon = ShouldShowIcon();
    bool shouldShowContent = ShouldShowContent();
  
    if (IsOnLeftNav())
    {
        winrt::VisualStateManager::GoToState(*this, m_isClosedCompact ? L"ClosedCompact" : L"NotClosedCompact", useTransitions); 

        // Backward Compatibility with RS4-, new implementation prefer IconOnLeft/IconOnly/ContentOnly
        winrt::VisualStateManager::GoToState(*this, shouldShowIcon ? L"IconVisible" : L"IconCollapsed", useTransitions);
    } 
   
    UpdateVisualStateForToolTip();

    UpdateVisualStateForIconAndContent(shouldShowIcon, shouldShowContent);

    // visual state for focus state. top navigation use it to provide different visual for selected and selected+focused
    UpdateVisualStateForKeyboardFocusedState();
}

bool NavigationViewItem::ShouldShowIcon()
{
    return static_cast<bool>(Icon());
}

bool NavigationViewItem::ShouldEnableToolTip()
{
    // We may enable Tooltip for IconOnly in the future, but not now
    return IsOnLeftNav() && m_isClosedCompact;
}

bool NavigationViewItem::ShouldShowContent()
{
    return static_cast<bool>(Content());
}

bool NavigationViewItem::IsOnLeftNav()
{
    return Position() == NavigationViewListPosition::LeftNav;
}

bool NavigationViewItem::IsOnTopPrimary()
{
    return Position() == NavigationViewListPosition::TopPrimary;
}

NavigationViewItemPresenter * NavigationViewItem::GetPresenter()
{
    NavigationViewItemPresenter * presenter = nullptr;
    if (m_navigationViewItemPresenter)
    {
        presenter = winrt::get_self<NavigationViewItemPresenter>(m_navigationViewItemPresenter.get());
    }
    return presenter;
}

// IUIElement / IUIElementOverridesHelper
winrt::AutomationPeer NavigationViewItem::OnCreateAutomationPeer()
{
    return winrt::make<NavigationViewItemAutomationPeer>(*this);
}

// IContentControlOverrides / IContentControlOverridesHelper
void NavigationViewItem::OnContentChanged(winrt::IInspectable const& oldContent, winrt::IInspectable const& newContent)
{
    __super::OnContentChanged(oldContent, newContent);
    SuggestedToolTipChanged(newContent);
    UpdateVisualStateNoTransition();

    // Two ways are used to notify the content change on TopNav and asking for a layout update:
    //  1. The NavigationViewItem can't find its parent NavigationView, just mark it. Possibly NavigationViewItem is moved to overflow but menu is not opened.
    //  2. NavigationViewItem request update by NavigationView::TopNavigationViewItemContentChanged.
    if (!IsOnLeftNav())
    {
        if (auto navView = GetNavigationView())
        {
            winrt::get_self<NavigationView>(navView)->TopNavigationViewItemContentChanged();
        } 
        else
        {
            m_isContentChangeHandlingDelayedForTopNav = true;
        }
    }
}

void NavigationViewItem::OnGotFocus(winrt::RoutedEventArgs const& e)
{
    __super::OnGotFocus(e);
    auto originalSource = e.OriginalSource().try_as<winrt::Control>();
    if (originalSource)
    {
        // It's used to support bluebar have difference appearance between focused and focused+selection. 
        // For example, we can move the SelectionIndicator 3px up when focused and selected to make sure focus rectange doesn't override SelectionIndicator. 
        // If it's a pointer or programatic, no focus rectangle, so no action
        auto focusState = originalSource.FocusState();
        if (focusState == winrt::FocusState::Keyboard)
        {
            m_hasKeyboardFocus = true;
            UpdateVisualStateNoTransition();
        }
    }
}

void NavigationViewItem::OnLostFocus(winrt::RoutedEventArgs const& e)
{
    __super::OnLostFocus(e);
    if (m_hasKeyboardFocus)
    {
        m_hasKeyboardFocus = false;
        UpdateVisualStateNoTransition();
    }
}

void NavigationViewItem::UpdateIsExpanded(winrt::TreeViewNode node)
{
    if (node.IsExpanded() != IsExpanded())
    {
        node.IsExpanded(IsExpanded());
    }
}

void NavigationViewItem::UpdateSelectionIndicatorVisiblity()
{
    auto selectionIndicator = GetSelectionIndicator();
    if (IsChildSelected() && !IsExpanded())
    {
        selectionIndicator.Opacity(1);
    }
    else
    {
        selectionIndicator.Opacity(0);
    }
}

void NavigationViewItem::OnIsSelectedChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    auto selectionIndicator = GetSelectionIndicator();
    bool isSelected = unbox_value<bool>(GetValue(args));
    if (isSelected)
    {
        selectionIndicator.Opacity(1);
    }
    else
    {
        selectionIndicator.Opacity(0);
    }
}
