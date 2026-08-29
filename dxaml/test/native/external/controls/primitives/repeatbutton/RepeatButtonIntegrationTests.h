// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace RepeatButton {

    class RepeatButtonIntegrationTests : public WEX::TestClass<RepeatButtonIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(RepeatButtonIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"2b4b21d5-e5c0-4fad-bed4-62030fe191a1;eadeac67-1552-4876-a67e-dc1fcb8a6e25;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a RepeatButton.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a RepeatButton from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClickUsingTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that RepeatButton can launch an event when tapped.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UIETree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of RepeatButton in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of RepeatButton.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeRepeatDelay)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can change a RepeatButton's delay and cause it to repeatedly fire an event when tapped.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 20928844: Re-enable after investigating why software injection causes this to fail.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanActivateWithSpaceKeyInput)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can activate a RepeatButton using the space key.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")  // Allow XAML apps to also have XamlIslandRoots (part 2)
                                            // This test is failing in WPF hosting because the XamlIslandRoot content doesn't automatically
                                            // get focused.  This will be fixed when we move XamlIslandRoot content into a RootScrollViewer.
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanActivateWithClickModeHover)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can activate a RepeatButton set to ClickMode::Hover by hovering a pointer over it.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

    private:
        static const UINT c_repeatButtonDelay = 10;
        static const UINT c_longRepeatButtonDelay = 100;
        static const UINT c_repeatButtonInterval = 10;
        static const UINT c_longRepeatButtonInterval = 100;
        static const UINT c_holdDuration = 200;

        xaml_primitives::RepeatButton^ SetupButtonTestUI(
            UINT delay,
            UINT interval,
            xaml_controls::ClickMode mode,
            SafeEventRegistrationType(xaml_primitives::RepeatButton, Click)& clickRegistration,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> clickEvent);
    };

} } } } } } }

