// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// ============================================================================================
// InkToolbar container - faithful C++/WinRT translation of the UWP InkToolbar
// (onecoreuap\...\inkcontrols\lib\InkToolbar_Partial.cpp). The portable orchestration is ported
// 1:1: auto-population and ordering, L2/L3 flyouts, active-tool and drawing-attribute wiring,
// ruler/stencil state, keyboard navigation, and InkPresenter integration.
//
// OS-internal capabilities that are not on the WinUI 3 lift surface are stubbed and documented
// rather than reinvented: Surface Dial / ExternalActor, ICoreInkIndependentInputSource
// (independent input), dynamic stroke width (IInkPointDrawingAttributesStatics),
// ITestableInkToolbar, IInkToolbarInternal, and telemetry. Stencils use InkPresenterRuler /
// InkPresenterProtractor directly (there is no InkPresenterStencil base in the lift). OnLoaded
// (no-op) and DeactivateMenuButton (no callers) are dropped as unreachable in the lift.
// ============================================================================================

#include "pch.h"
#include "common.h"
#include "InkToolbar.h"
#include "ButtonManager.h"
#include "InkToolbarToolButton.h"
#include "InkToolbarPenButton.h"
#include "InkToolbarToggleButton.h"
#include "InkToolbarMenuButton.h"
#include "InkToolbarEraserButton.h"
#include "InkToolbarStencilButton.h"
#include "InkToolbarPenConfigurationControl.h"
#include "InkToolbarBallpointPenButton.h"
#include "InkToolbarPencilButton.h"
#include "InkToolbarHighlighterButton.h"
#include "InkToolbarAutomationPeer.h"
#include "InkCanvas.h"
#include "InkPresenter.h"
#include "InkToolbarIsStencilButtonCheckedChangedEventArgs.h"
#include "InkToolbarEraserFlyoutItemClickedEventArgs.h"

namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;
namespace muxcp = winrt::Microsoft::UI::Xaml::Controls::Primitives;

// ---- Construction / init (faithful port of UWP InitializeImpl @102; dial/high-contrast/dispatcher dropped) ----

InkToolbar::InkToolbar()
{
    // Apply the control's default style (Style TargetType="InkToolbar" in generic.xaml).
    SetDefaultStyleKey(this);

    // Create the Children collection and observe changes to it.
    auto children = winrt::DependencyObjectCollection();
    m_childrenChangedToken = children.VectorChanged({ get_weak(), &InkToolbar::OnChildrenChanged });
    Children(children);

    // ButtonManager owns the button model; lifetime tied to this container.
    m_buttonManager = std::make_unique<ButtonManager>(get_weak());
}

// ---- Auto-population + ordering (faithful port of UWP @1943 / OrderChildren) ----------------

void InkToolbar::PerformAutoPopulation()
{
    auto added = m_buttonManager->AutoPopulate(InitialControls());
    for (auto const& child : added)
    {
        ConfigureAddedChild(child);
        // Root the auto-populated button so it survives even before the template's Panel adopts it.
        // (ButtonManager stores weak_ref; without this, GetToolButton before layout - or a control
        // with no default style applied - would find the buttons already collected.)
        m_autoPopulatedButtons.push_back(child);
    }
}

std::vector<winrt::UIElement> InkToolbar::OrderChildren()
{
    std::vector<winrt::UIElement> orderedChildren;

    // System pens, then app (custom) pens.
    for (auto const& item : m_buttonManager->GetSystemPenButtons()) { orderedChildren.push_back(item.as<winrt::UIElement>()); }
    for (auto const& item : m_buttonManager->GetCustomPenButtons()) { orderedChildren.push_back(item.as<winrt::UIElement>()); }

    // Eraser.
    if (auto eraserButton = m_buttonManager->GetEraserButton()) { orderedChildren.push_back(eraserButton.as<winrt::UIElement>()); }

    // Custom tools.
    for (auto const& item : m_buttonManager->GetCustomToolButtons()) { orderedChildren.push_back(item.as<winrt::UIElement>()); }

    // Stencil (modern path; RS2 ruler-only path is not used). Only add if a stencil is selectable.
    if (auto stencilButton = m_buttonManager->GetStencilButton())
    {
        if (winrt::get_self<InkToolbarStencilButton>(stencilButton)->NumberOfStencils() > 0)
        {
            orderedChildren.push_back(stencilButton.as<winrt::UIElement>());
        }
    }

    // Custom toggles.
    for (auto const& item : m_buttonManager->GetCustomToggleButtons()) { orderedChildren.push_back(item.as<winrt::UIElement>()); }

    return orderedChildren;
}

// ---- OnPropertyChanged dispatch (faithful port of UWP @450) ---------------------------------

void InkToolbar::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto property = args.Property();

    if (property == winrt::InkToolbar::ActiveToolProperty()) { OnActiveToolChanged(args); }
    else if (property == winrt::InkToolbar::InkDrawingAttributesProperty()) { OnInkDrawingAttributesChanged(args); }
    else if (property == winrt::InkToolbar::IsRulerButtonCheckedProperty()) { OnIsRulerButtonCheckedChanged(args); }
    else if (property == winrt::InkToolbar::IsStencilButtonCheckedProperty()) { OnIsStencilButtonCheckedChanged(args); }
    else if (property == winrt::InkToolbar::TargetInkCanvasProperty()) { OnTargetInkCanvasChanged(args); }
    else if (property == winrt::InkToolbar::TargetInkPresenterProperty()) { OnTargetInkPresenterChanged(args); }
    else if (property == winrt::InkToolbar::ButtonFlyoutPlacementProperty()) { OnButtonFlyoutPlacementChanged(args); }
    else if (property == winrt::InkToolbar::OrientationProperty()) { OnOrientationChanged(args); }
    // Children / InitialControls: no-op.
}

// ---- Pen L3 got-focus callbacks (UWP set the Surface Dial mode here; dial unavailable -> no-op) ----

void InkToolbar::OnPenL3ColorPickerGotFocus()
{
    // Documented no-op: UWP set ExternalActor (Surface Dial) mode to InkColor. Focus is handled by the
    // PenConfigurationControl itself; dial is not available in the lift.
}

void InkToolbar::OnPenL3StrokeWidthGotFocus()
{
    // Documented no-op (see OnPenL3ColorPickerGotFocus).
}

// ---- Static button check helpers (faithful port of UWP @5583-5720) -------------------------

void InkToolbar::SetButtonCheck(winrt::InkToolbarToolButton const& button, bool check)
{
    SetButtonCheck(button.as<muxcp::ToggleButton>(), check);
}

void InkToolbar::SetButtonCheck(winrt::InkToolbarToggleButton const& button, bool check)
{
    SetButtonCheck(button.as<muxcp::ToggleButton>(), check);
}

// InkToolbarMenuButton is a tri-state toggle button; map the checked-state enum onto IsChecked.
void InkToolbar::SetButtonCheck(winrt::InkToolbarMenuButton const& button, InkToolbarMenuButtonCheckedState check)
{
    auto toggleButton = button.as<muxcp::ToggleButton>();

    switch (check)
    {
    case InkToolbarMenuButtonCheckedState::Unchecked:
        SetButtonCheck(toggleButton, false);
        break;
    case InkToolbarMenuButtonCheckedState::Indeterminate:
        toggleButton.IsChecked(nullptr);
        break;
    case InkToolbarMenuButtonCheckedState::Checked:
        SetButtonCheck(toggleButton, true);
        break;
    default:
        throw winrt::hresult_error(E_UNEXPECTED, L"Unexpected InkToolbarMenuButtonCheckedState");
    }
}

void InkToolbar::SetButtonCheck(muxcp::ToggleButton const& button, bool check)
{
    button.IsChecked(check);
}

bool InkToolbar::IsButtonChecked(winrt::InkToolbarToolButton const& button)
{
    return IsButtonChecked(button.as<muxcp::ToggleButton>());
}

bool InkToolbar::IsButtonChecked(winrt::InkToolbarToggleButton const& button)
{
    return IsButtonChecked(button.as<muxcp::ToggleButton>());
}

InkToolbarMenuButtonCheckedState InkToolbar::IsButtonChecked(winrt::InkToolbarMenuButton const& button)
{
    auto isCheckedReference = button.as<muxcp::ToggleButton>().IsChecked();

    if (!isCheckedReference)
    {
        return InkToolbarMenuButtonCheckedState::Indeterminate;
    }

    return isCheckedReference.Value() ? InkToolbarMenuButtonCheckedState::Checked : InkToolbarMenuButtonCheckedState::Unchecked;
}

