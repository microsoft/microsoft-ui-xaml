// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// ============================================================================================
// InkToolbar container - C++/WinRT translation of the UWP container contract
// (onecoreuap\...\inkcontrols\lib\InkToolbar_Partial.h).
//
// Lift adaptations vs UWP (documented; these APIs are absent from the lift surface):
//   * Dependency properties are the generated ones (InkToolbarProperties) - never shadowed.
//   * Dropped: ExternalActor / RadialController (Surface Dial), ITestableInkToolbar,
//     ICoreInkIndependentInputSource, IInkPointDrawingAttributesStatics (dynamic stroke width),
//     IInkToolbarInternal, telemetry (TraceLogging). These are OS-internal, absent from the lift.
//   * Stencils use InkPresenterRuler / InkPresenterProtractor directly (no InkPresenterStencil base).
// ============================================================================================

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbar.g.h"
#include "InkToolbar.properties.h"

#include "ButtonManager.h"
#include "InkToolbarMenuButton.h"   // InkToolbarMenuButtonCheckedState

class InkToolbar :
    public ReferenceTracker<InkToolbar, winrt::implementation::InkToolbarT>,
    public InkToolbarProperties
{
public:
    InkToolbar();

    // IFrameworkElementOverrides / IUIElementOverrides
    // NOTE: like UWP, the container populates its buttons in MeasureOverride (runs even when Children
    // change after template apply), not OnApplyTemplate.
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::AutomationPeer OnCreateAutomationPeer();

    // DependencyObject property-changed hook (dispatches to the per-property handlers).
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    // Projected query methods.
    winrt::InkToolbarToolButton GetToolButton(winrt::InkToolbarTool tool);
    winrt::InkToolbarToggleButton GetToggleButton(winrt::InkToolbarToggle tool);
    winrt::InkToolbarMenuButton GetMenuButton(winrt::InkToolbarMenuKind menu);

    // ---- Static button check helpers (used across the cluster) ----
    static void SetButtonCheck(winrt::InkToolbarToolButton const& button, bool check);
    static void SetButtonCheck(winrt::InkToolbarToggleButton const& button, bool check);
    static void SetButtonCheck(winrt::InkToolbarMenuButton const& button, InkToolbarMenuButtonCheckedState check);
    static void SetButtonCheck(winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton const& button, bool check);
    static bool IsButtonChecked(winrt::InkToolbarToolButton const& button);
    static bool IsButtonChecked(winrt::InkToolbarToggleButton const& button);
    static bool IsButtonChecked(winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton const& button);
    static InkToolbarMenuButtonCheckedState IsButtonChecked(winrt::InkToolbarMenuButton const& button);

    // ---- Stencil kind conversion ----
    static winrt::Windows::UI::Input::Inking::InkPresenterStencilKind InkToolbarStencilKindToInkPresenterStencilKind(winrt::InkToolbarStencilKind kind);
    static winrt::InkToolbarStencilKind InkPresenterStencilKindToInkToolbarStencilKind(winrt::Windows::UI::Input::Inking::InkPresenterStencilKind kind);

    // ---- Pen L3 (PenConfigurationControl) callbacks ----
    void OnPenL3ColorPickerGotFocus();
    void OnPenL3StrokeWidthGotFocus();
    void DismissL3(winrt::InkToolbarPenConfigurationControl const& penL3);

    // ---- Eraser / Stencil L3 item click callbacks ----
    void OnEraserL3ItemsClicked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnStencilL3ItemsClicked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

    // ---- Button actions (called by ButtonManager) ----
    void ExecuteToolAction(winrt::InkToolbarToolButton const& toolButton, bool isNormalActivation);
    void ExecuteToggleAction(winrt::InkToolbarToggleButton const& toggleButton);
    void ExecuteMenuAction(winrt::InkToolbarMenuButton const& menuButton, bool isNormalActivation);
    void OnUnhandledButtonKeyPress(winrt::UIElement const& button, winrt::KeyRoutedEventArgs const& args);
    bool SetFocusToChild(winrt::Windows::Foundation::Collections::IVector<winrt::UIElement> const& children, uint32_t index);
    void OnCheckStateChanged(winrt::UIElement const& child, bool checked);
    void ChangeToTool(double delta);

    // ---- L3 open/close (for automation peers) ----
    bool IsL3Open(winrt::InkToolbarToolButton const& toolButton);
    void OpenL3(winrt::InkToolbarToolButton const& toolButton);
    void CloseL3(winrt::InkToolbarToolButton const& toolButton);
    bool IsL3Open(winrt::InkToolbarMenuButton const& menuButton);
    void OpenL3(winrt::InkToolbarMenuButton const& menuButton);
    void CloseL3(winrt::InkToolbarMenuButton const& menuButton);

    static winrt::Windows::UI::Input::Inking::InkPresenter GetInkPresenter(winrt::InkToolbar const& inkToolbar);

private:
    // Children observation.
    void OnChildrenChanged(winrt::IObservableVector<winrt::DependencyObject> const& sender, winrt::IVectorChangedEventArgs const& args);
    void OnChildAdded(winrt::UIElement const& child);
    void OnChildRemoved(winrt::UIElement const& child);
    void ConfigureAddedChild(winrt::UIElement const& child);
    void ConfigureRemovedChild(winrt::UIElement const& child);
    void PerformAutoPopulation();
    std::vector<winrt::UIElement> OrderChildren();
    winrt::Windows::Foundation::Collections::IVector<winrt::UIElement> FindTemplatePanelChildren();

    void OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void ConfigureExternalMenuPenItems();
    void OnFlyoutClosed(winrt::IInspectable const& sender, winrt::IInspectable const& args);
    void OnFlyoutOpened(winrt::IInspectable const& sender, winrt::IInspectable const& args);
    void OnAnySelectedPenPropertyChanged(winrt::DependencyObject const& obj, winrt::DependencyProperty const& dp);

    // Per-property changed handlers.
    void OnActiveToolChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnInkDrawingAttributesChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnIsRulerButtonCheckedChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnIsStencilButtonCheckedChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnTargetInkCanvasChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnButtonFlyoutPlacementChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnOrientationChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnTargetInkPresenterChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnTargetInkPresenterChanged(
        winrt::Windows::UI::Input::Inking::InkPresenter const& oldInkPresenter,
        winrt::Windows::UI::Input::Inking::InkPresenter const& newInkPresenter);

    void UpdateButtonDirection();
    void UpdateInkToolbarOrientation(winrt::Orientation orientation);

    // Visuals / flyout helpers.
    void HideAllFlyouts();
    void UpdateToolButtonVisuals();
    void UpdateToolButtonVisuals(winrt::InkToolbarToolButton const& button, winrt::InkToolbarToolButton const& activeTool);

    // Stencils.
    void SetStencilVisibility(bool isVisible, winrt::InkToolbarStencilKind kind);
    void UpdateRulerButtonState();
    void UpdateStencilButtonState();
    void ShowSingleStencil(winrt::InkToolbarStencilButton const& stencilButton, winrt::InkToolbarStencilKind kind);
    void HideAllStencils(winrt::InkToolbarStencilButton const& stencilButton);
    void ShowLastSelectedStencil(winrt::InkToolbarStencilButton const& stencilButton);
    bool IsAnyStencilVisible(winrt::Windows::UI::Input::Inking::InkPresenter const& inkPresenter, winrt::InkToolbarStencilKind& kind);
    winrt::Windows::UI::Input::Inking::InkPresenterStencilKind GetShowingStencilKind();

    // Tool state.
    void ApplyToolStateToInkCanvas();
    void UpdateInkDrawingAttributes(winrt::InkToolbarToolButton const& toolButton);
    bool IsCustomDry();
    void SelectMostRecentNonEraserTool();
    void ClearAllStrokes();
    void DeactivateMenuButton(winrt::InkToolbarMenuButton const& menuButton);

    winrt::Windows::UI::Input::Inking::InkPresenter GetInkPresenter();

    // ---- State ----
    std::unique_ptr<ButtonManager> m_buttonManager;
    std::vector<winrt::UIElement> m_childrenMirror;
    // Strong hold for auto-populated buttons. UWP's ButtonManager keeps these via TrackerPtr (strong);
    // the lift ButtonManager stores weak_ref, so the container roots them here to prevent collection
    // before the template's Panel adopts them.
    std::vector<winrt::UIElement> m_autoPopulatedButtons;
    bool m_autoPopulated = false;
    bool m_childrenDirty = true;

    // Tracks an open L3 for as long as it is open (registered for flyout Closed to prune).
    struct OpenFlyout
    {
        winrt::Microsoft::UI::Xaml::Controls::Primitives::FlyoutBase m_flyout{ nullptr };
        winrt::InkToolbarToolButton m_toolButton{ nullptr };
        winrt::InkToolbarMenuButton m_menuButton{ nullptr };
        winrt::InkToolbarPenConfigurationControl m_penL3{ nullptr };
        winrt::event_token m_closedRegistrationToken{};
        winrt::event_token m_openedRegistrationToken{};
    };
    std::vector<OpenFlyout> m_openFlyouts;

    bool m_hookedUpToEraserEvents = false;
    winrt::Microsoft::UI::Xaml::Controls::Primitives::FlyoutBase m_eraserFlyout{ nullptr };
    bool m_hookedUpToStencilEvents = false;
    winrt::Microsoft::UI::Xaml::Controls::Primitives::FlyoutBase m_stencilFlyout{ nullptr };

    bool m_inking = false;
    winrt::Visibility m_lastVisibility = winrt::Visibility::Collapsed;
    bool m_configuredStencilButtonIntoToggleMode = false;

    // Most-recently selected non-eraser tool (restored after "Clear all").
    winrt::weak_ref<winrt::InkToolbarToolButton> m_mostRecentNonEraserTool;

    winrt::event_token m_childrenChangedToken{};
    winrt::event_token m_loadedToken{};
    winrt::event_token m_eraserEventToken{};
    winrt::event_token m_stencilEventToken{};
};
