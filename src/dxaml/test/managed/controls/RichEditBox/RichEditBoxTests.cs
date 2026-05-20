// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Text;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class RichEditBoxTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
            TestDeploymentDir = context.TestDeploymentDir;
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        // This test simulate what StickyNotes does, for each TextChanged event it updates CharacterFormat
        public void UpdateCharacterFormatForTextChangedEvent()
        {
            RichEditBox richEditBox = null;
            UIExecutor.Execute(() =>
            {
                richEditBox = new RichEditBox();
                TestServices.WindowHelper.WindowContent = richEditBox;
            });

            TestServices.WindowHelper.WaitForIdle();
            string oldText ="";

            UIExecutor.Execute(() =>
            {
                richEditBox.TextChanged += (s, e) =>
                {
                    Log.Comment("TextChanged event fired...");
                    var myDocument = richEditBox.Document;
                    string currentText;
                    myDocument.GetText(TextGetOptions.None, out currentText);
                    if (currentText != oldText) //avoid dead loop, only update format once for each key stroke
                    {
                        Log.Comment("Update character format...");
                        oldText = currentText;
                        ITextRange currentRange = myDocument.GetRange(0, 0);
                        currentRange.Expand(TextRangeUnit.CharacterFormat);
                        ITextCharacterFormat currentFormat = currentRange.CharacterFormat;
                        currentFormat.Bold = FormatEffect.Toggle;
                        currentRange.CharacterFormat = currentFormat;
                    }
                };
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(richEditBox, FocusState.Keyboard);
            Log.Comment("Input Some text..");
            for (int i = 0; i < 100; i++)
            {
                TestServices.KeyboardHelper.PressKeySequence("a");
                TestServices.WindowHelper.WaitForIdle();
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies RichEditBox.CopyingToClipboard's behavior")]
        public void VerifyRichEditBoxCopyBehavior()
        {
            string initialText = "Lorem";
            var preventCopyHandler = new Action<object, Microsoft.UI.Xaml.Controls.TextControlCopyingToClipboardEventArgs>((source, args) =>
            {
                Log.Comment("Preventing copy by setting handled to true");
                args.Handled = true;
            });


            RichEditBox richEditBox = null;
            UIExecutor.Execute(() =>
            {
                richEditBox = new RichEditBox();
                richEditBox.Document.SetText(Microsoft.UI.Text.TextSetOptions.None, initialText);
                TestServices.WindowHelper.WindowContent = richEditBox;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(richEditBox, FocusState.Keyboard);

            using (var textCopied = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.TextControlCopyingToClipboardEventArgs>(richEditBox, "CopyingToClipboard"))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Ctrl+C--->Copy...");
                TestServices.KeyboardHelper.Copy();
                textCopied.Wait();
            }


            UIExecutor.Execute(() =>
            {
                richEditBox.Document.SetText(Microsoft.UI.Text.TextSetOptions.None, "Ipsum");
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(richEditBox, FocusState.Keyboard);

            using (var textCopyEvent = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.TextControlCopyingToClipboardEventArgs>(richEditBox, "CopyingToClipboard", preventCopyHandler))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Ctrl+C--->Copy...");
                TestServices.KeyboardHelper.Copy();
                textCopyEvent.Wait();
            }

            using (var textPasted = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.TextControlPasteEventArgs>(richEditBox, "Paste"))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Ctrl+V--->Paste...");
                TestServices.KeyboardHelper.Paste();
                textPasted.Wait();

                UIExecutor.Execute(() =>
                {
                    string textValue = string.Empty;
                    richEditBox.Document.GetText(Microsoft.UI.Text.TextGetOptions.AdjustCrlf, out textValue);
                    Verify.AreEqual(initialText, textValue.TrimEnd('\r', '\n')); //RichEdit appends a newline
                });
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies RichEditBox.CuttingToClipboard's behavior")]
        public void VerifyRichEditBoxCutBehavior()
        {
            string initialText = "Lorem";
            var preventCopyHandler = new Action<object, Microsoft.UI.Xaml.Controls.TextControlCuttingToClipboardEventArgs>((source, args) =>
            {
                Log.Comment("Preventing cut by setting handled to true");
                args.Handled = true;
            });


            RichEditBox richEditBox = null;
            UIExecutor.Execute(() =>
            {
                richEditBox = new RichEditBox();
                richEditBox.Document.SetText(Microsoft.UI.Text.TextSetOptions.None, initialText);
                TestServices.WindowHelper.WindowContent = richEditBox;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(richEditBox, FocusState.Keyboard);

            using (var textCopied = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.TextControlCuttingToClipboardEventArgs>(richEditBox, "CuttingToClipboard"))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Ctrl+X--->Cut...");
                TestServices.KeyboardHelper.Cut();
                textCopied.Wait();
            }


            UIExecutor.Execute(() =>
            {
                richEditBox.Document.SetText(Microsoft.UI.Text.TextSetOptions.None, "Ipsum");
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(richEditBox, FocusState.Keyboard);

            using (var textCopyEvent = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.TextControlCuttingToClipboardEventArgs>(richEditBox, "CuttingToClipboard", preventCopyHandler))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Ctrl+X--->Cut...");
                TestServices.KeyboardHelper.Cut();
                textCopyEvent.Wait();
            }

            using (var textPasted = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.TextControlPasteEventArgs>(richEditBox, "Paste"))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Ctrl+V--->Paste...");
                TestServices.KeyboardHelper.Paste();
                textPasted.Wait();

                UIExecutor.Execute(() =>
                {
                    string textValue = string.Empty;
                    richEditBox.Document.GetText(Microsoft.UI.Text.TextGetOptions.AdjustCrlf, out textValue);
                    Verify.AreEqual(initialText, textValue.TrimEnd('\r', '\n')); //RichEdit appends a newline
                });
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void RichEditBoxCharacterCasingTest()
        {
            RichEditBox richEditBox = null;
            string expectedUpper = "A";
            string expectedLower = "Ab";
            string expectedFinal = "AbaB";

            //Event handlers
            var textChangingUpperCaseHandler = new Action<object, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>((source, args) =>
            {
                string textValue = string.Empty;
                richEditBox.Document.GetText(Microsoft.UI.Text.TextGetOptions.AdjustCrlf, out textValue);
                Verify.AreEqual(textValue.TrimEnd('\r', '\n'), expectedUpper);
            });

            var textChangingLowerCaseHandler = new Action<object, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>((source, args) =>
            {
                string textValue = string.Empty;
                richEditBox.Document.GetText(Microsoft.UI.Text.TextGetOptions.AdjustCrlf, out textValue);
                Verify.AreEqual(textValue.TrimEnd('\r', '\n'), expectedLower);
            });

            var textChangingFinalHandler = new Action<object, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>((source, args) =>
            {
                string textValue = string.Empty;
                richEditBox.Document.GetText(Microsoft.UI.Text.TextGetOptions.AdjustCrlf, out textValue);
                Verify.AreEqual(textValue.TrimEnd('\r', '\n'), expectedFinal);
            });

            UIExecutor.Execute(() =>
            {
                richEditBox = new RichEditBox();
                richEditBox.CharacterCasing = Microsoft.UI.Xaml.Controls.CharacterCasing.Upper;
                TestServices.WindowHelper.WindowContent = richEditBox;
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(richEditBox, FocusState.Keyboard);


            using (var textChanged = new EventTester<RichEditBox, RoutedEventArgs>(richEditBox, "TextChanged"))
            using (var textChanging = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>(richEditBox, "TextChanging", textChangingUpperCaseHandler))
            {
                Log.Comment("Input a");
                TestServices.KeyboardHelper.PressKeySequence("a");

                textChanging.Wait();
                textChanged.Wait();
            }

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Log.Comment("Switching casing to Lower");
                richEditBox.CharacterCasing = Microsoft.UI.Xaml.Controls.CharacterCasing.Lower;
            });

            using (var textChanged = new EventTester<RichEditBox, RoutedEventArgs>(richEditBox, "TextChanged"))
            using (var textChanging = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>(richEditBox, "TextChanging", textChangingLowerCaseHandler))
            {
                Log.Comment("Input B");
                TestServices.KeyboardHelper.PressKeySequence("B");
                textChanging.Wait();
                textChanged.Wait();
            }

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Log.Comment("Switching casing to Normal");
                richEditBox.CharacterCasing = Microsoft.UI.Xaml.Controls.CharacterCasing.Normal;
            });


            Log.Comment("Input a");
            TestServices.KeyboardHelper.PressKeySequence("a");
            TestServices.WindowHelper.WaitForIdle();

            using (var textChanged = new EventTester<RichEditBox, RoutedEventArgs>(richEditBox, "TextChanged"))
            using (var textChanging = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>(richEditBox, "TextChanging", textChangingFinalHandler))
            {
                Log.Comment("Input B");
                TestServices.KeyboardHelper.PressKeySequence("B");
                textChanging.Wait();
                textChanged.Wait();
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void VerifyRichEditBoxCharacterCasingPasteBehavior()
        {
            RichEditBox richEditBox = null;

            string testString = "Test String";
            string testStringUpper = "TEST STRING";
            string testStringLower = "test string";

            var textChangingUpperCaseHandler = new Action<object, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>((source, args) =>
            {
                string textValue = string.Empty;
                richEditBox.Document.GetText(Microsoft.UI.Text.TextGetOptions.AdjustCrlf, out textValue);
                Verify.AreEqual(textValue.TrimEnd('\r', '\n'), testStringUpper);
            });

            var textChangingLowerCaseHandler = new Action<object, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>((source, args) =>
            {
                string textValue = string.Empty;
                richEditBox.Document.GetText(Microsoft.UI.Text.TextGetOptions.AdjustCrlf, out textValue);
                Verify.AreEqual(textValue.TrimEnd('\r', '\n'), testStringLower);
            });


            UIExecutor.Execute(() =>
            {
                richEditBox = new RichEditBox();
                richEditBox.CharacterCasing = Microsoft.UI.Xaml.Controls.CharacterCasing.Normal;
                richEditBox.Document.SetText(Microsoft.UI.Text.TextSetOptions.None, testString);
                TestServices.WindowHelper.WindowContent = richEditBox;
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(richEditBox, FocusState.Keyboard);

            using (var textChanged = new EventTester<RichEditBox, RoutedEventArgs>(richEditBox, "TextChanged"))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.Cut();
                textChanged.Wait();

                UIExecutor.Execute(() =>
                {
                    richEditBox.CharacterCasing = Microsoft.UI.Xaml.Controls.CharacterCasing.Upper;
                });
                TestServices.WindowHelper.WaitForIdle();
            }

            using (var textChanged = new EventTester<RichEditBox, RoutedEventArgs>(richEditBox, "TextChanged"))
            using (var textChanging = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>(richEditBox, "TextChanging", textChangingUpperCaseHandler))
            {
                TestServices.KeyboardHelper.Paste();
                textChanging.Wait();
                textChanged.Wait();
            }

            using (var textChanged = new EventTester<RichEditBox, RoutedEventArgs>(richEditBox, "TextChanged"))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.Cut();
                textChanged.Wait();

                UIExecutor.Execute(() =>
                {
                    richEditBox.CharacterCasing = Microsoft.UI.Xaml.Controls.CharacterCasing.Lower;
                });
                TestServices.WindowHelper.WaitForIdle();
            }

            using (var textChanged = new EventTester<RichEditBox, RoutedEventArgs>(richEditBox, "TextChanged"))
            using (var textChanging = new EventTester<RichEditBox, Microsoft.UI.Xaml.Controls.RichEditBoxTextChangingEventArgs>(richEditBox, "TextChanging", textChangingLowerCaseHandler))
            {
                TestServices.KeyboardHelper.Paste();
                textChanging.Wait();
                textChanged.Wait();
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates that text alignment can be set on placeholder text.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Ignore", "True")] // Unreliable test: RichEditBoxTests.VerifyRichEditBoxPlaceholderTextAlignment
        public void VerifyRichEditBoxPlaceholderTextAlignment()
        {
            const string rootPanelXaml =
                    @"<StackPanel Width='500' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                      <Button x:Name='btn'> Button </Button>
                      <RichEditBox x:Name='rb' PlaceholderText='PlaceHolder' TextAlignment='Right'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button btn = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                btn = (Button)rootPanel.FindName("btn");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(btn, FocusState.Pointer);
            TestServices.Utilities.VerifyUIElementTree();
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates that placeholder text does not overlap with richeditbox text.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void VerifyRichEditBoxPlaceholderTextDoesNotShowWhenTextIsNotEmpty()
        {
            const string rootPanelXaml =
                    @"<StackPanel Width='500' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btn'> Button </Button>
                        <RichEditBox x:Name='reb'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button btn = null;
            RichEditBox reb = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                btn = (Button)rootPanel.FindName("btn");
                reb = (RichEditBox)rootPanel.FindName("reb");
                reb.Document.SetText(Microsoft.UI.Text.TextSetOptions.None, "Lorem Ipsum");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(btn, FocusState.Pointer);

            TestServices.Utilities.VerifyUIElementTree("1");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                reb.PlaceholderText = "PLACEHOLDER";
            });
            TestServices.WindowHelper.WaitForIdle();
            TestServices.Utilities.VerifyUIElementTree("2");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                reb.Document.SetText(Microsoft.UI.Text.TextSetOptions.None, "");
            });
            TestServices.WindowHelper.WaitForIdle();
            TestServices.Utilities.VerifyUIElementTree("3");
        }

    }
}

