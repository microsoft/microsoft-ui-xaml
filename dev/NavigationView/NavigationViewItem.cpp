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

static constexpr auto c_normalChevronHidden = L"NormalChevronHidden"sv;
static constexpr auto c_normalChevronVisibleOpen = L"NormalChevronVisibleOpen"sv;
static constexpr auto c_normalChevronVisibleClosed = L"NormalChevronVisibleClosed"sv;
static constexpr auto c_pointerOverChevronHidden = L"PointerOverChevronHidden"sv;
static constexpr auto c_pointerOverChevronVisibleOpen = L"PointerOverChevronVisibleOpen"sv;
static constexpr auto c_pointerOverChevronVisibleClosed = L"PointerOverChevronVisibleClosed"sv;
static constexpr auto c_pressedChevronHidden = L"PressedChevronHidden"sv;
static constexpr auto c_pressedChevronVisibleOpen = L"PressedChevronVisibleOpen"sv;
static constexpr auto c_pressedChevronVisibleClosed = L"PressedChevronVisibleClosed"sv;

NavigationViewItem::NavigationViewItem()
{
    SetDefaultStyleKey(this);
    SetValue(s_MenuItemsProperty, winrt::make<Vector<winrt::IInspectable>>());
}

void NavigationViewItem::UpdateVisualStateNoTransition()
{
    UpdateVisualState(false /*useTransition*/);
}

void NavigationViewItem::OnNavigationViewItemBaseDepthChanged()
{
    UpdateItemIndentation();
    PropagateDepthToChildren(Depth() + 1);
}

void NavigationViewItem::OnNavigationViewItemBaseIsSelectedChanged()
{
    UpdateVisualStateForPointer();
}

void NavigationViewItem::OnNavigationViewItemBasePositionChanged()
{
    UpdateVisualStateNoTransition();
    ReparentRepeater();
}

void NavigationViewItem::OnApplyTemplate()
{
    // Stop UpdateVisualState before template is applied. Otherwise the visuals may be unexpected
    m_appliedTemplate = false;

    UnhookEventsAndClearFields();

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

    HookInputEvents(controlProtected);

    m_isEnabledChangedRevoker = IsEnabledChanged(winrt::auto_revoke, { this,  &NavigationViewItem::OnIsEnabledChanged });

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
        m_itemsSourceViewCollectionChangedRevoker.revoke();
        repeater.ItemsSource(itemsSource);
        m_itemsSourceViewCollectionChangedRevoker = repeater.ItemsSourceView().CollectionChanged(winrt::auto_revoke, { this, &NavigationViewItem::OnItemsSourceViewChanged });
    }
}

void NavigationViewItem::OnItemsSourceViewChanged(const winrt::IInspectable& /*sender*/, const winrt::NotifyCollectionChangedEventArgs& /*args*/)
{
    UpdateVisualStateForChevron();
}
 
winrt::UIElement NavigationViewItem::GetSelectionIndicator() const
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
    const bool validStringableToolTip = potentialString
        && potentialString.Type() == winrt::PropertyType::String
        && !potentialString.GetString().empty();
    
    winrt::IInspectable newToolTipContent{ nullptr };
    if (validStringableToolTip)
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

