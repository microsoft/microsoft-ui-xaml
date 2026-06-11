// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "InkToolBar.h"
#include "InkToolBarBallpointPenButton.h"
#include "InkToolBarPencilButton.h"
#include "InkToolBarHighlighterButton.h"
#include "InkToolBarEraserButton.h"
#include "InkToolBarStencilButton.h"
#include "InkToolBarTrace.h"

//
// InkToolBar - A toolbar control that pairs with InkCanvas to provide
// inking tools: pen types, eraser, color/thickness selection, ruler/stencil.
//

InkToolBar::InkToolBar()
{
    INKTOOLBAR_TRACE_INFO(nullptr, L"InkToolBar::InkToolBar\n");

    m_children = winrt::DependencyObjectCollection();
}

void InkToolBar::OnApplyTemplate()
{
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::OnApplyTemplate\n");

    if (!m_defaultToolsCreated)
    {
        CreateDefaultToolButtons();
    }

    // If we have a target ink canvas, connect to its presenter
    OnTargetInkCanvasChanged();
}

//
// Property accessors
//

winrt::InkToolBarInitialControls InkToolBar::InitialControls()
{
    return m_initialControls;
}

void InkToolBar::InitialControls(winrt::InkToolBarInitialControls value)
{
    m_initialControls = value;
    // Recreate defaults if we haven't been applied yet
    m_defaultToolsCreated = false;
}

winrt::DependencyObjectCollection InkToolBar::Children()
{
    return m_children;
}

winrt::InkToolBarToolButton InkToolBar::ActiveTool()
{
    return m_activeTool;
}

void InkToolBar::ActiveTool(winrt::InkToolBarToolButton value)
{
    if (m_activeTool != value)
    {
        m_activeTool = value;
        OnActiveToolChanged();
    }
}

winrt::InkDrawingAttributes InkToolBar::InkDrawingAttributes()
{
    auto presenter = GetInkPresenter();
    if (presenter)
    {
        return presenter.CopyDefaultDrawingAttributes();
    }
    return nullptr;
}

bool InkToolBar::IsRulerButtonChecked()
{
    return m_isRulerButtonChecked;
}

void InkToolBar::IsRulerButtonChecked(bool value)
{
    m_isRulerButtonChecked = value;
}

winrt::InkCanvas InkToolBar::TargetInkCanvas()
{
    return m_targetInkCanvas;
}

void InkToolBar::TargetInkCanvas(winrt::InkCanvas value)
{
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::TargetInkCanvas set\n");
    m_targetInkCanvas = value;
    OnTargetInkCanvasChanged();
}

bool InkToolBar::IsStencilButtonChecked()
{
    return m_isStencilButtonChecked;
}

void InkToolBar::IsStencilButtonChecked(bool value)
{
    m_isStencilButtonChecked = value;
}

winrt::InkToolBarButtonFlyoutPlacement InkToolBar::ButtonFlyoutPlacement()
{
    return m_buttonFlyoutPlacement;
}

void InkToolBar::ButtonFlyoutPlacement(winrt::InkToolBarButtonFlyoutPlacement value)
{
    m_buttonFlyoutPlacement = value;
}

winrt::Orientation InkToolBar::Orientation()
{
    return m_orientation;
}

void InkToolBar::Orientation(winrt::Orientation value)
{
    m_orientation = value;
}

winrt::IInspectable InkToolBar::TargetInkPresenter()
{
    return m_targetInkPresenter;
}

void InkToolBar::TargetInkPresenter(winrt::IInspectable value)
{
    m_targetInkPresenter = value;
}

winrt::InkToolBarToolButton InkToolBar::GetToolButton(winrt::InkToolBarTool tool)
{
    for (auto const& button : m_toolButtons)
    {
        if (button.ToolKind() == tool)
        {
            return button;
        }
    }
    return nullptr;
}

winrt::InkToolBarToggleButton InkToolBar::GetToggleButton(winrt::InkToolBarToggle tool)
{
    for (auto const& button : m_toggleButtons)
    {
        if (button.ToggleKind() == tool)
        {
            return button;
        }
    }
    return nullptr;
}

winrt::InkToolBarMenuButton InkToolBar::GetMenuButton(winrt::InkToolBarMenuKind menu)
{
    for (auto const& button : m_menuButtons)
    {
        if (button.MenuKind() == menu)
        {
            return button;
        }
    }
    return nullptr;
}

