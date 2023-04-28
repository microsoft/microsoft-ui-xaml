﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Controls;

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
    public class ExpanderTests : ApiTestBase
    {
        [TestMethod]
        public void ExpanderAutomationPeerTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = (StackPanel)XamlReader.Load(
    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' 
                             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                             xmlns:primitives='using:Microsoft.UI.Xaml.Controls.Primitives'
                             xmlns:controls='using:Microsoft.UI.Xaml.Controls'> 
                             <controls:Expander x:Name ='ExpandedExpander' AutomationProperties.Name='ExpandedExpander' IsExpanded='True' Margin='12' HorizontalAlignment='Left'>
                                <controls:Expander.Header>
                                    <Grid>
                                        <Grid.ColumnDefinitions>
                                            <ColumnDefinition />
                                            <ColumnDefinition Width='80'/>
                                        </Grid.ColumnDefinitions>
                                        <StackPanel Margin='0,14,0,16'>
                                            <TextBlock AutomationProperties.Name='test' Text='This expander is expanded by default.'  Margin='0,0,0,4' />
                                            <TextBlock Text='This is the second line of text.' />
                                        </StackPanel>
                                        <ToggleSwitch Grid.Column='1'/>
                                    </Grid>
                                </controls:Expander.Header>
                                <Button AutomationProperties.AutomationId = 'ExpandedExpanderContent'> Content </Button>
                             </controls:Expander>
                                    </StackPanel>");

                Content = root;
                Content.UpdateLayout();

                var expander = VisualTreeHelper.GetChild(root, 0) as Expander;

                expander.IsExpanded = true;
                Content.UpdateLayout();

                var grid = VisualTreeHelper.GetChild(expander, 0);
                var toggleButton = VisualTreeHelper.GetChild(grid, 0);
                var toggleButtonGrid = VisualTreeHelper.GetChild(toggleButton, 0);
                var contentPresenter = VisualTreeHelper.GetChild(toggleButtonGrid, 0);
                var grid2 = VisualTreeHelper.GetChild(contentPresenter, 0);
                var stackPanel = VisualTreeHelper.GetChild(grid2, 0);
                var textBlock1 = VisualTreeHelper.GetChild(stackPanel, 0) as TextBlock;
                var textBlock2 = VisualTreeHelper.GetChild(stackPanel, 1) as TextBlock;
                var toggleSwitch = VisualTreeHelper.GetChild(grid2, 1) as ToggleSwitch;

                var border = VisualTreeHelper.GetChild(grid, 1);
                var expanderContentBorder = VisualTreeHelper.GetChild(border, 0);
                var expanderContentContentPresenter = VisualTreeHelper.GetChild(expanderContentBorder, 0);
                var button = VisualTreeHelper.GetChild(expanderContentContentPresenter, 0) as Button;

                Verify.AreEqual("ExpandedExpander", AutomationProperties.GetName(expander));

                // Verify ExpandedExpander header content are included in the accessibility tree
                Verify.AreEqual(AutomationProperties.GetAccessibilityView(textBlock1), AccessibilityView.Content);
                Verify.AreEqual(AutomationProperties.GetAccessibilityView(textBlock2), AccessibilityView.Content);
                Verify.AreEqual(AutomationProperties.GetAccessibilityView(toggleSwitch), AccessibilityView.Content);

                // Verify ExpandedExpander content is included in the accessibility tree
                Verify.AreEqual(AutomationProperties.GetAccessibilityView(button), AccessibilityView.Content);

                expander.IsExpanded = false;
                Content.UpdateLayout();

                // Verify ExpandedExpander content is not included in the accessibility tree and not readable once collapsed
                Verify.AreNotEqual(AutomationProperties.GetAccessibilityView(button), AccessibilityView.Raw);
            });
        }
    }
}