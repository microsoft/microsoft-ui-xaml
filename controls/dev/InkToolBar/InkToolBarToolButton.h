// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarToolButton.g.h"
#include "InkToolbarToolButton.properties.h"

class InkToolbarToolButton :
    public ReferenceTracker<InkToolbarToolButton, winrt::implementation::InkToolbarToolButtonT, winrt::composable>, 
    public InkToolbarToolButtonProperties
{
public:
    InkToolbarToolButton();

    // IUIElementOverrides / IFrameworkElementOverrides (UWP OnApplyTemplateImpl / OnCreateAutomationPeerImpl).
    void OnApplyTemplate();
    winrt::AutomationPeer OnCreateAutomationPeer();

    // DP change routing (InkToolbarToolButtonProperties calls this).
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    // Update the button orientation based on the preferred flyout direction.
    void SetButtonDirection(winrt::InkToolbarButtonFlyoutPlacement direction);

    // Parent back-reference (weak; UWP m_parentInkToolbar wrl::WeakRef).
    void SetParentInkToolbar(winrt::InkToolbar const& inkToolbar);
    winrt::InkToolbar GetParentInkToolbar();

    // For the automation peer.
    bool HasL3();
    bool IsL3Open();
    void OpenL3();
    void CloseL3();

    // get_ToolKind (plain read-only property, impl-owned; not a DP).
    winrt::InkToolbarTool ToolKind() { return m_toolKind; }

    // UWP IToolButtonDerived::GetLocalizedToolName: leaf buttons supply their localized tool name;
    // InkToolbarToolButton::OnApplyTemplate applies it as the tooltip + AutomationProperties.Name.
    virtual winrt::hstring GetLocalizedToolName() { return {}; }

protected:
    void SetToolKind(winrt::InkToolbarTool kind) { m_toolKind = kind; }

    // Derived-class OnApplyTemplate extension point. The generated per-leaf IFrameworkElementOverridesT<D>
    // default forwarder shadows any OnApplyTemplate declared on an *intermediate* impl (e.g. PenButton,
    // EraserButton), so only this deepest base's OnApplyTemplate is dispatched at runtime for a concrete
    // leaf button (Ballpoint/Pencil/Highlighter/Eraser). To reach the intermediate logic we invoke this
    // plain C++ virtual at the end of OnApplyTemplate: ordinary vtable dispatch on the impl object reliably
    // reaches the most-derived override.
    virtual void OnApplyTemplateCore() {}

private:
    // Pushes IsExtensionGlyphShown / RTL / flyout-direction visual states (UWP UpdateStates).
    void UpdateStates(bool useTransitions);

    winrt::weak_ref<winrt::InkToolbar> m_parentInkToolbar{ nullptr };
    winrt::InkToolbarButtonFlyoutPlacement m_direction{ winrt::InkToolbarButtonFlyoutPlacement::Bottom };

    winrt::InkToolbarTool m_toolKind{ winrt::InkToolbarTool::BallpointPen };
};

