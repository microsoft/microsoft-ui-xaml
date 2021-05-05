#include "pch.h"
#include "common.h"

#include "MicaController.h"
#include "SystemBackdropBrushFactory.h"

#include "MicaController.g.cpp"

MicaController::~MicaController()
{
    // If we are going away and we own the backdrop, clear it.
    if (m_target && m_target.SystemBackdrop() == m_currentBrush)
    {
        m_target.SystemBackdrop(nullptr);
    }
}


bool MicaController::SetTarget(winrt::Windows::UI::Xaml::Window const& xamlWindow)
{
    m_target = xamlWindow.try_as<winrt::ICompositionSupportsSystemBackdrop>();
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

    m_windowHandler = std::make_unique<SystemBackdropComponentInternal::XamlWindowHandler>(this, xamlWindow);

    // Ensure we are in the correct policy state only *after* creating the appropriate WindowHandler.
    // If we start in high contrast mode, the controller will query the window handler for the correct system fallback color.
    m_windowHandler->ActivateOrDeactivateController();

    return true;
}

void MicaController::TintColor(winrt::Windows::UI::Color const& value)
{
    m_tintColor = value;
        
    if (!m_isExplicitFallbackColorSet)
    {
        m_fallbackColor = value;
    }

    m_customColors = true;
    Update();
}
void MicaController::TintOpacity(float value)
{
    m_tintOpacity = value;
    m_customColors = true;
    Update();
}

void MicaController::LuminosityOpacity(float value)
{
    m_luminosityOpacity = value;
    m_customColors = true;
    Update();
}
    
void MicaController::FallbackColor(winrt::Windows::UI::Color const& value)
{
    m_fallbackColor = value;
    m_isExplicitFallbackColorSet = true;
    m_customColors = true;
    Update();
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
            m_tintColor = winrt::Windows::UI::ColorHelper::FromArgb(255, 243, 243, 243);
            m_tintOpacity = 0.65f;
            m_luminosityOpacity = 1.0f;
            break;

        case winrt::Windows::UI::Xaml::ElementTheme::Dark:
            m_tintColor = winrt::Windows::UI::ColorHelper::FromArgb(255, 32, 32, 32);
            m_tintOpacity = 0.8f;
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

    return (m_target &&
        m_compositor.try_as<winrt::ICompositorWithBlurredWallpaperBackdropBrush>() &&
        m_compositor.TryCreateBlurredWallpaperBackdropBrush());
}

void MicaController::Crossfade(const winrt::Windows::UI::Composition::CompositionBrush& newBrush)
{
    const winrt::CompositionBrush& oldBrush = m_target.SystemBackdrop();

    if (oldBrush == nullptr)
    {
        UpdateSystemBackdropBrush(newBrush);
        return;
    }

    if (oldBrush.Comment() == L"Crossfade")
    {
        // Stop previous animation.
        oldBrush.StopAnimation(L"Crossfade.Weight");
    }

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

void MicaController::UpdateSystemBackdropBrush(const winrt::CompositionBrush& brush)
{
    m_currentBrush = brush;
    m_target.SystemBackdrop(m_currentBrush);
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