void NavigationViewItem::OnIsExpandedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (winrt::AutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
    {
        auto navViewItemPeer = peer.as<winrt::NavigationViewItemAutomationPeer>();
        winrt::get_self<NavigationViewItemAutomationPeer>(navViewItemPeer)->RaiseExpandCollapseAutomationEvent(
            IsExpanded() ?
                winrt::ExpandCollapseState::Expanded :
                winrt::ExpandCollapseState::Collapsed
        );
    }
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
    const auto position = Position();
    auto stateName = c_OnLeftNavigation;

    bool handled = false;

    switch (position)
    {
    case NavigationViewRepeaterPosition::LeftNav:
    case NavigationViewRepeaterPosition::LeftFooter:
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
    case NavigationViewRepeaterPosition::TopFooter:
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
        const auto shouldEnableToolTip = ShouldEnableToolTip();
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
    const auto isEnabled = IsEnabled();
    const auto enabledStateValue = isEnabled ? c_enabled : c_disabled;
    // DisabledStates and CommonStates
    const auto selectedStateValue = [this, isEnabled, isSelected = IsSelected()]()
    {
        if (isEnabled)
        {
            if (isSelected)
            {
                if (m_isPressed)
                {
                    return c_pressedSelected;
                }
                else if (m_isPointerOver)
                {
                    return c_pointerOverSelected;
                }
                else
                {
                    return c_selected;
                }
            }
            else if (m_isPointerOver)
            {
                if (m_isPressed)
                {
                    return c_pressed;
                }
                else
                {
                    return c_pointerOver;
                }
            }
            else if (m_isPressed)
            {
                return c_pressed;
            }
        }
        else
        {
            if (isSelected)
            {
                return c_selected;
            }
        }
        return c_normal;
    }();

    // There are scenarios where the presenter may not exist.
    // For example, the top nav settings item. In that case,
    // update the states for the item itself.
    if (auto const presenter = m_navigationViewItemPresenter.get())
    {
        winrt::VisualStateManager::GoToState(presenter, enabledStateValue, true);
        winrt::VisualStateManager::GoToState(presenter, selectedStateValue, true);
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

    const bool shouldShowIcon = ShouldShowIcon();
    const bool shouldShowContent = ShouldShowContent();
  
    if (IsOnLeftNav())
    {
        if (auto const presenter = m_navigationViewItemPresenter.get())
        {
            // Backward Compatibility with RS4-, new implementation prefer IconOnLeft/IconOnly/ContentOnly
            winrt::VisualStateManager::GoToState(presenter, shouldShowIcon ? L"IconVisible" : L"IconCollapsed", useTransitions);
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
        enum class PointerStateValue{ Normal, PointerOver, Pressed };
        enum class ChevronStateValue { ChevronHidden, ChevronVisibleOpen, ChevronVisibleClosed };
        const auto pointerStateValue = [this, isEnabled = IsEnabled(), isSelected = IsSelected()]()
        {
            if (isEnabled)
            {
                if (m_isPointerOver)
                {
                    if (m_isPressed)
                    {
                        return PointerStateValue::Pressed; //Pressed
                    }
                    else
                    {
                        return PointerStateValue::PointerOver; //PointerOver
                    }
                }
                else if (m_isPressed)
                {
                    return PointerStateValue::Pressed; //Pressed
                }
            }
            return PointerStateValue::Normal; //Normal
        }();
        auto const chevronState = HasChildren() && !(m_isClosedCompact && ShouldRepeaterShowInFlyout()) ? (IsExpanded() ? ChevronStateValue::ChevronVisibleOpen : ChevronStateValue::ChevronVisibleClosed) : ChevronStateValue::ChevronHidden;

        auto const pointerChevronState = [this, pointerStateValue, chevronState]() {
            if (chevronState == ChevronStateValue::ChevronHidden)
            {
                if (pointerStateValue == PointerStateValue::Normal)
                {
                    return c_normalChevronHidden;
                }
                else if (pointerStateValue == PointerStateValue::PointerOver)
                {
                    return c_pointerOverChevronHidden;
                }
                else if (pointerStateValue == PointerStateValue::Pressed)
                {
                    return c_pressedChevronHidden;
                }
            }
            else if (chevronState == ChevronStateValue::ChevronVisibleOpen)
            {
                if (pointerStateValue == PointerStateValue::Normal)
                {
                    return c_normalChevronVisibleOpen;
                }
                else if (pointerStateValue == PointerStateValue::PointerOver)
                {
                    return c_pointerOverChevronVisibleOpen;
                }
                else if (pointerStateValue == PointerStateValue::Pressed)
                {
                    return c_pressedChevronVisibleOpen;
                }
            }
            else if (chevronState == ChevronStateValue::ChevronVisibleClosed)
            {
                if (pointerStateValue == PointerStateValue::Normal)
                {
                    return c_normalChevronVisibleClosed;
                }
                else if (pointerStateValue == PointerStateValue::PointerOver)
                {
                    return c_pointerOverChevronVisibleClosed;
                }
                else if (pointerStateValue == PointerStateValue::Pressed)
                {
                    return c_pressedChevronVisibleClosed;
                }
            }
            return c_normalChevronHidden;
        }();
        // Go to the appropriate pointerChevronState
        winrt::VisualStateManager::GoToState(presenter, pointerChevronState, true);

        // Go to the appropriate chevronState
        if (chevronState == ChevronStateValue::ChevronHidden)
        {
            winrt::VisualStateManager::GoToState(presenter, c_chevronHidden, true);
        }
        else if (chevronState == ChevronStateValue::ChevronVisibleOpen)
        {
            winrt::VisualStateManager::GoToState(presenter, c_chevronVisibleOpen, true);
        }
        else if (chevronState == ChevronStateValue::ChevronVisibleClosed)
        {
            winrt::VisualStateManager::GoToState(presenter, c_chevronVisibleClosed, true);
        }
    }
}

bool NavigationViewItem::HasChildren()
{
    return MenuItems().Size() > 0
        || (MenuItemsSource() != nullptr && m_repeater != nullptr && m_repeater.get().ItemsSourceView().Count() > 0)
        || HasUnrealizedChildren();
}

bool NavigationViewItem::ShouldShowIcon()
{
    return static_cast<bool>(Icon());
}

bool NavigationViewItem::ShouldEnableToolTip() const
{
    // We may enable Tooltip for IconOnly in the future, but not now
    return IsOnLeftNav() && m_isClosedCompact;
}

bool NavigationViewItem::ShouldShowContent()
{
    return static_cast<bool>(Content());
}

bool NavigationViewItem::IsOnLeftNav() const
{
    auto const position = Position();
    return position == NavigationViewRepeaterPosition::LeftNav || position == NavigationViewRepeaterPosition::LeftFooter;
}

bool NavigationViewItem::IsOnTopPrimary() const
{
    return Position() == NavigationViewRepeaterPosition::TopPrimary;
}

winrt::UIElement const NavigationViewItem::GetPresenterOrItem() const
{
    if (auto const presenter = m_navigationViewItemPresenter.get())
    {
        return presenter.try_as<winrt::UIElement>();
    }
    else
    {
        return this->try_as<winrt::UIElement>();
    }
}

NavigationViewItemPresenter* NavigationViewItem::GetPresenter() const
{
    NavigationViewItemPresenter* presenter = nullptr;
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
        const bool shouldShowChildren = IsExpanded();
        const auto visibility = shouldShowChildren ? winrt::Visibility::Visible : winrt::Visibility::Collapsed;
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

bool NavigationViewItem::ShouldRepeaterShowInFlyout() const
{
    return (m_isClosedCompact && IsTopLevelItem()) || IsOnTopPrimary();
}

bool NavigationViewItem::IsRepeaterVisible() const
{
    if (auto const repeater = m_repeater.get())
    {
        return repeater.Visibility() == winrt::Visibility::Visible;
    }
    return false;
}

void NavigationViewItem::UpdateItemIndentation()
{
    // Update item indentation based on its depth
    if (auto const presenter = m_navigationViewItemPresenter.get())
    {
        const auto newLeftMargin = Depth() * c_itemIndentation;
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
        const auto focusState = originalSource.FocusState();
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

void NavigationViewItem::ResetTrackedPointerId()
{
    m_trackedPointerId = 0;
}

// Returns False when the provided pointer Id matches the currently tracked Id.
// When there is no currently tracked Id, sets the tracked Id to the provided Id and returns False.
// Returns True when the provided pointer Id does not match the currently tracked Id.
bool NavigationViewItem::IgnorePointerId(const winrt::PointerRoutedEventArgs& args)
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

void NavigationViewItem::OnPresenterPointerPressed(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    MUX_ASSERT(!m_isPressed);
    MUX_ASSERT(!m_capturedPointer);

    // TODO: Update to look at presenter instead
    auto pointerProperties = args.GetCurrentPoint(*this).Properties();
    m_isPressed = pointerProperties.IsLeftButtonPressed() || pointerProperties.IsRightButtonPressed();

    auto pointer = args.Pointer();
    auto presenter = GetPresenterOrItem();

    MUX_ASSERT(presenter);

    if (presenter.CapturePointer(pointer))
    {
        m_capturedPointer = pointer;
    }

    UpdateVisualState(true);
}

void NavigationViewItem::OnPresenterPointerReleased(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    if (m_isPressed)
    {
        m_isPressed = false;

        if (m_capturedPointer)
        {
            auto presenter = GetPresenterOrItem();

            MUX_ASSERT(presenter);

            presenter.ReleasePointerCapture(m_capturedPointer);
        }

        UpdateVisualState(true);
    }
}

void NavigationViewItem::OnPresenterPointerEntered(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    ProcessPointerOver(args);
}

void NavigationViewItem::OnPresenterPointerMoved(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    ProcessPointerOver(args);
}

void NavigationViewItem::OnPresenterPointerExited(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    m_isPointerOver = false;

    if (!m_capturedPointer)
    {
        ResetTrackedPointerId();
    }

    UpdateVisualState(true);
}

void NavigationViewItem::OnPresenterPointerCanceled(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    ProcessPointerCanceled(args);
}

void NavigationViewItem::OnPresenterPointerCaptureLost(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    ProcessPointerCanceled(args);
}

void NavigationViewItem::OnIsEnabledChanged(const winrt::IInspectable&, const winrt::DependencyPropertyChangedEventArgs&)
{
    if (!IsEnabled())
    {
        m_isPressed = false;
        m_isPointerOver = false;

        if (m_capturedPointer)
        {
            auto presenter = GetPresenterOrItem();

            MUX_ASSERT(presenter);

            presenter.ReleasePointerCapture(m_capturedPointer);
            m_capturedPointer = nullptr;
        }

        ResetTrackedPointerId();
    }

    UpdateVisualState(true);
}

void NavigationViewItem::RotateExpandCollapseChevron(bool isExpanded)
{
    if (auto presenter = GetPresenter())
    {
        presenter->RotateExpandCollapseChevron(isExpanded);
    }
}

void NavigationViewItem::ProcessPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    m_isPressed = false;
    m_isPointerOver = false;
    m_capturedPointer = nullptr;
    ResetTrackedPointerId();
    UpdateVisualState(true);
}

void NavigationViewItem::ProcessPointerOver(const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    if (!m_isPointerOver)
    {
        m_isPointerOver = true;
        UpdateVisualState(true);
    }
}

void NavigationViewItem::HookInputEvents(const winrt::IControlProtected& controlProtected)
{
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

    MUX_ASSERT(presenter);

    // Handlers that set flags are skipped when args.Handled is already True.
    m_presenterPointerPressedRevoker = presenter.PointerPressed(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerPressed });
    m_presenterPointerEnteredRevoker = presenter.PointerEntered(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerEntered });
    m_presenterPointerMovedRevoker = presenter.PointerMoved(winrt::auto_revoke, { this, &NavigationViewItem::OnPresenterPointerMoved });

    // Handlers that reset flags are not skipped when args.Handled is already True to avoid broken states.
    m_presenterPointerReleasedRevoker = AddRoutedEventHandler<RoutedEventType::PointerReleased>(
        presenter,
        { this, &NavigationViewItem::OnPresenterPointerReleased },
        true /*handledEventsToo*/);
    m_presenterPointerExitedRevoker = AddRoutedEventHandler<RoutedEventType::PointerExited>(
        presenter,
        { this, &NavigationViewItem::OnPresenterPointerExited },
        true /*handledEventsToo*/);
    m_presenterPointerCanceledRevoker = AddRoutedEventHandler<RoutedEventType::PointerCanceled>(
        presenter,
        { this, &NavigationViewItem::OnPresenterPointerCanceled },
        true /*handledEventsToo*/);
    m_presenterPointerCaptureLostRevoker = AddRoutedEventHandler<RoutedEventType::PointerCaptureLost>(
        presenter,
        { this, &NavigationViewItem::OnPresenterPointerCaptureLost },
        true /*handledEventsToo*/);
}

void NavigationViewItem::UnhookInputEvents()
{
    m_presenterPointerPressedRevoker.revoke();
    m_presenterPointerEnteredRevoker.revoke();
    m_presenterPointerMovedRevoker.revoke();
    m_presenterPointerReleasedRevoker.revoke();
    m_presenterPointerExitedRevoker.revoke();
    m_presenterPointerCanceledRevoker.revoke();
    m_presenterPointerCaptureLostRevoker.revoke();
}

void NavigationViewItem::UnhookEventsAndClearFields()
{
    UnhookInputEvents();

    m_flyoutClosingRevoker.revoke();
    m_splitViewIsPaneOpenChangedRevoker.revoke();
    m_splitViewDisplayModeChangedRevoker.revoke();
    m_splitViewCompactPaneLengthChangedRevoker.revoke();
    m_repeaterElementPreparedRevoker.revoke();
    m_repeaterElementClearingRevoker.revoke();
    m_isEnabledChangedRevoker.revoke();
    m_itemsSourceViewCollectionChangedRevoker.revoke();

    m_rootGrid.set(nullptr);
    m_navigationViewItemPresenter.set(nullptr);
    m_toolTip.set(nullptr);
    m_repeater.set(nullptr);
    m_flyoutContentGrid.set(nullptr);
}
