// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Private.Infrastructure;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Enterprise.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco
{
    [TestClass]
    public partial class ResetTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void VerifyNonGroupedReset()
        {
            Dictionary<int, ListViewItem> recycleQueue = null;
            ListView lv = null;
            ScrollViewer sv = null;
            ListViewItem focusedContainer = null;
            ListViewItem topContainer = null;
            int focusedItem = -1;
            NonGroupedICollectionViewImplementation src = null;

            int counterFoundPerfectContainer = 0;
            int counterGoingWithSuggestion = 0;
            int counterCreatingNewLVI = 0;
            int counterUsingImperfectContainer = 0;

            bool ignoreEvents = false;

            UIExecutor.Execute(() =>
            {
                lv = (ListView)XamlReader.Load(@"<ListView
                                                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                                     <ListView.Resources>
                                                         <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>
                                                     </ListView.Resources>
                                                 </ListView>");
                List<object> tst = new List<object>();
                for (int i = 0; i < 300; i++)
                {
                    tst.Add(i);
                }
                recycleQueue = new Dictionary<int, ListViewItem>();

                lv.ShowsScrollingPlaceholders = false;
                src = new NonGroupedICollectionViewImplementation(tst);
                lv.ItemsSource = src;
                lv.Height = 500;

                lv.ChoosingItemContainer += (s, args) =>
                    {
                        if (!ignoreEvents)
                        {
                            if (recycleQueue.ContainsKey((int)args.Item))
                            {
                                // found the perfect container
                                args.ItemContainer = recycleQueue[(int)args.Item];
                                recycleQueue.Remove((int)args.Item);

                                counterFoundPerfectContainer++;
                            }
                            else if (args.ItemContainer != null)
                            {
                                // we got a suggestion and we are going to use it
                                // remove old one if it exists
                                recycleQueue.Remove(Int32.Parse((args.ItemContainer.ContentTemplateRoot as TextBlock).Text));

                                counterGoingWithSuggestion++;
                            }
                            if (args.ItemContainer == null)
                            {
                                if (recycleQueue.Count() == 0)
                                {
                                    // create new
                                    args.ItemContainer = new ListViewItem();
                                    counterCreatingNewLVI++;
                                }
                                else
                                {
                                    // found random container
                                    args.ItemContainer = recycleQueue.First().Value;
                                    recycleQueue.Remove(recycleQueue.First().Key);

                                    counterUsingImperfectContainer++;
                                }
                            }
                        }
                        Log.Comment("index: " + args.ItemIndex + " is being generated, Item is: " + args.Item.ToString() + " and args.container = " + args.ItemContainer.GetHashCode());
                    };

                lv.ContainerContentChanging += (s, args) =>
                    {
                        if (args.InRecycleQueue)
                        {
                            if (!ignoreEvents)
                            {
                                Log.Comment("index: " + args.ItemIndex + " is being recycled. Item is: " + args.Item.ToString() + " container is " + args.ItemContainer.GetHashCode());
                                recycleQueue.Add(Int32.Parse((args.ItemContainer.ContentTemplateRoot as TextBlock).Text), args.ItemContainer as ListViewItem);
                            }
                        }
                        else
                        {
                            Log.Comment("index: " + args.ItemIndex + " is being contentbound. Item is: " + args.Item.ToString() + " current content of container is " + (args.ItemContainer.ContentTemplateRoot as TextBlock).Text);
                            (args.ItemContainer.ContentTemplateRoot as TextBlock).Text = args.Item.ToString();
                        }
                    };

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                sv = VisualTreeUtils.FindElementOfTypeInSubtree<ScrollViewer>(lv);
                Verify.IsTrue(sv != null, "found the scrollviewer in the control");

                // focus container 5
                focusedItem = 5;
                focusedContainer = lv.ContainerFromItem(focusedItem) as ListViewItem;
                focusedContainer.Focus(FocusState.Keyboard);
                // after focus, always introduce a wait for idle, since it is async
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                // scroll down quite a bit
                lv.ScrollIntoView(200, ScrollIntoViewAlignment.Leading);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => sv.ChangeView(null, sv.VerticalOffset + 5, null, true)); // scroll a few pixels so that container 200 is at an offset
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                topContainer = lv.ContainerFromItem(200) as ListViewItem;
                double offset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Verify.IsTrue(offset == -5, "offset at -5");

                // validation check
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is at container 5");

                // *********************************************************
                Log.Comment("starting to do a regular reset, no mutation");

                counterFoundPerfectContainer = 0;
                counterGoingWithSuggestion = 0;
                counterCreatingNewLVI = 0;
                counterUsingImperfectContainer = 0;

                src.RaiseReset();
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // ended up finding all containers
                Verify.IsTrue(recycleQueue.Count() == 0, "recycle queue is completely empty");

                // still have focus on the correct container
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is still on the correct container");

                // top container still the same
                Verify.IsTrue(topContainer == lv.ContainerFromItem(200) as ListViewItem, "top container still same identity");

                // ended up on correct position
                double offset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Verify.IsTrue(offset == -5, "top container still at -5 offset");

                Verify.IsTrue(counterGoingWithSuggestion + counterCreatingNewLVI + counterUsingImperfectContainer == 0, "all imperfect counters were not hit");
                Verify.IsTrue(counterFoundPerfectContainer == 60, "the perfect counter was hit correctly"); // one page is 10, 4 pages of cache, plus first container and focused container

            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                ignoreEvents = true;
                counterFoundPerfectContainer = 0;

                // *********************************************************
                Log.Comment("starting to do a regular reset, no mutation, no smart recycle queue work");
                src.RaiseReset();

            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // still have focus on the correct container
                // this is achieved by recycling and generating immediately after each other
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is still on the correct container");

                // top container is _not_ the same because we do not understand identity without help
                Verify.IsTrue(topContainer != lv.ContainerFromItem(200) as ListViewItem, "top container is not same identity");
                topContainer = lv.ContainerFromItem(200) as ListViewItem;

                // ended up on correct position
                double offset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Verify.IsTrue(offset == -5, "top container still at -5 offset");
            });


            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // *********************************************************
                Log.Comment("starting to mutate collection by inserting 3 items at beginning, smart recycling time");

                ignoreEvents = false;
                List<object> tst = new List<object>();
                tst.Add(-5);
                tst.Add(-4);
                tst.Add(-3);
                tst.Add(-2);
                tst.Add(-1);
                for (int i = 0; i < 300; i++)
                {
                    tst.Add(i);
                }
                src.ReplaceBackingAndReset(tst);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // ended up finding all containers
                Verify.IsTrue(recycleQueue.Count() == 0, "recycle queue is completely empty");

                // still have focus on the correct container
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is still on the correct container");

                // top container still the same
                Verify.IsTrue(topContainer == lv.ContainerFromItem(200) as ListViewItem, "top container still same identity");

                // ended up on correct position
                double offset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Verify.IsTrue(offset == -5, "top container still at -5 offset");

                // so interesting, we regenerate the special container as well, which makes sense: we have the correct new index
                // but it ends up no longer being the special container in this particular scenario. That is fine, it will be removed by
                // prunepinnedcontainers later.
                Verify.IsTrue(counterGoingWithSuggestion + counterCreatingNewLVI + counterUsingImperfectContainer == 3, "imperfect counters");
                Verify.IsTrue(counterFoundPerfectContainer == 58, "perfect counter");

            });

        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCVSGroupedReset()
        {
            ListView lv = null;
            ScrollViewer sv = null;
            ListViewItem focusedContainer = null;
            ListViewItem topContainer = null;
            Employee focusedItem = null;
            CollectionViewSource cvs = null;
            ObservableCollection<Manager> managers = null;
            double offset = 0;

            UIExecutor.Execute(() =>
            {
                cvs = new CollectionViewSource();
                managers = new ObservableCollection<Manager>();
                lv = new ListView();
                for (int i = 0; i < 100; ++i)
                {
                    Manager manager = managers.Where((m) => m.Age == i % 10).FirstOrDefault();
                    if (manager == null)
                    {
                        manager = new Manager();
                        manager.Age = i % 10;
                        managers.Add(manager);
                    }
                    manager.Employees.Add(new Employee() { Age = i });
                }

                cvs.IsSourceGrouped = true;
                cvs.ItemsPath = new PropertyPath("Employees");
                cvs.Source = managers;

                lv.ItemsSource = cvs.View;

                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;

                lv.GroupStyle.Add((GroupStyle) Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding Age}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         " ));

                lv.ItemTemplate = (DataTemplate) Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
                   "     <TextBlock Text='{Binding Age}' />  " +
                   " </DataTemplate>                         " );

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                sv = VisualTreeUtils.FindElementOfTypeInSubtree<ScrollViewer>(lv);
                Verify.IsTrue(sv != null, "found the scrollviewer in the control");

                // focus container
                focusedItem = managers[0].Employees[5];
                focusedContainer = lv.ContainerFromItem(focusedItem) as ListViewItem;
                focusedContainer.Focus(FocusState.Keyboard);
                // after focus, always introduce a wait for idle, since it is async
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                // scroll down quite a bit
                lv.ScrollIntoView(managers[2].Employees[2], ScrollIntoViewAlignment.Leading);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
                {
                    topContainer = lv.ContainerFromItem(managers[2].Employees[2]) as ListViewItem;
                    offset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                    Log.Comment("Offset of top container is " + offset);
                    sv.ChangeView(null, sv.VerticalOffset + 5, null, true);
                }); // scroll a few pixels so that container is at an offset
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                double newoffset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Log.Comment("Offset of top container (after scrolling 5 pixels) is " + newoffset);
                Verify.IsTrue(offset - newoffset == 5, "offset at 5");
                offset = newoffset;

                // validation check
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is still at correct container");

                // *********************************************************
                Log.Comment("starting to do an inner reset, no mutation, this does not do our reset functionality");
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                managers[1].Employees.Clear();
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                double newoffset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Log.Comment("Offset of top container (after clear) is " + newoffset);
                Verify.IsTrue(offset == newoffset, "top container still at correct offset");
                // still have focus on the correct container
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is still on the correct container");

                // top container still the same
                Verify.IsTrue(topContainer == lv.ContainerFromItem(managers[2].Employees[2]) as ListViewItem, "top container still same identity");

                // *********************************************************
                Log.Comment("starting to do an outer clear, this does translate into reset, but just validating nothing breaks");

                lv.ChoosingItemContainer += ((s, args) =>
                    {
                        Verify.Fail("do not expect to regenerate anything!");
                    });

                managers.Clear();

            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCustomGroupedReset()
        {
            Dictionary<int, ListViewItem> recycleQueue = null;
            ListView lv = null;
            ScrollViewer sv = null;
            ListViewItem focusedContainer = null;
            ListViewItem topContainer = null;
            int focusedItem = -1;
            GroupedICollectionViewImplementation groups = null;
            double offset = 0;

            int counterFoundPerfectContainer = 0;
            int counterGoingWithSuggestion = 0;
            int counterCreatingNewLVI = 0;
            int counterUsingImperfectContainer = 0;

            bool ignoreEvents = false;

            UIExecutor.Execute(() =>
            {
                lv = (ListView)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                    "<ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>" +
                    "  <ListView.Resources>" +
                    "    <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" +
                    "  </ListView.Resources>" +
                    "</ListView>");

                groups = new GroupedICollectionViewImplementation();
                for (int ix = 0; ix < 135; ++ix)
                {
                    IList<object> group = new List<object>();
                    for (int i = ix * 10; i < ix * 10 + 8; i++)
                    {
                        group.Add(i);
                    }

                    groups.Add(new InnerObservableVector("group for " + ix.ToString(), group));
                }

                lv.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderContainerStyle>                                                 " +
                   "         <Style TargetType='ListViewHeaderItem'>                                       " +
                   "             <Setter Property='Margin' Value='0,0,0,0' />                              " +
                   "             <Setter Property='Padding' Value='12,16,12,0' />                          " +
                   "             <Setter Property='Height' Value='52' />                                   " +
                   "         </Style>                                                                      " +
                   "     </GroupStyle.HeaderContainerStyle>                                                " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>        " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                lv.ItemsSource = groups;
                recycleQueue = new Dictionary<int, ListViewItem>();

                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;

                lv.ChoosingItemContainer += (s, args) =>
                {
                    Verify.IsNotNull(args, "args is null");
                    Verify.IsNotNull(args.Item, "item is null");
                    if (!ignoreEvents)
                    {
                        if (recycleQueue.ContainsKey((int)args.Item))
                        {
                            // found the perfect container
                            args.ItemContainer = recycleQueue[(int)args.Item];
                            recycleQueue.Remove((int)args.Item);

                            counterFoundPerfectContainer++;
                        }
                        else if (args.ItemContainer != null)
                        {
                            // we got a suggestion and we are going to use it
                            // remove old one if it exists
                            Verify.IsNotNull(args.ItemContainer.ContentTemplateRoot, "content template root is null");
                            recycleQueue.Remove(Int32.Parse((args.ItemContainer.ContentTemplateRoot as TextBlock).Text));

                            counterGoingWithSuggestion++;
                        }
                        if (args.ItemContainer == null)
                        {
                            if (recycleQueue.Count() == 0)
                            {
                                // create new
                                args.ItemContainer = new ListViewItem();
                                counterCreatingNewLVI++;
                            }
                            else
                            {
                                // found random container
                                args.ItemContainer = recycleQueue.First().Value;
                                recycleQueue.Remove(recycleQueue.First().Key);

                                counterUsingImperfectContainer++;
                            }
                        }
                    }
                    Log.Comment("index: " + args.ItemIndex + " is being generated, Item is: " + args.Item.ToString() + " and args.container = " + args.ItemContainer.GetHashCode());
                };

                lv.ContainerContentChanging += (s, args) =>
                {
                    Verify.IsNotNull(args, "args is null in ccc");
                    if (args.InRecycleQueue)
                    {
                        if (!ignoreEvents)
                        {
                            Verify.IsNotNull(args.Item, "args.item is null in ccc");
                            Verify.IsNotNull(args.ItemContainer, "args.itemcontainer is null in ccc");
                            Verify.IsNotNull(args.ItemContainer.ContentTemplateRoot, "content template root is null in ccc");
                            Log.Comment("index: " + args.ItemIndex + " is being recycled. Item is: " + args.Item.ToString() + " container is " + args.ItemContainer.GetHashCode());
                            recycleQueue.Add(Int32.Parse((args.ItemContainer.ContentTemplateRoot as TextBlock).Text), args.ItemContainer as ListViewItem);
                        }
                    }
                    else
                    {
                        Verify.IsNotNull(args.Item, "args.item is null in ccc");
                        Verify.IsNotNull(args.ItemContainer, "args.itemcontainer is null in ccc");
                        Verify.IsNotNull(args.ItemContainer.ContentTemplateRoot, "content template root is null in ccc");
                        Log.Comment("index: " + args.ItemIndex + " is being contentbound. Item is: " + args.Item.ToString() + " current content of container is " + (args.ItemContainer.ContentTemplateRoot as TextBlock).Text);
                        (args.ItemContainer.ContentTemplateRoot as TextBlock).Text = args.Item.ToString();
                    }
                };

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                sv = VisualTreeUtils.FindElementOfTypeInSubtree<ScrollViewer>(lv);
                Verify.IsTrue(sv != null, "found the scrollviewer in the control");


                // focus container 5
                focusedItem = 5;
                focusedContainer = lv.ContainerFromItem(focusedItem) as ListViewItem;
                focusedContainer.Focus(FocusState.Keyboard);
                // after focus, always introduce a wait for idle, since it is async
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                // scroll down quite a bit
                lv.ScrollIntoView(940, ScrollIntoViewAlignment.Leading);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
                {
                    Log.Comment("done - expecting 940 at the top");
                    topContainer = lv.ContainerFromItem(940) as ListViewItem;
                    offset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                });
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => sv.ChangeView(null, sv.VerticalOffset + 5, null, true)); // scroll a few pixels so that container 200 is at an offset
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {

                double newoffset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Verify.IsTrue(newoffset - offset == -5, "offset at -5");
                offset = newoffset;

                // validation check
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is at container 5");

                // *********************************************************
                Log.Comment("starting to do a regular reset, no mutation");

                counterFoundPerfectContainer = 0;
                counterGoingWithSuggestion = 0;
                counterCreatingNewLVI = 0;
                counterUsingImperfectContainer = 0;

                // note: first do a reset from the innergroups
                (groups.CollectionGroups as ObservableVector<object>).RaiseVectorChanged(CollectionChange.Reset, 0);
                // the above allowed us to prime the correct group reset order

                // now raise reset from main collection
                groups.RaiseReset(0);
                // the above will actually kick off our work
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // ended up finding all containers
                Verify.IsTrue(recycleQueue.Count() == 0, "recycle queue is completely empty");

                // still have focus on the correct container
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is still on the correct container");

                // top container still the same
                Verify.IsTrue(topContainer == lv.ContainerFromItem(940) as ListViewItem, "top container still same identity");

                // ended up on correct position
                double newoffset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Verify.IsTrue(offset == newoffset, "top container still at -5 offset");

                Verify.IsTrue(counterGoingWithSuggestion + counterCreatingNewLVI + counterUsingImperfectContainer == 0, "all imperfect counters were not hit");
                Verify.IsTrue(counterFoundPerfectContainer == 53, "the perfect counter was hit correctly");

            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                ignoreEvents = true;
                counterFoundPerfectContainer = 0;

                // *********************************************************
                Log.Comment("starting to do a regular reset, no mutation, no smart recycle queue work");

                // note: first do a reset from the innergroups
                (groups.CollectionGroups as ObservableVector<object>).RaiseVectorChanged(CollectionChange.Reset, 0);
                // the above allowed us to prime the correct group reset order

                // now raise reset from main collection
                groups.RaiseReset(0);
                // the above will actually kick off our work

            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // still have focus on the correct container
                // this is achieved by recycling and generating immediately after each other
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is still on the correct container");

                // top container is _not_ the same because we do not understand identity without help
                Verify.IsTrue(topContainer != lv.ContainerFromItem(940) as ListViewItem, "top container is not same identity");
                topContainer = lv.ContainerFromItem(940) as ListViewItem;

                // ended up on correct position
                double newoffset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Verify.IsTrue(offset == newoffset, "top container still at -5 offset");
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // *********************************************************
                Log.Comment("starting to mutate collection smart recycling time");

                ignoreEvents = false;
                IObservableVector<object> newgroups = new ObservableVector<object>();
                for (int ix = 0; ix < 135; ++ix)
                {
                    // first 10 groups will get one extra. This is nice, so that our view (around item 940)
                    // is all perfectly recycleable
                    IList<object> group = new List<object>();
                    for (int i = ix * 10; i < ix * 10 + (ix < 10 ? 9 : 8) ; i++)
                    {
                        group.Add(i);
                    }

                    newgroups.Add(new InnerObservableVector("group for " + ix.ToString(), group));
                }
                groups.NewGroupsAndReset(newgroups);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // ended up finding all containers
                Verify.IsTrue(recycleQueue.Count() == 0, "recycle queue is completely empty");

                // still have focus on the correct container
                Verify.AreEqual(focusedContainer, Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), "focus is still on the correct container");

                // top container still the same
                Verify.IsTrue(topContainer == lv.ContainerFromItem(940) as ListViewItem, "top container still same identity");

                // ended up on correct position
                double newoffset = topContainer.TransformToVisual(lv).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
                Verify.IsTrue(Math.Abs(offset - newoffset) < 1, "top container still at -5 offset");

                Verify.IsTrue(counterGoingWithSuggestion + counterCreatingNewLVI + counterUsingImperfectContainer == 0, "all imperfect counters were not hit");
                Verify.IsTrue(counterFoundPerfectContainer == 53, "the perfect counter was hit correctly");

            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateCacheRenewal()
        {
            ListView lv = null;
            ScrollViewer sv = null;
            ItemsStackPanel isp = null;
            CollectionViewSource cvs = null;
            ObservableCollection<Manager> managers = null;

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                cvs = new CollectionViewSource();
                managers = new ObservableCollection<Manager>(); ;
                for (int i = 0; i < 100; ++i)
                {
                    Manager manager = managers.Where((m) => m.Age == i % 10).FirstOrDefault();
                    if (manager == null)
                    {
                        manager = new Manager();
                        manager.Age = i % 10;
                        managers.Add(manager);
                    }
                    manager.Employees.Add(new Employee() { Age = i });
                }

                cvs.IsSourceGrouped = true;
                cvs.ItemsPath = new PropertyPath("Employees");
                cvs.Source = managers;

                lv.ItemsSource = cvs.View;
                lv.ShowsScrollingPlaceholders = false;

                lv.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding Age}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                lv.Height = 500;

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                sv = VisualTreeUtils.FindElementOfTypeInSubtree<ScrollViewer>(lv);
                Verify.IsTrue(sv != null, "found the scrollviewer in the control");
                isp = lv.ItemsPanelRoot as ItemsStackPanel;

                sv.ChangeView(null, sv.ScrollableHeight, null, true);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(isp.LastVisibleIndex == cvs.View.Count -1);
                sv.ChangeView(null, 0, null, true);

            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // remove a few groups
                managers.RemoveAt(3);
                managers.RemoveAt(3);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => { sv.ChangeView(null, sv.ScrollableHeight, null, true); });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(isp.LastVisibleIndex == cvs.View.Count -1);
                sv.ChangeView(null, 0, null, true);

                // add a group
                Manager manager = new Manager();
                manager.Age = 19;
                manager.Employees.Add(new Employee());
                managers.Add(manager);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => { sv.ChangeView(null, sv.ScrollableHeight, null, true); });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(isp.LastVisibleIndex == cvs.View.Count -1);
                sv.ChangeView(null, 0, null, true);

                // replace a group
                Manager manager = new Manager();
                manager.Age = 19;
                manager.Employees.Add(new Employee());
                managers[3] = manager;
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => { sv.ChangeView(null, sv.ScrollableHeight, null, true); });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(isp.LastVisibleIndex == cvs.View.Count -1);
                sv.ChangeView(null, 0, null, true);

                // add an element
                Manager manager = managers[2];
                manager.Employees.Add(new Employee());
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => { sv.ChangeView(null, sv.ScrollableHeight, null, true); });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(isp.LastVisibleIndex == cvs.View.Count -1);
                sv.ChangeView(null, 0, null, true);

                // remove an element
                Manager manager = managers[2];
                manager.Employees.RemoveAt(0);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => { sv.ChangeView(null, sv.ScrollableHeight, null, true); });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(isp.LastVisibleIndex == cvs.View.Count -1);
                sv.ChangeView(null, 0, null, true);
            });
        }

        [TestMethod]
        public void VerifySettingItemsSourceToNullClearsOutAllContainers()
        {
            ListView lv = null;
            int recycleQueue = 0;

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.ItemsSource = Enumerable.Range(0,5).ToList();
                lv.Height = 500;

                lv.ContainerContentChanging += (s, args) =>
                    {
                        if (args.InRecycleQueue)
                        {
                            Log.Comment("index: " + args.ItemIndex + " is being recycled. Item is: " + args.Item.ToString() + " container is " + args.ItemContainer.GetHashCode());
                            recycleQueue++;
                        }
                        else
                        {
                            Log.Comment("index: " + args.ItemIndex + " is being contentbound. Item is: " + args.Item.ToString() + " current content of container is " + (args.ItemContainer.ContentTemplateRoot as TextBlock).Text);
                            (args.ItemContainer.ContentTemplateRoot as TextBlock).Text = args.Item.ToString();
                        }
                    };

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            // wait till everything is created
            UIExecutor.Execute(() =>
            {
                (lv.ContainerFromItem(0) as ListViewItem).Focus(FocusState.Keyboard);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // clear out itemssource
                lv.ItemsSource = null;
            });

            TestServices.WindowHelper.WaitForIdle();

            Verify.IsTrue(recycleQueue == 5, "there should be 5 recycled containers now");
        }

        [TestMethod]
        public void VerifyBackToBackResets()
        {
            ListView lv = null;
            ObservableCollection<int> data = new ObservableCollection<int>();

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                for (int i = 0; i < 20; i++)
                {
                    data.Add(i);
                }

                lv.ItemsSource = data;
                lv.Height = 300;

                lv.ContainerContentChanging += (s, args) =>
                {
                    // bug was that we were getting args.Item as null when doing
                    // multiple resets without a layout in between
                    Verify.IsNotNull(args.Item);
                };

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // move will trigger a reset. do multiple moves to trigger
                // repeated reset calls before layout.
                data.Move(2, 3);
                data.Move(4, 5);
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        #region collection implementations
        public partial class NonGroupedICollectionViewImplementation : ICollectionView
        {
            public void RaiseReset()
            {
                if (VectorChanged != null)
                {
                    VectorChanged.Invoke(this, new VectorChangedEventArgs(CollectionChange.Reset, 0));
                }
            }

            IList<object> backing = new List<object>();
            public NonGroupedICollectionViewImplementation()
            {

            }

            public NonGroupedICollectionViewImplementation(IList<object> src)
            {
                backing = src;
            }

            public void ReplaceBackingAndReset(IList<object> newsrc)
            {
                backing = newsrc;
                RaiseReset();
            }

            public object this[int index]
            {
                get
                {
                    return backing[index];
                }

                set
                {
                    backing[index] = value;
                }
            }

            public IObservableVector<object> CollectionGroups
            {
                get
                {
                    return null;
                }
            }

            public int Count
            {
                get
                {
                    return backing.Count;
                }
            }

            int current = -1;
            public object CurrentItem
            {
                get
                {
                    if (current > -1 && current < Count - 1)
                        return backing[current];
                    return null;
                }
            }

            public int CurrentPosition
            {
                get
                {
                    return current;
                }
            }

            public bool HasMoreItems
            {
                get
                {
                    return false;
                }
            }

            public bool IsCurrentAfterLast
            {
                get
                {
                    throw new NotImplementedException();
                }
            }

            public bool IsCurrentBeforeFirst
            {
                get
                {
                    throw new NotImplementedException();
                }
            }

            public bool IsReadOnly
            {
                get
                {
                    return false;
                }
            }

            public event EventHandler<object> CurrentChanged;
            public event CurrentChangingEventHandler CurrentChanging;
            public event VectorChangedEventHandler<object> VectorChanged;

            public void Add(object item)
            {
                backing.Add(item);
                if (VectorChanged != null)
                {
                    VectorChanged.Invoke(this, new VectorChangedEventArgs(CollectionChange.ItemInserted, (uint)backing.Count - 1));
                }
            }

            public void Clear()
            {
                backing.Clear();
                if (VectorChanged != null)
                {
                    VectorChanged.Invoke(this, new VectorChangedEventArgs(CollectionChange.Reset, 0));
                }
            }

            public bool Contains(object item)
            {
                return backing.Contains(item);
            }

            public void CopyTo(object[] array, int arrayIndex)
            {
                backing.CopyTo(array, arrayIndex);
            }

            public IEnumerator<object> GetEnumerator()
            {
                return backing.GetEnumerator();
            }

            public int IndexOf(object item)
            {
                return backing.IndexOf(item);
            }

            public void Insert(int index, object item)
            {
                backing.Insert(index, item);
            }

            public global::Windows.Foundation.IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
            {
                throw new NotImplementedException();
            }

            public bool MoveCurrentTo(object item)
            {
                int index = backing.IndexOf(item);
                if (index > -1)
                {
                    current = index;
                    return true;
                }
                return false;
            }

            public bool MoveCurrentToFirst()
            {
                if (CurrentChanging != null)
                {
                    throw new NotImplementedException();
                }
                current = 0;
                if (CurrentChanged != null)
                {
                    throw new NotImplementedException();
                }
                return true;
            }

            public bool MoveCurrentToLast()
            {
                current = backing.Count - 1;
                return true;
            }

            public bool MoveCurrentToNext()
            {
                if (current < backing.Count - 1)
                {
                    current++;
                    return true;
                }
                return false;
            }

            public bool MoveCurrentToPosition(int index)
            {
                if (index < backing.Count)
                {

                    current = index;
                    return true;
                }
                return false;
            }

            public bool MoveCurrentToPrevious()
            {
                if (current > 0)
                {
                    current--;
                    return true;
                }
                return false;
            }

            public bool Remove(object item)
            {
                return backing.Remove(item);
            }

            public void RemoveAt(int index)
            {
                backing.RemoveAt(index);
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return backing.GetEnumerator();
            }
        }

        public partial class VectorChangedEventArgs : IVectorChangedEventArgs
        {
            public VectorChangedEventArgs(CollectionChange collectionChange, uint index)
            {
                Index = index;
                this.CollectionChange = collectionChange;
            }
            public object Item { get; set; }

            public CollectionChange CollectionChange { get; set; }

            public uint Index { get; set; }
        }

        public class Manager : Employee
        {
            public Manager()
            {
                Employees = new ObservableCollection<Employee>();
            }

            public ObservableCollection<Employee> Employees
            {
                get { return (ObservableCollection<Employee>)GetValue(EmployeesProperty); }
                set { SetValue(EmployeesProperty, value); }
            }

            // Using a DependencyProperty as the backing store for Employees.  This enables animation, styling, binding, etc...
            public static readonly DependencyProperty EmployeesProperty =
                DependencyProperty.Register("Employees", typeof(ObservableCollection<Employee>), typeof(Manager), new PropertyMetadata(0));

        }

        public class Employee : DependencyObject
        {
            public int Age
            {
                get { return (int)GetValue(AgeProperty); }
                set { SetValue(AgeProperty, value); }
            }

            // Using a DependencyProperty as the backing store for Age.  This enables animation, styling, binding, etc...
            public static readonly DependencyProperty AgeProperty =
                DependencyProperty.Register("Age", typeof(int), typeof(Employee), new PropertyMetadata(0));

        }

        public partial class GroupedICollectionViewImplementation : ICollectionView
        {
            IObservableVector<object> m_groups = new ObservableVector<object>();
            public GroupedICollectionViewImplementation()
            {
            }

            // apparently this is not correct implementation. This should go through a flat index
            public object this[int index]
            {
                get
                {
                    foreach (ObservableVector<object> grp in m_groups)
                    {
                        if (index > grp.Count())
                        {
                            index -= grp.Count();
                        }
                        else
                        {
                            return grp.ElementAt(index);
                        }
                    }
                    return null;
                }

                set
                {
                    throw new NotImplementedException();

                    //VectorChanged.Invoke(this, new VectorChangedEventArgs(CollectionChange.ItemChanged, (uint)index));
                }
            }

            public IObservableVector<object> CollectionGroups
            {
                get
                {
                    return m_groups;
                }
            }

            // apparently this is not correct implementation. This should go through flat index
            public int Count
            {
                get
                {
                    int count = 0;
                    foreach (ObservableVector<object> grp in m_groups)
                    {
                        count += grp.Count();
                    }
                    return count;
                }
            }

            int current = -1;
            public object CurrentItem
            {
                get
                {
                    if (current > -1 && current < Count - 1)
                        return m_groups[current];
                    return null;
                }
            }

            public int CurrentPosition
            {
                get
                {
                    return current;
                }
            }

            public bool HasMoreItems
            {
                get
                {
                    return false;
                }
            }

            public bool IsCurrentAfterLast
            {
                get
                {
                    throw new NotImplementedException();
                }
            }

            public bool IsCurrentBeforeFirst
            {
                get
                {
                    throw new NotImplementedException();
                }
            }

            public bool IsReadOnly
            {
                get
                {
                    return false;
                }
            }

            public event EventHandler<object> CurrentChanged;
            public event CurrentChangingEventHandler CurrentChanging;
            public event VectorChangedEventHandler<object> VectorChanged;

            public void Add(object item)
            {
                m_groups.Add(item);
                if (VectorChanged != null)
                {
                    VectorChanged.Invoke(this, new VectorChangedEventArgs(CollectionChange.ItemInserted, (uint)m_groups.Count - 1));
                }
            }

            public void Clear()
            {
                m_groups.Clear();
                if (VectorChanged != null)
                {
                    VectorChanged.Invoke(this, new VectorChangedEventArgs(CollectionChange.Reset, 0));
                }
            }

            public bool Contains(object item)
            {
                foreach(ObservableVector<object> grp in m_groups)
                {
                    if (grp.Contains(item))
                        return true;
                }
                return false;
            }

            public void CopyTo(object[] array, int arrayIndex)
            {
                m_groups.CopyTo(array, arrayIndex);
            }

            public IEnumerator<object> GetEnumerator()
            {
                return m_groups.GetEnumerator();
            }

            public int IndexOf(object item)
            {
                int index = 0;
                foreach (IList<object> grp in m_groups)
                {
                    int grpindex = grp.IndexOf(item);
                    if (grpindex > -1)
                    {
                        index += grpindex;
                        break;
                    }
                    else
                    {
                        index += grp.Count;
                    }
                }
                return index;
            }

            // I wonder about this, should this be about groups?
            public void Insert(int index, object item)
            {
                m_groups.Insert(index, item);
            }

            public global::Windows.Foundation.IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
            {
                throw new NotImplementedException();
            }

            public bool MoveCurrentTo(object item)
            {
                if (CurrentChanged != null)
                {
                    throw new NotImplementedException();
                }
                int index = m_groups.IndexOf(item);
                if (index > -1)
                {
                    current = index;
                    if (CurrentChanging != null)
                    {
                        throw new NotImplementedException();
                    }
                    return true;
                }
                return false;
            }

            public bool MoveCurrentToFirst()
            {
                current = 0;
                return true;
            }

            public bool MoveCurrentToLast()
            {
                current = m_groups.Count - 1;
                return true;
            }

            public bool MoveCurrentToNext()
            {
                if (current < m_groups.Count - 1)
                {
                    current++;
                    return true;
                }
                return false;
            }

            public bool MoveCurrentToPosition(int index)
            {
                if (index < m_groups.Count)
                {

                    current = index;
                    return true;
                }
                return false;
            }

            public bool MoveCurrentToPrevious()
            {
                if (current > 0)
                {
                    current--;
                    return true;
                }
                return false;
            }

            public bool Remove(object item)
            {
                return m_groups.Remove(item);
            }

            public void RemoveAt(int index)
            {
                m_groups.RemoveAt(index);
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return m_groups.GetEnumerator();
            }

            internal void RaiseReset(uint index)
            {
                if (VectorChanged != null)
                {
                    VectorChanged.Invoke(this, new VectorChangedEventArgs(CollectionChange.Reset, index));
                }
            }

            internal void NewGroupsAndReset(IObservableVector<object> newgroups)
            {
                (m_groups as ObservableVector<object>).ChangeUnderlying(newgroups);
                (m_groups as ObservableVector<object>).RaiseVectorChanged(CollectionChange.Reset, 0);
                RaiseReset(0);
            }
        }

        partial class InnerObservableVector
            : ObservableVector<object>
            , ICollectionViewGroup
            , ICustomPropertyProvider
        {
            private string m_Header;
            private Type m_Type;

            public InnerObservableVector(string header, IEnumerable<object> collection)
                : base(collection)
            {
                m_Header = header;
                m_Type = typeof(Microsoft.UI.Xaml.Controls.ListViewHeaderItem);
            }

            #region ICollectionViewGroup
            object ICollectionViewGroup.Group
            {
                get
                {
                    return this;
                }
            }

            IObservableVector<object> ICollectionViewGroup.GroupItems
            {
                get
                {
                    return this;
                }
            }
            #endregion

            #region ICustomPropertyProvider
            Type ICustomPropertyProvider.Type
            {
                get
                {
                    return typeof(ICollectionViewGroup);
                }
            }

            ICustomProperty ICustomPropertyProvider.GetCustomProperty(string name)
            {
                return null;
            }

            ICustomProperty ICustomPropertyProvider.GetIndexedProperty(string name, Type type)
            {
                throw new NotImplementedException();
            }

            string ICustomPropertyProvider.GetStringRepresentation()
            {
                return m_Header;
            }
            #endregion

            public void RaiseReset()
            {
                RaiseVectorChanged(CollectionChange.Reset, 0);
            }
        }

        // todo: log a bug on someone. This is very bad that I have to write this class
        class ObservableVector<T>
        : IObservableVector<T>
        {
            private List<T> m_Vector;

            private bool storingChanges = false;

            public void SuspendChanges()
            {
                storingChanges = true;
            }

            public void ResumeChanges()
            {
                storingChanges = false;
                RaiseVectorChanged(CollectionChange.Reset, 0);
            }

            public ObservableVector()
                : this(Enumerable.Empty<T>())
            {
                m_Vector = new List<T>();
            }

            public ObservableVector(IEnumerable<T> collection)
            {
                m_Vector = new List<T>(collection);
            }

            public void ChangeUnderlying(IEnumerable<T> collection)
            {
                m_Vector = new List<T>(collection);
            }



            T IList<T>.this[int index]
            {
                get
                {
                    return m_Vector[index];
                }

                set
                {
                    m_Vector[index] = value;
                    RaiseVectorChanged(CollectionChange.ItemChanged, index);
                }
            }

            int ICollection<T>.Count
            {
                get
                {
                    return m_Vector.Count;
                }
            }

            bool ICollection<T>.IsReadOnly
            {
                get
                {
                    return false;
                }
            }

            public event VectorChangedEventHandler<T> VectorChanged;

            public void RaiseVectorChanged(CollectionChange collectionChange, int index)
            {
                if (VectorChanged != null && !storingChanges)
                {
                    VectorChanged(this, new VectorChangedEventArgs(collectionChange, (uint)index));
                }
            }

            void ICollection<T>.Add(T item)
            {
                m_Vector.Add(item);
                RaiseVectorChanged(CollectionChange.ItemInserted, m_Vector.Count - 1);
            }

            void ICollection<T>.Clear()
            {
                m_Vector.Clear();
                RaiseVectorChanged(CollectionChange.Reset, 0);
            }


            bool ICollection<T>.Contains(T item)
            {
                return m_Vector.Contains(item);
            }

            void ICollection<T>.CopyTo(T[] array, int arrayIndex)
            {
                m_Vector.CopyTo(array, arrayIndex);
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return m_Vector.GetEnumerator();
            }

            IEnumerator<T> IEnumerable<T>.GetEnumerator()
            {
                return m_Vector.GetEnumerator();
            }

            int IList<T>.IndexOf(T item)
            {
                return m_Vector.IndexOf(item);
            }

            void IList<T>.Insert(int index, T item)
            {
                m_Vector.Insert(index, item);
                RaiseVectorChanged(CollectionChange.ItemInserted, index);
            }

            bool ICollection<T>.Remove(T item)
            {
                var index = m_Vector.IndexOf(item);
                if (index == -1)
                {
                    return false;
                }

                m_Vector.RemoveAt(index);
                RaiseVectorChanged(CollectionChange.ItemRemoved, index);
                return true;
            }

            void IList<T>.RemoveAt(int index)
            {
                m_Vector.RemoveAt(index);
                RaiseVectorChanged(CollectionChange.ItemRemoved, index);
            }
        }
        #endregion

        [TestMethod]
        public void VerifyMoveDoesNotKeepSelection()
        {
            // Verify regression.

            ListView lv = null;
            ObservableCollection<string> items = null;

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                items = new ObservableCollection<string>();

                items.Add("A");
                items.Add("B");
                items.Add("C");

                lv.ItemsSource = items;
                lv.SelectedIndex = 1;

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(lv.SelectedIndex == 1);
                Verify.IsTrue(lv.SelectedItems.Count == 1);

                items.Move(1, 0);

                Verify.IsTrue(lv.SelectedIndex == -1);
                Verify.IsTrue(lv.SelectedItems.Count == 0);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyMaintainViewportResetHappensBeforeSynchronousLayoutDuringReset()
        {
            ListView list = null;

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.ItemsSource = new ObservableCollection<string>(Enumerable.Range(0, 500).Select(x => "Item " + x).ToList());
                TestServices.WindowHelper.WindowContent = list;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling to index 100...");
                list.ScrollIntoView(list.Items[100]);
                list.UpdateLayout();

                bool canReset = true;
                var scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                scrollViewer.ViewChanged += async delegate
                {
                    if(canReset)
                    {
                        Log.Comment("ScrollViewer viewport changed.");
                        canReset = false;

                        await list.Dispatcher.RunAsync(global::Windows.UI.Core.CoreDispatcherPriority.High, delegate
                        {
                            Log.Comment("Triggering the maintain viewport behavior with a collection change.");
                            ((ObservableCollection<string>)list.ItemsSource).Insert(0, "Inserted Item");
                            Log.Comment("Hard reset.");
                            // We used to crash during the rest
                            list.ItemsSource = Enumerable.Range(0, 500).Select(x => "Item " + x).ToList();
                        });
                    }
                };
            });
            TestServices.WindowHelper.WaitForIdle();
            TestServices.InputHelper.PanFromCenter(list, 0, 200, 0.5);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(0, ((ItemsStackPanel)list.ItemsPanelRoot).FirstVisibleIndex);
            });
        }

        [TestMethod]
        public void VerifyCccRaisedOnceOnResetDuringReset()
        {
            ListView list = null;
            var data = new NonGroupedICollectionViewImplementation(Enumerable.Range(0, 1).Select(x => (object)("Item " + x)).ToList());
            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.ItemsSource = data;
                TestServices.WindowHelper.WindowContent = list;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(list.Focus(FocusState.Keyboard));
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                int cccRecycleCount = 0;
                int cccPrepareCount = 0;
                list.ContainerContentChanging += (s, e) =>
                {
                    Verify.AreEqual("Item 0", (string)e.Item);
                    if (e.InRecycleQueue)
                    {
                        ++cccRecycleCount;
                    }
                    else
                    {
                        ++cccPrepareCount;
                    }
                };

                Log.Comment("Selecting index 0. This will give it keyboard focus");
                list.SelectedIndex = 0;
                var oldContainer = list.ContainerFromIndex(0);

                // At this point, oldContainer is both pinned (because the modern panels
                // always pin the container at index 0) and focused.
                // The rest of this test will validate that we don't raise CCC twice
                // when we recycle the focused/pinned container during reset.

                Log.Comment("Reset the data source with the same data.");
                data.RaiseReset();
                Verify.AreEqual(1, cccRecycleCount);
                Verify.AreEqual(1, cccPrepareCount);
                Verify.AreEqual(oldContainer, list.ContainerFromIndex(0));
                Verify.AreEqual(oldContainer, Input.FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                cccRecycleCount = cccPrepareCount = 0;

                Log.Comment("Clear the data source.");
                data.Clear();
                Verify.AreEqual(1, cccRecycleCount);
                Verify.AreEqual(0, cccPrepareCount);
                Verify.AreEqual(-1, list.SelectedIndex);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanRepopulateGroupWithHidesIfEmpty()
        {
            ObservableCollection<Manager> managers = null;
            ListView list = null;

            UIExecutor.Execute(() =>
            {
                managers = new ObservableCollection<Manager>
                {
                    new Manager
                    {
                        Age = 32,
                        Employees = new ObservableCollection<Employee>
                        {
                            new Employee { Age = 20 }
                        }
                    },
                    new Manager
                    {
                        Age = 36,
                        Employees = new ObservableCollection<Employee>
                        {
                            new Employee { Age = 22 },
                            new Employee { Age = 21 }
                        }
                    }
                };

                list = new ListView();
                var cvs = new CollectionViewSource();
                cvs.IsSourceGrouped = true;
                cvs.ItemsPath = new PropertyPath("Employees");
                cvs.Source = managers;
                list.ItemsSource = cvs.View;

                list.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle HidesIfEmpty='True' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                   "     <GroupStyle.HeaderTemplate>" +
                   "         <DataTemplate>" +
                   "             <Grid Background='Red' >" +
                   "                 <TextBlock Text='{Binding Age}' Foreground='White' FontSize='30'/>" +
                   "             </Grid>" +
                   "         </DataTemplate>" +
                   "     </GroupStyle.HeaderTemplate>" +
                   " </GroupStyle>"));
                list.ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                   "     <TextBlock Text='{Binding Age}' />" +
                   " </DataTemplate>");

                TestServices.WindowHelper.WindowContent = list;
            });
            TestServices.WindowHelper.WaitForIdle();


            UIExecutor.Execute(() =>
            {
                Log.Comment("Resetting the two groups and adding an item to the first one...");
                managers[1].Employees.Clear();
                managers[0].Employees.Clear();
                managers[0].Employees.Add(new Employee { Age = 25 });

                // We used to crash during this call.
                list.UpdateLayout();

                var isp = (ItemsStackPanel)list.ItemsPanelRoot;
                var firstGroupHeader = isp.Children[0];
                var firstGroupContainer = isp.Children[1];

                var firstGroupHeaderOffset = firstGroupHeader.TransformToVisual(isp).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0));
                var firstGroupContainerOffset = firstGroupContainer.TransformToVisual(isp).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0));
                Verify.AreEqual(0.0, firstGroupHeaderOffset.X);
                Verify.AreEqual(0.0, firstGroupHeaderOffset.Y);
                Verify.AreEqual(0.0, firstGroupContainerOffset.X);
                Verify.IsTrue(Math.Abs(firstGroupContainerOffset.Y - 61.0) < 1);

                Log.Comment("Validate that the remaining containers are recycled.");
                for(int i = 2; i < isp.Children.Count; ++i)
                {
                    var offset = isp.Children[i].TransformToVisual(isp).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0));
                    Verify.IsLessThanOrEqual(10000.0, -offset.X);
                    Verify.IsLessThanOrEqual(10000.0, -offset.Y);
                }
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanResetGroupFromEmptyToNonEmptyCollection()
        {
            //
            // In this test, we start with 10 groups with 5 items in each one.
            // We will then reset group 0 with an empty collection.
            // After that, we reset group 0 with a non-empty collection of 5 items.
            // The modern panels don't realize that group 0 is no
            // longer empty and they hold on the previous state causing an out of
            // sync condition with the underlying data source.
            //
            ListView list = null;
            var listLoaded = new AutoResetEvent(false);
            var data = new ObservableCollection<ResetCollectionGroup>(Enumerable.Range(0, 10).Select(i => new ResetCollectionGroup
            {
                Name = $"Group #{i}",
                Items = new ResetCollection(Enumerable.Range(0, 5).Select(j => $"Item #{i}.{j}"))
            }));

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                var cvs = new CollectionViewSource();
                cvs.IsSourceGrouped = true;
                cvs.ItemsPath = new PropertyPath("Items");
                cvs.Source = data;
                list.ItemsSource = cvs.View;
                list.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                   "     <GroupStyle.HeaderTemplate>" +
                   "         <DataTemplate>" +
                   "                 <TextBlock Text='{Binding Name}' Foreground='White' FontSize='30'/>" +
                   "         </DataTemplate>" +
                   "     </GroupStyle.HeaderTemplate>" +
                   " </GroupStyle>"));

                list.Loaded += delegate
                {
                    listLoaded.Set();
                };

                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "ListView loaded.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var group = data[0];
                var groupContainer = (ContentControl)list.GroupHeaderContainerFromItemContainer(list.ContainerFromIndex(0));

                Log.Comment("Reset group 0 to an empty list.");
                group.Items.ResetWith(Enumerable.Range(0, 0).Select(i => ""));

                Log.Comment("Reset group 0 to a non-empty list.");
                group.Items.ResetWith(Enumerable.Range(0, 5).Select(i => $"New Item #{i}"));

                list.UpdateLayout();

                Log.Comment("Validate the 5 containers for group 0 got realized.");
                for (int i = 0; i < group.Items.Count; ++i)
                {
                    Verify.AreEqual(groupContainer, list.GroupHeaderContainerFromItemContainer(list.ContainerFromIndex(i)));
                }
            });
        }

        public class ResetCollectionGroup
        {
            public string Name { get; set; }
            public ResetCollection Items { get; set; }
        }
    }
}
