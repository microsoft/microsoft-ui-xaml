// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InkToolBar.h"
#include "InkToolBarTrace.h"
#include "ResourceAccessor.h"
#include "InkToolBarBallpointPenButton.h"
#include "InktoolBarPencilButton.h"
#include "InkToolBarHighlighterButton.h"
#include "InkToolBarEraserButton.h"
#include "InkToolBarStencilButton.h"
#include "InkToolBarToggleButton.h"
#include "InkToolBarRulerButton.h"
#include "InkToolBarIsStencilButtonCheckedChangedEventArgs.h"
#include "InkToolBarEraserFlyoutItemClickedEventArgs.h"
#include "InkCanvas.h"  // For the InkCanvas type; canvas-backed configuration flows through its InkPresenter.
#include "InkPresenter.h"  // For the InkPresenter proxy that owns the ruler / protractor stencils.
#include "InkToolBarAutomationPeer.h"

// Define static trace variables
bool InkToolBarTrace::s_IsDebugOutputEnabled = false;
bool InkToolBarTrace::s_IsVerboseDebugOutputEnabled = false;

namespace {
// Falls back to an English literal when the .resw isn't resolvable
// (seen on x86chk unpackaged TAEF runs where the InkToolBar PRI
// satellite isn't merged into the test app's resources.pri).
winrt::hstring GetLabelOrFallback(const wchar_t* resourceId, const wchar_t* fallback)
{
    try
    {
        return ResourceAccessor::GetLocalizedStringResource(resourceId);
    }
    catch (...)
    {
        return winrt::hstring{ fallback };
    }
}

// Self-contained ControlTemplate for the InkToolBarToolButton (a RadioButton).
// The lifted control ships no default tool-button style yet, so without this the
// RadioButton-derived tool buttons render as invisible dots. Loaded at runtime
// through XamlReader so the 40x40 chrome + hover/pressed/checked visual states +
// the 2px accent selection stripe travel with the code instead of depending on a
// merged theme dictionary. Brushes are hardcoded on purpose: the UWP-era brush
// keys are not shipped by the lifted theme dictionary, and a ThemeResource lookup
// throws during template apply on x86chk hosts (STATUS_STOWED_EXCEPTION
// 0xC000027B). Mirrors DefaultInkToolBarToolButtonStyle from the WinUI 2
// InkToolbar (onecoreuap\windows\dxaml\xcp\components\inkcontrols\lib).
constexpr wchar_t c_toolButtonStyleXaml[] = LR"XAML(
<Style xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
       xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
       TargetType="RadioButton">
  <Setter Property="Width" Value="40"/>
  <Setter Property="Height" Value="40"/>
  <Setter Property="MinWidth" Value="40"/>
  <Setter Property="MinHeight" Value="40"/>
  <Setter Property="Padding" Value="0"/>
  <Setter Property="Margin" Value="0"/>
  <Setter Property="Background" Value="Transparent"/>
  <Setter Property="BorderThickness" Value="0"/>
  <Setter Property="HorizontalContentAlignment" Value="Center"/>
  <Setter Property="VerticalContentAlignment" Value="Center"/>
  <Setter Property="Template">
    <Setter.Value>
      <ControlTemplate TargetType="RadioButton">
        <Grid x:Name="RootGrid" Background="{TemplateBinding Background}" CornerRadius="4">
          <VisualStateManager.VisualStateGroups>
            <VisualStateGroup x:Name="CommonStates">
              <VisualState x:Name="Normal"/>
              <VisualState x:Name="PointerOver">
                <Storyboard>
                  <ObjectAnimationUsingKeyFrames Storyboard.TargetName="RootGrid" Storyboard.TargetProperty="Background">
                    <DiscreteObjectKeyFrame KeyTime="0" Value="#14000000"/>
                  </ObjectAnimationUsingKeyFrames>
                </Storyboard>
              </VisualState>
              <VisualState x:Name="Pressed">
                <Storyboard>
                  <ObjectAnimationUsingKeyFrames Storyboard.TargetName="RootGrid" Storyboard.TargetProperty="Background">
                    <DiscreteObjectKeyFrame KeyTime="0" Value="#28000000"/>
                  </ObjectAnimationUsingKeyFrames>
                </Storyboard>
              </VisualState>
              <VisualState x:Name="Disabled">
                <Storyboard>
                  <DoubleAnimation Storyboard.TargetName="RootGrid" Storyboard.TargetProperty="Opacity" To="0.4" Duration="0"/>
                </Storyboard>
              </VisualState>
            </VisualStateGroup>
            <VisualStateGroup x:Name="CheckStates">
              <VisualState x:Name="Checked">
                <Storyboard>
                  <ObjectAnimationUsingKeyFrames Storyboard.TargetName="RootGrid" Storyboard.TargetProperty="Background">
                    <DiscreteObjectKeyFrame KeyTime="0" Value="#28000000"/>
                  </ObjectAnimationUsingKeyFrames>
                  <ObjectAnimationUsingKeyFrames Storyboard.TargetName="SelectionStripe" Storyboard.TargetProperty="Visibility">
                    <DiscreteObjectKeyFrame KeyTime="0" Value="Visible"/>
                  </ObjectAnimationUsingKeyFrames>
                </Storyboard>
              </VisualState>
              <VisualState x:Name="Unchecked"/>
              <VisualState x:Name="Indeterminate"/>
            </VisualStateGroup>
          </VisualStateManager.VisualStateGroups>
          <ContentPresenter x:Name="ContentPresenter"
                            HorizontalAlignment="Center"
                            VerticalAlignment="Center"
                            Content="{TemplateBinding Content}"
                            ContentTemplate="{TemplateBinding ContentTemplate}"/>
          <Rectangle x:Name="SelectionStripe"
                     Height="2"
                     Fill="#FF0078D4"
                     VerticalAlignment="Bottom"
                     HorizontalAlignment="Stretch"
                     Margin="4,0,4,3"
                     Visibility="Collapsed"/>
        </Grid>
      </ControlTemplate>
    </Setter.Value>
  </Setter>
</Style>
)XAML";

