// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RpcClient.h"

namespace Private { namespace Infrastructure {

    class ThemingHelper : public Microsoft::WRL::RuntimeClass<test_infra::IThemingHelper>
    {
        InspectableClass(RuntimeClass_Private_Infrastructure_ThemingHelper, TrustLevel::BaseTrust);

    public:
        IFACEMETHOD(RuntimeClassInitialize)();

        IFACEMETHOD(SetApplicationRequestedTheme)(_In_ xaml::ApplicationTheme theme) override;
        IFACEMETHOD(UnsetApplicationRequestedTheme)() override;
        IFACEMETHOD(SimulateThemeChanged)() override;
        IFACEMETHOD(RestoreDefaultState)() override;
        IFACEMETHOD(RemoveThemingOverrides)() override;

        IFACEMETHOD(put_SystemTheme)(_In_ xaml::ApplicationTheme theme) override;
        IFACEMETHOD(get_SystemTheme)(_Out_ xaml::ApplicationTheme* theme) override;
        IFACEMETHOD(put_HighContrastTheme)(_In_ test_infra::HighContrastTheme theme) override;
        IFACEMETHOD(get_HighContrastTheme)(_Out_ test_infra::HighContrastTheme* theme) override;
        IFACEMETHOD(put_AccentColor)(_In_ UINT32 accentColor) override;
        IFACEMETHOD(get_AccentColor)(_Out_ UINT32* accentColor) override;

        static bool IsHighContrastColor(_In_ test_infra::HighContrastTheme theme, unsigned int colorValue);
        static unsigned int GetTestAccentColorValue()
        {
            return 0xFF5B2EC5;
        }

    private:
        void EnsureStylesAreLoaded();

        std::shared_ptr<std::list<std::pair<int, unsigned int>>> GetHighContrastPalette(test_infra::HighContrastTheme theme);

        // High-contrast color palettes.
        std::shared_ptr<std::list<std::pair<int, unsigned int>>> m_testHighContrastPalette;
        std::shared_ptr<std::list<std::pair<int, unsigned int>>> m_defaultHighContrastPalette;
        std::shared_ptr<std::list<std::pair<int, unsigned int>>> m_blackHighContrastPalette;
        std::shared_ptr<std::list<std::pair<int, unsigned int>>> m_whiteHighContrastPalette;
        std::shared_ptr<std::list<std::pair<int, unsigned int>>> m_customHighContrastPalette;

        xaml::ApplicationTheme m_systemTheme;
        test_infra::HighContrastTheme m_highContrastTheme;
        UINT32 m_accentColor;

    }; // class ThemingHelper

} } // namespace Private::Infrastructure
