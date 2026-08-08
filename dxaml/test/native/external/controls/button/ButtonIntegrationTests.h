// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Button {

class ButtonIntegrationTests : public WEX::TestClass<ButtonIntegrationTests>
{
public:
    BEGIN_TEST_CLASS(ButtonIntegrationTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"ab3da967-c6a7-4e5b-b3c4-c53c6c46175c;a69ddfa4-5142-4bed-887d-6d0ca14a3473")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(CanInstantiate)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a Button.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a Button from the live tree.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanClickUsingTap)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the click event is fired when the button is tapped.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanFireRightTappedEventUsingHold)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the RightTapped event is fired when the button is held.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanRecoverKeyboardInvocationAfterPointerCancellation)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that a button can still be invoked by keyboard after its parent is collapsed while the pointer is pressed.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateUIElementTree)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of Button in various visual states.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        TEST_METHOD_PROPERTY(L"Ignore", L"True") //Unreliable test: ButtonIntegrationTests::ValidateUIElementTree
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateFootprint)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of Button.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanActivateWithClickModeHover)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can activate a Button set to ClickMode::Hover by hovering a pointer over it.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanExecuteICommands)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can use CommandParameters and execute ICommands with a click.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanDetectChangesToCanExecuteWithoutBeingInVisualTree)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we properly pick up changes to a command's CanExecute property that occur when we're out of the visual tree once we're back in the visual tree.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyStatesForMouseInteraction)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies the VSM state and FocusState/IsPointerOver/IsPressed properties when Button is used with mouse")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanInheritFromButtonWithoutImplementingIWeakReferenceSource)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can inherit from Button/ButtonBase in WRL with a class that doesn't implement IWeakReferenceSource.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateAccentButtonUIElementTree)
        TEST_METHOD_PROPERTY(L"Description", L"Validates a button with the AccentButtonStyle style applied.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateSettingUICommandSetsProperties)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand causes the Button to properly pick up the properties from the object.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateSettingUICommandDoesNotOverwriteProperties)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand does not cause a Button to overwrite an already-set value of the properties.")
    END_TEST_METHOD()

private:
    Platform::String^ m_commandParameter = "buttonCommandParameter1";
};

} } } } } }

