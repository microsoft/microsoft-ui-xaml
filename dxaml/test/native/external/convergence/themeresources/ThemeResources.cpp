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

        // FrameworkElement.SetThemeResourceBinding exposes the markup-only {ThemeResource} mechanism to
        // code. These tests use SystemControlForegroundChromeMediumBrush, a system brush known to change
        // under high contrast, so we can exercise live re-resolution the way HighContrast() does for markup.
        void ThemeResourcesTests::SetThemeResourceBindingMarkupParity()
        {
            TestCleanupWrapper cleanup;

            // A code SetThemeResourceBinding and a markup {ThemeResource} to the same key must behave
            // identically across the whole lifecycle: eager resolution to a global value while detached,
            // re-resolution to a closer ancestor override on live enter, and theme tracking once live.
            //
            // Steps 1-4 run a markup/code pair parented directly under the override ancestor. From step 2 on
            // we also run a SECOND markup/code pair hosted inside a Popup that is itself declared under the
            // same ancestor - proving resolution follows the Popup's markup declaration scope (not the
            // popup-root visual it gets reparented under while open), for both markup and code. The popup
            // pair is only introduced once there's a popup to host it; parentless it would be identical to
            // the direct pair and prove nothing.
            xaml_controls::Grid^ markupGrid;
            xaml_controls::Grid^ codeGrid;
            xaml_controls::Grid^ popupMarkupGrid;
            xaml_controls::Grid^ popupCodeGrid;
            Microsoft::UI::Xaml::Controls::Primitives::Popup^ popup;
            ::Windows::UI::Color globalColor;

            String^ markupGridXaml =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"Background='{ThemeResource SystemControlForegroundChromeMediumBrush}'></Grid>";

            // Step 1: bind the direct pair while parentless. With no ancestor override in scope, each
            // resolves to the global brush, and markup and code must agree.
            RunOnUIThread([&]()
            {
                markupGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(markupGridXaml));

                codeGrid = ref new xaml_controls::Grid();
                codeGrid->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"SystemControlForegroundChromeMediumBrush");

                // A second code binding to the same key on a different property must resolve to the very same
                // object instance (a shared resource, not a per-binding clone).
                codeGrid->SetThemeResourceBinding(xaml_controls::Grid::BorderBrushProperty, L"SystemControlForegroundChromeMediumBrush");
                VERIFY_IS_TRUE(codeGrid->BorderBrush == codeGrid->Background);

                // Markup and code resolve the same key through the same global fallback, so they land on the
                // identical shared brush instance.
                VERIFY_IS_TRUE(markupGrid->Background == codeGrid->Background);

                globalColor = safe_cast<xaml_media::SolidColorBrush^>(codeGrid->Background)->Color;
            });

            // Step 2: enter the tree under an ancestor that overrides the key in its ThemeDictionaries. The
            // direct pair is parented straight under the ancestor. We also introduce the Popup pair here:
            // popupMarkupGrid (markup {ThemeResource}) and popupCodeGrid are hosted in a Popup that is itself
            // declared under the ancestor. Once the popup is open, all four bindings must resolve to the
            // closer ancestor override.
            ::Windows::UI::Color defaultOverrideColor = Microsoft::UI::Colors::Magenta;
            ::Windows::UI::Color highContrastOverrideColor = Microsoft::UI::Colors::Lime;
            RunOnUIThread([&]()
            {
                auto ancestor = ref new xaml_controls::Grid();

                auto defaultThemeDictionary = ref new ResourceDictionary();
                defaultThemeDictionary->Insert(L"SystemControlForegroundChromeMediumBrush", ref new xaml_media::SolidColorBrush(defaultOverrideColor));
                ancestor->Resources->ThemeDictionaries->Insert(L"Default", defaultThemeDictionary);

                auto highContrastThemeDictionary = ref new ResourceDictionary();
                highContrastThemeDictionary->Insert(L"SystemControlForegroundChromeMediumBrush", ref new xaml_media::SolidColorBrush(highContrastOverrideColor));
                ancestor->Resources->ThemeDictionaries->Insert(L"HighContrast", highContrastThemeDictionary);

                ancestor->Children->Append(markupGrid);
                ancestor->Children->Append(codeGrid);

                // Host the Popup pair inside a Popup declared under the same ancestor. The markup child binds
                // its {ThemeResource} at parse; the code child is bound later, once live inside the open
                // popup. When the popup opens, its content is reparented under the popup root, but its
                // resource scope must still follow the popup back to this ancestor.
                popupMarkupGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(markupGridXaml));
                popupCodeGrid = ref new xaml_controls::Grid();

                popup = ref new Microsoft::UI::Xaml::Controls::Primitives::Popup();
                auto popupContent = ref new xaml_controls::Grid();
                popupContent->Children->Append(popupMarkupGrid);
                popupContent->Children->Append(popupCodeGrid);
                popup->Child = popupContent;
                ancestor->Children->Append(popup);

                TestServices::WindowHelper->WindowContent = ancestor;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Open the popup so its content goes live and is reparented under the popup root.
                popup->IsOpen = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Entering the tree re-resolved the direct pair to the same shared ancestor override
                // instance...
                VERIFY_IS_TRUE(markupGrid->Background == codeGrid->Background);

                auto liveColor = safe_cast<xaml_media::SolidColorBrush^>(markupGrid->Background)->Color;

                // ...which is the ancestor's override value, and differs from the global value, proving the
                // update-on-enter actually happened.
                VERIFY_IS_TRUE(liveColor == defaultOverrideColor);
                VERIFY_IS_FALSE(liveColor == globalColor);

                // The popup markup child, now live inside the open popup, re-resolved its {ThemeResource} to
                // the SAME ancestor override - proving markup follows the popup back to its declaration scope
                // rather than stopping at the popup root.
                VERIFY_IS_TRUE(popupMarkupGrid->Background == codeGrid->Background);

                // Now install the code binding while popupCodeGrid is ALREADY live inside the open popup. This
                // runs ResolveThemeResourceForElement's ancestor walk with the element reparented under the
                // popup root: it must follow the popup to the ancestor and resolve to the override - not stop
                // at the popup root and fall back to the global value. This is the code counterpart to the
                // markup child above.
                popupCodeGrid->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"SystemControlForegroundChromeMediumBrush");
                VERIFY_IS_TRUE(popupCodeGrid->Background == codeGrid->Background);

                auto popupInstallColor = safe_cast<xaml_media::SolidColorBrush^>(popupCodeGrid->Background)->Color;
                VERIFY_IS_TRUE(popupInstallColor == defaultOverrideColor);
                VERIFY_IS_FALSE(popupInstallColor == globalColor);
            });

            // Step 3: with all four live, a theme change must still re-resolve every binding identically, now
            // picking up the ancestor's high contrast override.
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Default;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // All re-resolve to the same shared high contrast override instance...
                VERIFY_IS_TRUE(markupGrid->Background == codeGrid->Background);
                VERIFY_IS_TRUE(popupMarkupGrid->Background == codeGrid->Background);
                VERIFY_IS_TRUE(popupCodeGrid->Background == codeGrid->Background);

                auto highContrastColor = safe_cast<xaml_media::SolidColorBrush^>(markupGrid->Background)->Color;

                // ...which is the ancestor's high contrast value and differs from the default-theme override.
                VERIFY_IS_TRUE(highContrastColor == highContrastOverrideColor);
                VERIFY_IS_FALSE(highContrastColor == defaultOverrideColor);
            });

            // Step 4: switching back to the default theme must re-resolve every binding identically once more,
            // returning them to the default-theme override value (proving the tracking is bidirectional).
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::None;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // All return to the same shared default-override instance (tracking is bidirectional).
                VERIFY_IS_TRUE(markupGrid->Background == codeGrid->Background);
                VERIFY_IS_TRUE(popupMarkupGrid->Background == codeGrid->Background);
                VERIFY_IS_TRUE(popupCodeGrid->Background == codeGrid->Background);

                auto restoredColor = safe_cast<xaml_media::SolidColorBrush^>(markupGrid->Background)->Color;
                VERIFY_IS_TRUE(restoredColor == defaultOverrideColor);

                popup->IsOpen = false;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ThemeResourcesTests::SetThemeResourceBindingMissingKeyThrows()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto grid = ref new xaml_controls::Grid();

                DisableErrorReportingScopeGuard disableErrors;

                bool threw = false;
                try
                {
                    grid->SetThemeResourceBinding(
                        xaml_controls::Panel::BackgroundProperty,
                        L"ThemeResourceKeyThatDoesNotExist_SetThemeResourceBindingTest");
                }
                catch (Platform::Exception^)
                {
                    threw = true;
                }

                VERIFY_IS_TRUE(threw, L"Expected SetThemeResourceBinding to throw for an unresolvable key.");

                // A failed call must not touch the target property.
                VERIFY_IS_NULL(grid->Background);
            });
        }

        void ThemeResourcesTests::SetThemeResourceBindingLocalOverrideAndClear()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Grid^ grid;
            xaml_media::SolidColorBrush^ localBrush;

            RunOnUIThread([&]()
            {
                grid = ref new xaml_controls::Grid();
                grid->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"SystemControlForegroundChromeMediumBrush");
                grid->SetThemeResourceBinding(xaml_controls::Grid::BorderBrushProperty, L"SystemControlForegroundChromeMediumBrush");
                TestServices::WindowHelper->WindowContent = grid;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // The binding is active.
                VERIFY_IS_NOT_NULL(safe_cast<xaml_media::SolidColorBrush^>(grid->Background));
                VERIFY_IS_NOT_NULL(safe_cast<xaml_media::SolidColorBrush^>(grid->BorderBrush));

                // Setting a local value replaces the theme binding (installed at Local precedence).
                localBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                grid->Background = localBrush;
                VERIFY_IS_TRUE(grid->Background == localBrush);
            });

            // A theme switch must not disturb the local value.
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Default;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(grid->Background == localBrush);

                // Clearing the property restores its default (no value).
                grid->ClearValue(xaml_controls::Grid::BorderBrushProperty);
                VERIFY_IS_NULL(grid->BorderBrush);
            });

            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::None;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ThemeResourcesTests::SetThemeResourceBindingReadOnlyPropertyThrows()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto grid = ref new xaml_controls::Grid();

                DisableErrorReportingScopeGuard disableErrors;

                bool threw = false;
                try
                {
                    // ActualWidth is a read-only dependency property.
                    grid->SetThemeResourceBinding(FrameworkElement::ActualWidthProperty, L"SystemControlForegroundChromeMediumBrush");
                }
                catch (Platform::Exception^)
                {
                    threw = true;
                }

                VERIFY_IS_TRUE(threw, L"Expected SetThemeResourceBinding to reject a read-only dependency property.");
            });
        }

        void ThemeResourcesTests::SetThemeResourceBindingResolutionSources()
        {
            TestCleanupWrapper cleanup;

            // Consolidated coverage of every way SetThemeResourceBinding resolves a key. Distinct brush colors
            // make the source of each resolved value unambiguous:
            //   - global      : a system theme brush (global/application fallback)
            //   - application : an entry inserted into Application.Current.Resources (app fallback branch)
            //   - self        : a key in the element's OWN Resources.ThemeDictionaries (walk starts at element)
            //   - ancestor    : a plain entry in an ancestor's Resources (ambient walk + fallback-to-ambient
            //                    theme-reference dictionary)
            // First everything resolves while detached; then the global/application elements are entered under
            // an ancestor that overrides those keys, proving enter-time re-resolution to a closer tree value;
            // finally the same four sources are re-verified with the binding installed while already live.
            //
            // Markup parity for the *live-install* variants is intentionally not exercised here: markup
            // {ThemeResource} always resolves at parse time, so there is no natural "installed while live"
            // markup path to compare against. SetThemeResourceBindingMarkupParity covers markup/code parity.

            // RAII: keep an application-scoped resource alive for the whole test, and always remove it on the
            // way out (even if a VERIFY throws) so it cannot leak into other tests. The brush is created on the
            // UI thread to preserve thread affinity.
            struct ScopedAppResource
            {
                Platform::String^ Key;
                ScopedAppResource(Platform::String^ key, ::Windows::UI::Color color) : Key(key)
                {
                    RunOnUIThread([key, color]()
                    {
                        Application::Current->Resources->Insert(key, ref new xaml_media::SolidColorBrush(color));
                    });
                }
                ~ScopedAppResource()
                {
                    Platform::String^ key = Key;
                    RunOnUIThread([key]() { Application::Current->Resources->Remove(key); });
                }
            };
            ScopedAppResource appResource(L"ThemeResourceConsolidatedAppKey", Microsoft::UI::Colors::Orange);

            xaml_controls::Grid^ globalElem;
            xaml_controls::Grid^ appElem;
            ::Windows::UI::Color globalDetachedColor;

            // Phase 1: all four sources resolve while detached (no window content).
            RunOnUIThread([&]()
            {
                // global -> a system theme brush found via the global/application fallback.
                globalElem = ref new xaml_controls::Grid();
                globalElem->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"SystemControlForegroundChromeMediumBrush");
                auto globalBrush = safe_cast<xaml_media::SolidColorBrush^>(globalElem->Background);
                VERIFY_IS_NOT_NULL(globalBrush);
                globalDetachedColor = globalBrush->Color;

                // application -> the entry we inserted into Application.Current.Resources.
                appElem = ref new xaml_controls::Grid();
                appElem->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"ThemeResourceConsolidatedAppKey");
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(appElem->Background)->Color == Microsoft::UI::Colors::Orange);

                // self -> a key in the element's own Resources.ThemeDictionaries (walk's first iteration).
                auto selfElem = ref new xaml_controls::Grid();
                auto selfThemeDictionary = ref new ResourceDictionary();
                selfThemeDictionary->Insert(L"ThemeResourceConsolidatedSelfKey", ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green));
                selfElem->Resources->ThemeDictionaries->Insert(L"Default", selfThemeDictionary);
                selfElem->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"ThemeResourceConsolidatedSelfKey");
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(selfElem->Background)->Color == Microsoft::UI::Colors::Green);

                // ancestor -> a plain entry in a detached ancestor's Resources.
                auto ancestor = ref new xaml_controls::Grid();
                ancestor->Resources->Insert(L"ThemeResourceConsolidatedAncestorKey", ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue));
                auto ancestorChild = ref new xaml_controls::Grid();
                ancestor->Children->Append(ancestorChild);
                ancestorChild->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"ThemeResourceConsolidatedAncestorKey");
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(ancestorChild->Background)->Color == Microsoft::UI::Colors::Blue);
            });

            // Phase 2: enter the global and application elements under an ancestor that overrides both keys.
            // Going live must re-resolve each binding from its detached value to the closer tree override.
            RunOnUIThread([&]()
            {
                auto overrideAncestor = ref new xaml_controls::Grid();
                overrideAncestor->Resources->Insert(L"SystemControlForegroundChromeMediumBrush", ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red));
                overrideAncestor->Resources->Insert(L"ThemeResourceConsolidatedAppKey", ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red));

                overrideAncestor->Children->Append(globalElem);
                overrideAncestor->Children->Append(appElem);
                TestServices::WindowHelper->WindowContent = overrideAncestor;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto globalColor = safe_cast<xaml_media::SolidColorBrush^>(globalElem->Background)->Color;
                auto appColor = safe_cast<xaml_media::SolidColorBrush^>(appElem->Background)->Color;

                // Both re-resolved to the closer ancestor override...
                VERIFY_IS_TRUE(globalColor == Microsoft::UI::Colors::Red);
                VERIFY_IS_TRUE(appColor == Microsoft::UI::Colors::Red);

                // ...which differs from what they resolved to while detached, proving the update happened.
                VERIFY_IS_FALSE(globalColor == globalDetachedColor);
                VERIFY_IS_FALSE(appColor == Microsoft::UI::Colors::Orange);
            });

            // Phase 3: the same four sources, but with the binding installed while the element is already live
            // (exercises the IsActive() install path in CDependencyObject::UpdateThemeReference).
            xaml_controls::Grid^ liveGlobal;
            xaml_controls::Grid^ liveApp;
            xaml_controls::Grid^ liveSelf;
            xaml_controls::Grid^ liveAncestorChild;

            RunOnUIThread([&]()
            {
                auto liveRoot = ref new xaml_controls::StackPanel();

                liveGlobal = ref new xaml_controls::Grid();
                liveApp = ref new xaml_controls::Grid();

                liveSelf = ref new xaml_controls::Grid();
                auto selfThemeDictionary = ref new ResourceDictionary();
                selfThemeDictionary->Insert(L"ThemeResourceConsolidatedSelfKey", ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green));
                liveSelf->Resources->ThemeDictionaries->Insert(L"Default", selfThemeDictionary);

                auto liveAncestor = ref new xaml_controls::Grid();
                liveAncestor->Resources->Insert(L"ThemeResourceConsolidatedAncestorKey", ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue));
                liveAncestorChild = ref new xaml_controls::Grid();
                liveAncestor->Children->Append(liveAncestorChild);

                liveRoot->Children->Append(liveGlobal);
                liveRoot->Children->Append(liveApp);
                liveRoot->Children->Append(liveSelf);
                liveRoot->Children->Append(liveAncestor);

                // Everything is live before any binding is installed.
                TestServices::WindowHelper->WindowContent = liveRoot;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                liveGlobal->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"SystemControlForegroundChromeMediumBrush");
                liveApp->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"ThemeResourceConsolidatedAppKey");
                liveSelf->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"ThemeResourceConsolidatedSelfKey");
                liveAncestorChild->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"ThemeResourceConsolidatedAncestorKey");

                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(liveGlobal->Background)->Color == globalDetachedColor);
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(liveApp->Background)->Color == Microsoft::UI::Colors::Orange);
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(liveSelf->Background)->Color == Microsoft::UI::Colors::Green);
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(liveAncestorChild->Background)->Color == Microsoft::UI::Colors::Blue);
            });
        }

        void ThemeResourcesTests::SetThemeResourceBindingTemplateScopeDivergence()
        {
            TestCleanupWrapper cleanup;

            // Markup {ThemeResource} and code SetThemeResourceBinding use different resolution scopes:
            //   - markup resolves against the *lexical* (parse-time) ambient dictionary stack, which for
            //     control template content is the dictionary the template was authored in - reproduced at
            //     runtime through the template's saved parser context (ResourceResolver's
            //     TryResolveResourceFromCachedParserContext).
            //   - code resolves against the *runtime* parent chain (ResolveThemeResourceForElement walking
            //     GetParentInternal), which never consults that saved context.
            // These diverge whenever content is authored in one dictionary but instantiated elsewhere in the
            // tree - the canonical case being a control template. Here a brush lives only as a lexical sibling
            // of a ControlTemplate inside a standalone ResourceDictionary; the template is then applied to a
            // ContentControl whose runtime ancestors do NOT include that dictionary.

            // Keep the authoring dictionary alive for the whole test so the template's saved lexical scope
            // (which it references) survives until we finish verifying.
            ResourceDictionary^ lexicalDictionary;
            xaml_controls::ContentControl^ templatedControl;

            RunOnUIThread([&]()
            {
                String^ dictionaryXaml =
                    L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <SolidColorBrush x:Key='TemplateLexicalBrush' Color='Blue'/>"
                    L"  <ControlTemplate x:Key='DivergenceTemplate' TargetType='ContentControl'>"
                    L"    <Border x:Name='PART_TemplateRoot' Background='{ThemeResource TemplateLexicalBrush}'/>"
                    L"  </ControlTemplate>"
                    L"</ResourceDictionary>";
                lexicalDictionary = safe_cast<ResourceDictionary^>(xaml_markup::XamlReader::Load(dictionaryXaml));
                auto controlTemplate = safe_cast<xaml_controls::ControlTemplate^>(lexicalDictionary->Lookup(L"DivergenceTemplate"));

                // Apply the template to a control that lives directly under the window. The authoring
                // dictionary above is NOT part of this control's runtime ancestor chain.
                templatedControl = ref new xaml_controls::ContentControl();

                // A brush that IS reachable by the runtime tree walk: it sits in the templated control's own
                // Resources (a live ancestor of the template's Border). Used below to prove code binding does
                // work for tree-reachable keys - the divergence is specifically about lexical-only keys.
                templatedControl->Resources->Insert(L"TreeReachableBrush", ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green));

                templatedControl->Template = controlTemplate;
                TestServices::WindowHelper->WindowContent = templatedControl;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // The template root is the ContentControl's single visual child.
                auto templatePart = safe_cast<xaml_controls::Border^>(xaml_media::VisualTreeHelper::GetChild(templatedControl, 0));

                // Markup resolved {ThemeResource TemplateLexicalBrush} via the template's lexical scope, even
                // though that brush is not present anywhere in the live tree.
                auto markupBrush = safe_cast<xaml_media::SolidColorBrush^>(templatePart->Background);
                VERIFY_IS_NOT_NULL(markupBrush);
                VERIFY_IS_TRUE(markupBrush->Color == Microsoft::UI::Colors::Blue);

                // The SAME key from code resolves via the runtime parent chain, which does not include the
                // template's authoring dictionary. The lexical-only key is therefore invisible and the call
                // must throw - exactly as an unresolvable key does.
                {
                    DisableErrorReportingScopeGuard disableErrors;
                    bool threw = false;
                    try
                    {
                        templatePart->SetThemeResourceBinding(xaml_controls::Border::BorderBrushProperty, L"TemplateLexicalBrush");
                    }
                    catch (Platform::Exception^)
                    {
                        threw = true;
                    }

                    VERIFY_IS_TRUE(threw, L"Expected SetThemeResourceBinding to throw for a key visible only in the template's lexical scope.");

                    // The failed call must not touch the target property.
                    VERIFY_IS_NULL(templatePart->BorderBrush);
                }

                // Sanity check the other direction: a key that IS on the runtime parent chain (the templated
                // control's own Resources) binds successfully from the same template part. This proves the
                // throw above is about lexical-only invisibility, not that code binding fails inside templates.
                templatePart->SetThemeResourceBinding(xaml_controls::Border::BorderBrushProperty, L"TreeReachableBrush");
                auto codeBrush = safe_cast<xaml_media::SolidColorBrush^>(templatePart->BorderBrush);
                VERIFY_IS_NOT_NULL(codeBrush);
                VERIFY_IS_TRUE(codeBrush->Color == Microsoft::UI::Colors::Green);
            });
        }

        // Reparenting bound elements into a different subtree: when an element with a theme binding is live and
        // re-enters the tree at a new location, the binding is fully re-resolved by walking UP the new tree
        // (CDependencyObject::UpdateThemeReference -> ResourceResolver::FindNextResolvedValueNoRef). So it does
        // NOT stay pinned to the dictionary captured at install time - it re-scopes to whatever resource the new
        // ancestor chain provides for the key, under the new subtree's theme. (The captured-dictionary weak ref
        // in CThemeResource::RefreshValue is only the fallback used while the element is NOT live.) Markup
        // {ThemeResource} and code SetThemeResourceBinding share this one engine, so both must behave identically
        // and keep resolving to the very same shared object.
        //
        // Four children exercise both re-scoping outcomes together:
        //   - treeMarkupChild / treeCodeChild bind MovingKey, redefined in subtree B -> pick up B's value.
        //   - appMarkupChild / appCodeChild bind AppMovingKey, resolved from Application.Current.Resources.
        // All four start under subtree A (behind an intermediateGrid), are reparented together into subtree B
        // (RequestedTheme=Dark), and App.Resources is edited mid-reparent to prove the app-scoped pair picks up
        // the fresh value. Before the move, a MovingKey value is inserted into intermediateGrid's in-scope
        // dictionary to confirm the contrast: a plain mutation (no theme change, no reparent) does NOT re-resolve.
        // Finally, the tree-scoped pair is moved once more into subtree C, which does NOT define MovingKey: the
        // re-walk finds nothing, so the binding falls back to the dictionary captured at install time (subtree A,
        // still alive) rather than keeping subtree B's value or resolving to null. Two further steps probe the
        // captured dictionary's lifetime: EMPTYING it (dictionary object alive, key removed) makes the fallback
        // lookup fail and the reparent throw; RELEASING it (drop the last strong ref so it is destroyed) makes the
        // fallback no-op and the binding keep its last resolved value. Both behave identically for markup and code.
        void ThemeResourcesTests::SetThemeResourceBindingReparentDifferentThemeAndKey()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Grid^ subtreeA;          // authoring/install scope for the tree-scoped pair
            xaml_controls::Grid^ subtreeB;          // destination scope; RequestedTheme=Dark, redefines MovingKey
            xaml_controls::Grid^ subtreeC;          // third scope; RequestedTheme=Light, does NOT define MovingKey
            xaml_controls::Grid^ subtreeD;          // keyless scope reused for the captured-dictionary lifetime steps
            xaml_controls::Grid^ intermediateGrid;  // sits between subtreeA and the children; gets a mutated dict
            xaml_controls::Grid^ emptySource;       // separate source whose captured dict we EMPTY (throw case)
            xaml_controls::Grid^ emptyMarkupChild;  // markup {ThemeResource MovingKey} captured in emptySource
            xaml_controls::Grid^ emptyCodeChild;    // code SetThemeResourceBinding to MovingKey captured in emptySource
            xaml_controls::Grid^ treeMarkupChild;   // markup {ThemeResource MovingKey}
            xaml_controls::Grid^ treeCodeChild;     // code SetThemeResourceBinding to MovingKey
            xaml_controls::Grid^ appMarkupChild;    // markup {ThemeResource AppMovingKey}
            xaml_controls::Grid^ appCodeChild;      // code SetThemeResourceBinding to AppMovingKey

            // Distinct colors make the source of every resolved value unambiguous.
            //   subtree A       : MovingKey    -> Red (Default/Light), Green (Dark).
            //   subtree B       : MovingKey    -> Orange (Default), Magenta (Dark)   [picked up after reparent].
            //   intermediate    : MovingKey    -> Lime, inserted post-resolution     [must NOT be picked up].
            //   App.Resources   : AppMovingKey -> Blue (Default/Light), Cyan (Dark)  [Dark later edited to Yellow].
            const ::Windows::UI::Color aLight = Microsoft::UI::Colors::Red;
            const ::Windows::UI::Color aDark = Microsoft::UI::Colors::Green;
            const ::Windows::UI::Color bLight = Microsoft::UI::Colors::Orange;
            const ::Windows::UI::Color bDark = Microsoft::UI::Colors::Magenta;
            const ::Windows::UI::Color intermediateInsert = Microsoft::UI::Colors::Lime;
            const ::Windows::UI::Color appLight = Microsoft::UI::Colors::Blue;
            const ::Windows::UI::Color appDarkOriginal = Microsoft::UI::Colors::Cyan;
            const ::Windows::UI::Color appDarkUpdated = Microsoft::UI::Colors::Yellow;

            // RAII: publish AppMovingKey into Application.Current.Resources.ThemeDictionaries for the whole
            // test, and always pull it back out on the way (even if a VERIFY throws) so it cannot leak into
            // other tests. Existing Default/Dark theme dictionaries are reused (our key added/removed); if the
            // app has none we create them and remove them on teardown. DarkDict is exposed so the test body can
            // edit the Dark value in place. All dictionary/brush work happens on the UI thread for affinity.
            struct ScopedAppThemeResource
            {
                Platform::String^ Key;
                ResourceDictionary^ DefaultDict;
                ResourceDictionary^ DarkDict;
                bool CreatedDefault = false;
                bool CreatedDark = false;

                ScopedAppThemeResource(Platform::String^ key, ::Windows::UI::Color light, ::Windows::UI::Color dark) : Key(key)
                {
                    RunOnUIThread([&]()
                    {
                        auto themeDictionaries = Application::Current->Resources->ThemeDictionaries;

                        if (themeDictionaries->HasKey(L"Default"))
                        {
                            DefaultDict = safe_cast<ResourceDictionary^>(themeDictionaries->Lookup(L"Default"));
                        }
                        else
                        {
                            DefaultDict = ref new ResourceDictionary();
                            themeDictionaries->Insert(L"Default", DefaultDict);
                            CreatedDefault = true;
                        }

                        if (themeDictionaries->HasKey(L"Dark"))
                        {
                            DarkDict = safe_cast<ResourceDictionary^>(themeDictionaries->Lookup(L"Dark"));
                        }
                        else
                        {
                            DarkDict = ref new ResourceDictionary();
                            themeDictionaries->Insert(L"Dark", DarkDict);
                            CreatedDark = true;
                        }

                        DefaultDict->Insert(Key, ref new xaml_media::SolidColorBrush(light));
                        DarkDict->Insert(Key, ref new xaml_media::SolidColorBrush(dark));
                    });
                }

                ~ScopedAppThemeResource()
                {
                    RunOnUIThread([&]()
                    {
                        auto themeDictionaries = Application::Current->Resources->ThemeDictionaries;
                        if (CreatedDefault) { themeDictionaries->Remove(L"Default"); } else { DefaultDict->Remove(Key); }
                        if (CreatedDark) { themeDictionaries->Remove(L"Dark"); } else { DarkDict->Remove(Key); }
                    });
                }
            };
            ScopedAppThemeResource appResource(L"AppMovingKey", appLight, appDarkOriginal);

            // Step 1: build both subtrees from markup, with the app-scoped children set up so AppMovingKey (only
            // present in App.Resources) is resolved by walking past subtree A to the application dictionary.
            RunOnUIThread([&]()
            {
                String^ xaml =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid x:Name='subtreeA' RequestedTheme='Light'>"
                    L"    <Grid.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Default'>"
                    L"            <SolidColorBrush x:Key='MovingKey' Color='Red'/>"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='MovingKey' Color='Green'/>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Grid.Resources>"
                    L"    <Grid x:Name='intermediateGrid'>"
                    L"      <Grid.Resources>"
                    L"        <ResourceDictionary>"
                    L"          <ResourceDictionary.ThemeDictionaries>"
                    L"            <ResourceDictionary x:Key='Default'/>"
                    L"            <ResourceDictionary x:Key='Dark'/>"
                    L"          </ResourceDictionary.ThemeDictionaries>"
                    L"        </ResourceDictionary>"
                    L"      </Grid.Resources>"
                    L"      <Grid x:Name='treeMarkupChild' Background='{ThemeResource MovingKey}'/>"
                    L"      <Grid x:Name='treeCodeChild'/>"
                    L"      <Grid x:Name='appMarkupChild' Background='{ThemeResource AppMovingKey}'/>"
                    L"      <Grid x:Name='appCodeChild'/>"
                    L"    </Grid>"
                    L"  </Grid>"
                    L"  <Grid x:Name='subtreeB' RequestedTheme='Dark'>"
                    L"    <Grid.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Default'>"
                    L"            <SolidColorBrush x:Key='MovingKey' Color='Orange'/>"
                    L"          </ResourceDictionary>"
                    L"          <ResourceDictionary x:Key='Dark'>"
                    L"            <SolidColorBrush x:Key='MovingKey' Color='Magenta'/>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Grid.Resources>"
                    L"  </Grid>"
                    L"  <Grid x:Name='subtreeC' RequestedTheme='Light'/>"
                    L"  <Grid x:Name='subtreeD' RequestedTheme='Light'/>"
                    L"  <Grid x:Name='emptySource' RequestedTheme='Light'>"
                    L"    <Grid.Resources>"
                    L"      <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"          <ResourceDictionary x:Key='Default'>"
                    L"            <SolidColorBrush x:Key='MovingKey' Color='Red'/>"
                    L"          </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"      </ResourceDictionary>"
                    L"    </Grid.Resources>"
                    L"    <Grid x:Name='emptyMarkupChild' Background='{ThemeResource MovingKey}'/>"
                    L"    <Grid x:Name='emptyCodeChild'/>"
                    L"  </Grid>"
                    L"</StackPanel>";

                auto root = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(xaml));
                subtreeA = safe_cast<xaml_controls::Grid^>(root->FindName(L"subtreeA"));
                subtreeB = safe_cast<xaml_controls::Grid^>(root->FindName(L"subtreeB"));
                subtreeC = safe_cast<xaml_controls::Grid^>(root->FindName(L"subtreeC"));
                subtreeD = safe_cast<xaml_controls::Grid^>(root->FindName(L"subtreeD"));
                emptySource = safe_cast<xaml_controls::Grid^>(root->FindName(L"emptySource"));
                emptyMarkupChild = safe_cast<xaml_controls::Grid^>(root->FindName(L"emptyMarkupChild"));
                emptyCodeChild = safe_cast<xaml_controls::Grid^>(root->FindName(L"emptyCodeChild"));
                intermediateGrid = safe_cast<xaml_controls::Grid^>(root->FindName(L"intermediateGrid"));
                treeMarkupChild = safe_cast<xaml_controls::Grid^>(root->FindName(L"treeMarkupChild"));
                treeCodeChild = safe_cast<xaml_controls::Grid^>(root->FindName(L"treeCodeChild"));
                appMarkupChild = safe_cast<xaml_controls::Grid^>(root->FindName(L"appMarkupChild"));
                appCodeChild = safe_cast<xaml_controls::Grid^>(root->FindName(L"appCodeChild"));

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Install the two code bindings while their elements are live under subtree A (RequestedTheme=Light,
            // so the move into the Dark subtree B is a deterministic Light->Dark change regardless of the app's
            // base theme). The tree-scoped pair resolves MovingKey from A (Red); the app-scoped pair resolves
            // AppMovingKey from App.Resources (Blue). Each pair shares one object instance.
            RunOnUIThread([&]()
            {
                treeCodeChild->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"MovingKey");
                appCodeChild->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"AppMovingKey");

                VERIFY_IS_TRUE(treeMarkupChild->Background == treeCodeChild->Background);
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(treeCodeChild->Background)->Color == aLight);

                VERIFY_IS_TRUE(appMarkupChild->Background == appCodeChild->Background);
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(appCodeChild->Background)->Color == appLight);
            });

            // Step 2: mutate a dictionary that is already in scope, without any theme change or reparent.
            // intermediateGrid sits between subtree A and the children and carries an (initially empty) Light
            // theme dictionary. Inserting MovingKey=Lime into it makes intermediateGrid the CLOSEST ancestor
            // defining MovingKey, so a re-resolve would flip the tree-scoped pair from Red to Lime. A plain
            // dictionary mutation does not trigger a re-resolve, so both markup and code must stay pinned to
            // their already-resolved Red value.
            RunOnUIThread([&]()
            {
                auto intermediateDefault = safe_cast<ResourceDictionary^>(
                    intermediateGrid->Resources->ThemeDictionaries->Lookup(L"Default"));
                intermediateDefault->Insert(L"MovingKey", ref new xaml_media::SolidColorBrush(intermediateInsert));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(treeMarkupChild->Background == treeCodeChild->Background);
                auto colorAfterInsert = safe_cast<xaml_media::SolidColorBrush^>(treeCodeChild->Background)->Color;
                VERIFY_IS_TRUE(colorAfterInsert == aLight);
                VERIFY_IS_FALSE(colorAfterInsert == intermediateInsert);
            });

            // Step 3: reparent all four children from subtree A into subtree B, exactly as an app moving
            // elements between two live panels would. Critically, App.Resources' Dark value is edited in the
            // window between removing the children from the tree and re-inserting them under B: that insert is
            // what fires the theme change (B applies RequestedTheme=Dark), so the app-scoped bindings refresh
            // against the *already-updated* App.Resources.
            RunOnUIThread([&]()
            {
                intermediateGrid->Children->Clear();

                // Edit App.Resources while the children are detached, before they re-enter under B.
                appResource.DarkDict->Remove(L"AppMovingKey");
                appResource.DarkDict->Insert(L"AppMovingKey", ref new xaml_media::SolidColorBrush(appDarkUpdated));

                subtreeB->Children->Append(treeMarkupChild);
                subtreeB->Children->Append(treeCodeChild);
                subtreeB->Children->Append(appMarkupChild);
                subtreeB->Children->Append(appCodeChild);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // The tree-scoped pair still shares one object after the move...
                VERIFY_IS_TRUE(treeMarkupChild->Background == treeCodeChild->Background);

                auto movedColor = safe_cast<xaml_media::SolidColorBrush^>(treeCodeChild->Background)->Color;

                // ...and the binding was fully re-resolved by walking up the NEW subtree when the element became
                // live under B. B redefines MovingKey and applies RequestedTheme=Dark, so the binding re-scopes
                // to subtree B's Dark value (Magenta) - not subtree A's original values, and not B's Light value.
                VERIFY_IS_TRUE(movedColor == bDark);
                VERIFY_IS_FALSE(movedColor == bLight);

                // It re-scoped away from subtree A entirely: neither A's Light (Red) nor A's Dark (Green) survive
                // the move, proving the binding is not pinned to the dictionary captured at install time. The
                // value inserted into the (now-abandoned) intermediateGrid is likewise never seen.
                VERIFY_IS_FALSE(movedColor == aLight);
                VERIFY_IS_FALSE(movedColor == aDark);
                VERIFY_IS_FALSE(movedColor == intermediateInsert);

                // The app-scoped pair also still shares one object...
                VERIFY_IS_TRUE(appMarkupChild->Background == appCodeChild->Background);

                auto movedAppColor = safe_cast<xaml_media::SolidColorBrush^>(appCodeChild->Background)->Color;

                // ...and it resolved the UPDATED App.Resources Dark value (Yellow). The re-walk up the new tree
                // reaches Application.Current.Resources live, so it is neither the Light value (Blue, so it
                // updated to the new theme at its new location) nor the original Dark value (Cyan, so the
                // mid-reparent edit was observed).
                VERIFY_IS_TRUE(movedAppColor == appDarkUpdated);
                VERIFY_IS_FALSE(movedAppColor == appLight);
                VERIFY_IS_FALSE(movedAppColor == appDarkOriginal);

                // The two pairs resolved from different scopes, so they hold different objects.
                VERIFY_IS_FALSE(treeMarkupChild->Background == appMarkupChild->Background);
            });

            // Step 4: reparent just the tree-scoped pair once more, from subtree B into subtree C. Subtree C sets
            // RequestedTheme=Light but does NOT define MovingKey anywhere in its ancestor chain, so the re-walk up
            // the new tree finds nothing (FindNextResolvedValueNoRef returns null). The binding then falls back to
            // CThemeResource::RefreshValue against the dictionary it captured at install time - which is still
            // subtree A's dictionary (a successful tree-walk re-resolve updates only the resolved value, never the
            // captured dictionary weak ref). Subtree A is still alive (an empty Grid under the root), so that
            // fallback lookup succeeds under C's Light theme and yields subtree A's Default value (Red). The
            // binding neither keeps B's Magenta nor collapses to null.
            RunOnUIThread([&]()
            {
                unsigned int index = 0;
                if (subtreeB->Children->IndexOf(treeMarkupChild, &index)) { subtreeB->Children->RemoveAt(index); }
                if (subtreeB->Children->IndexOf(treeCodeChild, &index)) { subtreeB->Children->RemoveAt(index); }

                subtreeC->Children->Append(treeMarkupChild);
                subtreeC->Children->Append(treeCodeChild);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Markup and code stay in lockstep and keep sharing one object.
                VERIFY_IS_TRUE(treeMarkupChild->Background == treeCodeChild->Background);

                auto keylessColor = safe_cast<xaml_media::SolidColorBrush^>(treeCodeChild->Background)->Color;
                LOG_OUTPUT(L"> Keyless-subtree color is %x %x %x %x", keylessColor.A, keylessColor.R, keylessColor.G, keylessColor.B);

                // Fell back to the captured subtree A dictionary under C's Light theme -> A's Default (Red).
                VERIFY_IS_TRUE(keylessColor == aLight);
                // It did NOT keep subtree B's resolved value, and did NOT go null/transparent.
                VERIFY_IS_FALSE(keylessColor == bDark);
                VERIFY_IS_FALSE(keylessColor == bLight);
            });

            // Step 5: captured dictionary alive but EMPTIED. The emptySource subtree carries its own MovingKey
            // (Red) and its own child pair, so its captured dictionary is independent of subtree A. After the
            // pair resolves, MovingKey is removed from emptySource's dictionary (the dictionary OBJECT stays
            // alive - emptySource still owns it - it just no longer contains the key). Reparenting the pair into
            // the keyless subtree D forces a fallback RefreshValue against that still-live but now keyless
            // captured dictionary: GetKeyNoRef finds nothing, so RefreshValue takes the missing-key error path
            // (AG_E_PARSER_FAILED_RESOURCE_FIND) and the reparent throws - identically for markup and code.
            RunOnUIThread([&]()
            {
                // Install the code binding while emptyCodeChild is live under emptySource (captures its dict, Red).
                emptyCodeChild->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, L"MovingKey");
                VERIFY_IS_TRUE(emptyMarkupChild->Background == emptyCodeChild->Background);
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(emptyCodeChild->Background)->Color == aLight);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                DisableErrorReportingScopeGuard disableErrors;

                // Empty the captured dictionary: remove the key, but keep the dictionary object alive.
                auto emptyDefault = safe_cast<ResourceDictionary^>(
                    emptySource->Resources->ThemeDictionaries->Lookup(L"Default"));
                emptyDefault->Remove(L"MovingKey");

                // Detach both children from emptySource up front so each reparent below is a clean live-enter.
                emptySource->Children->Clear();

                // Reparenting into the keyless subtree D triggers the fallback lookup against the emptied (but
                // still alive) captured dictionary, which fails to find the key and throws - for both code and
                // markup bindings.
                bool codeThrew = false;
                try { subtreeD->Children->Append(emptyCodeChild); }
                catch (Platform::Exception^) { codeThrew = true; }
                VERIFY_IS_TRUE(codeThrew, L"Expected reparent to throw when the captured dictionary no longer contains the key (code).");

                bool markupThrew = false;
                try { subtreeD->Children->Append(emptyMarkupChild); }
                catch (Platform::Exception^) { markupThrew = true; }
                VERIFY_IS_TRUE(markupThrew, L"Expected reparent to throw when the captured dictionary no longer contains the key (markup).");
            });
            TestServices::WindowHelper->WaitForIdle();

            // Step 6: captured dictionary RELEASED. The tree-scoped pair's captured dictionary is subtree A's,
            // still alive. First reparent the pair back under subtree B so its last resolved value becomes a
            // distinctive Magenta (B's Dark). Then drop subtree A's only strong reference to that captured
            // dictionary by replacing subtreeA->Resources with a fresh dictionary; native ref-counting destroys
            // the old dictionary immediately (no GC needed), so the binding's weak ref can no longer be locked.
            // Reparenting the pair into the keyless subtree D then forces a fallback RefreshValue whose lock
            // fails - so it returns early and KEEPS the last resolved value (Magenta), neither re-resolving nor
            // throwing.
            RunOnUIThread([&]()
            {
                unsigned int index = 0;
                if (subtreeC->Children->IndexOf(treeMarkupChild, &index)) { subtreeC->Children->RemoveAt(index); }
                if (subtreeC->Children->IndexOf(treeCodeChild, &index)) { subtreeC->Children->RemoveAt(index); }
                subtreeB->Children->Append(treeMarkupChild);
                subtreeB->Children->Append(treeCodeChild);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Priming succeeded: the pair now shows subtree B's Dark value (Magenta).
                VERIFY_IS_TRUE(treeMarkupChild->Background == treeCodeChild->Background);
                VERIFY_IS_TRUE(safe_cast<xaml_media::SolidColorBrush^>(treeCodeChild->Background)->Color == bDark);

                // Release subtree A's captured dictionary. The binding holds only a weak ref to it, so once
                // subtree A drops its strong ref the dictionary is destroyed at once.
                subtreeA->Resources = ref new ResourceDictionary();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                unsigned int index = 0;
                if (subtreeB->Children->IndexOf(treeMarkupChild, &index)) { subtreeB->Children->RemoveAt(index); }
                if (subtreeB->Children->IndexOf(treeCodeChild, &index)) { subtreeB->Children->RemoveAt(index); }
                subtreeD->Children->Append(treeMarkupChild);
                subtreeD->Children->Append(treeCodeChild);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Markup and code stay in lockstep and keep sharing one object.
                VERIFY_IS_TRUE(treeMarkupChild->Background == treeCodeChild->Background);

                auto deadDictColor = safe_cast<xaml_media::SolidColorBrush^>(treeCodeChild->Background)->Color;
                LOG_OUTPUT(L"> Dead-dictionary color is %x %x %x %x", deadDictColor.A, deadDictColor.R, deadDictColor.G, deadDictColor.B);

                // The captured dictionary is gone, so the fallback lookup could not run: the binding kept its
                // last resolved value (Magenta), rather than reverting to subtree A's Red or going null.
                VERIFY_IS_TRUE(deadDictColor == bDark);
                VERIFY_IS_FALSE(deadDictColor == aLight);
            });
        }

        // Second-order (nested) override + cloning. SystemControlHighlightListAccentHighBrush is a
        // framework SolidColorBrush whose Color is itself {ThemeResource SystemAccentColor}. When an
        // ancestor scope overrides that *inner* resource (SystemAccentColor, via ColorPaletteResources),
        // resolving the outer brush there must not mutate the shared global brush: instead the engine
        // clones it and applies the overridden accent to the clone. A code SetThemeResourceBinding to the
        // outer key must participate in this exactly like markup {ThemeResource}: the in-scope bindings get
        // the cloned Purple brush, an out-of-scope binding keeps the untouched global original, and editing
        // that original has no effect on the clone.
        void ThemeResourcesTests::SetThemeResourceBindingSecondOrderOverrideAndClone()
        {
            TestCleanupWrapper cleanup;

            String^ outerKey = L"SystemControlHighlightListAccentHighBrush";
            ::Windows::UI::Color overrideAccent = Microsoft::UI::Colors::Purple;

            xaml_controls::Grid^ scopeGrid;         // ancestor overriding the inner SystemAccentColor -> Purple
            xaml_controls::Grid^ inScopeMarkup;     // markup {ThemeResource} to the outer key, under the override
            xaml_controls::Grid^ inScopeCode;       // code SetThemeResourceBinding to the outer key, under the override
            xaml_controls::Grid^ outOfScopeCode;    // code binding with no override in scope -> shared global original

            RunOnUIThread([&]()
            {
                // The scope grid overrides the second-order accent color via ColorPaletteResources, and hosts a
                // markup {ThemeResource} consumer of the outer brush in that same scope.
                String^ scopeXaml =
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources><ColorPaletteResources Accent='Purple'/></Grid.Resources>"
                    L"  <Grid x:Name='inScopeMarkup' Background='{ThemeResource SystemControlHighlightListAccentHighBrush}'/>"
                    L"</Grid>";
                scopeGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(scopeXaml));
                inScopeMarkup = safe_cast<xaml_controls::Grid^>(scopeGrid->FindName(L"inScopeMarkup"));

                // A code binding placed in the same override scope must resolve the outer key the same way.
                inScopeCode = ref new xaml_controls::Grid();
                inScopeCode->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, outerKey);
                scopeGrid->Children->Append(inScopeCode);

                // A code binding outside any override scope resolves to the shared global brush unchanged.
                outOfScopeCode = ref new xaml_controls::Grid();
                outOfScopeCode->SetThemeResourceBinding(xaml_controls::Panel::BackgroundProperty, outerKey);

                auto root = ref new xaml_controls::Grid();
                root->Children->Append(scopeGrid);
                root->Children->Append(outOfScopeCode);
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            xaml_media::SolidColorBrush^ markupClone;
            xaml_media::SolidColorBrush^ codeClone;
            xaml_media::SolidColorBrush^ original;
            ::Windows::UI::Color originalColor;

            RunOnUIThread([&]()
            {
                markupClone = safe_cast<xaml_media::SolidColorBrush^>(inScopeMarkup->Background);
                codeClone = safe_cast<xaml_media::SolidColorBrush^>(inScopeCode->Background);
                original = safe_cast<xaml_media::SolidColorBrush^>(outOfScopeCode->Background);
                originalColor = original->Color;

                // Second-order override applied: both in-scope consumers pick up the overridden accent...
                VERIFY_IS_TRUE(markupClone->Color == overrideAccent);
                VERIFY_IS_TRUE(codeClone->Color == overrideAccent);

                // ...while the out-of-scope consumer keeps the default (un-overridden) accent color.
                VERIFY_IS_FALSE(original->Color == overrideAccent);

                // The override produced a clone, not an in-place edit: the in-scope consumers resolved to a
                // different brush instance than the shared global original. Code binding clones just like markup.
                VERIFY_IS_TRUE(markupClone != original);
                VERIFY_IS_TRUE(codeClone != original);

                // The clone is cached per override scope, so markup and code share the one cloned instance.
                VERIFY_IS_TRUE(markupClone == codeClone);
            });

            // Editing the shared global original must NOT bleed into the in-scope clone: they are independent
            // objects. This is the whole point of cloning on override.
            ::Windows::UI::Color mutatedOriginal = Microsoft::UI::Colors::Red;
            RunOnUIThread([&]()
            {
                original->Color = mutatedOriginal;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // The edit took on the original (and hence on the out-of-scope consumer that uses it)...
                VERIFY_IS_TRUE(original->Color == mutatedOriginal);

                // ...but the in-scope clone is untouched, still holding the overridden accent.
                VERIFY_IS_TRUE(codeClone->Color == overrideAccent);
                VERIFY_IS_TRUE(markupClone->Color == overrideAccent);

                // Restore the shared global brush so this mutation cannot leak into later tests.
                original->Color = originalColor;
            });
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