bool InkToolbar::IsButtonChecked(muxcp::ToggleButton const& button)
{
    auto reference = button.IsChecked();
    return reference && reference.Value();
}

// ---- File-local helpers --------------------------------------------------------------------

namespace
{
    // Depth-first search of a subtree for the first InkToolbarPenConfigurationControl (a custom pen L3
    // may nest one inside its own content).
    winrt::InkToolbarPenConfigurationControl FindFirstPenConfigurationControl(winrt::DependencyObject const& root)
    {
        if (!root)
        {
            return nullptr;
        }
        if (auto penL3 = root.try_as<winrt::InkToolbarPenConfigurationControl>())
        {
            return penL3;
        }
        int count = winrt::VisualTreeHelper::GetChildrenCount(root);
        for (int i = 0; i < count; ++i)
        {
            if (auto found = FindFirstPenConfigurationControl(winrt::VisualTreeHelper::GetChild(root, i)))
            {
                return found;
            }
        }
        return nullptr;
    }
}

// ---- L3 flyout tracking (faithful port of UWP @2283-2433) -----------------------------------

void InkToolbar::DismissL3(winrt::InkToolbarPenConfigurationControl const& penL3)
{
    auto found = std::find_if(m_openFlyouts.cbegin(), m_openFlyouts.cend(),
        [&](OpenFlyout const& flyout) { return flyout.m_penL3 && flyout.m_penL3 == penL3; });

    if (m_openFlyouts.cend() != found)
    {
        found->m_flyout.Hide();
    }
}

bool InkToolbar::IsL3Open(winrt::InkToolbarToolButton const& toolButton)
{
    return std::find_if(m_openFlyouts.cbegin(), m_openFlyouts.cend(),
        [&](OpenFlyout const& flyout) { return flyout.m_toolButton && flyout.m_toolButton == toolButton; }) != m_openFlyouts.cend();
}

void InkToolbar::OpenL3(winrt::InkToolbarToolButton const& toolButton)
{
    // Treat as a right-click action so we get an implicit selection to go with the open.
    ExecuteToolAction(toolButton, false);
}

void InkToolbar::CloseL3(winrt::InkToolbarToolButton const& toolButton)
{
    auto found = std::find_if(m_openFlyouts.cbegin(), m_openFlyouts.cend(),
        [&](OpenFlyout const& flyout) { return flyout.m_toolButton && flyout.m_toolButton == toolButton; });
    if (found != m_openFlyouts.cend())
    {
        found->m_flyout.Hide();
    }
}

bool InkToolbar::IsL3Open(winrt::InkToolbarMenuButton const& menuButton)
{
    return std::find_if(m_openFlyouts.cbegin(), m_openFlyouts.cend(),
        [&](OpenFlyout const& flyout) { return flyout.m_menuButton && flyout.m_menuButton == menuButton; }) != m_openFlyouts.cend();
}

void InkToolbar::OpenL3(winrt::InkToolbarMenuButton const& menuButton)
{
    ExecuteMenuAction(menuButton, false);
}

void InkToolbar::CloseL3(winrt::InkToolbarMenuButton const& menuButton)
{
    auto found = std::find_if(m_openFlyouts.cbegin(), m_openFlyouts.cend(),
        [&](OpenFlyout const& flyout) { return flyout.m_menuButton && flyout.m_menuButton == menuButton; });
    if (found != m_openFlyouts.cend())
    {
        found->m_flyout.Hide();
    }
}

// ---- ExecuteToolAction (faithful port of UWP @2433; telemetry + dial mode dropped) ----------

void InkToolbar::ExecuteToolAction(winrt::InkToolbarToolButton const& toolButton, bool isNormalActivation)
{
    // If the active tool changed via left click, set it and we're done. If it changed via right-click,
    // set it and show the flyout. If it didn't change, show the flyout.
    if (toolButton != ActiveTool())
    {
        ActiveTool(toolButton);

        // If the new active tool is the eraser, hide the extension glyph when it has no L3 to show.
        if (auto eraserButton = toolButton.try_as<winrt::InkToolbarEraserButton>())
        {
            if (!winrt::get_self<InkToolbarEraserButton>(eraserButton)->ShouldShowL3())
            {
                toolButton.IsExtensionGlyphShown(false);
            }
        }

        if (isNormalActivation)
        {
            // Left press changed the active tool; do not open L3.
            return;
        }
    }

    // Already-active tool, or right-press changed the active tool: open the L3.
    auto flyoutBase = winrt::FlyoutBase::GetAttachedFlyout(toolButton);
    auto flyout = flyoutBase ? flyoutBase.try_as<winrt::Flyout>() : nullptr;
    if (!flyout)
    {
        return;   // No flyout associated with the button.
    }

    if (m_inking)
    {
        return;   // Mid-stroke; don't show L3.
    }

    auto penButton = toolButton.try_as<winrt::InkToolbarPenButton>();
    winrt::InkToolbarPenConfigurationControl penL3{ nullptr };

    if (penButton)
    {
        // Custom pen buttons supply their own configuration content.
        if (auto appPenButton = toolButton.try_as<winrt::InkToolbarCustomPenButton>())
        {
            auto configContent = appPenButton.ConfigurationContent();
            if (configContent)
            {
                flyout.Content(configContent);
            }
            else
            {
                return;   // App pen button has no config content; don't open L3.
            }
        }

        auto flyoutContent = flyout.Content();
        winrt::InkToolbarPenConfigurationControl penL3Temp{ nullptr };
        if (flyoutContent)
        {
            penL3Temp = flyoutContent.try_as<winrt::InkToolbarPenConfigurationControl>();
            if (!penL3Temp)
            {
                penL3Temp = FindFirstPenConfigurationControl(flyoutContent.try_as<winrt::DependencyObject>());
            }
            if (penL3Temp)
            {
                penL3 = penL3Temp;
                winrt::get_self<InkToolbarPenConfigurationControl>(penL3)->SetBindingData(*this, toolButton);
            }
        }

        if (!toolButton.try_as<winrt::InkToolbarCustomPenButton>() && !penL3Temp)
        {
            // No penL3 found for a system pen; create one.
            penL3 = winrt::make<InkToolbarPenConfigurationControl>();
            flyout.Content(penL3);
            winrt::get_self<InkToolbarPenConfigurationControl>(penL3)->SetBindingData(*this, toolButton);
        }

        toolButton.IsExtensionGlyphShown(false);
    }
    else
    {
        switch (toolButton.ToolKind())
        {
        case winrt::InkToolbarTool::Eraser:
        {
            auto eraserButton = toolButton.as<winrt::InkToolbarEraserButton>();
            auto eraserImpl = winrt::get_self<InkToolbarEraserButton>(eraserButton);
            if (!eraserImpl->ShouldShowL3())
            {
                toolButton.IsExtensionGlyphShown(false);
                return;   // Eraser flyout content is empty; don't open L3.
            }

            m_eraserFlyout = flyoutBase;
            eraserImpl->HookUpToEraserEvents(
                winrt::RoutedEventHandler([this](winrt::IInspectable const& s, winrt::RoutedEventArgs const& e) { OnEraserL3ItemsClicked(s, e); }),
                m_hookedUpToEraserEvents,
                m_eraserEventToken);

            // When the eraser L3 opens, one eraser item should be checked.
            eraserImpl->SetL3EraserItemCheck(eraserButton.SelectedEraser(), true);
            toolButton.IsExtensionGlyphShown(false);
            break;
        }

        case winrt::InkToolbarTool::CustomTool:
        {
            auto customToolButton = toolButton.as<winrt::InkToolbarCustomToolButton>();
            auto configContent = customToolButton.ConfigurationContent();
            if (configContent)
            {
                flyout.Content(configContent);
            }
            else
            {
                return;   // Custom tool button has no config content; don't open L3.
            }
            toolButton.IsExtensionGlyphShown(false);
            break;
        }

        default:
            break;
        }
    }

    OpenFlyout openFlyout;
    openFlyout.m_flyout = flyoutBase;
    openFlyout.m_toolButton = toolButton;
    openFlyout.m_penL3 = penL3;

    openFlyout.m_closedRegistrationToken = flyoutBase.Closed(
        winrt::Windows::Foundation::EventHandler<winrt::IInspectable>([this](winrt::IInspectable const& s, winrt::IInspectable const& e) { OnFlyoutClosed(s, e); }));
    openFlyout.m_openedRegistrationToken = flyoutBase.Opened(
        winrt::Windows::Foundation::EventHandler<winrt::IInspectable>([this](winrt::IInspectable const& s, winrt::IInspectable const& e) { OnFlyoutOpened(s, e); }));

    m_openFlyouts.push_back(openFlyout);

    winrt::FlyoutPlacementMode flyoutPlacement;
    switch (ButtonFlyoutPlacement())
    {
    case winrt::InkToolbarButtonFlyoutPlacement::Auto:
    case winrt::InkToolbarButtonFlyoutPlacement::Bottom:
        flyoutPlacement = winrt::FlyoutPlacementMode::Bottom;
        break;
    case winrt::InkToolbarButtonFlyoutPlacement::Top:
        flyoutPlacement = winrt::FlyoutPlacementMode::Top;
        break;
    case winrt::InkToolbarButtonFlyoutPlacement::Left:
        flyoutPlacement = winrt::FlyoutPlacementMode::Left;
        break;
    case winrt::InkToolbarButtonFlyoutPlacement::Right:
        flyoutPlacement = winrt::FlyoutPlacementMode::Right;
        break;
    default:
        throw winrt::hresult_invalid_argument(L"ExecuteToolAction: Unexpected InkToolbarButtonFlyoutPlacement");
    }

    flyoutBase.Placement(flyoutPlacement);
    flyoutBase.ShowAt(toolButton);
}

