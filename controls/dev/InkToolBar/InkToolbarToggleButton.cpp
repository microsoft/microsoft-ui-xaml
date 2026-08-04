// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarToggleButton_Partial.cpp.
// Logic preserved; OS-dxaml scaffolding (composable CheckBox factory dance, QueryInterfaceOverride,
// IToggleButtonDerived QI, HRESULT/COM string plumbing) dropped because the lift generated base
// composes CheckBox and *this is the Control. GetLocalizedToggleName is a plain virtual here.

#include "pch.h"
#include "common.h"
#include "InkToolbarToggleButton.h"

void InkToolbarToggleButton::OnApplyTemplate()
{
    // Ensure RTL/LTR + direction states are set up now that the template parts exist (we don't get a
    // notification when FlowDirection is set to its initial value).
    UpdateStates(false);

    // Set tooltip and automation name based on a localized name for the toggle button.
    auto localizedToggleName = GetLocalizedToggleName();
    if (!localizedToggleName.empty())
    {
        winrt::ToolTipService::SetToolTip(*this, winrt::box_value(localizedToggleName));
        winrt::AutomationProperties::SetName(*this, localizedToggleName);
    }
}

void InkToolbarToggleButton::SetButtonDirection(winrt::InkToolbarButtonFlyoutPlacement direction)
{
    m_direction = direction;
    UpdateStates(false);
}

// UWP UpdateStates: RTL/LTR state and flyout-direction state (no extension glyph on a toggle).
void InkToolbarToggleButton::UpdateStates(bool useTransitions)
{
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
