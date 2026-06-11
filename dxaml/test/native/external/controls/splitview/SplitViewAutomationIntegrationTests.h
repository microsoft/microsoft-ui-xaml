// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace SplitView {

    class SplitViewAutomationIntegrationTests : public WEX::TestClass<SplitViewAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(SplitViewAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Desktop
        //

        BEGIN_TEST_METHOD(ValidateLightDismissWindowPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the SplitView control Overlay and CompactOverlay modes support window pattern, whereas Inline and CompactInline modes do not.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF_HOSTING_MODE_FAILURE: Can't find element "PaneRoot"
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLightDismissCloseButton)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the SplitView control Overlay and CompactOverlay modes support lightdismiss button, whereas Inline and CompactInline modes do not.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF_HOSTING_MODE_FAILURE: Can't find element "LightDismiss"
        END_TEST_METHOD()

    private:
        void ValidateLightDismissWindowPatternWorker(xaml_controls::SplitViewDisplayMode displayMode, bool shouldLightDismiss);
        void ValidateLightDismissCloseButtonWorker(xaml_controls::SplitViewDisplayMode displayMode, bool shouldLightDismiss);
        Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureDisableTransitionsForTest;
    };

} } } } } }

