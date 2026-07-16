// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBar.g.h"
#include "InkToolBar.properties.h"

class InkToolBar :
    public ReferenceTracker<InkToolBar, winrt::implementation::InkToolBarT>, 
    public InkToolBarProperties
{
public:
    InkToolBar();
    
    void OnApplyTemplate();
    winrt::AutomationPeer OnCreateAutomationPeer();

    // Property accessors
    winrt::InkToolBarInitialControls InitialControls();
    void InitialControls(winrt::InkToolBarInitialControls value);

    winrt::DependencyObjectCollection Children();

    winrt::InkToolBarToolButton ActiveTool();
    void ActiveTool(winrt::InkToolBarToolButton value);

    winrt::InkDrawingAttributes InkDrawingAttributes();

    bool IsRulerButtonChecked();
    void IsRulerButtonChecked(bool value);

    winrt::InkCanvas TargetInkCanvas();
    void TargetInkCanvas(winrt::InkCanvas value);

    bool IsStencilButtonChecked();
    void IsStencilButtonChecked(bool value);

    winrt::InkToolBarButtonFlyoutPlacement ButtonFlyoutPlacement();
    void ButtonFlyoutPlacement(winrt::InkToolBarButtonFlyoutPlacement value);

    winrt::Orientation Orientation();
    void Orientation(winrt::Orientation value);

    winrt::IInspectable TargetInkPresenter();
    void TargetInkPresenter(winrt::IInspectable value);

    winrt::InkToolBarToolButton GetToolButton(winrt::InkToolBarTool tool);
    winrt::InkToolBarToggleButton GetToggleButton(winrt::InkToolBarToggle tool);
    winrt::InkToolBarMenuButton GetMenuButton(winrt::InkToolBarMenuKind menu);

private:
    void CreateDefaultToolButtons();
    void OnActiveToolChanged();
    void OnTargetInkCanvasChanged();
    winrt::Windows::UI::Input::Inking::InkPresenter GetInkPresenter();
    void UpdateInkPresenterFromActiveTool();
    void OnToolButtonChecked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void PopulateVisualPanel();

    // Pushes the current ruler-checked state onto the target InkCanvas (shows/hides the
    // ruler stencil) and keeps the toolbar's ruler toggle button visual in sync. Also unifies
    // the stencil state (ruler vs protractor) so the two paths do not fight over the canvas.
    void ApplyRulerState();

    // Member variables
    winrt::InkToolBarInitialControls m_initialControls{ winrt::InkToolBarInitialControls::All };
    winrt::DependencyObjectCollection m_children{ nullptr };
    winrt::InkToolBarToolButton m_activeTool{ nullptr };
    winrt::InkCanvas m_targetInkCanvas{ nullptr };
    winrt::IInspectable m_targetInkPresenter{ nullptr };
    winrt::InkToolBarButtonFlyoutPlacement m_buttonFlyoutPlacement{ winrt::InkToolBarButtonFlyoutPlacement::Auto };
    winrt::Orientation m_orientation{ winrt::Orientation::Horizontal };
    bool m_isRulerButtonChecked{ false };
    bool m_isStencilButtonChecked{ false };
    bool m_defaultToolsCreated{ false };
    bool m_toolButtonsWired{ false };

    winrt::StackPanel m_toolButtonPanel{ nullptr };
    std::vector<winrt::InkToolBarToolButton> m_toolButtons;
    std::vector<winrt::InkToolBarToggleButton> m_toggleButtons;
    std::vector<winrt::InkToolBarMenuButton> m_menuButtons;

    // Default ruler toggle (a ToggleButton styled as a 40x40 tool button). Present when the
    // InitialControls include non-pen tools; toggling it drives IsRulerButtonChecked and
    // the ruler stencil on the target InkCanvas.
    winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton m_rulerToggleButton{ nullptr };
    bool m_includeRulerButton{ false };
    bool m_syncingRulerToggle{ false };

    // Default stencil menu button. m_stencilButton is the registered InkToolBarStencilButton
    // (so GetMenuButton(Stencil) resolves); m_stencilMenuButton is its 40x40 visual with a
    // flyout to pick Ruler/Protractor. Toggling drives IsStencilButtonChecked and the selected
    // stencil on the target InkCanvas.
    winrt::InkToolBarStencilButton m_stencilButton{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton m_stencilMenuButton{ nullptr };
    bool m_includeStencilButton{ false };
    bool m_syncingStencilToggle{ false };
};

