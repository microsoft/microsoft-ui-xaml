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

    // UWP keeps the eraser-flyout selection and item visibilities off the public API (they lived on
    // the internal InkToolbarEraserButtonInternal object). Only IsClearAllVisible is a public DP here;
    // the rest is impl-owned state with the same accessor shape the flyout logic already uses.
    enum class EraserKind { Stroke, PrecisionSmall, PrecisionLarge };

    EraserKind SelectedEraser() const noexcept { return m_selectedEraser; }
    void SelectedEraser(EraserKind value)
    {
        if (m_selectedEraser == value) { return; }
        auto oldValue = m_selectedEraser;
        m_selectedEraser = value;
        OnSelectedEraserChanged(oldValue, value);
    }

    bool IsStrokeEraserVisible() const noexcept { return m_isStrokeEraserVisible; }
    void IsStrokeEraserVisible(bool value)
    {
        if (m_isStrokeEraserVisible == value) { return; }
        m_isStrokeEraserVisible = value;
        OnL3ItemsVisibilitiesChanged();
    }

    bool ArePrecisionErasersVisible() const noexcept { return m_arePrecisionErasersVisible; }
    void ArePrecisionErasersVisible(bool value)
    {
        if (m_arePrecisionErasersVisible == value) { return; }
        m_arePrecisionErasersVisible = value;
        OnL3ItemsVisibilitiesChanged();
    }

    // Runs from the base InkToolbarToolButton::OnApplyTemplate via the OnApplyTemplateCore hook.
    void OnApplyTemplateCore() override;
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    // Eraser L3 (flyout) management. NOTE: the eraser flyout content is provided by the XAML template
    // "InkToolbarEraserButtonFlyoutContentTemplate" (named items StrokeEraser/SmallEraser/LargeEraser/
    // ClearAll). Until that template is ported into the lift theme resources, FindChild returns null and
    // these methods no-op (documented gap).
    void OnSelectedEraserChanged(EraserKind oldValue, EraserKind newValue);
    void OnL3ItemsVisibilitiesChanged();
    void HookUpToEraserEvents(winrt::RoutedEventHandler const& handler, bool& eventHookedUp, winrt::event_token& token);
    void SetL3EraserItemCheck(EraserKind kind, bool check);
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
    static wchar_t const* EraserKindToEraserItemName(EraserKind kind);
    static EraserKind EraserItemNameToEraserKind(std::wstring_view name);
    void OnAccessKeyInvoked(winrt::IInspectable const& sender, winrt::AccessKeyInvokedEventArgs const& args);

    EraserKind m_selectedEraser{ EraserKind::Stroke };
    bool m_isStrokeEraserVisible{ false };
    bool m_arePrecisionErasersVisible{ false };

    winrt::event_token m_eraserItemCheckedRegistrationToken{};
    winrt::event_token m_accessKeyInvokedToken{};
};

