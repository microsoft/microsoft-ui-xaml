// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarStencilButton.g.h"
#include "InkToolbarStencilButton.properties.h"

#include "InkToolbarMenuButton.h"

class InkToolbarStencilButton :
    public winrt::implementation::InkToolbarStencilButtonT<InkToolbarStencilButton, InkToolbarMenuButton>, 
    public InkToolbarStencilButtonProperties
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarMenuButton)

    InkToolbarStencilButton()
    {
        SetMenuKind(winrt::InkToolbarMenuKind::Stencil);
        SetDefaultStyleKey(this);
    }

    // These functions are ambiguous with InkToolbarMenuButton, disambiguate
    using InkToolbarStencilButtonProperties::EnsureProperties;
    using InkToolbarStencilButtonProperties::ClearProperties;

    // NOTE: Ruler / Protractor / SelectedStencil / IsRulerItemVisible / IsProtractorItemVisible are generated
    // DEPENDENCY PROPERTIES (inherited from InkToolbarStencilButtonProperties). The container sets Ruler/
    // Protractor from the target InkPresenter. The manual shadowing members were removed. The stencil flyout
    // content comes from the XAML template "InkToolbarStencilButtonFlyoutContentTemplate" (named items
    // StencilRuler/StencilProtractor) - until that template is ported, FindChild returns null and the
    // flyout logic no-ops (documented gap).

    // Runs from the base InkToolbarMenuButton::OnApplyTemplate via the OnApplyTemplateCore hook.
    void OnApplyTemplateCore() override;
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    void SetL3StencilItemCheck(winrt::InkToolbarStencilKind stencilKind, bool check);
    void SetAllStencilItemsCheck(bool check);
    unsigned NumberOfStencils();
    bool IsAnyStencilSelected();
    void HookUpToStencilEvents(winrt::RoutedEventHandler const& handler, bool& eventHookedUp, winrt::event_token& token);
    void UpdateIcon(winrt::InkToolbarStencilKind kind);

protected:
    // IMenuButtonDerived override hooks (localized names are lift resource gaps -> empty).
    winrt::hstring GetLocalizedToolName() override;
    winrt::hstring GetFlyoutName() override;

private:
    void OnL3ItemsVisibilitiesChanged();
    void OnSelectedStencilChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void SetupL3(wchar_t const* itemName);
    bool GetIsItemVisible(winrt::InkToolbarStencilKind kind);
    void ConfigureStencilFlyoutItems(
        winrt::RoutedEventHandler const& handler,
        bool shouldHookupEvent,
        winrt::DependencyObject const& flyoutContent,
        winrt::InkToolbarStencilKind kind,
        winrt::event_token& token);

    winrt::event_token m_stencilItemCheckedRegistrationToken{};
};

