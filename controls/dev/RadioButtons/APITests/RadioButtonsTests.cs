// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;
using Common;
using Microsoft.UI.Xaml.Markup;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Media;
using System.Linq;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
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
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var radioButtonsLayoutRoot = (FrameworkElement)VisualTreeHelper.GetChild(radioButtons, 0);
                commonStatesGroup = VisualStateManager.GetVisualStateGroups(radioButtonsLayoutRoot).First(vsg => vsg.Name.Equals("CommonStates"));

                Verify.AreEqual("Normal", commonStatesGroup.CurrentState.Name);

                // Check 2: Set IsEnabled to false.
                radioButtons.IsEnabled = false;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
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
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Normal", commonStatesGroup.CurrentState.Name);
            });
        }

        [TestMethod]
        public void VerifySelectionChangedArgsDoNotContainNullItems()
        {
            RadioButtons radioButtons = null;
            var selectionChangedArgs = new List<SelectionChangedEventArgs>();

            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons();
                radioButtons.ItemsSource = new List<string>() { "0", "1", "2", "3" };
                radioButtons.SelectionChanged += (s, e) => selectionChangedArgs.Add(e);

                Content = radioButtons;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            // Local helper: verify every captured event so far has no null entries
            // in either collection, then clear the buffer for the next scenario.
            // The args collections are UI-thread objects, so access them on the UI thread.
            void VerifyNoNullItemsAndClear()
            {
                RunOnUIThread.Execute(() =>
                {
                    foreach (var args in selectionChangedArgs)
                    {
                        foreach (var item in args.AddedItems)
                        {
                            Verify.IsNotNull(item, "AddedItems should never contain a null entry.");
                        }
                        foreach (var item in args.RemovedItems)
                        {
                            Verify.IsNotNull(item, "RemovedItems should never contain a null entry.");
                        }
                    }
                });
                selectionChangedArgs.Clear();
            }

            // Scenario 1: first selection (nothing was selected before). RemovedItems
            // must be empty (Count == 0), AddedItems must contain the newly selected item.
            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = 0;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsGreaterThan(selectionChangedArgs.Count, 0, "SelectionChanged should have fired for the first selection.");
                var lastArgs = selectionChangedArgs.Last();
                Verify.AreEqual(0, lastArgs.RemovedItems.Count, "First selection should report no removed items.");
                Verify.AreEqual(1, lastArgs.AddedItems.Count, "First selection should report exactly one added item.");
            });
            VerifyNoNullItemsAndClear();

            // Scenario 2: switching selection to another item. No event should carry a null.
            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = 2;
            });
            IdleSynchronizer.Wait();
            VerifyNoNullItemsAndClear();

            // Scenario 3: out-of-range positive SelectedIndex. GetDataAtIndex returns
            // null for such an index, but the args must not carry a null entry.
            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = 99;
            });
            IdleSynchronizer.Wait();
            VerifyNoNullItemsAndClear();

            // Scenario 4: deselect everything. AddedItems must be empty and no null entries.
            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = 0;
            });
            IdleSynchronizer.Wait();
            selectionChangedArgs.Clear();

            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = -1;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                foreach (var args in selectionChangedArgs)
                {
                    Verify.AreEqual(0, args.AddedItems.Count, "Deselecting should report no added items.");
                }
            });
            VerifyNoNullItemsAndClear();
        }
    }
}
