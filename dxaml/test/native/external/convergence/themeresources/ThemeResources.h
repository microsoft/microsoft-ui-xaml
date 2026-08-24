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

            BEGIN_TEST_METHOD(SetThemeResourceBindingMarkupParity)
                TEST_METHOD_PROPERTY(L"Description", L"Same key via {ThemeResource} markup and via SetThemeResourceBinding produce equal resolved values and both respond to theme changes; two code bindings to the same key share one object instance.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetThemeResourceBindingMissingKeyThrows)
                TEST_METHOD_PROPERTY(L"Description", L"SetThemeResourceBinding with an unresolvable key throws, matching markup's parse failure.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetThemeResourceBindingLocalOverrideAndClear)
                TEST_METHOD_PROPERTY(L"Description", L"A local value overrides the code theme binding. ClearValue restores the default value.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetThemeResourceBindingReadOnlyPropertyThrows)
                TEST_METHOD_PROPERTY(L"Description", L"SetThemeResourceBinding targeting a read-only dependency property is rejected.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetThemeResourceBindingResolutionSources)
                TEST_METHOD_PROPERTY(L"Description", L"SetThemeResourceBinding resolves keys from global, Application.Resources, the element's own ThemeDictionaries, and an ancestor's Resources - while detached, after entering a tree that overrides them, and when installed on an already-live element.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetThemeResourceBindingTemplateScopeDivergence)
                TEST_METHOD_PROPERTY(L"Description", L"Markup {ThemeResource} (lexical/parse scope) and code SetThemeResourceBinding (runtime tree walk) resolve slightly differently by design.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetThemeResourceBindingReparentDifferentThemeAndKey)
                TEST_METHOD_PROPERTY(L"Description", L"Reparenting bound elements into a subtree with a different RequestedTheme re-resolves the binding by walking up the new tree, re-scoping to the new subtree's same-key resource (not the dictionary captured at install time) and reaching App.Resources live so a mid-reparent edit is picked up; a plain in-scope dictionary mutation (no theme change, no reparent) does NOT re-resolve; reparenting into a subtree that does not define the key falls back to the captured dictionary; if that captured dictionary is emptied the fallback throws, and if it is released the binding keeps its last resolved value; markup {ThemeResource} and code SetThemeResourceBinding behave identically and keep sharing one object per key.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetThemeResourceBindingSecondOrderOverrideAndClone)
                TEST_METHOD_PROPERTY(L"Description", L"Overriding a second-order resource (SystemAccentColor) in an ancestor scope clones the outer framework brush that references it and applies the overridden accent to the clone; a code SetThemeResourceBinding participates exactly like markup {ThemeResource} (in-scope bindings share the one cloned brush, an out-of-scope binding keeps the shared global original, and editing that original does not affect the clone).")
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

