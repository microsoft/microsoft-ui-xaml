﻿#include "pch.h"
#include "common.h"

#include "MicaController.h"
#include "RuntimeProfiler.h"
#include "SystemBackdropBrushFactory.h"

#include "MicaController.g.cpp"

MicaController::MicaController()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_MicaController);
}

MicaController::~MicaController()
{
    if (auto target = m_target.get())
    {
        try
        {
            if (auto xamlWindow = target.try_as<winrt::Windows::UI::Xaml::Window>())
            {
                // Workaround for null ref exception in Window::get_SystemBackdrop when the Window is shutting down.
                // GenerateRawElementProviderRuntimeId will trigger an exception caught below and thus prevent the 
                // crashing target.SystemBackdrop() call.
                auto const runtimeId = winrt::Automation::Peers::AutomationPeer::GenerateRawElementProviderRuntimeId();
            }

            // If we are going away and we own the backdrop, clear it.
            if (target.SystemBackdrop() == m_currentBrush)
            {
                target.SystemBackdrop(nullptr);
            }
        }
        catch (winrt::hresult_error)
        {
            // If we are called during shutdown then getting SystemBackdrop will fail with E_UNEXPECTED
        }
    }
}


bool MicaController::SetTarget(winrt::Windows::UI::Xaml::Window const& xamlWindow)
{
    m_target = xamlWindow.try_as<winrt::Microsoft::UI::Private::Controls::ICompositionSupportsSystemBackdrop>();
    m_compositor = xamlWindow.Compositor();

    WINRT_ASSERT(m_compositor);

    // Immediately check if the current OS can support the Mica APIs.
    if (IsMicaSupported())
    {
        m_isMicaSupported = true;
    }
    else
    {
        return false;
    }

    m_windowHandler = winrt::make_self<SystemBackdropComponentInternal::XamlWindowHandler>(this, xamlWindow);

    // Ensure we are in the correct policy state only *after* creating the appropriate WindowHandler.
    // If we start in high contrast mode, the controller will query the window handler for the correct system fallback color.
    m_windowHandler->ActivateOrDeactivateController();

    return true;
}

void MicaController::TintColor(winrt::Windows::UI::Color const& value)
{
    if (m_tintColor != value)
    {
        m_tintColor = value;

        if (!m_isExplicitFallbackColorSet)
        {
            m_fallbackColor = value;
        }

        m_customColors = true;
        Update();
    }
}
void MicaController::TintOpacity(float value)
{
    if (m_tintOpacity != value)
    {
        m_tintOpacity = value;
        m_customColors = true;
        Update();
    }
}

void MicaController::LuminosityOpacity(float value)
{
    if (m_luminosityOpacity != value)
    {
        m_luminosityOpacity = value;
        m_customColors = true;
        Update();
    }
}
    
void MicaController::FallbackColor(winrt::Windows::UI::Color const& value)
{
    if (m_fallbackColor != value)
    {
        m_fallbackColor = value;
        m_isExplicitFallbackColorSet = true;
        m_customColors = true;
        Update();
    }
}

void MicaController::Activate()
{
    if (!m_isMicaSupported)
    {
        // We cannot draw Mica, so immediately transition to the appropriate deactivated (solid color fallback) state.
        // We still need to react to high contrast changes, hence we go through the Deactivate method that handles this.
        Deactivate();
        return;
    }

    if (m_isActive && !m_propertyUpdated)
    {
        // Already active, return.
        return;
    }

    const winrt::CompositionBrush& newBrush = SystemBackdropComponentInternal::BuildMicaEffectBrush(m_compositor, m_tintColor, m_tintOpacity, m_luminosityOpacity);

    Crossfade(newBrush);

    m_isActive = true;
    m_isFirstRun = false;
}