// ---- Stencil kind conversion (faithful port) -----------------------------------------------

winrt::Windows::UI::Input::Inking::InkPresenterStencilKind InkToolbar::InkToolbarStencilKindToInkPresenterStencilKind(winrt::InkToolbarStencilKind kind)
{
    switch (kind)
    {
    case winrt::InkToolbarStencilKind::Protractor:
        return winrt::Windows::UI::Input::Inking::InkPresenterStencilKind::Protractor;
    case winrt::InkToolbarStencilKind::Ruler:
    default:
        return winrt::Windows::UI::Input::Inking::InkPresenterStencilKind::Ruler;
    }
}

winrt::InkToolbarStencilKind InkToolbar::InkPresenterStencilKindToInkToolbarStencilKind(winrt::Windows::UI::Input::Inking::InkPresenterStencilKind kind)
{
    switch (kind)
    {
    case winrt::Windows::UI::Input::Inking::InkPresenterStencilKind::Protractor:
        return winrt::InkToolbarStencilKind::Protractor;
    case winrt::Windows::UI::Input::Inking::InkPresenterStencilKind::Ruler:
    default:
        return winrt::InkToolbarStencilKind::Ruler;
    }
}

// Which stencil is currently visible on the target InkPresenter (defaults to Ruler).
winrt::Windows::UI::Input::Inking::InkPresenterStencilKind InkToolbar::GetShowingStencilKind()
{
    // Read the tracked on-canvas state (the Ruler/Protractor DPs are unused under the lift).
    if (m_protractorVisible)
    {
        return winrt::Windows::UI::Input::Inking::InkPresenterStencilKind::Protractor;
    }
    return winrt::Windows::UI::Input::Inking::InkPresenterStencilKind::Ruler;
}

// ---- ExecuteMenuAction (faithful port of UWP @2698; telemetry dropped) ----------------------

void InkToolbar::ExecuteMenuAction(winrt::InkToolbarMenuButton const& menuButton, bool isNormalActivation)
{
    auto kind = menuButton.MenuKind();

    auto menuAsStencil = menuButton.try_as<winrt::InkToolbarStencilButton>();
    InkToolbarStencilButton* stencilButtonConcrete = menuAsStencil ? winrt::get_self<InkToolbarStencilButton>(menuAsStencil) : nullptr;

    auto menuAsToggle = menuButton.as<muxcp::ToggleButton>();
    bool toggleMode = false;

    if (kind == winrt::InkToolbarMenuKind::Stencil && stencilButtonConcrete)
    {
        toggleMode = (stencilButtonConcrete->NumberOfStencils() == 1);

        if (toggleMode)
        {
            // Enter "toggle" mode: two-state button, no flyout. Manage the checked state ourselves.
            if (!m_configuredStencilButtonIntoToggleMode)
            {
                m_configuredStencilButtonIntoToggleMode = true;
                menuAsToggle.IsThreeState(false);
            }

            switch (IsButtonChecked(menuButton))
            {
            case InkToolbarMenuButtonCheckedState::Unchecked:
                SetButtonCheck(menuButton, InkToolbarMenuButtonCheckedState::Checked);
                break;
            case InkToolbarMenuButtonCheckedState::Checked:
                SetButtonCheck(menuButton, InkToolbarMenuButtonCheckedState::Unchecked);
                break;
            default:
                break;
            }

            menuButton.IsExtensionGlyphShown(false);   // Always hide the extension glyph in toggle mode.
            return;
        }
        else
        {
            // Turn off toggle mode.
            m_configuredStencilButtonIntoToggleMode = false;
            menuAsToggle.IsThreeState(true);
        }
    }

    // The toggle action is no-op'ed on the menu button, so set the checked state ourselves.
    if (InkToolbarMenuButtonCheckedState::Unchecked == IsButtonChecked(menuButton))
    {
        SetButtonCheck(menuButton, InkToolbarMenuButtonCheckedState::Checked);

        if (kind == winrt::InkToolbarMenuKind::Stencil)
        {
            menuButton.IsExtensionGlyphShown(!toggleMode);
        }

        if (isNormalActivation)
        {
            return;   // Left press selected the menu button; don't open L3.
        }
    }

    // Already-active menu button, or right-press selected it: open the L3.
    auto flyoutBase = winrt::FlyoutBase::GetAttachedFlyout(menuButton);
    auto flyout = flyoutBase ? flyoutBase.try_as<winrt::Flyout>() : nullptr;
    if (!flyout)
    {
        return;
    }

    if (m_inking)
    {
        return;
    }

    switch (kind)
    {
    case winrt::InkToolbarMenuKind::Stencil:
        m_stencilFlyout = flyoutBase;
        stencilButtonConcrete->HookUpToStencilEvents(
            winrt::RoutedEventHandler([this](winrt::IInspectable const& s, winrt::RoutedEventArgs const& e) { OnStencilL3ItemsClicked(s, e); }),
            m_hookedUpToStencilEvents,
            m_stencilEventToken);

        // When the L3 opens, check the item for the currently-visible stencil.
        stencilButtonConcrete->SetL3StencilItemCheck(InkPresenterStencilKindToInkToolbarStencilKind(GetShowingStencilKind()), true);
        menuButton.IsExtensionGlyphShown(false);
        break;

    default:
        throw winrt::hresult_invalid_argument(L"ExecuteMenuAction: Unexpected menu button kind");
    }

    OpenFlyout openFlyout;
    openFlyout.m_flyout = flyoutBase;
    openFlyout.m_menuButton = menuButton;

    openFlyout.m_closedRegistrationToken = flyoutBase.Closed(
        winrt::Windows::Foundation::EventHandler<winrt::IInspectable>([this](winrt::IInspectable const& s, winrt::IInspectable const& e) { OnFlyoutClosed(s, e); }));
    openFlyout.m_openedRegistrationToken = flyoutBase.Opened(
        winrt::Windows::Foundation::EventHandler<winrt::IInspectable>([this](winrt::IInspectable const& s, winrt::IInspectable const& e) { OnFlyoutOpened(s, e); }));

    m_openFlyouts.push_back(openFlyout);

    flyoutBase.ShowAt(menuButton);
}

// ---- ExecuteToggleAction (faithful port of UWP @2922; telemetry dropped) --------------------

void InkToolbar::ExecuteToggleAction(winrt::InkToolbarToggleButton const& button)
{
    if (button.ToggleKind() == winrt::InkToolbarToggle::Ruler)
    {
        IsRulerButtonChecked(IsButtonChecked(button));
    }
}

// ---- Projected query methods (delegate to ButtonManager) ------------------------------------

winrt::InkToolbarToolButton InkToolbar::GetToolButton(winrt::InkToolbarTool tool)
{
    return m_buttonManager ? m_buttonManager->GetToolButton(tool) : nullptr;
}

winrt::InkToolbarToggleButton InkToolbar::GetToggleButton(winrt::InkToolbarToggle tool)
{
    return m_buttonManager ? m_buttonManager->GetToggleButton(tool) : nullptr;
}

