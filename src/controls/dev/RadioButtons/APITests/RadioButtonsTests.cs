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
        public void VerifyVirtualizingTemplateDoesNotRetainRecycledContainers()
        {
            RadioButtons radioButtons = null;
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            int clearingCount = 0;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Creating virtualizing RadioButtons template.");
                var virtualizingTemplate = (ControlTemplate)XamlReader.Load(TestUtilities.ProcessTestXamlForRepo(
                    @"<ControlTemplate
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                        TargetType='controls:RadioButtons'>
                        <ScrollViewer Height='120'>
                            <controls:ItemsRepeater x:Name='InnerRepeater'>
                                <controls:ItemsRepeater.Layout>
                                    <controls:StackLayout />
                                </controls:ItemsRepeater.Layout>
                            </controls:ItemsRepeater>
                        </ScrollViewer>
                    </ControlTemplate>"));

                Log.Comment("Creating RadioButtons instance.");
                radioButtons = new RadioButtons
                {
                    ItemsSource = Enumerable.Range(0, 100).Select(index => $"Option {index}").ToList(),
                    SelectedIndex = 0,
                    Template = virtualizingTemplate
                };

                Log.Comment("Applying RadioButtons template.");
                Content = radioButtons;
                Content.UpdateLayout();

                Log.Comment("Finding virtualizing template descendants.");
                scrollViewer = radioButtons.FindVisualChildByType<ScrollViewer>();
                repeater = radioButtons.FindVisualChildByType<ItemsRepeater>();
                Verify.IsNotNull(scrollViewer);
                Verify.IsNotNull(repeater);

                repeater.ElementClearing += (sender, args) => clearingCount++;
            });

            IdleSynchronizer.Wait();

            void Scroll(bool toEnd)
            {
                RunOnUIThread.Execute(() =>
                {
                    scrollViewer.ChangeView(null, toEnd ? scrollViewer.ScrollableHeight : 0, null, true);
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();
            }

            Scroll(true);
            Scroll(false);

            int baselineChildCount = 0;
            RunOnUIThread.Execute(() =>
            {
                baselineChildCount = VisualTreeHelper.GetChildrenCount(repeater);
                Verify.IsLessThan(baselineChildCount, 100);
            });

            for (int iteration = 0; iteration < 5; iteration++)
            {
                Scroll(true);
                Scroll(false);
            }

            RunOnUIThread.Execute(() =>
            {
                int finalChildCount = VisualTreeHelper.GetChildrenCount(repeater);
                Verify.IsGreaterThan(clearingCount, 0);
                Verify.AreEqual(baselineChildCount, finalChildCount);

                var selectedContainer = radioButtons.ContainerFromIndex(0) as RadioButton;
                Verify.IsNotNull(selectedContainer);
                Verify.AreEqual("Option 0", selectedContainer.Content);
            });
        }

    }
}
