// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TestEvent.h>
#include <Versioning.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContentDialog {

class ContentDialogIntegrationTests : public WEX::TestClass<ContentDialogIntegrationTests>
{
public:
    BEGIN_TEST_CLASS(ContentDialogIntegrationTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"c84f8180-05b4-44ec-9575-917de852b45f;88333911-8806-45c4-840a-99c755703018;cc5953d4-6553-42e5-8c02-80720aa9d842")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(CanInstantiate)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ContentDialog.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ContentDialog from the live tree.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanOpenAndCloseProjectedShadow)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully open and close a ContentDialog, projected shadow mode.")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // We're on drop shadows now
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanOpenAndCloseDropShadow)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully open and close a ContentDialog, drop shadow mode.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateUnconstrainedPopupPlacementBehavior)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that when ContentDialog is opened with placement = UnconstrainedPopup, the opened popup will be indeed windowed.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateCanSwitchBetweenWindowedAndNotWindowedPopup)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the same ContentDialog can be opened as Windowed and not Windowed popup.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VisualElementsAreCorrect)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that all of the visual elements in a ContentDialog are populated correctly.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateFocusShift)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the focus shift between ContentDialog Content (different types) and last focused element.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateFocusTrapping)
        TEST_METHOD_PROPERTY(L"Description", L"Focus should not escape from an open ContentDialog when moving focus with GamePad and Keyboard")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateFocusShiftWhenPreviouslyFocusedElementIsRemoved)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the focus shift away from the ContentDialog Content (different types) when last focused element has been removed.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanClickButtons)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully click the buttons within a ContentDialog and that they raise the correct return value.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanDeferButtonClick)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can wait on an action before closing the content dialog as triggered by a button press.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesFullSizeWorkCorrectlyInV2Template)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that toggling the FullSizeDesired property of the ContentDialog results a correct layout with apps that use the ContentDialog V2 template.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateUIETDefault)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of default sized ContentDialog in various visual states.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateUIETFullSize)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of full sized ContentDialog in various visual states.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateTextPanelFitsWithinWindow)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the text panel fits within the window.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateAddingContentDialogToWindowContentDoesNotChangePositioning)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that adding a ContentDialog as a child of the window content does not change how it's positioned on screen.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateRTLContentDialogPosition)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the RTL ContentDialog can be positioned to the same position with the LTR ContentDialog.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DisengagementDoesNotCloseContentDialog)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that disengaging a control with a gamepad in a ContentDialog does not close it.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateUpdatingPropertiesChangesButtonValues)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that changing Primary/SecondaryButtonText, and IsPrimary/SecondaryButtonEnabled properties properly updates the buttons.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesRespondToEscapeKey)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the ContentDialog responds to the Escape key.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesTabBehaviorWork)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully tab between the ContentDialog contents and the buttons.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoButtonsGrowInHeightWithContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the buttons will grow taller with the size of their content.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesNotTopAlignInNonFullScreenWindow)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the ContentDialog does not top-align when the window is not fullscreen.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesNotTopAlignOnXbox)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the ContentDialog does not top-align on Xbox.")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateSIPInteraction)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that when the SIP comes up, the buttons shift up and the content height shrinks.")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")  // InputPane.Showing event doesn't fire on CShell (a known issue).
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"OneCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateSIPInteractionWithMultipleTextBoxes)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that there is no crash caused by grippers showing and hiding in the context of content dialog .")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyCommandAreaIsVisibleWithDefaultButtonsTallContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the buttons will not get pushed off of the dialog with tall content.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyCommandAreaIsVisibleWithTallButtonsTallContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the tall buttons will not get pushed off of the dialog with tall content.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyDialogBoundsConstrainedToWindowBoundsDefaultButtonsTallContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the dialog shows up within the window bounds with tall content.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyDialogBoundsConstrainedToWindowBoundsTallButtonsTallContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the dialog shows up within the window bounds with tall buttons and tall content.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyDialogBoundsConstrainedToWindowBoundsDefaultButtonsShortContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the dialog shows up within the window bounds with short content.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyDialogBoundsConstrainedToWindowBoundsTallButtonsShortContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the dialog shows up within the window bounds with tall buttons and short content.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyLongTitleDoesNotVerticallyClip)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that the height of the dialog title is not constrained by an arbitrary MaxHeight.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateAutomationNameByDelayingSetTitleValue)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the ContentDialog automation name property by delaying the Title property set.")
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateCommandSpaceSizeWhenFullSizeDesiredIsTrue)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the ContentDialog CommandSpace size when FullSizeDesired is true that ensures the command buttons shown correctly.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateClosingPopupInButtonHandlerDoesNotCrash)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that manually closing the ContentDialog's popup in response to a click of one of its buttons does not crash the app.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateParentedContentDialogSize)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the parented ContentDialog size.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesSupportCloseButton)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that a 3rd button can be added and interacted with.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateButtonsLayout)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the layoubt of the buttons through the various combinations of them being enabled/disabled.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanStyleButtons)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the styling scenario for buttons.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanCloseDialogWithGamepadBAndEscape)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the ContentDialog closes on key-down for Escape and on key-up for GamepadB.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesFireButtonCommandsAfterDeferralCompletes)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that button commands are only fired after a deferral completes.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesNotFireButtonCommandsOnCancel)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that button commands do not fire when closing is canceled.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesNotCrashWhenRestoringFocusToParentlessHyperlink)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we do not crash when 'trying' to restore focus to a hyperlink whose parent was destroyed after the dialog opened.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanMoveBetweenButtonsWithKeyboard)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we can move between the buttons using the arrow keys on the keyboard")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanUseTallBackgroundBorderThicknessInNonContentSpace)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the dialog supports tall BackgroundElement::BorderThickness in the non-content space.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanUseTallScrollViewerMarginInNonContentSpace)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the dialog supports tall ContentScrollViewer::Margin in the non-content space.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesEnterInvokeDefaultButtonWithFocusOnNonEnterConsumingContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that ENTER invokes the default button when focus is on a control in the content area that does not consume ENTER.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesEnterNotInvokeDefaultButtonWithFocusOnEnterConsumingContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that ENTER does not invoke the default button when focus is on a control in the content area that does consume ENTER.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesEnterNotInvokeDefaultButtonWithFocusOnAnotherCommandButton)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that ENTER does not invoke the default button when focus is on a control in the content area that does consume ENTER.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateDefaultButtonStyleAppliedCorrectly)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the default button style is applied correct depending on where focus currently resides within the ContentDialog.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesInitiallyFocusFirstFocusableContent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the first focusable content gets focus when the ContentDialog is opened.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesInitiallyFocusDefaultButton)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the default button gets focus when the ContentDialog is opened if there is no focusable content.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesInitiallyFocusFirstFocusableCommandButton)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the first focusable command button gets focus when the ContentDialog is opened if there is no focusable content or default button.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanClickButtonsNotCoveredByDialogWithPlacementInPlace)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that elements not covered by a dialog with ContentDialogPlacement.InPlace can be interacted with.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanCallHideInButtonClickHandler)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that there isn't a crash when calling Hide() from a button click handler.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanCallHideInClosedHandler)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that there isn't a crash when calling Hide() in the Closed event handler.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesRestyledXboxInsiderHubDialogStretchHorizontally)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that a dialog using a template similar to the Xbox Insider app's dialog stretches to the full window width.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanShowPopupAndInPlaceDialogsAtSameTime)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that a popup & an InPlace dialog can be shown at the same time.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanShowTwoNonSiblingInPlaceDialogsAtSameTime)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that two InPlace dialogs that aren't under the same parent can be shown at the same time.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanNotShowTwoPopupDialogsAtSameTime)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that popup dialogs can't be shown at the same time.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanNotShowTwoSiblingInPlaceDialogsAtSameTime)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that two InPlace dialogs that aren't under the same parent can't be shown at the same time.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanReOpenPopupDialogAsInPlaceDialog)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that a dialog opened in the popup can be re-opened as an InPlace dialog.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanReOpenInPlaceDialogAsPopupDialog)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that a dialog opened InPlace can be re-opened as a popup dialog.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanOpenAndCloseWithInvalidTemplate)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that a dialog with an unsupported template can still be opened/closed without crashing.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanShowAfterCancelingInSameTick)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that a dialog can be shown after another one was shown but canceled within the same tick.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateLayoutRootIsNotPutBackInTreeOnClose)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the LayoutRoot part is not put back into the ContentDialog's tree after it closes (for dialogs specified in markup).")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoesNotCrashWhenOpenWithUnsupportedTemplateAndInputPaneOpens)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that there is not crash when a dialog with an unsupported template is open when the input pane opens.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateSettingUICommandSetsButtonProperties)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the *ButtonCommand property to a UICommand causes the button to properly pick up the properties from the object.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateSettingUICommandDoesNotOverwriteButtonText)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the *ButtonCommand property to a UICommand does not cause the ContentDialog to overwrite an already-set value of *ButtonText.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ButtonsRetainFocusWhenWindowSizeChanges)
        TEST_METHOD_PROPERTY(L"Description", L"Ensure that when a ContentDialog button has focus, and the window size changes, focus isn't lost")
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")  // This test changes the window size, isolating it to reduce risk
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanUseContentDialogWithSmokeBackgroundPart)
        TEST_METHOD_PROPERTY(L"Description", L"Uses a ContentDialog style that includes a smoke background Rectangle which reflects theme brushes.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InPlaceContentDialogWithoutSmokeBackgroundPart)
        TEST_METHOD_PROPERTY(L"Description", L"Uses a ContentDialog style that includes a removed smoke background Rectangle in in-place placement.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LoadedAndUnloadedArriveAtTheRightTimes)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the Loaded and Unloaded events arrive in the correct order and at the correct time when a ContentDialog is opened and closed.")
    END_TEST_METHOD()

private:
    enum class ContentDialogContent
    {
        Empty,
        Default,
        TextContent,
        TextPanel,
        TextBoxContent,
        MultipleTextBoxContent,
        Button,
        ButtonAndTextBox,
        TallGrid
    };

    enum class ContentDialogSize
    {
        DefaultSize,
        FullSize
    };

    struct ValidateFootprintExpectedValues
    {
        double ContentDialogWidth;
        double ContentDialogHeight;
        double LayoutRootWidth;
        double LayoutRootHeight;
        double BackgroundElementWidth;
        double BackgroundElementHeight;
        double FullSizeBackgroundElementWidth;
        double FullSizeBackgroundElementHeight;
        double WithTextPanelWidth;
        double WithTextPanelHeight;
    };

    static xaml_controls::ContentDialog^ SetupContentDialogTest(ContentDialogContent content, xaml_controls::ContentDialogPlacement placement = xaml_controls::ContentDialogPlacement::Popup);
    static xaml_controls::ContentDialog^ CreateContentDialog(ContentDialogContent content);
    static wf::IAsyncOperation<xaml_controls::ContentDialogResult>^ OpenContentDialog(
        xaml_controls::ContentDialog^ contentDialog,
        xaml_controls::ContentDialogPlacement placement = xaml_controls::ContentDialogPlacement::Popup
        );
    static void CloseContentDialog(xaml_controls::ContentDialog^ contentDialog);
    static xaml_controls::Panel^ ValidateUIElementTestSetup(ContentDialogSize desiredSize);
    static void ValidateUIETDefaultWorker(wf::Size windowSize, ContentDialogSize contentDialogSize);
    static void DoesNotTopAlignWorker();
    static void ValidateCommandSpaceSizeWorker(bool isFullSizeDesired, wf::Size expectedCommandSpaceSize);
    static xaml_controls::Button^ GetButton(xaml_controls::ContentDialog^, xaml_controls::ContentDialogButton button);

    static void CanOpenAndCloseWorker(xaml_controls::ContentDialogPlacement placement = xaml_controls::ContentDialogPlacement::Popup, bool validateDCompTree = false);
    static void CanClickButtonsWorker(xaml_controls::ContentDialogPlacement placement = xaml_controls::ContentDialogPlacement::Popup);

    static void CanDeferButtonClickHelper(xaml_controls::ContentDialogButton buttonType);

    static void VerifyCommandAreaIsVisibleWorker(ContentDialogContent content, bool areButtonsTall);
    static void VerifyDialogBoundsConstrainedToWindowBoundsWorker(ContentDialogContent content, bool areButtonsTall);

    static xaml_controls::TextBox^ GetTextBoxFromContentDialogContent(xaml_controls::ContentDialog^ contentDialog);

    static void VerifyFocusedElement(Platform::Object^ expectedFocusedElement);

    static void ButtonsRetainFocusWhenWindowSizeChangesWorker();

    static Platform::String^ GetResourcesPath();

    static bool IsInWPFHostingMode();

    void CanOpenAndClose();
    void VerifyContentDialogSmokeBackgroundPart(xaml_controls::ContentDialogPlacement placement);
};

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ContentDialog
