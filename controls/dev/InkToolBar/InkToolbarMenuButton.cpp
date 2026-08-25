// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarMenuButton_Partial.cpp.
// Logic preserved; OS-dxaml scaffolding (composable ToggleButton factory dance, QueryInterfaceOverride,
// IMenuButtonDerived QI, RuntimeClassInitialize, HRESULT/boxing) dropped because the lift generated
// base composes Primitives.ToggleButton and *this is the Control. GetMenuKind/GetLocalizedToolName/
// GetFlyoutName are plain virtuals here.

#include "pch.h"
#include "common.h"
#include "InkToolbarMenuButton.h"
#include "InkToolbarMenuButtonAutomationPeer.h"
#include "InkToolbar.h"
#include "InkToolbarTrace.h"

InkToolbarMenuButton::InkToolbarMenuButton()
{
    // UWP InitializeImpl: create an attached Flyout for this menu's L3 (stencil/eraser config), let it
    // escape XamlIsland bounds, and apply InkToolbarFlyoutStyle if present. Built in code (not from
    // generic.xaml) so each button gets its own flyout instance. See InkToolbarToolButton ctor.
    auto flyout = winrt::Flyout{};
    flyout.ShouldConstrainToRootBounds(false);

    if (auto resources = Resources())
    {
        if (resources.HasKey(winrt::box_value(L"InkToolbarFlyoutStyle")))
        {
            if (auto style = resources.Lookup(winrt::box_value(L"InkToolbarFlyoutStyle")).try_as<winrt::Style>())
            {
                flyout.FlyoutPresenterStyle(style);
            }
        }
    }

    winrt::FlyoutBase::SetAttachedFlyout(*this, flyout);
}

void InkToolbarMenuButton::OnApplyTemplate()
{
    // Configure the toggle button to be tri-state (unchecked / checked / indeterminate).
    IsThreeState(true);

    // The extension glyph may have been set before we got here, so update visual state now. Also
    // ensures RTL/LTR is set up correctly (no notification when FlowDirection takes its initial value).
    UpdateStates(false);

    UpdateMenuButtonToolTip();

    // Set the flyout name for automation.
    auto flyoutName = GetFlyoutName();
    if (!flyoutName.empty())
    {
        if (auto flyout = winrt::FlyoutBase::GetAttachedFlyout(*this))
        {
            winrt::AutomationProperties::SetName(flyout, flyoutName);
        }
    }

    // Run derived-class template work (stencil glyph, etc.). See OnApplyTemplateCore.
    OnApplyTemplateCore();
}

// UWP OnToggleImpl: ignore all toggle actions. OnToggle() runs before the Click event, so if we let
// the base toggle, the InkToolbar would read an already-toggled value; the container determines the
// target Checked state itself to support the tri-state button behavior.
void InkToolbarMenuButton::OnToggle()
{
}

winrt::AutomationPeer InkToolbarMenuButton::OnCreateAutomationPeer()
{
    return winrt::make<InkToolbarMenuButtonAutomationPeer>(*this);
}

void InkToolbarMenuButton::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    if (args.Property() == winrt::InkToolbarMenuButton::IsExtensionGlyphShownProperty())
    {
        UpdateStates(true);
    }
}

void InkToolbarMenuButton::SetButtonDirection(winrt::InkToolbarButtonFlyoutPlacement direction)
{
    m_direction = direction;
    UpdateStates(false);
}

// UWP UpdateStates: manual Unchecked state, extension-glyph visibility, RTL/LTR, and flyout-direction
// (with dedicated RTL variants for Left/Right, unlike the tool button which flips the plain states).
void InkToolbarMenuButton::UpdateStates(bool useTransitions)
{
    // Manage the Unchecked state manually since ToggleButton doesn't do it for us.
    if (auto isChecked = IsChecked())
    {
        if (!isChecked.Value())
        {
            winrt::VisualStateManager::GoToState(*this, L"Unchecked", useTransitions);
        }
    }

    winrt::VisualStateManager::GoToState(
        *this, IsExtensionGlyphShown() ? L"ShowExtensionGlyph" : L"HideExtensionGlyph", useTransitions);

    const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;
    winrt::VisualStateManager::GoToState(*this, isRtl ? L"RightToLeft" : L"LeftToRight", useTransitions);

    winrt::hstring directionState;
    switch (m_direction)
    {
    case winrt::InkToolbarButtonFlyoutPlacement::Auto:
    case winrt::InkToolbarButtonFlyoutPlacement::Bottom:
        directionState = L"BottomDirection";
        break;
    case winrt::InkToolbarButtonFlyoutPlacement::Top:
        directionState = L"TopDirection";
        break;
    case winrt::InkToolbarButtonFlyoutPlacement::Left:
        directionState = isRtl ? L"LeftDirectionRTL" : L"LeftDirection";
        break;
    case winrt::InkToolbarButtonFlyoutPlacement::Right:
        directionState = isRtl ? L"RightDirectionRTL" : L"RightDirection";
        break;
    default:
        directionState = L"BottomDirection";
        break;
    }
    winrt::VisualStateManager::GoToState(*this, directionState, false);
}

// UWP: set tooltip and automation name based on a localized name for the tool button.
void InkToolbarMenuButton::UpdateMenuButtonToolTip()
{
    // Guard the resource lookup (UWP used the non-throwing LOG_IF_FAILED) so a failure to load the
    // localized string degrades gracefully instead of propagating out of the caller.
    winrt::hstring localizedToolName;
    try
    {
        localizedToolName = GetLocalizedToolName();
    }
    catch (winrt::hresult_error const& e)
    {
        InkToolbarLogHResult(e.code(), L"menu button name lookup");
    }
    if (!localizedToolName.empty())
    {
        winrt::ToolTipService::SetToolTip(*this, winrt::box_value(localizedToolName));
        winrt::AutomationProperties::SetName(*this, localizedToolName);
    }
}

void InkToolbarMenuButton::SetParentInkToolbar(winrt::InkToolbar const& inkToolbar)
{
    m_parentInkToolbar = winrt::make_weak(inkToolbar);
}

winrt::InkToolbar InkToolbarMenuButton::GetParentInkToolbar()
{
    return m_parentInkToolbar.get();
}

bool InkToolbarMenuButton::HasL3()
{
    return winrt::FlyoutBase::GetAttachedFlyout(*this) != nullptr;
}

bool InkToolbarMenuButton::IsL3Open()
{
    // Container-side open tracking is restored when InkToolbar_Partial is ported; report closed here.
    return false;
}

void InkToolbarMenuButton::OpenL3()
{
    winrt::FlyoutBase::ShowAttachedFlyout(*this);
}

void InkToolbarMenuButton::CloseL3()
{
    if (auto flyout = winrt::FlyoutBase::GetAttachedFlyout(*this))
    {
        flyout.Hide();
    }
}
