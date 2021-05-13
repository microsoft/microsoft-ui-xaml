// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BackdropMaterial.g.h"
#include "BackdropMaterial.properties.h"
#include "BackdropMaterialState.g.h"

#include "MicaController.h"

class BackdropMaterial : public winrt::implementation::BackdropMaterialT<BackdropMaterial>,
    public BackdropMaterialProperties
{
public:
    static void ClearProperties();
    static void EnsureProperties();

    static GlobalDependencyProperty s_StateProperty;

    static void OnApplyToRootOrPageBackgroundChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args);

    // Test helper
    static com_ptr<MicaController> GetMicaController() { return m_micaController; }

private:

    // This object gets attached to the target of the ApplyToRootOrPageBackground property to track additional
    // state that needs to be cleaned up if that target ever goes away.
    struct BackdropMaterialState : winrt::DependencyObjectT<BackdropMaterialState, winrt::IBackdropMaterialState>
    {
        BackdropMaterialState(winrt::Control const& target);
        ~BackdropMaterialState();

        // We're using this runtime instance in a place that it might leak out and .NET Core gets upset when
        // it sees types not in the public surface area. Return object since no one needs to know the real type.
        hstring GetRuntimeClassName() const
        {
            return winrt::hstring_name_of<winrt::IInspectable>();
        }

        void Dispose();
    private:

        void UpdateFallbackBrush();

        DispatcherHelper m_dispatcherHelper{ *this };
        winrt::weak_ref<winrt::Control> m_target;
        winrt::FrameworkElement::ActualThemeChanged_revoker m_themeChangedRevoker;
        winrt::UISettings::ColorValuesChanged_revoker m_colorValuesChangedRevoker;
        winrt::UISettings m_uiSettings{};
        winrt::AccessibilitySettings::HighContrastChanged_revoker m_highContrastChangedRevoker;
        bool m_isHighContrast{};
        bool m_isDisposed{};
    };

    static void CreateOrDestroyMicaController();

    static thread_local int m_connectedBrushCount;
    static thread_local winrt::com_ptr<MicaController> m_micaController;
};
