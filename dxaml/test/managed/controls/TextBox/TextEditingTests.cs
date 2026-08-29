// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Windows.Foundation;

using Microsoft.UI;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Shapes;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Text;
using System.Threading;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class TextEditingTests : XamlTestsBase
    {
        private static readonly string testARvsTW = "1#$d$_enter#$u$_enter#2#$d$_enter#$u$_enter#Last line causes horizontal scrolling";
        private static readonly string testRebARvsTW = "Line one\r\nSecond line\r\nLast line causes horizontal scrolling";
        static string TestDeploymentDir { get; set; }
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
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
        [TestProperty("Description", "Verifies the properties and methods of a large set of text controls.")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void TextControlPropertyAndMethodTests()
        {
            using (RuntimeFeature.Enable(15 /*DisableTextBoxCaret*/))
            {
                Canvas canvas = null;
                TextBox txbxNormal = null;
                RichEditBox rebxNormal = null;
                PasswordBox pwbxNormal = null;
                Button TestButton = null;

                UIExecutor.Execute(() =>
                {
                    String text = File.ReadAllText(TestDeploymentDir + @"resources\managed\controls\TextBox\TextEditingTests.xaml");
                    canvas = (Canvas)XamlReader.Load(text);
                    Verify.IsNotNull(canvas);
                    txbxNormal = (TextBox)canvas.FindName("txbxNormal");
                    rebxNormal = (RichEditBox)canvas.FindName("rebxNormal");
                    pwbxNormal = (PasswordBox)canvas.FindName("pwbxNormal");
                    TestButton = CreateTestButton(20, 410, 100, 30);
                    ((IList<UIElement>)canvas.Children).Add(TestButton);
                    TestServices.WindowHelper.WindowContent = canvas;
                });
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(TestButton, FocusState.Keyboard);
                Log.Comment("Test xaml loaded");
                DoTextBoxEventTests(txbxNormal);
                DoTextBoxPropertyTests(txbxNormal);
                DoTextBoxMethodTests(txbxNormal);

                DoRichEditBoxEventTests(rebxNormal);
                DoRichEditBoxPropertyTests(rebxNormal);

                DoPasswordBoxPmeTests(pwbxNormal);

                TestServices.KeyboardHelper.Escape();
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies text control input scenarios.")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TextControlInputTests()
        {
            using (RuntimeFeature.Enable(15 /*DisableTextBoxCaret*/))
            {
                Canvas canvas = null;
                TextBox txbxNormal = null;
                TextBox txbxHScroll = null;
                TextBox txbxVScroll = null;
                RichEditBox rebxNormal = null;
                RichEditBox rebxVScroll = null;
                PasswordBox pwbxNormal = null;
                Button TestButton = null;

                UIExecutor.Execute(() =>
                {
                    String text = File.ReadAllText(TestDeploymentDir + @"resources\managed\controls\TextBox\TextEditingTests.xaml");
                    canvas = (Canvas)XamlReader.Load(text);
                    Verify.IsNotNull(canvas);
                    txbxNormal = (TextBox)canvas.FindName("txbxNormal");
                    txbxHScroll = (TextBox)canvas.FindName("txbxHScroll");
                    txbxVScroll = (TextBox)canvas.FindName("txbxVScroll");
                    rebxNormal = (RichEditBox)canvas.FindName("rebxNormal");
                    rebxVScroll = (RichEditBox)canvas.FindName("rebxVScroll");
                    pwbxNormal = (PasswordBox)canvas.FindName("pwbxNormal");
                    TestButton = CreateTestButton(20, 410, 100, 30);
                    ((IList<UIElement>)canvas.Children).Add(TestButton);
                    TestServices.WindowHelper.WindowContent = canvas;
                });
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(TestButton, FocusState.Keyboard);
                Log.Comment("Test xaml loaded");

                DoTextBoxInputTests(
                    txbxNormal,
                    txbxHScroll,
                    txbxVScroll);

                DoRichEditBoxInputTests(rebxNormal, rebxVScroll);
                DoPasswordBoxInputTests(pwbxNormal, txbxNormal);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that text controls can accept return in various cases.")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void DoTextControlAcceptReturnTests()
        {
            using (RuntimeFeature.Enable(15 /*DisableTextBoxCaret*/))
            {
                Canvas canvas = null;
                Button TestButton = null;
                TextBox txbxARnoTWno = null;
                TextBox txbxARyesTWno = null;
                TextBox txbxARnoTWyes = null;
                TextBox txbxARyesTWyes = null;
                TextBox txbxCenter = null;
                TextBox txbxRight = null;
                Rectangle TestRectangle = null;
                RichEditBox rebxCenter = null;
                RichEditBox rebxRight = null;
                SolidColorBrush orangeBrush = null;
                PasswordBox pwbxNormal = null;
                RichEditBox rebxARyesTWyes = null;

                UIExecutor.Execute(() =>
                {
                    String text = File.ReadAllText(TestDeploymentDir + @"resources\managed\controls\TextBox\TextEditingTests.xaml");
                    canvas = (Canvas)XamlReader.Load(text);
                    Verify.IsNotNull(canvas);
                    txbxARnoTWno = (TextBox)canvas.FindName("txbxARnoTWno");
                    txbxARyesTWno = (TextBox)canvas.FindName("txbxARyesTWno");
                    txbxARnoTWyes = (TextBox)canvas.FindName("txbxARnoTWyes");
                    txbxARyesTWyes = (TextBox)canvas.FindName("txbxARyesTWyes");
                    txbxCenter = (TextBox)canvas.FindName("txbxCenter");
                    txbxRight = (TextBox)canvas.FindName("txbxRight");
                    rebxCenter = (RichEditBox)canvas.FindName("rebxCenter");
                    rebxRight = (RichEditBox)canvas.FindName("rebxRight");
                    rebxARyesTWyes = (RichEditBox)canvas.FindName("rebxARyesTWyes");

                    pwbxNormal = (PasswordBox)canvas.FindName("pwbxNormal");
                    orangeBrush = (SolidColorBrush)canvas.FindName("selectionHighlightBrush");

                    TestButton = CreateTestButton(20, 410, 100, 30);
                    TestRectangle = CreateTestRectangle(130, 410, 100, 30);
                    ((IList<UIElement>)canvas.Children).Add(TestButton);
                    ((IList<UIElement>)canvas.Children).Add(TestRectangle);

                    TestServices.WindowHelper.WindowContent = canvas;
                });
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(TestButton, FocusState.Keyboard);
                Log.Comment("Test xaml loaded");

                Log.Comment("DoTextBoxAccReturnTextWrapTests");
                DoTextBoxAccReturnTextWrapTests(
                    txbxARnoTWno,
                    txbxARyesTWno,
                    txbxARnoTWyes,
                    txbxARyesTWyes);
                Log.Comment("FillInAlignmentTestData");
                FillInAlignmentTestData(
                    txbxCenter,
                    txbxRight,
                    rebxCenter,
                    rebxRight);
                Log.Comment("DoRedlineTests");
                DoRedlineTests(TestButton, rebxARyesTWyes, orangeBrush);
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that richeditbox can accept return and wrap in various cases.")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void DoRichEditBoxControlAcceptReturnTests()
        {
            using (RuntimeFeature.Enable(15 /*DisableTextBoxCaret*/))
            {
                Canvas canvas = null;
                Button TestButton = null;
                
                TextBox txbxCenter = null;
                TextBox txbxRight = null;
                Rectangle TestRectangle = null;

                RichEditBox rebxARnoTWno = null;
                RichEditBox rebxARyesTWno = null;
                RichEditBox rebxARnoTWyes = null;
                RichEditBox rebxARyesTWyes = null;
                RichEditBox rebxCenter = null;
                RichEditBox rebxRight = null;
                SolidColorBrush orangeBrush = null;
                PasswordBox pwbxNormal = null;

                UIExecutor.Execute(() =>
                {
                    String text = File.ReadAllText(TestDeploymentDir + @"resources\managed\controls\TextBox\TextEditingTests.xaml");
                    canvas = (Canvas)XamlReader.Load(text);
                    Verify.IsNotNull(canvas);
                    txbxCenter = (TextBox)canvas.FindName("txbxCenter");
                    txbxRight = (TextBox)canvas.FindName("txbxRight");

                    rebxARnoTWno = (RichEditBox)canvas.FindName("rebxARnoTWno");
                    rebxARyesTWno = (RichEditBox)canvas.FindName("rebxARyesTWno");
                    rebxARnoTWyes = (RichEditBox)canvas.FindName("rebxARnoTWyes");
                    rebxARyesTWyes = (RichEditBox)canvas.FindName("rebxARyesTWyes");
                    rebxCenter = (RichEditBox)canvas.FindName("rebxCenter");
                    rebxRight = (RichEditBox)canvas.FindName("rebxRight");

                    pwbxNormal = (PasswordBox)canvas.FindName("pwbxNormal");
                    orangeBrush = (SolidColorBrush)canvas.FindName("selectionHighlightBrush");

                    TestButton = CreateTestButton(20, 410, 100, 30);
                    TestRectangle = CreateTestRectangle(130, 410, 100, 30);
                    ((IList<UIElement>)canvas.Children).Add(TestButton);
                    ((IList<UIElement>)canvas.Children).Add(TestRectangle);

                    TestServices.WindowHelper.WindowContent = canvas;
                });
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(TestButton, FocusState.Keyboard);
                Log.Comment("Test xaml loaded");

                Log.Comment("DoRichEditBoxAccReturnTextWrapTests");
                DoRichEditBoxAccReturnTextWrapTests(
                    rebxARnoTWno,
                    rebxARyesTWno,
                    rebxARnoTWyes,
                    rebxARyesTWyes);
                Log.Comment("FillInAlignmentTestData");
                FillInAlignmentTestData(
                    txbxCenter,
                    txbxRight,
                    rebxCenter,
                    rebxRight);
                Log.Comment("DoRedlineTests");
                DoRedlineTests(TestButton, rebxARyesTWyes, orangeBrush);
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies programmatic content set in RichEditBoxes.")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ProgrammaticRichEditBoxContentTests()
        {
            using (RuntimeFeature.Enable(15 /*DisableTextBoxCaret*/))
            {
                Canvas canvas = null;
                Button TestButton = null;
                TextBox txbxNormal = null;
                Rectangle TestRectangle = null;
                RichEditBox rebxNormal = null;
                RichEditBox rebxHScroll = null;
                RichEditBox rebxVScroll = null;
                RichEditBox rebxARnoTWno = null;
                RichEditBox rebxARyesTWno = null;
                RichEditBox rebxARnoTWyes = null;
                RichEditBox rebxARyesTWyes = null;
                RichEditBox rebxCenter = null;
                RichEditBox rebxRight = null;
                SolidColorBrush orangeBrush = null;
                PasswordBox pwbxNormal = null;

                UIExecutor.Execute(() =>
                {
                    String text = File.ReadAllText(TestDeploymentDir + @"resources\managed\controls\TextBox\TextEditingTests.xaml");
                    canvas = (Canvas)XamlReader.Load(text);
                    Verify.IsNotNull(canvas);

                    rebxNormal = (RichEditBox)canvas.FindName("rebxNormal");
                    rebxHScroll = (RichEditBox)canvas.FindName("rebxHScroll");
                    rebxVScroll = (RichEditBox)canvas.FindName("rebxVScroll");
                    rebxARnoTWno = (RichEditBox)canvas.FindName("rebxARnoTWno");
                    rebxARyesTWno = (RichEditBox)canvas.FindName("rebxARyesTWno");
                    rebxARnoTWyes = (RichEditBox)canvas.FindName("rebxARnoTWyes");
                    rebxARyesTWyes = (RichEditBox)canvas.FindName("rebxARyesTWyes");
                    rebxCenter = (RichEditBox)canvas.FindName("rebxCenter");
                    rebxRight = (RichEditBox)canvas.FindName("rebxRight");
                    txbxNormal = (TextBox)canvas.FindName("txbxNormal");
                    pwbxNormal = (PasswordBox)canvas.FindName("pwbxNormal");
                    orangeBrush = (SolidColorBrush)canvas.FindName("selectionHighlightBrush");

                    TestButton = CreateTestButton(20, 410, 100, 30);
                    TestRectangle = CreateTestRectangle(130, 410, 100, 30);
                    ((IList<UIElement>)canvas.Children).Add(TestButton);
                    ((IList<UIElement>)canvas.Children).Add(TestRectangle);

                    TestServices.WindowHelper.WindowContent = canvas;
                });
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(TestButton, FocusState.Keyboard);
                Log.Comment("Test xaml loaded");
                Log.Comment("VerifyDeleteAndRevealButtons");
                VerifyDeleteAndRevealButtons(txbxNormal, pwbxNormal);
                Log.Comment("DoProgrammaticRichEditBoxTests");
                DoProgrammaticRichEditBoxTests(
                    rebxNormal,
                    rebxHScroll,
                    rebxVScroll,
                    rebxARnoTWno,
                    rebxARyesTWno,
                    rebxARnoTWyes,
                    rebxARyesTWyes,
                    pwbxNormal,
                    orangeBrush,
                    TestButton);

                TestServices.KeyboardHelper.Escape();
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        #region Helpers
        private void DoTextBoxPropertyTests(TextBox txbxNormal)
        {
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("test events", txbxNormal.Text);
                txbxNormal.Text = "new text";
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("new text", txbxNormal.Text);
                Verify.AreEqual(String.Empty, txbxNormal.SelectedText);
                Verify.AreEqual(0, txbxNormal.SelectionLength);
                Verify.AreEqual(0, txbxNormal.SelectionStart);
                txbxNormal.Select(7, 1);
                txbxNormal.SelectedText = " new ";
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(" new ", txbxNormal.SelectedText);
                Verify.AreEqual(5, txbxNormal.SelectionLength);
                Verify.AreEqual(7, txbxNormal.SelectionStart);
                txbxNormal.SelectionStart = 2;
                txbxNormal.SelectionLength = 2;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(2, txbxNormal.SelectionLength);
                Verify.AreEqual(2, txbxNormal.SelectionStart);
                Verify.AreEqual(0, txbxNormal.MaxLength);
                txbxNormal.MaxLength = 42;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(42, txbxNormal.MaxLength);
                Verify.IsFalse(txbxNormal.IsReadOnly);
                txbxNormal.IsReadOnly = true;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(txbxNormal.IsReadOnly);
                txbxNormal.IsReadOnly = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(txbxNormal.AcceptsReturn);
                txbxNormal.AcceptsReturn = true;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(txbxNormal.AcceptsReturn);
                txbxNormal.AcceptsReturn = false;
                Verify.AreEqual(TextWrapping.NoWrap, txbxNormal.TextWrapping);
                txbxNormal.TextWrapping = TextWrapping.Wrap;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextWrapping.Wrap, txbxNormal.TextWrapping);
                txbxNormal.TextWrapping = TextWrapping.NoWrap;
                Verify.AreEqual(TextAlignment.DetectFromContent, txbxNormal.TextAlignment);
                txbxNormal.TextAlignment = TextAlignment.Center;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextAlignment.Center, txbxNormal.TextAlignment);
                txbxNormal.TextAlignment = TextAlignment.Right;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextAlignment.Right, txbxNormal.TextAlignment);
                txbxNormal.TextAlignment = TextAlignment.Justify;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextAlignment.Justify, txbxNormal.TextAlignment);
                txbxNormal.TextAlignment = TextAlignment.Left;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextAlignment.Left, txbxNormal.TextAlignment);
                Verify.IsTrue(txbxNormal.IsSpellCheckEnabled);
                txbxNormal.IsSpellCheckEnabled = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(txbxNormal.IsSpellCheckEnabled);
                txbxNormal.IsSpellCheckEnabled = true;
                Verify.IsTrue(txbxNormal.IsTextPredictionEnabled);
                txbxNormal.IsTextPredictionEnabled = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(txbxNormal.IsTextPredictionEnabled);
                txbxNormal.IsTextPredictionEnabled = true;
                InputScope scope = new InputScope();
                InputScopeName scopeName = new InputScopeName(InputScopeNameValue.Url);
                scope.Names.Add(scopeName);
                txbxNormal.InputScope = scope;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(txbxNormal.InputScope);
                Verify.AreEqual(1, txbxNormal.InputScope.Names.Count);
                Verify.AreEqual(InputScopeNameValue.Url, txbxNormal.InputScope.Names[0].NameValue);
                Verify.IsFalse(txbxNormal.PreventKeyboardDisplayOnProgrammaticFocus);
                txbxNormal.PreventKeyboardDisplayOnProgrammaticFocus = true;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(txbxNormal.PreventKeyboardDisplayOnProgrammaticFocus);
                txbxNormal.PreventKeyboardDisplayOnProgrammaticFocus = false;
                Verify.IsTrue(txbxNormal.IsColorFontEnabled);
                txbxNormal.IsColorFontEnabled = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(txbxNormal.IsColorFontEnabled);
                txbxNormal.IsColorFontEnabled = true;
                Verify.AreEqual(string.Empty, txbxNormal.PlaceholderText);            
                txbxNormal.PlaceholderText = "placeholder text";
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("placeholder text", txbxNormal.PlaceholderText);
                txbxNormal.PlaceholderText = string.Empty;
                Log.Comment("TextBox property tests done");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoTextBoxMethodTests(TextBox txbxNormal)
        {
            UIExecutor.Execute(() =>
            {
                txbxNormal.Text = "text box method tests";
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                txbxNormal.SelectAll();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("text box method tests", txbxNormal.SelectedText);
                txbxNormal.Select(9,6);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("method", txbxNormal.SelectedText);
                Rect leading = txbxNormal.GetRectFromCharacterIndex(6, false);
                Rect trailing = txbxNormal.GetRectFromCharacterIndex(6, true);

                Log.Comment("TextBox.GetRectFromCharacterIndex - leading: (" + leading.X + "," + leading.Y + "),(" + leading.Width + "," + leading.Height + ")");
                Log.Comment("TextBox.GetRectFromCharacterIndex - trailing: (" + trailing.X + "," + trailing.Y + "),(" + trailing.Width + "," + trailing.Height + ")");

                Verify.IsFalse((leading.Width != 0) || (leading.Height < 10.0) || (leading.Height > 30.0));
                Verify.IsFalse((trailing.X - leading.X < 8.0) || (trailing.Height < 10.0) || (trailing.Height > 30.0));

                Log.Comment("TextBox method tests done");
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoTextBoxEventTests(TextBox txbxNormal)
        {
            
            using (var textChanged = new EventTester<TextBox, TextChangedEventArgs>(txbxNormal, "TextChanged"))
            using (var textSelectionChanged = new EventTester<TextBox, RoutedEventArgs>(txbxNormal, "SelectionChanged"))
            {
                Log.Comment("TextBox event tests.");
                FocusHelper.EnsureFocus(txbxNormal, FocusState.Keyboard);

                UIExecutor.Execute(() =>
                {
                    txbxNormal.Text = "test events";
                    txbxNormal.Select(2, 3);
                });
                textChanged.Wait();
                textSelectionChanged.Wait();

                Verify.AreEqual(1,textChanged.ExecuteCount);
                Verify.AreEqual(1, textSelectionChanged.ExecuteCount);
                TestServices.WindowHelper.WaitForIdle();
            }
            Log.Comment("TextBox event tests done");
        }

        private void DoRichEditBoxPropertyTests(RichEditBox rebxNormal)
        {
            UIExecutor.Execute(() =>
            {
                rebxNormal.Document.SetText(TextSetOptions.None, "property tests");
                Verify.IsFalse(rebxNormal.IsReadOnly);

                rebxNormal.IsReadOnly = true;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(rebxNormal.IsReadOnly);
                rebxNormal.IsReadOnly = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(rebxNormal.AcceptsReturn);
                rebxNormal.AcceptsReturn = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(rebxNormal.AcceptsReturn);
                rebxNormal.AcceptsReturn = true;
                Verify.AreEqual(TextWrapping.Wrap, rebxNormal.TextWrapping);
                rebxNormal.TextWrapping = TextWrapping.NoWrap;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextWrapping.NoWrap, rebxNormal.TextWrapping);
                rebxNormal.TextWrapping = TextWrapping.Wrap;
                Verify.AreEqual(TextAlignment.DetectFromContent, rebxNormal.TextAlignment);
                rebxNormal.TextAlignment = TextAlignment.Center;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextAlignment.Center, rebxNormal.TextAlignment);
                rebxNormal.TextAlignment = TextAlignment.Right;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextAlignment.Right, rebxNormal.TextAlignment);
                rebxNormal.TextAlignment = TextAlignment.Justify;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextAlignment.Justify, rebxNormal.TextAlignment);
                rebxNormal.TextAlignment = TextAlignment.Left;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(TextAlignment.Left, rebxNormal.TextAlignment);
                Verify.IsTrue(rebxNormal.IsSpellCheckEnabled);
                rebxNormal.IsSpellCheckEnabled = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(rebxNormal.IsSpellCheckEnabled);
                Verify.IsTrue(rebxNormal.IsTextPredictionEnabled);
                rebxNormal.IsTextPredictionEnabled = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(rebxNormal.IsTextPredictionEnabled);
                rebxNormal.IsTextPredictionEnabled = true;
                Verify.IsNull(rebxNormal.InputScope);

                InputScope scope = new InputScope();
                InputScopeName scopeName = new InputScopeName(InputScopeNameValue.TelephoneNumber);
                scope.Names.Add(scopeName);
                rebxNormal.InputScope = scope;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(rebxNormal.InputScope);
                Verify.AreEqual(1, rebxNormal.InputScope.Names.Count);
                Verify.AreEqual(InputScopeNameValue.TelephoneNumber, rebxNormal.InputScope.Names[0].NameValue);

                ITextCharacterFormat format = rebxNormal.Document.GetDefaultCharacterFormat();
                format.Bold = FormatEffect.On;
                rebxNormal.Document.SetDefaultCharacterFormat(format);
                // re-get the format and verify bold is set
                format = rebxNormal.Document.GetDefaultCharacterFormat();
                Verify.AreEqual(FormatEffect.On, format.Bold);

                Verify.IsFalse(rebxNormal.PreventKeyboardDisplayOnProgrammaticFocus);
                rebxNormal.PreventKeyboardDisplayOnProgrammaticFocus = true;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(rebxNormal.PreventKeyboardDisplayOnProgrammaticFocus);
                rebxNormal.PreventKeyboardDisplayOnProgrammaticFocus = false;

                Verify.IsTrue(rebxNormal.IsColorFontEnabled);
                rebxNormal.IsColorFontEnabled = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(rebxNormal.IsColorFontEnabled);
                rebxNormal.IsColorFontEnabled = true;
                Verify.AreEqual(string.Empty, rebxNormal.PlaceholderText);               
                rebxNormal.PlaceholderText = "placeholder text";
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("placeholder text", rebxNormal.PlaceholderText);
                Log.Comment("RichEditBox property tests done");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoRichEditBoxEventTests(RichEditBox rebxNormal)
        {

            FocusHelper.EnsureFocus(rebxNormal, FocusState.Keyboard);

            using (var textChanged = new EventTester<RichEditBox, RoutedEventArgs>(rebxNormal, "TextChanged"))
            using (var textSelectionChanged = new EventTester<RichEditBox, RoutedEventArgs>(rebxNormal, "SelectionChanged"))
            {
                UIExecutor.Execute(() =>
                {
                    rebxNormal.Document.SetText(TextSetOptions.None, "test events"); // This will cause a TextChanged event
                    rebxNormal.Document.GetRange(2, 3).MatchSelection();
                });

                textChanged.Wait();
                textSelectionChanged.Wait();

                Verify.AreEqual(1, textChanged.ExecuteCount);
                Verify.AreEqual(1, textSelectionChanged.ExecuteCount);
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("RichEditBox event tests done");
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoPasswordBoxPmeTests(PasswordBox pwbxNormal)
        {
            Log.Comment("PasswordBox PME tests.");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(String.Empty, pwbxNormal.Password);
                pwbxNormal.Password = "new text";
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("new text", pwbxNormal.Password);
                Verify.AreEqual("\u25CF", pwbxNormal.PasswordChar);
                pwbxNormal.PasswordChar = "@";
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("@", pwbxNormal.PasswordChar);
                Verify.AreEqual(0, pwbxNormal.MaxLength);
                pwbxNormal.MaxLength = 42;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(42, pwbxNormal.MaxLength);
                Verify.AreEqual(Microsoft.UI.Xaml.Controls.PasswordRevealMode.Hidden, pwbxNormal.PasswordRevealMode);
                pwbxNormal.PasswordRevealMode = Microsoft.UI.Xaml.Controls.PasswordRevealMode.Visible;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(Microsoft.UI.Xaml.Controls.PasswordRevealMode.Visible, pwbxNormal.PasswordRevealMode);
                Verify.IsFalse(pwbxNormal.PreventKeyboardDisplayOnProgrammaticFocus);
                pwbxNormal.PreventKeyboardDisplayOnProgrammaticFocus = true;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(pwbxNormal.PreventKeyboardDisplayOnProgrammaticFocus);
                pwbxNormal.PreventKeyboardDisplayOnProgrammaticFocus = false;
                Verify.AreEqual(String.Empty, pwbxNormal.PlaceholderText);
                pwbxNormal.PlaceholderText = "placeholder text";
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("placeholder text", pwbxNormal.PlaceholderText);
            });
            TestServices.WindowHelper.WaitForIdle();

            using (var passwordChanged = new EventTester<PasswordBox, RoutedEventArgs>(pwbxNormal, "PasswordChanged"))
            {
                FocusHelper.EnsureFocus(pwbxNormal, FocusState.Keyboard);
                UIExecutor.Execute(() =>
                {
                    pwbxNormal.SelectAll();
                });
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.PressKeySequence("p");
                TestServices.WindowHelper.WaitForIdle();
                passwordChanged.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("p", pwbxNormal.Password);
                    
                });
                TestServices.WindowHelper.WaitForIdle();
                Log.Comment("PasswordBox PME tests done");
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoTextBoxInputTests(
            TextBox txbxNormal,
            TextBox txbxHScroll,
            TextBox txbxVScroll)
        {
            using (var textPasted = new EventTester<TextBox, TextControlPasteEventArgs>(txbxNormal, "Paste"))
            {
                FocusHelper.EnsureFocus(txbxHScroll, FocusState.Programmatic);
                TestServices.KeyboardHelper.PressKeySequence("This is enough text to cause horizontal scrolling");
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(txbxVScroll, FocusState.Keyboard);
                TestServices.KeyboardHelper.PressKeySequence("1#$d$_enter#$u$_enter#2#$d$_enter#$u$_enter#3");
                TestServices.WindowHelper.WaitForIdle();

                // Verify Select All, Delete, Copy/Paste
                FocusHelper.EnsureFocus(txbxNormal, FocusState.Keyboard);
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.PressKeySequence("$d$_delete#$u$_delete#");
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    txbxNormal.Text = "text box method tests";
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.Copy();
                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.Paste();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("text box method tests", txbxNormal.Text);
                    Verify.AreEqual(1, textPasted.ExecuteCount);
                });
                Log.Comment("TextBox input tests done");
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoRichEditBoxInputTests(RichEditBox rebxNormal, RichEditBox rebxVScroll)
        {
            using (var textPasted = new EventTester<RichEditBox, TextControlPasteEventArgs>(rebxNormal, "Paste"))
            {
                FocusHelper.EnsureFocus(rebxNormal, FocusState.Keyboard);
                TestServices.KeyboardHelper.PressKeySequence("This is enough text to cause horizontal scrolling");
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(rebxVScroll, FocusState.Keyboard);
                TestServices.KeyboardHelper.PressKeySequence("1#$d$_enter#$u$_enter#2#$d$_enter#$u$_enter#3");
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(rebxNormal, FocusState.Programmatic);
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.PressKeySequence("$d$_delete#$u$_delete#");
                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.PressKeySequence("x");

                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.Copy();
                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.Paste();
                TestServices.WindowHelper.WaitForIdle();

                Verify.AreEqual(1, textPasted.ExecuteCount); ;
            }

            Log.Comment("RichEditBox input tests done");
            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoPasswordBoxInputTests(PasswordBox pwbxNormal, TextBox txbxNormal)
        {
            using (var textPasted = new EventTester<PasswordBox, TextControlPasteEventArgs>(pwbxNormal, "Paste"))
            {
                FocusHelper.EnsureFocus(pwbxNormal, FocusState.Keyboard);
                TestServices.KeyboardHelper.PressKeySequence("pwd");
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.PressKeySequence("$d$_delete#$u$_delete#");
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(String.Empty, pwbxNormal.Password);
                });

                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(txbxNormal, FocusState.Programmatic);

                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.Copy();
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(pwbxNormal, FocusState.Programmatic);

                TestServices.KeyboardHelper.Paste();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(txbxNormal.Text, pwbxNormal.Password);
                    Verify.AreEqual(1, textPasted.ExecuteCount);
                });
                Log.Comment("PasswordBox input tests done");
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoTextBoxAccReturnTextWrapTests(
            TextBox txbxARnoTWno,
            TextBox txbxARyesTWno,
            TextBox txbxARnoTWyes,
            TextBox txbxARyesTWyes)
        {
            FocusHelper.EnsureFocus(txbxARnoTWno, FocusState.Programmatic);
            TestServices.KeyboardHelper.PressKeySequence(testARvsTW);
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(txbxARyesTWno, FocusState.Programmatic);
            TestServices.KeyboardHelper.PressKeySequence(testARvsTW);
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(txbxARnoTWyes, FocusState.Programmatic);
            TestServices.KeyboardHelper.PressKeySequence(testARvsTW);
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(txbxARyesTWyes, FocusState.Programmatic);
            TestServices.KeyboardHelper.PressKeySequence(testARvsTW);
            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoRichEditBoxAccReturnTextWrapTests(
            RichEditBox rebxARnoTWno,
            RichEditBox rebxARyesTWno,
            RichEditBox rebxARnoTWyes,
            RichEditBox rebxARyesTWyes)
        {
            FocusHelper.EnsureFocus(rebxARnoTWno, FocusState.Programmatic);

            TestServices.KeyboardHelper.PressKeySequence(testARvsTW);
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(rebxARyesTWno, FocusState.Programmatic);
            TestServices.KeyboardHelper.PressKeySequence(testARvsTW);
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(rebxARnoTWyes, FocusState.Programmatic);
            TestServices.KeyboardHelper.PressKeySequence(testARvsTW);
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(rebxARyesTWyes, FocusState.Programmatic);
            TestServices.KeyboardHelper.PressKeySequence(testARvsTW);
            TestServices.WindowHelper.WaitForIdle();

        }

        private void FillInAlignmentTestData(
            TextBox txbxCenter,
            TextBox txbxRight,
            RichEditBox rebxCenter,
            RichEditBox rebxRight)
        {
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                txbxCenter.Text = "Center aligned condensed";
                txbxRight.Text = "Right aligned bold";

                rebxCenter.Document.SetText(TextSetOptions.None, "Center aligned condensed");
                rebxRight.Document.SetText(TextSetOptions.None, "Right aligned bold");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        private void VerifyDeleteAndRevealButtons(TextBox txbxNormal, PasswordBox pwbxNormal)
        {
            FocusHelper.EnsureFocus(txbxNormal, FocusState.Programmatic);
            // test clicking TextBox delete button
            UIExecutor.Execute(() =>
            {
                txbxNormal.Text = "delete button test";
            });
            TestServices.WindowHelper.WaitForIdle();

            using (var textChanged = new EventTester<TextBox, TextChangedEventArgs>(txbxNormal, "TextChanged"))
            {
                Button deleteButton = null;
                UIExecutor.Execute(() =>
                {
                    deleteButton = (Button)(VisualTreeUtils.FindNameInSubtree(txbxNormal, "DeleteButton"));
                    Verify.IsNotNull(deleteButton);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.InputHelper.LeftMouseClick(deleteButton);
                textChanged.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(String.Empty, txbxNormal.Text);
                });
                TestServices.WindowHelper.WaitForIdle();
            }

            using (var passwordChanged = new EventTester<PasswordBox, RoutedEventArgs>(pwbxNormal, "PasswordChanged"))
            {
                FocusHelper.EnsureFocus(pwbxNormal, FocusState.Programmatic);
                // verify PasswordBox with password revealed
                UIExecutor.Execute(() =>
                {
                    // must first focus and clear password box, or reveal button won't be shown
                    pwbxNormal.SelectAll();

                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.PressKeySequence("delete");
                passwordChanged.Wait();
                TestServices.WindowHelper.WaitForIdle();
            }

            using (var passwordChanged = new EventTester<PasswordBox, RoutedEventArgs>(pwbxNormal, "PasswordChanged"))
            {
                TestServices.KeyboardHelper.PressKeySequence("new password");
                passwordChanged.Wait();
                Microsoft.UI.Xaml.Controls.Primitives.ToggleButton revealButton = null;
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    revealButton = (Microsoft.UI.Xaml.Controls.Primitives.ToggleButton)(VisualTreeUtils.FindNameInSubtree(pwbxNormal, "RevealButton"));
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.InputHelper.MouseButtonDown(revealButton, 0, 0, MouseButton.Left);
                TestServices.WindowHelper.WaitForIdle();

                TestServices.Utilities.VerifyUIElementTree("1");

                TestServices.InputHelper.MouseButtonUp(revealButton, 0, 0, MouseButton.Left);
                Log.Comment("Delete and reveal button tests done");
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        private void DoRedlineTests(Button TestButton, RichEditBox rebxARyesTWyes, SolidColorBrush orangeBrush)
        {
            FocusHelper.EnsureFocus(TestButton, FocusState.Programmatic);
            // Test the caret color.
            UIExecutor.Execute(() =>
            {
                rebxARyesTWyes.Background = orangeBrush;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Checking snapshot");
            // final snapshot for all text controls, plus textbox with touch selection
            TestServices.Utilities.VerifyUIElementTree();
        }

        private void DoProgrammaticRichEditBoxTests(
            RichEditBox rebxNormal,
            RichEditBox rebxHScroll,
            RichEditBox rebxVScroll,
            RichEditBox rebxARnoTWno,
            RichEditBox rebxARyesTWno,
            RichEditBox rebxARnoTWyes,
            RichEditBox rebxARyesTWyes,
            PasswordBox pwbxNormal,
            SolidColorBrush orangeBrush,
            Button TestButton)
        {
            UIExecutor.Execute(() =>
            {
                rebxNormal.Document.SetText(TextSetOptions.None, "perform RichEditBox redline tests");
                rebxHScroll.Document.SetText(TextSetOptions.None, "This is enough text to cause horizontal scrolling");
                rebxVScroll.Document.SetText(TextSetOptions.None, "Line one\r\nSecond line\r\nLast line");
                rebxARnoTWno.Document.SetText(TextSetOptions.None, testRebARvsTW);
                rebxARyesTWno.Document.SetText(TextSetOptions.None, testRebARvsTW);
                rebxARnoTWyes.Document.SetText(TextSetOptions.None, testRebARvsTW);
                rebxARyesTWyes.Document.SetText(TextSetOptions.None, testRebARvsTW);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // Change the selection highlight color for the password box to be 
                // orange.
                pwbxNormal.SelectionHighlightColor = orangeBrush;
                pwbxNormal.SelectAll();
            });
            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(TestButton, FocusState.Pointer);

            // final snapshot for all text edit controls, plus RichEditBox with touch selection
            TestServices.Utilities.VerifyUIElementTree("2");
        }

        private Button CreateTestButton(uint left, uint top, uint width, uint height)
        {
            Button b = new Button();
            b.Width = width;
            b.Height = height;
            b.Content = "Test";
            b.SetValue(Canvas.LeftProperty, left);
            b.SetValue(Canvas.TopProperty, top);
            return b;
        }

        private Rectangle CreateTestRectangle(uint left, uint top, uint width, uint height)
        {
            Rectangle r = new Rectangle();
            r.Width = width;
            r.Height = height;
            r.Fill = new SolidColorBrush(Colors.Black);
            r.SetValue(Canvas.LeftProperty, left);
            r.SetValue(Canvas.TopProperty, top);
            return r;
        }
        #endregion
    }
}


