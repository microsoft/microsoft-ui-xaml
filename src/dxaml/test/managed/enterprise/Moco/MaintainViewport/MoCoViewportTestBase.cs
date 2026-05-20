// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections;
using System.Collections.ObjectModel;
using System.IO;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.MaintainViewport
{
    /// <summary>
    /// Base class for all the ListView and GridView Viewport tests
    /// </summary>
    /// 
    public abstract class MoCoViewportTestBase : XamlTestsBase
    {
        
        #region General Actions

        public static FrameworkElement LoadXamlTestPage(string fileName)
        {
            string text = File.ReadAllText(fileName);
            FrameworkElement target = null;
            var targetLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                Log.Comment(string.Format("Loading xaml file '{0}'.", fileName));
                target = (FrameworkElement)XamlReader.Load(text);
                Verify.IsNotNull(target, "Unable to load xaml.");
                target.Loaded += delegate { targetLoaded.Set(); };
                TestServices.WindowHelper.WindowContent = target;
            });
            
            Verify.IsTrue(targetLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load XAML");

            return target;
        }

        /// <summary>
        /// Creates and returns a flat data collection to use at the ItemsSource in ListView
        /// </summary>
        /// <param name="count">Number of items</param>
        /// <returns>Collection of flat data</returns>
        /// 
        protected ObservableCollection<MoCoFlatDataModel> GetFlatDataCollection(int count)
        {
            ObservableCollection<MoCoFlatDataModel> data = new ObservableCollection<MoCoFlatDataModel>();
            for (int i = 0; i < count; i++)
            {
                data.Add(new MoCoFlatDataModel("Item " + i));
            }

            return data;
        }

        /// <summary>
        /// Creates and returns a grouped data collection to use at the ItemsSource in ListView
        /// </summary>
        /// <param name="count">Number of groups</param>
        /// <returns>Collection of grouped data</returns>
        /// 
        protected ObservableCollection<MoCoGroupedDataModel> GetGroupedDataCollection(int groupCount, int itemsPerGroup = 4)
        {
            string header = "abcdefghijklmnopqrstuvwxyz";
            ObservableCollection<MoCoGroupedDataModel> data = new ObservableCollection<MoCoGroupedDataModel>();

            for (int i = 0; i < groupCount; i++)
            {
                MoCoGroupedDataModel group = new MoCoGroupedDataModel((header[i % 26]).ToString());
                for (int j = 0; j < itemsPerGroup; j++)
                {
                    group.Items.Add(new MoCoFlatDataModel("Item " + i + "." + j));
                }

                data.Add(group);
            }

            return data;
        }

        /// <summary>
        /// Creates and returns a grouped data
        /// </summary>
        /// <param name="itemsCount">Number of items in the group</param>
        /// <returns>Grouped data</returns>
        /// 
        protected MoCoGroupedDataModel CreateGroupedData(int itemsCount = 4)
        {
            MoCoGroupedDataModel group = new MoCoGroupedDataModel("New");
            for (int j = 0; j < itemsCount; j++)
            {
                group.Items.Add(new MoCoFlatDataModel("New Item " + j));
            }

            return group;
        }

        /// <summary>
        /// Scrolls the list to the given indexed item
        /// </summary>
        /// <param name="index">index of the item to be at the top</param>
        /// <param name="list">ListView</param>
        /// <returns>True if success, False otherwise</returns>
        /// 
        protected bool ScrollTo(int index, ListViewBase list, ScrollIntoViewAlignment alignment = ScrollIntoViewAlignment.Default)
        {
            if (list != null && list.ItemsSource != null && list.ItemsSource is IList &&
                (list.ItemsSource as IList).Count > index && index >= 0)
            {
                list.ScrollIntoView((list.ItemsSource as IList)[index], alignment);
                return true;
            }

            return false;
        }

        /// <summary>
        /// Scrolls the list to the given indexed group
        /// </summary>
        /// <param name="index">index of the group to be at the top</param>
        /// <param name="list">ListView</param>
        /// <returns>True if success, False otherwise</returns>
        /// 
        protected bool ScrollToGroup(int index, ListViewBase list, ScrollIntoViewAlignment alignment = ScrollIntoViewAlignment.Default)
        {
            if (list != null && list.ItemsSource != null && this.cvs.Source is ObservableCollection<MoCoGroupedDataModel> &&
                (this.cvs.Source as ObservableCollection<MoCoGroupedDataModel>).Count > index && index >= 0)
            {
                list.ScrollIntoView((this.cvs.Source as ObservableCollection<MoCoGroupedDataModel>)[index], alignment);
                return true;
            }

            return false;
        }

        /// <summary>
        /// Performs data manipulations (such as Insert, Remove, Resize list items) with a Flat data collection
        /// </summary>
        /// <param name="list">ListView</param>
        /// <param name="dmType">DataManipulation type</param>
        /// <param name="index">Index of the item which is used as the reference for the data manipulation</param>
        /// <returns>True if succeeds, False otherwise</returns>
        protected bool PerformFlatDataManipulation(ListViewBase list, DataManipulationType dmType, int index)
        {
            const int BULK_COUNT = 100; // Number of bulk items to insert, remove and resize

            if (list != null && list.ItemsSource != null && list.ItemsSource is ObservableCollection<MoCoFlatDataModel> &&
                (list.ItemsSource as ObservableCollection<MoCoFlatDataModel>).Count > index && index >= 0)
            {
                ObservableCollection<MoCoFlatDataModel> dataSource = list.ItemsSource as ObservableCollection<MoCoFlatDataModel>;

                switch (dmType)
                {
                    case DataManipulationType.INSERT:
                    {
                        Log.Comment("Inserting item at index " + index + "...");
                        dataSource.Insert(index, new MoCoFlatDataModel() { Name = "New item " + index });
                        break;
                    }

                    case DataManipulationType.BULK_INSERT:
                    {
                        Log.Comment("Inserting bulk items at index " + index + "...");
                        for (int i = 0; i < BULK_COUNT; i++)
                        {
                            dataSource.Insert(index, new MoCoFlatDataModel() { Name = "New item " + i });
                        }
                        break;
                    }

                    case DataManipulationType.REMOVE:
                    {
                        Log.Comment("Removing item at index " + index + "...");
                        dataSource.RemoveAt(index);
                        break;
                    }

                    case DataManipulationType.BULK_REMOVE:
                    {
                        Log.Comment("Removing bulk items at index " + index + "...");
                        int count = (dataSource.Count - index) > BULK_COUNT ? BULK_COUNT : dataSource.Count - index;
                        for (int i = 0; i < BULK_COUNT; i++)
                        {
                            dataSource.RemoveAt(index);
                        }
                        break;
                    }

                    case DataManipulationType.EXPAND:
                    {
                        Log.Comment("Expanding item at index " + index + "...");
                        dataSource[index].IsExpanded = Visibility.Visible;
                        break;
                    }

                    case DataManipulationType.BULK_EXPAND:
                    {
                        Log.Comment("Expanding bulk items at index " + index + "...");
                        for (int i = 0; i < BULK_COUNT; i++)
                        {
                            dataSource[index + i].IsExpanded = Visibility.Visible;
                        }
                        break;
                    }

                    case DataManipulationType.COLLAPSE:
                    {
                        Log.Comment("Collapsing item at index " + index + "...");
                        dataSource[index].IsExpanded = Visibility.Collapsed;
                        break;
                    }

                    case DataManipulationType.BULK_COLLAPSE:
                    {
                        Log.Comment("Collapsing bulk items at index " + index + "...");
                        for (int i = 0; i < BULK_COUNT; i++)
                        {
                            dataSource[index + i].IsExpanded = Visibility.Collapsed;
                        }
                        break;
                    }

                    default:
                    {
                        Log.Error("Unknown data manipulation type found: " + dmType.ToString());
                        break;
                    }
                }

                return true;
            }

            return false;
        }

        /// <summary>
        /// Performs data manipulations (such as Insert, Remove, Resize list items) with a Grouped data collection
        /// </summary>
        /// <param name="list">ListView</param>
        /// <param name="dmType">DataManipulation type</param>
        /// <param name="groupKey">Group key which is used as the reference for Group manipulation</param>
        /// <param name="itemIndex">Index of the item which is used as the reference for the items manipulation</param>
        /// <returns>True if succeeds, False otherwise</returns>
        /// 
        protected bool PerformGroupedDataManipulation(ListViewBase list, DataManipulationType dmType, int groupIndex, int itemIndex = 0)
        {
            const int BULK_COUNT = 10; // Number of bulk groups to insert, remove and resize

            if (list != null && list.ItemsSource != null && this.cvs.Source is ObservableCollection<MoCoGroupedDataModel> &&
                (this.cvs.Source as ObservableCollection<MoCoGroupedDataModel>).Count > groupIndex && groupIndex >= 0)
            {
                ObservableCollection<MoCoGroupedDataModel> dataSource = this.cvs.Source as ObservableCollection<MoCoGroupedDataModel>;
                MoCoGroupedDataModel group = dataSource[groupIndex];

                switch (dmType)
                {
                    case DataManipulationType.INSERT:
                    {
                        Log.Comment("Inserting item#" + itemIndex + "...");
                        group.Items.Insert(itemIndex, new MoCoFlatDataModel() { Name = "New item " + itemIndex });
                        break;
                    }

                    case DataManipulationType.INSERT_GROUP:
                    {
                        Log.Comment("Inserting group#" + groupIndex + "...");
                        dataSource.Insert(groupIndex, CreateGroupedData());
                        break;
                    }

                    case DataManipulationType.BULK_INSERT:
                    {
                        Log.Comment("Inserting bulk groups...");
                        var newGroups = GetGroupedDataCollection(BULK_COUNT);
                        foreach (var grp in newGroups)
                        {
                            dataSource.Insert(groupIndex, grp);
                        }
                        break;
                    }

                    case DataManipulationType.REMOVE:
                    {
                        Log.Comment("Removing item#" + itemIndex + "...");
                        Verify.IsTrue(itemIndex >= 0 && itemIndex < group.Items.Count, "Invalid item index: " + itemIndex);
                        group.Items.RemoveAt(itemIndex);
                        break;
                    }

                    case DataManipulationType.REMOVE_GROUP:
                    {
                        Log.Comment("Removing group#" + groupIndex + "...");
                        dataSource.RemoveAt(groupIndex);
                        break;
                    }

                    case DataManipulationType.BULK_REMOVE:
                    {
                        Log.Comment("Removing bulk groups...");
                        int count = (dataSource.Count - groupIndex) > BULK_COUNT ? BULK_COUNT : dataSource.Count - groupIndex;
                        for (int i = 0; i < count; i++)
                        {
                            dataSource.RemoveAt(itemIndex);
                        }
                        break;
                    }

                    case DataManipulationType.EXPAND:
                    {
                        Log.Comment("Expanding item#" + itemIndex + "...");
                        Verify.IsTrue(itemIndex >= 0 && itemIndex < group.Items.Count, "Invalid item index: " + itemIndex);
                        group.Items[itemIndex].IsExpanded = Visibility.Visible;
                        break;
                    }

                    case DataManipulationType.EXPAND_GROUP:
                    {
                        Log.Comment("Expanding group#" + groupIndex + "...");
                        foreach (var item in group.Items)
                        {
                            item.IsExpanded = Visibility.Visible;
                        }
                        break;
                    }

                    case DataManipulationType.BULK_EXPAND:
                    {
                        Log.Comment("Expanding bulk groups...");
                        for (int i = 0; i < BULK_COUNT; i++)
                        {
                            if ((groupIndex + i) >= dataSource.Count) break;
                            foreach (var item in dataSource[groupIndex + i].Items)
                            {
                                item.IsExpanded = Visibility.Visible;
                            }
                        }
                        break;
                    }

                    case DataManipulationType.COLLAPSE:
                    {
                        Log.Comment("Collapsing item#" + itemIndex + "...");
                        Verify.IsTrue(itemIndex >= 0 && itemIndex < group.Items.Count, "Invalid item index: " + itemIndex);
                        group.Items[itemIndex].IsExpanded = Visibility.Collapsed;
                        break;
                    }

                    case DataManipulationType.COLLAPSE_GROUP:
                    {
                        Log.Comment("Collapsing group#" + groupIndex + "...");
                        foreach (var item in group.Items)
                        {
                            item.IsExpanded = Visibility.Collapsed;
                        }
                        break;
                    }

                    case DataManipulationType.BULK_COLLAPSE:
                    {
                        Log.Comment("Collapsing bulk groups...");
                        for (int i = 0; i < BULK_COUNT; i++)
                        {
                            if ((groupIndex + i) >= dataSource.Count) break;
                            foreach (var item in dataSource[groupIndex + i].Items)
                            {
                                item.IsExpanded = Visibility.Collapsed;
                            }
                        }
                        break;
                    }
                }

                return true;
            }

            return false;
        }

        /// <summary>
        /// Performs the given data manipulation in the list of flat data and verifies the viewport and visual changes
        /// </summary>
        /// <param name="list">ListView</param>
        /// <param name="dmType">Data manipulation type</param>
        /// <param name="itemIndex">Index of the item used as reference for the data manipulation</param>
        /// <param name="orientation">Panel orientation</param>
        /// <param name="scrollPos">Scroll state of the list (not-scrolled, scrolled)</param>
        /// <param name="scrollOffsetChangeExpected">True when the data manipulation is expected to result in a scroll offset change</param>
        protected void TestFlatDataManipulations(ListViewBase list, DataManipulationType dmType, int itemIndex, Orientation orientation, ScrollPosition scrollPos, bool scrollOffsetChangeExpected = true)
        {
            Point prevOffset = new Point();
            Point currOffset = new Point();

            // Perform data manipulation
            UIExecutor.Execute(() =>
            {
                // Save the current offset before manipulating item
                prevOffset = GetViewportOffset(list);

                // Now perform the data manipulation
                PerformFlatDataManipulation(list, dmType, itemIndex);
            });

            // Wait for few seconds to complete the transitions
            TestServices.WindowHelper.WaitForIdle();

            // Find the current offset and capture the current screenshot
            UIExecutor.Execute(() =>
            {
                currOffset = GetViewportOffset(list);
            });

            // Verify the viewport changes due to the data manipulation
            UIExecutor.Execute(() =>
            {
                // Verify the viewport offset
                VerifyViewport(dmType, prevOffset, currOffset, orientation, scrollPos, scrollOffsetChangeExpected);
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        /// <summary>
        /// Performs the given data manipulation in the list of grouped data and verifies the viewport and visual changes
        /// </summary>
        /// <param name="list">ListView</param>
        /// <param name="dmType">Data manipulation type</param>
        /// <param name="groupKey">Group key used as the reference for group manipulation</param>
        /// <param name="itemIndex">Index of the item used as reference for the items manipulation</param>
        /// <param name="orientation">Panel orientation</param>
        /// <param name="scrollPos">Scroll state of the list (not-scrolled, scrolled)</param>
        protected void TestGroupedDataManipulations(ListViewBase list, DataManipulationType dmType, int groupIndex, int itemIndex, Orientation orientation, ScrollPosition scrollPos)
        {
            Point prevOffset = new Point();
            Point currOffset = new Point();

            // Perform data manipulation
            UIExecutor.Execute(() =>
            {
                // Save the current offset before manipulating item
                prevOffset = GetViewportOffset(list);

                // Now perform the data manipulation
                PerformGroupedDataManipulation(list, dmType, groupIndex, itemIndex);
            });

            // Wait for few seconds to complete the transitions
            TestServices.WindowHelper.WaitForIdle();

            // Find the current offset and capture the current screenshot
            UIExecutor.Execute(() =>
            {
                currOffset = GetViewportOffset(list);
            });

            // Verify the viewport changes due to the data manipulation
            UIExecutor.Execute(() =>
            {
                // Verify the viewport offset
                VerifyViewport(dmType, prevOffset, currOffset, orientation, scrollPos);
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        /// <summary>
        /// Returns the group data model and its index from the given group key
        /// Rerturn -1 if no group is found from the group key
        /// </summary>
        /// <param name="collection">Grouped data collection</param>
        /// <param name="key">Group key to find</param>
        /// <param name="group">Returned Group</param>
        /// <returns>Index of the returned grou</returns>
        /// 
        protected int FindGroup(ObservableCollection<MoCoGroupedDataModel> collection, string key, out MoCoGroupedDataModel group)
        {
            group = null;
            if (collection != null)
            {
                int i = 0;
                foreach (var item in collection)
                {
                    if (item.Header == key)
                    {
                        group = item;
                        return i;
                    }
                    i++;
                }
            }
            return -1;
        }

        /// <summary>
        /// Performs the verification of the viewport changes after the data manipulation is done on the list
        /// </summary>
        /// <param name="dmType">Data manipulation just completed</param>
        /// <param name="prevOffset">Offset prior to data manipulation</param>
        /// <param name="currOffset">Offset after the data manipulation</param>
        /// <param name="orientation">Panel orientation</param>
        /// <param name="scrollPos">Scroll state of the List (such it is scrolled or not-scrolled)</param>
        /// <param name="scrollOffsetChangeExpected">True when the data manipulation is expected to result in a scroll offset change</param>
        protected void VerifyViewport(DataManipulationType dmType, Point prevOffset, Point currOffset, Orientation orientation, ScrollPosition scrollPos, bool scrollOffsetChangeExpected = true)
        {
            Log.Comment(string.Format("VerifyViewport. DataManipulationType:{0}, prevOffset:{1}, currOffset:{2}, orientation:{3}, scrollPos:{4}", dmType, prevOffset, currOffset, orientation, scrollPos));

            if (orientation == Orientation.Horizontal)
            {
                if (scrollOffsetChangeExpected)
                {
                    Verify.AreNotEqual(prevOffset.X, currOffset.X, "Viewport has not offset horizontally");
                }
                else
                {
                    Verify.AreEqual(prevOffset.X, currOffset.X, "Viewport has offset horizontally");
                }
                Verify.AreEqual(prevOffset.Y, currOffset.Y, "Unexpected vertical offset in Horizontal orientation");
            }
            else
            {
                if (scrollOffsetChangeExpected)
                {
                    Verify.AreNotEqual(prevOffset.Y, currOffset.Y, "Viewport has not offset vertically");
                }
                else
                {
                    Verify.AreEqual(prevOffset.Y, currOffset.Y, "Viewport has offset vertically");
                }
                Verify.AreEqual(prevOffset.X, currOffset.X, "Unexpected horizontal offset in Vertical orientation");
            }

            switch (dmType)
            {
                case DataManipulationType.INSERT:
                case DataManipulationType.INSERT_GROUP:
                case DataManipulationType.BULK_INSERT:
                case DataManipulationType.EXPAND:
                case DataManipulationType.EXPAND_GROUP:
                case DataManipulationType.BULK_EXPAND:
                {
                    if (scrollOffsetChangeExpected)
                    {
                        Verify.IsTrue(orientation == Orientation.Horizontal ? currOffset.X > prevOffset.X : currOffset.Y > prevOffset.Y, "Incorrect offset after inserting/expanding element");
                    }
                    break;
                }

                case DataManipulationType.REMOVE:
                case DataManipulationType.REMOVE_GROUP:
                case DataManipulationType.BULK_REMOVE:
                case DataManipulationType.COLLAPSE:
                case DataManipulationType.COLLAPSE_GROUP:
                case DataManipulationType.BULK_COLLAPSE:
                {
                    if (scrollOffsetChangeExpected)
                    {
                        Verify.IsTrue(orientation == Orientation.Horizontal ? currOffset.X < prevOffset.X : currOffset.Y < prevOffset.Y, "Incorrect offset after removing/collapsing element");
                    }
                    break;
                }

                default:
                {
                    Log.Error("Unknown data manipulation type: " + dmType.ToString());
                    break;
                }
            }
        }

        /// <summary>
        /// Returs the ItemsPanel's direction (ie., Horizontal or Vertical)
        /// </summary>
        /// <param name="list">ListView/GridView</param>
        /// <returns>ItemsPanel's orientation</returns>
        /// 
        protected Orientation GetPanelOrientation(ListViewBase list)
        {
            var isp = list.ItemsPanelRoot as ItemsStackPanel;

            return isp.Orientation;
        }

        /// <summary>
        /// Sets the ItemsPanel's direction (ie., Horizontal or Vertical)
        /// </summary>
        /// <param name="list">ListView/GridView</param>
        /// <returns>ItemsPanel's orientation</returns>
        /// 
        protected void SetPanelOrientation(ListViewBase list, Orientation orientation)
        {
            Verify.IsNotNull(list, "ListViewBase is null");
            Verify.IsNotNull(list.ItemsPanelRoot, "ItemsPanelRoot is null");

            var isp = list.ItemsPanelRoot as ItemsStackPanel;

            Verify.IsNotNull(isp, "ItemsStackPanel is null");

            isp.Orientation = orientation;
        }

        /// <summary>
        /// Returns the current offset of the viewport in the List's ScrollViewer
        /// </summary>
        /// <param name="list">ListView/GridView</param>
        /// <returns>Viewport offset coordinates</returns>
        /// 
        protected Point GetViewportOffset(ListViewBase list)
        {
            Point pt = new Point();
            ScrollViewer sv = list.FindElementOfTypeInSubtree<ScrollViewer>();
            Verify.IsNotNull(sv, "Unable to find ScrollViewer in the ListView/GridView");

            pt.X = sv.HorizontalOffset;
            pt.Y = sv.VerticalOffset;

            Log.Comment(string.Format("Current Offset: x={0}, y={1}", pt.X, pt.Y));
            return pt;
        }

        protected void DisplayCurrentOffset(ListViewBase list)
        {
            UIExecutor.Execute(() =>
            {
                string strOffset = string.Empty;
                ScrollViewer sv = list.FindElementOfTypeInSubtree<ScrollViewer>();
                if (sv != null)
                {
                    strOffset = string.Format("x={0}, y={1}", sv.HorizontalOffset, sv.VerticalOffset);
                }

                Log.Comment("Current Offset: " + strOffset);
            });
        }

        #endregion

        #region data

        protected const double LIST_WIDTH = 400;
        protected const double LIST_HEIGHT = 800;
        protected const double LIST_PADDING = 30;
        protected const double HEADER_WIDTH = 100;
        protected const double HEADER_HEIGHT = 50;
        protected const double ITEM_WIDTH = 100;
        protected const double ITEM_HEIGHT = 50;
        protected const double ITEM_GAP = 10;

        protected CollectionViewSource cvs = null;

        protected enum DataManipulationType
        {
            INSERT,
            INSERT_GROUP,
            BULK_INSERT,
            REMOVE,
            REMOVE_GROUP,
            BULK_REMOVE,
            EXPAND,
            BULK_EXPAND,
            EXPAND_GROUP,
            COLLAPSE,
            BULK_COLLAPSE,
            COLLAPSE_GROUP
        }

        protected enum ScrollPosition
        {
            INTERSECTING, // List has scrolled such that an item is intersecting the top edge
            MIDDLE // List has scrolled to some arbitrary item
        }

        #endregion
    }
}