winrt::InkToolbarMenuButton InkToolbar::GetMenuButton(winrt::InkToolbarMenuKind menu)
{
    return m_buttonManager ? m_buttonManager->GetMenuButton(menu) : nullptr;
}

// ---- OnCheckStateChanged (faithful port of UWP @2947; modern path, RS2 quirk = false) -------

void InkToolbar::OnCheckStateChanged(winrt::UIElement const& child, bool checked)
{
    // InkToolbarMenuButton inherits ToggleButton, which does NOT go to the unchecked visual state on its
    // own; do it manually.
    if (auto menuButton = child.try_as<winrt::InkToolbarMenuButton>())
    {
        winrt::get_self<InkToolbarMenuButton>(menuButton)->UpdateStates(false);

        if (auto stencilButton = child.try_as<winrt::InkToolbarStencilButton>())
        {
            IsStencilButtonChecked(checked);
            bool shouldShowExtensionGlyph =
                (winrt::get_self<InkToolbarStencilButton>(stencilButton)->NumberOfStencils() > 1) && checked;
            menuButton.IsExtensionGlyphShown(shouldShowExtensionGlyph);
        }
    }

    if (auto toolButton = child.try_as<winrt::InkToolbarToolButton>())
    {
        if (IsButtonChecked(toolButton))
        {
            // A programmatically-checked tool button becomes the active tool (glyph updated in OnActiveToolChanged).
            if (ActiveTool() != toolButton)
            {
                ActiveTool(toolButton);
            }
        }
    }
}

// ---- Flyout open/close bookkeeping (faithful port; telemetry + dial dropped) -----------------

void InkToolbar::OnFlyoutClosed(winrt::IInspectable const& sender, winrt::IInspectable const& args)
{
    UNREFERENCED_PARAMETER(args);

    auto flyout = sender.try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::FlyoutBase>();
    if (!flyout)
    {
        return;
    }

    auto found = std::find_if(m_openFlyouts.cbegin(), m_openFlyouts.cend(),
        [&](OpenFlyout const& of) { return of.m_flyout && of.m_flyout == flyout; });
    if (found == m_openFlyouts.cend())
    {
        return;   // Unknown flyout.
    }

    flyout.Closed(found->m_closedRegistrationToken);
    flyout.Opened(found->m_openedRegistrationToken);

    if (found->m_toolButton)
    {
        // UWP: refresh the tool button's checked state + extension glyph now that its L3 has closed.
        // (The telemetry UWP logs on this path is a documented lift gap.)
        UpdateToolButtonVisuals(found->m_toolButton, ActiveTool());
    }
    else if (found->m_menuButton)
    {
        if (auto menuAsStencil = found->m_menuButton.try_as<winrt::InkToolbarStencilButton>())
        {
            auto stencilImpl = winrt::get_self<InkToolbarStencilButton>(menuAsStencil);
            if (stencilImpl->IsAnyStencilSelected())
            {
                bool shouldShowExtensionGlyph =
                    (stencilImpl->NumberOfStencils() > 1) &&
                    (IsButtonChecked(found->m_menuButton) == InkToolbarMenuButtonCheckedState::Checked);
                found->m_menuButton.IsExtensionGlyphShown(shouldShowExtensionGlyph);
            }
            else
            {
                found->m_menuButton.IsExtensionGlyphShown(false);
                SetButtonCheck(found->m_menuButton, InkToolbarMenuButtonCheckedState::Unchecked);
            }
        }
    }

    m_openFlyouts.erase(found);
}

// When an L3 opens, move focus into the relevant control (pen color / eraser / stencil selection).
void InkToolbar::OnFlyoutOpened(winrt::IInspectable const& sender, winrt::IInspectable const& args)
{
    UNREFERENCED_PARAMETER(args);

    auto flyout = sender.try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::FlyoutBase>();
    if (!flyout)
    {
        return;
    }

    auto found = std::find_if(m_openFlyouts.cbegin(), m_openFlyouts.cend(),
        [&](OpenFlyout const& of) { return of.m_flyout && of.m_flyout == flyout; });
    if (found == m_openFlyouts.cend())
    {
        return;
    }

    if (found->m_penL3)
    {
        winrt::get_self<InkToolbarPenConfigurationControl>(found->m_penL3)->SetFocusToSelectedColor(winrt::FocusState::Programmatic);
    }
    else if (found->m_toolButton)
    {
        if (auto eraserButton = found->m_toolButton.try_as<winrt::InkToolbarEraserButton>())
        {
            winrt::get_self<InkToolbarEraserButton>(eraserButton)->SetFocusToSelectedEraser(winrt::FocusState::Programmatic);
        }
    }
}

// ---- Visual / flyout helpers ---------------------------------------------------------------

void InkToolbar::HideAllFlyouts()
{
    // FlyoutBase::Hide() raises Closed, and OnFlyoutClosed() erases entries from m_openFlyouts.
    // Snapshot the flyout references first so hiding does not invalidate the iterator. (This helper has
    // no UWP counterpart; it is added for the lift's synchronous Closed/erase behavior.)
    std::vector<winrt::FlyoutBase> flyoutsToHide;
    flyoutsToHide.reserve(m_openFlyouts.size());
    for (auto const& openFlyout : m_openFlyouts)
    {
        if (openFlyout.m_flyout)
        {
            flyoutsToHide.push_back(openFlyout.m_flyout);
        }
    }
    for (auto const& flyout : flyoutsToHide)
    {
        flyout.Hide();
    }
}

// Reflect a tool button's active/checked state and extension glyph. (Faithful port of the UWP
// UpdateToolButtonVisuals: checked iff it is the active tool and enabled; the extension glyph is shown
// only when the checked button has an attached flyout with additional options.)
void InkToolbar::UpdateToolButtonVisuals(winrt::InkToolbarToolButton const& button, winrt::InkToolbarToolButton const& activeTool)
{
    if (!button)
    {
        return;
    }

    bool isActive = (button == activeTool);

    // UWP gates the checked state on IsEnabled: a disabled tool button is never shown checked.
    bool isEnabled = button.IsEnabled();
    SetButtonCheck(button, isEnabled ? isActive : false);

    // Tie the extension (chevron) glyph to whether the checked button has an attached flyout with
    // additional options: eraser only when it has an L3; custom tool/pen only when they supply
    // configuration content; every other tool always does.
    bool hasMore = false;
    if (winrt::FlyoutBase::GetAttachedFlyout(button) && IsButtonChecked(button))
    {
        if (auto eraserButton = button.try_as<winrt::InkToolbarEraserButton>())
        {
            hasMore = winrt::get_self<InkToolbarEraserButton>(eraserButton)->ShouldShowL3();
        }
        else if (auto customTool = button.try_as<winrt::InkToolbarCustomToolButton>())
        {
            hasMore = customTool.ConfigurationContent() != nullptr;
        }
        else if (auto customPen = button.try_as<winrt::InkToolbarCustomPenButton>())
        {
            hasMore = customPen.ConfigurationContent() != nullptr;
        }
        else
        {
            hasMore = true;
        }
    }
    button.IsExtensionGlyphShown(hasMore);
}

void InkToolbar::UpdateToolButtonVisuals()
{
    auto activeTool = ActiveTool();
    if (m_buttonManager)
    {
        m_buttonManager->ForEachToolButton([&](winrt::InkToolbarToolButton const& b) { UpdateToolButtonVisuals(b, activeTool); });
    }
}

// ---- Per-property handlers (faithful ports; dial/telemetry dropped) --------------------------

void InkToolbar::OnActiveToolChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    // The active tool can change many ways (e.g. access key); make sure open flyouts close on change.
    HideAllFlyouts();

    auto oldTool = args.OldValue().try_as<winrt::InkToolbarToolButton>();
    auto newTool = args.NewValue().try_as<winrt::InkToolbarToolButton>();

    if (newTool == oldTool)
    {
        return;
    }

    if (oldTool)
    {
        UpdateToolButtonVisuals(oldTool, newTool);
    }

    winrt::InkToolbarPenButton penButton{ nullptr };
    if (newTool)
    {
        UpdateToolButtonVisuals(newTool, newTool);
        penButton = newTool.try_as<winrt::InkToolbarPenButton>();
    }

    if (penButton)
    {
        // The new tool is a pen: push its drawing attributes to the canvas.
        UpdateInkDrawingAttributes(newTool);
    }
    else
    {
        // Not a pen: still need to change modes on the canvas (eraser / custom tool).
        ApplyToolStateToInkCanvas();
    }

    // Remember the most-recent non-eraser tool (restored after "Clear all").
    if (newTool && !newTool.try_as<winrt::InkToolbarEraserButton>())
    {
        m_mostRecentNonEraserTool = winrt::make_weak(newTool);
    }

    m_activeToolChangedEventSource(*this, nullptr);
}

void InkToolbar::OnInkDrawingAttributesChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(args);
    ApplyToolStateToInkCanvas();
    m_inkDrawingAttributesChangedEventSource(*this, nullptr);
}

// ---- InkPresenter integration (faithful port; lift routes canvas config through the InkCanvas's
//      marshaling InkPresenter proxy, whose OS presenter lives on the ink thread) ---------------

winrt::Windows::UI::Input::Inking::InkPresenter InkToolbar::GetInkPresenter()
{
    // Only an explicitly-provided presenter can be handed back as the OS type; a target InkCanvas exposes
    // a marshaling proxy instead (its OS presenter is on the ink thread), used in ApplyToolStateToInkCanvas.
    if (auto targetInkPresenter = TargetInkPresenter())
    {
        return targetInkPresenter.try_as<winrt::Windows::UI::Input::Inking::InkPresenter>();
    }
    return nullptr;
}

winrt::Windows::UI::Input::Inking::InkPresenter InkToolbar::GetInkPresenter(winrt::InkToolbar const& inkToolbar)
{
    return winrt::get_self<InkToolbar>(inkToolbar)->GetInkPresenter();
}

// Current InkPresenter.HighContrastAdjustment as an int (0 UseSystemColorsWhenNecessary, 1 UseSystemColors,
// 2 UseOriginalColors), read from an explicit OS presenter or the target canvas's proxy.
int32_t InkToolbar::GetHighContrastAdjustmentValue()
{
    if (auto osPresenter = GetInkPresenter())
    {
        return static_cast<int32_t>(osPresenter.HighContrastAdjustment());
    }
    if (auto canvas = TargetInkCanvas())
    {
        if (auto proxy = canvas.InkPresenter())
        {
            return static_cast<int32_t>(proxy.GetHighContrastAdjustment());
        }
    }
    return 0; // UseSystemColorsWhenNecessary
}

// Build the drawing attributes for the active pen tool and push them onto the container's
// InkDrawingAttributes DP (which then flows to the canvas via OnInkDrawingAttributesChanged).
void InkToolbar::UpdateInkDrawingAttributes(winrt::InkToolbarToolButton const& toolButton)
{
    auto penButton = toolButton.try_as<winrt::InkToolbarPenButton>();
    if (!penButton)
    {
        return;
    }

    auto toolKind = toolButton.ToolKind();

    // Build the base attributes per tool kind so Pencil produces pencil-kind ink and Highlighter
    // produces highlighter ink (copying default attributes would only carry color/width).
    winrt::InkDrawingAttributes attrs{ nullptr };
    if (toolKind == winrt::InkToolbarTool::Pencil)
    {
        attrs = winrt::InkDrawingAttributes::CreateForPencil();
    }
    else
    {
        attrs = winrt::InkDrawingAttributes();
        attrs.DrawAsHighlighter(toolKind == winrt::InkToolbarTool::Highlighter);
    }

    float w = static_cast<float>(penButton.SelectedStrokeWidth());
    attrs.Size({ w, w });

    if (auto solidBrush = penButton.SelectedBrush().try_as<winrt::SolidColorBrush>())
    {
        attrs.Color(solidBrush.Color());
    }

    InkDrawingAttributes(attrs);
}

void InkToolbar::ApplyToolStateToInkCanvas()
{
    auto activeTool = ActiveTool();
    if (!activeTool)
    {
        return;
    }

    auto targetInkCanvas = TargetInkCanvas();
    if (!targetInkCanvas)
    {
        return;
    }

    // Drive the canvas through its InkPresenter proxy so the presenter's UI-thread cache (Mode, default
    // drawing attributes) stays in sync while it internally marshals each call to the ink thread.
    auto proxy = targetInkCanvas.InkPresenter();
    auto toolKind = activeTool.ToolKind();
    auto penButton = activeTool.try_as<winrt::InkToolbarPenButton>();

    switch (toolKind)
    {
    case winrt::InkToolbarTool::BallpointPen:
    case winrt::InkToolbarTool::Pencil:
    case winrt::InkToolbarTool::Highlighter:
    case winrt::InkToolbarTool::CustomPen:
        proxy.InputProcessingConfiguration().Mode(winrt::Microsoft::UI::Xaml::Controls::InkInputProcessingMode::Inking);
        break;
    case winrt::InkToolbarTool::Eraser:
        proxy.InputProcessingConfiguration().Mode(winrt::Microsoft::UI::Xaml::Controls::InkInputProcessingMode::Erasing);
        break;
    default:
        // For all other categories, shut off inking so the previous tool doesn't keep drawing.
        proxy.InputProcessingConfiguration().Mode(winrt::Microsoft::UI::Xaml::Controls::InkInputProcessingMode::None);
        break;
    }

    // For pens and the eraser, push the current default drawing attributes.
    if (penButton || toolKind == winrt::InkToolbarTool::Eraser)
    {
        if (auto attrs = InkDrawingAttributes())
        {
            proxy.UpdateDefaultDrawingAttributes(attrs);
        }
    }

    UpdateButtonDirection();
}

// ---- Flyout placement + orientation (faithful ports of UWP @1170-1290) ----------------------

void InkToolbar::OnButtonFlyoutPlacementChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto oldPlacement = winrt::unbox_value_or<winrt::InkToolbarButtonFlyoutPlacement>(args.OldValue(), winrt::InkToolbarButtonFlyoutPlacement::Auto);
    auto newPlacement = winrt::unbox_value_or<winrt::InkToolbarButtonFlyoutPlacement>(args.NewValue(), winrt::InkToolbarButtonFlyoutPlacement::Auto);
    if (oldPlacement != newPlacement)
    {
        UpdateButtonDirection();
    }
}

void InkToolbar::UpdateButtonDirection()
{
    if (!m_buttonManager)
    {
        return;
    }

    auto direction = ButtonFlyoutPlacement();
    m_buttonManager->ForEachToolButton([&](winrt::InkToolbarToolButton const& b) { winrt::get_self<InkToolbarToolButton>(b)->SetButtonDirection(direction); });
    m_buttonManager->ForEachToggleButton([&](winrt::InkToolbarToggleButton const& b) { winrt::get_self<InkToolbarToggleButton>(b)->SetButtonDirection(direction); });
    m_buttonManager->ForEachMenuButton([&](winrt::InkToolbarMenuButton const& b) { winrt::get_self<InkToolbarMenuButton>(b)->SetButtonDirection(direction); });
}

void InkToolbar::OnOrientationChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto oldOrientation = winrt::unbox_value_or<winrt::Orientation>(args.OldValue(), winrt::Orientation::Horizontal);
    auto newOrientation = winrt::unbox_value_or<winrt::Orientation>(args.NewValue(), winrt::Orientation::Horizontal);
    if (oldOrientation != newOrientation)
    {
        UpdateInkToolbarOrientation(newOrientation);
    }
}

void InkToolbar::UpdateInkToolbarOrientation(winrt::Orientation orientation)
{
    if (auto panel = GetTemplateChild(L"Panel").try_as<winrt::StackPanel>())
    {
        panel.Orientation(orientation);
    }
    // else: template not applied yet.
}

// ---- Children observation (faithful port of UWP @325 / @2114) -------------------------------

void InkToolbar::OnChildrenChanged(winrt::IObservableVector<winrt::DependencyObject> const& sender, winrt::IVectorChangedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);

    auto children = Children();
    uint32_t index = args.Index();

    switch (args.CollectionChange())
    {
    case winrt::CollectionChange::ItemInserted:
    {
        auto newChild = children.GetAt(index).try_as<winrt::UIElement>();
        if (index > m_childrenMirror.size()) { throw winrt::hresult_out_of_bounds(); }
        m_childrenMirror.insert(m_childrenMirror.begin() + index, newChild);
        if (newChild) { OnChildAdded(newChild); }
        break;
    }
    case winrt::CollectionChange::ItemRemoved:
    {
        if (index >= m_childrenMirror.size()) { throw winrt::hresult_out_of_bounds(); }
        auto oldChild = m_childrenMirror[index];
        m_childrenMirror.erase(m_childrenMirror.begin() + index);
        if (oldChild) { OnChildRemoved(oldChild); }
        break;
    }
    case winrt::CollectionChange::ItemChanged:
    {
        auto newChild = children.GetAt(index).try_as<winrt::UIElement>();
        if (index >= m_childrenMirror.size()) { throw winrt::hresult_out_of_bounds(); }
        auto oldChild = m_childrenMirror[index];
        m_childrenMirror[index] = newChild;
        if (oldChild) { OnChildRemoved(oldChild); }
        if (newChild) { OnChildAdded(newChild); }
        break;
    }
    case winrt::CollectionChange::Reset:
    {
        for (auto const& item : m_childrenMirror)
        {
            if (item) { OnChildRemoved(item); }
        }
        m_childrenMirror.clear();
        break;
    }
    }
}

