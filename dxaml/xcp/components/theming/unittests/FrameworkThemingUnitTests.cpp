// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FrameworkThemingUnitTests.h"

#include "XamlLogging.h"

#include "FrameworkTheming.h"
#include "TestThemingInterop.h"
#include "Theme.h"

using namespace Theming;
using namespace WEX::Common;


namespace WEX {
    namespace TestExecution
    {
        // Type traits used by TAEF for logging and comparing values of various types

        template <>
        class VerifyCompareTraits<Theming::Theme, unsigned int>
        {
        public:
            static bool AreEqual(Theming::Theme expected, unsigned int actual)
            {
                return static_cast<unsigned int>(expected) == actual;
            }
        };

        template <>
        class VerifyCompareTraits<unsigned int, Theming::Theme>
        {
        public:
            static bool AreEqual(unsigned int expected, Theming::Theme actual)
            {
                return expected == static_cast<unsigned int>(actual);
            }
        };
    }
}


namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Theming {

    void FrameworkThemingUnitTests::ValidateGetTheme()
    {
        std::shared_ptr<TestThemingInterop> spInterop(new TestThemingInterop());
        FrameworkTheming theming(spInterop, []() -> HRESULT { return S_OK; });

        struct
        {
            Theme SystemTheme;
            Theme RequestedTheme;
            Theme HighContrastTheme;

            Theme ExpectedBaseTheme;
            Theme ExpectedHighContrastTheme;
            Theme ExpectedTheme;
        } themeTestCases[] =
        {
            // SystemTheme      // RequestedTheme       // HighContrastTheme            //ExpectedBaseTheme     //ExpectedHighContrastTheme        //ExpectedTheme
            { Theme::Dark,      Theme::None,            Theme::None,                   Theme::Dark,              Theme::None,                       Theme::Dark },
            { Theme::Light,     Theme::None,            Theme::None,                   Theme::Light,             Theme::None,                       Theme::Light },
            { Theme::Dark,      Theme::Dark,            Theme::None,                   Theme::Dark,              Theme::None,                       Theme::Dark },
            { Theme::Light,     Theme::Dark,            Theme::None,                   Theme::Dark,              Theme::None,                       Theme::Dark },
            { Theme::Dark,      Theme::Light,           Theme::None,                   Theme::Light,             Theme::None,                       Theme::Light },
            { Theme::Light,     Theme::Light,           Theme::None,                   Theme::Light,             Theme::None,                       Theme::Light },
            { Theme::Dark,      Theme::None,            Theme::HighContrast,           Theme::Dark,              Theme::HighContrast,               Theme::Dark | Theme::HighContrast },
            { Theme::Light,     Theme::None,            Theme::HighContrast,           Theme::Light,             Theme::HighContrast,               Theme::Light | Theme::HighContrast },
            { Theme::Dark,      Theme::Dark,            Theme::HighContrast,           Theme::Dark,              Theme::HighContrast,               Theme::Dark | Theme::HighContrast },
            { Theme::Light,     Theme::Dark,            Theme::HighContrast,           Theme::Dark,              Theme::HighContrast,               Theme::Dark | Theme::HighContrast },
            { Theme::Dark,      Theme::Light,           Theme::HighContrast,           Theme::Light,             Theme::HighContrast,               Theme::Light | Theme::HighContrast },
            { Theme::Light,     Theme::Light,           Theme::HighContrast,           Theme::Light,             Theme::HighContrast,               Theme::Light | Theme::HighContrast },
            { Theme::Dark,      Theme::None,            Theme::HighContrastWhite,      Theme::Dark,              Theme::HighContrastWhite,          Theme::Dark | Theme::HighContrastWhite },
            { Theme::Light,     Theme::None,            Theme::HighContrastWhite,      Theme::Light,             Theme::HighContrastWhite,          Theme::Light | Theme::HighContrastWhite },
            { Theme::Dark,      Theme::Dark,            Theme::HighContrastWhite,      Theme::Dark,              Theme::HighContrastWhite,          Theme::Dark | Theme::HighContrastWhite },
            { Theme::Light,     Theme::Dark,            Theme::HighContrastWhite,      Theme::Dark,              Theme::HighContrastWhite,          Theme::Dark | Theme::HighContrastWhite },
            { Theme::Dark,      Theme::Light,           Theme::HighContrastWhite,      Theme::Light,             Theme::HighContrastWhite,          Theme::Light | Theme::HighContrastWhite },
            { Theme::Light,     Theme::Light,           Theme::HighContrastWhite,      Theme::Light,             Theme::HighContrastWhite,          Theme::Light | Theme::HighContrastWhite },
            { Theme::Dark,      Theme::None,            Theme::HighContrastBlack,      Theme::Dark,              Theme::HighContrastBlack,          Theme::Dark | Theme::HighContrastBlack },
            { Theme::Light,     Theme::None,            Theme::HighContrastBlack,      Theme::Light,             Theme::HighContrastBlack,          Theme::Light | Theme::HighContrastBlack },
            { Theme::Dark,      Theme::Dark,            Theme::HighContrastBlack,      Theme::Dark,              Theme::HighContrastBlack,          Theme::Dark | Theme::HighContrastBlack },
            { Theme::Light,     Theme::Dark,            Theme::HighContrastBlack,      Theme::Dark,              Theme::HighContrastBlack,          Theme::Dark | Theme::HighContrastBlack },
            { Theme::Dark,      Theme::Light,           Theme::HighContrastBlack,      Theme::Light,             Theme::HighContrastBlack,          Theme::Light | Theme::HighContrastBlack },
            { Theme::Light,     Theme::Light,           Theme::HighContrastBlack,      Theme::Light,             Theme::HighContrastBlack,          Theme::Light | Theme::HighContrastBlack },
            { Theme::Dark,      Theme::None,            Theme::HighContrastCustom,     Theme::Dark,              Theme::HighContrastCustom,         Theme::Dark | Theme::HighContrastCustom },
            { Theme::Light,     Theme::None,            Theme::HighContrastCustom,     Theme::Light,             Theme::HighContrastCustom,         Theme::Light | Theme::HighContrastCustom },
            { Theme::Dark,      Theme::Dark,            Theme::HighContrastCustom,     Theme::Dark,              Theme::HighContrastCustom,         Theme::Dark | Theme::HighContrastCustom },
            { Theme::Light,     Theme::Dark,            Theme::HighContrastCustom,     Theme::Dark,              Theme::HighContrastCustom,         Theme::Dark | Theme::HighContrastCustom },
            { Theme::Dark,      Theme::Light,           Theme::HighContrastCustom,     Theme::Light,             Theme::HighContrastCustom,         Theme::Light | Theme::HighContrastCustom },
            { Theme::Light,     Theme::Light,           Theme::HighContrastCustom,     Theme::Light,             Theme::HighContrastCustom,         Theme::Light | Theme::HighContrastCustom },
        };

        for (auto& testCase : themeTestCases)
        {
            spInterop->SetSystemTheme(testCase.SystemTheme);
            spInterop->SetSystemHighContrastTheme(testCase.HighContrastTheme);
            LogThrow_IfFailed(theming.SetRequestedTheme(testCase.RequestedTheme, false /*doNotifyThemeChange*/));

            // Force the theming object to pick up the new system & high-contrast theme.
            LogThrow_IfFailed(theming.OnThemeChanged());

            LOG_OUTPUT(
                L"SystemTheme=%d, RequestedTheme=%d, HighContrastTheme=%d, ExpectedBaseTheme=%d, ExpectedHighContrastTheme=%d, ExpectedTheme=%d",
                testCase.SystemTheme,
                testCase.RequestedTheme,
                testCase.HighContrastTheme,
                testCase.ExpectedBaseTheme,
                testCase.ExpectedHighContrastTheme,
                testCase.ExpectedTheme
                );

            VERIFY_ARE_EQUAL(theming.GetBaseTheme(), testCase.ExpectedBaseTheme);
            VERIFY_ARE_EQUAL(theming.GetHighContrastTheme(), testCase.ExpectedHighContrastTheme);
            VERIFY_ARE_EQUAL(theming.GetTheme(), testCase.ExpectedTheme);
        }
    }

    void FrameworkThemingUnitTests::ValidateNotifyThemeChangedHandlerInvoked()
    {
        bool waHandlerInvoked = false;

        std::shared_ptr<TestThemingInterop> spInterop(new TestThemingInterop());
        FrameworkTheming theming(
            spInterop,
            [&waHandlerInvoked]() -> HRESULT
            {
                waHandlerInvoked = true;
                return S_OK;
            });

        LOG_OUTPUT(L"Validate that the handler is invoked when explicitly called.");
        LogThrow_IfFailed(theming.OnThemeChanged());
        VERIFY_IS_TRUE(waHandlerInvoked);

        LOG_OUTPUT(L"Validate that the handler is invoked when changing the requested theme.");
        waHandlerInvoked = false;
        LogThrow_IfFailed(theming.SetRequestedTheme(Theme::Dark));
        VERIFY_IS_TRUE(waHandlerInvoked);

        LOG_OUTPUT(L"Validate that the handler is not invoked when changing requested\n"
                   L"theme but opting out of change notifications.");
        waHandlerInvoked = false;
        LogThrow_IfFailed(theming.SetRequestedTheme(Theme::Light, false /*doNotifyThemeChange*/));
        VERIFY_IS_FALSE(waHandlerInvoked);
    }

    void FrameworkThemingUnitTests::ValidateColorAndBrushResources()
    {
        std::shared_ptr<TestThemingInterop> spInterop(new TestThemingInterop());
        FrameworkTheming theming(spInterop, []() -> HRESULT { return S_OK; });

        const COLORREF expectedAccentColor = 0xFFABABAB;
        spInterop->SetSystemAccentColor(expectedAccentColor);

        LogThrow_IfFailed(theming.OnThemeChanged());

        struct
        {
            const wchar_t* ColorKey;
            const wchar_t* BrushKey;
            unsigned int rgbValue;
            bool OverrideAlpha;
        } expectedResourceInfoList[] = {
            { L"SystemColorActiveCaptionColor",        L"SystemColorActiveCaptionBrush",            spInterop->GetSystemColor(COLOR_ACTIVECAPTION),         true },
            { L"SystemColorBackgroundColor",           L"SystemColorBackgroundBrush",               spInterop->GetSystemColor(COLOR_BACKGROUND),            true },
            { L"SystemColorButtonFaceColor",           L"SystemColorButtonFaceBrush",               spInterop->GetSystemColor(COLOR_BTNFACE),               true },
            { L"SystemColorButtonTextColor",           L"SystemColorButtonTextBrush",               spInterop->GetSystemColor(COLOR_BTNTEXT),               true },
            { L"SystemColorCaptionTextColor",          L"SystemColorCaptionTextBrush",              spInterop->GetSystemColor(COLOR_CAPTIONTEXT),           true },
            { L"SystemColorGrayTextColor",             L"SystemColorGrayTextBrush",                 spInterop->GetSystemColor(COLOR_GRAYTEXT),              true },
            { L"SystemColorHighlightColor",            L"SystemColorHighlightBrush",                spInterop->GetSystemColor(COLOR_HIGHLIGHT),             true },
            { L"SystemColorHighlightTextColor",        L"SystemColorHighlightTextBrush",            spInterop->GetSystemColor(COLOR_HIGHLIGHTTEXT),         true },
            { L"SystemColorHotlightColor",             L"SystemColorHotlightBrush",                 spInterop->GetSystemColor(COLOR_HOTLIGHT),              true },
            { L"SystemColorInactiveCaptionColor",      L"SystemColorInactiveCaptionBrush",          spInterop->GetSystemColor(COLOR_INACTIVECAPTION),       true },
            { L"SystemColorInactiveCaptionTextColor",  L"SystemColorInactiveCaptionTextBrush",      spInterop->GetSystemColor(COLOR_INACTIVECAPTIONTEXT),   true },
            { L"SystemColorWindowColor",               L"SystemColorWindowBrush",                   spInterop->GetSystemColor(COLOR_WINDOW),                true },
            { L"SystemColorWindowTextColor",           L"SystemColorWindowTextBrush",               spInterop->GetSystemColor(COLOR_WINDOWTEXT),            true },
            { L"SystemColorDisabledTextColor",         L"SystemColorDisabledTextBrush",             spInterop->GetSystemColor(COLOR_GRAYTEXT),              true },
            { L"SystemColorControlAccentColor",        L"SystemColorControlAccentBrush",            expectedAccentColor,                                    false },
            { L"SystemAccentColor",                    nullptr,                                     expectedAccentColor,                                    false },
            { L"SystemAccentColorDark1",               nullptr,                                     spInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorDark1),  false },
            { L"SystemAccentColorDark2",               nullptr,                                     spInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorDark2),  false },
            { L"SystemAccentColorDark3",               nullptr,                                     spInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorDark3),  false },
            { L"SystemAccentColorLight1",              nullptr,                                     spInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorLight1), false },
            { L"SystemAccentColorLight2",              nullptr,                                     spInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorLight2), false },
            { L"SystemAccentColorLight3",              nullptr,                                     spInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorLight3), false },
            { L"SystemListAccentLowColor",             nullptr,                                     (0x66FFFFFF & expectedAccentColor),                     false },
            { L"SystemListAccentMediumColor",          nullptr,                                     (0x99FFFFFF & expectedAccentColor),                     false },
            { L"SystemListAccentHighColor",            nullptr,                                     (0xB2FFFFFF & expectedAccentColor),                     false },
        };

        auto& resourceInfoList = theming.GetColorAndBrushResourceInfoList();

        VERIFY_ARE_EQUAL(resourceInfoList.size(), ARRAY_SIZE(expectedResourceInfoList));

        for (size_t i = 0; i < resourceInfoList.size(); ++i)
        {
            bool areColorResourcesEqual = (
                wcscmp(resourceInfoList[i].ColorKey.GetBuffer(), expectedResourceInfoList[i].ColorKey) == 0);

            bool areBrushResourcesEqual = (
                (resourceInfoList[i].BrushKey.IsNullOrEmpty() && expectedResourceInfoList[i].BrushKey == nullptr)
                || wcscmp(resourceInfoList[i].BrushKey.GetBuffer(), expectedResourceInfoList[i].BrushKey) == 0);

            bool areColorValuesEqual = (resourceInfoList[i].rgbValue == expectedResourceInfoList[i].rgbValue);

            bool areOverrideAlphaValuesEqual = (resourceInfoList[i].OverrideAlpha == expectedResourceInfoList[i].OverrideAlpha);

            VERIFY_IS_TRUE(areColorResourcesEqual);
            VERIFY_IS_TRUE(areBrushResourcesEqual);
            VERIFY_IS_TRUE(areColorValuesEqual);
            VERIFY_IS_TRUE(areOverrideAlphaValuesEqual);
        }
    }

    void FrameworkThemingUnitTests::CanDisableHighContrast()
    {
        std::shared_ptr<TestThemingInterop> spInterop(new TestThemingInterop());
        FrameworkTheming theming(spInterop, []() -> HRESULT { return S_OK; });

        spInterop->SetSystemHighContrastTheme(Theme::HighContrast);
        theming.DisableHighContrastUpdateOnThemeChange();
        LogThrow_IfFailed(theming.OnThemeChanged());

        VERIFY_IS_FALSE(theming.HasHighContrastTheme());
    }

    void FrameworkThemingUnitTests::DoesInitialThemeMatchSystemTheme()
    {
        std::shared_ptr<TestThemingInterop> interop(new TestThemingInterop());
        std::unique_ptr<FrameworkTheming> theming;

        LOG_OUTPUT(L"Validate initial theme when system theme is dark.");
        interop->SetSystemTheme(Theme::Dark);
        theming.reset(new FrameworkTheming(interop, []() -> HRESULT { return S_OK; }));
        VERIFY_ARE_EQUAL(theming->GetTheme(), Theme::Dark);

        LOG_OUTPUT(L"Validate initial theme when system theme is light.");
        interop->SetSystemTheme(Theme::Light);
        theming.reset(new FrameworkTheming(interop, []() -> HRESULT { return S_OK; }));
        VERIFY_ARE_EQUAL(theming->GetTheme(), Theme::Light);
    }

} } } } } } // namespace ::Windows::UI::Xaml::Tests::Controls::Theming