void MicaController::Deactivate()
{
    if (!m_wasHighContrastOnLastDeactivation && !m_isHighContrast && !m_isActive && !m_isFirstRun && !m_propertyUpdated)
    {
        // Already in the appropriate fallback state, return.
        return;
    }

    // If we are in high contrast mode, the system fallback color might have changed, so we have to always check.
    const auto fallbackColor = m_isHighContrast ? m_windowHandler->GetHighContrastFallbackColor() : m_fallbackColor;

    const winrt::CompositionBrush& newBrush = m_compositor.CreateColorBrush(fallbackColor);

    Crossfade(newBrush);

    m_isActive = false;
    m_isFirstRun = false;
    m_wasHighContrastOnLastDeactivation = m_isHighContrast;
}

void MicaController::UpdateTheme(winrt::Windows::UI::Xaml::ElementTheme theme)
{
    if (!m_customColors)
    {
        switch (theme)
        {
        case winrt::Windows::UI::Xaml::ElementTheme::Light:
            m_tintColor = sc_lightThemeColor;
            m_tintOpacity = sc_lightThemeTintOpacity;
            m_luminosityOpacity = 1.0f;
            break;

        case winrt::Windows::UI::Xaml::ElementTheme::Dark:
            m_tintColor = sc_darkThemeColor;
            m_tintOpacity = sc_darkThemeTintOpacity;
            m_luminosityOpacity = 1.0f;
            break;
        }

        m_fallbackColor = m_tintColor;

        Update();
    }
}

bool MicaController::IsMicaSupported() const
{
    WINRT_ASSERT(m_compositor);

    if (auto blurredWallpaperBackdropBrush = m_compositor.try_as<winrt::Microsoft::UI::Private::Controls::ICompositorWithBlurredWallpaperBackdropBrush>())
    {
        return m_target && blurredWallpaperBackdropBrush.TryCreateBlurredWallpaperBackdropBrush();
    }

    return false;
}

void MicaController::Crossfade(const winrt::Windows::UI::Composition::CompositionBrush& newBrush)
{
    const winrt::CompositionBrush& oldBrush = m_target.get().SystemBackdrop();

    // Immediately set the new brush if:
    // 1) We don't have an old brush
    // 2) There was a cross fade happening, just jump to the new brush
    // 3) Both brushes are solid color (theme change or startup scenario), this doesn't need an animation
    if (  !oldBrush
        || (oldBrush.Comment() == L"Crossfade")
        || (oldBrush.try_as<winrt::ICompositionColorBrush>() && newBrush.try_as<winrt::ICompositionColorBrush>()))
    {
        UpdateSystemBackdropBrush(newBrush);
    }
    else
    {

        const winrt::CompositionBrush crossFadeBrush = SystemBackdropComponentInternal::CreateCrossFadeEffectBrush(m_compositor, oldBrush, newBrush);
        winrt::ScalarKeyFrameAnimation animation = SystemBackdropComponentInternal::CreateCrossFadeAnimation(m_compositor);
        UpdateSystemBackdropBrush(crossFadeBrush);

        const auto crossFadeAnimationBatch = m_compositor.CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        crossFadeBrush.StartAnimation(L"Crossfade.Weight", animation);
        crossFadeAnimationBatch.End();

        crossFadeAnimationBatch.Completed([weakThis = get_weak(), newBrush](auto&&, auto&&)
        {
            if (auto self = weakThis.get())
            {
                self->UpdateSystemBackdropBrush(newBrush);
            }
        });
    }
}

void MicaController::UpdateSystemBackdropBrush(const winrt::CompositionBrush& brush)
{
    m_currentBrush = brush;
    m_target.get().SystemBackdrop(m_currentBrush);
}

void MicaController::Update()
{
    if (!m_currentlyUpdatingProperty)
    {
        m_currentlyUpdatingProperty = true;
        auto queue = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
        queue.TryEnqueue([weakThis = get_weak()]() {
            if (auto self = weakThis.get())
            {
                self->m_currentlyUpdatingProperty = false;
                self->m_propertyUpdated = true;

                if (self->m_target)
                {
                    if (self->m_isMicaSupported && self->m_isActive)
                    {
                        self->Activate();
                    }
                    else
                    {
                        self->Deactivate();
                    }
                }

                self->m_propertyUpdated = false;
            }
        });
    }
}
