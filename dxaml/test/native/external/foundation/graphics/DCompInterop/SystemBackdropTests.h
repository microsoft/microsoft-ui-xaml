// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RegKeyHelper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class SystemBackdropTests : public WEX::TestClass<SystemBackdropTests>
{
public:
    BEGIN_TEST_CLASS(SystemBackdropTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
        TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(APITest)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create SystemBackdrops in code and markup.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DesktopAcrylicInWindowedPopup)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add DesktopAcrylic to Windowed Popup.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DesktopAcrylicInNonWindowedPopup)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add DesktopAcrylic to Non-Windowed Popup.")
        #ifndef MUX_PRERELEASE
        // Test fails in Release pipeline when Experimental Velocity types are unavailable
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        #endif
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(MicaInWindowedPopup)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add Mica to Windowed Popup.")
    END_TEST_METHOD()

    TEST_METHOD(OnTargetConnectedDisconnected)

    TEST_METHOD(SharedSystemBackdrop)

    TEST_METHOD(AddRemoveSystemBackdropTarget)

    TEST_METHOD(DefaultConfiguration)

    TEST_METHOD(MenuFlyoutBackdrop);

    TEST_METHOD(ChangeWindowContent)

private:
    void VerifyPopupHelper(Private::Infrastructure::WindowHelper^ wh, Microsoft::UI::Xaml::Controls::Primitives::Popup^ popup, bool expectedIsWindowedPopupOpen, bool expectedIsPlacementVisualParented);
};

} } } } } }
