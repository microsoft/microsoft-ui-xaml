// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using XamlMedia = Microsoft.UI.Xaml.Media;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class TextBoxTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Validates that text input is canceled when setting Cancel to true in BeforeTextChanging")]
        public void CanCancelInputtingText()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBox x:Name='tb'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            var tbBeforeTextChangingHandler = new Action<object, TextBoxBeforeTextChangingEventArgs>((source, args) =>
            {
                Log.Comment("BeforeTextChanging fired");
                
                // After typing four characters, cancel any subsequent text
                if(args.NewText.Length > 4)
                {
                    args.Cancel = true;
                }
            });

            using (var beforeTextChangingTester = new EventTester<TextBox, TextBoxBeforeTextChangingEventArgs>(textBox, "BeforeTextChanging", tbBeforeTextChangingHandler))
            {
                TestServices.KeyboardHelper.PressKeySequence("tester");
                beforeTextChangingTester.Wait();

                Verify.IsTrue(beforeTextChangingTester.ExecuteCount == 6);
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(textBox.Text, "test");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verify that the new text is what we expect")]
        public void VerifyNewText()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBox x:Name='tb'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            var tbBeforeTextChangingHandler = new Action<object, TextBoxBeforeTextChangingEventArgs>((source, args) =>
            {
                Log.Comment("BeforeTextChanging fired");
                Verify.AreEqual(args.NewText, "test");
            });

            using (var beforeTextChangingTester = new EventTester<TextBox, TextBoxBeforeTextChangingEventArgs>(textBox, "BeforeTextChanging", tbBeforeTextChangingHandler))
            {
                UIExecutor.Execute(() =>
                {
                    textBox.Text = "test";
                });

                beforeTextChangingTester.Wait();
                Verify.IsTrue(beforeTextChangingTester.ExecuteCount == 1);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that BeforeTextChanging, TextChanging, and TextChanged are ordered correctly")]
        public void VerifyTextEventsDoNotFollowBeforeTextChangingCancel()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBox x:Name='tb'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;
            string sequence = "";
            const string order = "[BeforeTextChanging][TextChanging][TextChanged][BeforeTextChanging][TextChanging][TextChanged]" +
                                "[BeforeTextChanging][TextChanging][TextChanged][BeforeTextChanging][TextChanging][TextChanged]" +
                                "[BeforeTextChanging][BeforeTextChanging]";

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            var tbBeforeTextChangingHandler = new Action<object, TextBoxBeforeTextChangingEventArgs>((source, args) =>
            {
                Log.Comment("BeforeTextChanging fired");
                sequence = sequence + "[BeforeTextChanging]";
                
                // After typing four characters, cancel any subsequent text
                if(args.NewText.Length > 4)
                {
                    args.Cancel = true;
                }
            });

            var tbTextChangingHandler = new Action<object, TextBoxTextChangingEventArgs>((source, args) =>
            {
                Log.Comment("TextChanging fired");
                sequence = sequence + "[TextChanging]";
            });

            var tbTextChangedHandler = new Action<object, TextChangedEventArgs>((source, args) =>
            {
                Log.Comment("TextChanged fired");
                sequence = sequence + "[TextChanged]";
            });

            using (var textChangingTester = new EventTester<TextBox, TextBoxTextChangingEventArgs>(textBox, "TextChanging", tbTextChangingHandler))
            using (var textChangedTester = new EventTester<TextBox, TextChangedEventArgs>(textBox, "TextChanged", tbTextChangedHandler)) 
            using (var beforeTextChangingTester = new EventTester<TextBox, TextBoxBeforeTextChangingEventArgs>(textBox, "BeforeTextChanging", tbBeforeTextChangingHandler))
            {

                string text = "tester";

                for(int count = 0; count < text.Length; count++)
                {
                    TestServices.KeyboardHelper.PressKeySequence("" + text[count]);
                    beforeTextChangingTester.Wait();

                    if(count < 4 )
                    {
                        textChangingTester.Wait();
                        textChangedTester.Wait();
                    }
                }

                Verify.IsTrue(textChangingTester.ExecuteCount == 4);
                Verify.IsTrue(textChangedTester.ExecuteCount == 4);
                Verify.IsTrue(beforeTextChangingTester.ExecuteCount == 6);
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(textBox.Text, "test");
                    Verify.AreEqual(sequence, order);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates that text inputted programmatically is canceled when setting Cancel to true in BeforeTextChanging")]
        public void CanCancelInputtingTextSetProgrammatically()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBox x:Name='tb'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            var tbBeforeTextChangingHandler = new Action<object, TextBoxBeforeTextChangingEventArgs>((source, args) =>
            {
                Log.Comment("BeforeTextChanging fired");
                
                // After typing four characters, cancel any subsequent text
                if(args.NewText.Length > 4)
                {
                    args.Cancel = true;
                }
            });

            using (var beforeTextChangingTester = new EventTester<TextBox, TextBoxBeforeTextChangingEventArgs>(textBox, "BeforeTextChanging", tbBeforeTextChangingHandler))
            {
                UIExecutor.Execute(() =>
                {
                    textBox.Text = "abcd";
                });

                beforeTextChangingTester.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(textBox.Text, "abcd");
                    textBox.Text = "tester";
                });

                beforeTextChangingTester.Wait();

                Verify.IsTrue(beforeTextChangingTester.ExecuteCount == 2);
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(textBox.Text, "abcd");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "When we cancel, we should verify that the text selection is maintained")]
        public void CancelingMaintainsTextSelection()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBox x:Name='tb' Text='Hello World'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            var tbBeforeTextChangingHandler = new Action<object, TextBoxBeforeTextChangingEventArgs>((source, args) =>
            {
                Log.Comment("BeforeTextChanging fired");
                
                args.Cancel = true; 
                Verify.AreEqual(args.NewText, "Heaorld");
            });

            using (var beforeTextChangingTester = new EventTester<TextBox, TextBoxBeforeTextChangingEventArgs>(textBox, "BeforeTextChanging", tbBeforeTextChangingHandler))
            {
                UIExecutor.Execute(() =>
                {
                    textBox.Select(2, 5);
                });

                TestServices.KeyboardHelper.PressKeySequence("a");
                beforeTextChangingTester.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(textBox.SelectionStart, 2);
                    Verify.AreEqual(textBox.SelectionLength, 5);
                    Verify.AreEqual(textBox.Text, "Hello World");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates that text inputted through paste is canceled when setting Cancel to true in BeforeTextChanging")]
        public void CanCancelInputtingTextThroughPaste()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBox x:Name='tb' Text='test'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            var tbBeforeTextChangingHandler = new Action<object, TextBoxBeforeTextChangingEventArgs>((source, args) =>
            {
                Log.Comment("BeforeTextChanging fired");
                args.Cancel = true;
            });

            using (var textCopied = new EventTester<TextBox, Microsoft.UI.Xaml.Controls.TextControlCopyingToClipboardEventArgs>(textBox, "CopyingToClipboard"))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
                TestServices.WindowHelper.WaitForIdle();
                Log.Comment("Ctrl+C--->Copy...");
                TestServices.KeyboardHelper.Copy();
                textCopied.Wait();
            }

            using (var beforeTextChangingTester = new EventTester<TextBox, TextBoxBeforeTextChangingEventArgs>(textBox, "BeforeTextChanging", tbBeforeTextChangingHandler))
            using (var textPasted = new EventTester<TextBox, TextControlPasteEventArgs>(textBox, "Paste"))
            {
                Log.Comment("Ctrl+A--->Select All...");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
                TestServices.WindowHelper.WaitForIdle();
                Log.Comment("Ctrl+V--->Paste...");
                TestServices.KeyboardHelper.Paste();
                beforeTextChangingTester.Wait();
                textPasted.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(textBox.Text, "test");
                });
            }
        }
        
        [TestMethod]
        [TestProperty("Description", "When we cancel incoming text, we need to make sure that the undo stack is cleared")]
        public void VerifyUndoStackClearedAfterCancel()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBox x:Name='tb'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            var tbBeforeTextChangingHandler = new Action<object, TextBoxBeforeTextChangingEventArgs>((source, args) =>
            {
                Log.Comment("BeforeTextChanging fired");
                
                // After typing four characters, cancel any subsequent text
                if(args.NewText.Length > 4)
                {
                    args.Cancel = true;
                }
            });

            using (var beforeTextChangingTester = new EventTester<TextBox, TextBoxBeforeTextChangingEventArgs>(textBox, "BeforeTextChanging", tbBeforeTextChangingHandler))
            {
                TestServices.KeyboardHelper.PressKeySequence("tester");
                beforeTextChangingTester.Wait();

                Verify.IsTrue(beforeTextChangingTester.ExecuteCount == 6);
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(textBox.Text, "test");
                });

                //Undo
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(textBox.Text, "test");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "WHen we cancel, no bindings should be updated")]
        public void CancelingShouldAlsoCancelBindingChange()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBox x:Name='tbSource' FontSize='20' Width='200' Text='test' />
                        <TextBox x:Name='tb' FontSize='20' Width='200' Text='{Binding Text, ElementName = tbSource}'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;
            TextBox sourceTextBox = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");
                sourceTextBox = (TextBox)rootPanel.FindName("tbSource");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(sourceTextBox, FocusState.Keyboard);

            var tbBeforeTextChangingHandler = new Action<object, TextBoxBeforeTextChangingEventArgs>((source, args) =>
            {
                Log.Comment("BeforeTextChanging fired");
                args.Cancel = true;
            });

            using (var beforeTextChangingTester = new EventTester<TextBox, TextBoxBeforeTextChangingEventArgs>(sourceTextBox, "BeforeTextChanging", tbBeforeTextChangingHandler))
            {
                UIExecutor.Execute(() =>
                {
                    sourceTextBox.Text = "XYZ";
                });

                beforeTextChangingTester.Wait();

               UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(sourceTextBox.Text, "test");
                    Verify.AreEqual(textBox.Text, "test");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates textbox control BeforeTextChanging event re-entrancy call does not crash")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void TextBoxBeforeTextChangingReentrancyCheck()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBlock x:Name='textBlock' FontSize='20' Width='200' Text='TextBlock' />
                        <TextBox x:Name='tb' FontSize='20' Width='200' Text='{Binding Text, ElementName = textBlock}'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;
            TextBlock textBlock = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");
                textBlock = (TextBlock)rootPanel.FindName("textBlock");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            using (var beforeTextChangingTester = new EventTester<TextBox, TextBoxBeforeTextChangingEventArgs>(textBox, "BeforeTextChanging"))
            {
                UIExecutor.Execute(() =>
                {
                    textBlock.Text = "XYZ";
                });

                beforeTextChangingTester.Wait();
            }
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void VerifyTextBoxPlaceholderForeground()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='Button' />
                        <TextBox x:Name='tb' FontSize='20' Width='200' PlaceholderText='Hello World' PlaceholderForeground='Yellow'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;
            Button button = null;
            TextBlock placeholdertb = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");
                button = (Button)rootPanel.FindName("button");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            
            Log.Comment("Test case: Unfocused");
            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                placeholdertb = (TextBlock)VisualTreeUtils.FindNameInSubtree(textBox, "PlaceholderTextContentPresenter");
                
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Yellow, solidColorBrush.Color, "Verify Foreground");
            });
            
            Log.Comment("Test case: Focused");
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Yellow, solidColorBrush.Color, "Verify Foreground");
            });

            Log.Comment("Test case: FocusedAfterSetting");
            UIExecutor.Execute(() =>
            {
                textBox.PlaceholderForeground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Colors.Red);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Red, solidColorBrush.Color, "Verify Foreground");
            });

            Log.Comment("Test case: Unfocused2");
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Red, solidColorBrush.Color, "Verify Foreground");
            });

            Log.Comment("Test case: Disabled");
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                textBox.IsEnabled = false;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Red, solidColorBrush.Color, "Verify Foreground");
            });

            Log.Comment("Test case: DisabledAfterSetting");
            UIExecutor.Execute(() =>
            {
                textBox.PlaceholderForeground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Colors.Blue);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Blue, solidColorBrush.Color, "Verify Foreground");
            });
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void VerifyTextBoxPlaceholderForegroundPersistBetweenThemeChanges()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='Button' />
                        <TextBox x:Name='tb' FontSize='20' Width='200' PlaceholderText='Hello World' PlaceholderForeground='Yellow'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            TextBox textBox = null;
            Button button = null;
            TextBlock placeholdertb = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");
                button = (Button)rootPanel.FindName("button");

                rootPanel.RequestedTheme = Microsoft.UI.Xaml.ElementTheme.Dark;

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button, FocusState.Keyboard);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                placeholdertb = (TextBlock)VisualTreeUtils.FindNameInSubtree(textBox, "PlaceholderTextContentPresenter");
                
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Yellow, solidColorBrush.Color, "Verify Foreground (Dark theme)");
            });

            // Test Light theme:
            UIExecutor.Execute(() =>
            {
                rootPanel.RequestedTheme = Microsoft.UI.Xaml.ElementTheme.Light;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Yellow, solidColorBrush.Color, "Verify Foreground (Lught theme)");
            });
        }

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyTextBoxPlaceholderForegroundHC()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='Button' />
                        <TextBox x:Name='tb' FontSize='20' Width='200' PlaceholderText='Hello World' PlaceholderForeground='Yellow'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            TextBox textBox = null;
            Button button = null;
            TextBlock placeholdertb = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                textBox = (TextBox)rootPanel.FindName("tb");
                button = (Button)rootPanel.FindName("button");
                TestServices.ThemingHelper.HighContrastTheme = HighContrastTheme.Test;
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            Log.Comment("Unfocused");
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                placeholdertb = (TextBlock)VisualTreeUtils.FindNameInSubtree(textBox, "PlaceholderTextContentPresenter");
                
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Yellow, solidColorBrush.Color, "Verify Foreground");
            });

            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);
            Log.Comment("Focused");

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Yellow, solidColorBrush.Color, "Verify Foreground");
            });

            UIExecutor.Execute(() =>
            {
                textBox.PlaceholderForeground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Colors.Red);
            });

            Log.Comment("FocusedAfterSetting");
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Red, solidColorBrush.Color, "Verify Foreground");
            });


            FocusHelper.EnsureFocus(button, FocusState.Keyboard);
            Log.Comment("Unfocused_2");

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Red, solidColorBrush.Color, "Verify Foreground");
            });

            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                textBox.IsEnabled = false;
            });

            Log.Comment("Disabled");
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Red, solidColorBrush.Color, "Verify Foreground");
            });

            UIExecutor.Execute(() =>
            {
                textBox.PlaceholderForeground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Colors.Blue);
            });

            Log.Comment("DisabledAfterSetting");
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var solidColorBrush = placeholdertb.Foreground as XamlMedia.SolidColorBrush;
                Verify.IsNotNull(solidColorBrush);
                Verify.AreEqual(Colors.Blue, solidColorBrush.Color, "Verify Foreground");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Validates that text alignment can be set on placeholder text.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]  // release queue not empty issue still happens in catgates run, blocking it for now
        public void VerifyPlaceholderTextAlignment()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btn'> Button </Button>
                        <TextBox x:Name='tb' PlaceholderText='PlaceHolder' TextAlignment='Right'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button btn = null;
            TextBox tb = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                btn = (Button)rootPanel.FindName("btn");
                tb = (TextBox)rootPanel.FindName("tb");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(btn, FocusState.Pointer);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var placeholdertb = (TextBlock)VisualTreeUtils.FindNameInSubtree(tb, "PlaceholderTextContentPresenter");
                Verify.AreEqual(TextAlignment.Right, placeholdertb.TextAlignment, "Verify TextAlignment");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Validates that placeholder text does not overlap with textbox text.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void VerifyPlaceholderTextDoesNotShowWhenTextIsNotEmpty()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btn'> Button </Button>
                        <TextBox x:Name='tb' Text='Lorem Ipsum'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button btn = null;
            TextBox txb = null;
            TextBlock placeholdertb = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                btn = (Button)rootPanel.FindName("btn");
                txb = (TextBox)rootPanel.FindName("tb");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(btn, FocusState.Pointer);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                placeholdertb = (TextBlock)VisualTreeUtils.FindNameInSubtree(txb, "PlaceholderTextContentPresenter");
                
                Verify.AreEqual(Visibility.Collapsed, placeholdertb.Visibility, "Verify Visibility");
            });
            TestServices.WindowHelper.WaitForIdle();
            
            Log.Comment("Set PlaceholderText");
            UIExecutor.Execute(() =>
            {
                txb.PlaceholderText = "PLACEHOLDER";
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(Visibility.Collapsed, placeholdertb.Visibility, "Verify Visibility");
            });

            Log.Comment("Set TextBox.Text");
            UIExecutor.Execute(() =>
            {
                txb.Text = "";
            });
            TestServices.WindowHelper.WaitForIdle();
            
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(Visibility.Visible, placeholdertb.Visibility, "Verify Visibility");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verify Hold on grippers invokes Context Menu.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Ignore", "True")] // WinUI 3: TextBoxTests.VerifyGripperHoldBringsUpContextMenu is unreliable
        public void VerifyGripperHoldBringsUpContextMenu()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btn'> Button </Button>
                        <TextBox x:Name='tb' Text='LoremIpsumdfgdgdsfkghsfgkjdsghjkdshgksdjghsdkjghdskfghdksjghdskghdsdkfgjhdslkgjhdklfghlfdkj'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button btn = null;
            TextBox txb = null;
            FrameworkElement rectToTap = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                btn = (Button)rootPanel.FindName("btn");
                txb = (TextBox)rootPanel.FindName("tb");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(txb, FocusState.Pointer);

            using (var selectionChanged = new EventTester<TextBox, RoutedEventArgs>(txb, "SelectionChanged"))
            {
                TestServices.InputHelper.DoubleTap(txb);
                selectionChanged.Wait();
            }

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var popups = XamlMedia.VisualTreeHelper.GetOpenPopupsForXamlRoot(btn.XamlRoot);
                foreach (var p in popups)
                {
                    FrameworkElement gripper = p.Child as FrameworkElement;
                    rectToTap = VisualTreeUtils.FindElementOfTypeInSubtree<Microsoft.UI.Xaml.Shapes.Rectangle>(gripper);

                    // If we find a popup with a rectantgle in it, we assume it is the gripper, and tap on the rectangle.
                    if (rectToTap != null) { break; }
                }
            });
            TestServices.WindowHelper.WaitForIdle();

            using (var contextMenuOpening = new EventTester<TextBox, ContextMenuEventArgs>(txb, "ContextMenuOpening"))
            {
                Verify.IsNotNull(rectToTap);
                TestServices.InputHelper.Hold(rectToTap);
                contextMenuOpening.Wait(TimeSpan.FromSeconds(2));
            }
            TestServices.WindowHelper.WaitForIdle();
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                TestServices.KeyboardHelper.Escape();
                TestServices.WindowHelper.WaitForIdle();
            }

        }
    }
}