// Maps the public ButtonFlyoutPlacement onto the XAML FlyoutPlacementMode used by
// the per-tool flyouts. Auto follows the WinUI 2 default of opening below the bar.
winrt::FlyoutPlacementMode ToFlyoutPlacementMode(winrt::InkToolBarButtonFlyoutPlacement placement)
{
    switch (placement)
    {
    case winrt::InkToolBarButtonFlyoutPlacement::Top:    return winrt::FlyoutPlacementMode::Top;
    case winrt::InkToolBarButtonFlyoutPlacement::Left:   return winrt::FlyoutPlacementMode::Left;
    case winrt::InkToolBarButtonFlyoutPlacement::Right:  return winrt::FlyoutPlacementMode::Right;
    case winrt::InkToolBarButtonFlyoutPlacement::Bottom: return winrt::FlyoutPlacementMode::Bottom;
    case winrt::InkToolBarButtonFlyoutPlacement::Auto:
    default:                                             return winrt::FlyoutPlacementMode::Bottom;
    }
}
}

//
// InkToolBar - A toolbar control that pairs with InkCanvas to provide
// inking tools: pen types, eraser, color/thickness selection, ruler/stencil.
//

InkToolBar::InkToolBar()
{
    INKTOOLBAR_TRACE_INFO(nullptr, L"InkToolBar::InkToolBar\n");

    SetDefaultStyleKey(this);
    m_children = winrt::DependencyObjectCollection();

    // Fallback: if OnApplyTemplate doesn't populate buttons (template failure),
    // build them in the Loaded event instead.
    Loaded([this](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
    {
        INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::Loaded  panel=%p  toolButtons=%d\n",
            winrt::get_abi(m_toolButtonPanel), static_cast<int>(m_toolButtons.size()));

        if (!m_toolButtonPanel)
        {
            // Template didn't load — build UI programmatically
            auto panel = winrt::StackPanel();
            panel.Orientation(m_orientation);
            panel.Spacing(4);
            m_toolButtonPanel = panel;
        }
        else
        {
            m_toolButtonPanel.Orientation(m_orientation);
        }

        if (!m_defaultToolsCreated)
        {
            CreateDefaultToolButtons();
        }

        // Always (re)populate — this is our safety net
        PopulateVisualPanel();
        OnTargetInkCanvasChanged();
    });
}

void InkToolBar::OnApplyTemplate()
{
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::OnApplyTemplate\n");

    m_toolButtonPanel = GetTemplateChild(L"ToolButtonPanel").try_as<winrt::StackPanel>();

    if (m_toolButtonPanel)
    {
        m_toolButtonPanel.Orientation(m_orientation);
    }

    if (!m_defaultToolsCreated)
    {
        CreateDefaultToolButtons();
    }

    PopulateVisualPanel();

    // If we have a target ink canvas, connect to its presenter
    OnTargetInkCanvasChanged();
}

winrt::AutomationPeer InkToolBar::OnCreateAutomationPeer()
{
    return winrt::make<InkToolBarAutomationPeer>(*this);
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
    if (m_initialControls == value)
    {
        return;
    }

    m_initialControls = value;
    // Recreate defaults when changed. If the template is already applied, refresh
    // the visual panel immediately so the property behaves as a live API.
    m_defaultToolsCreated = false;
    if (m_toolButtonPanel)
    {
        CreateDefaultToolButtons();
        PopulateVisualPanel();
        UpdateInkPresenterFromActiveTool();
        ApplyRulerState();
    }
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
    // Mirror through the DP so XAML bindings and the generated property store see the value.
    InkToolBarProperties::ActiveTool(value);
    if (m_activeTool != value)
    {
        m_activeTool = value;
        OnActiveToolChanged();
    }
}

winrt::InkDrawingAttributes InkToolBar::InkDrawingAttributes()
{
    if (auto presenter = GetInkPresenter())
    {
        return presenter.CopyDefaultDrawingAttributes();
    }

    // Canvas-backed case: the OS presenter lives on the ink thread, so read the
    // attributes cached on the InkCanvas's marshaling InkPresenter instead.
    if (m_targetInkCanvas)
    {
        return m_targetInkCanvas.InkPresenter().CopyDefaultDrawingAttributes();
    }

    return nullptr;
}

bool InkToolBar::IsRulerButtonChecked()
{
    return m_isRulerButtonChecked;
}

void InkToolBar::IsRulerButtonChecked(bool value)
{
    InkToolBarProperties::IsRulerButtonChecked(value);
    if (m_isRulerButtonChecked != value)
    {
        m_isRulerButtonChecked = value;
        ApplyRulerState();
    }
}

winrt::InkCanvas InkToolBar::TargetInkCanvas()
{
    return m_targetInkCanvas;
}

void InkToolBar::TargetInkCanvas(winrt::InkCanvas value)
{
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::TargetInkCanvas set\n");
    InkToolBarProperties::TargetInkCanvas(value);
    m_targetInkCanvas = value;
    OnTargetInkCanvasChanged();
}

bool InkToolBar::IsStencilButtonChecked()
{
    return m_isStencilButtonChecked;
}

