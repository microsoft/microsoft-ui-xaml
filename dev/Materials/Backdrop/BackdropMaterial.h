// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BackdropMaterial.g.h"
#include "BackdropMaterial.properties.h"

#include "MicaController.h"

class BackdropMaterial : public winrt::DependencyObjectT<BackdropMaterial>,
    public BackdropMaterialProperties
{
public:
    static void ClearProperties();
    static void EnsureProperties();

    static GlobalDependencyProperty s_StateProperty;

    static void OnApplyToRootOrPageBackgroundChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args);

private:

    struct BackdropMaterialState : winrt::implements<BackdropMaterialState, winrt::IInspectable>
    {
        BackdropMaterialState(winrt::Control const& target);
        ~BackdropMaterialState();

    private:
        void UpdateFallbackBrush();

        winrt::weak_ref<winrt::Control> m_target;
        winrt::FrameworkElement::ActualThemeChanged_revoker m_themeChangedRevoker;
        winrt::UISettings m_uiSettings{};
        bool m_isHighContrast{};
        winrt::AccessibilitySettings::HighContrastChanged_revoker m_highContrastChangedRevoker;
    };

    static void CreateOrDestroyMicaController();

    static thread_local int m_connectedBrushCount;
    static thread_local winrt::com_ptr<MicaController> m_micaController;
};