void InkToolbar::OnChildAdded(winrt::UIElement const& child)
{
    if (!m_buttonManager->AddButton(child))
    {
        return;   // Duplicate or illegal content; ignore.
    }

    ConfigureAddedChild(child);
    m_childrenDirty = true;
    InvalidateMeasure();
}

void InkToolbar::ConfigureAddedChild(winrt::UIElement const& child)
{
    // Tool buttons + menu buttons get a back-pointer to this toolbar.
    if (auto toolButton = child.try_as<winrt::InkToolbarToolButton>())
    {
        winrt::get_self<InkToolbarToolButton>(toolButton)->SetParentInkToolbar(*this);
    }

    // Pen buttons: observe SelectedBrush / SelectedStrokeWidth so we can refire drawing-attribute changes.
    if (auto penButton = child.try_as<winrt::InkToolbarPenButton>())
    {
        auto penButtonDO = penButton.as<winrt::DependencyObject>();
        penButtonDO.RegisterPropertyChangedCallback(
            winrt::InkToolbarPenButton::SelectedBrushProperty(),
            { get_weak(), &InkToolbar::OnAnySelectedPenPropertyChanged });
        penButtonDO.RegisterPropertyChangedCallback(
            winrt::InkToolbarPenButton::SelectedStrokeWidthProperty(),
            { get_weak(), &InkToolbar::OnAnySelectedPenPropertyChanged });
    }

    if (auto menuButton = child.try_as<winrt::InkToolbarMenuButton>())
    {
        winrt::get_self<InkToolbarMenuButton>(menuButton)->SetParentInkToolbar(*this);
    }
}

void InkToolbar::OnChildRemoved(winrt::UIElement const& child)
{
    ConfigureRemovedChild(child);
    m_childrenDirty = true;
    InvalidateMeasure();
}

void InkToolbar::ConfigureRemovedChild(winrt::UIElement const& child)
{
    m_buttonManager->RemoveButton(child);
}

// When the active pen's brush/width changes, rebuild + push its drawing attributes.
void InkToolbar::OnAnySelectedPenPropertyChanged(winrt::DependencyObject const& obj, winrt::DependencyProperty const& dp)
{
    UNREFERENCED_PARAMETER(dp);
    if (auto toolButton = obj.try_as<winrt::InkToolbarToolButton>())
    {
        if (ActiveTool() == toolButton)
        {
            UpdateInkDrawingAttributes(toolButton);
        }
    }
}

// ---- Population (faithful port of UWP MeasureOverrideImpl @1859) -----------------------------

winrt::Windows::Foundation::Collections::IVector<winrt::UIElement> InkToolbar::FindTemplatePanelChildren()
{
    if (auto panel = GetTemplateChild(L"Panel").try_as<winrt::Panel>())
    {
        return panel.Children();
    }
    return nullptr;
}

// UWP configured the Surface Dial menu's pen items here; dial is unavailable in the lift -> no-op.
void InkToolbar::ConfigureExternalMenuPenItems()
{
}

winrt::Size InkToolbar::MeasureOverride(winrt::Size const& availableSize)
{
    UpdateInkToolbarOrientation(Orientation());

    if (m_childrenDirty)
    {
        // Auto-population is performed at most once.
        if (!m_autoPopulated)
        {
            PerformAutoPopulation();
            m_autoPopulated = true;
        }

        // Order the buttons and replace the template panel's children with the ordered set.
        if (auto targetChildren = FindTemplatePanelChildren())
        {
            targetChildren.Clear();
            for (auto const& item : OrderChildren())
            {
                targetChildren.Append(item);
            }
            ConfigureExternalMenuPenItems();
        }

        // Select the first pen button if nothing is active yet.
        if (!ActiveTool())
        {
            if (auto firstButton = m_buttonManager->GetFirstPenButton())
            {
                ActiveTool(firstButton.as<winrt::InkToolbarToolButton>());
            }
        }

        UpdateToolButtonVisuals();
        m_childrenDirty = false;
    }

    // Measure the applied template root (standard templated-Control behavior).
    if (winrt::VisualTreeHelper::GetChildrenCount(*this) > 0)
    {
        auto root = winrt::VisualTreeHelper::GetChild(*this, 0).as<winrt::UIElement>();
        root.Measure(availableSize);
        return root.DesiredSize();
    }
    return winrt::Size{ 0, 0 };
}

winrt::AutomationPeer InkToolbar::OnCreateAutomationPeer()
{
    return winrt::make<InkToolbarAutomationPeer>(*this);
}

// ---- Ruler / stencil checked handlers (faithful ports; dial + ruler-event dropped) ----------

void InkToolbar::OnIsRulerButtonCheckedChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    bool oldValue = winrt::unbox_value_or<bool>(args.OldValue(), false);
    bool newValue = winrt::unbox_value_or<bool>(args.NewValue(), false);
    if (newValue == oldValue)
    {
        return;
    }

    if (!m_buttonManager->GetRulerButton())
    {
        return;
    }

    SetStencilVisibility(newValue, winrt::InkToolbarStencilKind::Ruler);
    // UWP also raised an IsRulerButtonCheckedChanged event; that event is not present on the lift InkToolbar.
}

void InkToolbar::OnIsStencilButtonCheckedChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    bool oldValue = winrt::unbox_value_or<bool>(args.OldValue(), false);
    bool newValue = winrt::unbox_value_or<bool>(args.NewValue(), false);
    if (newValue == oldValue)
    {
        return;
    }

    auto stencilButton = m_buttonManager->GetStencilButton();
    if (!stencilButton)
    {
        return;
    }

    if (newValue)
    {
        ShowLastSelectedStencil(stencilButton);
    }
    else
    {
        HideAllStencils(stencilButton);
    }

    // Keep the button's IsChecked consistent with the new value.
    SetButtonCheck(stencilButton.as<muxcp::ToggleButton>(), newValue);

    // Raise IsStencilButtonCheckedChanged with the current stencil kind.
    auto eventArgs = winrt::make<winrt::implementation::InkToolbarIsStencilButtonCheckedChangedEventArgs>(
        stencilButton, stencilButton.SelectedStencil());
    m_isStencilButtonCheckedChangedEventSource(*this, eventArgs);
}

// ---- Stencils (faithful orchestration; lift adaptation: stencil visibility is driven through the
//      StencilButton's Ruler/Protractor DPs, since the OS InkPresenterInternal::GetStencils used by UWP
//      is not available in the lift) ---------------------------------------------------------------

void InkToolbar::SetStencilVisibility(bool isVisible, winrt::InkToolbarStencilKind kind)
{
    auto stencilButton = m_buttonManager->GetStencilButton();
    if (!stencilButton)
    {
        return;
    }

    // Lift adaptation: the OS InkPresenterRuler/Protractor are thread-affine to the ink-thread OS
    // presenter, so drive visibility through the InkPresenter proxy (which marshals stencil creation
    // and IsVisible onto the ink thread) rather than the UI-thread Ruler/Protractor DPs. Honor the
    // TargetInkPresenter-over-TargetInkCanvas precedence (see OnTargetInkCanvasChanged).
    winrt::Microsoft::UI::Xaml::Controls::InkPresenter proxy{ nullptr };
    if (auto target = TargetInkPresenter())
    {
        proxy = target.try_as<winrt::Microsoft::UI::Xaml::Controls::InkPresenter>();
    }
    else if (auto targetInkCanvas = TargetInkCanvas())
    {
        proxy = targetInkCanvas.InkPresenter();
    }
    if (!proxy)
    {
        // No lift InkPresenter to drive: either no target is set, or TargetInkPresenter is a raw OS
        // InkPresenter the toolbar cannot marshal stencils to. UWP no-ops the same way - GetInkPresenter
        // returns null when neither target is set and SetStencilVisibility guards with if (inkPresenter).
        return;
    }
    auto proxyImpl = winrt::get_self<::InkPresenter>(proxy);

    switch (kind)
    {
    case winrt::InkToolbarStencilKind::Ruler:
        proxyImpl->SetRulerEnabled(isVisible);
        m_rulerVisible = isVisible;
        break;
    case winrt::InkToolbarStencilKind::Protractor:
        proxyImpl->SetProtractorEnabled(isVisible);
        m_protractorVisible = isVisible;
        break;
    default:
        break;
    }
}

