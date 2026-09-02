// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarEraserButton.g.h"
#include "InkToolbarEraserButton.properties.h"

#include "InkToolbarToolButton.h"
#include "ResourceAccessor.h"

class InkToolbarEraserButton :
    public winrt::implementation::InkToolbarEraserButtonT<InkToolbarEraserButton, InkToolbarToolButton>, 
    public InkToolbarEraserButtonProperties
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarToolButton)

    InkToolbarEraserButton()
    {
        SetToolKind(winrt::InkToolbarTool::Eraser);
        SetDefaultStyleKey(this);
    }

    winrt::hstring GetLocalizedToolName() override
    {
        return ResourceAccessor::GetLocalizedStringResource(SR_InkToolbarEraserButtonName);
    }

    // These functions are ambiguous with InkToolbarToolButton, disambiguate
    using InkToolbarEraserButtonProperties::EnsureProperties;
    using InkToolbarEraserButtonProperties::ClearProperties;

    // NOTE: SelectedEraser / IsClearAllVisible / IsStrokeEraserVisible / ArePrecisionErasersVisible are
    // generated DEPENDENCY PROPERTIES (inherited from InkToolbarEraserButtonProperties). UWP hid these on
    // a separate InkToolbarEraserButtonInternal object; the lift flattened them onto the button itself, so
    // the port uses the inherited DP accessors directly (the manual shadowing members were removed).

    // Runs from the base InkToolbarToolButton::OnApplyTemplate via the OnApplyTemplateCore hook.
    void OnApplyTemplateCore() override;
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    // Eraser L3 (flyout) management. NOTE: the eraser flyout content is provided by the XAML template
    // "InkToolbarEraserButtonFlyoutContentTemplate" (named items StrokeEraser/SmallEraser/LargeEraser/
    // ClearAll). Until that template is ported into the lift theme resources, FindChild returns null and
    // these methods no-op (documented gap).
    void OnSelectedEraserChanged(winrt::InkToolbarEraserKind oldValue, winrt::InkToolbarEraserKind newValue);
    void OnL3ItemsVisibilitiesChanged();
    void HookUpToEraserEvents(winrt::RoutedEventHandler const& handler, bool& eventHookedUp, winrt::event_token& token);
    void SetL3EraserItemCheck(winrt::InkToolbarEraserKind kind, bool check);
    bool ShouldShowL3();
    void UpdateFlyoutItemVisuals();
    void SetFocusToSelectedEraser(winrt::FocusState focusState);
    void AdvanceEraserSelection(bool forward);

private:
    enum class EraserFlyoutItemKind { StrokeEraser, PrecisionSmallEraser, PrecisionLargeEraser, ClearAll };

    void OnEraserItemChecked(winrt::InkToolbarFlyoutItem const& sender, winrt::IInspectable const& arg);
    void SetupL3(wchar_t const* itemName);
    void ConfigureEraserFlyoutItems(
        winrt::RoutedEventHandler const& handler,
        bool shouldHookupEvent,
        winrt::event_token& token,
        winrt::DependencyObject const& flyoutContent,
        EraserFlyoutItemKind kind);
    bool GetIsItemVisible(EraserFlyoutItemKind kind);
    static wchar_t const* EraserKindToEraserItemName(winrt::InkToolbarEraserKind kind);
    static winrt::InkToolbarEraserKind EraserItemNameToEraserKind(std::wstring_view name);
    void OnAccessKeyInvoked(winrt::IInspectable const& sender, winrt::AccessKeyInvokedEventArgs const& args);

    winrt::event_token m_eraserItemCheckedRegistrationToken{};
    winrt::event_token m_accessKeyInvokedToken{};
};

