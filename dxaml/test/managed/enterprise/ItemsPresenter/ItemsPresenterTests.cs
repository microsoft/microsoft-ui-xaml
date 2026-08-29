// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Enterprise.Common;

namespace Microsoft.UI.Xaml.Tests.Controls.ItemsPresenterTests
{
    // The focus of this test suite is ItemsPresenter used
    // with legacy panels.
    [TestClass]
    public class ItemsPresenterTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("__ExecutionUnit", "3d58ff19-7f87-4023-8cdf-dcff2b313f3f")]
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
        [TestProperty("Hosting:Mode", "UAP")]   // DCPP: Test Controls.ItemsPresenterTests.ItemsPresenterTests fails on WPF
        public void VerifyHeaderGetsMaxDesiredSizeOrAvailableSize()
        {
            ListView list = PrepareListView();

            UIExecutor.Execute(() =>
            {
                var header = (FrameworkElement)list.Header;
                Log.Comment("ListView's actual width: ", list.ActualWidth);
                Log.Comment("ListView's header actual width: ", header.ActualWidth);
                Verify.AreNotEqual(header.DesiredSize.Width, header.ActualWidth);
                Verify.AreEqual(header.ActualWidth, list.ActualWidth);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]   // DCPP: Test Controls.ItemsPresenterTests.ItemsPresenterTests fails on WPF
        public void CanNavigateByLineOrPage()
        {
            ListView list = PrepareListView();
            UIExecutor.Execute(() =>
            {
                list.ScrollIntoView(((IList)list.ItemsSource)[0], ScrollIntoViewAlignment.Leading);
                list.UpdateLayout();

                var scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.AreEqual(2, scrollViewer.VerticalOffset);    // logical offsets

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(list);
                var scrollPattern = (IScrollProvider)peer.GetPattern(PatternInterface.Scroll);

                Log.Comment("Invoke ItemsPresenter.LineUp()");
                scrollPattern.Scroll(Microsoft.UI.Xaml.Automation.ScrollAmount.NoAmount, Microsoft.UI.Xaml.Automation.ScrollAmount.SmallDecrement);
                list.UpdateLayout();
                Verify.IsLessThan(scrollViewer.VerticalOffset, 2.0);

                Log.Comment("Invoke ItemsPresenter.LineDown()");
                scrollPattern.Scroll(Microsoft.UI.Xaml.Automation.ScrollAmount.NoAmount, Microsoft.UI.Xaml.Automation.ScrollAmount.SmallIncrement);
                list.UpdateLayout();
                Verify.AreEqual(2.0, scrollViewer.VerticalOffset);

                Log.Comment("Invoke ItemsPresenter.PageUp()");
                scrollPattern.Scroll(Microsoft.UI.Xaml.Automation.ScrollAmount.NoAmount, Microsoft.UI.Xaml.Automation.ScrollAmount.LargeDecrement);
                list.UpdateLayout();
                Verify.AreEqual(1.0, scrollViewer.VerticalOffset);  // Looks odd, but it's 1.0 to account for padding.

                Log.Comment("Invoke ItemsPresenter.PageDown()");
                scrollPattern.Scroll(Microsoft.UI.Xaml.Automation.ScrollAmount.NoAmount, Microsoft.UI.Xaml.Automation.ScrollAmount.LargeIncrement);
                list.UpdateLayout();
                Verify.IsGreaterThan(scrollViewer.VerticalOffset, 2.0);
            });

            list = PrepareListView(true /* isHorizontal */);
            UIExecutor.Execute(() =>
            {
                list.ScrollIntoView(((IList)list.ItemsSource)[0], ScrollIntoViewAlignment.Leading);
                list.UpdateLayout();

                var scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.AreEqual(2, scrollViewer.HorizontalOffset);    // logical offsets

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(list);
                var scrollPattern = (IScrollProvider)peer.GetPattern(PatternInterface.Scroll);

                Log.Comment("Invoke ItemsPresenter.LineLeft()");
                scrollPattern.Scroll(Microsoft.UI.Xaml.Automation.ScrollAmount.SmallDecrement, Microsoft.UI.Xaml.Automation.ScrollAmount.NoAmount);
                list.UpdateLayout();
                Verify.IsLessThan(scrollViewer.HorizontalOffset, 2.0);

                Log.Comment("Invoke ItemsPresenter.LineRight()");
                scrollPattern.Scroll(Microsoft.UI.Xaml.Automation.ScrollAmount.SmallIncrement, Microsoft.UI.Xaml.Automation.ScrollAmount.NoAmount);
                list.UpdateLayout();
                Verify.AreEqual(2.0, scrollViewer.HorizontalOffset);

                Log.Comment("Invoke ItemsPresenter.PageLeft()");
                scrollPattern.Scroll(Microsoft.UI.Xaml.Automation.ScrollAmount.LargeDecrement, Microsoft.UI.Xaml.Automation.ScrollAmount.NoAmount);
                list.UpdateLayout();
                Verify.AreEqual(1.0, scrollViewer.HorizontalOffset);  // Looks odd, but it's 1.0 to account for padding.

                Log.Comment("Invoke ItemsPresenter.PageRight()");
                scrollPattern.Scroll(Microsoft.UI.Xaml.Automation.ScrollAmount.LargeIncrement, Microsoft.UI.Xaml.Automation.ScrollAmount.NoAmount);
                list.UpdateLayout();
                Verify.IsGreaterThan(scrollViewer.HorizontalOffset, 2.0);
            });
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanUseMouseWheelToNavigate()
        {
            ListView list = PrepareListView();
            ScrollViewer scrollViewer = null;
            double lastOffset = 0.0;

            UIExecutor.Execute(() =>
            {
                scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                lastOffset = scrollViewer.VerticalOffset;
            });

            Log.Comment("Invoke ItemsPresenter.MouseWheelDown()");
            TestServices.InputHelper.ScrollMouseWheel(list, -10);
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.IsGreaterThan(scrollViewer.VerticalOffset, lastOffset); // logical offsets
                lastOffset = scrollViewer.VerticalOffset;
            });

            Log.Comment("Invoke ItemsPresenter.MouseWheelUp()");
            TestServices.InputHelper.ScrollMouseWheel(list, 10);
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.IsLessThan(scrollViewer.VerticalOffset, lastOffset);
            });

            list = PrepareListView(true /* isHorizontal */);

            UIExecutor.Execute(() =>
            {
                scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                lastOffset = scrollViewer.HorizontalOffset;
            });

            Log.Comment("Invoke ItemsPresenter.MouseWheelRight()");
            TestServices.InputHelper.ScrollMouseWheel(list, -10);
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.IsGreaterThan(scrollViewer.HorizontalOffset, lastOffset); // logical offsets
                lastOffset = scrollViewer.HorizontalOffset;
            });

            Log.Comment("Invoke ItemsPresenter.MouseWheelLeft()");
            TestServices.InputHelper.ScrollMouseWheel(list, 10);
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.IsLessThan(scrollViewer.HorizontalOffset, lastOffset);
            });
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanUseMouseWheelToZoomOut()
        {
            CanUseMouseWheelToZoom(zoomIn: false);
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanUseMouseWheelToZoomIn()
        {
            CanUseMouseWheelToZoom(zoomIn: true);
        }

        private void CanUseMouseWheelToZoom(bool zoomIn)
        {
            ListView list = PrepareListView();
            ScrollViewer scrollViewer = null;
            float zoomFactor = 1.0f;

            UIExecutor.Execute(() =>
            {
                scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                scrollViewer.ZoomMode = ZoomMode.Enabled;
                zoomFactor = scrollViewer.ZoomFactor;
            });

            Log.Comment("Invoke {0}", zoomIn ? "ItemsPresenter.MouseWheelUp()" : "ItemsPresenter.MouseWheelDown()");
            TestServices.KeyboardHelper.PressKeySequence("$d$_ctrl");
            TestServices.WindowHelper.WaitForIdle();

            for (int i = 0; i < 10 && zoomFactor == 1.0f; ++i)
            {
                int sign = zoomIn ? 1 : -1;
                TestServices.InputHelper.ScrollMouseWheel(list, 1 * sign);
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    zoomFactor = scrollViewer.ZoomFactor;
                });
            }

            TestServices.KeyboardHelper.PressKeySequence("$u$_ctrl");
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                if (zoomIn)
                {
                    Verify.IsGreaterThan(scrollViewer.ZoomFactor, 1.0f);
                }
                else
                {
                    Verify.IsLessThan(scrollViewer.ZoomFactor, 1.0f);   
                }
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]   // DCPP: Test Controls.ItemsPresenterTests.ItemsPresenterTests fails on WPF
        public void CanProcessScrollIntoViewAfterCollectionChange()
        {
            ListView list = PrepareListView();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling into view item at index 50 with default alignment.");
                list.ScrollIntoView(list.Items[50]);
                Log.Comment("Clear data source.");
                ((ObservableCollection<string>)list.ItemsSource).Clear();
                Log.Comment("Calling UpdateLayout. We should not crash during this call like we used to.");
                list.UpdateLayout();

                list.ItemsSource = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(i => "Item #" + i).ToList());
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling into view item at index 50 with default alignment again.");
                list.ScrollIntoView(list.Items[50]);
                Log.Comment("Remove all but 20 items from the items source.");
                var data = (ObservableCollection<string>)list.ItemsSource;
                for (int i = 0; i < 80; ++i)
                {
                    data.RemoveAt(20);
                }
                Log.Comment("Calling UpdateLayout. We should not crash during this call like we used to.");
                list.UpdateLayout();

                var scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.AreEqual(scrollViewer.ExtentHeight, scrollViewer.ViewportHeight + scrollViewer.VerticalOffset);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanCarouselPanelHandleReset()
        {
            //
            // In this test, we reset the ItemsSource (in three different ways)
            // while we are panning a CarouselPanel. The goal is to validate that
            // we don't crash in that scenario.
            //

            ComboBox comboBox = null;
            var comboBoxLoaded = new AutoResetEvent(false);
            
            UIExecutor.Execute(() =>
            {
                comboBox = new ComboBox();
                comboBox.HorizontalAlignment = HorizontalAlignment.Center;
                comboBox.VerticalAlignment = VerticalAlignment.Center;
                comboBox.ItemsSource = new ResetCollection(Enumerable.Range(0, 100).Select(i => "Item #" + i).ToList());
                TestServices.WindowHelper.WindowContent = comboBox;
                comboBox.Loaded += delegate
                {
                    comboBox.IsDropDownOpen = true;
                    comboBoxLoaded.Set();
                };
            });

            Verify.IsTrue(comboBoxLoaded.WaitOne(TimeSpan.FromSeconds(10)), "ComboBox loaded.");
            TestServices.WindowHelper.WaitForIdle();

            bool dispatchingResetOperations = true;

            UIExecutor.Execute(() =>
            {
                var taskNotAwaited = comboBox.Dispatcher.RunAsync(global::Windows.UI.Core.CoreDispatcherPriority.Normal, async () =>
                {
                    await Task.Delay(200);

                    for (int i = 0; i < 2; ++i)
                    {
                        Log.Comment("Setting a new ItemsSource...");
                        var collection = new ResetCollection(Enumerable.Range(0, 75).Select(j => "Item #" + j).ToList());
                        comboBox.ItemsSource = collection;
                        await Task.Delay(200);

                        Log.Comment("Resetting the ItemsSource...");
                        collection.ResetWith(Enumerable.Range(75, 80).Select(j => "Item #" + j).ToList());
                        await Task.Delay(200);

                        Log.Comment("Reset with the same data...");
                        collection.Reset();
                        await Task.Delay(200);
                    }

                    dispatchingResetOperations = false;
                });
            });

            while (dispatchingResetOperations)
            {
                Log.Comment("Panning up...");
                TestServices.InputHelper.PanFromCenter(comboBox, 0, 200, 0.1);
                TestServices.WindowHelper.WaitForIdle();
                Log.Comment("Panning down...");
                TestServices.InputHelper.PanFromCenter(comboBox, 0, -200, 0.1);
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        private ListView PrepareListView(bool isHorizontal = false)
        {
            ListView list = null;
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = (ListView)XamlReader.Load(string.Format(@"
<ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' {0}>
    <ListView.ItemsPanel>
        <ItemsPanelTemplate>
            <VirtualizingStackPanel Orientation='{1}' />
        </ItemsPanelTemplate>
    </ListView.ItemsPanel>
    <ListView.Header>
        <Border {2}Alignment='Stretch' Background='Red'>
            <TextBlock {3}='100'>Header</TextBlock>
        </Border>
    </ListView.Header>
</ListView>",
            isHorizontal ? "ScrollViewer.VerticalScrollMode='Disabled' ScrollViewer.HorizontalScrollMode='Enabled'" : string.Empty,
            isHorizontal ? "Horizontal" : "Vertical",
            isHorizontal ? "Vertical" : "Horizontal",
            isHorizontal ? "Width" : "Height"));

                list.ItemsSource = new ObservableCollection<string>(Enumerable.Range(0, 1000).Select(i => "Item #" + i).ToList());
                TestServices.WindowHelper.WindowContent = list;

                list.Loaded += delegate
                {
                    listLoaded.Set();
                };
            });
            
            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "List loaded.");
            TestServices.WindowHelper.WaitForIdle();
            return list;
        }
    }
}