void InkToolBar::IsStencilButtonChecked(bool value)
{
    InkToolBarProperties::IsStencilButtonChecked(value);
    if (m_isStencilButtonChecked != value)
    {
        m_isStencilButtonChecked = value;

        // Find a stencil button to report; nullptr is acceptable when none has been added.
        winrt::InkToolBarStencilButton stencilButton{ nullptr };
        for (auto const& mb : m_menuButtons)
        {
            if (auto sb = mb.try_as<winrt::InkToolBarStencilButton>())
            {
                stencilButton = sb;
                break;
            }
        }
        auto kind = stencilButton ? stencilButton.SelectedStencil() : winrt::InkToolBarStencilKind::Ruler;
        auto args = winrt::make<winrt::implementation::InkToolBarIsStencilButtonCheckedChangedEventArgs>(stencilButton, kind);
        m_isStencilButtonCheckedChangedEventSource(*this, args);

        // Show/hide the selected stencil on the target canvas.
        ApplyRulerState();
    }
}

winrt::InkToolBarButtonFlyoutPlacement InkToolBar::ButtonFlyoutPlacement()
{
    return m_buttonFlyoutPlacement;
}

void InkToolBar::ButtonFlyoutPlacement(winrt::InkToolBarButtonFlyoutPlacement value)
{
    if (m_buttonFlyoutPlacement == value)
    {
        return;
    }

    InkToolBarProperties::ButtonFlyoutPlacement(value);
    m_buttonFlyoutPlacement = value;

    // Propagate to existing attached flyouts so runtime changes are observable.
    for (auto const& button : m_toolButtons)
    {
        if (auto rb = button.try_as<winrt::RadioButton>())
        {
            if (auto flyout = winrt::FlyoutBase::GetAttachedFlyout(rb))
            {
                flyout.Placement(ToFlyoutPlacementMode(value));
            }
        }
    }
}

winrt::Orientation InkToolBar::Orientation()
{
    return m_orientation;
}

void InkToolBar::Orientation(winrt::Orientation value)
{
    InkToolBarProperties::Orientation(value);
    if (m_orientation != value)
    {
        m_orientation = value;
        if (m_toolButtonPanel)
        {
            m_toolButtonPanel.Orientation(value);
        }
    }
}

winrt::IInspectable InkToolBar::TargetInkPresenter()
{
    return m_targetInkPresenter;
}

void InkToolBar::TargetInkPresenter(winrt::IInspectable value)
{
    InkToolBarProperties::TargetInkPresenter(value);
    m_targetInkPresenter = value;
    OnTargetInkCanvasChanged();
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
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::CreateDefaultToolButtons InitialControls=%d\\n", static_cast<int>(m_initialControls));

    m_toolButtons.clear();
    m_toggleButtons.clear();
    m_menuButtons.clear();
    m_stencilButton = nullptr;
    m_toolButtonsWired = false;

    bool includePens = (m_initialControls == winrt::InkToolBarInitialControls::All ||
                        m_initialControls == winrt::InkToolBarInitialControls::PensOnly);
    bool includeNonPens = (m_initialControls == winrt::InkToolBarInitialControls::All ||
                           m_initialControls == winrt::InkToolBarInitialControls::AllExceptPens);

    if (includePens)
    {
        auto ballpoint = winrt::make<InkToolBarBallpointPenButton>();
        auto pencil = winrt::make<InkToolBarPencilButton>();
        auto highlighter = winrt::make<InkToolBarHighlighterButton>();
        m_toolButtons.push_back(ballpoint);
        m_toolButtons.push_back(pencil);
        m_toolButtons.push_back(highlighter);
    }

    if (includeNonPens)
    {
        auto eraser = winrt::make<InkToolBarEraserButton>();
        m_toolButtons.push_back(eraser);

        // Register the ruler toggle as an InkToolBarRulerButton (a strongly-typed
        // InkToolBarToggleButton whose ToggleKind is Ruler) so GetToggleButton(Ruler) resolves and
        // apps can cast it to InkToolBarRulerButton (WUXC parity). Its 40x40 visual is the separate
        // m_rulerToggleButton built in PopulateVisualPanel, kept in sync via ApplyRulerState.
        auto rulerButton = winrt::make<InkToolBarRulerButton>();
        m_toggleButtons.push_back(rulerButton);

        // Register a stencil menu button so GetMenuButton(Stencil) resolves and the stencil
        // (ruler/protractor) can be driven on the target canvas.
        auto stencil = winrt::make<InkToolBarStencilButton>();
        m_stencilButton = stencil;
        m_menuButtons.push_back(stencil);
    }

    // The default toolbar includes a ruler toggle and a stencil menu button alongside the
    // non-pen tools (matches the WinUI 2 InkToolbar default set).
    m_includeRulerButton = includeNonPens;
    m_includeStencilButton = includeNonPens;

    // Keep ActiveTool valid after the default set is rebuilt. If the previous active
    // tool is no longer present, select the first available tool.
    bool activeStillPresent = false;
    if (m_activeTool)
    {
        for (auto const& button : m_toolButtons)
        {
            if (button == m_activeTool)
            {
                activeStillPresent = true;
                break;
            }
        }
    }

    if (!activeStillPresent)
    {
        m_activeTool = m_toolButtons.empty() ? nullptr : m_toolButtons[0];
    }

    if (!m_toolButtons.empty() && !m_activeTool)
    {
        m_activeTool = m_toolButtons[0];
    }

    m_defaultToolsCreated = true;
}