//
// Private helpers
//

void InkToolBar::CreateDefaultToolButtons()
{
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::CreateDefaultToolButtons InitialControls=%d\n", static_cast<int>(m_initialControls));

    m_toolButtons.clear();
    m_toggleButtons.clear();
    m_menuButtons.clear();

    bool includePens = (m_initialControls == winrt::InkToolBarInitialControls::All ||
                        m_initialControls == winrt::InkToolBarInitialControls::PensOnly);
    bool includeNonPens = (m_initialControls == winrt::InkToolBarInitialControls::All ||
                           m_initialControls == winrt::InkToolBarInitialControls::AllExceptPens);

    if (includePens)
    {
        // Create default pen buttons: Ballpoint, Pencil, Highlighter
        auto ballpoint = winrt::make<InkToolBarBallpointPenButton>();
        m_toolButtons.push_back(ballpoint);

        auto pencil = winrt::make<InkToolBarPencilButton>();
        m_toolButtons.push_back(pencil);

        auto highlighter = winrt::make<InkToolBarHighlighterButton>();
        m_toolButtons.push_back(highlighter);
    }

    if (includeNonPens)
    {
        // Create eraser button
        auto eraser = winrt::make<InkToolBarEraserButton>();
        m_toolButtons.push_back(eraser.as<winrt::InkToolBarToolButton>());

        // Create stencil/ruler button
        auto stencil = winrt::make<InkToolBarStencilButton>();
        m_menuButtons.push_back(stencil.as<winrt::InkToolBarMenuButton>());
    }

    // Set default active tool to first pen if available
    if (!m_toolButtons.empty() && !m_activeTool)
    {
        m_activeTool = m_toolButtons[0];
    }

    m_defaultToolsCreated = true;

    // Add all buttons to children collection
    for (auto const& button : m_toolButtons)
    {
        m_children.Append(button);
    }
    for (auto const& button : m_menuButtons)
    {
        m_children.Append(button);
    }
}

void InkToolBar::OnActiveToolChanged()
{
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::OnActiveToolChanged\n");
    UpdateInkPresenterFromActiveTool();
}

void InkToolBar::OnTargetInkCanvasChanged()
{
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::OnTargetInkCanvasChanged\n");
    UpdateInkPresenterFromActiveTool();
}

winrt::InkPresenter InkToolBar::GetInkPresenter()
{
    // First try the explicit presenter
    if (m_targetInkPresenter)
    {
        return m_targetInkPresenter.as<winrt::InkPresenter>();
    }

    // Then try the target ink canvas
    if (m_targetInkCanvas)
    {
        // Access the InkPresenter through the InkCanvas implementation
        auto inkCanvasImpl = winrt::get_self<InkCanvas>(m_targetInkCanvas);
        if (inkCanvasImpl)
        {
            return inkCanvasImpl->InkPresenter();
        }
    }

    return nullptr;
}

void InkToolBar::UpdateInkPresenterFromActiveTool()
{
    auto presenter = GetInkPresenter();
    if (!presenter || !m_activeTool)
    {
        return;
    }

    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::UpdateInkPresenterFromActiveTool tool=%d\n", 
        static_cast<int>(m_activeTool.ToolKind()));

    // Set the presenter mode based on the active tool
    auto toolKind = m_activeTool.ToolKind();
    
    if (m_targetInkCanvas)
    {
        switch (toolKind)
        {
        case winrt::InkToolBarTool::BallpointPen:
        case winrt::InkToolBarTool::Pencil:
        case winrt::InkToolBarTool::Highlighter:
        case winrt::InkToolBarTool::CustomPen:
            // Set canvas to draw mode
            m_targetInkCanvas.QueueInkPresenterWorkItem([](auto inkPresenter)
                {
                    inkPresenter.InputProcessingConfiguration().Mode(winrt::InkInputProcessingMode::Inking);
                });
            break;

        case winrt::InkToolBarTool::Eraser:
            // Set canvas to erase mode
            m_targetInkCanvas.QueueInkPresenterWorkItem([](auto inkPresenter)
                {
                    inkPresenter.InputProcessingConfiguration().Mode(winrt::InkInputProcessingMode::Erasing);
                });
            break;

        default:
            break;
        }
    }
}
