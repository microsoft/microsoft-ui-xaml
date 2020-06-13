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
static constexpr auto c_flyoutContentGrid = L"FlyoutContentGrid"sv;

// Visual States
static constexpr auto c_pressedSelected = L"PressedSelected"sv;
static constexpr auto c_pointerOverSelected = L"PointerOverSelected"sv;
static constexpr auto c_selected = L"Selected"sv;
static constexpr auto c_pressed = L"Pressed"sv;
static constexpr auto c_pointerOver = L"PointerOver"sv;
static constexpr auto c_disabled = L"Disabled"sv;
static constexpr auto c_enabled = L"Enabled"sv;
static constexpr auto c_normal = L"Normal"sv;
static constexpr auto c_chevronHidden = L"ChevronHidden"sv;
static constexpr auto c_chevronVisibleOpen = L"ChevronVisibleOpen"sv;
static constexpr auto c_chevronVisibleClosed = L"ChevronVisibleClosed"sv;

void NavigationViewItem::UpdateVisualStateNoTransition()
{
    UpdateVisualState(false /*useTransition*/);
}

void NavigationViewItem::OnNavigationViewRepeaterPositionChanged()
{
    UpdateVisualStateNoTransition();
    ReparentRepeater();
}

void NavigationViewItem::OnNavigationViewItemBaseDepthChanged()
{
    UpdateItemIndentation();
    PropagateDepthToChildren(Depth() + 1);
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

    winrt::UIElement const presenter = [this, controlProtected]()
    {
        if (auto presenter = GetTemplateChildT<winrt::NavigationViewItemPresenter>(c_navigationViewItemPresenterName, controlProtected))
        {
            m_navigationViewItemPresenter.set(presenter);
            return presenter.try_as<winrt::UIElement>();
        }
        // We don't have a presenter, so we are our own presenter.
        return this->try_as<winrt::UIElement>();
    }();

    m_presenterPointerPressedRevoker = presenter.PointerPressed(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerPressed });
    m_presenterPointerReleasedRevoker = presenter.PointerReleased(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerReleased });
    m_presenterPointerEnteredRevoker = presenter.PointerEntered(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerEntered });
    m_presenterPointerExitedRevoker = presenter.PointerExited(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerExited });
    m_presenterPointerCanceledRevoker = presenter.PointerCanceled(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerCanceled });
    m_presenterPointerCaptureLostRevoker = presenter.PointerCaptureLost(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerCaptureLost });

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
            m_repeaterElementPreparedRevoker = repeater.ElementPrepared(winrt::auto_revoke, { nvImpl,  &NavigationView::OnRepeaterElementPrepared });
            m_repeaterElementClearingRevoker = repeater.ElementClearing(winrt::auto_revoke, { nvImpl, &NavigationView::OnRepeaterElementClearing });

            repeater.ItemTemplate(*(nvImpl->GetNavigationViewItemsFactory()));
        }

        UpdateRepeaterItemsSource();
    }

    m_flyoutContentGrid.set(GetTemplateChildT<winrt::Grid>(c_flyoutContentGrid, controlProtected));

    m_appliedTemplate = true;
    UpdateItemIndentation();
    UpdateVisualStateNoTransition();
    ReparentRepeater();
    // We dont want to update the repeater visibilty during OnApplyTemplate if NavigationView is in a mode when items are shown in a flyout
    if (!ShouldRepeaterShowInFlyout())
    {
        ShowHideChildren();
    }

    auto visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    NavigationView::CreateAndAttachHeaderAnimation(visual);
}

void NavigationViewItem::UpdateRepeaterItemsSource()
{
    if (auto repeater = m_repeater.get())
    {
        auto const itemsSource = [this]()
        {
            if (auto const menuItemsSource = MenuItemsSource())
            {
                return menuItemsSource;
            }
            return MenuItems().as<winrt::IInspectable>();
        }();
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
        ReparentRepeater();
    }
}

