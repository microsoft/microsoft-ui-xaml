// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft::UI::Xaml::Tests {
    namespace Foundation::Input::Focus {
        class BasicFocusTests : public WEX::TestClass<BasicFocusTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicFocusTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
            END_TEST_CLASS()            

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)            

            BEGIN_TEST_METHOD(LeftMouseClickAButtonAndTextBox)
                TEST_METHOD_PROPERTY(L"Description", L"Validates Click, GotFocus and LostFocus events by clicking a Button and TextBox with the left mouse button.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanChangeFocusWithKeyboard)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ability to navigate from control to control with the keyboard.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CannotChangeFocusWithoutTabStop)
                TEST_METHOD_PROPERTY(L"Description", L"Validates inability to focus control with the FocusManager::TryMoveFocus method when IsTabStop is always false.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // DCPP: RS4 Test Failure: BasicFocusTests::CannotChangeFocusWithoutTabStop - unexpected focus state on elements
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanChangeFocusWithOneTabStop)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ability to focus control with the FocusManager::TryMoveFocus method when IsTabStop is true for one control.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeFocusWithTryMoveFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ability to navigate from control to control with the FocusManager::TryMoveFocus method.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanChangeFocusProgrammatically)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ability to navigate from control to control programmatically.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClearCollectionWithFocusedElement)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClearCollectionWithFocusedElementUsingKeyboard)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ProgramaticallySettingFocusStateToUnfocusedShouldThrowInvalidArgumentException)
                TEST_METHOD_PROPERTY(L"Description", L"Programatically setting FocusState to Unfocused should throw an InvalidArgumentException.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TabStopWrapsFocusForListView)
                TEST_METHOD_PROPERTY(L"Description", L"Hitting tab on the last focused element on a page should wrap focus to the first element.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Focus state mismatch after first run
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FireEventWhenFocusedElementLeavesTree)
                TEST_METHOD_PROPERTY(L"Description", L"On Xbox, when we a focused element is leaving the tree, we want to fire an event that allows them to change focus")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateFocusApisWithInvalidSearchRootInUAP)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TryMoveFocus and FindNextElement with FindElementOptions with invalid SearchRoot works fine in UAP")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") 
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FindNextFocusableElementReturnsCorrectElement)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that calling FindNextFocusableElement returns the appropriate element")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // FindNextFocusableElement is only useful in UAP hosting mode, and so is deprecated.
                                                              // Please use FindNextElement with a search root set to XamlRoot.Content instead.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FindNextFocusableElementForUIElement)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that calling FindNextFocusableElement returns the appropriate element, UIElement variant")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CannotSetRichTextBlockOverflowAsTabStop)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that setting RichTextBlock.IsTabStop() fails")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FindNextElementReturnsDependencyObject)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that calling FindNextElement will return a DependencyObject element")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FireEventWhenFocusedElementChangesState)
                TEST_METHOD_PROPERTY(L"Description", L"when we a focused element is collapsed/changed to disabled, we want to fire an event that allows them to change focus")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCyclingWhenTabNavigationSet)
                TEST_METHOD_PROPERTY(L"Description", L"When TabNavigation is set to Cycle, verify that pressing tab functions correctly")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyShiftTabWhenOnceTabNavigationSet)
                TEST_METHOD_PROPERTY(L"Description", L"When TabNavigation is set to Once, verify that pressing shift+tab functions correctly")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF") 
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyShiftTabWithNavigationOnceWithNestedButtons)
                TEST_METHOD_PROPERTY(L"Description", L"When TabNavigation is set to Once, verify that pressing shift+tab honors the setting for nested elements")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF") 
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCyclingWithTabIndexWhenTabNavigationSet)
                TEST_METHOD_PROPERTY(L"Description", L"When TabNavigation is set to Cycle and tab indexes have been set, verify that pressing tab functions correctly")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetFocusOnTheFirstFocusableElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when a TH app is launched, the focus manager sets focus on the first focusable element")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusActivationAndSIP)
                TEST_METHOD_PROPERTY(L"Description", L"Validates SIP showing/hiding when app is activated/deactivated")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyElementFocusIsSetAfterPluginFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that focus on the first focusable element is only set after plugin focus has been set")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Method") //We need to wait for CoreWindowActivated event
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Event timed out
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoNotSetFocusOnElementWithCollapsedParent)
                TEST_METHOD_PROPERTY(L"Description", L"When a parent of the focusing element is collapsed, don't set focus on the element")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFocusRectShowsOnXboxOnInitialLoad)
                TEST_METHOD_PROPERTY(L"Description", L"Verified that when the page loads initially, we set focus on the right element and show the focus rect on it")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // We need to this test to run after AppLaunch
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Event times out
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoNotSetFocusOnHyperlinkWithDisabledParent)
                TEST_METHOD_PROPERTY(L"Description", L"When a parent of a hyperlink is disabled, don't set focus on the hyperlink")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyInitialFocusOnLoadOnPhone)
                TEST_METHOD_PROPERTY(L"Description", L"Verify when a page loads, focus is set to a XAML element that will not cause a SIP to show")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // We need to this test to run after AppLaunch
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFocusMoveAfterLeavingTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verify the correct element gets focus after the focused element leaves the tree.")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
            void CheckFocusStatusForAllControls(
                _In_ std::vector<Microsoft::UI::Xaml::Controls::Control^>* pControlsVect,
                _In_opt_ Microsoft::UI::Xaml::Controls::Control^ expectedToBeFocused,
                FocusState expectedFocusState) const;
            void ChangeFocusBasedOnTabStop(_In_ bool hasATabStop);

            enum class TabType { Tab, ShiftTab };
            void BasicFocusTests::VerifyTabNavigationWorker(
                const wchar_t* xamlString,
                const wchar_t* expectedVisitationOrder,
                TabType tabType);
        };

    }
}

