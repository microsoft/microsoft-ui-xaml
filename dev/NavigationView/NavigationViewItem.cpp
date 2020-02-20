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
static constexpr auto c_rootGrid = L"NVIRootGrid"sv;
static constexpr auto c_flyoutRootGrid = L"FlyoutRootGrid"sv;
static constexpr int c_itemIndentation = 25;

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
    SetValue(s_MenuItemsProperty, winrt::make<Vector<winrt::IInspectable>>());
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

    if (auto rootGrid = GetTemplateChildT<winrt::Grid>(c_rootGrid, controlProtected))
    {
        m_rootGrid.set(rootGrid);

        if (auto flyoutBase = winrt::FlyoutBase::GetAttachedFlyout(rootGrid))
        {
            m_flyoutClosingRevoker = flyoutBase.Closing(winrt::auto_revoke, { this, &NavigationViewItem::OnFlyoutClosing });
        }

    }

    if (auto presenter = GetTemplateChildT<winrt::NavigationViewItemPresenter>(c_navigationViewItemPresenterName, controlProtected))
    {
        m_navigationViewItemPresenter.set(presenter);

        m_presenterPointerPressedRevoker = presenter.PointerPressed(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerPressed });
        m_presenterPointerReleasedRevoker = presenter.PointerReleased(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerReleased });
        m_presenterPointerEnteredRevoker = presenter.PointerEntered(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerEntered });
        m_presenterPointerExitedRevoker = presenter.PointerExited(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerExited });
        m_presenterPointerCanceledRevoker = presenter.PointerCanceled(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerCanceled });
        m_presenterPointerCaptureLostRevoker = presenter.PointerCaptureLost(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerCaptureLost });
    }

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

            // Primary element setup happens in NavigationView
            m_repeaterElementPreparedRevoker = repeater.ElementPrepared(winrt::auto_revoke, { nvImpl,  &NavigationView::RepeaterElementPrepared });
            m_repeaterElementClearingRevoker = repeater.ElementClearing(winrt::auto_revoke, { nvImpl, &NavigationView::RepeaterElementClearing });

            repeater.ItemTemplate(*(nvImpl->m_navigationViewItemsFactory));
        }

        UpdateRepeaterItemsSource();
    }

    if (auto flyoutRootGrid = GetTemplateChildT<winrt::Grid>(c_flyoutRootGrid, controlProtected))
    {
        m_flyoutRootGrid.set(flyoutRootGrid);
    }

    UpdateItemIndentation();

    m_appliedTemplate = true;
    UpdateVisualStateNoTransition();

    auto visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    NavigationView::CreateAndAttachHeaderAnimation(visual);
}

void NavigationViewItem::Depth(int depth)
{
    NavigationViewItemBase::Depth(depth);
    UpdateItemIndentation();
    PropagateDepthToChildren(depth + 1);
}

int NavigationViewItem::Depth()
{
    return NavigationViewItemBase::Depth();
}

void NavigationViewItem::UpdateRepeaterItemsSource()
{
    auto const itemsSource = [this]()
    {
        if (auto const menuItemsSource = MenuItemsSource())
        {
            return menuItemsSource;
        }
        return MenuItems().as<winrt::IInspectable>();
    }();

    if (auto repeater = m_repeater.get())
    {
        repeater.ItemsSource(itemsSource);
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

void NavigationViewItem::OnMenuItemsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateRepeaterItemsSource();
}

void NavigationViewItem::OnMenuItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateRepeaterItemsSource();
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

    // DisabledStates and CommonStates
    auto enabledStateValue = L"Enabled";
    bool isSelected = IsSelected();
    auto selectedStateValue = L"Normal";
    if (IsEnabled())
    {
        if (isSelected)
        {
            if (m_isPressed)
            {
                selectedStateValue = L"PressedSelected";
            }
            else if (m_isPointerOver)
            {
                selectedStateValue = L"PointerOverSelected";
            }
            else
            {
                selectedStateValue = L"Selected";
            }
        }
        else if (m_isPointerOver)
        {
            if (m_isPressed)
            {
                selectedStateValue = L"Pressed";
            }
            else
            {
                selectedStateValue = L"PointerOver";
            }
        }
        else if (m_isPressed)
        {
            selectedStateValue = L"Pressed";
        }
    }
    else
    {
        enabledStateValue = L"Disabled";
        if (isSelected)
        {
            selectedStateValue = L"Selected";
        }
    }

    // There are scenarios where the presenter may not exist.
    // For example, the top nav settings item. In that case,
    // update the states for the item itself.
    if (auto const presenter = m_navigationViewItemPresenter.get())
    {
        winrt::VisualStateManager::GoToState(m_navigationViewItemPresenter.get(), enabledStateValue, true);
        winrt::VisualStateManager::GoToState(m_navigationViewItemPresenter.get(), selectedStateValue, true);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, enabledStateValue, true);
        winrt::VisualStateManager::GoToState(*this, selectedStateValue, true);
    }

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

