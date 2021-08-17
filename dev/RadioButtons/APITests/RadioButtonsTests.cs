// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Windows.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;
using Common;
using Windows.UI.Xaml.Markup;
using System.Collections.Generic;
using Windows.UI.Xaml.Media;
using System.Linq;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class RadioButtonsTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyCustomItemTemplate()
        {
            RadioButtons radioButtons = null;
            RadioButtons radioButtons2 = null;
            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons();
                radioButtons.ItemsSource = new List<string>() { "Option 1", "Option 2" };

                // Set a custom ItemTemplate to be wrapped in a RadioButton.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text = '{Binding}'/>
                        </DataTemplate>");

                radioButtons.ItemTemplate = itemTemplate;

                radioButtons2 = new RadioButtons();
                radioButtons2.ItemsSource = new List<string>() { "Option 1", "Option 2" };

                // Set a custom ItemTemplate which is already a RadioButton. No wrapping should be performed.
                var itemTemplate2 = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <RadioButton Foreground='Blue'>
                              <TextBlock Text = '{Binding}'/>
                            </RadioButton>
                        </DataTemplate>");

                radioButtons2.ItemTemplate = itemTemplate2;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(radioButtons);
                stackPanel.Children.Add(radioButtons2);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var radioButton1 = radioButtons.ContainerFromIndex(0) as RadioButton;
                var radioButton2 = radioButtons2.ContainerFromIndex(0) as RadioButton;
                Verify.IsNotNull(radioButton1, "Our custom ItemTemplate should have been wrapped in a RadioButton.");
                Verify.IsNotNull(radioButton2, "Our custom ItemTemplate should have been wrapped in a RadioButton.");

                bool testCondition = !(radioButton1.Foreground is SolidColorBrush brush && brush.Color == Colors.Blue);
                Verify.IsTrue(testCondition, "Default foreground color of the RadioButton should not have been [blue].");

                testCondition = radioButton2.Foreground is SolidColorBrush brush2 && brush2.Color == Colors.Blue;
                Verify.IsTrue(testCondition, "The foreground color of the RadioButton should have been [blue].");
            });
        }

        [TestMethod]
        public void ValidateSelectionChangedEvent()
        {
            int testCaseNumber = -1; // No valid test case
            int selectionChangedRaisedCounter = 0;

            RadioButtons radioButtons = null;
            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons();
                radioButtons.Items.Add("0");
                radioButtons.Items.Add("1");
                radioButtons.Items.Add("2");

                Log.Comment("Register SelectionChanged event handler");
                radioButtons.SelectionChanged += OnSelectionChangedEvent;

                Content = radioButtons;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Select item \"1\" as the initial selection");
                testCaseNumber = 0;
                radioButtons.SelectedIndex = 1;

                Verify.AreEqual(1, selectionChangedRaisedCounter, "The SelectionChanged event should have been raised 1 time in total");

                Log.Comment("Select item \"2\"");
                testCaseNumber = 1;
                radioButtons.SelectedIndex = 2;
                Verify.AreEqual(2, selectionChangedRaisedCounter, "The SelectionChanged event should have been raised 2 times in total");

                Log.Comment("Clear selection");
                testCaseNumber = 2;
                radioButtons.SelectedIndex = -1;
                Verify.AreEqual(3, selectionChangedRaisedCounter, "The SelectionChanged event should have been raised 3 times in total");
            });

            void OnSelectionChangedEvent(object sender, SelectionChangedEventArgs e)
            {
                selectionChangedRaisedCounter++;

                bool testCondition = false;
                switch (testCaseNumber)
                {
                    case 0: // Verify selection of item "1" as initial selection
                        testCondition = e.AddedItems.Count == 1 && e.AddedItems[0] is string s1 && s1 == "1";
                        Verify.IsTrue(testCondition, "Initial selection: SelectionChangedEventArgs.AddedItems should have contained the single item \"1\"");

                        testCondition = e.RemovedItems.Count == 0;
                        Verify.IsTrue(testCondition, "Initial selection: SelectionChangedEventArgs.RemovedItems should have been empty");
                        break;
                    case 1: // Verify selection of item "2" while item "1" is selected
                        testCondition = e.AddedItems.Count == 1 && e.AddedItems[0] is string s2 && s2 == "2";
                        Verify.IsTrue(testCondition, "Updated selection: SelectionChangedEventArgs.AddedItems should have contained the single item \"2\"");

                        testCondition = e.RemovedItems.Count == 1 && e.RemovedItems[0] is string s3 && s3 == "1";
                        Verify.IsTrue(testCondition, "Updated selection: SelectionChangedEventArgs.RemovedItem should have contained the single item \"1\"");
                        break;
                    case 2: // Verify clearing selection
                        testCondition = e.AddedItems.Count == 0;
                        Verify.IsTrue(testCondition, "Cleared selection: SelectionChangedEventArgs.AddedItems should have been empty");

                        testCondition = e.RemovedItems.Count == 1 && e.RemovedItems[0] is string s4 && s4 == "2";
                        Verify.IsTrue(testCondition, "Cleared selection: SelectionChangedEventArgs.RemovedItems should have contained the single item \"2\"");
                        break;
                }
            }
        }

        [TestMethod]
        public void VerifyIsEnabledChangeUpdatesVisualState()
        {
            RadioButtons radioButtons = null; ;
            VisualStateGroup commonStatesGroup = null;
            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons();

                // Check 1: Set IsEnabled to true.
                radioButtons.IsEnabled = true;

                Content = radioButtons;
                Content.UpdateLayout();

                var radioButtonsLayoutRoot = (FrameworkElement)VisualTreeHelper.GetChild(radioButtons, 0);
                commonStatesGroup = VisualStateManager.GetVisualStateGroups(radioButtonsLayoutRoot).First(vsg => vsg.Name.Equals("CommonStates"));

                Verify.AreEqual("Normal", commonStatesGroup.CurrentState.Name);

                // Check 2: Set IsEnabled to false.
                radioButtons.IsEnabled = false;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Disabled", commonStatesGroup.CurrentState.Name);

                // Check 3: Set IsEnabled back to true.
                radioButtons.IsEnabled = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Normal", commonStatesGroup.CurrentState.Name);
            });
        }
    }
}