void InkToolbar::ShowSingleStencil(winrt::InkToolbarStencilButton const& stencilButton, winrt::InkToolbarStencilKind kind)
{
    if (!stencilButton)
    {
        return;
    }

    auto stencilImpl = winrt::get_self<InkToolbarStencilButton>(stencilButton);
    stencilImpl->SetAllStencilItemsCheck(false);

    switch (kind)
    {
    case winrt::InkToolbarStencilKind::Ruler:
        SetStencilVisibility(false, winrt::InkToolbarStencilKind::Protractor);
        SetStencilVisibility(true, winrt::InkToolbarStencilKind::Ruler);
        break;
    case winrt::InkToolbarStencilKind::Protractor:
        SetStencilVisibility(false, winrt::InkToolbarStencilKind::Ruler);
        SetStencilVisibility(true, winrt::InkToolbarStencilKind::Protractor);
        break;
    default:
        throw winrt::hresult_invalid_argument(L"Unexpected stencil kind");
    }

    stencilImpl->SetL3StencilItemCheck(kind, true);
    stencilButton.SelectedStencil(kind);
}

void InkToolbar::HideAllStencils(winrt::InkToolbarStencilButton const& stencilButton)
{
    if (!stencilButton)
    {
        return;
    }

    winrt::get_self<InkToolbarStencilButton>(stencilButton)->SetAllStencilItemsCheck(false);
    SetStencilVisibility(false, winrt::InkToolbarStencilKind::Protractor);
    SetStencilVisibility(false, winrt::InkToolbarStencilKind::Ruler);
}

void InkToolbar::ShowLastSelectedStencil(winrt::InkToolbarStencilButton const& stencilButton)
{
    if (!stencilButton)
    {
        return;
    }
    ShowSingleStencil(stencilButton, stencilButton.SelectedStencil());
}

// Which stencil (if any) is currently visible on the presenter. Lift adaptation: read the StencilButton's
// Ruler/Protractor DPs' IsVisible (UWP walked InkPresenterInternal::GetStencils, which is internal-only).
bool InkToolbar::IsAnyStencilVisible(winrt::Windows::UI::Input::Inking::InkPresenter const& inkPresenter, winrt::InkToolbarStencilKind& kind)
{
    UNREFERENCED_PARAMETER(inkPresenter);

    auto stencilButton = m_buttonManager->GetStencilButton();
    if (!stencilButton)
    {
        return false;
    }

    if (m_rulerVisible)
    {
        kind = winrt::InkToolbarStencilKind::Ruler;
        return true;
    }
    if (m_protractorVisible)
    {
        kind = winrt::InkToolbarStencilKind::Protractor;
        return true;
    }
    return false;
}

// ---- Ruler / stencil button state (lift-adapted: driven via the buttons' Ruler/Protractor DPs) ----

void InkToolbar::UpdateRulerButtonState()
{
    auto rulerButton = m_buttonManager->GetToggleButton(winrt::InkToolbarToggle::Ruler);
    if (!rulerButton)
    {
        return;
    }

    bool isRulerVisible = IsButtonChecked(rulerButton);
    if (isRulerVisible)
    {
        SetStencilVisibility(true, winrt::InkToolbarStencilKind::Ruler);
    }

    SetButtonCheck(rulerButton, isRulerVisible);
    IsRulerButtonChecked(isRulerVisible);
}

void InkToolbar::UpdateStencilButtonState()
{
    auto stencilButton = m_buttonManager->GetStencilButton();
    if (!stencilButton)
    {
        return;
    }

    winrt::InkToolbarStencilKind kind{ winrt::InkToolbarStencilKind::Ruler };
    bool isStencilVisible = IsAnyStencilVisible(nullptr, kind);

    if (isStencilVisible)
    {
        stencilButton.SelectedStencil(kind);
        SetButtonCheck(stencilButton.as<muxcp::ToggleButton>(), true);
        IsStencilButtonChecked(true);
    }
    else
    {
        SetButtonCheck(stencilButton.as<muxcp::ToggleButton>(), false);
        IsStencilButtonChecked(false);
        stencilButton.IsExtensionGlyphShown(false);
    }
}

// ---- Target InkCanvas / InkPresenter (faithful dispatch; 2-arg wiring's internal parts dropped) ----

void InkToolbar::OnTargetInkCanvasChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    // If TargetInkPresenter is explicitly set, it wins over the canvas (per API review); do nothing.
    if (TargetInkPresenter())
    {
        return;
    }

    // Hide any stencil still showing on the previous canvas before switching - its proxy owns the OS
    // stencil (the null Ruler/Protractor DPs can't reach it), so it would otherwise stay drawn.
    if (m_rulerVisible || m_protractorVisible)
    {
        if (auto oldCanvas = args.OldValue().try_as<winrt::InkCanvas>())
        {
            if (auto oldProxy = oldCanvas.InkPresenter())
            {
                auto oldImpl = winrt::get_self<::InkPresenter>(oldProxy);
                oldImpl->SetRulerEnabled(false);
                oldImpl->SetProtractorEnabled(false);
            }
        }
        m_rulerVisible = false;
        m_protractorVisible = false;
    }

    auto getPresenter = [](winrt::IInspectable const& val) -> winrt::Windows::UI::Input::Inking::InkPresenter
    {
        if (auto canvas = val.try_as<winrt::InkCanvas>())
        {
            return canvas.InkPresenter().try_as<winrt::Windows::UI::Input::Inking::InkPresenter>();
        }
        return nullptr;
    };

    OnTargetInkPresenterChanged(getPresenter(args.OldValue()), getPresenter(args.NewValue()));
}

void InkToolbar::OnTargetInkPresenterChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto resolve = [this](winrt::IInspectable const& val) -> winrt::Windows::UI::Input::Inking::InkPresenter
    {
        if (val)
        {
            return val.try_as<winrt::Windows::UI::Input::Inking::InkPresenter>();
        }
        // Cleared: fall back to the target InkCanvas's presenter, if any (both can be set; presenter wins).
        if (auto canvas = TargetInkCanvas())
        {
            return canvas.InkPresenter().try_as<winrt::Windows::UI::Input::Inking::InkPresenter>();
        }
        return nullptr;
    };

    OnTargetInkPresenterChanged(resolve(args.OldValue()), resolve(args.NewValue()));
}

void InkToolbar::OnTargetInkPresenterChanged(
    winrt::Windows::UI::Input::Inking::InkPresenter const& oldInkPresenter,
    winrt::Windows::UI::Input::Inking::InkPresenter const& newInkPresenter)
{
    UNREFERENCED_PARAMETER(oldInkPresenter);
    UNREFERENCED_PARAMETER(newInkPresenter);

    // Portable core: reconcile the stencil button state and push the current tool state to the canvas.
    UpdateStencilButtonState();
    ApplyToolStateToInkCanvas();

    // We can't query whether the user is mid-stroke on a new canvas; assume not.
    m_inking = false;

    // NOTE (documented lift gaps, dropped here): UWP also wired InkStrokeInput StrokeStarted/Ended/Canceled
    // (mid-stroke tracking), IInkPresenterInternal2::HighContrastAdjustmentChanged, AccessibilitySettings
    // HighContrastChanged, and a CoreInkIndependentInputSource pointer-pressing peek. Those are OS-internal /
    // unavailable in the lift; m_inking stays false (minor mid-stroke L3-suppression behavior delta).
}

// ---- Eraser L3 item click (faithful port of UWP @3417; modern path; internal fire dropped) ---

