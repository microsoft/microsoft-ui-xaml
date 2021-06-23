#pragma once

#include "SystemBackdropWindowHandler.h"
#include "SystemBackdropController.h"

#include "MicaController.g.h"

struct MicaController : winrt::implementation::MicaControllerT<MicaController>, SystemBackdropComponentInternal::ISystemBackdropController
{
    MicaController() = default;
    ~MicaController();

    bool SetTarget(winrt::Windows::UI::Xaml::Window const& xamlWindow);


    // Property getters.
    winrt::Windows::UI::Color TintColor() { return m_tintColor; }
    float TintOpacity() { return m_tintOpacity; }
    float LuminosityOpacity() { return m_luminosityOpacity; }
    winrt::Windows::UI::Color FallbackColor() { return m_fallbackColor; }

    // Property setters.
    void TintColor(winrt::Windows::UI::Color const& value);
    void TintOpacity(float value);
    void LuminosityOpacity(float value);
    void FallbackColor(winrt::Windows::UI::Color const& value);

    // SystemBackdropComponent::ISystemBackdropController methods.
    void Activate() override;
    void Deactivate() override;
    void SetHighContrast(bool isHighContrast) override { m_isHighContrast = isHighContrast; }
    void UpdateTheme(winrt::Windows::UI::Xaml::ElementTheme theme) override;

    static constexpr auto sc_darkThemeColor = winrt::Color{ 255, 32, 32, 32 };
    static constexpr auto sc_darkThemeTintOpacity = 0.8f;

    static constexpr auto sc_lightThemeColor = winrt::Color{ 255, 243, 243, 243 };
    static constexpr auto sc_lightThemeTintOpacity = 0.65f;

private:
    bool IsMicaSupported() const;
    void Crossfade(const winrt::Windows::UI::Composition::CompositionBrush& newBrush);
    void Update();
    void UpdateSystemBackdropBrush(const winrt::CompositionBrush& brush);

    winrt::Windows::UI::Color m_tintColor{ winrt::Windows::UI::Colors::Teal() };
    float m_tintOpacity{ 0.05f };
    float m_luminosityOpacity{ 0.15f };
    winrt::Windows::UI::Color m_fallbackColor{ m_tintColor };

    winrt::CompositionBrush m_currentBrush{ nullptr };
    winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
    winrt::weak_ref<winrt::ICompositionSupportsSystemBackdrop> m_target{ nullptr };
    std::unique_ptr<SystemBackdropComponentInternal::BaseWindowHandler> m_windowHandler{ nullptr };

    bool m_isActive{ false };
    bool m_isExplicitFallbackColorSet{ false };
    bool m_isFirstRun{ true };
    bool m_isHighContrast{ false };
    bool m_isMicaSupported{ false };
    bool m_customColors{ false };
    bool m_currentlyUpdatingProperty{ false };
    bool m_propertyUpdated{ false };
    bool m_wasHighContrastOnLastDeactivation{ false };
};

struct MicaControllerFactory : winrt::factory_implementation::MicaControllerT<MicaControllerFactory, MicaController>
{
};

namespace winrt::Microsoft::UI::Private::Controls
{
    namespace factory_implementation { using MicaController = ::MicaControllerFactory; };
    namespace implementation { using MicaController = ::MicaController; };
}
