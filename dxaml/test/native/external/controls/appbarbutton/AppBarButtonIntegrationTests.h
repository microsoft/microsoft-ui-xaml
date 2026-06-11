// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarButton {

    class AppBarButtonIntegrationTests : public WEX::TestClass<AppBarButtonIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AppBarButtonIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"d96ffe50-c538-42ef-918b-517ad455f1e0;65d77a94-832e-4d74-af02-4e3b3f5180cc;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create an AppBarButton.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove an AppBarButton from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully tap an AppBarButton.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetLabelProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get the AppBarButton.Label property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetIconProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get the AppBarButton.Icon property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseThemeAnimations)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorCreatesDefaultKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on an AppBarButton causes us to generate a default value for KeyboardAcceleratorTextOverride.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorDoesNotOverrideCustomKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on an AppBarButton with a value of KeyboardAcceleratorTextOverride already defined does not overwrite that value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorCreatesDefaultToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on an AppBarButton causes us to generate a default tool tip.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorDoesNotOverrideCustomToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on an AppBarButton with a tool tip already defined does not overwrite that value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandSetsProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand causes the AppBarButton to properly pick up the properties from the object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandDoesNotOverwriteProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand does not cause an AppBarButton to overwrite an already-set value of the properties from the object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of AppBarButton in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTreeWithIcons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of AppBarButton instances with icons in the primary and secondary command collections.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth & Height of AppBarButton in various configurations.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LabelOnRightStyleIsDisabled)
            TEST_METHOD_PROPERTY(L"Description", L"Verify LabelOnRightStyle is controlled by quirk.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()
        
    private:
        static xaml_input::KeyboardAccelerator^ CreateKeyboardAccelerator(::Windows::System::VirtualKey key, ::Windows::System::VirtualKeyModifiers modifiers);
    };

} } } } } }

