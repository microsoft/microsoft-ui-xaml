// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
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
        [TestProperty("Hosting:Mode", "UAP")]
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
        [TestProperty("Hosting:Mode", "UAP")]
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