void InkToolBar::OnActiveToolChanged()
{
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::OnActiveToolChanged\n");
    UpdateInkPresenterFromActiveTool();
    m_activeToolChangedEventSource(*this, nullptr);
}

void InkToolBar::OnTargetInkCanvasChanged()
{
    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::OnTargetInkCanvasChanged\n");

    // Bind any stencil buttons to the current ink presenter so their Ruler/Protractor properties work.
    if (auto presenter = GetInkPresenter())
    {
        for (auto const& mb : m_menuButtons)
        {
            if (auto sb = mb.try_as<winrt::InkToolBarStencilButton>())
            {
                winrt::get_self<InkToolBarStencilButton>(sb)->SetInkPresenter(presenter);
            }
        }
    }

    UpdateInkPresenterFromActiveTool();

    // Push the current ruler state onto the (possibly new) target canvas.
    ApplyRulerState();
}

winrt::Windows::UI::Input::Inking::InkPresenter InkToolBar::GetInkPresenter()
{
    // Only an explicitly-provided presenter can be handed back as the OS type. A target
    // InkCanvas instead exposes a marshaling InkPresenter (its OS presenter lives on
    // the ink thread and cannot be handed out), so canvas-backed configuration flows
    // through that InkPresenter in UpdateInkPresenterFromActiveTool.
    if (m_targetInkPresenter)
    {
        return m_targetInkPresenter.try_as<winrt::Windows::UI::Input::Inking::InkPresenter>();
    }

    return nullptr;
}

void InkToolBar::UpdateInkPresenterFromActiveTool()
{
    // Canvas-backed configuration flows through the InkPresenter below and does
    // not require a hand-out of the OS presenter, so only the active tool is required here.
    if (!m_activeTool)
    {
        return;
    }

    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::UpdateInkPresenterFromActiveTool tool=%d\n", 
        static_cast<int>(m_activeTool.ToolKind()));

    // Set the presenter mode based on the active tool
    auto toolKind = m_activeTool.ToolKind();
    
    if (m_targetInkCanvas)
    {
        // Drive the canvas through its InkPresenter so the presenter's UI-thread cache
        // (Mode, default drawing attributes) stays in sync with the toolbar while it
        // internally marshals each call to the ink thread.
        auto proxy = m_targetInkCanvas.InkPresenter();

        switch (toolKind)
        {
        case winrt::InkToolBarTool::BallpointPen:
        case winrt::InkToolBarTool::Pencil:
        case winrt::InkToolBarTool::Highlighter:
        case winrt::InkToolBarTool::CustomPen:
            proxy.InputProcessingConfiguration().Mode(winrt::Microsoft::UI::Xaml::Controls::InkInputProcessingMode::Inking);
            break;

        case winrt::InkToolBarTool::Eraser:
            proxy.InputProcessingConfiguration().Mode(winrt::Microsoft::UI::Xaml::Controls::InkInputProcessingMode::Erasing);
            break;

        default:
            break;
        }

        // Update drawing attributes from active pen button
        if (auto penButton = m_activeTool.try_as<winrt::InkToolBarPenButton>())
        {
            auto brush = penButton.SelectedBrush();
            auto strokeWidth = penButton.SelectedStrokeWidth();

            // Build base attributes appropriate to the tool kind. Copying the current
            // default attributes only carried color/width across, so Pencil and
            // Highlighter looked identical to the ballpoint pen. Create the correct
            // base per tool so Pencil produces pencil-kind ink and Highlighter produces
            // highlighter ink.
            winrt::InkDrawingAttributes attrs{ nullptr };
            if (toolKind == winrt::InkToolBarTool::Pencil)
            {
                attrs = winrt::InkDrawingAttributes::CreateForPencil();
            }
            else
            {
                attrs = winrt::InkDrawingAttributes();
                attrs.DrawAsHighlighter(toolKind == winrt::InkToolBarTool::Highlighter);
            }

            auto size = attrs.Size();
            size.Width = static_cast<float>(strokeWidth);
            size.Height = static_cast<float>(strokeWidth);
            attrs.Size(size);

            if (brush)
            {
                if (auto solidBrush = brush.try_as<winrt::Microsoft::UI::Xaml::Media::SolidColorBrush>())
                {
                    attrs.Color(solidBrush.Color());
                }
            }

            proxy.UpdateDefaultDrawingAttributes(attrs);

            // Notify subscribers that the drawing attributes pushed to the presenter changed.
            m_inkDrawingAttributesChangedEventSource(*this, nullptr);
        }
    }
}

void InkToolBar::OnToolButtonChecked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const&)
{
    if (auto toolButton = sender.try_as<winrt::InkToolBarToolButton>())
    {
        if (m_activeTool != toolButton)
        {
            m_activeTool = toolButton;
            OnActiveToolChanged();
        }
    }
}