void NavigationViewItem::UpdateCompactPaneLength()
{
    if (const auto splitView = GetSplitView())
    {
        SetValue(s_CompactPaneLengthProperty, winrt::PropertyValue::CreateDouble(splitView.CompactPaneLength()));

        // Only update when on left
        if (const auto presenter = GetPresenter())
        {
            presenter->UpdateCompactPaneLength(splitView.CompactPaneLength(), IsOnLeftNav());
        }
    }
}

void NavigationViewItem::UpdateIsClosedCompact()
{
    if (const auto splitView = GetSplitView())
    {
        // Check if the pane is closed and if the splitview is in either compact mode.
        m_isClosedCompact = !splitView.IsPaneOpen()
            && (splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactOverlay || splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactInline);

        UpdateVisualState(true /*useTransitions*/);

        if (const auto presenter = GetPresenter())
        {
            presenter->UpdateClosedCompactVisualState(IsTopLevelItem(), m_isClosedCompact);
        }
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
    UpdateVisualStateForChevron();
}

void NavigationViewItem::OnMenuItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateRepeaterItemsSource();
    UpdateVisualStateForChevron();
}

void NavigationViewItem::OnHasUnrealizedChildrenPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateVisualStateForChevron();
}

bool NavigationViewItem::ShowSelectionIndicatorIfRequired()
{
    if (!IsSelected())
    {
        if (!IsRepeaterVisible() && IsChildSelected())
        {
            ShowSelectionIndicator(true);
            return true;
        }
        else
        {
            ShowSelectionIndicator(false);
        }
    }
    return false;
}

void NavigationViewItem::ShowSelectionIndicator(bool visible)
{
    if (auto const selectionIndicator = GetSelectionIndicator())
    {
        selectionIndicator.Opacity(visible ? 1.0 : 0.0);
    }
}

