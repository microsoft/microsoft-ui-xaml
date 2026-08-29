// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ComboBoxTypeAheadTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <ComboBoxHelper.h>
#include <ControlHelper.h>
#include <TreeHelper.h>

#include <random>
#include <FileLoader.h>

#include "KeyboardInjectionOverride.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ComboBox {

    bool ComboBoxTypeAheadTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ComboBoxTypeAheadTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ComboBoxTypeAheadTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ComboBoxTypeAheadTests::CanChangeSelectionWhileComboBoxIsOpenedShort()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendExpectedString(L"Lizard");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        runnerParams->SetShouldOpenDropDown(true);

        CanChangeSelectionRunner(runnerParams, ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::Twenty) / 2);
    }

    void ComboBoxTypeAheadTests::CanChangeSelectionWhileComboBoxIsOpenedLong()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendExpectedString(L"Lizard");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetShouldOpenDropDown(true);

        CanChangeSelectionRunner(runnerParams, ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::TwoHundred) / 2);
    }

    void ComboBoxTypeAheadTests::CanChangeSelectionWhileComboBoxIsClosedShort()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendExpectedString(L"Lizard");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);

        CanChangeSelectionRunner(runnerParams, ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::Twenty) / 2);
    }

    void ComboBoxTypeAheadTests::CanChangeSelectionWhileComboBoxIsClosedLong()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendExpectedString(L"Lizard");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);

        CanChangeSelectionRunner(runnerParams, ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::TwoHundred) / 2);
    }

    void ComboBoxTypeAheadTests::NoResultsFoundOpenedShort()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"Q");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::NoResultsFoundClosedShort()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"Q");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::NoResultsFoundOpenedLong()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"Q");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::NoResultsFoundClosedLong()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"Q");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::NoResultsFoundComboBoxItems()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"Q");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        runnerParams->SetShouldOpenDropDown(true);
        runnerParams->SetItemType(ComboBoxItemType::ComboBoxItem);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::NoResultsFoundStackPanelItemsPanel()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"Q");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        runnerParams->SetShouldOpenDropDown(true);
        runnerParams->SetSetupType(ComboBoxSetupType::StackPanelItemsPanel);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::NoResultsFoundBecauseSearchIsDisabled()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");

        runnerParams->SetSetupType(ComboBoxSetupType::TextSearchIsDisabled);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultOpenedShort()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultClosedShort()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultOpenedLong()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultClosedLong()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultComboBoxItems()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        runnerParams->SetShouldOpenDropDown(true);
        runnerParams->SetItemType(ComboBoxItemType::ComboBoxItem);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultStackPanelItemsPanel()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        runnerParams->SetShouldOpenDropDown(true);
        runnerParams->SetSetupType(ComboBoxSetupType::StackPanelItemsPanel);
        TypeAheadRunner(runnerParams);

        runnerParams->SetStartIndex(0);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultLoopingOpenedShort()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        runnerParams->SetStartIndex(ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::Twenty) - 1);
        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultLoopingClosedShort()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        runnerParams->SetStartIndex(ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::Twenty) - 1);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultLoopingOpenedLong()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetStartIndex(ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::TwoHundred) - 1);
        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultLoopingClosedLong()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetStartIndex(ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::TwoHundred) - 1);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultLoopingStackPanelItemsPanel()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString(L"Kangaroo");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetStartIndex(ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::TwoHundred) - 1);
        runnerParams->SetShouldOpenDropDown(true);
        runnerParams->SetSetupType(ComboBoxSetupType::StackPanelItemsPanel);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultThenHitSecondResultSequentiallyAfterOpenedLong()
    {
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"r");
        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"Arrow crab");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultThenHitSecondResultSequentiallyAfterClosedLong()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"r");
        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"Arrow crab");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultThenHitSecondResultSequentiallyAfterOpenedShort()
    {
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"r");
        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"Arrow crab");

        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultThenHitSecondResultSequentiallyAfterClosedShort()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"r");
        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"Arrow crab");

        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstAndSecondResultAndStayAfterThirdLetterNoResultsFoundOpened()
    {
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"r");
        runnerParams->AppendKeySequence(L"q");
        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"Arrow crab");

        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitFirstAndSecondResultAndStayAfterThirdLetterNoResultsFoundClosed()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"r");
        runnerParams->AppendKeySequence(L"q");
        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"Arrow crab");

        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::EnsureLeadingSpaceOpensClosesComboBox()
    {
        LeadingSpaceRunner(ComboBoxSetupType::Default);
    }

    void ComboBoxTypeAheadTests::EnsureLeadingSpaceOpensClosesComboBoxInsideControlThatHandlesSpace()
    {
        LeadingSpaceRunner(ComboBoxSetupType::InsideControlThatHandlesSpace);
    }

    void ComboBoxTypeAheadTests::CanHitFirstResultLeadingSpaces()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"K");
        runnerParams->AppendExpectedString("  Kangaroo");
        runnerParams->SetItemType(ComboBoxItemType::StringWithLeadingSpaces);

        // 20, popup is closed
        runnerParams->SetNumberOfItems(NumberOfItems::Twenty);
        TypeAheadRunner(runnerParams);

        // 20, popup is open
        runnerParams->SetShouldOpenDropDown(true);
        TypeAheadRunner(runnerParams);

        // 200, popup is open
        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        TypeAheadRunner(runnerParams);

        // 200, popup is closed
        runnerParams->SetShouldOpenDropDown(false);
        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanHitResultIncludingInteriorSpaces()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"f");
        runnerParams->AppendKeySequence(L"r");
        runnerParams->AppendKeySequence(L"i");
        runnerParams->AppendKeySequence(L"c");
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"n");
        runnerParams->AppendKeySequence(L" ");
        runnerParams->AppendKeySequence(L"l");

        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"African buffalo");
        runnerParams->AppendExpectedString(L"African leopard");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);

        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::EnsureTypeAheadWithSpacesDoesntTriggerButtonClickOpened()
    {
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"f");
        runnerParams->AppendKeySequence(L"r");
        runnerParams->AppendKeySequence(L"i");
        runnerParams->AppendKeySequence(L"c");
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"n");
        runnerParams->AppendKeySequence(L" ");
        runnerParams->AppendKeySequence(L"l");

        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"African buffalo");
        runnerParams->AppendExpectedString(L"African leopard");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetSetupType(ComboBoxSetupType::InsideControlThatHandlesSpace);

        runnerParams->SetShouldOpenDropDown(true);

        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::EnsureTypeAheadWithSpacesDoesntTriggerButtonClickClosed()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"f");
        runnerParams->AppendKeySequence(L"r");
        runnerParams->AppendKeySequence(L"i");
        runnerParams->AppendKeySequence(L"c");
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"n");
        runnerParams->AppendKeySequence(L" ");
        runnerParams->AppendKeySequence(L"l");

        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"African buffalo");
        runnerParams->AppendExpectedString(L"African leopard");

        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetSetupType(ComboBoxSetupType::InsideControlThatHandlesSpace);

        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanFindTextInObjectWithNullDisplayMemberPath()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"m");
        runnerParams->AppendExpectedString(L"Matt");

        runnerParams->SetStartIndex(0);
        runnerParams->SetItemType(ComboBoxItemType::PersonObject);

        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanFindTextInObjectWithSimpleDisplayMemberPath()
    {
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"m");
        runnerParams->AppendExpectedString(L"Matt");

        runnerParams->SetStartIndex(0);
        runnerParams->SetItemType(ComboBoxItemType::PersonObject);

        runnerParams->AppendDisplayMemberPaths(L"FirstName");

        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanFindTextInObjectWithComplexDisplayMemberPath()
    {
        // Matt's pet is a Newt, so typing N should result in Matt's object returning.
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"N");
        runnerParams->AppendExpectedString(L"Matt");

        runnerParams->SetStartIndex(0);
        runnerParams->SetItemType(ComboBoxItemType::PersonObject);
        runnerParams->SetShouldOpenDropDown(true);

        runnerParams->AppendDisplayMemberPaths(L"PetAnimal.CommonName");

        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanFindTextInObjectAfterResettingDisplayMemberPath()
    {
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::WaitForIdleBeforeAndAfter);

        // First, DisplayMemberPath is empty, so we're searching a list of names. Go from 'M' to 'Matt'.
        // Then, we set DMP to the names of the pets. Go from 'B' to 'Cleffa' since we'll match "Blue bird", which is Cleffa's pet.
        // This ensures that we can get the correct strings out of the same objects if the DisplayMemberPath changes between searches.
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"M");
        runnerParams->AppendKeySequence(L"B");
        runnerParams->AppendExpectedString(L"Matt");
        runnerParams->AppendExpectedString(L"Cleffa");

        runnerParams->SetStartIndex(0);
        runnerParams->SetItemType(ComboBoxItemType::PersonObject);
        runnerParams->SetShouldOpenDropDown(true);

        runnerParams->SetTimeBetweenKeyPressesInMs(2000);

        runnerParams->AppendDisplayMemberPaths(L"");
        runnerParams->AppendDisplayMemberPaths(L"PetAnimal.CommonName");

        TypeAheadRunner(runnerParams);
    }

    void ComboBoxTypeAheadTests::CanFindFirstStringThenNextAfterTimeout()
    {
        // If the timeout isn't hit (ie. we end up appending to the old search string rather than creating a new one as intended),
        // then this test will fail by waiting for selectionChangedEvent - we'll effectively have searched for "aa" which will stay on Aardvark
        // rather than starting at Aardvark and going to Arrow Crab as intended.
        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->AppendKeySequence(L"a");
        runnerParams->AppendKeySequence(L"a");

        runnerParams->AppendExpectedString(L"Aardvark");
        runnerParams->AppendExpectedString(L"Arrow crab");

        runnerParams->SetShouldOpenDropDown(true);
        runnerParams->SetTimeBetweenKeyPressesInMs(2000);

        TypeAheadRunner(runnerParams);
    }

    // -- Runners --
    // This runner ensures that programatically setting the selected index on a ComboBox does the
    // correct thing, even if the selected index is outside of the currently realized items, and regardless
    // of if the box is opened or closed. Note that this is not explicitly testing TypeAhead, but rather the underlying
    // framework upon which typeahead is built.
    void ComboBoxTypeAheadTests::CanChangeSelectionRunner(TypeAheadRunnerParameters* runnerParams, int endingIndex)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = SetupTypeAheadComboBoxTest(runnerParams);

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        if (runnerParams->GetShouldOpenDropDown())
        {
            LOG_OUTPUT(L"Open the ComboBox.");
            ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Keyboard);
        }

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = endingIndex;

            auto selectedValue = comboBox->SelectedValue;
            VERIFY_IS_NOT_NULL(selectedValue);

            VerifySelectedValueMatchesExpectedString(selectedValue, runnerParams->GetExpectedStrings()->GetAt(0));
        });
        TestServices::WindowHelper->WaitForIdle();

        if (runnerParams->GetShouldOpenDropDown())
        {
            LOG_OUTPUT(L"Close the ComboBox.");
            ComboBoxHelper::CloseComboBox(comboBox);
        }
    }

    // This runner ensures that the TypeAhead scenario works in a number of different configurations.
    // First, the runner sets up the ComboBox, and opens it (or not).
    // Then, the runner presses some keys on the keyboard to simulate a user attempting to TypeAhead.
    // Since TypeAhead changes the SelectedIndex of the ComboBox, we listen for SelectionChange events and
    //   we verify that the pressed keys produce the expected selection string.
    // This method utilizes a TypeAheadRunnerParameters object to inform the setup and execution of the test.
    // It has a number of settable knobs:
    // - NumberOfItems : an enum that specifies the number of items in the ComboBox (see note at def of NumberOfItems)
    // - KeySequences : a vector of strings representing the keys to be pressed.
    // - ExpectedStrings : a vector of strings representing the expected values that the search should produce.
    //      We expect a SelectionChanged event to be fired for every string in this Vector.
    //      If ExpectedStrings is null, then we didn't expect a key sequence to produce a string, so no SelectionChanged
    //      events should be raised in this scenario.
    // - StartIndex : The initially set SelectedIndex value. Defaults to -1.
    // - ShouldOpenDropDown : Determines if the DropDown should be opened/closed. Flexes the character routing from the popup
    // - ComboBoxSetupType : An enum that specifies parameters about the ComboBox. (See note at def of ComboBoxSetupType)
    // - ComboBoxItemType : An enum that specifies the parameters about the ComboBox Items. (See note at def of ComboBoxItemType)
    // - DisplayMemberPaths : A vector of DisplayMemberPath strings. The first string will be used in initial setup, and
    //      secondary DMPs will be set after every character press. This tests the corner case of DMP changing between searches.
    // - TimeBetweenKeyPressesInMs : Informs a timeout between character presses, in ms. Can be used to reset the search string.
    void ComboBoxTypeAheadTests::TypeAheadRunner(TypeAheadRunnerParameters* runnerParams)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = SetupTypeAheadComboBoxTest(runnerParams);
        bool errorOccurred = false;

        if (runnerParams->GetSetupType() == ComboBoxSetupType::InsideControlThatHandlesSpace)
        {
            xaml_controls::Button^ button;
            RunOnUIThread([&]()
            {
                button = safe_cast<xaml_controls::Button^>(comboBox->Parent);
                THROW_IF_NULL(button);
            });

            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            clickRegistration.Attach(button,
                [&](){
                    // If the button ever gets the click, we should fail the test.
                    VERIFY_FAIL(L"The button should never have been clicked via Spacebar.");
                });
        }

        if (runnerParams->GetStartIndex() >= 0)
        {
            RunOnUIThread([&]()
            {
                comboBox->SelectedIndex = runnerParams->GetStartIndex();
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        UINT selectionChangedCounter = 0;
        auto selectionChangedCorrectNumberOfTimesEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
        selectionChangedRegistration.Attach(comboBox,
        [&]()
        {
            try
            {
                if (runnerParams->GetExpectedStrings()->Size == 0)
                {
                    // We didn't expect to get a selection changed if expectedStrings is null.
                    VERIFY_FAIL(L"SelectionChanged was fired, which we didn't expect.");
                }

                if (selectionChangedCounter >= runnerParams->GetExpectedStrings()->Size)
                {
                    // If we get more selection changes than we expect, fail the test.
                    VERIFY_FAIL(L"SelectionChanged fired more times than we expected.");
                }

                Platform::Object^ selectedValue = comboBox->SelectedValue;
                VERIFY_IS_NOT_NULL(selectedValue);

                VerifySelectedValueMatchesExpectedString(selectedValue, runnerParams->GetExpectedStrings()->GetAt(selectionChangedCounter));

                selectionChangedCounter++;

                if (selectionChangedCounter == runnerParams->GetExpectedStrings()->Size)
                {
                    selectionChangedCorrectNumberOfTimesEvent->Set();
                }
            }
            catch (...)
            {
                // Catching exceptions to prevent unhandled exception in an event handler which is reported as a Watson crash.
                errorOccurred = true;
            }
        });

        RunOnUIThread([&]()
        {
            comboBox->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        if (runnerParams->GetShouldOpenDropDown())
        {
            LOG_OUTPUT(L"Open the ComboBox.");
            ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Keyboard);
        }

        bool shouldUpdateDisplayMemberPath = runnerParams->GetDisplayMemberPaths()->Size > 1;

        for (UINT i = 0; i < runnerParams->GetKeySequences()->Size && !errorOccurred; i++)
        {
            LOG_OUTPUT(L"ComboBox: Injecting key sequence");
            LOG_OUTPUT(L"%s", runnerParams->GetKeySequences()->GetAt(i)->Data());

            TestServices::KeyboardHelper->PressKeySequence(runnerParams->GetKeySequences()->GetAt(i));

            Sleep(runnerParams->GetTimeBetweenKeyPressesInMs());

            // The first DMP was set in the original setup. After each keypress, update the DMP.
            // Only update the DMP if there are more to update.
            if (shouldUpdateDisplayMemberPath && runnerParams->GetDisplayMemberPaths()->Size > i + 1)
            {
                RunOnUIThread([&]()
                {
                    comboBox->DisplayMemberPath = runnerParams->GetDisplayMemberPaths()->GetAt(i + 1);
                });
            }
        }

        VERIFY_IS_FALSE(errorOccurred, L"Checking if an error occurred in the SelectionChanged event handler.");

        if (runnerParams->GetExpectedStrings()->Size != 0)
        {
            LOG_OUTPUT(L"Waiting for selectionChanged event.");
            selectionChangedCorrectNumberOfTimesEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }
        else
        {
            // If expectedStrings == nullptr, then our startIndex should not have changed.
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(runnerParams->GetStartIndex(), comboBox->SelectedIndex);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        if (runnerParams->GetShouldOpenDropDown())
        {
            // Close the comboBox programatically (instead of space) for two reasons:
            // 1) Don't want to change selection.
            // 2) Keyboard->Accept = Space, which will be appended to the search string in typing mode and not close the box.

            LOG_OUTPUT(L"Open the ComboBox.");
            ComboBoxHelper::CloseComboBox(comboBox);
        }
    }

    // Since we're potentially affecting the way we handle the space bar, this runner ensures that just pressing space will open and close the ComboBox
    // (even when it is in a control that would otherwise eat it). As with the SelectionChanged tests, this runner is a "make sure that the baseline works" runner.
    // If this behavior was broken and text search was incorrectly eating spaces, then our Opening event would time out.
    void ComboBoxTypeAheadTests::LeadingSpaceRunner(ComboBoxSetupType setupType)
    {
        TestCleanupWrapper cleanup;

        auto runnerParams = new TypeAheadRunnerParameters();
        runnerParams->SetNumberOfItems(NumberOfItems::TwoHundred);
        runnerParams->SetSetupType(setupType);

        xaml_controls::ComboBox^ comboBox = SetupTypeAheadComboBoxTest(runnerParams);

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the ComboBox with space");
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Keyboard);

        LOG_OUTPUT(L"Close the ComboBox.");
        ComboBoxHelper::CloseComboBox(comboBox);
    }

    // -- Setup Methods --
    xaml_controls::ComboBox^ ComboBoxTypeAheadTests::SetupTypeAheadComboBoxTest(TypeAheadRunnerParameters* runnerParams)
    {
        xaml_controls::ComboBox^ comboBox = nullptr;
        xaml_controls::Control^ subtree = nullptr;

        RunOnUIThread([&]()
        {
            switch(runnerParams->GetSetupType())
            {
                case ComboBox::ComboBoxSetupType::StackPanelItemsPanel:
                {
                    subtree = comboBox = CreateStackPanelComboBox();
                    break;
                }
                case ComboBox::ComboBoxSetupType::InsideControlThatHandlesSpace:
                {
                    auto button = CreateComboBoxInsideControlThatHandlesSpace();
                    comboBox = dynamic_cast<xaml_controls::ComboBox^>(button->FindName(L"comboBox"));

                    subtree = button;
                    break;
                }
                case ComboBox::ComboBoxSetupType::TextSearchIsDisabled:
                {
                    subtree = comboBox = ref new xaml_controls::ComboBox();
                    comboBox->IsTextSearchEnabled = false;
                    break;
                }
                case ComboBox::ComboBoxSetupType::Default:
                default:
                {
                    subtree = comboBox = ref new xaml_controls::ComboBox();
                    break;
                }
            }
        });

        // We can't do this work on the UI Thread because we can't await for the file to be loaded on the UI thread.
        Platform::Object^ comboBoxItems = nullptr;
        switch (runnerParams->GetItemType())
        {
            case ComboBoxItemType::String:
            {
                comboBoxItems = GetListOfAnimalStrings(runnerParams->GetNumberOfItems());
                break;
            }
            case ComboBoxItemType::StringWithLeadingSpaces:
            {
                comboBoxItems = GetListOfAnimalStrings(runnerParams->GetNumberOfItems(), L"  ");
                break;
            }
            case ComboBoxItemType::ComboBoxItem:
            {
                AddComboBoxItems(comboBox, runnerParams->GetNumberOfItems());
                break;
            }
            case ComboBoxItemType::PersonObject:
            {
                comboBoxItems = GetListOfPersonObjects();
                break;
            }
            default:
            {
                comboBoxItems = ref new Platform::Collections::Vector<Platform::String^>();
                break;
            }
        }

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();

            comboBox->ItemsSource = comboBoxItems;

            if (runnerParams->GetDisplayMemberPaths()->Size > 0)
            {
                comboBox->DisplayMemberPath = runnerParams->GetDisplayMemberPaths()->GetAt(0);
            }

            rootPanel->Children->Append(subtree);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return comboBox;
    }

    void ComboBoxTypeAheadTests::AddComboBoxItems(xaml_controls::ComboBox^ comboBox, ComboBox::NumberOfItems numberOfItems)
    {
        Platform::Collections::Vector<Platform::String^>^ animalList = GetListOfAnimalStrings(numberOfItems);

        RunOnUIThread([&]()
        {
            for (UINT i = 0; i < animalList->Size; i++)
            {
                xaml_controls::TextBlock^ itemTextBlock = ref new xaml_controls::TextBlock();
                itemTextBlock->Text = animalList->GetAt(i);

                xaml_controls::ComboBoxItem^ comboItem = ref new xaml_controls::ComboBoxItem();
                comboItem->Content = itemTextBlock;

                comboBox->Items->Append(comboItem);
            }
        });
    }

    Platform::Collections::Vector<Platform::String^>^ ComboBoxTypeAheadTests::GetListOfAnimalStrings(ComboBox::NumberOfItems numberOfItems, Platform::String^ prefix)
    {
        Platform::String^ fileName = L"TypeAheadAnimals" + ConvertNumberOfItemsEnumToInt(numberOfItems) + L".txt";

        auto animalList = ReadLinesFromFile(GetPackageFolder() + L"resources\\native\\controls\\ComboBox\\" + fileName);

        auto itemList = ref new Platform::Collections::Vector<Platform::String^>();

        for (UINT i = 0; i < animalList->Size; i++)
        {
            itemList->Append(Platform::String::Concat(prefix, animalList->GetAt(i)));
        }

        LOG_OUTPUT(L"Number of items: %d", itemList->Size);
        return itemList;
    }

    Platform::Collections::Vector<PetOwnerObject^>^ ComboBoxTypeAheadTests::GetListOfPersonObjects()
    {
        auto personList = ref new Platform::Collections::Vector<PetOwnerObject^>();

        // There are 20 people objects, to match up with our 20 animal objects. Each person gets an animal as a pet.
        Platform::String^ fileName = L"TypeAheadPeople20.txt";
        auto namesList = ReadLinesFromFile(GetPackageFolder() + L"resources\\native\\controls\\ComboBox\\" + fileName);

        VERIFY_ARE_EQUAL(namesList->Size, ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems::Twenty));

        auto animalList = GetListOfAnimalStrings(ComboBox::NumberOfItems::Twenty);
        for (UINT i = 0; i < namesList->Size; i++)
        {
            personList->Append(ref new PetOwnerObject(namesList->GetAt(i)));
            personList->GetAt(i)->SetPetAnimal(animalList->GetAt(i));
        }

        return personList;
    }

    xaml_controls::ComboBox^ ComboBoxTypeAheadTests::CreateStackPanelComboBox()
    {
        auto comboBox = dynamic_cast<xaml_controls::ComboBox^> (xaml_markup::XamlReader::Load(
            L"<ComboBox xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
            L"    <ComboBox.ItemsPanel> "
            L"        <ItemsPanelTemplate> "
            L"            <StackPanel /> "
            L"        </ItemsPanelTemplate> "
            L"    </ComboBox.ItemsPanel> "
            L"</ComboBox>"
            ));

        return comboBox;
    }

    xaml_controls::Button^ ComboBoxTypeAheadTests::CreateComboBoxInsideControlThatHandlesSpace()
    {
        auto button = dynamic_cast<xaml_controls::Button^> (xaml_markup::XamlReader::Load(
            L"<Button x:Name='parentButton' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
            L"  <Button.Content>"
            L"    <ComboBox x:Name='comboBox' />"
            L"  </Button.Content>"
            L"</Button>"));

        return button;
    }

    // -- Verification helpers --
    void ComboBoxTypeAheadTests::VerifySelectedValueMatchesExpectedString(Platform::Object^ selectedValue, Platform::String^ expectedString)
    {
        LOG_OUTPUT(L"VerifySelectedValueMatchesExpectedString - expectedString: %s", expectedString->Data());

        Platform::String^ selectedValueString;
        if (dynamic_cast<Platform::String^>(selectedValue) != nullptr)
        {
            selectedValueString = dynamic_cast<Platform::String^>(selectedValue);
            LOG_OUTPUT(L"String^ case - selectedValueString: %s", selectedValueString->Data());
        }
        else if (dynamic_cast<PetOwnerObject^>(selectedValue) != nullptr)
        {
            selectedValueString = dynamic_cast<PetOwnerObject^>(selectedValue)->ToString();
            LOG_OUTPUT(L"PetOwnerObject^ case - selectedValueString: %s", selectedValueString->Data());
        }
        else if (dynamic_cast<PetObject^>(selectedValue) != nullptr)
        {
            selectedValueString = dynamic_cast<PetObject^>(selectedValue)->ToString();
            LOG_OUTPUT(L"PetObject^ case - selectedValueString: %s", selectedValueString->Data());
        }
        else if (dynamic_cast<xaml_controls::ComboBoxItem^>(selectedValue) != nullptr)
        {
            // Assumes our ComboBoxItem just has a plain old TextBlock (default case).
            auto comboBoxItem = dynamic_cast<xaml_controls::ComboBoxItem^>(selectedValue);
            auto comboBoxTextBlock = safe_cast<xaml_controls::TextBlock^>(comboBoxItem->Content);
            selectedValueString = comboBoxTextBlock->Text;
            LOG_OUTPUT(L"ComboBoxItem^ case - selectedValueString: %s", selectedValueString->Data());
        }
        else
        {
            VERIFY_FAIL(L"The selectedItem is of an unexpected type.");
        }

        // Long term, flat string comparison isn't the way to go here, since our matching logic doesn't use literal ARE_EQUAL.
        // TODO [6750500]: Write test for inexact text matching (special character cases)
        VERIFY_ARE_EQUAL(expectedString, selectedValueString);
    }

    UINT ComboBoxTypeAheadTests::ConvertNumberOfItemsEnumToInt(ComboBox::NumberOfItems numberOfItemsEnum)
    {
        return static_cast<UINT>(numberOfItemsEnum);
    }

} } } } } }
