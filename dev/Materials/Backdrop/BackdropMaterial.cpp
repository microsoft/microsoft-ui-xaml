// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BackdropMaterial.h"


thread_local int BackdropMaterial::m_connectedBrushCount{};
thread_local winrt::com_ptr<MicaController> BackdropMaterial::m_micaController{ nullptr };

GlobalDependencyProperty BackdropMaterial::s_StateProperty{ nullptr };

void BackdropMaterial::ClearProperties()
{
    s_StateProperty = nullptr;
    BackdropMaterialProperties::ClearProperties();
}

void BackdropMaterial::EnsureProperties()
{
    BackdropMaterialProperties::EnsureProperties();
    if (!s_StateProperty)
    {
        s_StateProperty =
            InitializeDependencyProperty(
                L"State",
                winrt::name_of<bool>(),
                winrt::name_of<winrt::BackdropMaterial>(),
                true /* isAttached */,
                box_value(false),
                nullptr);
    }
}

void BackdropMaterial::OnApplyToRootOrPageBackgroundChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (const auto control = sender.try_as<winrt::Control>())
    {
        // When the ApplyToRootOrPageBackgroundChanged property is set on a control, create a
        // object to attach to that element in a "secret" slot called BackdropMaterial.State.
        // This object's lifetime manages the MicaController registration or ownership of the
        // assignment of the Background property.
        if (const bool value = BackdropMaterialProperties::GetApplyToRootOrPageBackground(control))
        {
            control.SetValue(s_StateProperty, winrt::make<BackdropMaterialState>(control));
        }
        else
        {
            if (auto state = winrt::get_self<BackdropMaterialState>(control.GetValue(s_StateProperty).try_as<winrt::IBackdropMaterialState>()))
            {
                state->Dispose();
            }

            control.ClearValue(s_StateProperty);
        }
    }
}

void BackdropMaterial::CreateOrDestroyMicaController()
{
    // If we are connecting the first BackdropMaterial on this thread, create and configure the MicaController.
    // Or if we're disconnecting the last one, clean up the shared MicaController.
    if (m_connectedBrushCount > 0 && !m_micaController)
    {
        auto currentWindow = winrt::Window::Current();

        m_micaController = winrt::make_self<MicaController>();
        if (!m_micaController->SetTarget(currentWindow))
        {
            m_micaController = nullptr;
        }
    }
    else if (m_connectedBrushCount == 0 && m_micaController)
    {
        m_micaController = nullptr;
    }
}

BackdropMaterial::BackdropMaterialState::BackdropMaterialState(winrt::Control const& target)
{
    m_target = winrt::make_weak(target);

    // Track whether we're connected and update the number of connected BackdropMaterial on this thread.
    m_connectedBrushCount++;
    CreateOrDestroyMicaController();

    // Normally QI would be fine, but .NET is lying about implementing this interface (e.g. C# TestFrame derives from Frame and this QI
    // returns success even on RS2, but it's not implemented by XAML until RS3).
    if (SharedHelpers::IsRS3OrHigher()) 
    {
        if (auto targetThemeChanged = target.try_as<winrt::IFrameworkElement6>())
        {
            m_themeChangedRevoker = targetThemeChanged.ActualThemeChanged(winrt::auto_revoke, [this](auto&&, auto&&)
                {
                    UpdateFallbackBrush();
                });
        }
    }

    // Listen for High Contrast changes
    auto accessibilitySettings = winrt::AccessibilitySettings();
    m_isHighContrast = accessibilitySettings.HighContrast();
    m_highContrastChangedRevoker = accessibilitySettings.HighContrastChanged(winrt::auto_revoke,
        [this, accessibilitySettings](auto& sender, auto& args)
        {
            m_isHighContrast = accessibilitySettings.HighContrast();
            UpdateFallbackBrush();
        });

    UpdateFallbackBrush();
}

BackdropMaterial::BackdropMaterialState::~BackdropMaterialState()
{
    Dispose();
}

void BackdropMaterial::BackdropMaterialState::Dispose()
{
    if (!m_isDisposed)
    {
        m_isDisposed = true;
        m_connectedBrushCount--;
        CreateOrDestroyMicaController();
    }
}

void BackdropMaterial::BackdropMaterialState::UpdateFallbackBrush()
{
    if (auto target = m_target.get())
    {
        if (!m_micaController)
        {
            // When not using mica, use the theme and high contrast states to determine the fallback color.
            const auto theme = [=]() {
                // See other IsRS3OrHigher usage for comment explaining why the version check and QI.
                if (SharedHelpers::IsRS3OrHigher())
                {
                    if (auto targetTheme = target.try_as<winrt::IFrameworkElement6>())
                    {
                        return targetTheme.ActualTheme();
                    }
                }

                const auto value = m_uiSettings.GetColorValue(winrt::UIColorType::Background);
                if (value.B == 0)
                {
                    return winrt::ElementTheme::Dark;
                }

                return winrt::ElementTheme::Light;
            }();

            const auto color = [=]()
            {
                if (m_isHighContrast)
                {
                    return m_uiSettings.GetColorValue(winrt::UIColorType::Background);
                }

                if (theme == winrt::ElementTheme::Dark)
                {
                    return MicaController::sc_darkThemeColor;
                }
                else
                {
                    return MicaController::sc_lightThemeColor;
                }
            }();

            target.Background(winrt::SolidColorBrush(color));
        }
        else
        {
            // When Mica is involved, use transparent for the background (this is so that the hit testing
            // behavior is consistent with/without the material).
            target.Background(winrt::SolidColorBrush(winrt::Color{0,0,0,0}));
        }
    }
}
