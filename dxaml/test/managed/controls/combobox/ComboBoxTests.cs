// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation;
using Windows.UI.Popups;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class ComboBoxTests : XamlTestsBase
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
        public void EnterAndEscapewithoutDelay()
        {
            ComboBox comboBox = null;

            UIExecutor.Execute(() =>
            {
                comboBox = new ComboBox();
                comboBox.Items.Add(new ComboBoxItem() { Content = "1" });
                comboBox.Items.Add(new ComboBoxItem() { Content = "2" });
                comboBox.Items.Add(new ComboBoxItem() { Content = "3" });

                TestServices.WindowHelper.WindowContent = comboBox;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                comboBox.Focus(FocusState.Programmatic);
            });

            TestServices.KeyboardHelper.Enter();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(comboBox.IsDropDownOpen);
            });

            TestServices.KeyboardHelper.Escape();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(comboBox.IsDropDownOpen);
            });

            TestServices.KeyboardHelper.Enter();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(comboBox.IsDropDownOpen);
            });
        }

        // ComboBox keeps track of the scrolled offset internally. If we 
        // scroll to the end in the open ComboBox, close it, reduce the number of items
        // then open and scroll, we used the incorrect offset and tried to access beyond the
        // collection bounds. The fix for this bug was to clamp the index to be within the
        // collection bounds so we don't crash.
        // The test does the following steps to repro the issue
        // 1. Create combobox with ItemsSource set to 200 items
        // 2. Open the combobox and go to the last item (keyboard.end)
        // 3. Close the combobox. It will remember the offset.
        // 4. Change the ItemsSource to have fewer items (30)
        // 5. Open the combobox and scroll (we used to crash here).
        [TestMethod]
        public void ComboBoxScrollAfterChangingItems()
        {
            ComboBox comboBox = null;
            ObservableCollection<int> data = new ObservableCollection<int>(Enumerable.Range(0, 200));
            UIExecutor.Execute(() =>
            {
                comboBox = new ComboBox();
                comboBox.ItemsSource = data;
                TestServices.WindowHelper.WindowContent = comboBox;
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                comboBox.IsDropDownOpen = true;
                comboBox.Focus(FocusState.Keyboard);
            });

            TestServices.WindowHelper.WaitForIdle();
            TestServices.KeyboardHelper.End();
            TestServices.WindowHelper.WaitForIdle();
            TestServices.KeyboardHelper.Escape();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                data.Clear();
                for (int i = 0; i < 30; i++)
                {
                    data.Add(i);
                }

                comboBox.IsDropDownOpen = true;
            });

            TestServices.WindowHelper.WaitForIdle();
            TestServices.InputHelper.PanFromCenter(comboBox, 10, 500, 1);
            TestServices.WindowHelper.WaitForIdle();
            // we used to crash while panning
            UIExecutor.Execute(() =>
            {
                comboBox.IsDropDownOpen = false;
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void VerifyComboBoxPlaceholderForeground()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='Button' />
                        <ComboBox x:Name='comboBox' FontSize='20' Width='200' PlaceholderText='Hello World' PlaceholderForeground='Yellow'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            ComboBox comboBox = null;
            Button button = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                comboBox = (ComboBox)rootPanel.FindName("comboBox");
                button = (Button)rootPanel.FindName("button");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            TestServices.Utilities.VerifyUIElementTree("Unfocused");

            FocusHelper.EnsureFocus(comboBox, FocusState.Keyboard);
            TestServices.Utilities.VerifyUIElementTree("Focused");

            UIExecutor.Execute(() =>
            {
                comboBox.PlaceholderForeground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Microsoft.UI.Colors.Red);
            });

            TestServices.Utilities.VerifyUIElementTree("FocusedAfterSetting");

            FocusHelper.EnsureFocus(button, FocusState.Keyboard);
            TestServices.Utilities.VerifyUIElementTree("Unfocused_2");

            FocusHelper.EnsureFocus(comboBox, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                comboBox.IsEnabled = false;
            });

            TestServices.Utilities.VerifyUIElementTree("Disabled");

            UIExecutor.Execute(() =>
            {
                comboBox.PlaceholderForeground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Microsoft.UI.Colors.Blue);
            });

            TestServices.Utilities.VerifyUIElementTree("DisabledAfterSetting");
        }

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void VerifyComboBoxPlaceholderForegroundHC()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='Button' />
                        <ComboBox x:Name='comboBox' FontSize='20' Width='200' PlaceholderText='Hello World'/>
                    </StackPanel>";
            
            StackPanel rootPanel = null;
            ComboBox comboBox = null;
            Button button = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                comboBox = (ComboBox)rootPanel.FindName("comboBox");
                button = (Button)rootPanel.FindName("button");

                TestServices.ThemingHelper.HighContrastTheme = HighContrastTheme.Test;
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            TestServices.Utilities.VerifyUIElementTree("Unfocused");

            FocusHelper.EnsureFocus(comboBox, FocusState.Keyboard);
            TestServices.Utilities.VerifyUIElementTree("Focused");

            UIExecutor.Execute(() =>
            {
                comboBox.PlaceholderForeground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Microsoft.UI.Colors.Red);
            });

            TestServices.Utilities.VerifyUIElementTree("FocusedAfterSetting");

            FocusHelper.EnsureFocus(button, FocusState.Keyboard);
            TestServices.Utilities.VerifyUIElementTree("Unfocused_2");

            FocusHelper.EnsureFocus(comboBox, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                comboBox.IsEnabled = false;
            });

            TestServices.Utilities.VerifyUIElementTree("Disabled");

            UIExecutor.Execute(() =>
            {
                comboBox.PlaceholderForeground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Microsoft.UI.Colors.Blue);
            });

            TestServices.Utilities.VerifyUIElementTree("DisabledAfterSetting");
        }

        [TestMethod]
        // Test relies on MessageDialog behavior of taking focus off the current window on Desktop and Xbox.
        [TestProperty("TestPass:ExcludeOn", "OneCore,WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateFocusStateWhenWindowLosesFocus()
        {
            ComboBox comboBox = null;
            VisualStateGroup focusGroup = null;
            IAsyncOperation<IUICommand> dialogOperation = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Setting up the visual tree...");
                comboBox = new ComboBox();
                comboBox.Items.Add(new ComboBoxItem() { Content = "Option 1" });
                comboBox.Items.Add(new ComboBoxItem() { Content = "Option 2" });
                comboBox.Items.Add(new ComboBoxItem() { Content = "Option 3" });

                TestServices.WindowHelper.WindowContent = comboBox;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Ensure ComboBox has keyboard focus.");
            FocusHelper.EnsureFocus(comboBox, FocusState.Keyboard);

            using (var eventTester = new EventTester<ComboBox, RoutedEventArgs>(comboBox, "LostFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    var groups = VisualStateManager.GetVisualStateGroups((FrameworkElement)VisualTreeHelper.GetChild(comboBox, 0));
                    focusGroup = groups.Where(g => g.Name == "FocusStates").Single();

                    Verify.AreEqual("Focused", focusGroup.CurrentState.Name);

                    Log.Comment("Open dialog to make the current window lose foreground.");
                    var dialog = new MessageDialog("Foreground Dialog");
                    dialogOperation = dialog.ShowAsync();
                });

                eventTester.Wait(TimeSpan.FromSeconds(5));
            }

            using (var eventTester = new EventTester<ComboBox, RoutedEventArgs>(comboBox, "GotFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("Unfocused", focusGroup.CurrentState.Name);

                    Log.Comment("Close dialog to make the current window regain foreground.");
                    dialogOperation.Cancel();
                });

                eventTester.Wait(TimeSpan.FromSeconds(5));
            }

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual("Focused", focusGroup.CurrentState.Name);
            });
        }

        [TestMethod]
        public void SelectItemWithCharacterKeystrokeAndNullItem()
        {
            ComboBox comboBox = null;

            UIExecutor.Execute(() =>
            {
                List<string> items = new List<string>();
                items.Add("a");
                items.Add("b");
                items.Add(null);
                items.Add("c");

                comboBox = new ComboBox();
                comboBox.ItemsSource = items;

                TestServices.WindowHelper.WindowContent = comboBox;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                comboBox.Focus(FocusState.Programmatic);
            });

            TestServices.KeyboardHelper.Enter();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(comboBox.IsDropDownOpen);
            });

            TestServices.KeyboardHelper.PressKeySequence("c");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(comboBox.IsDropDownOpen);
            });

            TestServices.KeyboardHelper.Enter();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(comboBox.IsDropDownOpen);
                Verify.AreEqual(3, comboBox.SelectedIndex);
            });
        }
    }
}

