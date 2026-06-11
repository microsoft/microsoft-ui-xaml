// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ToggleSwitch {

    class ToggleSwitchIntegrationTests : public WEX::TestClass<ToggleSwitchIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ToggleSwitchIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ToggleSwitch.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ToggleSwitch from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFireToggleEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ToggleSwitch can fire a Toggled event when the state of the toggle is changed.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of ToggleSwitch in various visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPanVerticallyOverToggleSwitchToScroll)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can pan vertically over a ToggleSwitch to scroll content.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of a ToggleSwitch with/without a Header")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPanHorizontallyOverToggleSwitchToSelect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can pan horizontally over a ToggleSwitch to toggle state of the control.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDragHorizontallyOverToggleSwitchToSelect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can drag horizontally over a ToggleSwitch to toggle state of the control.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanToggleUsingTapInput)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can use tap input to toggle state of the control.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanToggleUsingSpace)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that pressing Space toggles the state of the control.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesNotToggleUsingDirectionalKeys)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that pressing Home, End, Up, Down, Left, and Right does not toggle the state of the control.")
        END_TEST_METHOD()
    };

} } } } } }

