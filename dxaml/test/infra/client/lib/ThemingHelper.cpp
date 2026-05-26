// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ThemingHelper.h"

#include <XamlTailored.h>

#include "WindowHelper.h"
#include "IXamlTestHooks-win.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private { namespace Infrastructure {

    HRESULT ThemingHelper::RuntimeClassInitialize()
    {
        COM_START
        {
            // Override the theme/accent color to make sure it's consistent
            // regardless of the environment.
            RestoreDefaultState();
        }
        COM_END
    }

    HRESULT ThemingHelper::SetApplicationRequestedTheme(_In_ xaml::ApplicationTheme theme)
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->SetApplicationRequestedTheme(theme);
            });
        }
        COM_END
    }

    HRESULT ThemingHelper::UnsetApplicationRequestedTheme()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->UnsetApplicationRequestedTheme();
            });
        }
        COM_END
    }

    HRESULT ThemingHelper::SimulateThemeChanged()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                windowTestHooks->SimulateThemeChanged();
            });
        }
        COM_END
    }

    HRESULT ThemingHelper::RestoreDefaultState()
    {
        COM_START
        {
            // Because we may unload the theme resource information for memory leak detection, we
            // always need to ensure it loaded.
            EnsureStylesAreLoaded();

            LogThrow_IfFailed(put_SystemTheme(xaml::ApplicationTheme_Dark));
            LogThrow_IfFailed(put_HighContrastTheme(test_infra::HighContrastTheme_None));
            LogThrow_IfFailed(put_AccentColor(GetTestAccentColorValue()));
        }
        COM_END
    }

    HRESULT ThemingHelper::put_SystemTheme(_In_ xaml::ApplicationTheme theme)
    {
        COM_START
        {
            m_systemTheme = theme;

            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                LogThrow_IfFailed(windowTestHooks->OverrideSystemTheme(m_systemTheme));
            });
        }
        COM_END
    }

    HRESULT ThemingHelper::get_SystemTheme(_Out_ xaml::ApplicationTheme* theme)
    {
        *theme = m_systemTheme;
        return S_OK;
    }

    HRESULT ThemingHelper::put_HighContrastTheme(_In_ test_infra::HighContrastTheme theme)
    {
        COM_START
        {
            m_highContrastTheme = theme;

            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                LogThrow_IfFailed(windowTestHooks->OverrideHighContrast(GetHighContrastPalette(m_highContrastTheme)));
            });
        }
        COM_END
    }

    HRESULT ThemingHelper::get_HighContrastTheme(_Out_ test_infra::HighContrastTheme* theme)
    {
        *theme = m_highContrastTheme;
        return S_OK;
    }

    HRESULT ThemingHelper::put_AccentColor(_In_ UINT32 accentColor)
    {
        COM_START
        {
            m_accentColor = accentColor;

            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                LogThrow_IfFailed(windowTestHooks->OverrideAccentColor(m_accentColor));
            });
        }
        COM_END
    }

    HRESULT ThemingHelper::RemoveThemingOverrides()
    {
        COM_START
        {
            RunOnUIThread([&]()
            {
                wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
                LogThrow_IfFailed(windowTestHooks->RemoveThemingOverrides());
            });
        }
        COM_END
    }

    HRESULT ThemingHelper::get_AccentColor(_Out_ UINT32* accentColor)
    {
        *accentColor = m_accentColor;
        return S_OK;
    }

    void ThemingHelper::EnsureStylesAreLoaded()
    {
        // Set the language on an object to force styles to get loaded.
        RunOnUIThread([&]()
        {
            wrl::ComPtr<wg::IApplicationLanguagesStatics> spApplicationLanguagesStatics;
            wrl::ComPtr<wfc::IVectorView<HSTRING>> spLanguagesVector;
            wrl::Wrappers::HString strPrimaryLanguage;

            LogThrow_IfFailed(wf::GetActivationFactory(
                wrl::Wrappers::HStringReference(RuntimeClass_Windows_Globalization_ApplicationLanguages).Get(), &spApplicationLanguagesStatics));
            LogThrow_IfFailed(spApplicationLanguagesStatics->get_Languages(&spLanguagesVector));
            LogThrow_IfFailed(spLanguagesVector->GetAt(0, strPrimaryLanguage.ReleaseAndGetAddressOf()));

            wrl::ComPtr<xaml::IFrameworkElement> spFrameworkElement;
            LogThrow_IfFailed(wf::ActivateInstance(
                wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_FrameworkElement).Get(), &spFrameworkElement));
            LogThrow_IfFailed(spFrameworkElement->put_Language(strPrimaryLanguage.Get()));
        });
    }

    bool ThemingHelper::IsHighContrastColor(_In_ test_infra::HighContrastTheme theme, unsigned int colorValue)
    {
        // Accept Transparent as a high-contrast color.
        if ((colorValue & 0xFF000000) == 0)
        {
            return true;
        }

        bool isHighContrastColor = false;

        wrl::ComPtr<test_infra::ITestServicesStatics> testServicesStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
            &testServicesStatics
            ));

        wrl::ComPtr<test_infra::IThemingHelper> themingHelper;
        LogThrow_IfFailed(testServicesStatics->get_ThemingHelper(&themingHelper));

        if (themingHelper)
        {
            auto hcPalette = static_cast<ThemingHelper*>(themingHelper.Get())->GetHighContrastPalette(theme);
            for (const auto& idColorPair : *hcPalette)
            {
                if (idColorPair.second == colorValue)
                {
                    isHighContrastColor = true;
                    break;
                }
            }
        }

        return isHighContrastColor;
    }

    std::shared_ptr<std::list<std::pair<int, unsigned int>>> ThemingHelper::GetHighContrastPalette(test_infra::HighContrastTheme theme)
    {
        std::shared_ptr<std::list<std::pair<int, unsigned int>>> palette;

        switch (theme)
        {
            case test_infra::HighContrastTheme_Test:
            {
                if (!m_testHighContrastPalette)
                {
                    m_testHighContrastPalette = std::make_shared<std::list<std::pair<int, unsigned int>>>();

                    m_testHighContrastPalette->push_back({ COLOR_WINDOWTEXT, 0xFFFF7F00 });         // Text
                    m_testHighContrastPalette->push_back({ COLOR_HOTLIGHT, 0xFFFFFF00 });           // Hyperlink
                    m_testHighContrastPalette->push_back({ COLOR_GRAYTEXT, 0xFF8080FF });           // Disabled Text
                    m_testHighContrastPalette->push_back({ COLOR_HIGHLIGHTTEXT, 0xFFC0C0C0 });      // Selected Text Foreground
                    m_testHighContrastPalette->push_back({ COLOR_HIGHLIGHT, 0xFFFF0066 });          // Selected Text Background
                    m_testHighContrastPalette->push_back({ COLOR_BTNTEXT, 0xFF00FF00 });            // Button Text Foreground
                    m_testHighContrastPalette->push_back({ COLOR_BTNFACE, 0xFF952AAB });            // Button Text Background
                    m_testHighContrastPalette->push_back({ COLOR_WINDOW, 0xFF00007F });             // Background
                }

                palette = m_testHighContrastPalette;
                break;
            }

            case test_infra::HighContrastTheme_Default:
            {
                if (!m_defaultHighContrastPalette)
                {
                    m_defaultHighContrastPalette = std::make_shared<std::list<std::pair<int, unsigned int>>>();

                    m_defaultHighContrastPalette->push_back({ COLOR_WINDOWTEXT, 0xFFFFFF00 });      // Text
                    m_defaultHighContrastPalette->push_back({ COLOR_HOTLIGHT, 0xFF8080FF });        // Hyperlink
                    m_defaultHighContrastPalette->push_back({ COLOR_GRAYTEXT, 0xFF00FF00 });        // Disabled Text
                    m_defaultHighContrastPalette->push_back({ COLOR_HIGHLIGHTTEXT, 0xFFFFFFFF });   // Selected Text Foreground
                    m_defaultHighContrastPalette->push_back({ COLOR_HIGHLIGHT, 0xFF008000 });       // Selected Text Background
                    m_defaultHighContrastPalette->push_back({ COLOR_BTNTEXT, 0xFFFFFFFF });         // Button Text Foreground
                    m_defaultHighContrastPalette->push_back({ COLOR_BTNFACE, 0xFF000000 });         // Button Text Background
                    m_defaultHighContrastPalette->push_back({ COLOR_WINDOW, 0xFF000000 });          // Background
                }

                palette = m_defaultHighContrastPalette;
                break;
            }

            case test_infra::HighContrastTheme_Black:
            {
                if (!m_blackHighContrastPalette)
                {
                    m_blackHighContrastPalette = std::make_shared<std::list<std::pair<int, unsigned int>>>();

                    m_blackHighContrastPalette->push_back({ COLOR_WINDOWTEXT, 0xFFFFFFFF });        // Text
                    m_blackHighContrastPalette->push_back({ COLOR_HOTLIGHT, 0xFFFFFF00 });          // Hyperlink
                    m_blackHighContrastPalette->push_back({ COLOR_GRAYTEXT, 0xFF3FF23F });          // Disabled Text
                    m_blackHighContrastPalette->push_back({ COLOR_HIGHLIGHTTEXT, 0xFF000000 });     // Selected Text Foreground
                    m_blackHighContrastPalette->push_back({ COLOR_HIGHLIGHT, 0xFF1AEBFF });         // Selected Text Background
                    m_blackHighContrastPalette->push_back({ COLOR_BTNTEXT, 0xFFFFFFFF });           // Button Text Foreground
                    m_blackHighContrastPalette->push_back({ COLOR_BTNFACE, 0xFF000000 });           // Button Text Background
                    m_blackHighContrastPalette->push_back({ COLOR_WINDOW, 0xFF000000 });            // Background
                }

                palette = m_blackHighContrastPalette;
                break;
            }

            case test_infra::HighContrastTheme_White:
            {
                if (!m_whiteHighContrastPalette)
                {
                    m_whiteHighContrastPalette = std::make_shared<std::list<std::pair<int, unsigned int>>>();

                    m_whiteHighContrastPalette->push_back({ COLOR_WINDOWTEXT, 0xFF000000 });        // Text
                    m_whiteHighContrastPalette->push_back({ COLOR_HOTLIGHT, 0xFF00009F });          // Hyperlink
                    m_whiteHighContrastPalette->push_back({ COLOR_GRAYTEXT, 0xFF600000 });          // Disabled Text
                    m_whiteHighContrastPalette->push_back({ COLOR_HIGHLIGHTTEXT, 0xFFFFFFFF });     // Selected Text Foreground
                    m_whiteHighContrastPalette->push_back({ COLOR_HIGHLIGHT, 0xFF37006E });         // Selected Text Background
                    m_whiteHighContrastPalette->push_back({ COLOR_BTNTEXT, 0xFF000000 });           // Button Text Foreground
                    m_whiteHighContrastPalette->push_back({ COLOR_BTNFACE, 0xFFFFFFFF });           // Button Text Background
                    m_whiteHighContrastPalette->push_back({ COLOR_WINDOW, 0xFFFFFFFF });            // Background
                }

                palette = m_whiteHighContrastPalette;
                break;
            }

            case test_infra::HighContrastTheme_Custom:
            {
                if (!m_customHighContrastPalette)
                {
                    m_customHighContrastPalette = std::make_shared<std::list<std::pair<int, unsigned int>>>();

                    m_customHighContrastPalette->push_back({ COLOR_WINDOWTEXT, 0xFF111111 });        // Text
                    m_customHighContrastPalette->push_back({ COLOR_HOTLIGHT, 0xFF00009F });          // Hyperlink
                    m_customHighContrastPalette->push_back({ COLOR_GRAYTEXT, 0xFF600000 });          // Disabled Text
                    m_customHighContrastPalette->push_back({ COLOR_HIGHLIGHTTEXT, 0xFFFFFFFF });     // Selected Text Foreground
                    m_customHighContrastPalette->push_back({ COLOR_HIGHLIGHT, 0xFF37006E });         // Selected Text Background
                    m_customHighContrastPalette->push_back({ COLOR_BTNTEXT, 0xFF000000 });           // Button Text Foreground
                    m_customHighContrastPalette->push_back({ COLOR_BTNFACE, 0xFFFFFFFF });           // Button Text Background
                    m_customHighContrastPalette->push_back({ COLOR_WINDOW, 0xFFEEEEEE });            // Background
                }

                palette = m_customHighContrastPalette;
                break;
            }
        }

        return palette;
    }

} } // namespace Private::Infrastructure
