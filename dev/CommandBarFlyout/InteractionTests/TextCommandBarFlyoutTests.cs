// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class TextCommandBarFlyoutTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        internal class TextCommandBarFlyoutTestSetupHelper : TestSetupHelper
        {
            public TextCommandBarFlyoutTestSetupHelper(string languageOverride = "", bool attemptRestartOnDispose = true)
                : base(new[] { "CommandBarFlyout Tests", "TextCommandBarFlyout Tests" }, languageOverride, attemptRestartOnDispose)
            {
                FindElement.ById<Button>("ClearClipboardContentsButton").InvokeAndWait();
            }
        }

        [TestMethod]
        public void CanCutTextFromTextBox()
        {
            CanCutCopyTextFrom("TextBox", cutText: true);
        }

        [TestMethod]
        public void CanCutTextFromRichEditBox()
        {
            CanCutCopyTextFrom("RichEditBox", cutText: true);
        }

        [TestMethod]
        public void CanCopyTextFromTextBox()
        {
            CanCutCopyTextFrom("TextBox", cutText: false);
        }

        [TestMethod]
        public void CanCopyTextFromTextBlock()
        {
            CanCutCopyTextFrom("TextBlock", cutText: false);
        }

        [TestMethod]
        public void CanCopyTextFromRichEditBox()
        {
            CanCutCopyTextFrom("RichEditBox", cutText: false);
        }

        [TestMethod]
        public void CanCopyTextFromRichTextBlock()
        {
            CanCutCopyTextFrom("RichTextBlock", cutText: false);
        }

        private void CanCutCopyTextFrom(string textControlName, bool cutText)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because TextCommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new TextCommandBarFlyoutTestSetupHelper())
            {
                string initialTextControlContents = FindElement.ById(textControlName).GetText();

                Log.Comment("Selecting all text in the {0}.", textControlName);
                FindElement.ById<Button>(string.Format("{0}SelectAllButton", textControlName)).InvokeAndWait();

                OpenFlyoutOn(textControlName, asTransient: false);

                Log.Comment("{0} selected text.", cutText ? "Cutting" : "Copying");
                FindElement.ByName<Button>(cutText ? "Cut" : "Copy").InvokeAndWait();

                string textControlContents = FindElement.ById(textControlName).GetText();

                Log.Comment("{0} contents are now {1}", textControlName, textControlContents);
                Verify.IsNotNull(textControlContents);

                if (cutText)
                {
                    Verify.AreEqual(string.Empty, textControlContents);
                }
                else
                {
                    Verify.AreEqual(initialTextControlContents, textControlContents);
                }
            }
        }

        [TestMethod]
        public void CanPasteTextToTextBox()
        {
            CanPasteTextTo("TextBox");
        }

        [TestMethod]
        public void CanPasteTextToRichEditBox()
        {
            CanPasteTextTo("RichEditBox");
        }

        [TestMethod]
        public void CanPasteTextToPasswordBox()
        {
            CanPasteTextTo("PasswordBox");
        }

        private void CanPasteTextTo(string textControlName)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because TextCommandBarFlyout is not supported pre-RS2");
                return;
            }

            if (textControlName == "PasswordBox" && PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("Test is disabled pre-RS5 because programmatically pasting into a PasswordBox is not supported pre-RS5");
                return;
            }

            using (var setup = new TextCommandBarFlyoutTestSetupHelper())
            {
                Log.Comment("Loading the clipboard with text.", textControlName);
                FindElement.ById<Button>("SetClipboardContentsButton").InvokeAndWait();

                if (textControlName != "PasswordBox")
                {
                    Log.Comment("Selecting all text in the {0}.", textControlName);
                    FindElement.ById<Button>(string.Format("{0}SelectAllButton", textControlName)).InvokeAndWait();
                }
                else
                {
                    Log.Comment("Deleting all text in the {0}.", textControlName);

                    // PasswordBox does not raise the ValueChanged UIA event, so we can't use SetValueAndWait in that case.
                    FindElement.ById<Edit>(textControlName).SetValue(string.Empty);
                    Wait.ForIdle();
                }

                OpenFlyoutOn(textControlName, asTransient: false);

                Log.Comment("Pasting selected text.");
                FindElement.ByName<Button>("Paste").InvokeAndWait();

                string textControlContents = FindElement.ById(textControlName).GetText();
                string expectedString = "Automatically set clipboard text";

                if (textControlName == "PasswordBox")
                {
                    string obfuscatedString = string.Empty;

                    for (int i = 0; i < expectedString.Length; i++)
                    {
                        obfuscatedString += "\u25CF";
                    }

                    expectedString = obfuscatedString;
                }

                Log.Comment("{0} contents are now {1}", textControlName, textControlContents);
                Verify.AreEqual(expectedString, textControlContents);
            }
        }

        [TestMethod]
        public void CanBoldTextInARichEditBox()
        {
            CanStyleTextInARichEditBox("Bold", "\\b ", "\\b0 ");
        }

        [TestMethod]
        public void CanItalicizeTextInARichEditBox()
        {
            CanStyleTextInARichEditBox("Italic", "\\i ", "\\i0 ");
        }

        [TestMethod]
        public void CanUnderlineTextInARichEditBox()
        {
            CanStyleTextInARichEditBox("Underline", "\\ul ", "\\ulnone ");
        }

        private void CanStyleTextInARichEditBox(string styleName, string styleStart, string styleEnd)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because TextCommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new TextCommandBarFlyoutTestSetupHelper())
            {
                Log.Comment("Getting initial RichEditBox RTF content.");
                FindElement.ById<Button>(string.Format("GetRichEditBoxRtfContentButton")).InvokeAndWait();
                string textControlContents = FindElement.ById("StatusReportingTextBox").GetText();
                Log.Comment("Content is '{0}'", textControlContents.Replace(Environment.NewLine, " "));

                Log.Comment("Ensuring RichEditBox initially has no {0} content.", styleName.ToLower());
                Verify.IsFalse(textControlContents.Contains(styleStart));
                Verify.IsFalse(textControlContents.Contains(styleEnd));

                var richedit = FindElement.ById<Edit>("RichEditBox");

                FocusAndSelectText(richedit, "ergo");
                OpenFlyoutOn("RichEditBox", asTransient: true);

                var toggleFormatButton = FindElement.ByName<ToggleButton>(styleName);
                Verify.AreEqual(ToggleState.Off, toggleFormatButton.ToggleState);

                Log.Comment("Making text {0}.", styleName.ToLower());
                toggleFormatButton.ToggleAndWait();
                VerifyRichEditBoxHasContent(string.Format("Lorem ipsum {0}ergo{1} sum", styleStart, styleEnd));

                DismissFlyout();

                Log.Comment("Select mixed format text");
                // Note, in this case the selection starts in unstyled text ("sum") and includes styled text ("ergo"). The next case covers
                // starting in styled text and including unstyled text
                FocusAndSelectText(richedit, "sum ergo");

                Log.Comment("Showing flyout should not change bold status");
                OpenFlyoutOn("RichEditBox", asTransient: true);
                Verify.AreEqual(ToggleState.Off, toggleFormatButton.ToggleState);
                VerifyRichEditBoxHasContent(string.Format("Lorem ipsum {0}ergo{1} sum", styleStart, styleEnd));

                Log.Comment("Apply formatting to new selection");
                toggleFormatButton.ToggleAndWait();
                VerifyRichEditBoxHasContent(string.Format("Lorem ip{0}sum ergo{1} sum", styleStart, styleEnd));

                DismissFlyout();

                Log.Comment("Select mixed format text");
                FocusAndSelectText(richedit, "ergo su");
                OpenFlyoutOn("RichEditBox", asTransient: true);
                Verify.AreEqual(ToggleState.Off, toggleFormatButton.ToggleState);
                VerifyRichEditBoxHasContent(string.Format("Lorem ip{0}sum ergo{1} sum", styleStart, styleEnd));

                toggleFormatButton.ToggleAndWait();
                VerifyRichEditBoxHasContent(string.Format("Lorem ip{0}sum ergo su{1}m", styleStart, styleEnd));

                DismissFlyout();
                FocusAndSelectText(richedit, "ergo");
                OpenFlyoutOn("RichEditBox", asTransient: true);
                Verify.AreEqual(ToggleState.On, toggleFormatButton.ToggleState);
                VerifyRichEditBoxHasContent(string.Format("Lorem ip{0}sum ergo su{1}m", styleStart, styleEnd));

                toggleFormatButton.ToggleAndWait();
                VerifyRichEditBoxHasContent(string.Format("Lorem ip{0}sum {1}ergo{0} su{1}m", styleStart, styleEnd));
            }
        }

        private void FocusAndSelectText(Edit editControl, string textToSelect)
        {
            Log.Comment("Selecting text '{0}' in editControl '{1}'", textToSelect, editControl.Name);

            FocusHelper.SetFocus(editControl);
            Wait.ForIdle();
            editControl.DocumentRange.FindText(textToSelect, false /*backwards*/, false /*ignorecase*/).Select();
            Wait.ForIdle();
        }

        private static void VerifyRichEditBoxHasContent(string expectedContent)
        {
            Log.Comment("Ensuring RichEditBox contains string '{0}'.", expectedContent);

            Log.Comment("Getting RichEditBox RTF content.");
            FindElement.ById<Button>(string.Format("GetRichEditBoxRtfContentButton")).InvokeAndWait();
            var textControlContents = FindElement.ById("StatusReportingTextBox").GetText();
            Log.Comment("Content is '{0}'", textControlContents.Replace(Environment.NewLine, " "));

            Verify.IsTrue(textControlContents.Contains(expectedContent));
        }

        private static void DismissFlyout()
        {
            var statusReportingTextBox = FindElement.ById("StatusReportingTextBox");
            InputHelper.LeftClick(statusReportingTextBox);
            Wait.ForIdle();
        }

        [TestMethod]
        public void CanUndoAndRedoInTextBox()
        {
            CanUndoAndRedoIn("TextBox");
        }

        [TestMethod]
        public void CanUndoAndRedoInRichEditBox()
        {
            CanUndoAndRedoIn("RichEditBox");
        }

        private void CanUndoAndRedoIn(string textControlName)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("Test is disabled pre-RS5 because programmatically undoing and redoing in text controls is not supported pre-RS5");
                return;
            }

            using (var setup = new TextCommandBarFlyoutTestSetupHelper())
            {
                UIObject textControl = FindElement.ById(textControlName);
                ValueImplementation textControlAsValue = new ValueImplementation(textControl);

                string initialString = textControl.GetText();
                Log.Comment("Initial contents of {0} are '{1}'", textControlName, initialString);

                Log.Comment("Selecting all text in the {0}.", textControlName);
                FindElement.ById<Button>(string.Format("{0}SelectAllButton", textControlName)).InvokeAndWait();

                Log.Comment("Giving the {0} focus.", textControlName);
                FocusHelper.SetFocus(textControl);

                Log.Comment("Typing 'hello' into the {0}.", textControlName);
                TextInput.SendText("hello");
                Wait.ForIdle();

                string finalString = textControl.GetText();
                Log.Comment("Contents of {0} are now '{1}'", textControlName, finalString);

                Log.Comment("Opening TextCommandBarFlyout.");
                OpenFlyoutOn(textControlName, asTransient: false);

                using (ValueChangedEventWaiter waiter = textControlAsValue.IsAvailable ? new ValueChangedEventWaiter(new Edit(textControl), initialString) : null)
                {
                    Log.Comment("Undoing text entry.");
                    FindElement.ByName<Button>("Undo").InvokeAndWait();
                }

                Log.Comment("Contents of {0} are now '{1}'", textControlName, textControl.GetText());
                Verify.AreEqual(initialString, textControl.GetText());
                
                Log.Comment("Reopening TextCommandBarFlyout.");
                OpenFlyoutOn(textControlName, asTransient: false);

                using (ValueChangedEventWaiter waiter = textControlAsValue.IsAvailable ? new ValueChangedEventWaiter(new Edit(textControl), finalString) : null)
                {
                    Log.Comment("Redoing text entry.");
                    FindElement.ByName<Button>("Redo").InvokeAndWait();
                }

                Log.Comment("Contents of {0} are now '{1}'", textControlName, textControl.GetText());
                Verify.AreEqual(finalString, textControl.GetText());
            }
        }

        [TestMethod]
        public void CanSelectAllInTextBox()
        {
            CanSelectAllIn("TextBox");
        }

        [TestMethod]
        public void CanSelectAllInTextBlock()
        {
            CanSelectAllIn("TextBlock");
        }

        [TestMethod]
        public void CanSelectAllInRichEditBox()
        {
            CanSelectAllIn("RichEditBox");
        }

        [TestMethod]
        public void CanSelectAllInRichTextBlock()
        {
            CanSelectAllIn("RichTextBlock");
        }

        [TestMethod]
        public void CanSelectAllInPasswordBox()
        {
            CanSelectAllIn("PasswordBox");
        }

        private void CanSelectAllIn(string textControlName)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because TextCommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new TextCommandBarFlyoutTestSetupHelper())
            {
                OpenFlyoutOn(textControlName, asTransient: false);

                Log.Comment("Selecting all text in the {0}.", textControlName);
                FindElement.ByName<Button>("Select All").InvokeAndWait();

                string textControlSelectionContents = FindElement.ById(textControlName).GetTextSelection();

                Log.Comment("{0} selection contents are now {1}", textControlName, textControlSelectionContents);

                string expectedString = "Lorem ipsum ergo sum";

                if (textControlName == "PasswordBox")
                {
                    string obfuscatedString = string.Empty;

                    for (int i = 0; i < expectedString.Length; i++)
                    {
                        obfuscatedString += "\u25CF";
                    }

                    expectedString = obfuscatedString;
                }

                Verify.AreEqual(expectedString, textControlSelectionContents);
            }
        }

        [TestMethod]
        public void ValidateKeyboarding()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("Test is disabled pre-RS5 because the keyboarding fixes needed to support this were not available pre-RS5.");
                return;
            }

            using (var setup = new TextCommandBarFlyoutTestSetupHelper())
            {
                Log.Comment("Give focus to the TextBox.");
                var textBox = FindElement.ById("TextBox");
                FocusHelper.SetFocus(textBox);

                using (var waiter = new FocusAcquiredWaiter(UICondition.CreateFromName("Select All")))
                {
                    Log.Comment("Use Shift+F10 to bring up the context menu. The Select All button should get focus.");
                    KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);
                    waiter.Wait();
                }
                
                using (var waiter = new FocusAcquiredWaiter(UICondition.CreateFromId("TextBox")))
                {
                    Log.Comment("Use Escape to close the context menu. The TextBox should now have focus.");
                    KeyboardHelper.PressKey(Key.Escape);

                    // On 19H1, there's a bug with the focus restoration code, so we'll manually restore focus to allow the rest of the test to run.
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1))
                    {
                        textBox.SetFocus();
                    }

                    waiter.Wait();
                }
                
                Log.Comment("Give focus to the RichEditBox.");
                var richEditBox = FindElement.ById("RichEditBox");
                FocusHelper.SetFocus(richEditBox);

                using (var waiter = new FocusAcquiredWaiter(UICondition.CreateFromName("Bold")))
                {
                    Log.Comment("Use Shift+F10 to bring up the context menu. The Bold button should get focus.");
                    KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);
                    waiter.Wait();
                }

                Log.Comment("Press the spacebar to invoke the bold button. Focus should stay in the flyout.");
                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();

                using (var waiter = new FocusAcquiredWaiter(UICondition.CreateFromName("More app bar")))
                {
                    Log.Comment("Press the right arrow key three times.  The '...' button should get focus.");
                    KeyboardHelper.PressKey(Key.Right, ModifierKey.None, numPresses: 3);
                    waiter.Wait();
                }

                using (var waiter = new FocusAcquiredWaiter(UICondition.CreateFromName("Select All")))
                {
                    Log.Comment("Press the down arrow key twice.  The Select All button should get focus.");
                    KeyboardHelper.PressKey(Key.Down, ModifierKey.None, numPresses: 2);
                    waiter.Wait();
                }
                
                using (var waiter = new FocusAcquiredWaiter(UICondition.CreateFromId("RichEditBox")))
                {
                    Log.Comment("Use Escape to close the context menu. The RichEditBox should now have focus.");
                    KeyboardHelper.PressKey(Key.Escape);

                    // On 19H1, there's a bug with the focus restoration code, so we'll manually restore focus to allow the rest of the test to run.
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1))
                    {
                        richEditBox.SetFocus();
                    }

                    waiter.Wait();
                }
            }
        }

        [TestMethod]
        public void ValidateProofingMenu()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("Test is disabled pre-RS5 because the ability to add the proofing menu did not exist pre-RS5");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "CommandBarFlyout Tests", "Extra CommandBarFlyout Tests" }))
            {
                Edit textBox = FindElement.ById<Edit>("TextBox");

                Log.Comment("Type \"asdf\" plus a space to create a misspelled word.");
                KeyboardHelper.EnterText(textBox, "asdf ", useKeyboard: true);

                // We know that the word appears at the start of the text box's content,
                // so we'll use a point 10 pixels from the text box's left edge as a point
                // known to be within the word's bounding box.
                Log.Comment("Right-click on the word's location in the text box to get the proofing menu.");
                InputHelper.RightClick(textBox, 10, textBox.BoundingRectangle.Height / 2);

                Log.Comment("Find \"ads\" in the proofing menu to fix the spelling error.");
                var proofingItem = new MenuItem(FindElement.ByNameAndClassName("ads", "MenuFlyoutItem"));
                Log.Comment($"Invoke {proofingItem}");
                proofingItem.InvokeAndWait();

                Verify.AreEqual("ads ", textBox.Value);
            }
        }

        [TestMethod]
        public void ValidateRightClickOnEmptyTextBoxDoesNotShowFlyout()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("Test is disabled pre-RS5 because the fix needed for this functionality didn't exist until then");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "CommandBarFlyout Tests", "Extra CommandBarFlyout Tests" }))
            {
                Log.Comment("Clear the clipboard.");
                FindElement.ById<Button>("ClearClipboardContentsButton").InvokeAndWait();

                Log.Comment("Right-click on the text box.");
                InputHelper.RightClick(FindElement.ById("TextBox"));

                Log.Comment("Count the number of open popups.");
                FindElement.ById<Button>("CountPopupsButton").InvokeAndWait();

                Verify.AreEqual("0", FindElement.ById<Edit>("PopupCountTextBox").Value);
            }
        }

        [TestMethod]
        public void ValidateRichTextBlockOverflowUsesSourceFlyouts()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("Test is disabled pre-RS5 because the fix needed for this functionality didn't exist until then");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "CommandBarFlyout Tests", "Extra CommandBarFlyout Tests" }))
            {
                Log.Comment("Right-click on the rich text block.");
                InputHelper.RightClick(FindElement.ById("RichTextBlock"), 10, 10);

                Log.Comment("Select all the text.");
                FindElement.ByName<Button>("Select All").InvokeAndWait();

                Log.Comment("Now right-click on the rich text block overflow.");
                InputHelper.RightClick(FindElement.ById("RichTextBlockOverflow"), 10, 10);

                Log.Comment("The copy option should be available now, because the rich text block's overflow element delegates to the rich text block.");
                FindElement.ByName<Button>("Copy").InvokeAndWait();
            }
        }

        [TestMethod]
        public void ValidateUnhandledKeysOnNonTransientFlyoutDoNotCloseFlyout()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.NineteenH1))
            {
                Log.Warning("Test is disabled pre-19H1 because the bug fix needed to support this were not available pre-19H1.");
                return;
            }

            using (var setup = new TextCommandBarFlyoutTestSetupHelper())
            {
                Log.Comment("Give focus to the RichEditBox.");
                FocusHelper.SetFocus(FindElement.ById("RichEditBox"));

                using (var waiter = new FocusAcquiredWaiter(UICondition.CreateFromName("Bold")))
                {
                    Log.Comment("Double-tap to select a word and bring up the context menu. The Bold button should get focus.");
                    KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);
                    waiter.Wait();
                }

                Log.Comment("Press the down arrow key. Focus should stay in the flyout.");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                Log.Comment("Press the up arrow key. Focus should stay in the flyout.");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                
                using (var waiter = new FocusAcquiredWaiter(UICondition.CreateFromId("RichEditBox")))
                {
                    Log.Comment("Use Escape to close the context menu. The RichEditBox should now have focus.");
                    KeyboardHelper.PressKey(Key.Escape);
                    waiter.Wait();
                }
            }
        }

        [TestMethod]
        public void VerifyTextCommandBarRemainsOpenWithItems()
        {
            using (var setup = new TestSetupHelper(new[] { "CommandBarFlyout Tests", "Extra CommandBarFlyout Tests" }))
            {
                Log.Comment("Clear the clipboard.");
                FindElement.ById<Button>("ClearClipboardContentsButton").InvokeAndWait();

                Log.Comment("Right-click on the text box with additional items.");
                FindElement.ByName("TextBoxWithAdditionalItems").Click(PointerButtons.Secondary, 20, 20);

                Wait.ForIdle();

                Log.Comment("Count the number of open popups.");
                FindElement.ById<Button>("CountPopupsButton").InvokeAndWait();

                Verify.AreEqual("1", FindElement.ById<Edit>("CustomButtonsOpenCount").Value);
            }
        }

        private void OpenFlyoutOn(string textControlName, bool asTransient)
        {
            Log.Comment("Opening text control flyout on the {0} in {1} mode.", textControlName, asTransient ? "transient" : "standard");
            FindElement.ById<Button>(string.Format("Show{1}TextControlFlyoutOn{0}Button", textControlName, asTransient ? string.Empty : "Standard")).InvokeAndWait();
        }
    }
}