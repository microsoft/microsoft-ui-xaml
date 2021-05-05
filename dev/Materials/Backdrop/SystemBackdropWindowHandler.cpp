#include "pch.h"
#include "common.h"

#include "SystemBackdropWindowHandler.h"

namespace SystemBackdropComponentInternal
{
    void BaseWindowHandler::ActivateOrDeactivateController()
    {
        // This method calls back into the window handler in high contrast mode to retrieve the system backdrop color.
        // Hence we cannot call this method during construction.
        if (m_policy->IsActive())
        {
            m_controller->Activate();
        }
        else
        {
            m_controller->Deactivate();
        }
    }

    winrt::Windows::UI::Color BaseWindowHandler::GetHighContrastFallbackColor() const
    {
        return m_uiSettings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Background);
    }

    BaseWindowHandler::BaseWindowHandler(ISystemBackdropController* owningController) :
        m_controller{ owningController }
    {
        m_dispatcherQueue = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();

        m_policy = std::make_unique<SystemBackdropPolicyStateMachine>(SystemBackdropPolicyState::Active);

        m_powerManagerEventRevoker = winrt::Windows::System::Power::PowerManager::EnergySaverStatusChanged(winrt::auto_revoke, [&](auto&&, auto&&)
            {
                m_dispatcherQueue.TryEnqueue([&]()
                    {
                        if (winrt::Windows::System::Power::PowerManager::EnergySaverStatus() == winrt::Windows::System::Power::EnergySaverStatus::On)
                        {
                            m_policy->SetPowerSavingMode(true);
                        }
                        else
                        {
                            m_policy->SetPowerSavingMode(false);
                        }

                        ActivateOrDeactivateController();
                    });
            });

        m_capabilities = winrt::Windows::UI::Composition::CompositionCapabilities::GetForCurrentView();
        m_capabilitiesEventRevoker = { m_capabilities, m_capabilities.Changed([&](auto&&, auto&&)
            {
                m_dispatcherQueue.TryEnqueue([&]()
                    {
                        if (m_capabilities.AreEffectsFast())
                        {
                            m_policy->SetIncompatibleGraphicsDevice(false);
                        }
                        else
                        {
                            m_policy->SetIncompatibleGraphicsDevice(true);
                        }

                        ActivateOrDeactivateController();
                    });
            }) };

        m_uiSettings = winrt::Windows::UI::ViewManagement::UISettings();
        m_uiSettingsEventRevoker = m_uiSettings.AdvancedEffectsEnabledChanged(winrt::auto_revoke, [&](auto&&, auto&&)
            {
                m_dispatcherQueue.TryEnqueue([&]()
                    {
                        if (m_uiSettings.AdvancedEffectsEnabled())
                        {
                            m_policy->SetTransparencyDisabled(false);
                        }
                        else
                        {
                            m_policy->SetTransparencyDisabled(true);
                        }

                        ActivateOrDeactivateController();
                    });
            });
    }

    void BaseWindowHandler::InitializePolicy()
    {
        // Check battery saver mode.
        if (winrt::Windows::System::Power::PowerManager::EnergySaverStatus() == winrt::Windows::System::Power::EnergySaverStatus::On)
        {
            m_policy->SetPowerSavingMode(true);
        }

        // Check GPU capabilities.
        if (!m_capabilities.AreEffectsFast())
        {
            m_policy->SetIncompatibleGraphicsDevice(true);
        }

        // Check transparency setting.
        if (!m_uiSettings.AdvancedEffectsEnabled())
        {
            m_policy->SetTransparencyDisabled(true);
        }
    }


    CoreWindowHandler::CoreWindowHandler(ISystemBackdropController* owningController, winrt::Windows::UI::Core::CoreWindow const& coreWindow) :
        BaseWindowHandler(owningController),
        m_coreWindow(coreWindow)
    {
        // Handle window activation.
        m_coreWindowActivationEventRevoker = m_coreWindow.Activated(winrt::auto_revoke, [&](auto&&, auto&&)
            {
                m_dispatcherQueue.TryEnqueue([&]()
                    {
                        if (m_coreWindow.ActivationMode() != winrt::Windows::UI::Core::CoreWindowActivationMode::ActivatedInForeground)
                        {
                            m_policy->SetWindowNotInFocus(true);
                        }
                        else
                        {
                            m_policy->SetWindowNotInFocus(false);
                        }

                        ActivateOrDeactivateController();
                    });
            });

        // Handle high contrast.
        m_accessibilitySettings = winrt::Windows::UI::ViewManagement::AccessibilitySettings();
        m_accessibilitySettingsEventRevoker = m_accessibilitySettings.HighContrastChanged(winrt::auto_revoke, [&](auto&&, auto&&)
            {
                m_dispatcherQueue.TryEnqueue([&]()
                    {
                        bool isHighContrast = m_accessibilitySettings.HighContrast();
                        m_policy->SetHighContrastMode(isHighContrast);

                        // Controller needs to know high contrast is active to pick the system level fallback color.
                        m_controller->SetHighContrast(isHighContrast);

                        ActivateOrDeactivateController();
                    });
            });

        InitializePolicy();
    }

    void CoreWindowHandler::InitializePolicy()
    {
        BaseWindowHandler::InitializePolicy();

        // Check window in focus.
        if (m_coreWindow.ActivationMode() != winrt::Windows::UI::Core::CoreWindowActivationMode::ActivatedInForeground)
        {
            m_policy->SetWindowNotInFocus(true);
        }

        // Check high contrast.
        if (m_accessibilitySettings.HighContrast())
        {
            m_policy->SetHighContrastMode(true);

            // Controller needs to know high contrast is active to pick the system level fallback color.
            m_controller->SetHighContrast(true);
        }
    }

    XamlWindowHandler::XamlWindowHandler(ISystemBackdropController* owningController, winrt::Windows::UI::Xaml::Window const& xamlWindow) :
        CoreWindowHandler(owningController, xamlWindow.CoreWindow()),
        m_xamlWindow(xamlWindow)
    {
        InitializePolicy();

        // Attach to theme events.
        AttachToThemeChanged();
    }

    void XamlWindowHandler::AttachToThemeChanged(bool retryOnNextTick)
    {
        if (auto root = m_xamlWindow.Content().try_as<winrt::Windows::UI::Xaml::FrameworkElement>())
        {
            m_themeEventRevoker = root.ActualThemeChanged(winrt::auto_revoke, [this](auto&&, auto&&)
                {
                RefreshTheme();
                });

            // On first run, we must update the theme so that the controller has the correct configuration.
            m_actualTheme = root.ActualTheme();
            m_controller->UpdateTheme(m_actualTheme);
        }
        else if (retryOnNextTick && !m_nextTickRevoker)
        {
            // Try again after the next tick.
            m_nextTickRevoker = winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering(winrt::auto_revoke, [this](auto&&, auto&&) {
                m_nextTickRevoker = {};
                AttachToThemeChanged(true);
                });
        }
    }

    void XamlWindowHandler::RefreshTheme()
    {
        if (auto root = m_xamlWindow.Content().try_as<winrt::Windows::UI::Xaml::FrameworkElement>())
        {
            const auto actualTheme = root.ActualTheme();
            if (m_actualTheme != actualTheme)
            {
                m_actualTheme = actualTheme;
                m_controller->UpdateTheme(m_actualTheme);
            }
        }
    }
}