void NavigationViewItem::UpdateVisualStateForIconAndContent(bool showIcon, bool showContent)
{
    if (auto const presenter = m_navigationViewItemPresenter.get())
    {
        auto stateName = showIcon ? (showContent ? L"IconOnLeft" : L"IconOnly") : L"ContentOnly";
        winrt::VisualStateManager::GoToState(presenter, stateName, false /*useTransitions*/);
    }
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

void NavigationViewItem::UpdateVisualStateForPointer()
{
    // DisabledStates and CommonStates
    auto enabledStateValue = c_enabled;
    bool isSelected = IsSelected();
    auto selectedStateValue = c_normal;
    if (IsEnabled())
    {
        if (isSelected)
        {
            if (m_isPressed)
            {
                selectedStateValue = c_pressedSelected;
            }
            else if (m_isPointerOver)
            {
                selectedStateValue = c_pointerOverSelected;
            }
            else
            {
                selectedStateValue = c_selected;
            }
        }
        else if (m_isPointerOver)
        {
            if (m_isPressed)
            {
                selectedStateValue = c_pressed;
            }
            else
            {
                selectedStateValue = c_pointerOver;
            }
        }
        else if (m_isPressed)
        {
            selectedStateValue = c_pressed;
        }
    }
    else
    {
        enabledStateValue = c_disabled;
        if (isSelected)
        {
            selectedStateValue = c_selected;
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

}

void NavigationViewItem::UpdateVisualState(bool useTransitions)
{
    if (!m_appliedTemplate)
        return;

    UpdateVisualStateForPointer();

    UpdateVisualStateForNavigationViewPositionChange();

    bool shouldShowIcon = ShouldShowIcon();
    bool shouldShowContent = ShouldShowContent();
  
    if (IsOnLeftNav())
    {
        if (auto const presenter = m_navigationViewItemPresenter.get())
        {
            // Backward Compatibility with RS4-, new implementation prefer IconOnLeft/IconOnly/ContentOnly
            winrt::VisualStateManager::GoToState(m_navigationViewItemPresenter.get(), shouldShowIcon ? L"IconVisible" : L"IconCollapsed", useTransitions);
        }
    } 
   
    UpdateVisualStateForToolTip();

    UpdateVisualStateForIconAndContent(shouldShowIcon, shouldShowContent);

    // visual state for focus state. top navigation use it to provide different visual for selected and selected+focused
    UpdateVisualStateForKeyboardFocusedState();

    UpdateVisualStateForChevron();
}

void NavigationViewItem::UpdateVisualStateForChevron()
{
    if (auto const presenter = m_navigationViewItemPresenter.get())
    {
        auto const chevronState = HasChildren() && !(m_isClosedCompact && ShouldRepeaterShowInFlyout()) ? ( IsExpanded() ? c_chevronVisibleOpen : c_chevronVisibleClosed) : c_chevronHidden;
        winrt::VisualStateManager::GoToState(m_navigationViewItemPresenter.get(), chevronState, true);
    }
}

bool NavigationViewItem::HasChildren()
{
    return MenuItems().Size() > 0 || MenuItemsSource() != nullptr || HasUnrealizedChildren();
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

void NavigationViewItem::ShowHideChildren()
{
    if (auto const repeater = m_repeater.get())
    {
        bool shouldShowChildren = IsExpanded();
        auto visibility = shouldShowChildren ? winrt::Visibility::Visible : winrt::Visibility::Collapsed;
        repeater.Visibility(visibility);

        if (ShouldRepeaterShowInFlyout())
        {
            if (shouldShowChildren)
            {
                // Verify that repeater is parented correctly
                if (!m_isRepeaterParentedToFlyout)
                {
                    ReparentRepeater();
                }

                // There seems to be a race condition happening which sometimes
                // prevents the opening of the flyout. Queue callback as a workaround.
                SharedHelpers::QueueCallbackForCompositionRendering(
                    [strongThis = get_strong()]()
                {
                    winrt::FlyoutBase::ShowAttachedFlyout(strongThis->m_rootGrid.get());
                });
            }
            else
            {
                if (auto const flyout = winrt::FlyoutBase::GetAttachedFlyout(m_rootGrid.get()))
                {
                    flyout.Hide();
                }
            }
        }
    }
}

void NavigationViewItem::ReparentRepeater()
{
    if (HasChildren())
    {
        if (auto const repeater = m_repeater.get())
        {
            if (ShouldRepeaterShowInFlyout() && !m_isRepeaterParentedToFlyout)
            {
                // Reparent repeater to flyout
                // TODO: Replace removeatend with something more specific
                m_rootGrid.get().Children().RemoveAtEnd();
                m_flyoutContentGrid.get().Children().Append(repeater);
                m_isRepeaterParentedToFlyout = true;

                PropagateDepthToChildren(0);
            }
            else if (!ShouldRepeaterShowInFlyout() && m_isRepeaterParentedToFlyout)
            {
                m_flyoutContentGrid.get().Children().RemoveAtEnd();
                m_rootGrid.get().Children().Append(repeater);
                m_isRepeaterParentedToFlyout = false;

                PropagateDepthToChildren(1);
            }
        }
    }
}

bool NavigationViewItem::ShouldRepeaterShowInFlyout()
{
    return (m_isClosedCompact && IsTopLevelItem()) || IsOnTopPrimary();
}

bool NavigationViewItem::IsRepeaterVisible()
{
    return m_repeater.get().Visibility() == winrt::Visibility::Visible;
}

void NavigationViewItem::UpdateItemIndentation()
{
    // Update item indentation based on its depth
    if (auto const presenter = m_navigationViewItemPresenter.get())
    {
        auto newLeftMargin = Depth() * c_itemIndentation;
        winrt::get_self<NavigationViewItemPresenter>(presenter)->UpdateContentLeftIndentation(static_cast<double>(newLeftMargin));
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

void NavigationViewItem::OnExpandCollapseChevronTapped(const winrt::IInspectable& sender, const winrt::TappedRoutedEventArgs& args)
{
    IsExpanded(!IsExpanded());
    args.Handled(true);
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

void NavigationViewItem::RotateExpandCollapseChevron(bool isExpanded)
{
    winrt::get_self<NavigationViewItemPresenter>(m_navigationViewItemPresenter.get())->RotateExpandCollapseChevron(isExpanded);
}
