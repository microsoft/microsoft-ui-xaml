// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarMenuButton.g.h"
#include "InkToolbarMenuButton.properties.h"

// UWP InkToolbarMenuButtonCheckedState (from InkToolbarMenuButton_Partial.h): the tri-state check value
// the InkToolbar container uses for menu buttons (a plain enum, not a projected type).
enum class InkToolbarMenuButtonCheckedState
{
    Unchecked = 0,
    Checked,
    Indeterminate
};

class InkToolbarMenuButton :
    public ReferenceTracker<InkToolbarMenuButton, winrt::implementation::InkToolbarMenuButtonT, winrt::composable>, 
    public InkToolbarMenuButtonProperties
{
public:
    InkToolbarMenuButton();

    // IFrameworkElementOverrides
    void OnApplyTemplate();

    // Derived-class OnApplyTemplate extension point. Same rationale as InkToolbarToolButton::OnApplyTemplateCore:
    // the generated per-leaf IFrameworkElementOverridesT<D> forwarder shadows an OnApplyTemplate declared on an
    // intermediate impl (e.g. StencilButton), so only this base's OnApplyTemplate is dispatched for a concrete
    // leaf; this plain C++ virtual reaches the derived logic via ordinary vtable dispatch.
    virtual void OnApplyTemplateCore() {}

    // IToggleButtonOverrides - suppress default toggle (container manages tri-state checked value).
    void OnToggle();
    // IUIElementOverrides
    winrt::AutomationPeer OnCreateAutomationPeer();
    // DependencyObject property-changed hook
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    winrt::InkToolbarMenuKind MenuKind() { return m_menuKind; }

    void SetParentInkToolbar(winrt::InkToolbar const& inkToolbar);
    winrt::InkToolbar GetParentInkToolbar();
    void SetButtonDirection(winrt::InkToolbarButtonFlyoutPlacement direction);
    void UpdateStates(bool useTransitions);
    void UpdateMenuButtonToolTip();

    // Methods for use by the automation peer.
    bool HasL3();
    bool IsL3Open();
    void OpenL3();
    void CloseL3();

protected:
    void SetMenuKind(winrt::InkToolbarMenuKind value) { m_menuKind = value; }

    // Leaf override hooks (UWP IMenuButtonDerived). Empty defaults = lifted localization gap
    // (per-tool localized names/flyout names, same class of delta as ColorNames).
    virtual winrt::hstring GetLocalizedToolName() { return {}; }
    virtual winrt::hstring GetFlyoutName() { return {}; }

private:
    winrt::InkToolbarMenuKind m_menuKind{ winrt::InkToolbarMenuKind::Stencil };
    winrt::InkToolbarButtonFlyoutPlacement m_direction{ winrt::InkToolbarButtonFlyoutPlacement::Auto };
    winrt::weak_ref<winrt::InkToolbar> m_parentInkToolbar{ nullptr };
};

