// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Enterprise.Moco.Data;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco
{
    [TestClass]
    public partial class IncrementalLoadingTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }
        ListView listView = null;
        ObservableCollection<object> itemsSource;
        CustomCollection cc;
        int _eventFireCount = 0;
        AutoResetEvent loadMoreItemsAsyncFiredEvent = null;

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
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

        public FrameworkElement LoadXamlIntoWindow(string xamlText)
        {
            FrameworkElement target = null;
            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading xaml: ");
                target = (FrameworkElement)XamlReader.Load(xamlText);
                Verify.IsNotNull(target, "Unable to load xaml.");

                TestServices.WindowHelper.WindowContent = target;
            });

            return target;
        }

        private void SetupTest(string xamlText)
        {
            var TargetPanel = LoadXamlIntoWindow(xamlText);

            UIExecutor.Execute(() =>
            {
                listView = (ListView)TargetPanel.FindNameInSubtree("_listView");
                Verify.IsTrue(listView != null, "Could not find the _listView in the xaml");

                itemsSource = DataSourceHelper.GeneratePersonDataSource(40, 2);
                cc = new CustomCollection(itemsSource);

                listView.ItemsSource = cc;
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [Priority(1)]
        [TestProperty("Hosting:Mode", "WPF")]
        public void LoadMoreItemsEdgeTest()
        {
            SetupTest(xamlText);

            loadMoreItemsAsyncFiredEvent = new AutoResetEvent(false);
            ScrollViewer sv = null;
            UIExecutor.Execute(() =>
            {
                // hook up the event for verification
                _eventFireCount = 0;
                cc.LoadMoreItemsAsyncFired += OnDataSource_LoadMoreItemsAsyncFired;
                cc.HasMoreItems = true;

                Log.Comment("1. scroll the ListViewBase to the edge");
                sv = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsNotNull(sv, "ListViewBase should have a scrollviewer");
            });

            UIExecutor.Execute(() =>
            {
                sv.ChangeView(null, sv.ExtentHeight + 1, null);
            });

            loadMoreItemsAsyncFiredEvent.WaitOne(4000);

            Verify.IsTrue(_eventFireCount > 0, "LoadMoreItemsAsync did not fire on edge scrolling");
        }

        [Description("Verify IIncrementalLoadingVector.LoadMoreItemsAsync is called for automatic edge loading multiple times")]
        [TestMethod]
        [Priority(1)]
        [TestProperty("Hosting:Mode", "WPF")]
        public void LoadMoreItemsEdgeItemsWrapGridTest()
        {
            SetupTest(xamlTextItemsWrapGrid);

            loadMoreItemsAsyncFiredEvent = new AutoResetEvent(false);
            ScrollViewer sv = null;
            UIExecutor.Execute(() =>
            {
                // hook up the event for verification
                _eventFireCount = 0;
                cc.LoadMoreItemsAsyncFired += OnDataSource_LoadMoreItemsAsyncFired;
                cc.HasMoreItems = true;

                Log.Comment("1. scroll the ListViewBase to the edge");
                sv = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsNotNull(sv, "ListViewBase should have a scrollviewer");
            });

            UIExecutor.Execute(() =>
            {
                sv.ChangeView(null, sv.ExtentHeight + 1, null);
            });

            loadMoreItemsAsyncFiredEvent.WaitOne(4000);

            Verify.IsTrue(_eventFireCount > 0, "LoadMoreItemsAsync did not fire on edge scrolling");
        }

        [TestMethod]
        [Priority(1)]
        [TestProperty("Hosting:Mode", "WPF")]
        public void LoadMoreItemsEdgeWrapGridTest()
        {
            SetupTest(xamlTextWrapGrid);

            loadMoreItemsAsyncFiredEvent = new AutoResetEvent(false);
            ScrollViewer sv = null;
            UIExecutor.Execute(() =>
            {
                // hook up the event for verification
                _eventFireCount = 0;
                cc.LoadMoreItemsAsyncFired += OnDataSource_LoadMoreItemsAsyncFired;
                cc.HasMoreItems = true;

                Log.Comment("1. scroll the ListViewBase to the edge");
                sv = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsNotNull(sv, "ListViewBase should have a scrollviewer");
            });

            UIExecutor.Execute(() =>
            {
                sv.ChangeView(null, sv.ExtentHeight + 1, null);
            });

            loadMoreItemsAsyncFiredEvent.WaitOne(4000);

            Verify.IsTrue(_eventFireCount > 0, "LoadMoreItemsAsync did not fire on edge scrolling");
        }

        [Description("Verify IIncrementalLoadingVector.LoadMoreItemsAsync is called for automatic edge loading multiple times")]
        [TestMethod]
        [Priority(1)]
        [TestProperty("Hosting:Mode", "WPF")]
        public void RepeatedLoadMoreItemsTest()
        {
            SetupTest(xamlText);

            ScrollViewer sv = null;
            UIExecutor.Execute(() =>
            {
                sv = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsNotNull(sv, "ListViewBase should have a scrollviewer");
            });

            for (int i = 0; i < 3; i++)
            {
                Log.Comment(string.Format("== Srcoll counter: {0}", i));

                loadMoreItemsAsyncFiredEvent = new AutoResetEvent(false);
                UIExecutor.Execute(() =>
                {
                    // hook up the event for verification
                    _eventFireCount = 0;
                    cc.LoadMoreItemsAsyncFired += OnDataSource_LoadMoreItemsAsyncFired;
                    cc.HasMoreItems = true;

                    Log.Comment("1. scroll the ListViewBase to the edge");
                    sv.ChangeView(null, sv.ScrollableHeight - 2, null);

                });

                TestServices.WindowHelper.WaitForIdle();

                loadMoreItemsAsyncFiredEvent.WaitOne(4000);
                Log.Comment(string.Format("Total event fire count: {0}", _eventFireCount));
                Verify.IsTrue(_eventFireCount == 1, "LoadMoreItemsAsync did not fire on edge scrolling");

                UIExecutor.Execute(() =>
               {
                   cc.LoadMoreItemsAsyncFired -= OnDataSource_LoadMoreItemsAsyncFired;
               });
            }
        }

        [TestMethod]
        [Priority(1)]
        [Description("Verify IIncrementalLoadingVector.LoadMoreItemsAsync by calling programmatically")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void LoadMoreItemsProgrammaticTest()
        {
            SetupTest(xamlText);
            int prevItemCount = cc.Count;

            LoadMoreItemsAsync loadMoreItemsAsync = null;
            UIExecutor.Execute(() =>
            {
                cc.HasMoreItems = true;

                Log.Comment("1. Call LoadMoreItemsAsync programmatically");
                loadMoreItemsAsync = cc.LoadMoreItemsAsync(30) as LoadMoreItemsAsync;
            });

            Verify.IsTrue(cc.Count == prevItemCount + 30, "Items did not load correctly.");
        }

        [TestMethod]
        [Priority(1)]
        [Description("Verify ListViewBase.LoadMoreItemsAsync by calling programmatically")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void LoadMoreItemsProgrammaticTest2()
        {
            SetupTest(xamlText);
            int prevItemCount = cc.Count;

            UIExecutor.Execute(() =>
            {
                cc.HasMoreItems = true;

                Log.Comment("1. Call LoadMoreItemsAsync programmatically");
                LoadMoreItemsAsyncHelper(listView);
            });

            Log.Comment(string.Format("NumItemCount passed to LoadMoreItemsAsync: {0}", cc.LastNumItemsToLoad));
            Verify.IsTrue(cc.Count > prevItemCount, "Items did not load correctly.");
        }

        [TestMethod]
        [Priority(1)]
        [Description("Verify ListViewBase.LoadMoreItemsAsync doesn't trigger a load programmatically when HasMoreItems is false.")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void LoadMoreItemsProgrammaticTest3()
        {
            SetupTest(xamlText);
            int prevItemCount = cc.Count;

            UIExecutor.Execute(() =>
            {
                cc.HasMoreItems = false;

                Log.Comment("1. Call LoadMoreItemsAsync programmatically");
                LoadMoreItemsAsyncHelper(listView);
            });

            Log.Comment(string.Format("NumItemCount passed to LoadMoreItemsAsync: {0}", cc.LastNumItemsToLoad));
            Verify.IsTrue(cc.Count == prevItemCount, "Items were still loaded.");
        }

        [TestMethod]
        [Priority(1)]
        [Description("Verify IncrementalLoadingTrigger set to None does not trigger LoadMoreItemsAsync")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void IncrementalLoadingTriggerNoneTest1()
        {
            SetupTest(xamlText);
            loadMoreItemsAsyncFiredEvent = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                // hook up the event for verification
                _eventFireCount = 0;
                cc.LoadMoreItemsAsyncFired += OnDataSource_LoadMoreItemsAsyncFired;
                cc.HasMoreItems = true;
                listView.IncrementalLoadingTrigger = IncrementalLoadingTrigger.None;

                Log.Comment("1. scroll the ListViewBase to the edge");
                var sv = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsNotNull(sv, "ListViewBase should have a scrollviewer");


                sv.ChangeView(null, sv.ScrollableHeight, null);

            });

            loadMoreItemsAsyncFiredEvent.WaitOne(300);
            Verify.IsTrue(_eventFireCount == 0, "LoadMoreItemsAsync did fire on edge scrolling");
        }

        [TestMethod]
        [Priority(1)]
        [Description("Verify HasMoreItems = false does not trigger LoadMoreItemsAsync")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void HasMoreItemsFalseTest()
        {
            SetupTest(xamlText);
            loadMoreItemsAsyncFiredEvent = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                // hook up the event for verification
                _eventFireCount = 0;
                cc.LoadMoreItemsAsyncFired += OnDataSource_LoadMoreItemsAsyncFired;
                cc.HasMoreItems = false;

                Log.Comment("1. scroll the ListViewBase to the edge");
                var sv = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsNotNull(sv, "ListViewBase should have a scrollviewer");

                sv.ChangeView(null, sv.ScrollableHeight, null);
            });

            loadMoreItemsAsyncFiredEvent.WaitOne(300);
            Verify.IsTrue(_eventFireCount == 0, "LoadMoreItemsAsync did fire on edge scrolling");
        }

        [TestMethod]
        [Priority(1)]
        [Description("Verify setting IncrementalLoadingThreshold is honored for edge scrolling")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void IncrementalLoadingThresholdTest()
        {
            SetupTest(xamlText);

            UIExecutor.Execute(() =>
            {
                listView.ItemsSource = new CustomCollection(DataSourceHelper.GeneratePersonDataSource(40, 2));
            });

            loadMoreItemsAsyncFiredEvent = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                // hook up the event for verification
                _eventFireCount = 0;
                (listView.ItemsSource as CustomCollection).LoadMoreItemsAsyncFired += OnDataSource_LoadMoreItemsAsyncFired;
                (listView.ItemsSource as CustomCollection).HasMoreItems = true;

                // This is the number of pages from the end before having to load
                // setting to a high number just to verify that the first change in scroll offset will trigger the LoadMoreItemsAsync
                listView.IncrementalLoadingThreshold = 15;

                Log.Comment("1. scroll the ListViewBase by a small step");
                var sv = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsNotNull(sv, "ListViewBase should have a scrollviewer");

                sv.ChangeView(null, sv.ScrollableHeight, null);
            });

            Log.Comment("count now is : " + listView.Items.Count);

            loadMoreItemsAsyncFiredEvent.WaitOne(4000);
            Verify.IsTrue(_eventFireCount > 0, "LoadMoreItemsAsync did NOT fire on edge scrolling");
            UIExecutor.Execute(() =>
            {
                (listView.ItemsSource as CustomCollection).LoadMoreItemsAsyncFired -= OnDataSource_LoadMoreItemsAsyncFired;
            });

            Log.Comment("event count is : " + _eventFireCount);
        }

        [TestMethod]
        [Priority(1)]
        [Description("Removing items before selected index, in an incremental loading case")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void DeleteItemsBeforeSelectedIndexWithIncrementalLoading()
        {
            ListView listView = null;
            IncrementallyLoadingData data = new IncrementallyLoadingData(9);

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"<ListView Height='400'
                                                              xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                              xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                                            <ListView.ItemTemplate>
                                                                <DataTemplate>
                                                                    <StackPanel Height='50'>
                                                                        <TextBlock Text='{Binding}' />
                                                                    </StackPanel>
                                                                </DataTemplate>
                                                            </ListView.ItemTemplate>
                                                        </ListView>");
                listView.ItemsSource = data;
                TestServices.WindowHelper.WindowContent = listView;

                listView.SelectionChanged += (sender, args) =>
                {
                    // when selected index is 7, remove 6 items above the selected index.
                    if (listView.SelectedIndex == 7)
                    {
                        int index = listView.SelectedIndex;
                        int itemsToDelete = 6;
                        for (int i = 0; i < itemsToDelete; i++)
                        {
                            data.RemoveAt(index - 1 - i);
                        }
                    }
                };
            });

            TestServices.WindowHelper.WaitForIdle();
            
            UIExecutor.Execute(() =>
            {
                // select index 7 and cause selection changed which
                // will remove several items above it. Since the data only had 9 items
                // to start with, the last index in viewport will be the last item in the 
                // data (incremental loading has not been triggered yet)
                listView.Focus(FocusState.Pointer);
                listView.SelectedIndex = 7;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                int lastVisibleIndex = (listView.ItemsPanelRoot as ItemsStackPanel).LastVisibleIndex;
                Log.Comment("Last visible index:" + lastVisibleIndex);
                
                // make sure that more data was loaded and displayed
                Verify.IsTrue(lastVisibleIndex > 3);
            });
        }

        [TestMethod]
        [Description("Load call should not be raised after listview is not active")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void NoLoadCallsAfterListViewIsInactive()
        {
            StackPanel panel = null;
            ListView listView = null;
            var data = new ListViewLoader();

            UIExecutor.Execute(() =>
            {
                listView = new ListView() { Width = 300, Height = 300 };
                listView.ItemsSource = data;
                panel = new StackPanel();
                panel.Children.Add(listView);
                TestServices.WindowHelper.WindowContent = panel;
            });

            // wait for one Load call 
            ListViewLoader.LoadCalledEvent.WaitOne();
            
            UIExecutor.Execute(() =>
            {
                // remove listview from live tree
                panel.Children.Remove(listView);
                ListViewLoader.LoadCalledEvent.Reset();
                ListViewLoader.LoadCallCount = 0;
            });

            TestServices.WindowHelper.WaitForIdle();

            Verify.IsFalse(ListViewLoader.LoadCalledEvent.WaitOne(100));
            Verify.AreEqual(0, ListViewLoader.LoadCallCount);
        }

        private void OnDataSource_LoadMoreItemsAsyncFired(object sender, object e)
        {

            Log.Comment("event is triggered!");
            _eventFireCount++;
            loadMoreItemsAsyncFiredEvent.Set();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void SynchronousStartupWaitsForItemMeasureWithItemsWrapGrid()
        {
            VerifySynchronousStartup(wrapping: true, seeded: false);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void SynchronousStartupWaitsForItemMeasureWithItemsStackPanel()
        {
            VerifySynchronousStartup(wrapping: false, seeded: false);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void SynchronousStartupWithMeasuredSeedUsesPageSizedBatches()
        {
            VerifySynchronousStartup(wrapping: true, seeded: true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void SynchronousStartupWaitsForItemMeasureWithVerticalItemsWrapGrid()
        {
            VerifySynchronousStartup(wrapping: true, seeded: false, vertical: true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void SynchronousStartupWaitsForItemMeasureWithVerticalItemsStackPanel()
        {
            VerifySynchronousStartup(wrapping: false, seeded: false, vertical: true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void SynchronousStartupWithPartialBatchesFillsViewport()
        {
            VerifySynchronousStartup(wrapping: true, seeded: false, maximumBatchSize: 1);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void DeferredStartupWaitsForItemMeasure()
        {
            SynchronousStartupSource data = null;
            UIExecutor.Execute(() =>
            {
                data = new SynchronousStartupSource(false) { HoldCompletion = true };
                ShowStartupListView(data, wrapping: true);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(1, data.RequestedCounts.Count, "Only one load may be outstanding.");
                Verify.AreEqual(0, data.Count);
                data.CompletePendingLoad();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() => VerifyStartupRequests(data, seeded: false));
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void DeferredStartupDoesNotRetryAfterRemoval()
        {
            SynchronousStartupSource data = null;
            UIExecutor.Execute(() =>
            {
                data = new SynchronousStartupSource(false) { HoldCompletion = true };
                ShowStartupListView(data, wrapping: true);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(1, data.RequestedCounts.Count);
                TestServices.WindowHelper.WindowContent = new Grid();
                data.CompletePendingLoad();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() => Verify.AreEqual(1, data.RequestedCounts.Count,
                "Completing a load on a detached list must not request another batch."));
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ManualStartupDoesNotEnableAutomaticLoading()
        {
            SynchronousStartupSource data = null;
            StartupListView inner = null;
            UIExecutor.Execute(() =>
            {
                data = new SynchronousStartupSource(false);
                inner = ShowStartupListView(data, wrapping: true, manual: true);
            });
            TestServices.WindowHelper.WaitForIdle();

            IAsyncOperation<LoadMoreItemsResult> load = null;
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(0, data.RequestedCounts.Count);
                load = inner.LoadMoreItemsAsync();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(1, data.RequestedCounts.Count);
                Verify.AreEqual(1, data.Count);
                load.Cancel();
                load.Close();
                load = inner.LoadMoreItemsAsync();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(2, data.RequestedCounts.Count, "Only the two explicit calls may request data.");
                Verify.IsTrue(data.Count > 1, "The second explicit load must add a measured page-sized batch.");
                load.Cancel();
                load.Close();
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void EmptyBatchDoesNotStopAutomaticLoading()
        {
            SynchronousStartupSource data = null;
            StartupListView inner = null;
            UIExecutor.Execute(() =>
            {
                data = new SynchronousStartupSource(false) { EmptyLoadsRemaining = 1 };
                for (int index = 0; index < 40; index++)
                {
                    data.Add(index.ToString());
                }
                inner = ShowStartupListView(data, wrapping: true, manual: true);
                inner.Width = 500;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(0, data.RequestedCounts.Count);
                inner.IncrementalLoadingTrigger = IncrementalLoadingTrigger.Edge;
                var scrollViewer = inner.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsTrue(scrollViewer.ScrollableWidth > 0);
                Verify.IsTrue(scrollViewer.ChangeView(scrollViewer.ScrollableWidth, null, null, true));
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(0, data.EmptyLoadsRemaining);
                Verify.IsTrue(data.RequestedCounts.Count > 1, "An empty batch must not prevent the next load.");
                Verify.IsTrue(data.Count > 40, "The following load must supply items without another user action.");
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void DeferredEdgeLoadingPreservesLoadingWithStackPanel()
        {
            VerifyLegacyPanelEdgeLoading("<StackPanel Orientation='Horizontal'/>");
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void DeferredEdgeLoadingPreservesLoadingWithWrapGrid()
        {
            VerifyLegacyPanelEdgeLoading("<WrapGrid Orientation='Vertical' MaximumRowsOrColumns='2'/>");
        }

        private void VerifyLegacyPanelEdgeLoading(string panelMarkup)
        {
            SynchronousStartupSource data = null;
            StartupListView inner = null;
            UIExecutor.Execute(() =>
            {
                data = new SynchronousStartupSource(false)
                {
                    MaximumBatchSize = 1,
                    HoldCompletion = true,
                    HoldSubsequentLoads = true,
                    EmptyLoadsRemaining = 1
                };
                for (int index = 0; index < 40; index++)
                {
                    data.Add(index.ToString());
                }
                inner = ShowStartupListView(data, wrapping: true, manual: true, panelMarkup: panelMarkup);
                inner.Width = 500;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(0, data.RequestedCounts.Count);
                inner.IncrementalLoadingTrigger = IncrementalLoadingTrigger.Edge;
                var scrollViewer = inner.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsTrue(scrollViewer.ScrollableWidth > 0);
                Verify.IsTrue(scrollViewer.ChangeView(scrollViewer.ScrollableWidth, null, null, true));
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(40, data.Count);
                Verify.AreEqual(1, data.RequestedCounts.Count);
                // Isolate the resume-notification contract from adding unmeasured legacy containers.
                inner.ItemsPanelRoot.InvalidateMeasure();
                data.CompletePendingLoad();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Requested batches: " + string.Join(", ", data.RequestedCounts));
                Verify.AreEqual(40, data.Count);
                Verify.AreEqual(2, data.RequestedCounts.Count, "Loading must continue after the first completed batch.");
                TestServices.WindowHelper.WindowContent = new Grid();
                data.CompletePendingLoad();
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void UnrelatedArrangeDoesNotStartLoading()
        {
            SynchronousStartupSource data = null;
            StartupListView inner = null;
            UIExecutor.Execute(() =>
            {
                data = new SynchronousStartupSource(false) { LoadingEnabled = false };
                inner = ShowStartupListView(data, wrapping: true);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(0, data.RequestedCounts.Count);
                data.LoadingEnabled = true;
                inner.ItemsPanelRoot.InvalidateArrange();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() => Verify.AreEqual(0, data.RequestedCounts.Count,
                "An unrelated arrange must not initiate an automatic loading check."));
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void IncrementalLoadingZoomOutHorizontal()
        {
            VerifyZoomedPageEstimate(vertical: false, zoomFactor: 0.5f);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void IncrementalLoadingZoomInHorizontal()
        {
            VerifyZoomedPageEstimate(vertical: false, zoomFactor: 2.0f);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void IncrementalLoadingZoomOutVertical()
        {
            VerifyZoomedPageEstimate(vertical: true, zoomFactor: 0.5f);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void IncrementalLoadingZoomInVertical()
        {
            VerifyZoomedPageEstimate(vertical: true, zoomFactor: 2.0f);
        }

        private void VerifyZoomedPageEstimate(bool vertical, float zoomFactor)
        {
            SynchronousStartupSource data = null;
            StartupListView inner = null;
            ScrollViewer scrollViewer = null;
            using (var viewChanged = new AutoResetEvent(false))
            {
                UIExecutor.Execute(() =>
                {
                    data = new SynchronousStartupSource(true);
                    inner = ShowStartupListView(data, wrapping: true, vertical: vertical, manual: true);
                    inner.DataFetchSize = 1;
                    if (vertical)
                    {
                        inner.Height = 300;
                        inner.Width = 100;
                    }
                    else
                    {
                        inner.Width = 500;
                        inner.Height = 100;
                    }
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var panel = (ItemsWrapGrid)inner.ItemsPanelRoot;
                    panel.ItemWidth = 100;
                    panel.ItemHeight = 100;
                    panel.MaximumRowsOrColumns = 1;
                    scrollViewer = inner.FindElementOfTypeInSubtree<ScrollViewer>();
                    scrollViewer.MinZoomFactor = 0.5f;
                    scrollViewer.MaxZoomFactor = 2.0f;
                    scrollViewer.ZoomMode = ZoomMode.Enabled;
                    scrollViewer.ViewChanged += (sender, args) =>
                    {
                        if (!args.IsIntermediate)
                        {
                            viewChanged.Set();
                        }
                    };
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() => Verify.IsTrue(scrollViewer.ChangeView(null, null, zoomFactor, true)));
                Verify.IsTrue(viewChanged.WaitOne(TimeSpan.FromSeconds(10)), "Zoom must complete.");
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(zoomFactor, scrollViewer.ZoomFactor);
                    Verify.AreEqual(0, data.RequestedCounts.Count);
                    double viewport = vertical ? scrollViewer.ViewportHeight : scrollViewer.ViewportWidth;
                    uint expected = (uint)Math.Ceiling(viewport / zoomFactor / 100);
                    Verify.IsTrue(expected > 0);
                    var load = inner.LoadMoreItemsAsync();
                    Verify.AreEqual(1, data.RequestedCounts.Count);
                    Log.Comment("Zoom " + zoomFactor + ", viewport " + viewport + ", requested " + data.RequestedCounts[0]);
                    Verify.AreEqual(expected, data.RequestedCounts[0],
                        "A page must use the zoom-adjusted scrolling viewport and one stacking line.");
                    load.Cancel();
                    load.Close();
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() => VerifyStartupViewportFilled(inner, data, vertical));
            }
        }

        private void VerifySynchronousStartup(bool wrapping, bool seeded, bool vertical = false, uint maximumBatchSize = 64)
        {
            SynchronousStartupSource data = null;
            StartupListView inner = null;
            UIExecutor.Execute(() =>
            {
                data = new SynchronousStartupSource(seeded) { MaximumBatchSize = maximumBatchSize };
                inner = ShowStartupListView(data, wrapping, vertical);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                VerifyStartupRequests(data, seeded);
                VerifyStartupViewportFilled(inner, data, vertical);
            });
        }

        private static StartupListView ShowStartupListView(SynchronousStartupSource data, bool wrapping, bool vertical = false, bool manual = false, string panelMarkup = null)
        {
            string panel = panelMarkup ?? (wrapping
                ? "<ItemsWrapGrid Orientation='" + (vertical ? "Horizontal" : "Vertical") + "' MaximumRowsOrColumns='2'/>"
                : "<ItemsStackPanel Orientation='" + (vertical ? "Vertical" : "Horizontal") + "'/>");
            var inner = new StartupListView(data.RecordFirstItemMeasure)
            {
                ItemsSource = data,
                VerticalAlignment = vertical ? VerticalAlignment.Stretch : VerticalAlignment.Center,
                HorizontalAlignment = vertical ? HorizontalAlignment.Center : HorizontalAlignment.Stretch,
                DataFetchSize = 3,
                IncrementalLoadingThreshold = 0,
                IncrementalLoadingTrigger = manual ? IncrementalLoadingTrigger.None : IncrementalLoadingTrigger.Edge,
                ItemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <Grid Width='100' Height='100'><TextBlock Text='{Binding}'/></Grid>
                      </DataTemplate>"),
                ItemsPanel = (ItemsPanelTemplate)XamlReader.Load(
                    "<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                    panel + "</ItemsPanelTemplate>")
            };
            if (wrapping)
            {
                // Isolate startup scheduling from the separate unbounded wrap-grid estimator fix.
                if (vertical)
                {
                    inner.Width = 200;
                }
                else
                {
                    inner.Height = 200;
                }
            }
            ScrollViewer.SetVerticalScrollBarVisibility(inner, vertical ? ScrollBarVisibility.Visible : ScrollBarVisibility.Disabled);
            ScrollViewer.SetVerticalScrollMode(inner, vertical ? ScrollMode.Enabled : ScrollMode.Disabled);
            ScrollViewer.SetHorizontalScrollBarVisibility(inner, vertical ? ScrollBarVisibility.Disabled : ScrollBarVisibility.Visible);
            ScrollViewer.SetHorizontalScrollMode(inner, vertical ? ScrollMode.Disabled : ScrollMode.Enabled);

            var outer = new ListView { Width = 600, Height = 400 };
            if (vertical)
            {
                outer.ItemsPanel = (ItemsPanelTemplate)XamlReader.Load(
                    @"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <ItemsStackPanel Orientation='Horizontal'/>
                      </ItemsPanelTemplate>");
                ScrollViewer.SetVerticalScrollBarVisibility(outer, ScrollBarVisibility.Disabled);
                ScrollViewer.SetVerticalScrollMode(outer, ScrollMode.Disabled);
                ScrollViewer.SetHorizontalScrollBarVisibility(outer, ScrollBarVisibility.Visible);
                ScrollViewer.SetHorizontalScrollMode(outer, ScrollMode.Enabled);
            }
            outer.Items.Add(inner);
            TestServices.WindowHelper.WindowContent = outer;
            return inner;
        }

        private static void VerifyStartupRequests(SynchronousStartupSource data, bool seeded)
        {
            Log.Comment("Requested batches: " + string.Join(", ", data.RequestedCounts));
            Log.Comment("Requests before first item measure: " + data.RequestsBeforeFirstItemMeasure);
            Verify.AreEqual(seeded ? 0 : 1, data.RequestsBeforeFirstItemMeasure,
                "Loading must allow the first item to be measured before requesting another batch.");
            Verify.IsTrue(data.RequestedCounts.Exists(count => count > 1),
                "Loading should use a page-sized batch once the item has been measured.");
            Verify.IsTrue(data.Count < 64, "Startup must not exhaust a source larger than the viewport.");
        }

        private static void VerifyStartupViewportFilled(StartupListView inner, SynchronousStartupSource data, bool vertical)
        {
            int lastVisible = inner.ItemsPanelRoot is ItemsWrapGrid wrap
                ? wrap.LastVisibleIndex
                : ((ItemsStackPanel)inner.ItemsPanelRoot).LastVisibleIndex;
            Verify.IsTrue(lastVisible >= 0);
            Verify.IsTrue(lastVisible < data.Count - 1,
                "Loaded content must extend beyond the viewport, not stop after an underfilled partial batch.");

            var scrollViewer = inner.FindElementOfTypeInSubtree<ScrollViewer>();
            var lastContainer = inner.ContainerFromIndex(lastVisible) as FrameworkElement;
            Verify.IsNotNull(scrollViewer);
            Verify.IsNotNull(lastContainer);
            Rect bounds = lastContainer.TransformToVisual(scrollViewer).TransformBounds(
                new Rect(0, 0, lastContainer.ActualWidth, lastContainer.ActualHeight));
            double viewport = vertical ? scrollViewer.ViewportHeight : scrollViewer.ViewportWidth;
            Verify.IsTrue(viewport > 0);
            Verify.IsTrue(vertical ? bounds.Bottom >= viewport - bounds.Height : bounds.Right >= viewport - bounds.Width,
                "Realized containers must fill the scrolling viewport to within one item.");
        }

        private sealed partial class SynchronousStartupSource : ObservableCollection<string>, ISupportIncrementalLoading
        {
            private TaskCompletionSource<LoadMoreItemsResult> pendingLoad;
            private uint pendingCount;

            public List<uint> RequestedCounts { get; } = new List<uint>();
            public int RequestsBeforeFirstItemMeasure { get; private set; } = -1;
            public bool HasMoreItems => LoadingEnabled && Count < 64 && RequestedCounts.Count < 64;
            public bool LoadingEnabled { get; set; } = true;
            public uint MaximumBatchSize { get; set; } = 64;
            public bool HoldCompletion { get; set; }
            public bool HoldSubsequentLoads { get; set; }
            public int EmptyLoadsRemaining { get; set; }

            public SynchronousStartupSource(bool seeded)
            {
                if (seeded)
                {
                    Add("seed");
                }
            }

            public void RecordFirstItemMeasure()
            {
                if (RequestsBeforeFirstItemMeasure == -1)
                {
                    RequestsBeforeFirstItemMeasure = RequestedCounts.Count;
                }
            }

            public IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
            {
                RequestedCounts.Add(count);
                return AsyncInfo.Run(_ =>
                {
                    if (HoldCompletion || (HoldSubsequentLoads && RequestedCounts.Count > 1))
                    {
                        Verify.IsNull(pendingLoad, "Only one load may be outstanding.");
                        pendingCount = count;
                        pendingLoad = new TaskCompletionSource<LoadMoreItemsResult>();
                        return pendingLoad.Task;
                    }
                    return Task.FromResult(AddItems(count));
                });
            }

            public void CompletePendingLoad()
            {
                Verify.IsNotNull(pendingLoad);
                var completion = pendingLoad;
                pendingLoad = null;
                HoldCompletion = false;
                completion.SetResult(AddItems(pendingCount));
            }

            private LoadMoreItemsResult AddItems(uint count)
            {
                if (EmptyLoadsRemaining > 0)
                {
                    EmptyLoadsRemaining--;
                    return new LoadMoreItemsResult();
                }
                uint returned = Math.Min(Math.Min(count, MaximumBatchSize), (uint)(64 - Count));
                for (uint index = 0; index < returned; index++)
                {
                    Add(Count.ToString());
                }
                return new LoadMoreItemsResult { Count = returned };
            }
        }

        private sealed partial class StartupListView : ListView
        {
            private readonly Action itemMeasured;

            public StartupListView(Action itemMeasured)
            {
                this.itemMeasured = itemMeasured;
            }

            protected override DependencyObject GetContainerForItemOverride()
            {
                return new StartupListViewItem(itemMeasured);
            }
        }

        private sealed partial class StartupListViewItem : ListViewItem
        {
            private readonly Action measured;

            public StartupListViewItem(Action measured)
            {
                this.measured = measured;
            }

            protected override Size MeasureOverride(Size availableSize)
            {
                Size desired = base.MeasureOverride(availableSize);
                measured();
                return desired;
            }
        }

        private async void LoadMoreItemsAsyncHelper(ListViewBase lvb)
        {
            await lvb.LoadMoreItemsAsync();
        }


        const string xamlText = @"<StackPanel xmlns='http://schemas.microsoft.com/client/2007'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            x:Name='spOwner' Orientation='Horizontal'>
                            <StackPanel x:Name='spListBoxesH' Orientation='Horizontal'>
                                <ListView x:Name='_listView' Margin='10' Height='500' Width='200'>          
                                  <ListView.ItemContainerStyle>
                                    <Style TargetType='ListViewItem'>
                                      <Setter Property='MinHeight' Value='83' />
                                    </Style>
                                  </ListView.ItemContainerStyle>
                                  <ListView.ItemsPanel>
                                    <ItemsPanelTemplate>
                                      <ItemsStackPanel CacheLength='0'/>
                                    </ItemsPanelTemplate>
                                  </ListView.ItemsPanel>
                                  <ListView.ItemTemplate>
                                    <DataTemplate>
                                      <Grid>
                                        <TextBlock Text='{Binding FirstName}'/>
                                      </Grid>
                                    </DataTemplate>
                                  </ListView.ItemTemplate>
                                </ListView>
                            </StackPanel>
                        </StackPanel>";

        public partial class IncrementallyLoadingData : ObservableCollection<int>, ISupportIncrementalLoading
        {
            private int itemCount;
            public IncrementallyLoadingData(int initialNumberOfItems)
            {
                this.AddItems(initialNumberOfItems);
            }

            private void AddItems(int count)
            {

                for (int i = itemCount; i < itemCount + count; i++)
                {
                    this.Add(i);
                }

                itemCount += count;
            }

            public bool HasMoreItems
            {
                get
                {
                    return true;
                }
            }

            public IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
            {
                return LoadMoreItemsAsyncInternal(count).AsAsyncOperation();
            }

            private async Task<LoadMoreItemsResult> LoadMoreItemsAsyncInternal(uint count)
            {
                // small delay to emulate async nature of data coming back in //
                await Task.Delay(100);
                this.AddItems((int)count);

                return new LoadMoreItemsResult() { Count = count };
            }
        }

        const string xamlTextItemsWrapGrid = @"<StackPanel xmlns='http://schemas.microsoft.com/client/2007'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            x:Name='spOwner' Orientation='Horizontal'>
                            <StackPanel x:Name='spListBoxesH' Orientation='Horizontal'>
                                <ListView x:Name='_listView' Margin='10' Height='500' Width='200'>          
                                  <ListView.ItemsPanel>
                                    <ItemsPanelTemplate>
                                      <ItemsWrapGrid Orientation='Horizontal' CacheLength='0'/>
                                    </ItemsPanelTemplate>
                                  </ListView.ItemsPanel>
                                  <ListView.ItemTemplate>
                                    <DataTemplate>
                                      <Grid>
                                        <TextBlock Text='{Binding FirstName}'/>
                                      </Grid>
                                    </DataTemplate>
                                  </ListView.ItemTemplate>
                                </ListView>
                            </StackPanel>
                        </StackPanel>";

        const string xamlTextWrapGrid = @"<StackPanel xmlns='http://schemas.microsoft.com/client/2007'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            x:Name='spOwner' Orientation='Horizontal'>
                            <StackPanel x:Name='spListBoxesH' Orientation='Horizontal'>
                                <ListView x:Name='_listView' Margin='10' Height='500' Width='200'>
                                  <ListView.ItemsPanel>
                                    <ItemsPanelTemplate>
                                        <WrapGrid Orientation='Horizontal' />
                                    </ItemsPanelTemplate>
                                  </ListView.ItemsPanel>
                                  <ListView.ItemTemplate>
                                    <DataTemplate>
                                      <Grid>
                                        <TextBlock Text='{Binding FirstName}'/>
                                      </Grid>
                                    </DataTemplate>
                                  </ListView.ItemTemplate>
                                </ListView>
                            </StackPanel>
                        </StackPanel>";

        public partial class ListViewLoader : ObservableCollection<string>, ISupportIncrementalLoading
        {
            private int loadCount = 0;
            private bool busy = false;
            public static int LoadCallCount { get;set;}
            public static ManualResetEvent LoadCalledEvent = new ManualResetEvent(false);

            public ListViewLoader()
            {
                this.HasMoreItems = true;
            }

            public IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
            {
                if (busy)
                {
                    throw new InvalidOperationException("busy");
                }

                busy = true;

                return this.InternalLoadMoreItemsAsync(count).AsAsyncOperation();
            }

            private async Task<LoadMoreItemsResult> InternalLoadMoreItemsAsync(uint count)
            {

                LoadCallCount++;
                try
                {
                    Debug.WriteLine(
                        $"InternalLoadMoreItemsAsync. Count={count}. Loaded has been called {this.loadCount} times.");

                    LoadCalledEvent.Set();
                    // first call takes some time to simulate delay from network. We set the event before
                    // the wait so it lets the test method continue while the async call waits.
                    await Task.Delay(TimeSpan.FromMilliseconds(loadCount++ == 0 ? 500 : 10));

                    for (int i = 0; i < count; i++)
                    {
                        this.Add(Guid.NewGuid().ToString());
                    }

                    return new LoadMoreItemsResult()
                    {
                        Count = count,
                    };
                }
                finally
                {
                    busy = false;
                }
            }

            public bool HasMoreItems { get; }
        }
    }
}
