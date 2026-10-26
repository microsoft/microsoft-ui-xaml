// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Common;
using Microsoft.UI.Xaml.Markup;
using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Media;
using System.Security.Cryptography.X509Certificates;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using System.Linq;
using Windows.Foundation;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{

    [TestClass]
    public partial class BreadcrumbTests : ApiTestBase
    {

        [TestMethod]
        public void VerifyBreadcrumbDefaultAPIValues()
        {
            RunOnUIThread.Execute(() =>
            {
                BreadcrumbBar breadcrumb = new BreadcrumbBar();
                var stackPanel = new StackPanel();
                stackPanel.Children.Add(breadcrumb);

                Content = stackPanel;
                Content.UpdateLayout();

                Verify.IsNull(breadcrumb.ItemsSource, "The default ItemsSource property value must be null");
                Verify.IsNull(breadcrumb.ItemTemplate, "The default ItemTemplate property value must be null");
            });
        }

        [TestMethod]
        public void VerifyDefaultBreadcrumb()
        {
            BreadcrumbBar breadcrumb = null;
            RunOnUIThread.Execute(() =>
            {
                breadcrumb = new BreadcrumbBar();
                var stackPanel = new StackPanel();
                stackPanel.Children.Add(breadcrumb);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater breadcrumbItemsRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                Verify.IsNotNull(breadcrumbItemsRepeater, "The underlying items repeater could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemsRepeater.TryGetElement(1);
                Verify.IsNull(breadcrumbNode1, "There should be no items.");
            });
        }

        [TestMethod]
        public void VerifyCustomItemTemplate()
        {
            BreadcrumbBar breadcrumb = null;
            BreadcrumbBar breadcrumb2 = null;
            RunOnUIThread.Execute(() =>
            {
                breadcrumb = new BreadcrumbBar();
                breadcrumb.ItemsSource = new List<string>() { "Node 1", "Node 2" };

                // Set a custom ItemTemplate to be wrapped in a BreadcrumbBarItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}'/>
                        </DataTemplate>");

                breadcrumb.ItemTemplate = itemTemplate;

                breadcrumb2 = new BreadcrumbBar();
                breadcrumb2.ItemsSource = new List<string>() { "Node 1", "Node 2" };

                // Set a custom ItemTemplate which is already a BreadcrumbBarItem. No wrapping should be performed.
                var itemTemplate2 = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                            <controls:BreadcrumbBarItem Foreground='Blue'>
                              <TextBlock Text = '{Binding}'/>
                            </controls:BreadcrumbBarItem>
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
                ItemsRepeater breadcrumbItemsRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                ItemsRepeater breadcrumbItemsRepeater2 = (ItemsRepeater)breadcrumb2.FindVisualChildByName("PART_ItemsRepeater");
                Verify.IsNotNull(breadcrumbItemsRepeater, "The underlying items repeater could not be retrieved");
                Verify.IsNotNull(breadcrumbItemsRepeater2, "The underlying items repeater could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemsRepeater.TryGetElement(1) as BreadcrumbBarItem;
                var breadcrumbNode2 = breadcrumbItemsRepeater2.TryGetElement(1) as BreadcrumbBarItem;
                Verify.IsNotNull(breadcrumbNode1, "Our custom ItemTemplate should have been wrapped in a BreadcrumbBarItem.");
                Verify.IsNotNull(breadcrumbNode2, "Our custom ItemTemplate should have been wrapped in a BreadcrumbBarItem.");

                // change this conditions
                bool testCondition = !(breadcrumbNode1.Foreground is SolidColorBrush brush && brush.Color == Colors.Blue);
                Verify.IsTrue(testCondition, "Default foreground color of the BreadcrumbBarItem should not have been [blue].");

                testCondition = breadcrumbNode2.Foreground is SolidColorBrush brush2 && brush2.Color == Colors.Blue;
                Verify.IsTrue(testCondition, "The foreground color of the BreadcrumbBarItem should have been [blue].");
            });
        }

        [TestMethod]
        public void VerifyNumericItemsSource()
        {
            BreadcrumbBar breadcrumb = null;
            BreadcrumbBar breadcrumb2 = null;
            RunOnUIThread.Execute(() =>
            {
                // Set a custom ItemTemplate to be wrapped in a BreadcrumbBarItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}'/>
                        </DataTemplate>");

                breadcrumb = new BreadcrumbBar();
                breadcrumb.ItemsSource = new List<int>() { 1, 2 };
                breadcrumb.ItemTemplate = itemTemplate;

                breadcrumb2 = new BreadcrumbBar();
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
                ItemsRepeater breadcrumbItemsRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                Verify.IsNotNull(breadcrumbItemsRepeater, "The underlying items repeater could not be retrieved");

                ItemsRepeater breadcrumbItemsRepeater2 = (ItemsRepeater)breadcrumb2.FindVisualChildByName("PART_ItemsRepeater");
                Verify.IsNotNull(breadcrumbItemsRepeater2, "The underlying items repeater could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemsRepeater.TryGetElement(1) as BreadcrumbBarItem;
                var breadcrumbNode2 = breadcrumbItemsRepeater2.TryGetElement(1) as BreadcrumbBarItem;
                Verify.IsNotNull(breadcrumbNode1, "Our custom ItemTemplate should have been wrapped in a BreadcrumbBarItem.");
                Verify.IsNotNull(breadcrumbNode2, "Our custom ItemTemplate should have been wrapped in a BreadcrumbBarItem.");
            });
        }

        [TestMethod]
        [TestProperty("Data:XamlOptionalChanges", "{DefaultStyleOptimizations:false,DefaultStyleOptimizations:true}")]
        public void VerifyLongBreadcrumbTrimsWithinRemainingWidth()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var flowDirection in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
                {
                    var breadcrumb = CreateTextBreadcrumb(new[] { "Root", "Parent", new string('W', 100) });
                    breadcrumb.FlowDirection = flowDirection;
                    breadcrumb.Width = 2000;
                    Content = breadcrumb;
                    Content.UpdateLayout();

                    var repeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                    var ellipsis = (BreadcrumbBarItem)repeater.TryGetElement(0);
                    var lastItem = (BreadcrumbBarItem)repeater.TryGetElement(3);
                    var presenter = (ContentPresenter)lastItem.FindVisualChildByName("PART_LastItemContentPresenter");
                    var text = (TextBlock)presenter.FindVisualChildByName("CrumbText");
                    double naturalWidth = lastItem.DesiredSize.Width;

                    Verify.IsFalse(text.IsTextTrimmed);
                    Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot(ellipsis).Width);

                    breadcrumb.Width = 200;
                    Content.UpdateLayout();

                    var ellipsisSlot = LayoutInformation.GetLayoutSlot(ellipsis);
                    var lastSlot = LayoutInformation.GetLayoutSlot(lastItem);
                    Verify.IsGreaterThan(ellipsisSlot.Width, 0.0);
                    Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot((FrameworkElement)repeater.TryGetElement(1)).Width);
                    Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot((FrameworkElement)repeater.TryGetElement(2)).Width);
                    Verify.IsTrue(lastSlot.Width <= breadcrumb.ActualWidth - ellipsisSlot.Width + 1);
                    Verify.IsTrue(lastSlot.X + lastSlot.Width <= breadcrumb.ActualWidth + 1);
                    Verify.IsTrue(text.IsTextTrimmed);
                    VerifyTextIsInsideBreadcrumb(text, repeater);

                    breadcrumb.Width = 2000;
                    Content.UpdateLayout();

                    Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot(ellipsis).Width);
                    Verify.AreEqual(naturalWidth, lastItem.DesiredSize.Width);
                    Verify.IsFalse(text.IsTextTrimmed);
                    Verify.IsGreaterThan(LayoutInformation.GetLayoutSlot((FrameworkElement)repeater.TryGetElement(1)).Width, 0.0);

                    breadcrumb.Width = 200;
                    Content.UpdateLayout();
                    Verify.AreEqual(lastSlot, LayoutInformation.GetLayoutSlot(lastItem));

                    breadcrumb.Width = ellipsisSlot.Width / 2;
                    Content.UpdateLayout();
                    double pixelWidth = 1 / breadcrumb.XamlRoot.RasterizationScale;
                    Verify.IsTrue(LayoutInformation.GetLayoutSlot(lastItem).Width <= pixelWidth + 0.001,
                        "A tiny bar may leave at most one physical pixel after layout rounding.");

                    breadcrumb.Width = 0;
                    Content.UpdateLayout();
                    Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot(lastItem).Width);
                }
            });
        }

        [TestMethod]
        [TestProperty("Data:XamlOptionalChanges", "{DefaultStyleOptimizations:false,DefaultStyleOptimizations:true}")]
        public void VerifyOverflowPreservesFullyVisibleBreadcrumbs()
        {
            RunOnUIThread.Execute(() =>
            {
                var breadcrumb = CreateTextBreadcrumb(new[] { new string('W', 100), "Parent", "Current" });
                breadcrumb.Width = 2000;
                Content = breadcrumb;
                Content.UpdateLayout();

                var repeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                var ellipsis = (BreadcrumbBarItem)repeater.TryGetElement(0);
                var parentItem = (BreadcrumbBarItem)repeater.TryGetElement(2);
                var lastItem = (BreadcrumbBarItem)repeater.TryGetElement(3);
                double parentWidth = parentItem.DesiredSize.Width;
                double lastWidth = lastItem.DesiredSize.Width;

                breadcrumb.Width = ellipsis.DesiredSize.Width + parentWidth + lastWidth + 1;
                Content.UpdateLayout();

                Verify.IsGreaterThan(LayoutInformation.GetLayoutSlot(ellipsis).Width, 0.0);
                Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot((FrameworkElement)repeater.TryGetElement(1)).Width);
                Verify.AreEqual(parentWidth, LayoutInformation.GetLayoutSlot(parentItem).Width);
                Verify.AreEqual(lastWidth, LayoutInformation.GetLayoutSlot(lastItem).Width);
                var lastSlot = LayoutInformation.GetLayoutSlot(lastItem);
                Verify.IsTrue(lastSlot.X + lastSlot.Width <= breadcrumb.ActualWidth + 1);

                var parentPresenter = (ContentPresenter)parentItem.FindVisualChildByName("PART_ItemContentPresenter");
                var lastPresenter = (ContentPresenter)lastItem.FindVisualChildByName("PART_LastItemContentPresenter");
                var parentText = (TextBlock)parentPresenter.FindVisualChildByName("CrumbText");
                var lastText = (TextBlock)lastPresenter.FindVisualChildByName("CrumbText");
                Verify.IsFalse(parentText.IsTextTrimmed);
                Verify.IsFalse(lastText.IsTextTrimmed);
                VerifyTextIsInsideBreadcrumb(parentText, repeater);
                VerifyTextIsInsideBreadcrumb(lastText, repeater);
            });
        }

        [TestMethod]
        [TestProperty("Data:XamlOptionalChanges", "{DefaultStyleOptimizations:false,DefaultStyleOptimizations:true}")]
        public void VerifySingleBreadcrumbTrimsWithoutOverflow()
        {
            RunOnUIThread.Execute(() =>
            {
                var breadcrumb = CreateTextBreadcrumb(new[] { new string('W', 100) });
                breadcrumb.Width = 200;
                var stackPanel = new StackPanel();
                stackPanel.Children.Add(breadcrumb);
                Content = stackPanel;
                Content.UpdateLayout();

                var repeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                var ellipsis = (BreadcrumbBarItem)repeater.TryGetElement(0);
                var lastItem = (BreadcrumbBarItem)repeater.TryGetElement(1);
                var presenter = (ContentPresenter)lastItem.FindVisualChildByName("PART_LastItemContentPresenter");
                var text = (TextBlock)presenter.FindVisualChildByName("CrumbText");
                Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot(ellipsis).Width);
                Verify.IsTrue(text.IsTextTrimmed);
                VerifyTextIsInsideBreadcrumb(text, repeater);

                breadcrumb.ClearValue(FrameworkElement.WidthProperty);
                stackPanel.Orientation = Orientation.Horizontal;
                Content.UpdateLayout();

                Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot(ellipsis).Width);
                Verify.IsFalse(text.IsTextTrimmed);
            });
        }

        [TestMethod]
        [TestProperty("Data:XamlOptionalChanges", "{DefaultStyleOptimizations:false,DefaultStyleOptimizations:true}")]
        public void VerifyVisibleAncestorIsConstrained()
        {
            RunOnUIThread.Execute(() =>
            {
                var breadcrumb = new BreadcrumbBar
                {
                    Width = 200,
                    ItemsSource = new[] { new string('W', 100), string.Empty },
                    ItemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                            <controls:BreadcrumbBarItem Content='{Binding}'>
                                <controls:BreadcrumbBarItem.Template>
                                    <ControlTemplate TargetType='controls:BreadcrumbBarItem'>
                                        <ContentPresenter Content='{TemplateBinding Content}'>
                                            <ContentPresenter.ContentTemplate>
                                                <DataTemplate>
                                                    <TextBlock x:Name='CrumbText' Text='{Binding}' TextTrimming='CharacterEllipsis' IsTextScaleFactorEnabled='False'/>
                                                </DataTemplate>
                                            </ContentPresenter.ContentTemplate>
                                        </ContentPresenter>
                                    </ControlTemplate>
                                </controls:BreadcrumbBarItem.Template>
                            </controls:BreadcrumbBarItem>
                        </DataTemplate>")
                };
                Content = breadcrumb;
                Content.UpdateLayout();

                var repeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                var parentItem = (BreadcrumbBarItem)repeater.TryGetElement(1);
                var text = (TextBlock)parentItem.FindVisualChildByName("CrumbText");
                Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot((FrameworkElement)repeater.TryGetElement(0)).Width);
                Verify.IsTrue(LayoutInformation.GetLayoutSlot(parentItem).Width <= breadcrumb.ActualWidth + 1);
                Verify.IsTrue(text.IsTextTrimmed);
                VerifyTextIsInsideBreadcrumb(text, repeater);
            });
        }

        [TestMethod]
        [TestProperty("Data:XamlOptionalChanges", "{DefaultStyleOptimizations:false,DefaultStyleOptimizations:true}")]
        public void VerifyConstrainedBreadcrumbUpdatesDesiredHeight()
        {
            RunOnUIThread.Execute(() =>
            {
                var breadcrumb = new BreadcrumbBar
                {
                    Width = 2000,
                    ItemsSource = new[] { "Root", string.Join(" ", Enumerable.Repeat("Folder", 30)) },
                    ItemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}' TextWrapping='Wrap' FontSize='14' IsTextScaleFactorEnabled='False'/>
                        </DataTemplate>")
                };
                Content = breadcrumb;
                Content.UpdateLayout();

                var repeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                var lastItem = (BreadcrumbBarItem)repeater.TryGetElement(2);
                double naturalHeight = lastItem.DesiredSize.Height;

                breadcrumb.Width = 200;
                Content.UpdateLayout();

                Verify.IsGreaterThan(lastItem.DesiredSize.Height, naturalHeight);
                Verify.IsTrue(breadcrumb.DesiredSize.Height >= lastItem.DesiredSize.Height);
            });
        }

        [TestMethod]
        [TestProperty("Data:XamlOptionalChanges", "{DefaultStyleOptimizations:false,DefaultStyleOptimizations:true}")]
        public void VerifyHiddenBreadcrumbDoesNotWrap()
        {
            RunOnUIThread.Execute(() =>
            {
                var breadcrumb = new BreadcrumbBar
                {
                    Width = 2000,
                    ItemsSource = new[] { string.Join(" ", Enumerable.Repeat("Ancestor", 100)), "Current" },
                    ItemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}' TextWrapping='Wrap' FontSize='14' IsTextScaleFactorEnabled='False'/>
                        </DataTemplate>")
                };
                Content = breadcrumb;
                Content.UpdateLayout();
                double naturalHeight = breadcrumb.DesiredSize.Height;

                breadcrumb.Width = 200;
                Content.UpdateLayout();

                var repeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                Verify.AreEqual(0.0, LayoutInformation.GetLayoutSlot((FrameworkElement)repeater.TryGetElement(1)).Width);
                Verify.AreEqual(naturalHeight, breadcrumb.DesiredSize.Height);
            });
        }

        [TestMethod]
        [TestProperty("Data:XamlOptionalChanges", "{DefaultStyleOptimizations:false,DefaultStyleOptimizations:true}")]
        public void VerifyBreadcrumbMeasureCachingDuringResize()
        {
            BreadcrumbBar breadcrumb = null;
            ItemsRepeater repeater = null;
            MeasureCountingElement rootItem = null;
            MeasureCountingElement parentItem = null;
            MeasureCountingElement lastItem = null;

            RunOnUIThread.Execute(() =>
            {
                breadcrumb = new BreadcrumbBar
                {
                    Width = 10000,
                    ItemsSource = new[] { "Root", "Parent", "Current" }
                };
                Content = breadcrumb;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                repeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                rootItem = new MeasureCountingElement(50);
                parentItem = new MeasureCountingElement(60);
                lastItem = new MeasureCountingElement(500);
                ((BreadcrumbBarItem)repeater.TryGetElement(1)).Content = rootItem;
                ((BreadcrumbBarItem)repeater.TryGetElement(2)).Content = parentItem;
                ((BreadcrumbBarItem)repeater.TryGetElement(3)).Content = lastItem;
                Content.UpdateLayout();

                int rootMeasureCount = rootItem.MeasureCount;
                int parentMeasureCount = parentItem.MeasureCount;
                int lastMeasureCount = lastItem.MeasureCount;

                breadcrumb.Width = 200;
                Content.UpdateLayout();

                Verify.AreEqual(rootMeasureCount, rootItem.MeasureCount, "An unchanged ancestor should reuse its natural measurement.");
                Verify.AreEqual(parentMeasureCount, parentItem.MeasureCount, "An unchanged ancestor should reuse its natural measurement.");
                Verify.AreEqual(lastMeasureCount + 1, lastItem.MeasureCount, "The first shrink should only add the constrained measurement.");

                foreach (double width in new[] { 180.0, 160.0, 140.0 })
                {
                    lastItem.MeasureCount = 0;
                    breadcrumb.Width = width;
                    Content.UpdateLayout();

                    Verify.AreEqual(rootMeasureCount, rootItem.MeasureCount);
                    Verify.AreEqual(parentMeasureCount, parentItem.MeasureCount);
                    Verify.AreEqual(2, lastItem.MeasureCount, "Only the squeezed item needs natural and constrained measurements.");
                }

                breadcrumb.Width = 0;
                Content.UpdateLayout();
                rootItem.MeasureCount = 0;
                parentItem.MeasureCount = 0;
                lastItem.MeasureCount = 0;
                repeater.InvalidateMeasure();
                Content.UpdateLayout();

                Verify.AreEqual(0, rootItem.MeasureCount, "Repeated zero-width layout should reuse cached measurements.");
                Verify.AreEqual(0, parentItem.MeasureCount);
                Verify.AreEqual(0, lastItem.MeasureCount);
            });
        }

        private partial class MeasureCountingElement : FrameworkElement
        {
            private readonly double _naturalWidth;

            public MeasureCountingElement(double naturalWidth)
            {
                _naturalWidth = naturalWidth;
            }

            public int MeasureCount { get; set; }

            protected override Size MeasureOverride(Size availableSize)
            {
                ++MeasureCount;
                return new Size(Math.Min(_naturalWidth, availableSize.Width), 20);
            }
        }

        private static BreadcrumbBar CreateTextBreadcrumb(IEnumerable<string> items)
        {
            return new BreadcrumbBar
            {
                ItemsSource = items,
                ItemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <TextBlock x:Name='CrumbText' Text='{Binding}' TextTrimming='CharacterEllipsis' FontSize='14' IsTextScaleFactorEnabled='False'/>
                    </DataTemplate>")
            };
        }

        private static void VerifyTextIsInsideBreadcrumb(TextBlock text, ItemsRepeater repeater)
        {
            var bounds = text.TransformToVisual(repeater).TransformBounds(new Rect(0, 0, text.ActualWidth, text.ActualHeight));
            Verify.IsTrue(bounds.X >= -1);
            Verify.IsTrue(bounds.X + bounds.Width <= repeater.ActualWidth + 1);
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
            BreadcrumbBar breadcrumb = null;
            RunOnUIThread.Execute(() =>
            {
                // Set a custom ItemTemplate to be wrapped in a BreadcrumbBarItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                            xmlns:local='using:Microsoft.UI.Xaml.Tests.MUXControls.ApiTests'>
                            <controls:BreadcrumbBarItem Content='{Binding}'>
                                <controls:BreadcrumbBarItem.ContentTemplate>
                                    <DataTemplate>
                                        <TextBlock Text='{Binding MockProperty}'/>
                                    </DataTemplate>
                                </controls:BreadcrumbBarItem.ContentTemplate>
                            </controls:BreadcrumbBarItem>
                        </DataTemplate>");
              

                breadcrumb = new BreadcrumbBar();
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
                ItemsRepeater breadcrumbItemsRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                Verify.IsNotNull(breadcrumbItemsRepeater, "The underlying items repeater could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemsRepeater.TryGetElement(1) as BreadcrumbBarItem;
                Verify.IsNotNull(breadcrumbNode1, "Our custom ItemTemplate should have been wrapped in a BreadcrumbBarItem.");
            });
        }

        [TestMethod]
        public void VerifyDropdownItemTemplate()
        {
            BreadcrumbBar breadcrumb = null;

            RunOnUIThread.Execute(() =>
            {
                breadcrumb = new BreadcrumbBar();
                breadcrumb.ItemsSource = new List<MockClass>() {
                    new MockClass { MockProperty = "Node 1" },
                    new MockClass { MockProperty = "Node 2" },
                };

                // Set a custom ItemTemplate to be wrapped in a BreadcrumbBarItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                            xmlns:local='using:Microsoft.UI.Xaml.Tests.MUXControls.ApiTests'>
                            <controls:BreadcrumbBarItem Content='{Binding}'>
                                <controls:BreadcrumbBarItem.ContentTemplate>
                                    <DataTemplate>
                                        <TextBlock Text='{Binding MockProperty}'/>
                                    </DataTemplate>
                                </controls:BreadcrumbBarItem.ContentTemplate>
                            </controls:BreadcrumbBarItem>
                        </DataTemplate>");

                breadcrumb.ItemTemplate = itemTemplate;

                var stackPanel = new StackPanel();
                stackPanel.Width = 60;
                stackPanel.Children.Add(breadcrumb);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            Button ellipsisButton = null;
            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater breadcrumbItemsRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                Verify.IsNotNull(breadcrumbItemsRepeater, "The underlying items repeater (1) could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemsRepeater.TryGetElement(0) as BreadcrumbBarItem;
                Verify.IsNotNull(breadcrumbNode1, "Our custom ItemTemplate (1) should have been wrapped in a BreadcrumbBarItem.");

                ellipsisButton = (Button)breadcrumbNode1.FindVisualChildByName("PART_ItemButton");
                Verify.IsNotNull(ellipsisButton, "The ellipsis item (1) could not be retrieved");

                var automationPeer = new ButtonAutomationPeer(ellipsisButton);
                var invokationPattern = automationPeer.GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                invokationPattern?.Invoke();                          
            });

            // GetOpenPopups returns empty list in RS3. The scenario works, this just seems to be a
            // test/infra issue in RS3, so filtering it out for now.
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var flyout = VisualTreeHelper.GetOpenPopupsForXamlRoot(Content.XamlRoot).Last();
                    Verify.IsNotNull(flyout, "Flyout could not be retrieved");
                    var ellipsisItemsRepeater = TestUtilities.FindDescendents<ItemsRepeater>(flyout).Single();
                    Verify.IsNotNull(ellipsisItemsRepeater, "The underlying flyout items repeater (1) could not be retrieved");

                    ellipsisItemsRepeater.Loaded += (object sender, RoutedEventArgs e) =>
                    {
                        TextBlock ellipsisNode1 = ellipsisItemsRepeater.TryGetElement(0) as TextBlock;
                        Verify.IsNotNull(ellipsisNode1, "Our flyout ItemTemplate (1) should have been wrapped in a TextBlock.");

                    // change this conditions
                    bool testCondition = !(ellipsisNode1.Foreground is SolidColorBrush brush && brush.Color == Colors.Blue);
                        Verify.IsTrue(testCondition, "Default foreground color of the BreadcrumbBarItem should not have been [blue].");
                    };
                });
            }
        }

        [TestMethod]
        public void VerifyDropdownItemTemplateWithNoControl()
        {
            BreadcrumbBar breadcrumb = null;

            RunOnUIThread.Execute(() =>
            {
                breadcrumb = new BreadcrumbBar();
                breadcrumb.ItemsSource = new List<string>() { "Node 1", "Node 2" };

                // Set a custom ItemTemplate to be wrapped in a BreadcrumbBarItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}'/>
                        </DataTemplate>");

                breadcrumb.ItemTemplate = itemTemplate;

                var stackPanel = new StackPanel();
                stackPanel.Width = 60;
                stackPanel.Children.Add(breadcrumb);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            Button ellipsisButton = null;
            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater breadcrumbItemsRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                Verify.IsNotNull(breadcrumbItemsRepeater, "The underlying items repeater (1) could not be retrieved");

                var breadcrumbNode1 = breadcrumbItemsRepeater.TryGetElement(0) as BreadcrumbBarItem;
                Verify.IsNotNull(breadcrumbNode1, "Our custom ItemTemplate (1) should have been wrapped in a BreadcrumbBarItem.");

                ellipsisButton = (Button)breadcrumbNode1.FindVisualChildByName("PART_ItemButton");
                Verify.IsNotNull(ellipsisButton, "The ellipsis item (1) could not be retrieved");

                var automationPeer = new ButtonAutomationPeer(ellipsisButton);
                var invokationPattern = automationPeer.GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                invokationPattern?.Invoke();
            });

            // GetOpenPopups returns empty list in RS3. The scenario works, this just seems to be a
            // test/infra issue in RS3, so filtering it out for now.
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var flyout = VisualTreeHelper.GetOpenPopupsForXamlRoot(Content.XamlRoot).Last();
                    Verify.IsNotNull(flyout, "Flyout could not be retrieved");
                    var ellipsisItemsRepeater = TestUtilities.FindDescendents<ItemsRepeater>(flyout).Single();
                    Verify.IsNotNull(ellipsisItemsRepeater, "The underlying flyout items repeater (1) could not be retrieved");

                    ellipsisItemsRepeater.Loaded += (object sender, RoutedEventArgs e) =>
                    {
                        TextBlock ellipsisNode1 = ellipsisItemsRepeater.TryGetElement(0) as TextBlock;
                        Verify.IsNotNull(ellipsisNode1, "Our flyout ItemTemplate (1) should have been wrapped in a TextBlock.");

                    // change this conditions
                    bool testCondition = !(ellipsisNode1.Foreground is SolidColorBrush brush && brush.Color == Colors.Blue);
                        Verify.IsTrue(testCondition, "Default foreground color of the BreadcrumbBarItem should not have been [blue].");
                    };
                });
            }
        }

        [TestMethod]
        public void VerifyCollectionChangeGetsRespected()
        {
            BreadcrumbBar breadcrumb = null;
            ItemsRepeater breadcrumbItemsRepeater = null;
            RunOnUIThread.Execute(() =>
            {
                // Set a custom ItemTemplate to be wrapped in a BreadcrumbBarItem.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                            xmlns:local='using:Microsoft.UI.Xaml.Tests.MUXControls.ApiTests'>
                            <controls:BreadcrumbBarItem Content='{Binding}'>
                                <controls:BreadcrumbBarItem.ContentTemplate>
                                    <DataTemplate>
                                        <TextBlock Text='{Binding MockProperty}'/>
                                    </DataTemplate>
                                </controls:BreadcrumbBarItem.ContentTemplate>
                            </controls:BreadcrumbBarItem>
                        </DataTemplate>");


                breadcrumb = new BreadcrumbBar();
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
                breadcrumbItemsRepeater = (ItemsRepeater)breadcrumb.FindVisualChildByName("PART_ItemsRepeater");
                Verify.IsNotNull(breadcrumbItemsRepeater, "The underlying items repeater could not be retrieved");

                var breadcrumbNode2 = breadcrumbItemsRepeater.TryGetElement(1) as BreadcrumbBarItem;
                Verify.IsNotNull(breadcrumbNode2, "Our custom ItemTemplate should have been wrapped in a BreadcrumbBarItem.");

                breadcrumb.ItemsSource = new List<MockClass>() {
                    new MockClass { MockProperty = "Node 1" },
                    new MockClass { MockProperty = "Node 2" },
                    new MockClass { MockProperty = "Node 3" },
                    new MockClass { MockProperty = "Node 4" },
                };
            });

            IdleSynchronizer.Wait();


            RunOnUIThread.Execute(() =>
            {
                var breadcrumbNode3 = breadcrumbItemsRepeater.TryGetElement(3) as BreadcrumbBarItem;
                Verify.IsNotNull(breadcrumbNode3, "A fourth item should have been rendered by the BreadcrumControl");
            });
        }
    }
}
