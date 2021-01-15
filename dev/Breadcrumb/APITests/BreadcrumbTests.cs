// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;

using Windows.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;
using Common;
using Windows.UI.Xaml.Markup;
using System.Collections.Generic;
using Windows.UI.Xaml.Media;
using System.Security.Cryptography.X509Certificates;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

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
    public class BreadcrumbTests : ApiTestBase
    {

        [TestMethod]
        public void VerifyDefaultBreadcrumb()
        {
            Breadcrumb breadcrumb = null;
            RunOnUIThread.Execute(() =>
            {
                breadcrumb = new Breadcrumb();
                var stackPanel = new StackPanel();
                stackPanel.Children.Add(breadcrumb);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater breadcrumbItemRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_BreadcrumbItemsRepeater");
                Verify.IsNotNull(breadcrumbItemRepeater, "The underlying items repeater could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemRepeater.TryGetElement(1);
                Verify.IsNull(breadcrumbNode1, "There should be no items.");
            });
        }

        [TestMethod]
        public void VerifyCustomItemTemplate()
        {
            Breadcrumb breadcrumb = null;
            Breadcrumb breadcrumb2 = null;
            RunOnUIThread.Execute(() =>
            {
                breadcrumb = new Breadcrumb();
                breadcrumb.ItemsSource = new List<string>() { "Node 1", "Node 2" };

                // Set a custom ItemTemplate to be wrapped in a BreadcrumbItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}'/>
                        </DataTemplate>");

                breadcrumb.ItemTemplate = itemTemplate;

                breadcrumb2 = new Breadcrumb();
                breadcrumb2.ItemsSource = new List<string>() { "Node 1", "Node 2" };

                // Set a custom ItemTemplate which is already a BreadcrumbItem. No wrapping should be performed.
                var itemTemplate2 = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                            <controls:BreadcrumbItem Foreground='Blue'>
                              <TextBlock Text = '{Binding}'/>
                            </controls:BreadcrumbItem>
                        </DataTemplate>");       

                breadcrumb2.ItemTemplate = itemTemplate2;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(breadcrumb);
                stackPanel.Children.Add(breadcrumb2);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater breadcrumbItemRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_BreadcrumbItemsRepeater");
                ItemsRepeater breadcrumbItemRepeater2 = (ItemsRepeater)breadcrumb2.FindVisualChildByName("PART_BreadcrumbItemsRepeater");
                Verify.IsNotNull(breadcrumbItemRepeater, "The underlying items repeater could not be retrieved");
                Verify.IsNotNull(breadcrumbItemRepeater2, "The underlying items repeater could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemRepeater.TryGetElement(1) as BreadcrumbItem;
                var breadcrumbNode2 = breadcrumbItemRepeater2.TryGetElement(1) as BreadcrumbItem;
                Verify.IsNotNull(breadcrumbNode1, "Our custom ItemTemplate should have been wrapped in a BreadcrumbItem.");
                Verify.IsNotNull(breadcrumbNode2, "Our custom ItemTemplate should have been wrapped in a BreadcrumbItem.");

                // change this conditions
                bool testCondition = !(breadcrumbNode1.Foreground is SolidColorBrush brush && brush.Color == Colors.Blue);
                Verify.IsTrue(testCondition, "Default foreground color of the BreadcrumbItem should not have been [blue].");

                testCondition = breadcrumbNode2.Foreground is SolidColorBrush brush2 && brush2.Color == Colors.Blue;
                Verify.IsTrue(testCondition, "The foreground color of the BreadcrumbItem should have been [blue].");
            });
        }

        [TestMethod]
        public void VerifyNumericItemsSource()
        {
            Breadcrumb breadcrumb = null;
            Breadcrumb breadcrumb2 = null;
            RunOnUIThread.Execute(() =>
            {
                // Set a custom ItemTemplate to be wrapped in a BreadcrumbItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}'/>
                        </DataTemplate>");

                breadcrumb = new Breadcrumb();
                breadcrumb.ItemsSource = new List<int>() { 1, 2 };
                breadcrumb.ItemTemplate = itemTemplate;

                breadcrumb2 = new Breadcrumb();
                breadcrumb2.ItemsSource = new List<float>() { 1.4f, 4.5f };
                breadcrumb2.ItemTemplate = itemTemplate;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(breadcrumb);
                stackPanel.Children.Add(breadcrumb2);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater breadcrumbItemRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_BreadcrumbItemsRepeater");
                Verify.IsNotNull(breadcrumbItemRepeater, "The underlying items repeater could not be retrieved");

                ItemsRepeater breadcrumbItemRepeater2 = (ItemsRepeater)breadcrumb2.FindVisualChildByName("PART_BreadcrumbItemsRepeater");
                Verify.IsNotNull(breadcrumbItemRepeater2, "The underlying items repeater could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemRepeater.TryGetElement(1) as BreadcrumbItem;
                var breadcrumbNode2 = breadcrumbItemRepeater2.TryGetElement(1) as BreadcrumbItem;
                Verify.IsNotNull(breadcrumbNode1, "Our custom ItemTemplate should have been wrapped in a BreadcrumbItem.");
                Verify.IsNotNull(breadcrumbNode2, "Our custom ItemTemplate should have been wrapped in a BreadcrumbItem.");
            });
        }

        class MockClass
        {
            public string MockProperty { get; set; }

            public MockClass()
            {
            }
        };

        [TestMethod]
        public void VerifyObjectItemsSource()
        {
            Breadcrumb breadcrumb = null;
            RunOnUIThread.Execute(() =>
            {
                // Set a custom ItemTemplate to be wrapped in a BreadcrumbItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                            xmlns:local='using:Windows.UI.Xaml.Tests.MUXControls.ApiTests'>
                            <controls:BreadcrumbItem Content='{Binding}'>
                                <controls:BreadcrumbItem.ContentTemplate>
                                    <DataTemplate>
                                        <TextBlock Text='{Binding MockProperty}'/>
                                    </DataTemplate>
                                </controls:BreadcrumbItem.ContentTemplate>
                            </controls:BreadcrumbItem>
                        </DataTemplate>");
              

                breadcrumb = new Breadcrumb();
                breadcrumb.ItemsSource = new List<MockClass>() { 
                    new MockClass { MockProperty = "Node 1" }, 
                    new MockClass { MockProperty = "Node 2" },
                };
                breadcrumb.ItemTemplate = itemTemplate;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(breadcrumb);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater breadcrumbItemRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_BreadcrumbItemsRepeater");
                Verify.IsNotNull(breadcrumbItemRepeater, "The underlying items repeater could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemRepeater.TryGetElement(1) as BreadcrumbItem;
                Verify.IsNotNull(breadcrumbNode1, "Our custom ItemTemplate should have been wrapped in a BreadcrumbItem.");
            });
        }

        [TestMethod]
        public void VerifyDropdownItemTemplate()
        {
            Breadcrumb breadcrumb = null;

            RunOnUIThread.Execute(() =>
            {
                breadcrumb = new Breadcrumb();
                breadcrumb.ItemsSource = new List<string>() { "Node 1", "Node 2" };

                // Set a custom ItemTemplate to be wrapped in a BreadcrumbItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}'/>
                        </DataTemplate>");

                breadcrumb.DropdownItemTemplate = itemTemplate;

                var stackPanel = new StackPanel();
                stackPanel.Width = 130;
                stackPanel.Children.Add(breadcrumb);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            Button ellipsisButton = null;
            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater breadcrumbItemRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_BreadcrumbItemsRepeater");
                Verify.IsNotNull(breadcrumbItemRepeater, "The underlying items repeater (1) could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemRepeater.TryGetElement(0) as BreadcrumbItem;
                Verify.IsNotNull(breadcrumbNode1, "Our custom ItemTemplate (1) should have been wrapped in a BreadcrumbItem.");

                ellipsisButton = (Button)breadcrumbNode1.FindVisualChildByName("PART_BreadcrumbItemButton");
                Verify.IsNotNull(ellipsisButton, "The ellipsis item (1) could not be retrieved");

                var automationPeer = new ButtonAutomationPeer(ellipsisButton);
                var invokationPattern = automationPeer.GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                invokationPattern?.Invoke();                          
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Flyout ellipsisFlyout = (Flyout)ellipsisButton.Flyout;
                Verify.IsNotNull(ellipsisButton, "The ellipsis flyout (1) could not be retrieved");

                ItemsRepeater ellipsisItemsRepeater = (ItemsRepeater)ellipsisFlyout.Content;
                Verify.IsNotNull(ellipsisItemsRepeater, "The underlying flyout items repeater (1) could not be retrieved");

                ellipsisItemsRepeater.Loaded += (object sender, RoutedEventArgs e) => {
                    TextBlock ellipsisNode1 = ellipsisItemsRepeater.TryGetElement(0) as TextBlock;
                    Verify.IsNotNull(ellipsisNode1, "Our flyout ItemTemplate (1) should have been wrapped in a TextBlock.");

                    // change this conditions
                    bool testCondition = !(ellipsisNode1.Foreground is SolidColorBrush brush && brush.Color == Colors.Blue);
                    Verify.IsTrue(testCondition, "Default foreground color of the BreadcrumbItem should not have been [blue].");
                };
            });
        }

    }
}
