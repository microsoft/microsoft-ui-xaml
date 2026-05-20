// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Windows.UI;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls.ListViewBase
{
    [TestClass]
    public partial class ListViewBaseTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Classification", "Integration")]
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

        public ListView LoadXamlFileIntoWindow(string fileName)
        {
            string path = Path.Combine(TestDeploymentDir, fileName);
            string text = File.ReadAllText(path);
            ListView target = null;
            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading xaml: ");
                target = (ListView)XamlReader.Load(text);
                Verify.IsNotNull(target, "Unable to load xaml.");

                StackPanel sp = new StackPanel();
                sp.Children.Add(target);

                TestServices.WindowHelper.WindowContent = sp;
            });

            return target;
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanChangeSelectionWithTap()
        {
            ListView list = null;
            IList<string> data = Enumerable.Range(0, 5).Select(x => "Item " + x).ToList();
            FrameworkElement itemContainerToTap = null;
            int indexOfItemToTap = 2;
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = LoadXamlFileIntoWindow(@"resources\managed\controls\ListViewBase\ListView.xaml");
                list.ItemsSource = data;
                list.Loaded += (s, e) =>
                {
                    listLoaded.Set();
                };

                Verify.AreEqual(-1, list.SelectedIndex);
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                itemContainerToTap = (FrameworkElement)list.ContainerFromIndex(indexOfItemToTap);
            });

            TestServices.InputHelper.Tap(itemContainerToTap);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(indexOfItemToTap, list.SelectedIndex);
            });
        }

        [TestMethod]
        public void RaiseVectorChangedEvent()
        {
            var vectorChanged = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                ObservableCollection<string> source = new ObservableCollection<string>(Enumerable.Range(0, 20).Select(x => "Item " + x));
                ListView list = new ListView();
                list.ItemsSource = source;
                ItemCollection items = list.Items;

                items.VectorChanged += (sender, args) =>
                {
                    Log.Comment("ItemCollection.VectorChanged event raised");

                    Verify.AreEqual(items, list.Items);
                    Verify.AreEqual(items, sender);
                    Verify.AreEqual(source, list.ItemsSource);

                    vectorChanged.Set();
                };

                Log.Comment("Removing item at index 3");
                Verify.IsTrue(source.Remove(source[3]));
            });

            Verify.IsTrue(vectorChanged.WaitOne(TimeSpan.FromSeconds(3)), "Received vectorChanged event from items");
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "OneCore")]
        public void CanUpdateVisualStateForPinnedContainersOnSelectionModeChange()
        {
            ListView list = null;
            var listLoaded = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                list = (ListView)XamlReader.Load(
                "<ListView ItemContainerStyle='{ThemeResource ListViewItemExpanded}' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' />");
                list.ItemsSource = Enumerable.Range(0, 500).Select(i => "Item #" + i).ToList();
                TestServices.WindowHelper.WindowContent = list;
                list.Loaded += (s, e) => listLoaded.Set();
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "List failed to raise Loaded event.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                list.ScrollIntoView(list.Items.Last());
                list.UpdateLayout();
                list.SelectionMode = ListViewSelectionMode.Multiple;
                list.ScrollIntoView(list.Items.First());
                list.UpdateLayout();

                var pinnedContainer = (UIElement)list.ContainerFromIndex(0);
                var groups = VisualStateManager.GetVisualStateGroups((FrameworkElement)VisualTreeHelper.GetChild(pinnedContainer, 0));
                VisualStateGroup multiSelectStatesGroup = null;

                foreach (VisualStateGroup group in groups)
                {
                    if (group.Name == "MultiSelectStates")
                    {
                        multiSelectStatesGroup = group;
                        break;
                    }
                }
                Verify.AreEqual("MultiSelectEnabled", multiSelectStatesGroup.CurrentState.Name);
            });
        }

        [TestMethod]
        public void CanScrollAndChangeSelectionDuringTheSameTick()
        {
            ListView list = null;
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView
                {
                    ItemsSource = new ObservableCollection<string>(Enumerable.Range(0, 1000).Select(x => "Item " + x)),
                    Height = 500
                };
                list.Loaded += (s, e) => { listLoaded.Set(); };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Didn't receive loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(list.Focus(FocusState.Keyboard));
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Selecting index 200.");
                list.SelectedIndex = 200;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling to index 400 and selecting index 500.");
                list.ScrollIntoView(list.Items[400]);
                list.SelectedIndex = 500;
                Verify.AreEqual(500, list.SelectedIndex);
                Verify.AreEqual(500, ((ItemsStackPanel)list.ItemsPanelRoot).LastVisibleIndex);
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void VerifyListDoesntScrollDueToSelectionChangeOnCollectionChange()
        {
            ListView list = null;
            var data = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(x => "Item " + x));
            var listLoaded = new AutoResetEvent(false);

            // In this test, we have a list of 100 items.
            // We select and give focus to the first item.
            // Then we scroll to index 10 making it the first visible index.
            // We insert an item at index 0 and we validate we haven't scrolled
            // to the selected item at (now) index 1. We then remove the item we
            // inserted and validate again we haven't scrolled to index 0.
            // The goal is to validate that we don't bring into view the selected
            // item if the selection change was due to a collection change.

            UIExecutor.Execute(() =>
            {
                list = new ListView
                {
                    ItemsSource = data,
                    Height = 200
                };

                list.Loaded += (s, e) => { listLoaded.Set(); };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Selecting and giving focus to the first item.");
                list.SelectedIndex = 0;
                ((Control)list.ContainerFromIndex(0)).Focus(FocusState.Keyboard);

                Log.Comment("Scrolling to index 10.");
                list.ScrollIntoView(data[10], ScrollIntoViewAlignment.Leading);
                list.UpdateLayout();

                Log.Comment("Inserting item at index 0.");
                data.Insert(0, "Inserted item");
                list.UpdateLayout();
                Verify.AreEqual(11, ((ItemsStackPanel)list.ItemsPanelRoot).FirstVisibleIndex);

                Log.Comment("Removing inserted item at index 0.");
                data.RemoveAt(0);
                list.UpdateLayout();
                Verify.AreEqual(10, ((ItemsStackPanel)list.ItemsPanelRoot).FirstVisibleIndex);
            });
        }

        [TestMethod]
        public void VerifyFocusedContainerIndexIsUpdatedAfterCollectionChange()
        {
            ListView list = null;
            var data = new ObservableCollection<string>(Enumerable.Range(0, 4).Select(x => "Item " + x));
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView
                {
                    ItemsSource = data,
                    SelectionMode = ListViewSelectionMode.None
                };

                list.Loaded += (s, e) => { listLoaded.Set(); };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                list.Focus(FocusState.Keyboard);
            });

            TestServices.WindowHelper.WaitForIdle();

            TestServices.KeyboardHelper.Down();
            TestServices.KeyboardHelper.Down();

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var thirdContainer = list.ContainerFromIndex(2);
                Verify.AreEqual(thirdContainer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));

                // Remove an item before the focused item and make sure it
                // still got focus.
                data.RemoveAt(1);
                list.UpdateLayout();
                Verify.AreEqual(thirdContainer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            // To validate that the internal Selector's focus indices are not
            // out-of-sync with the UI, we use keyboard navigation to move the focus
            // to the next item then back, essentially no-oping.
            TestServices.KeyboardHelper.Down();
            TestServices.KeyboardHelper.Up();

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(list.ContainerFromIndex(1), FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));

                data.Insert(1, "New Item");
                list.UpdateLayout();
                Verify.AreEqual(list.ContainerFromIndex(2), FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));

                // Remove the focused item and make sure the item that follows it now have focus.
                data.RemoveAt(2);
                Verify.AreEqual(list.ContainerFromIndex(2), FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));

                // Remove the last item in the data who also happen to be focused and make sure
                // we update focus correctly.
                {
                    data.RemoveAt(2);
                    list.UpdateLayout();
                    Verify.AreEqual(list.ContainerFromIndex(1), FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));

                    data.RemoveAt(1);
                    list.UpdateLayout();
                    Verify.AreEqual(list.ContainerFromIndex(0), FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                }
            });
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void VerifyReorderOfInlineItems()
        {
            ListView lv = null;
            ListViewItem dragContainer = null;

            UIExecutor.Execute(() =>
            {
                lv = (ListView)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                 @"<ListView ReorderMode='Enabled' AllowDrop='True' CanDragItems='True' CanReorderItems='True'
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' >
                        <ListView.Items>
                            <TextBlock>1</TextBlock>
                            <TextBlock>2</TextBlock>
                            <TextBlock>3</TextBlock>
                            <TextBlock>4</TextBlock>
                            <TextBlock>5</TextBlock>
                        </ListView.Items>
                   </ListView>");

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                dragContainer = (ListViewItem)lv.ContainerFromIndex(0);
            });

            TestServices.WindowHelper.WaitForIdle();

            PressHoldAndDragFromCenter(dragContainer, 20, 50, .1, 500);

            TestServices.WindowHelper.WaitForIdle();
            // yay. we didn't crash.
            TestServicesExtensions.EnsureForegroundWindow();

            UIExecutor.Execute(() =>
            {
                int index = lv.IndexFromContainer(dragContainer);
                Verify.AreNotSame(0, index);
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyDragItemsEventsFiredUsingMouse()
        {
            ListView list = null;
            IList<string> data = Enumerable.Range(0, 5).Select(x => "Item " + x).ToList();
            FrameworkElement itemContainerToStartDrag = null;
            var dropResult = DataPackageOperation.None;
            var dragEnterEvent = new AutoResetEvent(false);
            var dropEvent = new AutoResetEvent(false);
            var dragItemsStartingEvent = new AutoResetEvent(false);
            var dragItemsCompletedEvent = new AutoResetEvent(false);
            var listLoaded = new AutoResetEvent(false);

            DragEventHandler dragEnterEventAction = (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Set AcceptedOperation in DragEnter handler.");
                    args.AcceptedOperation = DataPackageOperation.Move;
                    dragEnterEvent.Set();
                };

            DragEventHandler dropEventAction = (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Set AcceptedOperation in Drop handler.");
                    args.AcceptedOperation = DataPackageOperation.Move;
                    dropEvent.Set();
                };

            DragItemsStartingEventHandler dragItemsStartingEventAction =
                (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Verifying event args in DragItemsStarting handler.");

                    Verify.IsNotNull(args.Items, "Items on DragItemsEventArgs should not be null");
                    Verify.IsNotNull(args.Data, "Data on DragItemsEventArgs should not be null");

                    dragItemsStartingEvent.Set();
                };

            global::Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.ListViewBase, Microsoft.UI.Xaml.Controls.DragItemsCompletedEventArgs> dragItemsCompletedEventAction =
                (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Verifying event args in DragItemsCompleted handler.");

                    Verify.IsNotNull(args.Items, "Items on DragItemsEventArgs should not be null");

                    WEX.Logging.Interop.Log.Comment("DropResult: " + args.DropResult.ToString());
                    dropResult = args.DropResult;

                    dragItemsCompletedEvent.Set();
                };

            UIExecutor.Execute(() =>
            {
                list = LoadXamlFileIntoWindow(@"resources\managed\controls\ListViewBase\ListView.xaml");
                list.CanDragItems = true;
                list.AllowDrop = true;
                list.ItemsSource = data;
                list.Loaded += (s, e) =>
                {
                    listLoaded.Set();
                };
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                itemContainerToStartDrag = (FrameworkElement)list.ContainerFromIndex(index: 1);

                list.DragEnter += dragEnterEventAction;
                list.Drop += dropEventAction;
                list.DragItemsStarting += dragItemsStartingEventAction;
                list.DragItemsCompleted += dragItemsCompletedEventAction;
            });

            // Let's force the InputHelper to initialize before doing the real test
            WEX.Logging.Interop.Log.Comment("Tapping list item.");
            TestServices.InputHelper.Tap(itemContainerToStartDrag);
            TestServices.WindowHelper.WaitForIdle();

            WEX.Logging.Interop.Log.Comment("Dragging list item.");
            TestServices.InputHelper.DragFromCenter(itemContainerToStartDrag, relX: 150, relY: 0, velocityFactor: 0.1);
            TestServices.WindowHelper.WaitForIdle();
            Verify.IsTrue(dragItemsStartingEvent.WaitOne(TimeSpan.FromSeconds(5)), "Element didn't get the DragItemsStarting event.");
            Verify.IsTrue(dragEnterEvent.WaitOne(TimeSpan.FromSeconds(2)), "Element didn't get the DragEnter event.");
            Verify.IsTrue(dropEvent.WaitOne(TimeSpan.FromSeconds(2)), "Element didn't get the Drop event.");
            Verify.IsTrue(dragItemsCompletedEvent.WaitOne(TimeSpan.FromSeconds(2)), "Element didn't get the DragItemsCompleted event.");
            TestServicesExtensions.EnsureForegroundWindow();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(dropResult == DataPackageOperation.Move, "Drop result is set correctly.");

                list.DragEnter -= dragEnterEventAction;
                list.Drop -= dropEventAction;
                list.DragItemsStarting -= dragItemsStartingEventAction;
                list.DragItemsCompleted -= dragItemsCompletedEventAction;
            });
        }

        private static UIElement GetInnerMostTarget(FrameworkElement target)
        {
            GeneralTransform transform = target.TransformToVisual(TestServices.WindowHelper.WindowContent);
            Rect bounds = transform.TransformBounds(new Rect(0, 0, target.ActualWidth, target.ActualHeight));
            Point center = new Point(bounds.X + bounds.Width / 2, bounds.Y + bounds.Height / 2);

            WEX.Logging.Interop.Log.Comment("Bounds: " + bounds.ToString());
            WEX.Logging.Interop.Log.Comment("Center: " + center.ToString());

            return VisualTreeHelper.FindElementsInHostCoordinates(center, target).First();
        }

        public static void PressHoldAndDragFromCenter(FrameworkElement target, int relX, int relY, double velocity, uint holdTime)
        {
            WEX.Logging.Interop.Log.Comment("PressHoldAndDragFromCenter");

            if (target == null)
            {
                throw new ArgumentNullException("target", "You cannot call Drag with a null reference target. Make sure you call Commit before you call PressHoldAndDragFromCenter.");
            }

            var resetEvent = new AutoResetEvent(false);
            PointerEventHandler resetEventAction = delegate { resetEvent.Set(); };
            UIElement innerMostTarget = null;

            UIExecutor.Execute(() =>
            {
                innerMostTarget = GetInnerMostTarget(target);
                innerMostTarget.PointerPressed += resetEventAction;
            });

            TestServices.InputHelper.PressHoldAndPanFromCenter(target, relX, relY, velocity, holdTime);

            Verify.IsTrue(resetEvent.WaitOne(TimeSpan.FromSeconds(10)), "Drag didn't start on the element.");
            UIExecutor.Execute(() =>
            {
                innerMostTarget.PointerPressed -= resetEventAction;
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyDragItemsEventsFiredUsingTouch()
        {
            ListView list = null;
            bool gotStartingEvent = false;
            IList<string> data = Enumerable.Range(0, 5).Select(x => "Item " + x).ToList();
            FrameworkElement itemContainerToStartDrag = null;
            var completedEvent = new AutoResetEvent(false);
            var listLoaded = new AutoResetEvent(false);


            DragEventHandler dragOverEventAction =
                (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Set AcceptedOperation in DragOver handler");
                    args.AcceptedOperation = DataPackageOperation.Move;
                };

            DragEventHandler dropEventAction =
                (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Set AcceptedOperation in DropEvent handler");
                    args.AcceptedOperation = DataPackageOperation.Move;
                };

            DragItemsStartingEventHandler startingEventAction =
                (sender, args) =>
                {
                    gotStartingEvent = true;
                    WEX.Logging.Interop.Log.Comment("Verifying event args");

                    if (args.Items == null)
                    {
                        Verify.Fail("Items on DragItemsEventArgs should not be null");
                    }

                    if (args.Data == null)
                    {
                        Verify.Fail("Data on DragItemsEventArgs should not be null");
                    }
                };

            global::Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.ListViewBase, Microsoft.UI.Xaml.Controls.DragItemsCompletedEventArgs> completedEventAction =
                (sender, args) =>
                {
                    if (args.Items == null)
                    {
                        Verify.Fail("Items on DragItemsEventArgs should not be null");
                    }

                    Verify.IsTrue(args.DropResult == DataPackageOperation.Move, "Drop result is set correctly");

                    completedEvent.Set();
                };

            int indexOfItemToStartDrag = 1;

            UIExecutor.Execute(() =>
            {
                list = LoadXamlFileIntoWindow(@"resources\managed\controls\ListViewBase\ListView.xaml");
                list.CanDragItems = true;
                list.AllowDrop = true;
                list.ItemsSource = data;
                list.Loaded += (s, e) =>
                {
                    listLoaded.Set();
                };
            });
            TestServices.WindowHelper.WaitForIdle();

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                itemContainerToStartDrag = (FrameworkElement)list.ContainerFromIndex(indexOfItemToStartDrag);
                list.DragOver += dragOverEventAction;
                list.Drop += dropEventAction;
                list.DragItemsStarting += startingEventAction;
                list.DragItemsCompleted += completedEventAction;
                Log.Comment("****1. validate press hold and drag succeeds");
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Calling into PressHoldAndDragFromCenter.");
            PressHoldAndDragFromCenter(itemContainerToStartDrag, 200, 0, .1, 3500);
            Log.Comment("PressHoldAndDragFromCenter complete.");
            TestServices.WindowHelper.WaitForIdle();

            Verify.IsTrue(completedEvent.WaitOne(TimeSpan.FromSeconds(10)), "Element didn't get the DragItemsCompleted event.");
            TestServicesExtensions.EnsureForegroundWindow();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                if (!gotStartingEvent)
                {
                    Verify.Fail("ListView did not get DragItemsStarting event");
                }
                gotStartingEvent = false;

                Log.Comment("****2. validate drag fails if hold is too short");
            });
            TestServices.WindowHelper.WaitForIdle();

            PressHoldAndDragFromCenter(itemContainerToStartDrag, 200, 0, .1, 50);
            TestServices.WindowHelper.WaitForIdle();
            TestServicesExtensions.EnsureForegroundWindow();

            UIExecutor.Execute(() =>
            {
                if (gotStartingEvent)
                {
                    Verify.Fail("ListView should not get DragItemsStarting event");
                }

                completedEvent.Reset();
                gotStartingEvent = false;

                Log.Comment("****3. validate press hold and drag succeeds after a failed attempt");
            });
            TestServices.WindowHelper.WaitForIdle();

            PressHoldAndDragFromCenter(itemContainerToStartDrag, 200, 0, .1, 3000);
            TestServices.WindowHelper.WaitForIdle();

            Verify.IsTrue(completedEvent.WaitOne(TimeSpan.FromSeconds(10)), "Element didn't get the DragItemsCompleted event.");
            TestServicesExtensions.EnsureForegroundWindow();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                if (!gotStartingEvent)
                {
                    Verify.Fail("ListView did not get DragItemsStarting event");
                }
                list.DragOver -= dragOverEventAction;
                list.Drop -= dropEventAction;
                list.DragItemsStarting -= startingEventAction;
                list.DragItemsCompleted -= completedEventAction;
            });
        }

        [TestMethod]
        public void CanCancelDrag()
        {
            ListView list = null;
            bool dragDropEventFired = false;
            bool dragCompletedEventFired = false;
            IList<string> data = Enumerable.Range(0, 5).Select(x => "Item " + x).ToList();
            FrameworkElement itemContainerToStartDrag = null;
            var CompletedEvent = new AutoResetEvent(false);
            var listLoaded = new AutoResetEvent(false);

            DragItemsStartingEventHandler startingEventAction =
                (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Drag event canceled");
                    args.Cancel = true;
                };

            global::Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.ListViewBase, Microsoft.UI.Xaml.Controls.DragItemsCompletedEventArgs> completedEventAction = delegate { dragCompletedEventFired = true; };
            DragEventHandler dragEventAction = delegate { dragDropEventFired = true; };
            int indexOfItemToStartDrag = 1;

            UIExecutor.Execute(() =>
            {
                list = LoadXamlFileIntoWindow(@"resources\managed\controls\ListViewBase\ListView.xaml");
                list.CanDragItems = true;
                list.AllowDrop = true;
                list.ItemsSource = data;
                list.Loaded += (s, e) =>
                {
                    listLoaded.Set();
                };
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                itemContainerToStartDrag = (FrameworkElement)list.ContainerFromIndex(indexOfItemToStartDrag);
                list.DragItemsStarting += startingEventAction;
                list.DragEnter += dragEventAction;
                list.DragOver += dragEventAction;
                list.DragLeave += dragEventAction;
                list.Drop += dragEventAction;
                list.DragItemsCompleted += completedEventAction;
            });

            TestServices.InputHelper.DragFromCenter(itemContainerToStartDrag, 300, 0, .1);
            TestServices.WindowHelper.WaitForIdle();
            TestServicesExtensions.EnsureForegroundWindow();

            UIExecutor.Execute(() =>
            {
                if (dragDropEventFired)
                {
                    Verify.Fail("Drag drop events still fired after Cancel is set on DragItemsStartingEventArgs");
                }

                if (dragCompletedEventFired)
                {
                    Verify.Fail("DragItemsCompleted event still fired after Cancel is set on DragItemsStartingEventArgs");
                }
                list.DragItemsStarting -= startingEventAction;
                list.DragEnter -= dragEventAction;
                list.DragOver -= dragEventAction;
                list.DragLeave -= dragEventAction;
                list.Drop -= dragEventAction;
                list.DragItemsCompleted -= completedEventAction;
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanReorderUsingMouse()
        {
            ListView list = null;
            FrameworkElement itemContainerToStartDrag = null;
            var completedEvent = new AutoResetEvent(false);
            var listLoaded = new AutoResetEvent(false);
            ObservableCollection<string> collection = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(x => "Item " + x));

            global::Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.ListViewBase, Microsoft.UI.Xaml.Controls.DragItemsCompletedEventArgs> completedEventAction = delegate { completedEvent.Set(); };

            UIExecutor.Execute(() =>
            {
                list = LoadXamlFileIntoWindow(@"resources\managed\controls\ListViewBase\ListView.xaml");
                list.CanReorderItems = true;
                list.AllowDrop = true;
                list.ItemsSource = collection;
                list.Loaded += (s, e) =>
                {
                    listLoaded.Set();
                };

                //
                // Reorder scenario one: CanDragItems == true
                //
                list.CanDragItems = true;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                itemContainerToStartDrag = (FrameworkElement)list.ContainerFromIndex(1);

                list.DragItemsCompleted += completedEventAction;
            });

            TestServices.InputHelper.DragFromCenter(itemContainerToStartDrag, 0, 150, .1);
            Verify.IsTrue(completedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Element didn't get the DragItemsCompleted event.");
            TestServicesExtensions.EnsureForegroundWindow();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue((list.Items[0] as string).Equals("Item 0"), "Item 0 at the wrong position");
                Verify.IsTrue((list.Items[1] as string).Equals("Item 2"), "Item 2 at the wrong position");
                Verify.IsTrue((list.Items[2] as string).Equals("Item 3"), "Item 3 at the wrong position");
                Verify.IsTrue((list.Items[3] as string).Equals("Item 1"), "Item 1 at the wrong position");
                Verify.IsTrue((list.Items[4] as string).Equals("Item 4"), "Item 4 at the wrong position");
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanReorderUsingTouch()
        {
            ListView list = null;
            FrameworkElement itemContainerToStartDrag = null;
            var completedEvent = new AutoResetEvent(false);
            ObservableCollection<string> collection = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(x => "Item " + x));
            var listLoaded = new AutoResetEvent(false);
            global::Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.ListViewBase, Microsoft.UI.Xaml.Controls.DragItemsCompletedEventArgs> completedEventAction = delegate { completedEvent.Set(); };

            UIExecutor.Execute(() =>
            {
                list = LoadXamlFileIntoWindow(@"resources\managed\controls\ListViewBase\ListView.xaml");
                list.CanReorderItems = true;
                list.AllowDrop = true;
                list.ItemsSource = collection;
                list.Loaded += (s, e) =>
                {
                    listLoaded.Set();
                };

                //
                // Reorder scenario one: CanDragItems == true
                //
                list.CanDragItems = true;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                itemContainerToStartDrag = (FrameworkElement)list.ContainerFromIndex(1);

                list.DragItemsCompleted += completedEventAction;
            });

            PressHoldAndDragFromCenter(itemContainerToStartDrag, 0, 150, .1, 3000);
            Verify.IsTrue(completedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Element didn't get the DragItemsCompleted event.");
            TestServicesExtensions.EnsureForegroundWindow();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue((list.Items[0] as string).Equals("Item 0"), "Item 0 at the wrong position");
                Verify.IsTrue((list.Items[1] as string).Equals("Item 2"), "Item 2 at the wrong position");
                Verify.IsTrue((list.Items[2] as string).Equals("Item 3"), "Item 3 at the wrong position");
                Verify.IsTrue((list.Items[3] as string).Equals("Item 1"), "Item 1 at the wrong position");
                Verify.IsTrue((list.Items[4] as string).Equals("Item 4"), "Item 4 at the wrong position");
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateListViewVisualStateOnCaptureLostTouch()
        {
            ValidateListViewItemPointerOverVisualStateOnCaptureLost(false /* isPenInsteadOfTouch */);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ValidateListViewVisualStateOnCaptureLostPen()
        {
            ValidateListViewItemPointerOverVisualStateOnCaptureLost(true /* isPenInsteadOfTouch */);
        }

        // Validate that if we lose pointer capture, we land back in normal state and dont end up
        // in PointerOver state accidentally.
        private void ValidateListViewItemPointerOverVisualStateOnCaptureLost(bool isPenInsteadOfTouch)
        {
            ListView list = null;
            bool isCommonVisualStateNormal = false;

            UIExecutor.Execute(() =>
            {
                // Listview with items that can be dragged but not reordered
                list = (ListView)XamlReader.Load("<ListView ItemContainerStyle='{ThemeResource ListViewItemExpanded}' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' />");
                list.CanDragItems = true;
                // Just one item is enough to repro
                list.ItemsSource = Enumerable.Range(0, 1).Select(x => "Item " + x);

                TestServices.WindowHelper.WindowContent = list;
            });

            TestServices.WindowHelper.WaitForIdle();

            FrameworkElement itemContainerToStartDrag = null;
            VisualStateGroup commonState = null;
            var pointerCaptureLostEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                itemContainerToStartDrag = (FrameworkElement)list.ContainerFromIndex(0);
                commonState = GetCommonStates(itemContainerToStartDrag as ListViewItem);
            });

            var startDraghandler = new Action<object, PointerRoutedEventArgs >((s, e) =>
            {
                pointerCaptureLostEvent.Set();
            });

            var stateChangedhandler = new Action<object, VisualStateChangedEventArgs>( (s, e) =>
            {
                Log.Comment("Target State Changed: {0}->{1}", e.OldState.Name, e.NewState.Name);
                isCommonVisualStateNormal = e.NewState.Name == "Normal";
            });

            using (new EventTester<FrameworkElement, PointerRoutedEventArgs>(itemContainerToStartDrag, "PointerCaptureLost", startDraghandler))
            using (new EventTester<VisualStateGroup, VisualStateChangedEventArgs>(commonState, "CurrentStateChanged", stateChangedhandler))
            {
                if (isPenInsteadOfTouch)
                {
                    TestServices.InputHelper.PenStrokeFromCenter(itemContainerToStartDrag, 0, 150, .1);
                }
                else
                {
                    // Touch
                    PressHoldAndDragFromCenter(itemContainerToStartDrag, 0, 150, .1, 3000);
                }

                Verify.IsTrue(pointerCaptureLostEvent.WaitOne(TimeSpan.FromSeconds(10)), "PointerCaptureLost event not fired");
                // Drag operation can cause the window to lose focus so set the app back in focus.
                TestServicesExtensions.EnsureForegroundWindow();

                // Verify that we reached the normal state at the end.
                Verify.IsTrue(isCommonVisualStateNormal);
            }
        }

        [TestMethod]
        [Description("Validates that we can drag an item over a ListView that hasn't run layout yet. We should not crash in those circumstances.")]
        public void CanDragItemOverListViewWithPendingLayoutPass()
        {
            ListView list = null;
            Shapes.Rectangle rect = null;
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView
                {
                    CanReorderItems = true,
                    CanDragItems = true,
                    AllowDrop = true,
                    Height = 500,
                    Background = new SolidColorBrush(Colors.LightGray)
                };

                rect = new Shapes.Rectangle
                {
                    CanDrag = true,
                    Fill = new SolidColorBrush(Colors.Red),
                    Width = 100,
                    Height = 100
                };

                var host = new StackPanel();
                host.Children.Add(rect);
                host.Children.Add(list);

                list.Loaded += (s, e) => { listLoaded.Set(); };
                list.DragEnter += (s, e) =>
                {
                    e.AcceptedOperation = DataPackageOperation.Copy;

                    // The crash happened during the following drag over events because
                    // we haven't run layout yet to realize the data.
                    Log.Comment("Setting ItemsSource.");
                    list.ItemsSource = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => $"Item #{i}"));
                };

                TestServices.WindowHelper.WindowContent = host;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list.");
            TestServices.WindowHelper.WaitForIdle();

            TestServices.InputHelper.DragFromCenter(rect, relX: 0, relY: 500, velocityFactor: 0.1);
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void EstimationCorrectionAfterScrollIntoViewUngrouped()
        {
            ListView lv = null;

            // 500 items.
            // item 0 is 50 high and the rest are 200 high
            // if we do a scroll into view item 450 right after setting the itemsource,
            // we will measure the first item (item 0) and then estimate the location of 450
            // the estimate will be 450*50 since 50 is the only item we base the estimate on - it is
            // off by a lot. This will show when you start scrolling the thumb, once we get near
            // the top or bottom of the list we will correct by a lot causing the thumb to jump.
            // This test does a one time correction after a scroll into view once we have more items
            // generated and hence a little better estimate.
            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;

                lv.ContainerContentChanging += (s, args) =>
                {
                    // verify that only the first item is first page is loaded
                    Log.Comment("CCC Item: " + args.ItemIndex);

                    // 400 to account for caching
                    if (args.ItemIndex > 0 && args.ItemIndex < 400)
                    {
                        Log.Error("We should be getting calls only for item 0 and then item 450. Rest of first page should not be loaded");
                    }

                    if (args.ItemIndex == 0)
                    {
                        args.ItemContainer.Height = 50;
                    }
                    else
                    {
                        args.ItemContainer.Height = 200;
                    }
                };

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                lv.ItemsSource = Enumerable.Range(0, 500);
                lv.ScrollIntoView(450);
            });

            TestServices.WindowHelper.WaitForIdle();

            // Check that we have
            UIExecutor.Execute(() =>
            {
                var isp = lv.ItemsPanelRoot as ItemsStackPanel;

                var container = lv.ContainerFromIndex(450) as ListViewItem;
                double offset = container.TransformToVisual(isp).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;

                // the offset of container 450 in the panel should be
                // 50 + 449*200 = 89,850
                // If we accounted only the first item (item 0) as we used to
                // we would estimate item 450 to be at 450*50 = 22500
                Verify.IsTrue(offset > 75000, "offset of the scrolled item is lower than expected");


                var height = isp.ActualHeight;
                // If only the first item was accounted for then the entire panel
                // would only be 500*50 = 25000 high. Once error corrected we should be
                // getting much bigger than that (actual extent is 50 + 199*500 = 99,550)
                Log.Comment("Height is = " + height.ToString());

                // 85000 because we are still working with an estimate that includes
                // 50 for the first item, 200 for the items in the cache - hence the estimate
                // is still lower than the actual extent of 99550
                Verify.IsTrue(height > 85000, "Panel extent is lower than expected after correction");
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void EstimationCorrectionAfterScrollIntoViewGroupedItem(bool scrollToHeader)
        {
            EstimationCorrectionAfterScrollIntoViewGrouped(false);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void EstimationCorrectionAfterScrollIntoViewGroupedHeader(bool scrollToHeader)
        {
            EstimationCorrectionAfterScrollIntoViewGrouped(true);
        }

        public void EstimationCorrectionAfterScrollIntoViewGrouped(bool scrollToHeader)
        {
            ListView lv = null;
            CollectionViewSource cvs = null;
            var groups = GetGroupedData(numGroups: 50, itemsPerGroup: 10);

            // 500 items.
            // item 0 is 50 high and the rest are 200 high
            // if we do a scroll into view item 450 right after setting the itemsource,
            // we will measure the first item (item 0) and then estimate the location of 450
            // the estimate will be 450*50 since 50 is the only item we base the estimate on - it is
            // off by a lot. This will show when you start scrolling the thumb, once we get near
            // the top or bottom of the list we will correct by a lot causing the thumb to jump.
            // This test does a one time correction after a scroll into view once we have more items
            // generated and hence a little better estimate.
            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;
                cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = groups;

                lv.ContainerContentChanging += (s, args) =>
                {
                    // verify that only the first item is first page is loaded
                    Log.Comment("CCC Item: " + args.ItemIndex);

                    // 400 to account for caching
                    if (args.ItemIndex > 0 && args.ItemIndex < 400)
                    {
                        Log.Error("We should be getting calls only for item 0 and then item 450. Rest of first page should not be loaded");
                    }

                    if (args.ItemIndex == 0)
                    {
                        args.ItemContainer.Height = 50;
                    }
                    else
                    {
                        args.ItemContainer.Height = 200;
                    }
                };

                lv.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                lv.ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
                   "     <TextBlock Text='{Binding}' />  " +
                   " </DataTemplate>                         ");

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                lv.ItemsSource = cvs.View;
                if (scrollToHeader)
                {
                    lv.ScrollIntoView(groups[45]);
                }
                else
                {
                    lv.ScrollIntoView(groups[45][0]);
                }
            });

            TestServices.WindowHelper.WaitForIdle();

            // Check that we have
            UIExecutor.Execute(() =>
            {
                var isp = lv.ItemsPanelRoot as ItemsStackPanel;

                var container = lv.ContainerFromItem(groups[45][0]) as ListViewItem;
                double offset = container.TransformToVisual(isp).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;

                Log.Comment("Offset is " + offset.ToString());
                // the offset of container 450 in the panel should be
                // 50 + 449*200 = 89,850
                // If we accounted only the first item (item 0) as we used to
                // we would estimate item 450 to be at 450*50 = 22500
                Verify.IsTrue(offset > 75000, "offset of the scrolled item is lower than expected");


                var height = isp.ActualHeight;
                // If only the first item was accounted for then the entire panel
                // would only be 500*50 = 25000 high. Once error corrected we should be
                // getting much bigger than that (actual extent is 50 + 199*500 = 99,550)
                Log.Comment("Height is = " + height.ToString());

                // 85000 because we are still working with an estimate that includes
                // 50 for the first item, 200 for the items in the cache - hence the estimate
                // is still lower than the actual extent of 99550
                Verify.IsTrue(height > 85000, "Panel extent is lower than expected after correction");
            });
        }

        // inserting before the selected index, when the selected index is
        // not in view causes the selected index to be scrolled back into view.
        // If this happens during a group insertion, we end up crashing because
        // group notifications and item notifications are expected to happen in the
        // same tick and a layout in the middle of that is not expected.
        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void SelectionChangeDuringGroupInsert()
        {
            ListView lv = null;
            CollectionViewSource cvs = null;
            var groups = GetGroupedData(numGroups: 10, itemsPerGroup: 10);

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;
                cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = groups;

                lv.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                lv.ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
                   "     <TextBlock Text='{Binding}' />  " +
                   " </DataTemplate>                         ");

                lv.ItemsSource = cvs.View;
                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            // scroll down to the end of the list and
            // select an item.
            UIExecutor.Execute(() =>
            {
                lv.ScrollIntoView(groups[9][0]);
                lv.SelectedIndex = 90;
            });

            TestServices.WindowHelper.WaitForIdle();

            // scroll back up the list
            UIExecutor.Execute(() =>
            {
                lv.ScrollIntoView(groups[0][0]);
            });

            TestServices.WindowHelper.WaitForIdle();

            // insert a group (with many items). This used
            // to cause selection changed to be fired and
            // a forced scroll item into view for the selected
            // item while the group change was in progress.
            UIExecutor.Execute(() =>
            {
                // insert at 0 a group with 10 items
                var group = new ObservableCollection<int>();
                for (int j = 0; j < 10; j++)
                {
                    group.Add(j);
                }
                groups.Insert(0, group);
            });

            TestServices.WindowHelper.WaitForIdle();

            // yay. we didn't crash.
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void InsertGroupInMiddleOfVisibleWindow()
        {
            ListView lv = null;
            CollectionViewSource cvs = null;
            var groups = GetGroupedData(numGroups: 10, itemsPerGroup: 1);

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;
                cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = groups;

                lv.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                lv.ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
                   "     <TextBlock Text='{Binding}' />  " +
                   " </DataTemplate>");

                lv.ItemsSource = cvs.View;
                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            // insert a group in the middle of visible window.
            // This will cause a sentinel to be added for the group header
            UIExecutor.Execute(() =>
            {
                var group = new ObservableCollection<int>();
                group.Add(4444);
                groups.Insert(2, group);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var container = lv.ContainerFromItem(4444);
                Verify.IsNotNull(container, "Container is not null");
                var groupContainer = lv.GroupHeaderContainerFromItemContainer(container);
                Verify.IsNotNull(groupContainer, "GroupContainer is not null");
            });

        }

        // when using mouse to click on the large increase/decrease
        // buttons on the thumb, and using sticky headers - we did not
        // account for the sticky header taking up space in the viewport.
        // this test makes sure that we account for that.
        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void MouseLargeClickWithStickyHeaders()
        {
            ListView lv = null;
            CollectionViewSource cvs = null;
            var groups = GetGroupedData(numGroups: 10, itemsPerGroup: 20);
            RepeatButton largeIncreaseButton = null;
            RepeatButton largeDecreaseButton = null;
            ItemsStackPanel isp = null;

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;
                lv.Width = 400;
                cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = groups;

                lv.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' Height='45'>                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                lv.ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
                   "    <Grid Height='45' > " +
                   "        <TextBlock Text='{Binding}' />  " +
                   "    </Grid> " +
                   " </DataTemplate>");
                lv.ItemsSource = cvs.View;

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();
            TestServices.InputHelper.MoveMouse(lv);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                isp = lv.ItemsPanelRoot as ItemsStackPanel;
                largeIncreaseButton = lv.FindNameInSubtree("VerticalLargeIncrease") as RepeatButton;
                largeDecreaseButton = lv.FindNameInSubtree("VerticalLargeDecrease") as RepeatButton;

                Log.Comment("FirstVisibleIndex:" + isp.FirstVisibleIndex);
                Verify.IsTrue(isp.FirstVisibleIndex == 0);
            });

            TestServices.InputHelper.MoveMouse(largeIncreaseButton);
            TestServices.InputHelper.LeftMouseClick(largeIncreaseButton);
            TestServices.WindowHelper.WaitForIdle();

            // 0 to 9 is visible in the first view, after clicking on the
            // mouse large increase button - the new first visible index
            // would have been 10 (but 10 would be hidden by the sticky header)
            // with the fix, the new first visible index is 9. same logic for the
            // rest of the verification.
            UIExecutor.Execute(() =>
            {
                Log.Comment("FirstVisibleIndex:" + isp.FirstVisibleIndex);
                Verify.IsTrue(isp.FirstVisibleIndex == 9);
            });

            TestServices.InputHelper.MoveMouse(largeIncreaseButton);
            TestServices.InputHelper.LeftMouseClick(largeIncreaseButton);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("FirstVisibleIndex:" + isp.FirstVisibleIndex);
                Verify.IsTrue(isp.FirstVisibleIndex == 19);
            });

            TestServices.InputHelper.MoveMouse(largeDecreaseButton);
            TestServices.InputHelper.LeftMouseClick(largeDecreaseButton);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("FirstVisibleIndex:" + isp.FirstVisibleIndex);
                Verify.IsTrue(isp.FirstVisibleIndex == 9);
            });

            TestServices.InputHelper.MoveMouse(largeDecreaseButton);
            TestServices.InputHelper.LeftMouseClick(largeDecreaseButton);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("FirstVisibleIndex:" + isp.FirstVisibleIndex);
                Verify.IsTrue(isp.FirstVisibleIndex == 0);
            });
        }

        private Dictionary<int, int> containerPhaseReached;
        void lv_ContainerContentChanging(Xaml.Controls.ListViewBase sender, ContainerContentChangingEventArgs args)
        {
            if (args.Phase < 5)
            {
                args.RegisterUpdateCallback(lv_ContainerContentChanging);
            }
            Log.Comment("CCC Item: " + args.ItemIndex, " Phase:" + args.Phase);

            containerPhaseReached[args.ItemIndex]++;
        }

        [TestMethod]
        public void PhasingContinuesAfterRenteringLiveTree()
        {
            Grid rootPanel = null;
            ListView lv = null;
            AutoResetEvent middleOfCCC = new AutoResetEvent(false);
            containerPhaseReached = new Dictionary<int, int>();

            var loadedhandler = new Action<object, RoutedEventArgs>((s, args) =>
            {
                (lv.ItemsPanelRoot as ItemsStackPanel).CacheLength = 0;
            });

            var containerContentChangingHandler = new Action<object, ContainerContentChangingEventArgs>((s, args) =>
            {
                if (args.InRecycleQueue)
                {
                    Log.Comment("CCC Recycle Item: " + args.ItemIndex);
                }
                else
                {
                    Log.Comment("CCC Item: " + args.ItemIndex, " Phase:" + args.Phase);
                    if (args.ItemIndex == 1)
                    {
                        middleOfCCC.Set();
                    }

                    if (!containerPhaseReached.ContainsKey(args.ItemIndex))
                    {
                        containerPhaseReached.Add(args.ItemIndex, 0);
                    }

                    args.RegisterUpdateCallback(lv_ContainerContentChanging);
                }
            });

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;
            });

            using (new EventTester<ListView, RoutedEventArgs>(lv, "Loaded", loadedhandler))
            using (new EventTester<ListView, ContainerContentChangingEventArgs>(lv, "ContainerContentChanging", containerContentChangingHandler))
            {
                UIExecutor.Execute(() =>
                {
                    lv.ItemsSource = Enumerable.Range(0, 500);
                    rootPanel = new Grid();
                    rootPanel.Children.Add(lv);
                    TestServices.WindowHelper.WindowContent = rootPanel;
                });

                // wait until we are in the middle of ccc events //
                Verify.IsTrue(middleOfCCC.WaitOne(TimeSpan.FromSeconds(20)), "Reached ccc for item 1");

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Removing from live tree");
                    rootPanel.Children.Clear();
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Adding back to live tree");
                    rootPanel.Children.Add(lv);
                });
                TestServices.WindowHelper.WaitForIdle();

                // verify that we got ccc for all
                foreach (var key in containerPhaseReached.Keys)
                {
                    Log.Comment("Index:" + key.ToString() + " Phase:" + containerPhaseReached[key].ToString());
                    Verify.IsTrue(containerPhaseReached[key] == 5);
                }
            }
        }

        // Regression test
        [TestMethod]
        public void SetItemWidthToNanAfterFirstItemIsMeasured()
        {
            Grid rootPanel = null;
            GridView gv = null;
            AutoResetEvent firstItemMeasured = new AutoResetEvent(false);

            // data containing null. We will replace it later with data //
            ObservableCollection<object> data = new ObservableCollection<object>();

            UIExecutor.Execute(() =>
            {
                gv = new GridView();
                gv.ShowsScrollingPlaceholders = false;
                gv.Height = 500;
                gv.Loaded += (s, args) =>
                {
                    (gv.ItemsPanelRoot as ItemsWrapGrid).CacheLength = 0;
                };

                gv.ContainerContentChanging += (s, args) =>
                {
                    if (args.InRecycleQueue)
                    {
                        Log.Comment("CCC Recycle Item: " + args.ItemIndex);
                    }
                    else
                    {
                        Log.Comment("CCC Item: " + args.ItemIndex, " Phase:" + args.Phase);
                        if (args.ItemIndex == 0)
                        {
                            firstItemMeasured.Set();
                        }
                    }
                };

                for (int i = 0; i < 20; i++)
                {
                    data.Add(null);
                }

                gv.ItemsSource = data;
                rootPanel = new Grid();
                rootPanel.Children.Add(gv);
                TestServices.WindowHelper.WindowContent = rootPanel;

            });

            // wait until we are in the middle of ccc events //
            Verify.IsTrue(firstItemMeasured.WaitOne(TimeSpan.FromSeconds(20)), "Reached ccc for item 0");

            // we have measured item 0, now change ItemHeight to Nan.
            // This should cause us to re-measure the first item. If not,
            // we will end up crashing when measuring items following that.
            UIExecutor.Execute(() =>
            {
                (gv.ItemsPanelRoot as ItemsWrapGrid).ItemHeight = double.NaN;
                data[0] = "Item0";
                data[0] = "Item1";
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                data[2] = "Item2";
            });

            TestServices.WindowHelper.WaitForIdle();

            // we didn't crash when measuring the rest of the items.
        }

        partial class CustomGroupStyleSelector : GroupStyleSelector
        {
            protected override GroupStyle SelectGroupStyleCore(object group, uint level)
            {
                var g = group as ICollectionViewGroup;
                int count = g != null ? g.GroupItems.Count : -1;
                string xaml = string.Format(
                    @"<GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                  xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>
                        <GroupStyle.HeaderTemplate>
                            <DataTemplate>
                                <TextBlock x:Name='TB' Text='{0}'/>
                            </DataTemplate>
                        </GroupStyle.HeaderTemplate>
                    </GroupStyle> ", count);
                return (GroupStyle)XamlReader.Load(xaml);
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void GroupStyleSelectorTest()
        {
            Grid rootPanel = null;
            ListView lv = null;
            CollectionViewSource cvs = null;

            UIExecutor.Execute(() =>
            {
                lv = new ListView();

                List<Object> data = new List<Object>();

                for (int i = 0; i < 4; i++)
                {
                    List<int> group = new List<int>();
                    for (int j = 0; j < i; j++)
                    {
                        group.Add(i * 100 + j);
                    }
                    data.Add(group);
                }

                cvs = new CollectionViewSource();
                cvs.IsSourceGrouped = true;
                cvs.Source = data;
                lv.ItemsSource = cvs.View;
                lv.GroupStyleSelector = new CustomGroupStyleSelector();

                rootPanel = new Grid();
                rootPanel.Children.Add(lv);
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // in below test we'll iterate all groups and check if the group styles are correctly applied.
                // note there is no direct way to iterate all groups, instead i hve to iterate all items
                // and call GroupHeaderContainerFromItemContainer to find the HeaderItem.
                foreach (var item in lv.Items)
                {
                    var container = lv.ContainerFromItem(item);
                    var header = lv.GroupHeaderContainerFromItemContainer(container) as ListViewBaseHeaderItem;
                    var tb = header.FindNameInSubtree("TB") as TextBlock;
                    // verify we applied correct group style by checking the TextBlock.Text inside the header template.
                    Verify.AreEqual(tb.Text, (header.Content as List<int>).Count.ToString());
                }
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void ZoomTest()
        {
            ListView listView = null;
            ItemsStackPanel itemsStackPanel = null;
            ScrollViewer scrollViewer = null;
            int firstVisibleIndex = 0;
            int lastVisibleIndex = 0;
            ManualResetEvent viewChangedEvent = new ManualResetEvent(false);

            UIExecutor.Execute(() =>
            {
                listView = new ListView();
                listView.ShowsScrollingPlaceholders = false;
                listView.Height = 500;
                listView.Loaded += (s, args) =>
                {
                    (listView.ItemsPanelRoot as ItemsStackPanel).CacheLength = 0;
                };

                listView.ItemsSource = Enumerable.Range(0, 500);
                TestServices.WindowHelper.WindowContent = listView;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                scrollViewer = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                scrollViewer.ZoomMode = ZoomMode.Enabled;
                scrollViewer.ViewChanged += (sender, e) =>
                                                {
                                                    if (e.IsIntermediate == false)
                                                    {
                                                        viewChangedEvent.Set();
                                                    }
                                                };
                itemsStackPanel = (ItemsStackPanel)listView.ItemsPanelRoot;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                firstVisibleIndex = itemsStackPanel.FirstVisibleIndex;
                lastVisibleIndex = itemsStackPanel.LastVisibleIndex;
                Verify.IsTrue(firstVisibleIndex == 0, "First visible index is 0");

                // zoom out
                scrollViewer.ChangeView(null, null, 0.5f, true);
            });

            // wait for the view changed event and then wait for
            // listview to handle the view change.
            viewChangedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(firstVisibleIndex == 0, "First visible index is 0");
                Verify.IsTrue(itemsStackPanel.LastVisibleIndex > lastVisibleIndex, "lastvisibleindex is bigger when zoomed out");
                lastVisibleIndex = itemsStackPanel.LastVisibleIndex;

                // scroll
                scrollViewer.ChangeView(null, 500, null, true);
            });

            // wait for the view changed event and then wait for
            // listview to handle the view change.
            Verify.IsTrue(viewChangedEvent.WaitOne(TimeSpan.FromSeconds(10)));
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(itemsStackPanel.FirstVisibleIndex > 0, "First visible index is not 0");
                Verify.IsTrue(itemsStackPanel.LastVisibleIndex > lastVisibleIndex, "lastvisibleindex is bigger when zoomed out");
            });
        }

        [TestMethod]
        public void ItemGenerationAndRecycleTest()
        {
            ListView listView = null;
            Dictionary<object, int> containers = new Dictionary<object, int>();
            List<int> data = Enumerable.Range(0, 30).ToList();

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@" <ListView
                                                              xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                              Height='200'
                                                              Width='200'>
                                                            <ListView.ItemsPanel>
                                                                <ItemsPanelTemplate>
                                                                    <ItemsStackPanel CacheLength='0' />
                                                                </ItemsPanelTemplate>
                                                            </ListView.ItemsPanel>
                                                            <ListView.ItemContainerStyle>
                                                                <Style TargetType='ListViewItem'>
                                                                    <Setter Property='Height' Value='50' />
                                                                </Style>
                                                            </ListView.ItemContainerStyle>
                                                        </ListView>");
                listView.ItemsSource = data;
                TestServices.WindowHelper.WindowContent = listView;
                listView.ContainerContentChanging += (sender, args) =>
                {
                    if (!args.InRecycleQueue && args.ItemContainer != null)
                    {
                        if (!containers.ContainsKey(args.ItemContainer))
                        {
                            containers.Add(args.ItemContainer, args.ItemIndex);
                        }
                    }
                };
            });

            TestServices.WindowHelper.WaitForIdle();

            // verify the number of containers created - should be 5 since the viewport
            // is 200 pixels and each item is 50 pixels high, and we go one more before stopping
            Verify.IsTrue(containers.Values.Count == 5, "Container count is not 5");

            UIExecutor.Execute(() =>
            {
                listView.ScrollIntoView(data[15], ScrollIntoViewAlignment.Leading);
            });

            TestServices.WindowHelper.WaitForIdle();

            // verify that the containers were recycled, one pinned, 4 visible and
            // one more in the forward direction, one more in the backward direction
            Log.Comment("container count - " + containers.Values.Count);
            Verify.IsTrue(containers.Values.Count == 7, "Containers were not recycled");

            UIExecutor.Execute(() =>
            {
                listView.ScrollIntoView(data[0], ScrollIntoViewAlignment.Leading);
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("container count - " + containers.Values.Count);
            Verify.IsTrue(containers.Values.Count == 7, "Containers were not recycled");

            UIExecutor.Execute(() =>
            {
                listView.ScrollIntoView(data[5], ScrollIntoViewAlignment.Leading);
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("container count - " + containers.Values.Count);
            Verify.IsTrue(containers.Values.Count <= 8, "Containers were not recycled");
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Ignore", "True")] // ListViewBaseTests.ThumbDragTest is unreliable
        public void ThumbDragTest()
        {
            ListView listView = null;
            Dictionary<object, int> containers = new Dictionary<object, int>();
            List<int> data = Enumerable.Range(0, 50).ToList();
            Thumb verticalThumb = null;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@" <ListView
                                                              xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                              Height='200'
                                                              Width='200'>
                                                            <ListView.ItemsPanel>
                                                                <ItemsPanelTemplate>
                                                                    <ItemsStackPanel CacheLength='0' />
                                                                </ItemsPanelTemplate>
                                                            </ListView.ItemsPanel>
                                                            <ListView.ItemContainerStyle>
                                                                <Style TargetType='ListViewItem'>
                                                                    <Setter Property='Height' Value='50' />
                                                                </Style>
                                                            </ListView.ItemContainerStyle>
                                                        </ListView>");
                listView.ItemsSource = data;
                TestServices.WindowHelper.WindowContent = listView;
                listView.ContainerContentChanging += (sender, args) =>
                {
                    if (!args.InRecycleQueue && args.ItemContainer != null)
                    {
                        if (!containers.ContainsKey(args.ItemContainer))
                        {
                            containers.Add(args.ItemContainer, args.ItemIndex);
                        }
                    }
                };
            });

            TestServices.WindowHelper.WaitForIdle();

            // verify the number of containers created - should be 5 since the viewport
            // is 200 pixels and each item is 50 pixels high, and we go one more before stopping
            Verify.IsTrue(containers.Values.Count == 5, "Container count is not 5");

            UIExecutor.Execute(() =>
            {
                verticalThumb = (Thumb)listView.FindNameInSubtree("VerticalThumb");
            });

            var thumbBounds = GetBounds(verticalThumb);

            TestServices.InputHelper.LeftMouseClick(verticalThumb); // Click to ensure scroll thumb activation

            TestServices.InputHelper.MouseButtonDown(verticalThumb, 0, 0, MouseButton.Left);

            TestServices.InputHelper.MouseDrag(
                            new Point(thumbBounds.Left + (thumbBounds.Width / 2), thumbBounds.Top + (thumbBounds.Height / 2)),
                            new Point(thumbBounds.Left + (thumbBounds.Width / 2), thumbBounds.Top + (thumbBounds.Height / 2) + 200),
                            MouseButton.Left);

            Log.Comment("Drag injected. Waiting for the thumb to move so we can get its new bounds.");
            TestServices.WindowHelper.WaitForIdle();
            TestServices.InputHelper.MouseButtonUp(verticalThumb, 0, 0, MouseButton.Left);

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var isp = listView.ItemsPanelRoot as ItemsStackPanel;
                Log.Comment("first visible index is " + isp.FirstVisibleIndex);
                Verify.IsTrue(isp.FirstVisibleIndex == 46, "did not reach end of the list by dragging thumb");
            });
        }

        [TestMethod]
        public void ScrollItemIntoViewOnLoaded()
        {
            ListView list = null;
            IList<string> data = Enumerable.Range(0, 500).Select(x => "Item " + x).ToList();
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.ShowsScrollingPlaceholders = false;
                list.Height = 500;
                list.ItemsSource = data;
                list.Loaded += (s, e) =>
                {
                    list.ScrollIntoView(data[490]);
                    listLoaded.Set();
                };

                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var container = list.ContainerFromIndex(490);
                Verify.IsNotNull(container);
            });

            ValidateListViewScrollIntoViewDoesntCreateContainerOutsideTree();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ScrollGroupIntoViewOnLoaded()
        {
            ListView list = null;
            var data = GetGroupedData(numGroups: 5, itemsPerGroup: 500);
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.ShowsScrollingPlaceholders = false;
                list.Height = 500;

                list.Loaded += (s, e) =>
                {
                    list.ScrollIntoView(data[4]);
                    listLoaded.Set();
                };

                var cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = data;
                list.ItemsSource = cvs.View;

                list.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                list.ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
                   "     <TextBlock Text='{Binding}' />  " +
                   " </DataTemplate>");

                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var isp = list.ItemsPanelRoot as ItemsStackPanel;
                Verify.IsNotNull(isp);
                Verify.IsTrue(isp.FirstVisibleIndex != 0);
            });

            ValidateListViewScrollIntoViewDoesntCreateContainerOutsideTree();
        }

        private static void ValidateListViewScrollIntoViewDoesntCreateContainerOutsideTree()
        {
            ListView list = null;
            UIExecutor.Execute(() =>
            {
                list = (ListView)TestServices.WindowHelper.WindowContent;
                TestServices.WindowHelper.WindowContent = null;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var targetIndex = 1; // Should not be realized.
                Verify.IsNull(list.ContainerFromIndex(targetIndex));

                // This method validates that we don't raise CCC after a
                // ScrollIntoView call when the list is outside of the visual
                // tree.
                // The reason for this is that we can't prepare a new container
                // correctly outside of the visual tree. The template of the container
                // is simply not applied.
                int cccCounter = 0;
                list.ContainerContentChanging += (o, e) =>
                {
                    Verify.IsNotNull(e.ItemContainer.ContentTemplateRoot);
                    ++cccCounter;
                };

                list.ScrollIntoView(list.Items[targetIndex]);
                Verify.AreEqual(0, cccCounter);
            });
        }

        private static Rect GetBounds(FrameworkElement element)
        {
            Rect rect = new Rect();
            UIExecutor.Execute(() =>
            {
                var point1 = element.TransformToVisual(null).TransformPoint(new Point(0, 0));
                var point2 = element.TransformToVisual(null).TransformPoint(new Point((float)element.ActualWidth, (float)element.ActualHeight));

                rect.X = Math.Min(point1.X, point2.X);
                rect.Y = point1.Y;
                rect.Width = Math.Abs(point1.X - point2.X);
                rect.Height = point2.Y - point1.Y;
            });

            return rect;
        }

        [TestMethod]
        public void PageDownWithNoCache()
        {
            ListView list = null;
            IList<string> data = Enumerable.Range(0, 100).Select(x => "Item " + x).ToList();

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.Height = 300;
                list.ItemsSource = data;
                list.Loaded += (s, e) =>
                {
                    (list.ItemsPanelRoot as ItemsStackPanel).CacheLength = 0.75;
                };

                TestServices.WindowHelper.WindowContent = list;

                Verify.AreEqual(-1, list.SelectedIndex);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                list.Focus(FocusState.Keyboard);
            });

            TestServices.WindowHelper.WaitForIdle();
            for (int i = 0; i < 3; i++)
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_pagedown#$u$_pagedown");
            }

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                // verify that the selection moved and we did not crash.
                Verify.IsGreaterThan(list.SelectedIndex, 17);
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void FocusResetAfterSwitchingItemsSource()
        {
            ListView list = null;
            FrameworkElement itemContainerToTap = null;
            int indexOfItemToTap = 2;
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = LoadXamlFileIntoWindow(@"resources\managed\controls\ListViewBase\ListView.xaml");
                list.ItemsSource = Enumerable.Range(0, 10).Select(x => "Item " + x).ToList(); ;
                list.Loaded += (s, e) =>
                {
                    listLoaded.Set();
                };
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                itemContainerToTap = (FrameworkElement)list.ContainerFromIndex(indexOfItemToTap);
            });

            TestServices.InputHelper.Tap(itemContainerToTap);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(indexOfItemToTap, list.SelectedIndex);
                list.ItemsSource = Enumerable.Range(0, 10).Select(x => "Item " + x).ToList();
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ListViewItem;
                Verify.IsNotNull(focusedElement);
                int index = list.IndexFromContainer(focusedElement);

                Verify.AreEqual(0, index);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void GroupPaddingShouldReduceMeasureSizeForHeadersAndItems_ItemsStackPanel()
        {
            GroupPaddingShouldReduceMeasureSizeForHeadersAndItems(
                @"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                    <ItemsStackPanel GroupPadding='10,20,30,40' Orientation='Vertical'/>
                 </ItemsPanelTemplate>");
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void GroupPaddingShouldReduceMeasureSizeForHeadersAndItems_ItemsWrapGrid()
        {
            GroupPaddingShouldReduceMeasureSizeForHeadersAndItems(
                @"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                    <ItemsWrapGrid GroupPadding='10,20,30,40' Orientation='Horizontal'/>
                 </ItemsPanelTemplate>",
                true /*isItemsWrapGrid*/);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateListViewHeaderArrangBounds()
        {
            ValidateHeaderArrangBounds(true /*useListView*/);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateGridViewHeaderArrangeBounds()
        {
            ValidateHeaderArrangBounds(false /*useListView*/);
        }

        public void ValidateHeaderArrangBounds(bool useListView)
        {
            Microsoft.UI.Xaml.Controls.ListViewBase list = null;
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                if (useListView)
                {
                    list = new ListView();
                }
                else
                {
                    list = new GridView();
                }

                list.ShowsScrollingPlaceholders = false;
                list.Height = 500;
                list.Width = 400;

                list.Loaded += (s, e) =>
                {
                    listLoaded.Set();
                };

                var data = new ObservableCollection<ObservableCollection<int>>();
                var group = new ObservableCollection<int>();
                group.Add(0);
                data.Add(group);

                var cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = data;
                list.ItemsSource = cvs.View;

                list.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                list.ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
                   "     <TextBlock Text='{Binding}' />  " +
                   " </DataTemplate>");

                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                if (useListView)
                {
                    var isp = list.ItemsPanelRoot as ItemsStackPanel;
                    Verify.IsNotNull(isp);
                    isp.GroupPadding = new Thickness(10, 20, 30, 40);
                    isp.GroupHeaderPlacement = GroupHeaderPlacement.Top;
                }
                else
                {
                    var iwg = list.ItemsPanelRoot as ItemsWrapGrid;
                    Verify.IsNotNull(iwg);
                    iwg.GroupPadding = new Thickness(10, 20, 30, 40);
                }

                list.InvalidateMeasure();
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var container = list.ContainerFromIndex(0);
                var header = list.GroupHeaderContainerFromItemContainer(container) as FrameworkElement;
                // 400 - (10+30)//group padding
                Verify.AreEqual(360, header.ActualWidth);
            });
        }

        private void GroupPaddingShouldReduceMeasureSizeForHeadersAndItems(string panelTemplate, bool isItemsWrapGrid = false)
        {
            ListView list = null;
            var data = GetGroupedData(numGroups: 2, itemsPerGroup: 10);

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.ShowsScrollingPlaceholders = false;
                list.Height = 300;
                list.Width = 300;

                // Expected Width for available size in measure is 260 pixels
                // windowConstraint - (grouppadding.Left + groupPadding.Right)
                // 300 - (10 + 30) = 260
                list.ChoosingGroupHeaderContainer += (s, args) =>
                {
                    args.GroupHeaderContainer = new MyListViewHeaderItem() { Padding = new Thickness(0), ExpectedWidth = 260 };
                };

                list.ChoosingItemContainer += (s, args) =>
                {
                    args.ItemContainer = new MyListViewItem() { Padding = new Thickness(0), ExpectedWidth = 260, IsParentItemsWrapGrid = isItemsWrapGrid };
                };

                var cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = data;
                list.ItemsSource = cvs.View;

                list.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   @"<GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <GroupStyle.HeaderTemplate>
                            <DataTemplate>
                                <TextBlock Text='{Binding GroupTitle}'/>
                            </DataTemplate>
                        </GroupStyle.HeaderTemplate>
                    </GroupStyle>"));

                list.ItemsPanel = (ItemsPanelTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(panelTemplate);

                TestServices.WindowHelper.WindowContent = list;
            });

            TestServices.WindowHelper.WaitForIdle();

            // verification is done in Measure Override of MyListViewItem and MyListViewHeaderItem
            // we just ensure below that items were shown on screen

            UIExecutor.Execute(() =>
            {
                int lastVisibleIndex = 0;
                if (list.ItemsPanelRoot is ItemsStackPanel)
                {
                    lastVisibleIndex = (list.ItemsPanelRoot as ItemsStackPanel).LastCacheIndex;
                }
                else
                {
                    lastVisibleIndex = (list.ItemsPanelRoot as ItemsWrapGrid).LastCacheIndex;
                }

                Verify.AreNotEqual(lastVisibleIndex, 0);
            });
        }

        [TestMethod]
        public void SettingProgrammaticFocusDoesNotGetRedirectedToSelectedItem()
        {
            ListView listView = null;
            Button button = null;

            const int selectedIndex = 1;
            const int expectedFocusedIndex = 2;
            ListViewItem expectedFocusedItem = null;

            UIExecutor.Execute(() =>
            {
                listView = new ListView();
                for (uint i = 0; i < 5; ++i)
                {
                    listView.Items.Add(string.Format("Item {0}", i + 1));
                }
                listView.SingleSelectionFollowsFocus = true;
                listView.SelectedIndex = selectedIndex;

                button = new Button() { Content = "button" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(listView);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                button.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                expectedFocusedItem = listView.ContainerFromIndex(expectedFocusedIndex) as ListViewItem;
                expectedFocusedItem.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.IsTrue(focusedElement.Equals(expectedFocusedItem));
            });
        }

        [TestMethod]
        public void ValidateNonGroupingFocusOrderWithTabNavigationLocal()
        {
            ListView listView = null;
            Button button = null;

            int numItems = 5;

            UIExecutor.Execute(() =>
            {
                listView = new ListView() { TabNavigation = KeyboardNavigationMode.Local };
                for (int i = 0; i < numItems; ++i)
                {
                    // Disable all the even numbered items.
                    listView.Items.Add(new ListViewItem() { Content = string.Format("Item {0}", i + 1) });
                }

                button = new Button() { Content = "button" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(listView);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            // Start with focus on the button.
            UIExecutor.Execute(() =>
            {
                button.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            // Validate forward tab navigation.
            for (int i = 0; i < numItems; ++i)
            {
                TestServices.KeyboardHelper.Tab();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var expectedFocusedItem = listView.Items[i] as ListViewItem;
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.AreEqual(expectedFocusedItem, focusedElement);
                });
            }

            // Tab once more to get focus back on the button.
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            // Validate backward tab navigation.
            for (int i = numItems - 1; i >= 0; --i)
            {
                TestServices.KeyboardHelper.ShiftTab();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var expectedFocusedItem = listView.Items[i] as ListViewItem;
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.AreEqual(expectedFocusedItem, focusedElement);
                });
            }
        }

        [TestMethod]
        public void DoesTabNavigationLocalSkipOverDisabledItems()
        {
            ListView listView = null;
            Button button = null;

            UIExecutor.Execute(() =>
            {
                listView = new ListView() { TabNavigation = KeyboardNavigationMode.Local };
                for (uint i = 0; i < 5; ++i)
                {
                    // Disable all the even numbered items.
                    listView.Items.Add(new ListViewItem() { Content = string.Format("Item {0}", i + 1), IsEnabled = (i % 2 != 0) });
                }

                button = new Button() { Content = "button" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(listView);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                button.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            // We should have skipped over the 1st item, because it is disabled, and
            // focused the second one.
            UIExecutor.Execute(() =>
            {
                var expectedFocusedItem = listView.Items[1] as ListViewItem;
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(expectedFocusedItem, focusedElement);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            // We should have skipped over the 3rd item, because it is disabled, and
            // focused the fourth one.
            UIExecutor.Execute(() =>
            {
                var expectedFocusedItem = listView.Items[3] as ListViewItem;
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(expectedFocusedItem, focusedElement);
            });
        }

        [TestMethod]
        public void DoesBottomsUpListTabNavigationInitiallyFocusLastItem()
        {
            ListView listView = null;
            Button button = null;

            UIExecutor.Execute(() =>
            {
                listView = new ListView();
                for (uint i = 0; i < 5; ++i)
                {
                    // Disable all the even numbered items.
                    listView.Items.Add(new ListViewItem() { Content = string.Format("Item {0}", i + 1) });
                }

                button = new Button() { Content = "button" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(listView);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var itemsStackPanel = listView.ItemsPanelRoot as ItemsStackPanel;
                itemsStackPanel.ItemsUpdatingScrollMode = ItemsUpdatingScrollMode.KeepLastItemInView;

                button.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var expectedFocusedItem = listView.Items[listView.Items.Count - 1] as ListViewItem;
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(expectedFocusedItem, focusedElement);
            });
        }

        [TestMethod]
        public void DoesRestoreFocusToPreviouslyFocusedItemWhenTabbingBackIn()
        {
            ListView listView = null;
            Button button = null;

            const int expectedFocusIndex = 3;

            UIExecutor.Execute(() =>
            {
                listView = new ListView();
                for (uint i = 0; i < 5; ++i)
                {
                    // Disable all the even numbered items.
                    listView.Items.Add(new ListViewItem() { Content = string.Format("Item {0}", i + 1) });
                }

                button = new Button() { Content = "button" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(listView);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var expectedFocusedItem = listView.Items[expectedFocusIndex] as ListViewItem;
                expectedFocusedItem.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            // Tab once to get out of the list and put focus on the button.
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            // Tab again to put focus back into the list.
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var expectedFocusedItem = listView.Items[expectedFocusIndex] as ListViewItem;
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(expectedFocusedItem, focusedElement);
            });
        }

        [TestMethod]
        public void DoesNotHandleKeyboardArrowKeysWithFocusOnHeaderOrFooter()
        {
            ListView listView = null;
            Button header = null;
            Button footer = null;

            var keyDownEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                listView = new ListView();
                listView.Items.Add("Item");
                header = new Button() { Content = "Header" };
                footer = new Button() { Content = "Footer" };

                listView.Header = header;
                listView.Footer = footer;

                var root = new Grid();
                root.Children.Add(listView);

                root.KeyDown += (s, e) => { keyDownEvent.Set(); };

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            Action<Button> runScenario = (Button target) =>
            {
                UIExecutor.Execute(() =>
                {
                    target.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.Left();
                TestServices.WindowHelper.WaitForIdle();
                keyDownEvent.WaitOne();
                UIExecutor.Execute(() => { Verify.AreEqual(target, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)); });

                TestServices.KeyboardHelper.Right();
                TestServices.WindowHelper.WaitForIdle();
                keyDownEvent.WaitOne();
                UIExecutor.Execute(() => { Verify.AreEqual(target, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)); });

                TestServices.KeyboardHelper.Up();
                TestServices.WindowHelper.WaitForIdle();
                keyDownEvent.WaitOne();
                UIExecutor.Execute(() => { Verify.AreEqual(target, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)); });

                TestServices.KeyboardHelper.Down();
                TestServices.WindowHelper.WaitForIdle();
                keyDownEvent.WaitOne();
                UIExecutor.Execute(() => { Verify.AreEqual(target, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)); });
            };

            Log.Comment("Validate arrow keys are not handled for header object.");
            runScenario(header);

            Log.Comment("Validate arrow keys are not handled for footer object.");
            runScenario(footer);
        }

        [TestMethod]
        public void HeaderOrFooterDoesNotTrap2DNavigation()
        {
            ListView listView = null;
            Button beforeButton = null;
            Button afterButton = null;

            var keyDownEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                listView = new ListView();
                listView.Items.Add("Item");
                listView.Header = new Button() { Content = "Header" };
                listView.Footer = new Button() { Content = "Footer" };

                beforeButton = new Button() { Content = "Before" };
                afterButton = new Button() { Content = "After" };

                var root = new StackPanel() { XYFocusKeyboardNavigation = XYFocusKeyboardNavigationMode.Enabled };
                root.Children.Add(beforeButton);
                root.Children.Add(listView);
                root.Children.Add(afterButton);

                root.KeyDown += (s, e) => { keyDownEvent.Set(); };

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                beforeButton.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            TestServices.KeyboardHelper.Down();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => { Verify.AreEqual(listView.Header, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)); });

            TestServices.KeyboardHelper.Down();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => { Verify.AreEqual(listView.ContainerFromIndex(0), FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)); });

            TestServices.KeyboardHelper.Down();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => { Verify.AreEqual(listView.Footer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)); });

            TestServices.KeyboardHelper.Down();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() => { Verify.AreEqual(afterButton, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)); });
        }

        [TestMethod]
        [TestProperty("Description", "If focus is set to a ListViewItem programmatically (or via 2D navigation) and the item is then selected, it should not require 2 tabs to move focus out of the ListView.")]
        public void DoesNotRequireTwoTabsToMoveFocusWhenItemIsFocusedProgrammaticallyAndThenSelected()
        {
            ListView listView = null;
            Button button = null;

            UIExecutor.Execute(() =>
            {
                listView = new ListView();
                listView.Items.Add("Item");

                button = new Button() { Content = "Button" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(listView);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Programmatically focus the item and then select it.");
            UIExecutor.Execute(() =>
            {
                var item = (ListViewItem)listView.ContainerFromIndex(0);
                item.Focus(FocusState.Keyboard);

                listView.SelectedIndex = 0;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Press TAB -> focus should move out of the ListView.");
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(button, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
        }

        [TestMethod]
        public void ValidateKeyNavWithVariableSizedItems()
        {
            GridView gridView = null;
            UIExecutor.Execute(() =>
            {
                // Gridview with three items spanning multiple rows. Even when items span multiple rows, key navigation
                // should go through items only once. We do not wrap at the edges since the items might be non-contiguous.
                // | 0 | 1 |
                // |   2   |
                gridView = (GridView)XamlReader.Load(@"<GridView Width='200' Height='300' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                            <GridView.ItemsPanel>
                                                                <ItemsPanelTemplate>
                                                                    <VariableSizedWrapGrid ItemHeight='100' ItemWidth='100' MaximumRowsOrColumns='6' Orientation='Horizontal' />
                                                                </ItemsPanelTemplate>
                                                            </GridView.ItemsPanel>
                                                            <GridViewItem VariableSizedWrapGrid.ColumnSpan='1' VariableSizedWrapGrid.RowSpan='2'>0</GridViewItem>
                                                            <GridViewItem VariableSizedWrapGrid.ColumnSpan='1' VariableSizedWrapGrid.RowSpan='2'>1</GridViewItem>
                                                            <GridViewItem VariableSizedWrapGrid.ColumnSpan='1' VariableSizedWrapGrid.RowSpan='1'>2</GridViewItem>
                                                        </GridView>");
                TestServices.WindowHelper.WindowContent = gridView;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                gridView.Focus(FocusState.Keyboard);
            });

            // layout is as follows. VSWG does not wrap on key navigation
            // | 0 | 1 |
            // |   2   |
            ValidateFocusedGridViewItemContent("0");

            TestServices.KeyboardHelper.Right();
            ValidateFocusedGridViewItemContent("1");

            TestServices.KeyboardHelper.Right();
            ValidateFocusedGridViewItemContent("1");

            TestServices.KeyboardHelper.Left();
            ValidateFocusedGridViewItemContent("0");

            TestServices.KeyboardHelper.Left();
            ValidateFocusedGridViewItemContent("0");

            TestServices.KeyboardHelper.Down();
            ValidateFocusedGridViewItemContent("2");

            TestServices.KeyboardHelper.Down();
            ValidateFocusedGridViewItemContent("2");

            TestServices.KeyboardHelper.Up();
            ValidateFocusedGridViewItemContent("0");

            TestServices.KeyboardHelper.Up();
            ValidateFocusedGridViewItemContent("0");
        }

        [TestMethod]
        public void ValidateHomeEndKeyNavWithDisabledItems()
        {
            GridView gridView = null;

            UIExecutor.Execute(() =>
            {
                gridView = (GridView)XamlReader.Load(@"<GridView  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                          <GridViewItem IsEnabled='False'>0</GridViewItem>
                                                          <GridViewItem IsEnabled='False'>1</GridViewItem>
                                                          <GridViewItem>2</GridViewItem>
                                                          <GridViewItem>3</GridViewItem>
                                                          <GridViewItem>4</GridViewItem>
                                                          <GridViewItem>5</GridViewItem>
                                                          <GridViewItem IsEnabled='False'>6</GridViewItem>
                                                          <GridViewItem IsEnabled='False'>7</GridViewItem>
                                                      </GridView>");
                TestServices.WindowHelper.WindowContent = gridView;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                gridView.Focus(FocusState.Keyboard);
            });

            TestServices.WindowHelper.WaitForIdle();
            Verify.AreEqual(2, GetFocusedItemIndex(gridView));

            // End should put focus on the last eligible item which is 5
            TestServices.KeyboardHelper.End();
            TestServices.WindowHelper.WaitForIdle();
            Verify.AreEqual(5, GetFocusedItemIndex(gridView));

            // Home should put focus on first eligible item which is 2
            TestServices.KeyboardHelper.Home();
            TestServices.WindowHelper.WaitForIdle();
            Verify.AreEqual(2, GetFocusedItemIndex(gridView));

            // Disable all but one of the items in the girdview. Focus should
            // stay on the same item.
            UIExecutor.Execute(() =>
            {
                ((GridViewItem)gridView.ContainerFromIndex(2)).IsEnabled = false;
                ((GridViewItem)gridView.ContainerFromIndex(3)).Visibility = Visibility.Collapsed;
                ((GridViewItem)gridView.ContainerFromIndex(4)).IsTabStop = false;

                // set focus on the only focusible item
                ((GridViewItem)gridView.ContainerFromIndex(5)).Focus(FocusState.Keyboard);
            });

            // End should put focus on the last eligible item which is 5
            TestServices.KeyboardHelper.End();
            TestServices.WindowHelper.WaitForIdle();
            Verify.AreEqual(5, GetFocusedItemIndex(gridView));

            // Home should put focus on first eligible item which is also 5
            TestServices.KeyboardHelper.Home();
            TestServices.WindowHelper.WaitForIdle();
            Verify.AreEqual(5, GetFocusedItemIndex(gridView));
        }

        [TestMethod]
        [Description("Home/End key press when focus is on a group header caused us to mess up key navigation")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateListViewHomeEndAndArrowKeyNav()
        {
            ListView listView = null;
            const int numGroups = 30;
            const int itemsPerGroup = 2;
            UIExecutor.Execute((Action)(() =>
            {
                listView = new ListView();
                listView.ShowsScrollingPlaceholders = false;
                listView.Height = 500;
                var cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = GetGroupedData(numGroups, itemsPerGroup);

                listView.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));
                listView.ItemsSource = cvs.View;
                TestServices.WindowHelper.WindowContent = listView;
            }));

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var thirdContainer = listView.ContainerFromIndex(2);
                var secondHeader = (ListViewHeaderItem)listView.GroupHeaderContainerFromItemContainer(thirdContainer);
                secondHeader.Focus(FocusState.Keyboard);
            });

            const int totalNumberOfItems = numGroups * itemsPerGroup;
            TestServices.WindowHelper.WaitForIdle();
            TestServices.KeyboardHelper.End();
            Verify.AreEqual(totalNumberOfItems - 1, GetFocusedItemIndex(listView));

            TestServices.KeyboardHelper.Up();
            Verify.AreEqual(totalNumberOfItems - 2, GetFocusedItemIndex(listView));

            TestServices.KeyboardHelper.Up();
            UIExecutor.Execute(() =>
            {
                var focusedElement = (ListViewHeaderItem)FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.IsNotNull(focusedElement);
            });

            TestServices.WindowHelper.WaitForIdle();
            TestServices.KeyboardHelper.Home();
            Verify.AreEqual(0, GetFocusedItemIndex(listView));

            TestServices.KeyboardHelper.Down();
            Verify.AreEqual(1, GetFocusedItemIndex(listView));
        }

        [TestMethod]
        [TestProperty("Description", "When a ListView is the root focusable control in the tree, validate that focus does not get trapped within the header sub-tree with multiple focusable controls.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanTabOutOfHeaderWithMultipleFocusableChildren()
        {
            ListView listView = null;
            Button button1 = null;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"<ListView
                                                           xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                                           <ListView.Header>
                                                               <StackPanel>
                                                                   <Button x:Name='button1' Content='Button 1'/>
                                                                   <Button Content='Button 2'/>
                                                               </StackPanel>
                                                           </ListView.Header>
                                                           <ListViewItem Content='Item 1'/>
                                                       </ListView>");

                button1 = (Button)listView.FindName("button1");

                TestServices.WindowHelper.WindowContent = listView;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                listView.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                if (!Object.ReferenceEquals(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button1))
                {
                    throw new Exception("First button in the header should have focus.");
                }
            });

            Log.Comment("Press TAB twice, which should move focus past the second header button and onto the first item.");
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var firstItemContainer = listView.ContainerFromIndex(0);
                Verify.AreEqual(firstItemContainer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
        }

        [TestMethod]
        [TestProperty("Description", "When a ListView is the root focusable control in the tree, validate that focus does not get trapped within the footer sub-tree with multiple focusable controls.")]
        public void CanShiftTabOutOfFooterWithMultipleFocusableChildren()
        {
            ListView listView = null;
            Button button2 = null;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"<ListView
                                                           xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                                           <ListView.Footer>
                                                               <StackPanel>
                                                                   <Button Content='Button 1'/>
                                                                   <Button x:Name='button2' Content='Button 2'/>
                                                               </StackPanel>
                                                           </ListView.Footer>
                                                           <ListViewItem Content='Item 1'/>
                                                       </ListView>");

                button2 = (Button)listView.FindName("button2");

                TestServices.WindowHelper.WindowContent = listView;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                button2.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Press SHIFT-TAB twice, which should move focus past the first footer button and onto the first item.");
            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var firstItemContainer = listView.ContainerFromIndex(0);
                Verify.AreEqual(firstItemContainer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
        }

        [TestMethod]
        [TestProperty("Description", "If the selected item's container is not focusable, validate that focus is sent to children instead.")]
        public void DoesFocusChildrenOfSelectedItemIfContainerIsNotFocusable()
        {
            StackPanel root = null;
            ListView listView = null;
            Button beforeButton = null;
            Button afterButton = null;
            Button firstFocusableChild = null;
            Button lastFocusableChild = null;

            UIExecutor.Execute(() =>
            {
                firstFocusableChild = new Button() { Content = "First Focusable" };
                lastFocusableChild = new Button() { Content = "Last Focusable" };

                var itemContentRoot = new StackPanel();
                itemContentRoot.Children.Add(firstFocusableChild);
                itemContentRoot.Children.Add(lastFocusableChild);

                listView = new ListView();
                listView.Items.Add(new ListViewItem() { IsTabStop = false, Content = itemContentRoot });
                listView.SelectedIndex = 0;

                beforeButton = new Button() { Content = "Before" };
                afterButton = new Button() { Content = "After" };

                root = new StackPanel()
                {
                    XYFocusKeyboardNavigation = XYFocusKeyboardNavigationMode.Enabled,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center
                };

                root.Children.Add(beforeButton);
                root.Children.Add(listView);
                root.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            Action<Action, Action, UIElement, UIElement> runScenario = (Action fowardNav, Action backwardNav, UIElement forwardExpectedFocused, UIElement backwardExpectedFocused) =>
            {
                Log.Comment("Start focus on the control before the ListView.");
                UIExecutor.Execute(() =>
                {
                    beforeButton.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Navigate Focus Forwards -> focus should move into the selected item's container and focus the forwardExpectedFocused element.");
                fowardNav();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(forwardExpectedFocused, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });

                Log.Comment("Reset focus to the control after the ListView.");
                UIExecutor.Execute(() =>
                {
                    afterButton.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Navigate Focus Backwards -> focus should move into the selected item's container and focus the backwardExpectedFocused element.");
                backwardNav();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(backwardExpectedFocused, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });
            };

            Log.Comment("Run Scenario with Tab/Shift-Tab.");
            runScenario(() => { TestServices.KeyboardHelper.Tab(); }, () => { TestServices.KeyboardHelper.ShiftTab(); }, firstFocusableChild, lastFocusableChild);

            Log.Comment("Run Scenario with Down/Up.");
            runScenario(() => { TestServices.KeyboardHelper.Down(); }, () => { TestServices.KeyboardHelper.Up(); }, firstFocusableChild, firstFocusableChild);

            // The following scenarios use horizontal navigation, so change the orientation of the panel.
            UIExecutor.Execute(() => { root.Orientation = Orientation.Horizontal; });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Run Scenario with Right/Left.");
            runScenario(() => { TestServices.KeyboardHelper.Right(); }, () => { TestServices.KeyboardHelper.Left(); }, firstFocusableChild, firstFocusableChild);

            // The next scenario requires us to be in RTL.
            UIExecutor.Execute(() => { root.FlowDirection = FlowDirection.RightToLeft; });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Run Scenario with Left/Right (RTL-case).");
            runScenario(() => { TestServices.KeyboardHelper.Left(); }, () => { TestServices.KeyboardHelper.Right(); }, firstFocusableChild, firstFocusableChild);
        }


        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanRemoveGroupFromListViewWithEmptyGroupAfter()
        {
            ListView listView = null;

            ObservableCollection<Group> data = null;

            UIExecutor.Execute(() =>
            {
                listView = new ListView();

                listView.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                    " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                    "     <GroupStyle.HeaderTemplate>                                                       " +
                    "         <DataTemplate>                                                                " +
                    "             <Grid Background='Red' >                                                  " +
                    "                 <TextBlock Text='{Binding Name}' Foreground='White' FontSize='30'/>   " +
                    "             </Grid>                                                                   " +
                    "         </DataTemplate>                                                               " +
                    "     </GroupStyle.HeaderTemplate>                                                      " +
                    " </GroupStyle>                                                                         "));

                var root = new Grid();
                root.Children.Add(listView);

                TestServices.WindowHelper.WindowContent = root;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                data = new ObservableCollection<Group>(new Group[]
                {
                    new Group("Non-empty Group #1", new string[] { "Item #1" }),
                    new Group("Non-empty Group #2", new string[] { "Item #2", "Item #3" }),
                    new Group("Empty group")
                });

                var cvs = new CollectionViewSource();
                cvs.Source = data;
                cvs.IsSourceGrouped = true;
                listView.ItemsSource = cvs.View;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                data.RemoveAt(1);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanRemoveFromGroupInListViewWithEmptyGroupBefore()
        {
            ListView listView = null;

            ObservableCollection<Group> data = null;

            UIExecutor.Execute(() =>
            {
                listView = new ListView();

                listView.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                    " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                    "     <GroupStyle.HeaderTemplate>                                                       " +
                    "         <DataTemplate>                                                                " +
                    "             <Grid Background='Red' >                                                  " +
                    "                 <TextBlock Text='{Binding Name}' Foreground='White' FontSize='30'/>   " +
                    "             </Grid>                                                                   " +
                    "         </DataTemplate>                                                               " +
                    "     </GroupStyle.HeaderTemplate>                                                      " +
                    " </GroupStyle>                                                                         "));

                var root = new Grid();
                root.Children.Add(listView);

                data = new ObservableCollection<Group>(new Group[]
                {
                    new Group("Empty Group", new string[] {}),
                    new Group("Non-empty Group #1", new string[] { "Item #1" })
                });

                var cvs = new CollectionViewSource();
                cvs.Source = data;
                cvs.IsSourceGrouped = true;
                listView.ItemsSource = cvs.View;

                TestServices.WindowHelper.WindowContent = root;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                data[1].Insert(0, "New Item");
                data[1].RemoveAt(1);

                // We used to crash during the next layout pass
                listView.UpdateLayout();

                // Validate the layout
                Verify.AreEqual("New Item", ((ListViewItem)listView.ContainerFromIndex(0)).Content);
                //Verify.AreEqual("Non-empty Group #1", ((ListViewHeaderItem)listView.GroupHeaderContainerFromItemContainer(listView.ContainerFromIndex(0))).Content);
                Verify.AreEqual("Non-empty Group #1", ((Group)((ListViewHeaderItem)listView.GroupHeaderContainerFromItemContainer(listView.ContainerFromIndex(0))).Content).Name);
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void CanSetSelectedItemBeforeItemsSource()
        {
            var data = Enumerable.Range(0, 10).Select(i => $"Item #{i}").ToList();
            Grid host = null;

            Func<UIElement, bool> isCommonStateSelected = (container) =>
            {
                var groups = VisualStateManager.GetVisualStateGroups((FrameworkElement)VisualTreeHelper.GetChild(container, 0));
                return groups[0].CurrentState.Name == "Selected";
            };
            Func<ListView> createList = () =>
            {
                var list = (ListView)XamlReader.Load("<ListView ItemContainerStyle='{ThemeResource ListViewItemExpanded}' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' />");
                list.ItemContainerTransitions = new Media.Animation.TransitionCollection(); // Speeds up the test execution.
                return list;
            };

            Log.Comment("Validate we can set SelectedItem before ItemsSource.");
            UIExecutor.Execute(() =>
            {
                host = new Grid();
                TestServices.WindowHelper.WindowContent = host;

                var list = createList();
                list.SelectedItem = data[3];
                list.ItemsSource = data;
                host.Children.Add(list);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var list = (ListView)host.Children[0];

                Verify.AreEqual(3, list.SelectedIndex);
                Verify.AreEqual(data[3], list.SelectedItem);

                list.ContainerFromIndex(list.SelectedIndex);
                Verify.IsTrue(isCommonStateSelected((ContentControl)list.ContainerFromItem(list.SelectedItem)));
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Validate we can set SelectedItem before ItemsSource when synchronising two lists with a collection view.");
            UIExecutor.Execute(() =>
            {
                var leftList = createList();
                var rightList = createList();

                var cvs = new CollectionViewSource();
                cvs.Source = data;

                leftList.SelectedItem = data[3];
                leftList.ItemsSource = cvs.View;
                rightList.ItemsSource = cvs.View;

                host.Children.Clear();
                host.Children.Add(leftList);
                host.Children.Add(rightList);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                foreach (ListView list in host.Children)
                {
                    Verify.AreEqual(3, list.SelectedIndex);
                    Verify.AreEqual(data[3], list.SelectedItem);
                    Verify.IsTrue(isCommonStateSelected((ContentControl)list.ContainerFromItem(list.SelectedItem)));
                }
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Validate we can set SelectedItem before ItemsSource when in multiple selection mode.");
            UIExecutor.Execute(() =>
            {
                var list = createList();
                list.SelectedItem = data[3];
                list.SelectionMode = ListViewSelectionMode.Multiple;
                list.SelectedItems.Add(data[1]);
                list.ItemsSource = data;
                list.SelectedItems.Add(data[5]);

                host.Children.Clear();
                host.Children.Add(list);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var list = (ListView)host.Children[0];

                Verify.AreEqual(3, list.SelectedIndex);
                Verify.AreEqual(data[3], list.SelectedItem);

                foreach (var item in list.SelectedItems)
                {
                    isCommonStateSelected((UIElement)list.ContainerFromItem(item));
                }
            });
        }

        [TestMethod]
        public void ValidateDoNotRestorePendingSelectionIfNotFound()
        {
            ListView list = null;
            bool attemptedToRestorePendingSelection = false;

            // This test is a follow-up to CanSetSelectedItemBeforeItemsSource.
            // Here we validate that if SelectedItem is set before ItemsSource and if it's
            // not in the data, then we don't attempt to reset it at all after the ItemsSource
            // is set.
            // When we set SelectedItem to a value that's not in the data, the framework
            // automatically undo the operation. However, it may cause issues if there is a
            // two-way binding on the SelectedItem. In that case, the source of the binding
            // will be set to null and some apps don't expect that.

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.SelectedItem = "This string (pending for selection) is not going to be found in the ItemsSource.";

                // Speeds up the test execution.
                {
                    list.ShowsScrollingPlaceholders = false;
                    list.ItemContainerTransitions = new Media.Animation.TransitionCollection();
                }

                list.RegisterPropertyChangedCallback(Selector.SelectedItemProperty, new DependencyPropertyChangedCallback((obj, property) =>
                {
                    // In the repro, we hit this path because Selector::OnItemsSourceChanged is attempting to restore
                    // an item that's not in the data. It's causing an issue because of the side effects
                    // of two way bindings on the SelectedItem.
                    attemptedToRestorePendingSelection = true;
                }));

                list.ItemsSource = Enumerable.Range(0, 10).Select(i => $"Item #{i}").ToList();
                TestServices.WindowHelper.WindowContent = list;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(attemptedToRestorePendingSelection);
                Verify.IsNull(list.SelectedItem);
                Verify.AreEqual(-1, list.SelectedIndex);
            });
        }

        [TestMethod]
        public void VerifyPhasingUsesIDataTemplateComponent()
        {
            ListView lv = null;
            int numPhases = 5;
            int numElements = 3;
            ObservableCollection<int> data = new ObservableCollection<int>(Enumerable.Range(0, numElements));
            TemplateComponentLogger logger = new TemplateComponentLogger();

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;

                lv.ContainerContentChanging += (s, args) =>
                {
                    if (!args.InRecycleQueue && args.Phase == 0)
                    {
                        XamlBindingHelper.SetDataTemplateComponent(args.ItemContainer.ContentTemplateRoot, new ElementTemplateComponent(numPhases, logger));
                    }
                };

                lv.ItemsSource = data;
                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                List<Tuple<int, int>> expected = new List<Tuple<int, int>>();
                for (int i = 0; i < numPhases; i++)
                {
                    for (int j = 0; j < numElements; j++)
                    {
                        expected.Add(new Tuple<int, int>(j /* data */, i /* phase */));
                        Log.Comment(i + " " + j);
                    }
                }

                Verify.AreEqual(expected.Count, logger.ProcessBindingCalls.Count);
                for (int i = 0; i < expected.Count; i++)
                {
                    var actualValue = logger.ProcessBindingCalls[i];
                    var expectedValue = expected[i];
                    Verify.AreEqual(expectedValue.Item1, actualValue.Item1);
                    Verify.AreEqual(expectedValue.Item2, actualValue.Item2);
                }

                Verify.AreEqual(0, logger.RecycleCalls.Count);

                data.RemoveAt(0);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(1, logger.RecycleCalls.Count);
                Verify.AreEqual(0, logger.RecycleCalls[0]);
            });
        }

        [TestMethod]
        public void VerifyVisibleIndicesAtDifferentZoomFactors()
        {
            ListView listView = null;
            ScrollViewer scroller = null;
            ItemsStackPanel panel = null;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"<ListView Width='400' Height='400'
                                                           xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                                           <ListView.ItemTemplate>
                                                                <DataTemplate>
                                                                    <TextBlock Text='{Binding}' Height='100' />
                                                                </DataTemplate>
                                                           </ListView.ItemTemplate >
                                                       </ListView>");
                listView.ItemsSource = Enumerable.Range(0, 1000);
                TestServices.WindowHelper.WindowContent = listView;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                scroller = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                panel = (ItemsStackPanel)listView.ItemsPanelRoot;
                Verify.AreEqual(0, panel.FirstVisibleIndex);
                Verify.AreEqual(3, panel.LastVisibleIndex);

                Log.Comment("Scrolling to offset 5000 ZoomFactor 1");
                scroller.ChangeView(0, 5000, null, true);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(50, panel.FirstVisibleIndex);
                Verify.AreEqual(53, panel.LastVisibleIndex);

                Log.Comment("Scrolling to offset 5000 ZoomFactor .5");
                scroller.ChangeView(0, 5000, 0.5f, true);
            });

            TestServices.WindowHelper.WaitForIdle();
            // WaitForIdle is not sufficient here as a wait for
            // the visuals to get updated.
            // Adding sleep as a workaround.
            Thread.Sleep(150);
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(100, panel.FirstVisibleIndex);
                Verify.AreEqual(107, panel.LastVisibleIndex);
                Log.Comment("Scrolling to offset 5000 ZoomFactor 1.5");
                scroller.ChangeView(0, 5000, 1.5f, true);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(33, panel.FirstVisibleIndex);
                Verify.AreEqual(35, panel.LastVisibleIndex);
            });
        }

        // Regression Test: ListView crash on RS3
        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateCollectionChangeAndResetBeforeLayout()
        {
            ListView lv = null;
            var groups = GetGroupedData(numGroups: 10, itemsPerGroup: 10);

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;
                var cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = groups;

                lv.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' />"));

                lv.ItemsSource = cvs.View;
                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            // Scroll down to the end of the list
            UIExecutor.Execute(() =>
            {
                lv.ScrollIntoView(groups[9][0]);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // Add an item before the viewport so that we start tracking
                groups[0].Add(10);
                // Reset before layout runs. We used to crash here.
                groups.Clear();
            });

            // Yay. we didn't crash.
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(0, groups.Count);
                Verify.IsNull(lv.ContainerFromIndex(0));
            });
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "OneCore, WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Ignore", "TRUE")] // TODO 20928844: Re-enable after investigating why software injection causes this to inconsistently fail.
        public void ValidateVisibilityChangeDuringDrag()
        {
            ListView listView = null;
            DragEventHandler dragHandler = null;
            FrameworkElement dragContainer = null;
            ManualResetEvent dragStarting = new ManualResetEvent(false);
            UIExecutor.Execute(() =>
            {
                listView = (ListView)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                  @"<ListView
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        CanReorderItems='True'
                        CanDragItems='True'
                        AllowDrop='True'>
                        <ListView.ItemTemplate>
                            <DataTemplate>
                                <Grid>
                                    <TextBlock Text='{Binding}' />
                                </Grid>
                            </DataTemplate>
                        </ListView.ItemTemplate>
                    </ListView>");
                dragHandler = new DragEventHandler((s, args) => { listView.Visibility = Visibility.Collapsed; });
                listView.AddHandler(UIElement.DragEnterEvent, dragHandler, true);
                listView.ItemsSource = new ObservableCollection<int>(Enumerable.Range(0, 2));
                TestServices.WindowHelper.WindowContent = listView;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(Visibility.Visible, listView.Visibility);
                dragContainer = (FrameworkElement)listView.ContainerFromIndex(0);
            });

            PressHoldAndDragFromCenter(dragContainer, 20, 100, .1, 500);
            TestServices.WindowHelper.WaitForIdle();
            TestServicesExtensions.EnsureForegroundWindow();

            UIExecutor.Execute(() =>
            {
                // Our drag handler will Collapse the ListView and drag should be cancelled.
                Verify.AreEqual(Visibility.Collapsed, listView.Visibility);
                var item = listView.ItemFromContainer(dragContainer);
                Verify.AreEqual(0, item);
                listView.RemoveHandler(UIElement.DragEnterEvent, dragHandler);
                listView.Visibility = Visibility.Visible;
                listView.DragItemsStarting += (sender, args) => { dragStarting.Set(); };
            });

            // Verify that the ListView is still kicking by trying a drag
            TestServices.WindowHelper.WaitForIdle();
            PressHoldAndDragFromCenter(dragContainer, 20, 0, .1, 500);
            TestServices.WindowHelper.WaitForIdle();
            TestServicesExtensions.EnsureForegroundWindow();
            Verify.IsTrue(dragStarting.WaitOne(1000));

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(Visibility.Visible, listView.Visibility);
                var item = listView.ItemFromContainer(dragContainer);
                Verify.AreEqual(0, item);
            });

            // Do a reorder
            TestServices.WindowHelper.WaitForIdle();
            PressHoldAndDragFromCenter(dragContainer, 20, 100, .1, 500);
            TestServices.WindowHelper.WaitForIdle();
            TestServicesExtensions.EnsureForegroundWindow();

            UIExecutor.Execute(() =>
            {
                // The items should be reordered now.
                Verify.AreEqual(Visibility.Visible, listView.Visibility);
                var item = listView.ItemFromContainer(dragContainer);
                Verify.AreNotEqual(0, item);
            });
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ValidateFocusAfterItemDeletion()
        {
            ListView list = null;
            Button button = null;
            var data = new ObservableCollection<int>(Enumerable.Range(0, 2));
            var listLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.ItemsSource = data;
                var sp = new StackPanel();
                button = new Button() { Content = " Hello " };
                sp.Children.Add(button);
                sp.Children.Add(list);
                TestServices.WindowHelper.WindowContent = list;
                list.Loaded += (s, e) => listLoaded.Set();
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "List failed to raise Loaded event.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                list.Focus(FocusState.Keyboard);
            });

            TestServices.WindowHelper.WaitForIdle();
            // Move to second item so that ListView tracks lastfocused index.
            TestServices.KeyboardHelper.Down();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                button.Focus(FocusState.Keyboard);
            });

            TestServices.WindowHelper.WaitForIdle();

            // Remove previously focused element.
            UIExecutor.Execute(() =>
            {
                data.RemoveAt(1);
            });

            TestServices.WindowHelper.WaitForIdle();

            // Tab should now move focus to first listview item.
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var item = list.ContainerFromIndex(0);
                var focused = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ListViewItem;
                Verify.IsNotNull(focused);
                Verify.AreEqual("0", focused.Content.ToString());
            });
        }

        [TestMethod]
        public void VerifyContainerAtRootOfItemTemplate()
        {
            VerifyContainerAtRootOfItemTemplate(hookToContainerContentChangingEvent: false);
        }

        [TestMethod]
        public void VerifyContainerAtRootOfItemTemplateWithCCC()
        {
            VerifyContainerAtRootOfItemTemplate(hookToContainerContentChangingEvent: true);
        }

        // Verify that if the ItemTemplate has a valid container at the root,
        // we don't create an implicit container on top of that.
        private void VerifyContainerAtRootOfItemTemplate(bool hookToContainerContentChangingEvent)
        {
            ListView lv = null;
            UIExecutor.Execute(() =>
            {
                lv = (ListView)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                 @"<ListView
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Height='400'>
                      <ListView.ItemTemplate>
                        <DataTemplate>
                            <ListViewItem Height='120'>
                                <TextBlock Text='{Binding}' />
                            </ListViewItem >
                        </DataTemplate >
                      </ListView.ItemTemplate>
                   </ListView>");

                if(hookToContainerContentChangingEvent)
                {
                    lv.ContainerContentChanging += (sender, args) =>
                    {
                        // just register to the event to exercise code path
                        // that used to set the content.
                    };
                }

                lv.ItemsSource = Enumerable.Range(0, 1000);
                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var container0 = (ListViewItem)lv.ContainerFromIndex(0);
                Verify.IsNotNull(container0);
                Verify.AreEqual(120.0, container0.Height);
                lv.ScrollIntoView(500);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var container500 = (ListViewItem)lv.ContainerFromIndex(500);
                Verify.IsNotNull(container500);
                Verify.AreEqual(120.0, (int)container500.Height);
            });
        }

        // Verify that if the ItemTemplateSelector has a valid container at the root of the template it returns,
        // we do not create an implicit container on top of that.
        [TestMethod]
        public void VerifyContainerAtRootWithTemplateSelector()
        {
            ListView lv = null;
            UIExecutor.Execute(() =>
            {
                lv = new ListView()
                {
                    Height = 400
                };

                var selector = new MyTemplateSelector()
                {
                    Templates =
                    {
                         (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <ListViewItem Height='120'>
                                <Button Content='{Binding}' />
                            </ListViewItem >
                        </DataTemplate >"),
                        (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <ListViewItem  Height='120'>
                                <TextBlock Text='{Binding}' />
                            </ListViewItem >
                        </DataTemplate>")
                    },
                    SelectTemplateFunc = (object item) => { return ((int)item) % 2; }
                };

                lv.ItemTemplateSelector = selector;

                lv.ItemsSource = Enumerable.Range(0, 1000);
                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var container0 = (ListViewItem)lv.ContainerFromIndex(0);
                Verify.IsNotNull(container0);
                var button = container0.Content as Button;
                Verify.IsNotNull(button);
                Verify.AreEqual(0, (int)button.DataContext);

                var container1 = (ListViewItem)lv.ContainerFromIndex(1);
                Verify.IsNotNull(container1);
                var tb = container1.Content as TextBlock;
                Verify.IsNotNull(tb);
                Verify.AreEqual("1", tb.Text.ToString());

                lv.ScrollIntoView(500);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var container500 = (ListViewItem)lv.ContainerFromIndex(500);
                Verify.IsNotNull(container500);
                var button = container500.Content as Button;
                Verify.IsNotNull(container500);
                Verify.AreEqual(500, (int)button.DataContext);
            });
        }

        [TestMethod]
        public void VerifySettingSelectedIndexBeforeItemsInMarkup()
        {
            ListView list = null;
            ManualResetEvent listLoaded = new ManualResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = (ListView)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                 @"<ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        SelectedIndex='2'>
                        <ListView.Items>
                            <TextBlock>1</TextBlock>
                            <TextBlock>2</TextBlock>
                            <TextBlock>3</TextBlock>
                            <TextBlock>4</TextBlock>
                            <TextBlock>5</TextBlock>
                        </ListView.Items>
                   </ListView>");
                list.Loaded += (sender, args) =>
                {
                    listLoaded.Set();
                };

                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(2, list.SelectedIndex);
            });
        }

        [TestMethod]
        public void VerifySettingSelectedIndexBeforeItemsInCode()
        {
            ListView list = null;
            ManualResetEvent listLoaded = new ManualResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.Loaded += (sender, args) =>
                {
                    listLoaded.Set();
                };

                list.SelectedIndex = 1;
                list.Items.Add(0);
                list.Items.Add(1);
                list.Items.Add(2);
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(1, list.SelectedIndex);
            });
        }

        [TestMethod]
        public void VerifySettingSelectedIndexBeforeItemsSource()
        {
            ListView list = null;
            ManualResetEvent listLoaded = new ManualResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.Loaded += (sender, args) =>
                {
                    listLoaded.Set();
                };

                list.SelectedIndex = 1;
                list.ItemsSource = Enumerable.Range(0, 5);
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from list");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(1, list.SelectedIndex);
            });
        }

        // Regression test: [CRASH] Crash on updating list view with empty headers
        [TestMethod]
        public void ValidateAddingGroupWithItemInMiddle()
        {
            ListView lv = null;
            CollectionViewSource cvs = null;
            var groups = GetGroupedData(numGroups: 3, itemsPerGroup: 0);

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;
                cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = groups;

                lv.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                lv.ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
                   "     <TextBlock Text='{Binding}' />  " +
                   " </DataTemplate>                         ");

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                lv.ItemsSource = cvs.View;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                groups.RemoveAt(1);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var newGroup = new ObservableCollection<int>();
                newGroup.Add(500);
                groups.Insert(1, newGroup);
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void ValidateTabNavAfterDeletingFocusedElement()
        {
            GridView gridView = null;
            Button beforeButton = null;
            Button afterButton = null;
            UIExecutor.Execute(() =>
            {
                var root = (StackPanel)XamlReader.Load(@" <StackPanel  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                                     xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >
                                                            <Button Content='BeforeButton' x:Name='beforeButton' />
                                                            <GridView x:Name='gridView' IsTabStop='False' SelectionMode='None'>
                                                                <GridView.Items>
                                                                    <x:String>Item</x:String>
                                                                </GridView.Items>
                                                                <GridView.ItemTemplate>
                                                                    <DataTemplate>
                                                                        <Button Content='{Binding}' />
                                                                    </DataTemplate>
                                                                </GridView.ItemTemplate>
                                                                <GridView.ItemContainerStyle>
                                                                    <Style TargetType='GridViewItem'>
                                                                        <Setter Property='IsTabStop' Value='False'/>
                                                                    </Style>
                                                                </GridView.ItemContainerStyle>
                                                                <GridView.ItemContainerTransitions>
                                                                    <TransitionCollection />
                                                                </GridView.ItemContainerTransitions>
                                                            </GridView>
                                                            <Button Content='AfterButton' x:Name='afterButton' />
                                                        </StackPanel>");
                TestServices.WindowHelper.WindowContent = root;
                gridView = (GridView)root.FindName("gridView"); 
                beforeButton = (Button)root.FindName("beforeButton"); 
                afterButton = (Button)root.FindName("afterButton"); 
            });

            TestServices.WindowHelper.WaitForIdle();

            // set focus on the first item
            UIExecutor.Execute(() =>
            {
                gridView.Focus(FocusState.Keyboard);
            });

            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var focusedItem  =  (Button)FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual("Item", focusedItem.Content.ToString());

                // Delete the focused Item
                gridView.Items.RemoveAt(0);
            });

            // set focus on the before button and add a new item
            UIExecutor.Execute(() =>
            {
                beforeButton.Focus(FocusState.Keyboard);
                gridView.Items.Add("Item1");
            });

            TestServices.WindowHelper.WaitForIdle();
            TestServices.KeyboardHelper.Tab();

            // Validate that tabbing from before button, goes into the gridview and sets focus on the new first item 
            UIExecutor.Execute(() =>
            {
                var focusedItem  =  (Button)FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual("Item1", focusedItem.Content.ToString());
            });
        }

        public partial class MyTemplateSelector : DataTemplateSelector
        {
            private List<DataTemplate> _templates= new List<DataTemplate>();
            public List<DataTemplate> Templates
            {
                get
                {
                    return _templates;
                }
            }

            public Func<object, int> SelectTemplateFunc;

            protected override DataTemplate SelectTemplateCore(object item, DependencyObject container)
            {
                return Templates[SelectTemplateFunc(item)];
            }

            protected override DataTemplate SelectTemplateCore(object item)
            {
                return Templates[SelectTemplateFunc(item)];
            }
        }

        public class Group : ObservableCollection<string>
        {
            public string Name { get; }
            public Group(string name)
            {
                Name = name;
            }
            public Group(string name, IEnumerable<string> items)
                : base(items)
            {
                Name = name;
            }
        }

        public partial class MyListViewItem : ListViewItem
        {
            public double ExpectedWidth { get; set; }

            public bool IsParentItemsWrapGrid { get; set; }

            protected override Size MeasureOverride(Size availableSize)
            {
                Log.Comment("Item Available Size:" + availableSize);
                if (!IsParentItemsWrapGrid || (IsParentItemsWrapGrid && firstMeasure))
                {
                    Verify.AreEqual(ExpectedWidth, availableSize.Width);
                    firstMeasure = false;
                }

                return base.MeasureOverride(availableSize);
            }

            private static bool firstMeasure = true;
        }

        public partial class MyListViewHeaderItem : ListViewHeaderItem
        {
            public double ExpectedWidth { get; set; }

            protected override Size MeasureOverride(Size availableSize)
            {
                Log.Comment("Header Available Size:" + availableSize);
                Verify.AreEqual(ExpectedWidth, availableSize.Width);
                return base.MeasureOverride(availableSize);
            }
        }

        private void ValidateFocusedGridViewItemContent(string content)
        {
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as GridViewItem;
                Verify.IsNotNull(focused);
                Verify.AreEqual(content, focused.Content.ToString());
            });
        }

        private VisualStateGroup GetCommonStates(ListViewItem item)
        {
            FrameworkElement itemTemplate = (FrameworkElement)VisualTreeHelper.GetChild(item, 0);
            Log.Comment("Item template root: {0}", itemTemplate.Name);

            var groups = VisualStateManager.GetVisualStateGroups(itemTemplate);
            Verify.IsNotNull(groups, "Cannot find visual state groups!!!");
            Verify.IsGreaterThan(groups.Count, 0, "Groups count was not greater than 0!!!");

            VisualStateGroup commonStates = null;
            foreach (VisualStateGroup g in groups)
            {
                Log.Comment("Group Name: {0}", g.Name);
                if (g.Name.Equals("CommonStates", StringComparison.OrdinalIgnoreCase))
                {
                    Log.Comment("Found CommonStates.");
                    commonStates = g;
                    break;
                }
            }

            Verify.IsNotNull(commonStates, "Cannot find CommonStates group!!!");

            return commonStates;
        }

        private int GetFocusedItemIndex(Microsoft.UI.Xaml.Controls.ListViewBase listViewBase)
        {
            int focusedIndex = -1;
            UIExecutor.Execute(() =>
            {
                var focused = (DependencyObject)FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.IsNotNull(focused);
                focusedIndex = listViewBase.IndexFromContainer(focused);
            });

            return focusedIndex;
        }

        private static ObservableCollection<ObservableCollection<int>> GetGroupedData(int numGroups, int itemsPerGroup)
        {
            var groups = new ObservableCollection<ObservableCollection<int>>();
            for (int i = 0; i < numGroups; i++)
            {
                var group = new ObservableCollection<int>();
                for (int j = 0; j < itemsPerGroup; j++)
                {
                    group.Add(i * 1000 + j);
                }

                groups.Add(group);
            }

            return groups;
        }

        class TemplateComponentLogger
        {
            public List<Tuple<int, int>> ProcessBindingCalls { get; set; } = new List<Tuple<int, int>>();
            public List<int> RecycleCalls { get; set; } = new List<int>();
        }

        partial class ElementTemplateComponent : IDataTemplateComponent
        {
            private int _numPhases;
            private int _data;
            private TemplateComponentLogger _logger;

            public ElementTemplateComponent(int numPhases, TemplateComponentLogger logger)
            {
                _numPhases = numPhases;
                _logger = logger;
            }

            public void ProcessBindings(object item, int itemIndex, int phase, out int nextPhase)
            {
                _data = (int)item;
                _logger.ProcessBindingCalls.Add(new Tuple<int, int>(_data, phase));
                nextPhase = phase >= _numPhases - 1 ? -1 : phase + 1;
                Log.Comment(string.Format("Index:{0}  Phase:{1}", item.ToString(), phase));
            }

            public void Recycle()
            {
                _logger.RecycleCalls.Add(_data);
                Log.Comment(string.Format("Recycle Index:{0}", _data));
            }

        }
    }
}
