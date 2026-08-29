// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

    class AccessKeyIntegrationTests : public WEX::TestClass<AccessKeyIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AccessKeyIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ButtonBasicIntegrationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that basic access key functions on Button.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RadioButtonBasicIntegrationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that basic access key functions on RadioButton.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RepeatButtonBasicIntegrationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that basic access key functions on RepeatButton.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HyperlinkButtonBasicIntegrationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that basic access key functions on HyperLinkButton.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CheckBoxBasicIntegrationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that basic access key functions on CheckBox.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InvokeAccessKeysOnMultipleButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can invoke on multiple buttons with corresponding access keys.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InvokeAccessKeysOnMultipleButtonsOfAppBar)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can invoke on buttons inside AppBar using access keys.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InvokeAccessKeysOnMultipleButtonsOfContentDialog)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can invoke on buttons inside content dialog using access keys.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ComboBoxAccessKeyIntegrationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates access key invocation on ComboBox.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TextBoxAccessKeyIntegrationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates text input while access key mode is active.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // UAP hosting required: display mode failure
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AccessHotKeyIntegrationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Integration tests for hot key invocation.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AccessKeyFilteringIntegrationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Integration tests for access key fitering and events.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NestedMenuFlyoutCanBeNavigatedWithAccessKeys)
            TEST_METHOD_PROPERTY(L"Description", L"A nested MenyFlyOut can be navigated with Access Keys.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNavigateScopesOnInvocation)
            TEST_METHOD_PROPERTY(L"Description", L"Scope will automatically change when an element that is a scope owner is invoked.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PointerInputExitsAccessKeyMode)
            TEST_METHOD_PROPERTY(L"Description", L"Verify pointer input causes accesskey mode to exit.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoNotEnterAKModeWhenNoAKElements)
            TEST_METHOD_PROPERTY(L"Description", L"When nothing in the tree is using AccessKeys, do not enter AK mode")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TabExitsAKMode)
            TEST_METHOD_PROPERTY(L"Description", L"When we tab, we should exit ak mode and process the tab normally")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // UAP hosting required: cannot exit access key mode
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DirectionArrowsExitsAKMode)
            TEST_METHOD_PROPERTY(L"Description", L"When using the direction arrows, we should exit ak mode and process the input normally")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // UAP hosting required: cannot exit access key mode
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ElementEnteringTreeFiresAKEvents)
            TEST_METHOD_PROPERTY(L"Description", L"When an element is entering the tree, we should fire the Show event")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ElementEnteringTreeDuringFilterFiresAKEvents)
            TEST_METHOD_PROPERTY(L"Description", L"When an element is entering the tree, we should fire the Show event")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ElementEnteringTreeDuringFilterDoesNotFiresAKEvents)
            TEST_METHOD_PROPERTY(L"Description",
                L"When an element is leaving the tree during filtering, but it has filtered out already, we should not fire the Hide event")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ElementLeavingTreeFiresAKEvents)
            TEST_METHOD_PROPERTY(L"Description", L"When an element is entering the tree during filter, we should fire the Hide event")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ElementLeavingTreeDuringFilterFiresAKEvents)
            TEST_METHOD_PROPERTY(L"Description", L"When an element is leaving the tree during filter and is a valid filter, we should fire the Hide event")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ElementLeavingTreeDuringFilterDoesNotFiresAKEvents)
            TEST_METHOD_PROPERTY(L"Description",
                L"When an element is leaving the tree during filter and is not a valid filter, we should not fire the Hide event")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangingVisibilityFiresCorrectEvents)
            TEST_METHOD_PROPERTY(L"Description", L"When an element is being collapsed or is becoming visible, we should fire the right events at the right time ")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangingIsEnabledFiresCorrectEvents)
            TEST_METHOD_PROPERTY(L"Description", L"When an element becomes disabled or enabled, we should fire the right events at the right time ")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ScopeOwnerLeavingTreeShouldUpdateScope)
            TEST_METHOD_PROPERTY(L"Description", L"When the scope owner leaves the tree, we should update the scope to it's parent")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BackOutAcrossEmptyScope)
            TEST_METHOD_PROPERTY(L"Description", L"Back out across an empty scope.  Ensure we skip the empty scope and don't get stuck.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BackOutAcrossEmptyScopeAndExit)
            TEST_METHOD_PROPERTY(L"Description", L"Back out across an empty scope and exit AccessKey DisplayMode entirely.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AppBarExitsAKModeWhenESCPressed)
            TEST_METHOD_PROPERTY(L"Description", L"When we are on an appbar (which handles ESC) and it is the root scope, we should still exit ak mode")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanControlWhenAKModeExits)
            TEST_METHOD_PROPERTY(L"Description", L"When the DismissAccessKeyOnInvoke property is set to true on a control, AKMode should dismiss when that control is invoked")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(OnlyNakedAltsEnterAKMode)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that when other keys are pressed with alt, that AK mode does not enter.  If a non-modifier key is down when an alt down/up is pressed, still enter.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EnterAndSpaceExitAKMode)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that when a space or enter is pressed while in access key mode, that we exit the mode and do not handle the enter or space.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThatErrorsThrownInInvalidScenarios)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that when we have an AKO with IsAccessKeyOwner set to false, we throw an error")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAltAkInvokesAccessKeysInAKMode)
            TEST_METHOD_PROPERTY(L"Description", L"Verify holding alt while pressing a valid access key while in AK mode will invoke the AKO in the scope")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NakedAltExitsAKMode)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that when alt is used to exit AK mode, that it comes with a down and that it is naked.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeScopeOwnerToRootScope)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that an element's scope can be updated to root scope.")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ElementWithoutAutomationPeerPatternGainsFocusWhenInvoked)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that an element that doesn't have a pattern will gain focus when invoked")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ScopeOwnerWithoutAutomationPeerPatternGainsFocusWhenInvoked)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a scope owner that doesn't have a pattern is invoked, it gains focus as well as changes the scope")
        END_TEST_METHOD()

    private:
        wf::IAsyncOperation<xaml_controls::ContentDialogResult>^ OpenContentDialog(xaml_controls::ContentDialog^);
        void PressKeyVerifyNoThrow(Platform::String^ keySequence, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> noThrowEvent);
    };

} } } } }
