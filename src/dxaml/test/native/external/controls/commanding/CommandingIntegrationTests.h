// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Commanding {

    class CommandingIntegrationTests : public WEX::TestClass<CommandingIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CommandingIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"d96ffe50-c538-42ef-918b-517ad455f1e0;65d77a94-832e-4d74-af02-4e3b3f5180cc;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ValidateUICommandExecute)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ExecuteRequested is raised in response to a call to UICommand.Execute().")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUICommandCanExecuteDefaultsToTrue)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ExecuteRequested.CanExecute() returns true by default if no CanExecuteRequested handler is provided.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUICommandCanExecute)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CanExecuteRequested is raised in response to a call to ExecuteRequested.CanExecute().")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UICommandCanUpdateCanExecute)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ExecuteRequested.NotifyCanExecuteChanged() raises CanExecuteChanged.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChildCommandIsDelegatedTo)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that having a child ICommand causes Execute and CanExecute to be delegated to it.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCommandLibraryProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the properties set by default by the command library UICommands are as expected.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCommandLibraryPropertiesNonEnglish)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the properties set by default by the command library UICommands are different when not on en-US.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCommandLibraryCanBeUsedInXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StandardUICommand.Kind is settable in XAML.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChangingStandardUICommandKindChangesProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing StandardUICommand.Kind causes the command's properties to change.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChangingStandardUICommandKindDoesNotOverrideDirectlySetProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing StandardUICommand.Kind does not change properties that have been directly set by an app author.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardAcceleratorsFromCommands)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that keyboard accelerators received from commands function when the accelerator keys are pressed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectionFlyoutDismissalOnPointerMove)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that moving the mouse pointer away from a SelectionFlyout hides it.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectionFlyoutDoesNotDismissAfterExpand)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that moving the mouse pointer away from a SelectionFlyout does not hide it if it has been expanded once.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardingToOpenSubMenuGivesFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that using the keyboard to navigate to an open sub-menu gives focus to that sub-menu.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateNoLayoutCycleAt125PercentScale)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandBarFlyout does not encounter a layout cycle at 125%.")
        END_TEST_METHOD()

    private:
        void ValidateSelectionFlyoutDismissalOnPointerMove(bool shouldExpandBeforePointerMove);

        void ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind kind,
            Platform::String^ expectedLabel,
            xaml_controls::Symbol expectedSymbol,
            ::Windows::System::VirtualKey expectedAcceleratorKey,
            ::Windows::System::VirtualKeyModifiers expectedAcceleratorModifiers,
            Platform::String^ expectedDescription);

        void ValidateCommandLibraryKind(
            xaml_input::StandardUICommand^ command,
            Platform::String^ expectedLabel,
            xaml_controls::Symbol expectedSymbol,
            ::Windows::System::VirtualKey expectedAcceleratorKey,
            ::Windows::System::VirtualKeyModifiers expectedAcceleratorModifiers,
            Platform::String^ expectedDescription);

        void ValidateCommandLibraryKind(
            xaml_input::StandardUICommand^ command,
            Platform::String^ expectedLabel,
            xaml_controls::SymbolIconSource^ expectedSymbolIconSource,
            ::Windows::System::VirtualKey expectedAcceleratorKey,
            ::Windows::System::VirtualKeyModifiers expectedAcceleratorModifiers,
            Platform::String^ expectedDescription);

        void ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind kind,
            Platform::String^ englishLabel,
            Platform::String^ englishDescription);
    };

} } } } } }