// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarToolButton_Partial.cpp.
// Logic preserved 1:1; OS-dxaml scaffolding (composable RadioButton factory dance,
// QueryInterfaceOverride, RS1 CQuirksMode2 quirk, HRESULT/boxing) dropped because the lift
// generated base composes RadioButton and *this is the Control.

#include "pch.h"
#include "common.h"
#include "InkToolbarToolButton.h"
#include "InkToolbarToolButtonAutomationPeer.h"
#include "InkToolbar.h"

InkToolbarToolButton::InkToolbarToolButton()
{
    // UWP InitializeImpl: create an attached Flyout for this tool's L3 (pen/eraser/stencil config),
    // let it escape XamlIsland bounds, and apply InkToolbarFlyoutStyle if present. RS1 apps kept the
    // AttachedFlyout from generic.xaml; the lift always builds it in code (single generic.xaml style
    // instance would otherwise be shared across all buttons of a type). Done in the ctor here since
    // the RadioButton base is already composed.
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

void InkToolbarToolButton::OnApplyTemplate()
{
    // Push initial visual states now that the template parts (extension glyph, etc.) exist.
    UpdateStates(false);

    // UWP OnApplyTemplateImpl: set the tooltip + automation name from the leaf's localized tool name.
    // UWP used the non-throwing LOG_IF_FAILED; guard the resource lookup so a failure to load the
    // localized string degrades gracefully (empty name) instead of propagating out of OnApplyTemplate.
    winrt::hstring localizedToolName;
    try
    {
        localizedToolName = GetLocalizedToolName();
    }
    catch (winrt::hresult_error const&)
    {
    }
    if (!localizedToolName.empty())
    {
        winrt::ToolTipService::SetToolTip(*this, winrt::box_value(localizedToolName));
        winrt::AutomationProperties::SetName(*this, localizedToolName);
    }

    // Run derived-class template work (pen palette, eraser flyout, etc.). See OnApplyTemplateCore.
    OnApplyTemplateCore();
}

winrt::AutomationPeer InkToolbarToolButton::OnCreateAutomationPeer()
{
    return winrt::make<InkToolbarToolButtonAutomationPeer>(*this);
}

void InkToolbarToolButton::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    // UWP: only IsExtensionGlyphShown drives a state change here.
    if (args.Property() == winrt::InkToolbarToolButton::IsExtensionGlyphShownProperty())
    {
        UpdateStates(true);
    }
}

void InkToolbarToolButton::SetButtonDirection(winrt::InkToolbarButtonFlyoutPlacement direction)
{
    m_direction = direction;
    UpdateStates(false);
}

// UWP UpdateStates: extension-glyph visibility state, RTL/LTR state, and flyout-direction state.
void InkToolbarToolButton::UpdateStates(bool useTransitions)
{
    const bool isExtensionGlyphShown = IsExtensionGlyphShown();
    winrt::VisualStateManager::GoToState(
        *this, isExtensionGlyphShown ? L"ShowExtensionGlyph" : L"HideExtensionGlyph", useTransitions);

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
        directionState = isRtl ? L"RightDirection" : L"LeftDirection";
        break;
    case winrt::InkToolbarButtonFlyoutPlacement::Right:
        directionState = isRtl ? L"LeftDirection" : L"RightDirection";
        break;
    default:
        directionState = L"BottomDirection";
        break;
    }
    winrt::VisualStateManager::GoToState(*this, directionState, useTransitions);
}

void InkToolbarToolButton::SetParentInkToolbar(winrt::InkToolbar const& inkToolbar)
{
    m_parentInkToolbar = winrt::make_weak(inkToolbar);
}

winrt::InkToolbar InkToolbarToolButton::GetParentInkToolbar()
{
    return m_parentInkToolbar.get();
}

// L3 = the tool's config flyout (the attached Flyout created above / assigned by the pen buttons).
bool InkToolbarToolButton::HasL3()
{
    return winrt::FlyoutBase::GetAttachedFlyout(*this) != nullptr;
}

bool InkToolbarToolButton::IsL3Open()
{
    // UWP tracked open L3s in InkToolbar::m_openFlyouts (Opened/Closed notifications). That container-
    // side tracking is restored when InkToolbar_Partial is ported; until then report closed.
    return false;
}

void InkToolbarToolButton::OpenL3()
{
    winrt::FlyoutBase::ShowAttachedFlyout(*this);
}

void InkToolbarToolButton::CloseL3()
{
    if (auto flyout = winrt::FlyoutBase::GetAttachedFlyout(*this))
    {
        flyout.Hide();
    }
}
