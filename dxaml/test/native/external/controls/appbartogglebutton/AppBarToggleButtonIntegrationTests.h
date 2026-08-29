// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarToggleButton {

    class AppBarToggleButtonIntegrationTests : public WEX::TestClass<AppBarToggleButtonIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AppBarToggleButtonIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e9192bce-1f8a-48ea-8327-14058db070f2;70a0f79e-de5f-46d3-bd45-a84dcb6e99df;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create an AppBarToggleButton.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove an AppBarToggleButton from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully tap an AppBarToggleButton.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanToggle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBarToggleButton can be toggled using various input types.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetLabelProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get the AppBarButton.Label property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetIconProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get the AppBarButton.Icon property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorCreatesDefaultKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on an AppBarToggleButton causes us to generate a default value for KeyboardAcceleratorTextOverride.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorDoesNotOverrideCustomKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on an AppBarToggleButton with a value of KeyboardAcceleratorTextOverride already defined does not overwrite that value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorCreatesDefaultToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on an AppBarToggleButton causes us to generate a default tool tip.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorDoesNotOverrideCustomToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on an AppBarToggleButton with a tool tip already defined does not overwrite that value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandSetsProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand causes the AppBarToggleButton to properly pick up the properties from the object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandDoesNotOverwriteProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand does not cause an AppBarToggleButton to overwrite an already-set properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of AppBarToggleButton in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore, WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth & Height of AppBarToggleButton in various configurations.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

    private:
        static xaml_input::KeyboardAccelerator^ CreateKeyboardAccelerator(::Windows::System::VirtualKey key, ::Windows::System::VirtualKeyModifiers modifiers);
    };

} } } } } }

