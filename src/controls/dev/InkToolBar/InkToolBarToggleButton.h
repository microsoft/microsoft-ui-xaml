// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarToggleButton.g.h"

class InkToolbarToggleButton :
    public ReferenceTracker<InkToolbarToggleButton, winrt::implementation::InkToolbarToggleButtonT, winrt::composable>
{
public:
    InkToolbarToggleButton();

    // IFrameworkElementOverrides
    void OnApplyTemplate();

    winrt::InkToolbarToggle ToggleKind() { return m_toggleKind; }

    // Internal (not projected): lets the InkToolbar register a built-in toggle (e.g. the ruler)
    // with a specific ToggleKind so GetToggleButton(kind) can resolve it.
    void SetToggleKind(winrt::InkToolbarToggle value) { m_toggleKind = value; }

    // Update the button orientation based on the preferred flyout direction.
    void SetButtonDirection(winrt::InkToolbarButtonFlyoutPlacement direction);

protected:
    // Leaf-derived override hook (UWP IToggleButtonDerived::GetLocalizedToggleName): supplies a
    // localized name used for tooltip + automation name. Empty => none. NOTE: per-toggle localized
    // strings are a lifted resource gap (same class of delta as ColorNames) - default empty for now.
    virtual winrt::hstring GetLocalizedToggleName() { return {}; }

private:
    void UpdateStates(bool useTransitions);

    winrt::InkToolbarToggle m_toggleKind{ winrt::InkToolbarToggle::Custom };
    winrt::InkToolbarButtonFlyoutPlacement m_direction{ winrt::InkToolbarButtonFlyoutPlacement::Auto };
};