void InkToolbar::OnEraserL3ItemsClicked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(args);

    auto eraserButton = m_buttonManager->GetEraserButton();
    if (!eraserButton)
    {
        return;
    }

    auto name = sender.as<winrt::FrameworkElement>().Name();
    auto kind = winrt::InkToolbarEraserFlyoutItemKind::StrokeEraser;

    if (name == L"StrokeEraser")
    {
        kind = winrt::InkToolbarEraserFlyoutItemKind::StrokeEraser;
        eraserButton.SelectedEraser(winrt::InkToolbarEraserKind::Stroke);
    }
    else if (name == L"SmallEraser")
    {
        kind = winrt::InkToolbarEraserFlyoutItemKind::PrecisionSmallEraser;
        eraserButton.SelectedEraser(winrt::InkToolbarEraserKind::PrecisionSmall);
    }
    else if (name == L"LargeEraser")
    {
        kind = winrt::InkToolbarEraserFlyoutItemKind::PrecisionLargeEraser;
        eraserButton.SelectedEraser(winrt::InkToolbarEraserKind::PrecisionLarge);
    }
    else if (name == L"ClearAll")
    {
        kind = winrt::InkToolbarEraserFlyoutItemKind::ClearAll;
        // The app is expected to clear strokes, but the active tool must still be restored by the toolbar.
        SelectMostRecentNonEraserTool();
    }

    // Fire EraseAllClicked before clearing so undo can capture the strokes.
    if (kind == winrt::InkToolbarEraserFlyoutItemKind::ClearAll)
    {
        m_eraseAllClickedEventSource(*this, nullptr);
    }

    // Fire the EraserFlyoutItemClicked event (UWP routed via IInkToolbarInternal; raised directly here).
    auto eventArgs = winrt::make<InkToolbarEraserFlyoutItemClickedEventArgs>(kind);
    m_eraserFlyoutItemClickedEventSource(*this, eventArgs);

    if (kind == winrt::InkToolbarEraserFlyoutItemKind::ClearAll && !IsCustomDry())
    {
        ClearAllStrokes();
    }
}

// ---- Misc helpers ---------------------------------------------------------------------------

// Custom-drying detection needs internal InkPresenter state not surfaced in the lift; assume default-dry.
bool InkToolbar::IsCustomDry()
{
    return false;
}

void InkToolbar::ClearAllStrokes()
{
    if (auto canvas = TargetInkCanvas())
    {
        if (auto container = canvas.InkPresenter().StrokeContainer())
        {
            container.Clear();
        }
    }
}

void InkToolbar::SelectMostRecentNonEraserTool()
{
    if (auto tool = m_mostRecentNonEraserTool.get())
    {
        ActiveTool(tool);
    }
    else if (auto firstPen = m_buttonManager->GetFirstPenButton())
    {
        ActiveTool(firstPen.as<winrt::InkToolbarToolButton>());
    }
}

// ---- Stencil L3 item click (faithful port of UWP @3661; UpdateExternalRulerMode/telemetry dropped) ---

void InkToolbar::OnStencilL3ItemsClicked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(args);

    auto stencilButton = m_buttonManager->GetStencilButton();
    if (!stencilButton)
    {
        return;
    }

    auto flyoutItem = sender.try_as<winrt::InkToolbarFlyoutItem>();
    auto senderElement = sender.try_as<winrt::FrameworkElement>();
    if (!flyoutItem || !senderElement)
    {
        return;
    }

    auto name = senderElement.Name();
    bool isItemChecked = flyoutItem.IsChecked();
    auto stencilImpl = winrt::get_self<InkToolbarStencilButton>(stencilButton);

    if (name == L"StencilRuler")
    {
        if (isItemChecked)
        {
            // Hide the protractor and show the ruler.
            ShowSingleStencil(stencilButton, winrt::InkToolbarStencilKind::Ruler);
        }
        else
        {
            SetStencilVisibility(false, winrt::InkToolbarStencilKind::Ruler);
        }

        stencilImpl->UpdateIcon(winrt::InkToolbarStencilKind::Ruler);
    }
    else if (name == L"StencilProtractor")
    {
        if (isItemChecked)
        {
            // Hide the ruler and show the protractor.
            ShowSingleStencil(stencilButton, winrt::InkToolbarStencilKind::Protractor);
        }
        else
        {
            SetStencilVisibility(false, winrt::InkToolbarStencilKind::Protractor);
        }

        stencilImpl->UpdateIcon(winrt::InkToolbarStencilKind::Protractor);
    }

    // Dismiss L3.
    if (m_stencilFlyout)
    {
        m_stencilFlyout.Hide();
    }
}

// ---- Keyboard navigation between toolbar buttons (faithful port of UWP @3661/3940) ----------

void InkToolbar::OnUnhandledButtonKeyPress(winrt::UIElement const& button, winrt::KeyRoutedEventArgs const& e)
{
    // Must use the actual visual children (not ButtonManager's array) so that order-button placement is honored.
    auto children = FindTemplatePanelChildren();
    if (!children)
    {
        return;
    }

    uint32_t childCount = children.Size();
    if (childCount <= 1)
    {
        // No children, or only one child. No impact of moving focus within toolbar.
        return;
    }

    auto key = e.Key();
    uint32_t currentChildIndex = 0;
    uint32_t nextChildIndex = 0;
    bool found = false;
    bool focusSet = false;

    if (key == winrt::Windows::System::VirtualKey::Left || key == winrt::Windows::System::VirtualKey::Up)
    {
        found = children.IndexOf(button, currentChildIndex);
        if (found)
        {
            nextChildIndex = currentChildIndex;

            // Find the first enabled and visible child in front of the current child.
            while (nextChildIndex > 0)
            {
                --nextChildIndex;

                if (children.GetAt(nextChildIndex).Visibility() == winrt::Visibility::Collapsed)
                {
                    continue;
                }

                if (SetFocusToChild(children, nextChildIndex))
                {
                    focusSet = true;
                    break;
                }
            }

            // Can't find any enabled child in front of the current child; try wrapping around.
            if (!focusSet)
            {
                nextChildIndex = childCount - 1;

                while (nextChildIndex > currentChildIndex)
                {
                    if (children.GetAt(nextChildIndex).Visibility() == winrt::Visibility::Collapsed)
                    {
                        --nextChildIndex;
                        continue;
                    }

                    if (SetFocusToChild(children, nextChildIndex))
                    {
                        break;
                    }

                    --nextChildIndex;
                }
            }
        }

        e.Handled(true);
    }
    else if (key == winrt::Windows::System::VirtualKey::Right || key == winrt::Windows::System::VirtualKey::Down)
    {
        found = children.IndexOf(button, currentChildIndex);
        if (found)
        {
            nextChildIndex = currentChildIndex;

            // Find the first enabled child after the current child.
            while (nextChildIndex < (childCount - 1))
            {
                ++nextChildIndex;

                if (children.GetAt(nextChildIndex).Visibility() == winrt::Visibility::Collapsed)
                {
                    continue;
                }

                if (SetFocusToChild(children, nextChildIndex))
                {
                    focusSet = true;
                    break;
                }
            }

            // Can't find any enabled child after the current child; try wrapping around.
            if (!focusSet)
            {
                nextChildIndex = 0;

                while (nextChildIndex < currentChildIndex)
                {
                    if (children.GetAt(nextChildIndex).Visibility() == winrt::Visibility::Collapsed)
                    {
                        ++nextChildIndex;
                        continue;
                    }

                    if (SetFocusToChild(children, nextChildIndex))
                    {
                        break;
                    }

                    ++nextChildIndex;
                }
            }
        }

        e.Handled(true);
    }
}

bool InkToolbar::SetFocusToChild(winrt::Windows::Foundation::Collections::IVector<winrt::UIElement> const& children, uint32_t index)
{
    auto childControl = children.GetAt(index).try_as<winrt::Control>();
    if (!childControl || !childControl.IsEnabled())
    {
        return false;
    }

    return childControl.Focus(winrt::FocusState::Keyboard);
}

// ---- ChangeToTool (faithful port of UWP @5499; UWP called it from the Surface Dial handler, which is
//      unavailable in the lift, so this is currently unreferenced but kept for contract completeness) ----

void InkToolbar::ChangeToTool(double delta)
{
    auto toolButton = ActiveTool();
    auto toolButtonAsUIElement = toolButton ? toolButton.try_as<winrt::UIElement>() : nullptr;

    if (!toolButton || !m_buttonManager->IsOneOfOurs(toolButtonAsUIElement))
    {
        // The 'equal' in 'greater than or equal' is significant: a 0 delta selects the first button.
        toolButton = (delta >= 0)
            ? m_buttonManager->GetFirstToolButton()
            : m_buttonManager->GetLastToolButton();
    }

    ActiveTool(toolButton);
}