void NavigationViewItem::IsRepeaterVisible(bool visible)
{
    ReparentRepeater();

    auto visibility = visible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed;
    m_repeater.get().Visibility(visibility);

    if (ShouldRepeaterShowInFlyout() && visible)
    {
        winrt::FlyoutBase::ShowAttachedFlyout(m_rootGrid.get());
    }
}

void NavigationViewItem::ReparentRepeater()
{
    if (auto const repeater = m_repeater.get())
    {
        if (ShouldRepeaterShowInFlyout() && !m_parentedToFlyout)
        {
            m_rootGrid.get().Children().RemoveAtEnd();
            m_flyoutRootGrid.get().Children().Append(repeater);
            m_parentedToFlyout = true;

            PropagateDepthToChildren(0);
        }
        else if (!ShouldRepeaterShowInFlyout() && m_parentedToFlyout)
        {
            m_flyoutRootGrid.get().Children().RemoveAtEnd();
            m_rootGrid.get().Children().Append(repeater);
            m_parentedToFlyout = false;

            PropagateDepthToChildren(1);
        }
    }
}

// We only want to show flyouts if the item is at the top level of the
// item tree and navigationview is in compact mode.
bool NavigationViewItem::ShouldRepeaterShowInFlyout()
{
    UpdateIsClosedCompact();
    return (m_isClosedCompact && IsTopLevelItem()) || IsOnTopPrimary();
}

void NavigationViewItem::UpdateItemIndentation()
{
    // Update item indentation based on its depth
    if (auto const presenter = m_navigationViewItemPresenter.get())
    {
        auto newLeftMargin = Depth() * c_itemIndentation;
        winrt::get_self<NavigationViewItemPresenter>(presenter)->UpdateLeftIndentation(static_cast<double>(newLeftMargin));
    }
}

void NavigationViewItem::PropagateDepthToChildren(int depth)
{
    if (auto const repeater = m_repeater.get())
    {
        auto itemsCount = repeater.ItemsSourceView().Count();
        for (int index = 0; index < itemsCount; index++)
        {
            if (auto const element = repeater.TryGetElement(index))
            {
                if (auto const nvib = element.try_as<winrt::NavigationViewItemBase>())
                {
                    winrt::get_self<NavigationViewItemBase>(nvib)->Depth(depth);
                }
            }
        }
    }
}

void NavigationViewItem::OnFlyoutClosing(const winrt::IInspectable& sender, const winrt::FlyoutBaseClosingEventArgs& args)
{
    IsExpanded(false);
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

void NavigationViewItem::OnIsSelectedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    //TODO: Verify that handling here is not requied for visual states
}

void NavigationViewItem::OnPresenterPointerPressed(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    // TODO: Update to look at presenter instead
    auto pointerProperties = args.GetCurrentPoint(*this).Properties();
    m_isPressed = pointerProperties.IsLeftButtonPressed() || pointerProperties.IsRightButtonPressed();

    UpdateVisualState(true);
}

void NavigationViewItem::OnPresenterPointerReleased(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    m_isPressed = false;
    UpdateVisualState(true);
}

void NavigationViewItem::OnPresenterPointerEntered(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    m_isPointerOver = true;
    UpdateVisualState(true);
}

void NavigationViewItem::OnPresenterPointerExited(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    m_isPointerOver = false;
    UpdateVisualState(true);
}

void NavigationViewItem::OnPresenterPointerCanceled(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    m_isPressed = false;
    m_isPointerOver = false;
    UpdateVisualState(true);
}

void NavigationViewItem::OnPresenterPointerCaptureLost(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    m_isPressed = false;
    m_isPointerOver = false;
    UpdateVisualState(true);
}
