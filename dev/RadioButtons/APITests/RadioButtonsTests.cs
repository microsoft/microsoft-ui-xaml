// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Windows.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;
using Common;
using Windows.UI.Xaml.Markup;
using System.Collections.Generic;
using Windows.UI.Xaml.Media;

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
            bool item1InitiallySelected = false;
            bool item2Selected = false;
            bool clearedSelection = false;
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
                item1InitiallySelected = true;
                radioButtons.SelectedIndex = 1;
                item1InitiallySelected = false;

                Verify.AreEqual(1, selectionChangedRaisedCounter, "The SelectionChanged event should have been raised 1 time in total");

                Log.Comment("Select item \"2\"");
                item2Selected = true;
                radioButtons.SelectedIndex = 2;
                item2Selected = false;
                Verify.AreEqual(2, selectionChangedRaisedCounter, "The SelectionChanged event should have been raised 2 times in total");

                Log.Comment("Clear selection");
                clearedSelection = true;
                radioButtons.SelectedIndex = -1;
                clearedSelection = false;
                Verify.AreEqual(3, selectionChangedRaisedCounter, "The SelectionChanged event should have been raised 3 times in total");
            });

            void OnSelectionChangedEvent(object sender, SelectionChangedEventArgs e)
            {
                selectionChangedRaisedCounter++;

                bool testCondition = false;
                if (item1InitiallySelected)
                {
                    testCondition = e.AddedItems.Count == 1 && e.AddedItems[0] is string s1 && s1 == "1";
                    Verify.IsTrue(testCondition, "Initial selection: SelectionChangedEventArgs.AddedItems should have contained the single item \"1\"");

                    testCondition = e.RemovedItems.Count == 0;
                    Verify.IsTrue(testCondition, "Initial selection: SelectionChangedEventArgs.RemovedItems should have been empty");
                }
                else if (item2Selected)
                {
                    testCondition = e.AddedItems.Count == 1 && e.AddedItems[0] is string s2 && s2 == "2";
                    Verify.IsTrue(testCondition, "Updated selection: SelectionChangedEventArgs.AddedItems should have contained the single item \"2\"");

                    testCondition = e.RemovedItems.Count == 1 && e.RemovedItems[0] is string s3 && s3 == "1";
                    Verify.IsTrue(testCondition, "Updated selection: SelectionChangedEventArgs.RemovedItem should have contained the single item \"1\"");
                }
                else if (clearedSelection)
                {
                    testCondition = e.AddedItems.Count == 0;
                    Verify.IsTrue(testCondition, "Cleared selection: SelectionChangedEventArgs.AddedItems should have been empty");

                    testCondition = e.RemovedItems.Count == 1 && e.RemovedItems[0] is string s4 && s4 == "2";
                    Verify.IsTrue(testCondition, "Cleared selection: SelectionChangedEventArgs.RemovedItems should have contained the single item \"2\"");
                }
                else
                {
                    Verify.IsTrue(false, "OnSelectionChangedEvent: Shouldn't have reached here, test needs to be updated");
                }
            }
        }
    }
}
