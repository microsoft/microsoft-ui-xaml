// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBar.g.h"
#include "InkToolBar.properties.h"
#include "InkToolBarTrace.h"
#include "../InkCanvas/InkCanvas.h"

class InkToolBar :
    public ReferenceTracker<InkToolBar, winrt::implementation::InkToolBarT>, 
    public InkToolBarProperties
{
public:
    InkToolBar();

    // IInkToolBar
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

    // Lifecycle
    void OnApplyTemplate();

private:
    void CreateDefaultToolButtons();
    void UpdateInkPresenterFromActiveTool();
    void OnActiveToolChanged();
    void OnTargetInkCanvasChanged();
    winrt::InkPresenter GetInkPresenter();

    // State
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

    // Tool button storage
    std::vector<winrt::InkToolBarToolButton> m_toolButtons;
    std::vector<winrt::InkToolBarToggleButton> m_toggleButtons;
    std::vector<winrt::InkToolBarMenuButton> m_menuButtons;
};

