// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>

#include "NavigationView.h"
#include "NavigationViewItem.h"
#include "NavigationViewItemAutomationPeer.h"
#include "Utils.h"

static constexpr wstring_view c_navigationViewItemPresenterName = L"NavigationViewItemPresenter"sv;
static constexpr auto c_repeater = L"NavigationViewItemMenuItemsHost"sv;

void NavigationViewItem::UpdateVisualStateNoTransition()
{
    UpdateVisualState(false /*useTransition*/);
}

void NavigationViewItem::OnNavigationViewRepeaterPositionChanged()
{
    UpdateVisualStateNoTransition();
}

NavigationViewItem::NavigationViewItem()
{
    SetDefaultStyleKey(this);

    auto items = winrt::make<Vector<winrt::IInspectable>>();
    SetValue(s_MenuItemsProperty, items);
}

void NavigationViewItem::OnApplyTemplate()
{
    // Stop UpdateVisualState before template is applied. Otherwise the visual may not the same as we expect
    m_appliedTemplate = false;
 
    NavigationViewItemBase::OnApplyTemplate();

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

    // Retrieve reference to NavigationView
    if (auto nvImpl = winrt::get_self<NavigationView>(GetNavigationView()))
    {
        if (auto repeater = GetTemplateChildT<winrt::ItemsRepeater>(c_repeater, controlProtected))
        {
            m_repeater.set(repeater);

            m_repeaterElementPreparedRevoker = repeater.ElementPrepared(winrt::auto_revoke, { nvImpl,  &NavigationView::RepeaterElementPrepared });
            m_repeaterElementClearingRevoker = repeater.ElementClearing(winrt::auto_revoke, { nvImpl, &NavigationView::RepeaterElementClearing });

            repeater.ItemTemplate(*(nvImpl->m_navigationViewItemsFactory));
        }

        UpdateRepeaterItemsSource();
    }

    m_appliedTemplate = true;
    UpdateVisualStateNoTransition();

    auto visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    NavigationView::CreateAndAttachHeaderAnimation(visual);
}

void NavigationViewItem::UpdateRepeaterItemsSource()
{
    if (auto menuItems = MenuItems())
    {
        if (auto repeater = m_repeater.get())
        {
            repeater.ItemsSource(menuItems);
        }
    }
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
    auto potentialString = newContent.try_as<winrt::IPropertyValue>();
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

void NavigationViewItem::OnIconPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateVisualStateNoTransition();
}

void NavigationViewItem::OnHasUnrealizedChildrenPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
}

void NavigationViewItem::OnIsChildSelectedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
}

void NavigationViewItem::OnIsExpandedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
}

void NavigationViewItem::OnMenuItemsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
}

void NavigationViewItem::OnMenuItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
}

void NavigationViewItem::UpdateVisualStateForIconAndContent(bool showIcon, bool showContent)
{
    auto stateName = showIcon ? (showContent ? L"IconOnLeft": L"IconOnly") : L"ContentOnly"; 
    winrt::VisualStateManager::GoToState(*this, stateName, false /*useTransitions*/);
}

void NavigationViewItem::UpdateVisualStateForNavigationViewPositionChange()
{
    auto position = Position();
    auto stateName = c_OnLeftNavigation;

    bool handled = false;

    switch (position)
    {
    case NavigationViewRepeaterPosition::LeftNav:
        if (SharedHelpers::IsRS4OrHigher() && winrt::Application::Current().FocusVisualKind() == winrt::FocusVisualKind::Reveal)
        {
            // OnLeftNavigationReveal is introduced in RS6. 
            // Will fallback to stateName for the customer who re-template rs5 NavigationViewItem
            if (winrt::VisualStateManager::GoToState(*this, c_OnLeftNavigationReveal, false /*useTransitions*/))
            {
                handled = true;
            }
        }
        break;
    case NavigationViewRepeaterPosition::TopPrimary:
        if (SharedHelpers::IsRS4OrHigher() && winrt::Application::Current().FocusVisualKind() == winrt::FocusVisualKind::Reveal)
        {
            stateName = c_OnTopNavigationPrimaryReveal;
        }
        else
        {
            stateName = c_OnTopNavigationPrimary;
        }
        break;
    case NavigationViewRepeaterPosition::TopOverflow:
        stateName = c_OnTopNavigationOverflow;
        break;
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

    UpdateVisualStateForNavigationViewPositionChange();

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
    return Position() == NavigationViewRepeaterPosition::LeftNav;
}

bool NavigationViewItem::IsOnTopPrimary()
{
    return Position() == NavigationViewRepeaterPosition::TopPrimary;
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
    NavigationViewItemBase::OnContentChanged(oldContent, newContent);
    SuggestedToolTipChanged(newContent);
    UpdateVisualStateNoTransition();

    if (!IsOnLeftNav())
    {
        // Content has changed for the item, so we want to trigger a re-measure
        if (auto navView = GetNavigationView())
        {
            winrt::get_self<NavigationView>(navView)->TopNavigationViewItemContentChanged();
        }
    }
}

void NavigationViewItem::OnGotFocus(winrt::RoutedEventArgs const& e)
{
    NavigationViewItemBase::OnGotFocus(e);
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
    NavigationViewItemBase::OnLostFocus(e);
    if (m_hasKeyboardFocus)
    {
        m_hasKeyboardFocus = false;
        UpdateVisualStateNoTransition();
    }
}
