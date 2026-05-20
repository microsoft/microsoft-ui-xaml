// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ThemeResources.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <WUCRenderingScopeGuard.h>

using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Convergence {

        void ThemeResourcesTests::TestThemeResourcesFor_Current()
        {
            CurrentOSMaxVersionTested = L"current";
            RunOnUIThread([&]()
            {
                VerifyDoubleThemeResource(L"AppBarThemeMinHeight", true);           // added in 6.2.1
                VerifyDoubleThemeResource(L"ComboBoxThemeMinWidth", true);          // added in 6.3, not in 6.3.1
                VerifyColorThemeResource(L"ContentDialogDimmingThemeBrush", true);  // added in 6.3.1
                VerifyDoubleThemeResource(L"ScrollBarMinThemeHeight", false);       // only in 6.3.1
                VerifyThicknessThemeResource(L"PivotItemMargin", true);             // only in 6.4\10.0
                VerifyColorThemeResource(L"AppBarButtonForegroundSubMenuOpened", true);    // added in RS5
            });
        }

        void ThemeResourcesTests::TestThemeResourcesFor_PackagedXamlBridge()
        {
            CurrentOSMaxVersionTested = L"Packaged XamlBridge";
            // Same as TestThemeResourcesFor_Current
            RunOnUIThread([&]()
            {
                VerifyDoubleThemeResource(L"AppBarThemeMinHeight", true);           // added in 6.2.1
                VerifyDoubleThemeResource(L"ComboBoxThemeMinWidth", true);          // added in 6.3, not in 6.3.1
                VerifyColorThemeResource(L"ContentDialogDimmingThemeBrush", true);  // added in 6.3.1
                VerifyDoubleThemeResource(L"ScrollBarMinThemeHeight", false);       // only in 6.3.1
                VerifyThicknessThemeResource(L"PivotItemMargin", true);             // only in 6.4\10.0
                VerifyColorThemeResource(L"AppBarButtonForegroundSubMenuOpened", true);    // added in RS5
            });
        }

        void ThemeResourcesTests::TestPivotItemInstantiation()
        {
            TestCleanupWrapper cleanup;

            Microsoft::UI::Xaml::Controls::PivotItem^ pivotItem;

            RunOnUIThread([&]()
            {
                pivotItem = ref new Microsoft::UI::Xaml::Controls::PivotItem;
            });

            VERIFY_IS_TRUE(pivotItem != nullptr);
        }

        void ThemeResourcesTests::VerifyDoubleThemeResource(String^ resourceName, bool shouldSucceed)
        {
            String^ xaml =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"Height='{ThemeResource " + resourceName +  L"}'></Canvas>";

            TryLoadXaml(resourceName, xaml, shouldSucceed);
        }

        void ThemeResourcesTests::VerifyColorThemeResource(String^ resourceName, bool shouldSucceed)
        {
            String^ xaml =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"Background='{ThemeResource " + resourceName +  L"}'></Canvas>";

            TryLoadXaml(resourceName, xaml, shouldSucceed);
        }

        void ThemeResourcesTests::VerifyThicknessThemeResource(String^ resourceName, bool shouldSucceed)
        {
            String^ xaml =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"Margin='{ThemeResource " + resourceName +  L"}'></Canvas>";

            TryLoadXaml(resourceName, xaml, shouldSucceed);
        }

        void ThemeResourcesTests::TryLoadXaml(String^ resourceName, String^ xaml, bool shouldSucceed)
        {
            DisableErrorReportingScopeGuard disableErrors;

            bool succeeded = false;
            String^ exceptionString;

            try
            {
                Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml);
                succeeded = true;
            }
            catch (Platform::COMException ^ex)
            {
                exceptionString = ex->ToString();
                succeeded = false;
            }

            VERIFY_IS_TRUE(
                shouldSucceed == succeeded,
                WEX::Common::String().Format(
                    L"Expected loading ThemeResource %s to %s for OSMaxVersionTested %s, and it %s.",
                    resourceName->Data(),
                    shouldSucceed ? L"succeed" : L"fail",
                    CurrentOSMaxVersionTested->Data(),
                    succeeded ? L"succeeded" : L"failed"
                    ));
        }

        bool operator==(::Windows::UI::Color left, ::Windows::UI::Color right)
        {
            return left.A == right.A && left.R == right.R && left.G == right.G && left.B == right.B;
        }


        void ThemeResourcesTests::HighContrast()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Canvas^ canvas;
            ::Windows::UI::Color defaultColor;

            RunOnUIThread([&]()
            {
                String^ xaml =
                    L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                    L"Background='{ThemeResource SystemControlForegroundChromeMediumBrush}'></Canvas>";

                canvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(xaml));
                TestServices::WindowHelper->WindowContent = canvas;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Verify color of canvas");

            RunOnUIThread([&]()
            {
                auto brush = safe_cast<xaml_media::SolidColorBrush^>(canvas->Background);
                defaultColor = brush->Color;
                LOG_OUTPUT(L"Default color is %x %x %x %x", defaultColor.A, defaultColor.R, defaultColor.G, defaultColor.B);
            });

            // Change to high contrast
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Default;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Validate high contrast color
            RunOnUIThread([&]()
            {
                auto brush = safe_cast<xaml_media::SolidColorBrush^>(canvas->Background);
                auto highContrastColor = brush->Color;
                LOG_OUTPUT(L"High contrast color is %x %x %x %x", highContrastColor.A, highContrastColor.R, highContrastColor.G, highContrastColor.B);

                LOG_OUTPUT(L"Verify SystemControlForegroundChromeMediumBrush changes when we're in high-contrast mode");
                VERIFY_IS_FALSE(highContrastColor == defaultColor);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Back to normal (non-HighContrast) mode
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::None;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Validate return to default color
            RunOnUIThread([&]()
            {
                auto brush = safe_cast<xaml_media::SolidColorBrush^>(canvas->Background);
                auto returnToDefaultColor = brush->Color;
                LOG_OUTPUT(L"returnToDefault color is %x %x %x %x", returnToDefaultColor.A, returnToDefaultColor.R, returnToDefaultColor.G, returnToDefaultColor.B);

                LOG_OUTPUT(L"Verify we changed back to default (non-high contrast) mode");
                VERIFY_IS_TRUE(returnToDefaultColor == defaultColor);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void ThemeResourcesTests::VariantAccentColors()
        {
            RunOnUIThread([&]()
            {
                VerifyColorThemeResource(L"SystemAccentColorDark1", true);
                VerifyColorThemeResource(L"SystemAccentColorDark2", true);
                VerifyColorThemeResource(L"SystemAccentColorDark3", true);
                VerifyColorThemeResource(L"SystemAccentColorLight1", true);
                VerifyColorThemeResource(L"SystemAccentColorLight2", true);
                VerifyColorThemeResource(L"SystemAccentColorLight3", true);
                VerifyColorThemeResource(L"SystemAccentColorLight321", false); // Resource does not exist
            });
        }

        void ThemeResourcesTests::AccentColorHighContrast()
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
            TestServices::ThemingHelper->RestoreDefaultState();

            xaml_controls::StackPanel^ root;
            xaml_controls::Border^ border1;
            xaml_controls::Border^ border2;
            xaml_media::SolidColorBrush^ accentColorBrush;

            RunOnUIThread([&]()
            {
                border1 = safe_cast<xaml_controls::Border^>(xaml_markup::XamlReader::Load("<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                    L"Background='{ThemeResource SystemAccentColor}' Width='100' Height='40'></Border>"));
                border2 = safe_cast<xaml_controls::Border^>(xaml_markup::XamlReader::Load("<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                    L"Background='{ThemeResource SystemControlHighlightListAccentLowBrush}' Width='100' Height='40'></Border>"));
                root = ref new xaml_controls::StackPanel;
                root->Children->Append(border1);
                root->Children->Append(border2);
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->RemoveThemingOverrides();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                accentColorBrush = safe_cast<xaml_media::SolidColorBrush^>(border1->Background);
                LOG_OUTPUT(L"Accent color is %x %x %x %x", accentColorBrush->Color.A, accentColorBrush->Color.R, accentColorBrush->Color.G, accentColorBrush->Color.B);
            });

            LOG_OUTPUT(L"Validate the Accent Color in high-contrast.");
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Test;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto systemHighlightBrush = safe_cast<xaml_media::SolidColorBrush^>(border1->Background);
                auto border2Brush = safe_cast<xaml_media::SolidColorBrush^>(border2->Background);

                LOG_OUTPUT(L"Accent color in High Contrast is %x %x %x %x", systemHighlightBrush->Color.A, systemHighlightBrush->Color.R, systemHighlightBrush->Color.G, systemHighlightBrush->Color.B);
                LOG_OUTPUT(L"System Highlight Color is %x %x %x %x", border2Brush->Color.A, border2Brush->Color.R, border2Brush->Color.G, border2Brush->Color.B);
                VERIFY_IS_FALSE(accentColorBrush->Color == systemHighlightBrush->Color);
                VERIFY_IS_TRUE(systemHighlightBrush->Color == border2Brush->Color);
            });
        }

        void ThemeResourcesTests::RootVisualBackgroundHighContrast()
        {
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::ThemingHelper->RestoreDefaultState();

            RunOnUIThread([&]()
            {
                String^ xaml =
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                    L"Background='Transparent'></Grid>";

                auto grid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xaml));
                TestServices::WindowHelper->WindowContent = grid;
            });
            TestServices::WindowHelper->WaitForIdle();

            VerifyRootVisualHighContrastHelper(HighContrastTheme::Black, L"HCDark");
            VerifyRootVisualHighContrastHelper(HighContrastTheme::White, L"HCLight");
            VerifyRootVisualHighContrastHelper(HighContrastTheme::Test, L"HCTest");
        }

        void ThemeResourcesTests::VerifyRootVisualHighContrastHelper(HighContrastTheme theme, Platform::String^ highContrastResource)
        {
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = theme;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, highContrastResource);
        }

        bool ThemeResourcesTests::ClassSetup()
        {
            // It's very important to call EnsureInitialized on TestServices
            // from ClassSetup. This method will wait for the window to be
            // activated on launch, which avoids a race condition that will block
            // input from being routed to the app. It will also wait for the
            // debugger to attach when the waitForDebugger runtime parameter is
            // specified.
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ThemeResourcesTests::TestSetup()
        {
            //
            // It's very important to have your test clean up the window contents
            // when it completes. When creating new tests be sure to copy this
            // method over or implement it in a similar way. By cleaning
            // up the window content and waiting for the page to go idle you ensure
            // that if your test fails while the UI element tree is being torn down
            // that the failure is associated with your test and doesn't occur
            // nondeterministically in the future. By waiting for the page to go
            // idle you ensure that all transitions have completed and that jupiter
            // is in a 'tabula rasa' state for the next test.
            //
            // Use the TestCleanupWrapper in each test method to handle cleanup, even
            // in cases of failure or repeated runs. Use VerifyTestCleanup here to
            // ensure that the test was cleaned up correctly.
            //
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ThemeResourcesTests::TestCleanup()
        {
            //
            // It's very important to have your test clean up the window contents
            // when it completes. When creating new tests be sure to copy this
            // method over or implement it in a similar way. By cleaning
            // up the window content and waiting for the page to go idle you ensure
            // that if your test fails while the UI element tree is being torn down
            // that the failure is associated with your test and doesn't occur
            // nondeterministically in the future. By waiting for the page to go
            // idle you ensure that all transitions have completed and that jupiter
            // is in a 'tabula rasa' state for the next test.
            //
            // Use the TestCleanupWrapper in each test method to handle cleanup, even
            // in cases of failure or repeated runs. Use VerifyTestCleanup here to
            // ensure that the test was cleaned up correctly.
            //
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

    }
} } } }