void InkToolBar::PopulateVisualPanel()
{
    if (!m_toolButtonPanel)
    {
        return;
    }

    m_toolButtonPanel.Children().Clear();

    // Wire the real InkToolBarToolButton objects (RadioButton-derived) exactly once:
    // apply the self-contained tool-button template, build each tool's flyout, and
    // subscribe selection/flyout handlers. On later repopulations (template re-apply,
    // Loaded safety net) the buttons are only re-parented into the panel, preserving
    // their checked state and avoiding duplicate event handlers.
    const bool wire = !m_toolButtonsWired;
    auto toolButtonStyle = wire
        ? winrt::XamlReader::Load(winrt::hstring{ c_toolButtonStyleXaml }).try_as<winrt::Style>()
        : nullptr;
    auto glyphFont = winrt::Microsoft::UI::Xaml::Media::FontFamily(L"Segoe MDL2 Assets");

    for (size_t i = 0; i < m_toolButtons.size(); i++)
    {
        auto toolData = m_toolButtons[i];
        auto rb = toolData.try_as<winrt::RadioButton>();
        if (!rb)
        {
            continue;
        }

        if (wire)
        {
            auto kind = toolData.ToolKind();

            winrt::hstring label;
            winrt::hstring outlineGlyph;
            winrt::hstring fillGlyph;
            switch (kind)
            {
            case winrt::InkToolBarTool::BallpointPen: label = GetLabelOrFallback(SR_InkToolBarBallpointPenButtonLabel, L"Ballpoint pen"); outlineGlyph = L"\uE76D"; fillGlyph = L"\uE88F"; break;
            case winrt::InkToolBarTool::Pencil:       label = GetLabelOrFallback(SR_InkToolBarPencilButtonLabel,      L"Pencil");         outlineGlyph = L"\uED63"; fillGlyph = L"\uF0C6"; break;
            case winrt::InkToolBarTool::Highlighter:  label = GetLabelOrFallback(SR_InkToolBarHighlighterButtonLabel, L"Highlighter");    outlineGlyph = L"\uE7E6"; fillGlyph = L"\uE891"; break;
            case winrt::InkToolBarTool::Eraser:       label = GetLabelOrFallback(SR_InkToolBarEraserButtonLabel,      L"Eraser");         outlineGlyph = L"\uED60"; fillGlyph = L"";        break;
            default:                                  label = GetLabelOrFallback(SR_InkToolBarToolButtonLabel,        L"Tool");           outlineGlyph = L"\uED63"; fillGlyph = L"";        break;
            }

            auto pen = toolData.try_as<winrt::InkToolBarPenButton>();
            winrt::Brush fillBrush{ nullptr };
            if (pen)
            {
                fillBrush = pen.SelectedBrush();
            }

            // Layered Segoe MDL2 Assets glyph: colored fill behind, outline in front.
            winrt::Grid iconGrid;
            iconGrid.Width(24);
            iconGrid.Height(24);

            winrt::TextBlock fillTb{ nullptr };
            if (!fillGlyph.empty())
            {
                winrt::TextBlock fill;
                fill.FontFamily(glyphFont);
                fill.FontSize(18);
                fill.Text(fillGlyph);
                fill.HorizontalAlignment(winrt::HorizontalAlignment::Center);
                fill.VerticalAlignment(winrt::VerticalAlignment::Center);
                if (fillBrush)
                {
                    fill.Foreground(fillBrush);
                }
                iconGrid.Children().Append(fill);
                fillTb = fill;
            }

            winrt::TextBlock outline;
            outline.FontFamily(glyphFont);
            outline.FontSize(18);
            outline.Text(outlineGlyph);
            outline.HorizontalAlignment(winrt::HorizontalAlignment::Center);
            outline.VerticalAlignment(winrt::VerticalAlignment::Center);
            iconGrid.Children().Append(outline);

            // Turn the data button into a templated, exclusive 40x40 tool button.
            if (toolButtonStyle)
            {
                rb.Style(toolButtonStyle);
            }
            rb.GroupName(L"InkToolBarTools");
            rb.Content(iconGrid);
            winrt::ToolTipService::SetToolTip(rb, winrt::box_value(label));

            // Build the per-tool flyout: for pens a color palette + a live stroke-width
            // slider (WinUI 2 "pen configuration" strip); for the eraser a clear-all.
            winrt::FlyoutBase flyout{ nullptr };
            if (pen)
            {
                winrt::Flyout penFlyout;
                winrt::StackPanel rootPanel;
                rootPanel.Spacing(10);
                rootPanel.MinWidth(220);

                // Live stroke-width preview whose thickness and color track the pen.
                double minW = pen.MinStrokeWidth();
                double maxW = pen.MaxStrokeWidth();
                double curW = pen.SelectedStrokeWidth();
                if (maxW <= minW) { maxW = minW + 1.0; }
                if (curW < minW) { curW = minW; }
                if (curW > maxW) { curW = maxW; }

                winrt::Border previewHost;
                previewHost.Height(28);
                previewHost.VerticalAlignment(winrt::VerticalAlignment::Center);
                winrt::Rectangle previewBar;
                previewBar.RadiusX(4);
                previewBar.RadiusY(4);
                previewBar.Height(curW);
                previewBar.VerticalAlignment(winrt::VerticalAlignment::Center);
                previewBar.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
                if (auto initBrush = pen.SelectedBrush())
                {
                    previewBar.Fill(initBrush);
                }
                previewHost.Child(previewBar);

                // Color palette laid out 6 swatches per row (30 pen colors / 6 highlighter).
                winrt::StackPanel colorGrid;
                colorGrid.Spacing(4);
                winrt::StackPanel colorRow{ nullptr };
                const uint32_t kColumns = 6;
                if (auto palette = pen.Palette())
                {
                    for (uint32_t j = 0; j < palette.Size(); j++)
                    {
                        if (j % kColumns == 0)
                        {
                            colorRow = winrt::StackPanel();
                            colorRow.Orientation(winrt::Orientation::Horizontal);
                            colorRow.Spacing(4);
                            colorGrid.Children().Append(colorRow);
                        }

                        auto brush = palette.GetAt(j);
                        winrt::Button swatch;
                        swatch.Width(28);
                        swatch.Height(28);
                        swatch.MinWidth(0);
                        swatch.MinHeight(0);
                        swatch.Padding(winrt::ThicknessHelper::FromUniformLength(0));
                        swatch.Background(brush);

                        uint32_t idx = j;
                        auto penCopy = pen;
                        auto toolCopy = toolData;
                        auto fillCopy = fillTb;
                        auto brushCopy = brush;
                        auto previewCopy = previewBar;
                        swatch.Click([weakThis = get_weak(), penCopy, toolCopy, fillCopy, brushCopy, previewCopy, idx](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
                        {
                            penCopy.SelectedBrushIndex(static_cast<int32_t>(idx));
                            if (fillCopy)
                            {
                                fillCopy.Foreground(brushCopy);
                            }
                            if (previewCopy)
                            {
                                previewCopy.Fill(brushCopy);
                            }
                            // Flyout content lives in a Popup that can outlive the toolbar; re-acquire
                            // through a weak ref so a click after teardown is a safe no-op.
                            if (auto strong = weakThis.get())
                            {
                                if (strong->m_activeTool == toolCopy)
                                {
                                    strong->UpdateInkPresenterFromActiveTool();
                                }
                            }
                        });
                        colorRow.Children().Append(swatch);
                    }
                }
                rootPanel.Children().Append(colorGrid);

                // Continuous stroke-width slider (replaces Thin/Medium/Thick presets)
                // with the live preview above it.
                rootPanel.Children().Append(previewHost);

                winrt::TextBlock sizeLabel;
                sizeLabel.Text(winrt::hstring{ L"Size: " } + winrt::to_hstring(static_cast<int>(curW + 0.5)));
                rootPanel.Children().Append(sizeLabel);

                winrt::Slider widthSlider;
                widthSlider.Minimum(minW);
                widthSlider.Maximum(maxW);
                widthSlider.StepFrequency(1);
                widthSlider.Value(curW);
                {
                    auto penCopy = pen;
                    auto toolCopy = toolData;
                    auto previewCopy = previewBar;
                    auto labelCopy = sizeLabel;
                    widthSlider.ValueChanged([weakThis = get_weak(), penCopy, toolCopy, previewCopy, labelCopy](winrt::IInspectable const&, winrt::RangeBaseValueChangedEventArgs const& args)
                    {
                        double v = args.NewValue();
                        penCopy.SelectedStrokeWidth(v);
                        if (previewCopy)
                        {
                            previewCopy.Height(v);
                        }
                        labelCopy.Text(winrt::hstring{ L"Size: " } + winrt::to_hstring(static_cast<int>(v + 0.5)));
                        // Weak re-acquire: the slider lives in a flyout Popup that can outlive the toolbar.
                        if (auto strong = weakThis.get())
                        {
                            if (strong->m_activeTool == toolCopy)
                            {
                                strong->UpdateInkPresenterFromActiveTool();
                            }
                        }
                    });
                }
                rootPanel.Children().Append(widthSlider);

                penFlyout.Content(rootPanel);
                flyout = penFlyout;
            }
            else if (kind == winrt::InkToolBarTool::Eraser)
            {
                winrt::Flyout eraserFlyout;
                winrt::StackPanel eraserPanel;
                eraserPanel.Spacing(4);
                eraserPanel.MinWidth(160);

                // Honor the eraser button's visibility flags (matches WUXC InkToolBarEraserButton):
                // IsStrokeEraserVisible, ArePrecisionErasersVisible, IsClearAllVisible. Defaults true.
                auto eraserButton = toolData.try_as<winrt::InkToolBarEraserButton>();
                bool showStroke = !eraserButton || eraserButton.IsStrokeEraserVisible();
                bool showPrecision = !eraserButton || eraserButton.ArePrecisionErasersVisible();
                bool showClearAll = !eraserButton || eraserButton.IsClearAllVisible();

                // Adds a flyout item that selects an eraser kind: records SelectedEraser on the
                // button, switches the presenter to Erasing mode, and raises EraserFlyoutItemClicked
                // with the matching item kind.
                auto addEraserItem = [this, eraserButton, &eraserPanel](
                    winrt::hstring const& label,
                    winrt::InkToolBarEraserKind eraserKind,
                    winrt::InkToolBarEraserFlyoutItemKind itemKind)
                {
                    winrt::Button btn;
                    btn.Content(winrt::box_value(label));
                    btn.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
                    btn.Click([weakThis = get_weak(), eraserButton, eraserKind, itemKind](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
                    {
                        if (eraserButton)
                        {
                            eraserButton.SelectedEraser(eraserKind);
                        }
                        // Weak re-acquire: the flyout Popup can outlive the toolbar.
                        auto strong = weakThis.get();
                        if (!strong)
                        {
                            return;
                        }
                        if (strong->m_targetInkCanvas)
                        {
                            strong->m_targetInkCanvas.InkPresenter().InputProcessingConfiguration().Mode(
                                winrt::Microsoft::UI::Xaml::Controls::InkInputProcessingMode::Erasing);
                        }
                        auto args = winrt::make<InkToolBarEraserFlyoutItemClickedEventArgs>(itemKind);
                        strong->m_eraserFlyoutItemClickedEventSource(args, nullptr);
                    });
                    eraserPanel.Children().Append(btn);
                };

                // Stroke eraser: erases whole strokes on contact (presenter Erasing mode).
                if (showStroke)
                {
                    addEraserItem(L"Stroke eraser", winrt::InkToolBarEraserKind::Stroke,
                        winrt::InkToolBarEraserFlyoutItemKind::StrokeEraser);
                }

                // Precision erasers (small/large): same Erasing mode, distinguished by the raised
                // item kind + SelectedEraser (fine-grained tip sizing is a presenter feature not
                // yet surfaced through the marshaling proxy).
                if (showPrecision)
                {
                    addEraserItem(L"Precision eraser (small)", winrt::InkToolBarEraserKind::PrecisionSmall,
                        winrt::InkToolBarEraserFlyoutItemKind::PrecisionSmallEraser);
                    addEraserItem(L"Precision eraser (large)", winrt::InkToolBarEraserKind::PrecisionLarge,
                        winrt::InkToolBarEraserFlyoutItemKind::PrecisionLargeEraser);
                }

                // Clear all ink: wipes the stroke container and raises both the generic
                // EraseAllClicked event and the eraser-flyout-item event.
                if (showClearAll)
                {
                    winrt::Button clearBtn;
                    clearBtn.Content(winrt::box_value(winrt::hstring{ L"Clear all ink" }));
                    clearBtn.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
                    clearBtn.Click([weakThis = get_weak()](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
                    {
                        // Weak re-acquire: the flyout Popup can outlive the toolbar.
                        auto strong = weakThis.get();
                        if (!strong)
                        {
                            return;
                        }
                        if (strong->m_targetInkCanvas)
                        {
                            if (auto presenter = strong->m_targetInkCanvas.InkPresenter())
                            {
                                if (auto container = presenter.StrokeContainer())
                                {
                                    container.Clear();
                                }
                            }
                        }
                        strong->m_eraseAllClickedEventSource(*strong, nullptr);
                        auto args = winrt::make<InkToolBarEraserFlyoutItemClickedEventArgs>(
                            winrt::InkToolBarEraserFlyoutItemKind::ClearAll);
                        strong->m_eraserFlyoutItemClickedEventSource(args, nullptr);
                    });
                    eraserPanel.Children().Append(clearBtn);
                }

                eraserFlyout.Content(eraserPanel);
                flyout = eraserFlyout;
            }

            if (flyout)
            {
                flyout.Placement(ToFlyoutPlacementMode(m_buttonFlyoutPlacement));
                winrt::FlyoutBase::SetAttachedFlyout(rb, flyout);
            }

            // Set the initial selection BEFORE attaching handlers so the programmatic
            // check doesn't run the "click already-active tool" flyout path.
            bool makeActive = (m_activeTool && m_activeTool == toolData) || (!m_activeTool && i == 0);
            if (makeActive)
            {
                if (!m_activeTool)
                {
                    m_activeTool = toolData;
                }
                rb.IsChecked(true);
            }

            // GroupName keeps selection exclusive; Checked drives the active tool.
            auto toolCopy = toolData;
            auto justChecked = std::make_shared<bool>(false);
            rb.Checked([this, toolCopy, justChecked](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
            {
                *justChecked = true;
                if (m_activeTool != toolCopy)
                {
                    m_activeTool = toolCopy;
                    OnActiveToolChanged();
                }
            });

            // Clicking the already-active tool opens its flyout (WinUI 2 behavior);
            // a click that changes the selection is consumed by the Checked handler.
            auto flyoutCopy = flyout;
            auto rbCopy = rb;
            rb.Click([justChecked, flyoutCopy, rbCopy](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
            {
                if (*justChecked)
                {
                    *justChecked = false;
                    return;
                }
                if (flyoutCopy)
                {
                    flyoutCopy.ShowAt(rbCopy);
                }
            });
        }

        m_toolButtonPanel.Children().Append(rb);
    }

    // Ruler toggle (default non-pen control, matches the WinUI 2 default set). Built once
    // and re-parented on repopulate. A ToggleButton whose checked state mirrors
    // IsRulerButtonChecked and shows/hides the ruler stencil on the target canvas.
    if (m_includeRulerButton)
    {
        if (!m_rulerToggleButton)
        {
            winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton toggle;
            toggle.Width(40);
            toggle.Height(40);
            toggle.MinWidth(40);
            toggle.MinHeight(40);
            toggle.Padding(winrt::ThicknessHelper::FromUniformLength(0));

            winrt::TextBlock rulerGlyph;
            rulerGlyph.FontFamily(glyphFont);
            rulerGlyph.FontSize(18);
            rulerGlyph.Text(L"\uED5E"); // Ruler (Segoe MDL2 Assets)
            rulerGlyph.HorizontalAlignment(winrt::HorizontalAlignment::Center);
            rulerGlyph.VerticalAlignment(winrt::VerticalAlignment::Center);
            toggle.Content(rulerGlyph);

            winrt::ToolTipService::SetToolTip(toggle, winrt::box_value(GetLabelOrFallback(L"InkToolBarRulerButtonLabel", L"Ruler")));
            toggle.IsChecked(m_isRulerButtonChecked);

            toggle.Checked([this](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
            {
                if (!m_syncingRulerToggle) { IsRulerButtonChecked(true); }
            });
            toggle.Unchecked([this](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
            {
                if (!m_syncingRulerToggle) { IsRulerButtonChecked(false); }
            });

            m_rulerToggleButton = toggle;
        }
        m_toolButtonPanel.Children().Append(m_rulerToggleButton);
    }

    // Stencil menu button (ruler / protractor). Built once and re-parented on repopulate.
    // Toggling shows/hides the selected stencil; clicking the already-checked button opens a
    // flyout to switch between ruler and protractor.
    if (m_includeStencilButton)
    {
        if (!m_stencilMenuButton)
        {
            winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton toggle;
            toggle.Width(40);
            toggle.Height(40);
            toggle.MinWidth(40);
            toggle.MinHeight(40);
            toggle.Padding(winrt::ThicknessHelper::FromUniformLength(0));

            winrt::TextBlock stencilGlyph;
            stencilGlyph.FontFamily(glyphFont);
            stencilGlyph.FontSize(18);
            stencilGlyph.Text(L"\uED5F"); // Protractor (Segoe MDL2 Assets)
            stencilGlyph.HorizontalAlignment(winrt::HorizontalAlignment::Center);
            stencilGlyph.VerticalAlignment(winrt::VerticalAlignment::Center);
            toggle.Content(stencilGlyph);

            winrt::ToolTipService::SetToolTip(toggle, winrt::box_value(GetLabelOrFallback(L"InkToolBarStencilButtonLabel", L"Stencil")));
            toggle.IsChecked(m_isStencilButtonChecked);

            // Flyout: pick ruler vs protractor.
            winrt::Flyout stencilFlyout;
            winrt::StackPanel stencilPanel;
            stencilPanel.Spacing(4);
            stencilPanel.MinWidth(140);

            auto currentKind = m_stencilButton ? m_stencilButton.SelectedStencil() : winrt::InkToolBarStencilKind::Ruler;

            winrt::RadioButton rulerItem;
            rulerItem.Content(winrt::box_value(winrt::hstring{ L"Ruler" }));
            rulerItem.GroupName(L"InkToolBarStencil");
            rulerItem.IsChecked(currentKind == winrt::InkToolBarStencilKind::Ruler);
            rulerItem.Checked([weakThis = get_weak()](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
            {
                if (auto strong = weakThis.get())
                {
                    if (strong->m_stencilButton) { strong->m_stencilButton.SelectedStencil(winrt::InkToolBarStencilKind::Ruler); }
                    strong->ApplyRulerState();
                }
            });
            stencilPanel.Children().Append(rulerItem);

            winrt::RadioButton protractorItem;
            protractorItem.Content(winrt::box_value(winrt::hstring{ L"Protractor" }));
            protractorItem.GroupName(L"InkToolBarStencil");
            protractorItem.IsChecked(currentKind == winrt::InkToolBarStencilKind::Protractor);
            protractorItem.Checked([weakThis = get_weak()](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
            {
                if (auto strong = weakThis.get())
                {
                    if (strong->m_stencilButton) { strong->m_stencilButton.SelectedStencil(winrt::InkToolBarStencilKind::Protractor); }
                    strong->ApplyRulerState();
                }
            });
            stencilPanel.Children().Append(protractorItem);

            stencilFlyout.Content(stencilPanel);
            stencilFlyout.Placement(ToFlyoutPlacementMode(m_buttonFlyoutPlacement));
            winrt::FlyoutBase::SetAttachedFlyout(toggle, stencilFlyout);

            auto justChecked = std::make_shared<bool>(false);
            toggle.Checked([this, justChecked](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
            {
                *justChecked = true;
                if (!m_syncingStencilToggle) { IsStencilButtonChecked(true); }
            });
            toggle.Unchecked([this](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
            {
                if (!m_syncingStencilToggle) { IsStencilButtonChecked(false); }
            });
            auto toggleCopy = toggle;
            auto stencilFlyoutCopy = stencilFlyout;
            toggle.Click([justChecked, stencilFlyoutCopy, toggleCopy](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
            {
                // A click that just toggled the check state is consumed here; a click on the
                // already-checked button opens the ruler/protractor flyout.
                if (*justChecked) { *justChecked = false; return; }
                stencilFlyoutCopy.ShowAt(toggleCopy);
            });

            m_stencilMenuButton = toggle;
        }
        m_toolButtonPanel.Children().Append(m_stencilMenuButton);
    }

    m_toolButtonsWired = true;

    INKTOOLBAR_TRACE_INFO(*this, L"InkToolBar::PopulateVisualPanel added %d tool buttons\n",
        static_cast<int>(m_toolButtons.size()));
}

void InkToolBar::ApplyRulerState()
{
    // Keep the ruler toggle visual in sync (guard against re-entrancy from its own
    // Checked/Unchecked handlers).
    if (m_rulerToggleButton)
    {
        m_syncingRulerToggle = true;
        m_rulerToggleButton.IsChecked(m_isRulerButtonChecked);
        m_syncingRulerToggle = false;
    }

    // Keep the stencil menu visual in sync.
    if (m_stencilMenuButton)
    {
        m_syncingStencilToggle = true;
        m_stencilMenuButton.IsChecked(m_isStencilButtonChecked);
        m_syncingStencilToggle = false;
    }

    if (!m_targetInkCanvas)
    {
        return;
    }

    // Single source of truth for the two stencils so the legacy ruler toggle and the stencil
    // menu button don't fight over the canvas. The ruler is shown when either the ruler toggle
    // is checked or the stencil is checked with Ruler selected; the protractor only when the
    // stencil is checked with Protractor selected. The InkPresenter proxy marshals each toggle
    // to the ink thread, where the thread-affine InkPresenterRuler / InkPresenterProtractor live.
    auto stencilKind = m_stencilButton ? m_stencilButton.SelectedStencil() : winrt::InkToolBarStencilKind::Ruler;
    bool rulerOn = m_isRulerButtonChecked ||
        (m_isStencilButtonChecked && stencilKind == winrt::InkToolBarStencilKind::Ruler);
    bool protractorOn = m_isStencilButtonChecked && stencilKind == winrt::InkToolBarStencilKind::Protractor;

    auto presenter = winrt::get_self<::InkPresenter>(m_targetInkCanvas.InkPresenter());
    presenter->SetRulerEnabled(rulerOn);
    presenter->SetProtractorEnabled(protractorOn);
}
