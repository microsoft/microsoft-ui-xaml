#pragma once

#include "SystemBackdropController.h"
#include "SystemBackdropPolicyStateMachine.h"

namespace SystemBackdropComponentInternal
{
    struct CompositionCapabilitiesChanged_revoker
    {
        CompositionCapabilitiesChanged_revoker() noexcept = default;
        CompositionCapabilitiesChanged_revoker(CompositionCapabilitiesChanged_revoker const&) = delete;
        CompositionCapabilitiesChanged_revoker& operator=(CompositionCapabilitiesChanged_revoker const&) = delete;
        CompositionCapabilitiesChanged_revoker(CompositionCapabilitiesChanged_revoker&& other) noexcept
        {
            move_from(other);
        }

        CompositionCapabilitiesChanged_revoker& operator=(CompositionCapabilitiesChanged_revoker&& other) noexcept
        {
            move_from(other);
            return *this;
        }

        CompositionCapabilitiesChanged_revoker(winrt::Windows::UI::Composition::CompositionCapabilities const& capabilities, winrt::event_token token) :
            m_capabilities(capabilities),
            m_token(token) {}

        ~CompositionCapabilitiesChanged_revoker() noexcept
        {
            revoke();
        }

        void revoke() noexcept
        {
            if (!m_capabilities)
            {
                return;
            }
            m_capabilities.Changed(m_token);
            m_capabilities = nullptr;
        }

        explicit operator bool() const noexcept
        {
            return static_cast<bool>(m_capabilities);
        }
    private:
        void move_from(CompositionCapabilitiesChanged_revoker& other)
        {
            if (this != &other)
            {
                revoke();
                m_capabilities = other.m_capabilities;
                m_token = other.m_token;
                other.m_capabilities = nullptr;
                other.m_token.value = 0;
            }
        }

        winrt::Windows::UI::Composition::CompositionCapabilities m_capabilities{ nullptr };
        winrt::event_token m_token{};
    };

    struct BaseWindowHandler
    {
        virtual ~BaseWindowHandler() {}

        void ActivateOrDeactivateController();

        winrt::Windows::UI::Color GetHighContrastFallbackColor() const;

    protected:
        BaseWindowHandler(ISystemBackdropController* owningController);

        virtual void InitializePolicy();

        winrt::Windows::System::DispatcherQueue m_dispatcherQueue{ nullptr };
        std::unique_ptr<SystemBackdropPolicyStateMachine> m_policy{ nullptr };
        ISystemBackdropController* m_controller{ nullptr };

        winrt::Windows::System::Power::PowerManager::EnergySaverStatusChanged_revoker m_powerManagerEventRevoker;

        winrt::Windows::UI::Composition::CompositionCapabilities m_capabilities{ nullptr };
        CompositionCapabilitiesChanged_revoker m_capabilitiesEventRevoker;

        winrt::Windows::UI::ViewManagement::UISettings m_uiSettings{ nullptr };
        winrt::Windows::UI::ViewManagement::UISettings::AdvancedEffectsEnabledChanged_revoker m_uiSettingsEventRevoker;
    };

#if NEVER
    struct Win32HwndHandler : public BaseWindowHandler
    {
        Win32HwndHandler(ISystemBackdropController* owningController, winrt::Windows::UI::WindowId const& windowId);

        void InitializePolicy() override;

        void SniffWindowMessage(const UINT message, const WPARAM wparam, const LPARAM lparam);
    
    private:
        HWND m_hwnd{ nullptr };
    };
#endif

    struct CoreWindowHandler : public BaseWindowHandler
    {
        CoreWindowHandler(ISystemBackdropController* owningController, winrt::Windows::UI::Core::CoreWindow const& coreWindow);

        void InitializePolicy() override;

    private:
        winrt::Windows::UI::Core::CoreWindow m_coreWindow{ nullptr };
        winrt::Windows::UI::Core::CoreWindow::Activated_revoker m_coreWindowActivationEventRevoker;

        winrt::Windows::UI::ViewManagement::AccessibilitySettings m_accessibilitySettings{ nullptr };
        winrt::Windows::UI::ViewManagement::AccessibilitySettings::HighContrastChanged_revoker m_accessibilitySettingsEventRevoker;
    };

    struct XamlWindowHandler : public CoreWindowHandler
    {
        XamlWindowHandler(ISystemBackdropController* owningController, winrt::Windows::UI::Xaml::Window const& xamlWindow);

    private:
        void AttachToThemeChanged(bool retryOnNextTick = true);
        void RefreshTheme();

        winrt::Windows::UI::Xaml::Window m_xamlWindow{ nullptr };

        winrt::Windows::UI::Xaml::ElementTheme m_actualTheme{ winrt::Windows::UI::Xaml::ElementTheme::Light };
        winrt::Windows::UI::Xaml::FrameworkElement::ActualThemeChanged_revoker m_themeEventRevoker;

        winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering_revoker m_nextTickRevoker;
    };
}
