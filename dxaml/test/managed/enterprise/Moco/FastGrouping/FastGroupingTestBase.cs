// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.IO;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using WEX.Logging.Interop;
using WEX.TestExecution;
using Microsoft.UI.Xaml.Tests.Common;
using System.Threading;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.FastGrouping
{
    public abstract class FastGroupingTestBase : XamlTestsBase
    {
        public const int DefaultNumGroups = 5;
        public const int DefaultNumItemsPerGroup = 10;

        public void PerformScenarios(DataModel dataModel, Action OnLoaded, Action OnScrolledGroupIntoView, Action OnScrolledItemIntoView)
        {
            Log.Comment("1. Validate Layout OnLoaded");
            OnLoaded();

            Log.Comment("2. verify selections on moco");
            ValidateSelections(controlUnderTest);

            if (OnScrolledItemIntoView != null)
            {
                Log.Comment("3. verify dynamic add/remove from data source");
                ValidateDynamicAddRemove(controlUnderTest, dataModel, OnScrolledItemIntoView);
            }
            else
            {
                Log.Comment("skipping - 3. verify dynamic add/remove from data source");
            }

            if (OnScrolledGroupIntoView != null)
            {
                Log.Comment("4. verify dynamic add/remove group from data source");
                ValidateDynamicGroupAddRemove(controlUnderTest, dataModel, OnScrolledGroupIntoView);
            }
            else
            {
                Log.Comment("skipping - 4. verify dynamic add/remove group from data source");
            }
        }

        public void SetHidesIfEmpty(bool hidesIfEmpty)
        {
            UIExecutor.Execute(() =>
            {
                Log.Comment("0. Setting HidesIfEmpty=" + hidesIfEmpty.ToString());
                foreach (var style in controlUnderTest.GroupStyle)
                {
                    // coverage for getter and setter
#pragma warning disable 618
                    Style containerStyle = style.ContainerStyle;
                    style.ContainerStyle = containerStyle;
#pragma warning restore 618

                    style.HidesIfEmpty = (hidesIfEmpty == true);
                }
            });
        }

        protected void ValidateNumberOfItems(ControlUnderTest listViewBase, int items)
        {
            int numItems = (int)((IList<object>)listViewBase.Items).Count;
            Log.Comment("Number of items = " + numItems);
            Verify.IsTrue(numItems == items, "Number of items in the grid view is not " + items);
        }

        protected void ValidateVisibleIndeces(int firstVisibleIndex, int lastVisibleIndex)
        {
            UIExecutor.Execute(() =>
           {
               ListViewBase lvb = controlUnderTest.ControlInstance as ListViewBase;
               if (lvb != null)
               {
                   ItemsStackPanel isp = lvb.ItemsPanelRoot as ItemsStackPanel;
                   if (isp != null)
                   {
                       Log.Comment("ISP.FirstVisibleIndex:" + isp.FirstVisibleIndex);
                       Log.Comment("ISP.LastVisibleIndex:" + isp.LastVisibleIndex);

                       Verify.AreEqual<int>(isp.FirstVisibleIndex, firstVisibleIndex, "incorrect FirstVisibleIndex");
                       Verify.AreEqual<int>(isp.LastVisibleIndex, lastVisibleIndex, "incorrect LastVisibleIndex");
                   }

                   ItemsWrapGrid iwg = lvb.ItemsPanelRoot as ItemsWrapGrid;
                   if (iwg != null)
                   {
                       Log.Comment("IWG.FirstVisibleIndex:" + iwg.FirstVisibleIndex);
                       Log.Comment("IWG.LastVisibleIndex:" + iwg.LastVisibleIndex);

                       Verify.AreEqual<int>(iwg.FirstVisibleIndex, firstVisibleIndex, "incorrect FirstVisibleIndex");
                       Verify.AreEqual<int>(iwg.LastVisibleIndex, lastVisibleIndex, "incorrect LastVisibleIndex");
                   }

                   if (isp == null && iwg == null)
                   {
                       Verify.Fail("lvb.ItemsPanelRoot returned null");
                   }
               }
           });
        }

        private GroupedDataModel currentModel;
        protected void SetupMocoItemSource(DataModel dataModel, int numGroups, int numItemsPerGroup, int emptyGroupCluster = 0)
        {
            UIExecutor.Execute(() =>
            {
                switch (dataModel)
                {
                    case DataModel.OC_OC_IsA:
                        var isAViewModel = new ObservableIsAViewModel(numGroups, numItemsPerGroup, emptyGroupCluster);
                        cvs.Source = isAViewModel.GetGroupedDataSource();
                        this.currentModel = isAViewModel;
                        break;

                    //case DataModel.OC_OC_HasA:
                    //    var hasAViewModel = new HasAViewModel();
                    //    cvs.Source = hasAViewModel.GetGroupedDataSource();
                    //    cvs.ItemsPath = new PropertyPath("People");
                    //    this.currentModel = hasAViewModel;
                    //    break;

                    //case DataModel.IEnum_OC_IsA:
                    //    var ienumOC = new IsAIEnumerableGroupModel();
                    //    cvs.Source = ienumOC.GetGroupedDataSource();
                    //    this.currentModel = ienumOC;
                    //    break;

                    //case DataModel.OC_IEnum_IsA:
                    //    var OCIenum = new IsAIEnumerableItemsModel();
                    //    cvs.Source = OCIenum.GetGroupedDataSource();
                    //    this.currentModel = OCIenum;
                    //    break;

                    default:
                        throw new NotImplementedException("Unknown model" + dataModel.ToString());
                }

                cvs.IsSourceGrouped = true;
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        /// <summary>
        /// Note: This method requires a group named 'G:4' with 10 items in it
        /// </summary>
        protected void ValidateDynamicGroupAddRemove(ControlUnderTest listViewBase, DataModel dataModel, Action OnScrolledGroupIntoView)
        {
            int startItems = 0;

            // Add an EmptyGroup Named 'G:N' and scroll it into view
            UIExecutor.Execute(() =>
            {
                startItems = (int)((IList<object>)listViewBase.Items).Count;

                Log.Comment("Adding empty group named 'G:N'");
                currentModel.AddEmptyGroup("G:N");
                ValidateNumberOfItems(listViewBase, startItems);
            });

            UIExecutor.Execute(() =>
            {
                listViewBase.ScrollIntoView(currentModel.GetGroup("G:N"));
            });

            TestServices.WindowHelper.WaitForIdle();

            // Add 'I:0' to 'G:N'
            ObservableItem newItem = new ObservableItem() { ItemContent0 = "G:N", ItemContent1 = "I:0" };
            UIExecutor.Execute(() =>
            {
                startItems = (int)((IList<object>)listViewBase.Items).Count;

                Log.Comment("Adding item 'Item0' to empty group 'G:N'");
                currentModel.AddGroupItem(newItem.ItemContent0, newItem);
                if (dataModel != DataModel.IEnum_OC_IsA && dataModel != DataModel.OC_IEnum_IsA)
                {
                    ValidateNumberOfItems(listViewBase, startItems + 1);
                }
                else
                {
                    Log.Comment("when outer collection is IEnumerable, we wont see updates to the new group");
                    ValidateNumberOfItems(listViewBase, startItems);
                }
            });

            // scroll the group 4 intoview
            UIExecutor.Execute(() =>
            {
                startItems = (int)((IList<object>)listViewBase.Items).Count;

                Log.Comment("scroll group G:4 into view");
                var group = currentModel.GetGroup("G:4");
                listViewBase.ScrollIntoView(group);
                listViewBase.ControlInstance.UpdateLayout();
                ValidateNumberOfItems(listViewBase, startItems);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                OnScrolledGroupIntoView();
            });

            UIExecutor.Execute(() =>
            {
                startItems = (int)((IList<object>)listViewBase.Items).Count;

                Log.Comment("Clear the group 'G:4'");
                currentModel.ClearGroup("G:4");
                if (dataModel != DataModel.IEnum_OC_IsA && dataModel != DataModel.OC_IEnum_IsA)
                {
                    ValidateNumberOfItems(listViewBase, startItems - 11);
                }
                else
                {
                    Log.Comment("when outer collection is IEnumerable, we wont see updates to the new group");
                    ValidateNumberOfItems(listViewBase, startItems);
                }
            });

            // Add an item to the cleared group - 'Group 4'
            UIExecutor.Execute(() =>
            {
                startItems = (int)((IList<object>)listViewBase.Items).Count;

                Log.Comment("Adding item 'I:0' to cleared group - G:4");
                ObservableItem item0 = new ObservableItem() { ItemContent0 = "G:4", ItemContent1 = "I:0" };
                currentModel.AddGroupItem(item0.ItemContent0, item0);
                if (dataModel != DataModel.IEnum_OC_IsA && dataModel != DataModel.OC_IEnum_IsA)
                {
                    ValidateNumberOfItems(listViewBase, startItems + 1);
                }
                else
                {
                    Log.Comment("when outer collection is IEnumerable, we wont see updates to the new group");
                    ValidateNumberOfItems(listViewBase, startItems);
                }
            });

            // delete the group with 1 item - 'G:4'
            UIExecutor.Execute(() =>
            {
                startItems = (int)((IList<object>)listViewBase.Items).Count;

                Log.Comment("Delete the group (with 1 item)");
                currentModel.DeleteGroup("G:4");
                if (dataModel != DataModel.IEnum_OC_IsA && dataModel != DataModel.OC_IEnum_IsA)
                {
                    ValidateNumberOfItems(listViewBase, startItems - 1);
                }
                else
                {
                    Log.Comment("when outer collection is IEnumerable, we wont see updates to the new group");
                    ValidateNumberOfItems(listViewBase, startItems);
                }
            });
        }

        protected void ValidateReset(string groupName, Action OnReset)
        {
            ListViewBase listViewBase = controlUnderTest.ControlInstance as ListViewBase;

            UIExecutor.Execute(() =>
            {
                // scrolling group into view
                Log.Comment("Scrolling group " + groupName + " into view");
                var group = currentModel.GetGroup(groupName);
                listViewBase.ScrollIntoView(group);
                listViewBase.UpdateLayout();
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Reset " + groupName);
                currentModel.ResetGroup(groupName);
                listViewBase.UpdateLayout();
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                OnReset();
            });
        }

        protected void ValidateDynamicAddRemove(ControlUnderTest listViewBase, DataModel dataModel, Action OnItemScrolledIntoView)
        {
            int startItems = 0;
            UIExecutor.Execute(() =>
            {
                startItems = (int)((IList<object>)listViewBase.Items).Count;

                ObservableItem itemN = new ObservableItem() { ItemContent0 = "G:4", ItemContent1 = "I:N" };
                Log.Comment("Adding item 'I:N' to 'G:4' ");
                currentModel.AddGroupItem(itemN.ItemContent0, itemN);
                if (dataModel != DataModel.OC_IEnum_IsA)
                {
                    ValidateNumberOfItems(listViewBase, startItems + 1);
                }
                else
                {
                    Log.Comment("when using IEnumerable as inner collection, there will be no collection change events");
                    ValidateNumberOfItems(listViewBase, startItems);
                }
            });

            UIExecutor.Execute(() =>
            {
                Log.Comment("setting SelectedIndex to " + 4);
                listViewBase.ControlInstance.SelectedIndex = 4;
                // scrolling item into view
                Log.Comment("Scrolling selected item into view (item 4)");
                listViewBase.ScrollIntoView(listViewBase.ControlInstance.SelectedItem);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                OnItemScrolledIntoView();
            });

            UIExecutor.Execute(() =>
            {
                Log.Comment("Removing selected items");
                currentModel.RemoveSelectedGroupItems(listViewBase.SelectedItems);
                ValidateNumberOfItems(listViewBase, startItems);
            });
        }

        protected void ValidateSelections(ControlUnderTest listViewBase, int selectItemAtPosition = 5)
        {
            UIExecutor.Execute(() =>
            {
                Log.Comment("Set SelectionMode == Single");
                listViewBase.SetSingleSelectionMode();
            });

            VerifyListViewSingleSelect(listViewBase, selectItemAtPosition);
            VerifyListViewSingleSelect(listViewBase, 0);
        }

        protected void VerifyListViewSingleSelect(ControlUnderTest listViewBase, int selectItemAtPosition)
        {
            UIExecutor.Execute(() =>
            {
                Log.Comment("setting SelectedIndex to " + selectItemAtPosition);
                listViewBase.ControlInstance.SelectedIndex = selectItemAtPosition;
            });

            Log.Comment("Verify");
            UIExecutor.Execute(() =>
            {
                uint selectedItemsCount = (uint)((IList<Object>)listViewBase.SelectedItems).Count;
                int selectedIndex = listViewBase.ControlInstance.SelectedIndex;
                if (selectedItemsCount == 1 && selectedIndex == selectItemAtPosition)
                {
                    Log.Comment("Verified");
                }
                else
                {
                    throw new Exception("Single Selection at index " + selectItemAtPosition + " failed");
                }
            });
        }

        protected void VerifyListViewMultipleSelect(ControlUnderTest ctrl, int selectItemAtPosition, int numberOfItems, PointerInputType inputType)
        {

            ListViewBase lvb = ctrl.ControlInstance as ListViewBase;
            int retryCount = 0;
            bool done = false;
            while (retryCount++ < 3 && !done)
            {
                try
                {
                    Log.Comment("Attempt " + retryCount);

                    UIExecutor.Execute(() =>
                    {
                        lvb.SelectedItems.Clear();
                        lvb.ScrollIntoView(lvb.Items[selectItemAtPosition]);
                    });

                    TestServices.WindowHelper.WaitForIdle();
                    List<object> items = new List<object>();

                    for (int index = selectItemAtPosition; index < selectItemAtPosition + numberOfItems; index++)
                    {
                        UIExecutor.Execute(() =>
                        {
                            Log.Comment("Selecting item at " + index);
                            var container = lvb.ContainerFromIndex(index);
                            TestServices.InputHelper.Tap(container as FrameworkElement);
                            items.Add(lvb.Items[index]);
                            Log.Comment("Selected item at " + index);
                        });

                        TestServices.WindowHelper.WaitForIdle();
                    }

                    Log.Comment("Verify");
                    TestServices.WindowHelper.WaitForIdle();
                    UIExecutor.Execute(() =>
                    {
                        uint selectedItemsCount = (uint)((IList<Object>)lvb.SelectedItems).Count;
                        if (selectedItemsCount == numberOfItems)
                        {
                            Log.Comment("Selected number of items matched expectation");
                            int foundCount = 0;
                            foreach (var item in items)
                            {
                                if (lvb.SelectedItems.Contains(item))
                                {
                                    Log.Comment("Found item");
                                    foundCount++;
                                }
                            }

                            if (foundCount == numberOfItems)
                                done = true;
                        }
                        else
                        {
                            Log.Comment("SeletedItemsCount: Expected - " + numberOfItems + " Actual -" + selectedItemsCount);
                        }
                    });
                }
                catch (Exception ex)
                {
                    Log.Comment("Ignoring exception but logging...");
                    Log.Comment(ex.ToString());
                }
            }

            if (!done)
            {
                throw new Exception("Multiple Selection at index " + selectItemAtPosition + " failed");
            }
        }

        protected void ValidateXamlTestPage(Panel host, StackPanel targetPanel)
        {
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(host, "hostPanel is null");
                cvs = (CollectionViewSource)targetPanel.FindName("cvs");
                Verify.IsNotNull(cvs, "FindName('cvs') returned null");
                Selector ctrl = (Selector)targetPanel.FindName("moco");
                controlUnderTest = new ControlUnderTest(ctrl);
                Verify.IsNotNull(controlUnderTest, "FindName('moco') returned null");
            });
        }

        protected void LoadXaml(string templateName, string itemsPanel, string itemsPanelOrientation, string mocoProperties = "", string gridResources = "")
        {
            string text = "";
            string path = Path.Combine(TestDeploymentDir, @"resources\managed\enterprise\moco\" + templateName + ".xaml");
            text = File.ReadAllText(path);
            text = text.Replace("[[ItemsPanel]]", itemsPanel);
            text = text.Replace("[[ItemsPanel_Orientation]]", itemsPanelOrientation);
            text = text.Replace("[[Moco_Properties]]", mocoProperties);
            text = text.Replace("[[GridResources]]", gridResources);

            LoadXamlText(text);
            TestServices.WindowHelper.WaitForIdle();
        }

        protected void LoadXamlText(string xaml)
        {
            StackPanel targetPanel = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading xaml: ");
                Log.Comment(xaml);
                targetPanel = (StackPanel)XamlReader.Load(xaml);
                Verify.IsNotNull(targetPanel, "Unable to load xaml.");

                TestServices.WindowHelper.WindowContent = targetPanel;
            });

            UIExecutor.Execute(() =>
            {
                cvs = (CollectionViewSource)targetPanel.FindName("cvs");
                Verify.IsNotNull(cvs, "FindName('cvs') returned null");
                Selector ctrl = targetPanel.FindName("moco") as Selector;
                controlUnderTest = new ControlUnderTest(ctrl);
                Verify.IsNotNull(controlUnderTest, "FindName('moco') returned null");
            });

            Log.Comment("Xaml Load Succeeded");
        }

        public List<ObservableItem> GetItemsInViewPort()
        {
            UIElement panel = VisualTreeUtils.FindElementOfTypeInSubtree<ItemsStackPanel>(controlUnderTest.ControlInstance);
            if (panel == null)
                panel = VisualTreeUtils.FindElementOfTypeInSubtree<ItemsWrapGrid>(controlUnderTest.ControlInstance);

            var count = VisualTreeHelper.GetChildrenCount(panel);
            List<ObservableItem> items = new List<ObservableItem>();
            for (int i = 0; i < count; i++)
            {
                var vi = VisualTreeHelper.GetChild(panel, i) as SelectorItem;
                if (vi != null)
                {
                    if (vi.IsHitTestVisible && IsItemInViewPort(controlUnderTest.ControlInstance, vi))
                        items.Add(vi.Content as ObservableItem);
                }
            }

            return items;
        }

        /// <summary>
        /// Is item currently in the selector control's ScrollViewer viewport
        /// </summary>
        public bool IsItemInViewPort(Selector selector, SelectorItem item)
        {
            bool inViewport = false;

            ScrollViewer scrollviewer = selector.FindElementOfTypeInSubtree<ScrollViewer>();

            var gt = item.TransformToVisual(scrollviewer);
            var containerOffset = gt.TransformPoint(new Point());

            if (containerOffset.X + item.ActualWidth >= 0 && containerOffset.X < scrollviewer.ActualWidth &&
                containerOffset.Y + item.ActualHeight >= 0 && containerOffset.Y < scrollviewer.ActualHeight)
            {
                inViewport = true;
            }

            return inViewport;
        }

        public void SetGroupHeaderPlacement(GroupHeaderPlacement groupHeaderPlacement)
        {
            UIExecutor.Execute(() =>
            {
                Panel itemsPanel = controlUnderTest.ControlInstance.ItemsPanelRoot;

                if (itemsPanel is ItemsWrapGrid)
                {
                    (itemsPanel as ItemsWrapGrid).GroupHeaderPlacement = groupHeaderPlacement;
                }
                else if (itemsPanel is ItemsStackPanel)
                {
                    (itemsPanel as ItemsStackPanel).GroupHeaderPlacement = groupHeaderPlacement;
                }
            });
        }

        protected void PanLeftToBegining()
        {
            ScrollViewer sv = null;
            ListViewBase listViewBase = controlUnderTest.ControlInstance as ListViewBase;
            ManualResetEvent viewChanged = new ManualResetEvent(false);

            UIExecutor.Execute(() =>
            {
                sv = VisualTreeUtils.FindElementOfTypeInSubtree<ScrollViewer>(listViewBase);
                Verify.IsTrue(sv != null, "Could not find the scrollviewer in the control");
                Verify.IsTrue(sv != null, "Could not find the scrollviewer in the control");

                viewChanged.Reset();
                // go to the begining
                sv.ViewChanged += (sender, args) =>
                {
                    viewChanged.Set();
                };

                sv.ChangeView(0, null, null, true);
            });

            TestServices.WindowHelper.WaitForIdle();
            Verify.IsTrue(viewChanged.WaitOne(TimeSpan.FromSeconds(5)), "ViewChanged event was not raised");
        }

        protected void PanRightToEnd()
        {
            ScrollViewer sv = null;
            ListViewBase listViewBase = controlUnderTest.ControlInstance as ListViewBase;
            ManualResetEvent viewChanged = new ManualResetEvent(false);

            UIExecutor.Execute(() =>
            {
                sv = VisualTreeUtils.FindElementOfTypeInSubtree<ScrollViewer>(listViewBase);
                Verify.IsTrue(sv != null, "Could not find the scrollviewer in the control");

                // go to the end
                sv.ViewChanged += (sender, args) =>
                {
                    viewChanged.Set();
                };

                sv.ChangeView(sv.ExtentWidth, null, null, true);
            });

            TestServices.WindowHelper.WaitForIdle();
            Verify.IsTrue(viewChanged.WaitOne(TimeSpan.FromSeconds(5)), "ViewChanged event was not raised");
        }

        public void VerifyKeyNavOrder(bool navigateForward, List<string> inputKeys, List<Tuple<Type, string>> expectedFocusOrder)
        {
            Panel hostPanel = null;

            UIExecutor.Execute(() =>
            {
                ListViewBase lvb = controlUnderTest.ControlInstance as ListViewBase;
                lvb.Header = new Button() { Content = "Header" };
                lvb.Footer = new Button() { Content = "Footer" };

                hostPanel = lvb.Parent as Panel;
                Button afterButton = new Button() { Content = "After" };
                hostPanel.Children.Add(afterButton);

                Button beforeButton = new Button() { Content = "Before" };
                hostPanel.Children.Insert(0, beforeButton);
                hostPanel.UpdateLayout();

                if (navigateForward)
                {
                    beforeButton.Focus(FocusState.Programmatic);
                }
                else
                {
                    afterButton.Focus(FocusState.Programmatic);
                }
            });

            TestServices.WindowHelper.WaitForIdle();

            for (int i = 0; i < inputKeys.Count; i++)
            {
                Log.Comment("Sending input key: " + inputKeys[i]);
                switch (inputKeys[i])
                {
                    case "shifttab":
                        TestServices.KeyboardHelper.ShiftTab();
                        break;

                    case "tab":
                        TestServices.KeyboardHelper.Tab();
                        break;

                    case "left":
                        TestServices.KeyboardHelper.Left();
                        break;

                    case "right":
                        TestServices.KeyboardHelper.Right();
                        break;

                    case "up":
                        TestServices.KeyboardHelper.Up();
                        break;

                    case "down":
                        TestServices.KeyboardHelper.Down();
                        break;

                    case "pageup":
                        TestServices.KeyboardHelper.PressKeySequence("$d$_pageup#$u$_pageup");
                        break;

                    case "pagedown":
                        TestServices.KeyboardHelper.PressKeySequence("$d$_pagedown#$u$_pagedown");
                        break;

                    case "home":
                        TestServices.KeyboardHelper.PressKeySequence("$d$_home#$u$_home");
                        break;

                    case "end":
                        TestServices.KeyboardHelper.PressKeySequence("$d$_end#$u$_end");
                        break;

                    default:
                        TestServices.KeyboardHelper.PressKeySequence(inputKeys[i]);
                        break;
                }

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    hostPanel.UpdateLayout();
                    object focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);

                    Verify.IsNotNull(focusedElement, "focusedElement is null");
                    Log.Comment("Focused element: " + (focusedElement as ContentControl).GetType().Name + " " + (focusedElement as ContentControl).Content.ToString());

                    Verify.AreEqual<Type>(expectedFocusOrder[i].Item1, focusedElement.GetType(), "Focused element type is unexpected");

                    string content = null;
                    if (focusedElement is ContentControl)
                    {
                        content = (focusedElement as ContentControl).Content.ToString();
                    }
                    else
                    {
                        content = focusedElement.ToString();
                    }
                    Verify.AreEqual<string>(expectedFocusOrder[i].Item2, content, "Focused element content is unexpected");
                });

                TestServices.WindowHelper.WaitForIdle();
            }
        }

        protected CollectionViewSource cvs = null;
        protected ControlUnderTest controlUnderTest = null;
        protected static string TestDeploymentDir { get; set; }
    }

    public enum PointerInputType
    {
        Touch,
        Mouse
    }
    public enum DataModel
    {
        OC_OC_IsA,
        OC_OC_HasA,
        OC_IEnum_IsA,
        IEnum_OC_IsA
    }

    public class ControlUnderTest
    {
        public Selector ControlInstance { get; set; }
        public ControlUnderTest(Selector instance)
        {
            this.ControlInstance = instance;
        }
        public void SetSingleSelectionMode()
        {
            var listViewBase = ControlInstance as ListViewBase;
            if (listViewBase != null)
            {
                listViewBase.SelectionMode = ListViewSelectionMode.Single;
            }
            else
            {
                var listBox = ControlInstance as ListBox;
                if (listBox != null)
                {
                    listBox.SelectionMode = SelectionMode.Single;
                }
                else
                {
                    throw new InvalidOperationException("Only listviewbase and listbox are supported as controls under test");
                }
            }
        }

        public void ScrollIntoView(object item)
        {
            var listViewBase = ControlInstance as ListViewBase;
            if (listViewBase != null)
            {
                listViewBase.ScrollIntoView(item);
            }
            else
            {
                var listBox = ControlInstance as ListBox;
                if (listBox != null)
                {
                    listBox.ScrollIntoView(item);
                }
                else
                {
                    throw new InvalidOperationException("Only listviewbase and listbox are supported as controls under test");
                }
            }
        }

        public IList<object> SelectedItems
        {
            get
            {
                var listViewBase = ControlInstance as ListViewBase;
                if (listViewBase != null)
                {
                    return listViewBase.SelectedItems;
                }
                else
                {
                    var listBox = ControlInstance as ListBox;
                    if (listBox != null)
                    {
                        return listBox.SelectedItems;
                    }
                    else
                    {
                        throw new InvalidOperationException("Only listviewbase and listbox are supported as controls under test");
                    }
                }
            }
        }

        public IObservableVector<GroupStyle> GroupStyle
        {
            get
            {
                var itemsControl = ControlInstance as ItemsControl;
                if (itemsControl != null)
                {
                    return itemsControl.GroupStyle;
                }
                else
                {
                    throw new InvalidOperationException("Unable to cast ControlInstance to ItemsControl to get GroupStyle");
                }
            }
        }

        public IList<object> Items
        {
            get
            {
                var itemsControl = ControlInstance as ItemsControl;
                if (itemsControl != null)
                {
                    return itemsControl.Items;
                }
                else
                {
                    throw new InvalidOperationException("Unable to cast ControlInstance to ItemsControl");
                }
            }
        }

        internal void ScrollItemIntoView(int index)
        {
            var listViewBase = ControlInstance as ListViewBase;
            if (listViewBase != null)
            {
                var item = ((IList<object>)listViewBase.Items)[index];
                listViewBase.ScrollIntoView(item);
            }
            else
            {
                var listBox = ControlInstance as ListBox;
                if (listBox != null)
                {
                    var item = ((IList<object>)listBox.Items)[index];
                    listBox.ScrollIntoView(item);
                }
                else
                {
                    // throw new InvalidOperationException("Only listviewbase and listbox are supported as controls under test");
                    Log.Warning("Not scrolling into view becuause the control is not a listviewbase or listbox");
                }
            }
        }
    }
}
