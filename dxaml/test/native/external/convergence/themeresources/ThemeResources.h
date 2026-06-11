// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Convergence { 

        class ThemeResourcesTests : public WEX::TestClass<ThemeResourcesTests>
        {
        public:
            BEGIN_TEST_CLASS(ThemeResourcesTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TestThemeResourcesFor_Current)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies XAML loads the correct resources for apps targeting the current OS version")
                TEST_METHOD_PROPERTY(L"UAP:AppXManifest", APPXMANIFEST_WINDOWS_VERSION_CURRENT)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestThemeResourcesFor_PackagedXamlBridge)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies XAML loads the correct resources for a centennial XamlBridge app")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestPivotItemInstantiation)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies we can build against a WUXP type and instantiate it.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HighContrast)
                TEST_METHOD_PROPERTY(L"Description", L"Test high contrast mode")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VariantAccentColors)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies XAML loads variant accent color resources.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AccentColorHighContrast)
            // Doesn't work on desktop because uxtheme uses the High Contrast mode defined in the system while for testing we create
            // a simulated High Contrast mode that is not known to shell methods.
                TEST_METHOD_PROPERTY(L"Description", L"Verifies XAML changes accent color when on High Contrast Mode.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RootVisualBackgroundHighContrast)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies XAML applies correct RootVisual background when switching between themes in High Contrast.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            void VerifyDoubleThemeResource(Platform::String^ resourceName, bool shouldSucceed);
            void VerifyColorThemeResource(Platform::String^ resourceName, bool shouldSucceed);
            void VerifyThicknessThemeResource(Platform::String^ resourceName, bool shouldSucceed);
            void TryLoadXaml(Platform::String^ resourceName, Platform::String^ xaml, bool shouldSucceed);
            void VerifyRootVisualHighContrastHelper(test_infra::HighContrastTheme theme, Platform::String^ highContrastResource);

            Platform::String^ CurrentOSMaxVersionTested;
        };

    }
} } } }

