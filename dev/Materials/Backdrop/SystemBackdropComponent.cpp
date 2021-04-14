#include "pch.h"
#include "common.h"

#include "SystemBackdropComponent.h"
#include "SystemBackdropBrushFactory.h"

#include "MicaController.g.cpp"

//namespace winrt::SystemBackdropComponent::implementation
//{
#if NEVER
    bool MicaController::SetTarget(Windows::UI::WindowId const& windowId, Windows::UI::Composition::CompositionTarget const& desktopWindowTarget)
    {
        m_target = desktopWindowTarget.try_as<winrt::ICompositionSupportsSystemBackdropImplementation>();
        m_target2 = desktopWindowTarget.try_as<winrt::SystemBackdropComponent::ICompositionSupportsSystemBackdropLatest>();
        m_compositor = desktopWindowTarget.Compositor();

        WINRT_ASSERT(m_compositor != nullptr);

        // Immediately check if the current OS can support the Mica APIs.
        if (IsMicaSupportedOnCurrentOS())
        {
            m_isMicaSupported = true;
        }
        else
        {
            return false;
        }

        m_windowHandler = std::make_unique<SystemBackdropComponentInternal::Win32HwndHandler>(this, windowId);

        // Ensure we are in the correct policy state only *after* creating the appropriate WindowHandler.
        // If we start in high contrast mode, the controller will query the window handler for the correct system fallback color.
        m_windowHandler->ActivateOrDeactivateController();

        return true;
    }
    
    bool MicaController::SetTarget(Windows::UI::Core::CoreWindow const& coreWindow, Windows::UI::Composition::CompositionTarget const& compositionTarget)
    {
        m_target = compositionTarget.try_as<winrt::ICompositionSupportsSystemBackdropImplementation>();
        m_target2 = compositionTarget.try_as<winrt::SystemBackdropComponent::ICompositionSupportsSystemBackdropLatest>();
        m_compositor = compositionTarget.Compositor();

        WINRT_ASSERT(m_compositor != nullptr);

        // Immediately check if the current OS can support the Mica APIs.
        if (IsMicaSupportedOnCurrentOS())
        {
            m_isMicaSupported = true;
        }
        else
        {
            return false;
        }

        m_windowHandler = std::make_unique<SystemBackdropComponentInternal::CoreWindowHandler>(this, coreWindow);

        // Ensure we are in the correct policy state only *after* creating the appropriate WindowHandler.
        // If we start in high contrast mode, the controller will query the window handler for the correct system fallback color.
        m_windowHandler->ActivateOrDeactivateController();
        
        return true;
    }
#endif

    bool MicaController::SetTarget(winrt::Windows::UI::Xaml::Window const& xamlWindow)
    {
        m_target = xamlWindow.try_as<winrt::ICompositionSupportsSystemBackdropImplementation>();
        m_target2 = xamlWindow.try_as<winrt::ICompositionSupportsSystemBackdropLatest>();
        m_compositor = xamlWindow.Compositor();

        WINRT_ASSERT(m_compositor != nullptr);

        // Immediately check if the current OS can support the Mica APIs.
        if (IsMicaSupportedOnCurrentOS())
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

#if NEVER
    void MicaController::SniffWindowMessage(uint32_t message, uint64_t wparam, int64_t lparam)
    {
        if (m_target == nullptr)
        {
            // Nothing to do if no target is set.
            return;
        }

        SystemBackdropComponentInternal::Win32HwndHandler* hwndHandler = dynamic_cast<SystemBackdropComponentInternal::Win32HwndHandler*>(m_windowHandler.get());
        if (hwndHandler == nullptr)
        {
            // The controller is not being used in a legacy Win32 Hwnd, do nothing.
            return;
        }

        hwndHandler->SniffWindowMessage(message, (WPARAM)wparam, (LPARAM)lparam);
    }
#endif
        
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
            // Already in the appopriate fallback state, return.
            return;
        }

        // If we are in high contrast mode, the system fallback color might have changed, so we have to always check.
        auto fallbackColor = m_isHighContrast ? m_windowHandler->GetHighContrastFallbackColor() : m_fallbackColor;

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

    bool MicaController::IsMicaSupportedOnCurrentOS() const
    {
        WINRT_ASSERT(m_compositor != nullptr);

        return ((m_target || m_target2) &&
            (m_compositor.try_as<winrt::ICompositorWithBlurredWallpaperBackdropBrush>() != nullptr) &&
            (m_compositor.TryCreateBlurredWallpaperBackdropBrush() != nullptr));
    }

    void MicaController::Crossfade(const winrt::Windows::UI::Composition::CompositionBrush& newBrush)
    {
        const winrt::CompositionBrush& oldBrush = m_target ? m_target.SystemBackdrop() : m_target2.SystemBackdrop();

        if (oldBrush == nullptr)
        {
            if (m_target)
                m_target.SystemBackdrop(newBrush);
            else
                m_target2.SystemBackdrop(newBrush);
            return;
        }

        if (oldBrush.Comment() == L"Crossfade")
        {
            // Stop previous animation.
            oldBrush.StopAnimation(L"Crossfade.Weight");
        }

        const winrt::CompositionBrush crossFadeBrush = SystemBackdropComponentInternal::CreateCrossFadeEffectBrush(m_compositor, oldBrush, newBrush);
        winrt::ScalarKeyFrameAnimation animation = SystemBackdropComponentInternal::CreateCrossFadeAnimation(m_compositor);
        if (m_target)
            m_target.SystemBackdrop(crossFadeBrush);
        else
            m_target2.SystemBackdrop(crossFadeBrush);

        const auto crossFadeAnimationBatch = m_compositor.CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        crossFadeBrush.StartAnimation(L"Crossfade.Weight", animation);
        crossFadeAnimationBatch.End();

        crossFadeAnimationBatch.Completed([weakThis = get_weak(), newBrush](auto&&, auto&&)
            {
                if (auto self = weakThis.get())
                {
                    if (self->m_target)
                        self->m_target.SystemBackdrop(newBrush);
                    else
                        self->m_target2.SystemBackdrop(newBrush);
                }
            });
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

                    if (self->m_target || self->m_target2)
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
//}
