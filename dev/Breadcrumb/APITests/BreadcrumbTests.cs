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
        public void VerifyCustomItemTemplate()
        {
            Breadcrumb breadcrumb = null;
            Breadcrumb breadcrumb2 = null;
            RunOnUIThread.Execute(() =>
            {
                breadcrumb = new Breadcrumb();
                breadcrumb.ItemsSource = new List<int>() { 1, 2 };

                // Set a custom ItemTemplate to be wrapped in a BreadcrumbItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}'/>
                        </DataTemplate>");

                breadcrumb.ItemTemplate = itemTemplate;

                breadcrumb2 = new Breadcrumb();
                breadcrumb2.ItemsSource = new List<int>() { 1, 2 };

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
                ItemsRepeater breadcrumbItemRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("BreadcrumbItemRepeater");
                ItemsRepeater breadcrumbItemRepeater2 = (ItemsRepeater)breadcrumb2.FindVisualChildByName("BreadcrumbItemRepeater");
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
    }
}
