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
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Shapes;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls.ContextFlyout
{
    [TestClass]
    public class ContextFlyoutIntegrationTests : XamlTestsBase
    {
        private const uint HOLD_DURATION = 2000;
        private const uint SHORT_HOLD_DURATION = 600;
        private const double DRAG_VELOCITY_FACTOR = 0.1;

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
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
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCorrectFlyoutOpensOnParentAndChild0()
        {
            // No flyouts
            RunBasicParentChildTest(
                false, /* childHasFlyout */
                false, /* parentHasFlyout */
                false, /* childCanDrag */
                false /* parentCanDrag */);
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCorrectFlyoutOpensOnParentAndChild1()
        {
            // Both child & parent have flyout, NO drag
            RunBasicParentChildTest(
                true, /* childHasFlyout */
                true, /* parentHasFlyout */
                false, /* childCanDrag */
                false /* parentCanDrag */);
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCorrectFlyoutOpensOnParentAndChild2()
        {
            // Both child & parent have flyout, WITH drag
            RunBasicParentChildTest(
                true, /* childHasFlyout */
                true, /* parentHasFlyout */
                true, /* childCanDrag */
                true /* parentCanDrag */);
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCorrectFlyoutOpensOnParentAndChild3()
        {
            // Only child has flyout, NO drag
            RunBasicParentChildTest(
                true, /* childHasFlyout */
                false, /* parentHasFlyout */
                false, /* childCanDrag */
                false /* parentCanDrag */);
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCorrectFlyoutOpensOnParentAndChild4()
        {
            // Only child has flyout, WITH drag
            RunBasicParentChildTest(
                true, /* childHasFlyout */
                false, /* parentHasFlyout */
                false, /* childCanDrag */
                true /* parentCanDrag */);
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCorrectFlyoutOpensOnParentAndChild5()
        {
            // Only parent has flyout, NO drag
            RunBasicParentChildTest(
                false, /* childHasFlyout */
                true, /* parentHasFlyout */
                false, /* childCanDrag */
                false /* parentCanDrag */);
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCorrectFlyoutOpensOnParentAndChild6()
        {
            // Only parent has flyout, WITH drag
            RunBasicParentChildTest(
                false, /* childHasFlyout */
                true, /* parentHasFlyout */
                true, /* childCanDrag */
                false /* parentCanDrag */);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // Disabled on OneCore pending fix
        [TestProperty("Ignore", "True")]                    // TODO: Disable until the complete Drag and Drop story is implemented
        public void VerifyFlyoutClosesOnDragOrPan()
        {
            using (TestServices.InputHelper.SuppressGestureStateReset())
            {
                Grid root = null;

                Border dragParent = null;
                Flyout dragParentFlyout = null;
                bool dragParentOpened = false;
                bool dragParentClosed = false;

                Rectangle dragChild = null;
                MenuFlyout dragChildFlyout = null;
                bool dragChildOpened = false;
                bool dragChildClosed = false;

                Border panParent = null;
                Flyout panParentFlyout = null;
                bool panParentOpened = false;
                bool panParentClosed = false;

                Rectangle panChild = null;
                MenuFlyout panChildFlyout = null;
                bool panChildOpened = false;
                bool panChildClosed = false;

                UIExecutor.Execute(() =>
                {
                    root = SetupDraggingAndPanningTestXaml();

                    dragParent = (Border)root.FindName("dragParent");
                    dragParentFlyout = (Flyout)root.FindName("dragParentFlyout");
                    dragChild = (Rectangle)root.FindName("dragChild");
                    dragChildFlyout = (MenuFlyout)root.FindName("dragChildFlyout");
                    panParent = (Border)root.FindName("panParent");
                    panParentFlyout = (Flyout)root.FindName("panParentFlyout");
                    panChild = (Rectangle)root.FindName("panChild");
                    panChildFlyout = (MenuFlyout)root.FindName("panChildFlyout");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    dragParentFlyout.Opened += (s, e) =>
                    {
                        Verify.AreEqual(dragParent, dragParentFlyout.Target, "Flyout Target does not match the Drag parent!");
                        dragParentOpened = true;
                    };

                    dragParentFlyout.Closed += (s, e) =>
                    {
                        dragParentClosed = true;
                    };

                    dragChildFlyout.Opened += (s, e) =>
                    {
                        Verify.AreEqual(dragChild, dragChildFlyout.Target, "Flyout Target does not match the Drag child!");
                        dragChildOpened = true;
                    };

                    dragChildFlyout.Closed += (s, e) =>
                    {
                        dragChildClosed = true;
                    };

                    panParentFlyout.Opened += (s, e) =>
                    {
                        Verify.AreEqual(panParent, panParentFlyout.Target, "Flyout Target does not match the Pan parent!");
                        panParentOpened = true;
                    };

                    panParentFlyout.Closed += (s, e) =>
                    {
                        panParentClosed = true;
                    };

                    panChildFlyout.Opened += (s, e) =>
                    {
                        Verify.AreEqual(panChild, panChildFlyout.Target, "Flyout Target does not match the Pan child!");
                        panChildOpened = true;
                    };

                    panChildFlyout.Closed += (s, e) =>
                    {
                        panChildClosed = true;
                    };
                });
                TestServices.WindowHelper.WaitForIdle();

                // Scenario 1 (Drag Parent): Hold -> Open flyout -> Drag -> Close flyout
                TestServices.InputHelper.PressHoldAndPanFromCenter(dragParent, 100, 100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsTrue(dragParentOpened, "Drag parent flyout did not open!");
                Verify.IsTrue(dragParentClosed, "Drag parent flyout did not close!");

                // Scenario 2 (Drag Child): Hold -> Open flyout -> Drag -> Close flyout
                TestServices.InputHelper.PressHoldAndPanFromCenter(dragChild, -100, 100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsTrue(dragChildOpened, "Drag child flyout did not open!");
                Verify.IsTrue(dragChildClosed, "Drag child flyout did not close!");

                // Scenario 3 (Pan Parent): Hold -> Open flyout -> Pan -> Close flyout
                TestServices.InputHelper.PressHoldAndPanFromCenter(panParent, 0, -100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsTrue(panParentOpened, "Pan parent flyout did not open!");
                Verify.IsTrue(panParentClosed, "Pan parent flyout did not close!");
                TestServices.WindowHelper.WaitForIdle();

                // Scenario 4 (Pan Child): Hold -> Open flyout -> Pan -> Close flyout
                TestServices.InputHelper.PressHoldAndPanFromCenter(panChild, 0, 100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsTrue(panChildOpened, "Pan child flyout did not open!");
                Verify.IsTrue(panChildClosed, "Pan child flyout did not close!");
            }
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // Disabled on OneCore pending fix
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyFlyoutOnListViewWithDrag()
        {
            RunListViewTest(
                true, /* canDrag */
                false /* canReorder */);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Phone")] // Disabled on OneCore pending fix
        public void VerifyFlyoutOnListViewWithReorder()
        {
            RunListViewTest(
                false, /* canDrag */
                true /* canReorder */);
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void VerifyCanOpenFlyoutDefinedInItemContainerStyle()
        {
            Grid root = null;
            ListViewItem item1 = null;
            AutoResetEvent flyoutOpenedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                root = SetupTestListViewWithItemContainerStyle();
                item1 = (ListViewItem)root.FindName("item1");

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                item1.ContextFlyout.Opened += (s, e) =>
                {
                    flyoutOpenedEvent.Set();
                };
            });

            TestServices.InputHelper.ClickMouseButton(MouseButton.Right, item1);
            flyoutOpenedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                item1.ContextFlyout.Hide();
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void VerifyContextRequestedFiresOnPointerUp()
        {
            Grid root = null;
            Border border = null;
            bool contextRequestedFired = false;

            UIExecutor.Execute(() =>
            {
                root = (Grid)XamlReader.Load(@"
                    <Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Border x:Name='border' Width='400' Height='400' HorizontalAlignment='Center' VerticalAlignment='Center' BorderThickness='2' Background='YellowGreen'/>
                    </Grid>
                    ");
                border = (Border)root.FindName("border");

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                border.ContextRequested += (s, e) =>
                {
                    contextRequestedFired = true;
                };

                border.RightTapped += (s, e) =>
                {
                    e.Handled = true;
                };
            });
            TestServices.WindowHelper.WaitForIdle();

            TestServices.InputHelper.MouseButtonDown(border, 0, 0, MouseButton.Right);
            TestServices.WindowHelper.WaitForIdle();
            Verify.IsFalse(contextRequestedFired, "ContextRequested got fired on right mouse down!");
            TestServices.InputHelper.MouseButtonUp(border, 0, 0, MouseButton.Right);
            TestServices.WindowHelper.WaitForIdle();
            Verify.IsTrue(contextRequestedFired, "ContextRequested did not fire on right mouse up!");
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyContextRequestedCanBeFiredWithPen()
        {
            Grid root = null;
            Border border = null;
            AutoResetEvent contextRequestedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                root = (Grid)XamlReader.Load(@"
                    <Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Border x:Name='border' Width='400' Height='400' HorizontalAlignment='Center' VerticalAlignment='Center' BorderThickness='2' Background='YellowGreen'/>
                    </Grid>
                    ");
                border = (Border)root.FindName("border");

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                border.ContextRequested += (s, e) =>
                {
                    contextRequestedEvent.Set();
                };

                border.RightTapped += (s, e) =>
                {
                    e.Handled = true;
                };
            });
            TestServices.WindowHelper.WaitForIdle();

            TestServices.InputHelper.PenBarrelTap(border);

            contextRequestedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyContextRequestedEventCanBeSeenWhenHandled()
        {
            Grid root = null;
            Border border = null;

            var contextRequestedHandler = new Action<object, ContextRequestedEventArgs>((source, args) =>
            {
                Log.Comment("Context Requested fired on Border: Setting Handled to true.");
                args.Handled = true;
            });

            UIExecutor.Execute(() =>
            {
                root = (Grid)XamlReader.Load(@"
                    <Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Border x:Name='border' Width='400' Height='400' HorizontalAlignment='Center' VerticalAlignment='Center' BorderThickness='2' Background='YellowGreen'/>
                    </Grid>
                    ");
                border = (Border)root.FindName("border");

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            using (var borderContextRequested = new EventTester<Border, ContextRequestedEventArgs>(border, "ContextRequested", contextRequestedHandler))
            using (var rootContextRequested = EventTester<Grid, ContextRequestedEventArgs>.FromRoutedEvent(root, "ContextRequested", (t, u) => { } /*No action required*/))
            {
                TestServices.InputHelper.ClickMouseButton(MouseButton.Right, border);
                TestServices.WindowHelper.WaitForIdle();

                borderContextRequested.Wait();
                rootContextRequested.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(rootContextRequested.HasFired);
                });
            }
        }

        [TestMethod]
        public void VerifyContextMenuCanBeOpenedThroughUIA()
        {
            Button button = null;
            MenuFlyout flyout = null;
            bool flyoutOpened = false;

            UIExecutor.Execute(() =>
            {
                button = new Button();
                flyout = new MenuFlyout();
                flyout.Items.Add(new MenuFlyoutItem { Text = "Item #1" });
                flyout.Opened += delegate { flyoutOpened = true; };

                button.ContextFlyout = flyout;
                TestServices.WindowHelper.WindowContent = button;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                FrameworkElementAutomationPeer.FromElement(button).ShowContextMenu();
            });
            TestServices.WindowHelper.WaitForIdle();
            Verify.IsTrue(flyoutOpened);

            UIExecutor.Execute(() =>
            {
                flyout.Hide();
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates position of ContextFlyout when right-clicking a Button at various scale factors.")]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("TestPass:MaxOSVer", WindowsOSVersion._22H2)]    // This test is currently failing on 23h2.
        public void VerifyContextMenuPositionWithMouse()
        {
            VerifyContextMenuPosition(scale: 1.0f, withMouse: true, canDrag: false);
            VerifyContextMenuPosition(scale: 1.5f, withMouse: true, canDrag: false);
            VerifyContextMenuPosition(scale: 2.0f, withMouse: true, canDrag: false);
        }

        [TestMethod]
        [TestProperty("Description", "Validates position of ContextFlyout when holding a Button at various scale factors.")]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("TestPass:MaxOSVer", WindowsOSVersion._22H2)]    // This test is currently failing on 23h2.
        public void VerifyContextMenuPositionWithTouch()
        {
            VerifyContextMenuPosition(scale: 1.0f, withMouse: false, canDrag: false);
            VerifyContextMenuPosition(scale: 1.5f, withMouse: false, canDrag: false);
            VerifyContextMenuPosition(scale: 2.0f, withMouse: false, canDrag: false);
        }

        [TestMethod]
        [TestProperty("Description", "Validates position of ContextFlyout when holding a ListViewItem at various scale factors.")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void VerifyContextMenuPositionWithTouchOnDraggableElement()
        {
            VerifyContextMenuPosition(scale: 1.0f, withMouse: false, canDrag: true);
            VerifyContextMenuPosition(scale: 1.5f, withMouse: false, canDrag: true);
            VerifyContextMenuPosition(scale: 2.0f, withMouse: false, canDrag: true);
        }

        private void VerifyContextMenuPosition(float scale, bool withMouse, bool canDrag)
        {
            Log.Comment($">> ----------------------------------------------------------------");
            Log.Comment($">> VerifyContextMenuPosition scale={scale}, withMouse={withMouse}, canDrag={canDrag}");

            int padding = 30;
            int contextMenuClickHorizontalOffset = 50;
            int contextMenuClickVerticalOffset = withMouse ? 30 : -20;  // Touch will open the context menu to the top, and click will open the menu to the bottom
            Point gesturePosition = new Point();
            Control control = null;
            MenuFlyout menuFlyout = null;
            bool menuFlyoutOpen = false;
            AutoResetEvent menuFlyoutOpenedEvent = new AutoResetEvent(false);
            AutoResetEvent menuFlyoutClosedEvent = new AutoResetEvent(false);
            AutoResetEvent menuFlyoutItemClickEvent = new AutoResetEvent(false);

            TestServices.WindowHelper.SetWindowSizeOverrideWithScale(new Size(300, 300), scale);

            UIExecutor.Execute(() =>
            {
                Control rootElement = null;

                if (canDrag)
                {
                    control = new ListViewItem()
                    {
                        Content = "ListViewItem",
                    };

                    ListView listView = new ListView()
                    {
                        CanDragItems = true,
                        Padding = new Thickness(padding),
                        VerticalAlignment = VerticalAlignment.Top
                    };

                    listView.Items.Add(control);
                    rootElement = listView;
                }
                else
                {
                    control = new Button()
                    {
                        Content = "Button",
                        Margin = new Thickness(padding),
                        Padding = new Thickness(padding),
                        VerticalAlignment = VerticalAlignment.Top
                    };

                    rootElement = control;
                }

                menuFlyout = new MenuFlyout();

                var menuFlyoutItem = new MenuFlyoutItem
                {
                    Text = "Item #1"
                };
                menuFlyoutItem.Click += delegate
                {
                    Log.Comment("MenuFlyoutItem.Click event.");
                    menuFlyoutItemClickEvent.Set();
                };

                menuFlyout.Items.Add(menuFlyoutItem);

                menuFlyout.Opened += delegate
                {
                    Log.Comment("MenuFlyout.Opened event.");
                    menuFlyoutOpen = true;
                    menuFlyoutOpenedEvent.Set();
                };

                menuFlyout.Closed += delegate
                {
                    Log.Comment("MenuFlyout.Closed event.");
                    menuFlyoutOpen = false;
                    menuFlyoutClosedEvent.Set();
                };

                control.ContextFlyout = menuFlyout;
                TestServices.WindowHelper.WindowContent = rootElement;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment($"Control actual size: {control.ActualWidth}, {control.ActualHeight}");
                gesturePosition.X = (control.ActualWidth / 2.0 + padding) / scale;
                gesturePosition.Y = (control.ActualHeight / 2.0 + padding) / scale;
            });

            if (withMouse)
            {
                Log.Comment("Right-clicking control...");
                TestServices.InputHelper.ClickMouseButton(MouseButton.Right, gesturePosition);
            }
            else
            {
                Log.Comment("Holding control...");
                TestServices.InputHelper.Hold(gesturePosition);
            }
            TestServices.WindowHelper.WaitForIdle();

            Verify.IsTrue(menuFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for menuFlyoutOpenedEvent");
            Log.Comment("MenuFlyout opened.");
            Verify.IsTrue(menuFlyoutOpen);

            Log.Comment("Moving mouse over MenuFlyoutItem...");
            TestServices.InputHelper.MoveMouse(new Point(
                    gesturePosition.X + contextMenuClickHorizontalOffset / scale + 1.0,
                    gesturePosition.Y + contextMenuClickVerticalOffset / scale + 1.0));
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Left-clicking MenuFlyoutItem...");
            TestServices.InputHelper.LeftMouseClick(new Point(
                    gesturePosition.X + contextMenuClickHorizontalOffset / scale,
                    gesturePosition.Y + contextMenuClickVerticalOffset / scale));
            TestServices.WindowHelper.WaitForIdle();

            Verify.IsTrue(menuFlyoutItemClickEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for menuFlyoutItemClickEvent");
            Verify.IsTrue(menuFlyoutClosedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for menuFlyoutClosedEvent");
            Log.Comment("MenuFlyoutItem clicked and MenuFlyout closed.");
            Verify.IsFalse(menuFlyoutOpen);
        }

        #region Helpers

        private void RunListViewTest(bool canDrag, bool canReorder)
        {
            using (TestServices.InputHelper.SuppressGestureStateReset())
            {
                Grid root = null;

                ListView list = null;
                Flyout listFlyout = null;
                bool listFlyoutOpened = false;
                bool listFlyoutClosed = false;
                ListViewItem item1 = null;
                MenuFlyout itemFlyout = null;
                bool itemFlyoutOpened = false;
                bool itemFlyoutClosed = false;
                ListViewItem item2 = null;

                UIExecutor.Execute(() =>
                {
                    root = SetupTestListView(canDrag, canReorder);

                    list = (ListView)root.FindName("list");
                    listFlyout = (Flyout)root.FindName("listFlyout");
                    item1 = (ListViewItem)root.FindName("item1");
                    itemFlyout = (MenuFlyout)root.FindName("itemFlyout");
                    item2 = (ListViewItem)root.FindName("item2");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    listFlyout.Opened += (s, e) =>
                    {
                        Verify.AreEqual(item2, listFlyout.Target, "List flyout Target was not Item2!");
                        listFlyoutOpened = true;
                    };

                    listFlyout.Closed += (s, e) =>
                    {
                        listFlyoutClosed = true;
                    };

                    itemFlyout.Opened += (s, e) =>
                    {
                        Verify.AreEqual(item1, itemFlyout.Target, "Item flyout Target was not Item1!");
                        itemFlyoutOpened = true;
                    };

                    itemFlyout.Closed += (s, e) =>
                    {
                        itemFlyoutClosed = true;
                    };
                });
                TestServices.WindowHelper.WaitForIdle();

                // Scenario 1: Item with ContextFlyout displays its own flyout
                if (canDrag)
                {
                    TestServices.InputHelper.PressHoldAndPanFromCenter(item1, 100, 100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                }
                else
                {
                    TestServices.InputHelper.PressHoldAndPanFromCenter(item1, 0, 100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                }
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsTrue(itemFlyoutOpened, "Item flyout did not open!");
                Verify.IsTrue(itemFlyoutClosed, "Item flyout did not close!");

                // Scenario 2: Item without ContextFlyout displays the parent LV's flyout
                if (canDrag)
                {
                    TestServices.InputHelper.PressHoldAndPanFromCenter(item2, 100, 100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                }
                else
                {
                    TestServices.InputHelper.PressHoldAndPanFromCenter(item2, 0, 100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                }
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsTrue(listFlyoutOpened, "List flyout did not open!");
                Verify.IsTrue(listFlyoutClosed, "List flyout did not close!");

                // Scenario 3: Dragging into the flyout layer while press-hold-drag also closes flyout
                itemFlyoutOpened = false;
                itemFlyoutClosed = false;
                TestServices.InputHelper.PressHoldAndPanFromCenter(item1, 100, -100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsTrue(itemFlyoutOpened, "Item flyout did not open!");
                Verify.IsTrue(itemFlyoutClosed, "Item flyout did not close!");

                // Scenario 4: Press-hold-drag by holding long enough for holding visual, but not enough for ContextMenu will not open flyout
                itemFlyoutOpened = false;
                itemFlyoutClosed = false;
                TestServices.InputHelper.PressHoldAndPanFromCenter(item1, 0, 100, 0.5, SHORT_HOLD_DURATION); // Drag velocity is faster (0.5) in order to beat the context menu timer clock
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsFalse(itemFlyoutOpened, "Item flyout opened anyway!");
                Verify.IsFalse(itemFlyoutClosed, "Item flyout closed anyway!");

                // Scenario 5: Press-hold and lifting up finger by holding long enough for holding visual, but not enough for ContextMenu will not open flyout
                itemFlyoutOpened = false;
                itemFlyoutClosed = false;
                TestServices.InputHelper.Hold(item1, SHORT_HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsFalse(itemFlyoutOpened, "Item flyout opened anyway!");
                Verify.IsFalse(itemFlyoutClosed, "Item flyout closed anyway!");
            }
        }

        private void RunBasicParentChildTest(bool childHasFlyout, bool parentHasFlyout, bool childCanDrag, bool parentCanDrag)
        {
            using (TestCleanupWrapper testCleanup = new TestCleanupWrapper())
            using (TestServices.InputHelper.SuppressGestureStateReset())
            {
                DateTime start = DateTime.Now;
                DateTime end = DateTime.Now;

                Grid root = null;

                Border parent = null;
                Flyout parentFlyout = null;
                bool parentFlyoutOpen = false;

                Rectangle childUIE = null;
                MenuFlyout childUIEFlyout = null;
                bool childUIEFlyoutOpen = false;

                ToggleSwitch childControl = null;
                FrameworkElement switchKnob = null;
                MenuFlyout childControlFlyout = null;
                bool childControlFlyoutOpen = false;

                AutoResetEvent parentFlyoutOpenedEvent = new AutoResetEvent(false);
                AutoResetEvent childUIEFlyoutOpenedEvent = new AutoResetEvent(false);
                AutoResetEvent childControlFlyoutOpenedEvent = new AutoResetEvent(false);

                UIExecutor.Execute(() =>
                {
                    root = SetupXamlForBasicParentChildTest(childHasFlyout, parentHasFlyout, childCanDrag, parentCanDrag);

                    parent = (Border)root.FindName("border");
                    if (parentHasFlyout)
                    {
                        parentFlyout = (Flyout)root.FindName("borderFlyout");
                    }

                    childUIE = (Rectangle)root.FindName("rectangle");
                    childControl = (ToggleSwitch)root.FindName("switch");
                    if (childHasFlyout)
                    {
                        childUIEFlyout = (MenuFlyout)root.FindName("rectangleFlyout");
                        childControlFlyout = (MenuFlyout)root.FindName("switchFlyout");
                    }

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    switchKnob = VisualTreeUtils.FindNameInSubtree(childControl, "SwitchKnob");
                    Verify.IsNotNull(switchKnob);

                    Log.Comment("Setting up flyout handlers for verification.");

                    if (parentHasFlyout)
                    {
                        parentFlyout.Opened += (s, e) =>
                        {
                            Log.Comment("Flyout.Opened event raised for parentFlyout.");
                            end = DateTime.Now;
                            parentFlyoutOpen = true;
                            parentFlyoutOpenedEvent.Set();
                        };
                        parentFlyout.Closed += (s, e) =>
                        {
                            Log.Comment("Flyout.Closed event raised for parentFlyout.");
                            parentFlyoutOpen = false;
                        };
                    }

                    if (childHasFlyout)
                    {
                        childUIEFlyout.Opened += (s, e) =>
                        {
                            Log.Comment("MenuFlyout.Opened event raised for childUIEFlyout.");
                            end = DateTime.Now;
                            childUIEFlyoutOpen = true;
                            childUIEFlyoutOpenedEvent.Set();
                        };
                        childUIEFlyout.Closed += (s, e) =>
                        {
                            Log.Comment("MenuFlyout.Closed event raised for childUIEFlyout.");
                            childUIEFlyoutOpen = false;
                        };

                        childControlFlyout.Opened += (s, e) =>
                        {
                            Log.Comment("MenuFlyout.Opened event raised for childControlFlyout.");
                            end = DateTime.Now;
                            childControlFlyoutOpen = true;
                            childControlFlyoutOpenedEvent.Set();
                        };
                        childControlFlyout.Closed += (s, e) =>
                        {
                            Log.Comment("MenuFlyout.Closed event raised for childControlFlyout.");
                            childControlFlyoutOpen = false;
                        };
                    }

                    Log.Comment("Handlers have been set up for flyout verification, now listening for flyout Opened/Closed.");
                });
                TestServices.WindowHelper.WaitForIdle();

                VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                Log.Comment("Scenario 1a: Mouse right-click on parent");
                parentFlyoutOpenedEvent.Reset();
                childUIEFlyoutOpenedEvent.Reset();
                childControlFlyoutOpenedEvent.Reset();
                TestServices.InputHelper.ClickMouseButton(MouseButton.Right, parent);
                TestServices.WindowHelper.WaitForIdle();

                if (parentHasFlyout)
                {
                    Verify.IsTrue(parentFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for parentFlyoutOpenedEvent");
                    VerifyParentFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(parentFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else
                {
                    // No flyout
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }

                Log.Comment("Scenario 1b: Mouse right-click on child UIE");
                parentFlyoutOpenedEvent.Reset();
                childUIEFlyoutOpenedEvent.Reset();
                childControlFlyoutOpenedEvent.Reset();
                TestServices.InputHelper.ClickMouseButton(MouseButton.Right, childUIE);
                TestServices.WindowHelper.WaitForIdle();

                if (childHasFlyout)
                {
                    Verify.IsTrue(childUIEFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for childUIEFlyoutOpenedEvent");
                    VerifyChildUIEFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(childUIEFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else if (parentHasFlyout)
                {
                    Verify.IsTrue(parentFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for parentFlyoutOpenedEvent");
                    VerifyParentFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(parentFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else
                {
                    // No flyout
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }

                Log.Comment("Scenario 1c: Mouse right-click on child Control");
                // Right-clicking the center of the SwitchKnob element instead of the center of the ToggleSwitch
                // because it is not hit-test-visible and it would open the parent flyout.
                parentFlyoutOpenedEvent.Reset();
                childUIEFlyoutOpenedEvent.Reset();
                childControlFlyoutOpenedEvent.Reset();
                TestServices.InputHelper.ClickMouseButton(MouseButton.Right, switchKnob);
                TestServices.WindowHelper.WaitForIdle();

                if (childHasFlyout)
                {
                    Verify.IsTrue(childControlFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for childControlFlyoutOpenedEvent");
                    VerifyChildControlFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(childControlFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else if (parentHasFlyout)
                {
                    Verify.IsTrue(parentFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for parentFlyoutOpenedEvent");
                    VerifyParentFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(parentFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else
                {
                    // No flyout
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }

                Log.Comment("Scenario 2: Set keyboard focus on child Control and hit Shift-F10");
                parentFlyoutOpenedEvent.Reset();
                childUIEFlyoutOpenedEvent.Reset();
                childControlFlyoutOpenedEvent.Reset();
                UIExecutor.Execute(() =>
                {
                    childControl.Focus(FocusState.Programmatic);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.PressKeySequence("$d$_shift#$d$_f10#$u$_f10#$u$_shift");
                TestServices.WindowHelper.WaitForIdle();
                if (childHasFlyout)
                {
                    Verify.IsTrue(childControlFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for childControlFlyoutOpenedEvent");
                    VerifyChildControlFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(childControlFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else if (parentHasFlyout)
                {
                    Verify.IsTrue(parentFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for parentFlyoutOpenedEvent");
                    VerifyParentFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(parentFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else
                {
                    // No flyout
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }

                Log.Comment("Scenario 3: Keyboard Menu key on parent, child UIE, and child Control");
                parentFlyoutOpenedEvent.Reset();
                childUIEFlyoutOpenedEvent.Reset();
                childControlFlyoutOpenedEvent.Reset();
                UIExecutor.Execute(() =>
                {
                    childControl.Focus(FocusState.Programmatic);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.PressKeySequence("$d$_apps#$u$_apps");
                TestServices.WindowHelper.WaitForIdle();
                if (childHasFlyout)
                {
                    Verify.IsTrue(childControlFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for childControlFlyoutOpenedEvent");
                    VerifyChildControlFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(childControlFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else if (parentHasFlyout)
                {
                    Verify.IsTrue(parentFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for parentFlyoutOpenedEvent");
                    VerifyParentFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(parentFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else
                {
                    // No flyout
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }

                Log.Comment("Scenario 4: XBOX Gamepad Menu key on child Control");
                parentFlyoutOpenedEvent.Reset();
                childUIEFlyoutOpenedEvent.Reset();
                childControlFlyoutOpenedEvent.Reset();
                UIExecutor.Execute(() =>
                {
                    childControl.Focus(FocusState.Programmatic);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.KeyboardHelper.GamePadMenu();
                TestServices.WindowHelper.WaitForIdle();
                if (childHasFlyout)
                {
                    Verify.IsTrue(childControlFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for childControlFlyoutOpenedEvent");
                    VerifyChildControlFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(childControlFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else if (parentHasFlyout)
                {
                    Verify.IsTrue(parentFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for parentFlyoutOpenedEvent");
                    VerifyParentFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(parentFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else
                {
                    // No flyout
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }

                Log.Comment("Scenario 5: Touch Press & Hold on parent, and child");
                parentFlyoutOpenedEvent.Reset();
                childUIEFlyoutOpenedEvent.Reset();
                childControlFlyoutOpenedEvent.Reset();
                start = DateTime.Now;
                TestServices.InputHelper.Hold(parent, HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();

                if (parentHasFlyout)
                {
                    var elapsed = end - start;
                    double tolerance = 100.0;
                    if (parentCanDrag)
                    {
                        Verify.IsGreaterThanOrEqual(elapsed.TotalMilliseconds, 1000.0 - tolerance, "Draggable item took less than 1000ms!");
                    }
                    else
                    {
                        Verify.IsGreaterThanOrEqual(elapsed.TotalMilliseconds, 500.0 - tolerance, "Non-Draggable item took less than 500ms!");
                    }

                    Verify.IsTrue(parentFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for parentFlyoutOpenedEvent");
                    VerifyParentFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(parentFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else
                {
                    // No flyout
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }

                start = DateTime.Now;
                TestServices.InputHelper.Hold(childUIE, HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();

                if (childHasFlyout)
                {
                    var elapsed = end - start;
                    if (childCanDrag || parentCanDrag)
                    {
                        Verify.IsGreaterThanOrEqual(elapsed.TotalMilliseconds, 1000.0, "Draggable item took less than 1000ms!");
                    }
                    else
                    {
                        Verify.IsGreaterThanOrEqual(elapsed.TotalMilliseconds, 500.0, "Non-Draggable item took less than 500ms!");
                    }

                    Verify.IsTrue(childUIEFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for childUIEFlyoutOpenedEvent");
                    VerifyChildUIEFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(childUIEFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else if (parentHasFlyout)
                {
                    var elapsed = end - start;
                    if (childCanDrag || parentCanDrag)
                    {
                        Verify.IsGreaterThanOrEqual(elapsed.TotalMilliseconds, 1000.0, "Draggable item took less than 1000ms!");
                    }
                    else
                    {
                        Verify.IsGreaterThanOrEqual(elapsed.TotalMilliseconds, 500.0, "Non-Draggable item took less than 500ms!");
                    }

                    Verify.IsTrue(parentFlyoutOpenedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for parentFlyoutOpenedEvent");
                    VerifyParentFlyoutOpen(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);

                    HideFlyout(parentFlyout);
                    TestServices.WindowHelper.WaitForIdle();
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
                else
                {
                    // No flyout
                    VerifyInitialState(parentFlyoutOpen, childUIEFlyoutOpen, childControlFlyoutOpen);
                }
            }
        }

        private Grid SetupXamlForBasicParentChildTest(bool childHasFlyout, bool parentHasFlyout, bool childCanDrag, bool parentCanDrag)
        {
            string xamlFormat = @"
                <Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>
                    <Border x:Name='border' Width='400' Height='400' HorizontalAlignment='Center' VerticalAlignment='Center' BorderThickness='2' Background='YellowGreen' {3}>
                        <StackPanel Orientation='Horizontal'>
                            <Rectangle x:Name='rectangle' Width='50' Height='50' Fill='Red' Margin='10' {4}>
                                {1}
                            </Rectangle>

                            <ToggleSwitch x:Name='switch' {4}>
                                {2}
                            </ToggleSwitch>
                        </StackPanel>
                        {0}
                    </Border>
                </Grid>
                ";

            string parentFlyoutXaml0 = @"
                <Border.ContextFlyout>
                    <Flyout x:Name='borderFlyout'>
                        <StackPanel>
                            <TextBlock FontSize='15' Text='foo'/>
                            <TextBlock FontSize='15' Text='bar'/>
                        </StackPanel>
                    </Flyout>
                </Border.ContextFlyout>
                ";

            string childUIEFlyoutXaml1 = @"
                <Rectangle.ContextFlyout>
                    <MenuFlyout x:Name='rectangleFlyout'>
                        <MenuFlyoutItem>FOO</MenuFlyoutItem>
                        <MenuFlyoutItem>BAR</MenuFlyoutItem>
                    </MenuFlyout>
                </Rectangle.ContextFlyout>
                ";

            string childControlFlyoutXaml2 = @"
                <ToggleSwitch.ContextFlyout>
                    <MenuFlyout x:Name='switchFlyout'>
                        <MenuFlyoutItem>123</MenuFlyoutItem>
                        <MenuFlyoutItem>789</MenuFlyoutItem>
                    </MenuFlyout>
                </ToggleSwitch.ContextFlyout>
                ";

            string canDragXaml = "CanDrag='true'";

            string xaml = string.Format(
                xamlFormat,
                parentHasFlyout ? parentFlyoutXaml0 : string.Empty,
                childHasFlyout ? childUIEFlyoutXaml1 : string.Empty,
                childHasFlyout ? childControlFlyoutXaml2 : string.Empty,
                parentCanDrag ? canDragXaml : string.Empty,
                childCanDrag ? canDragXaml : string.Empty);

            Log.Comment("Test XAML:");
            Log.Comment(xaml);

            return (Grid)XamlReader.Load(xaml);
        }

        private void VerifyInitialState(bool parentFlyoutOpen, bool childUIEFlyoutOpen, bool childControlFlyoutOpen)
        {
            Verify.IsFalse(parentFlyoutOpen, "Parent flyout is not in closed state!");
            Verify.IsFalse(childUIEFlyoutOpen, "Child UIElement flyout is not in closed state!");
            Verify.IsFalse(childControlFlyoutOpen, "Child control flyout is not in closed state!");
        }

        private void VerifyParentFlyoutOpen(bool parentFlyoutOpen, bool childUIEFlyoutOpen, bool childControlFlyoutOpen)
        {
            Verify.IsTrue(parentFlyoutOpen, "Parent flyout is not opened!");
            Verify.IsFalse(childUIEFlyoutOpen, "Child UIElement flyout is not in closed state!");
            Verify.IsFalse(childControlFlyoutOpen, "Child control flyout is not in closed state!");
        }

        private void VerifyChildUIEFlyoutOpen(bool parentFlyoutOpen, bool childUIEFlyoutOpen, bool childControlFlyoutOpen)
        {
            Verify.IsFalse(parentFlyoutOpen, "Parent flyout is not in closed state!");
            Verify.IsTrue(childUIEFlyoutOpen, "Child UIElement flyout is not opened!");
            Verify.IsFalse(childControlFlyoutOpen, "Child control flyout is not in closed state!");
        }

        private void VerifyChildControlFlyoutOpen(bool parentFlyoutOpen, bool childUIEFlyoutOpen, bool childControlFlyoutOpen)
        {
            Verify.IsFalse(parentFlyoutOpen, "Parent flyout is not in closed state!");
            Verify.IsFalse(childUIEFlyoutOpen, "Child UIElement flyout is not in closed state!");
            Verify.IsTrue(childControlFlyoutOpen, "Child control flyout is not opened!");
        }

        private Grid SetupDraggingAndPanningTestXaml()
        {
            string xamlFormat = @"
                <Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>
                    <ScrollViewer>
                        <StackPanel>
                            <Border x:Name='dragParent' Width='200' Height='200' BorderThickness='2' Background='YellowGreen' CanDrag='true'>
                                <StackPanel Orientation='Horizontal'>
                                    <Rectangle x:Name='dragChild' Width='50' Height='50' Fill='Red' Margin='10'>
                                        <Rectangle.ContextFlyout>
                                            <MenuFlyout x:Name='dragChildFlyout'>
                                                <MenuFlyoutItem>FOO</MenuFlyoutItem>
                                                <MenuFlyoutItem>BAR</MenuFlyoutItem>
                                            </MenuFlyout>
                                        </Rectangle.ContextFlyout>
                                    </Rectangle>
                                    <Rectangle Width='50' Height='10' Fill='Yellow'/>
                                </StackPanel>

                                <Border.ContextFlyout>
                                    <Flyout x:Name='dragParentFlyout'>
                                        <StackPanel>
                                            <TextBlock Text='foo'/>
                                            <TextBlock Text='bar'/>
                                        </StackPanel>
                                    </Flyout>
                                </Border.ContextFlyout>
                            </Border>

                            <Border x:Name='panParent' Width='200' Height='200' BorderThickness='2' Background='YellowGreen'>
                                <StackPanel Orientation='Horizontal'>
                                    <Rectangle x:Name='panChild' Width='50' Height='50' Fill='Red' Margin='10'>
                                        <Rectangle.ContextFlyout>
                                            <MenuFlyout x:Name='panChildFlyout'>
                                                <MenuFlyoutItem>FOO</MenuFlyoutItem>
                                                <MenuFlyoutItem>BAR</MenuFlyoutItem>
                                            </MenuFlyout>
                                        </Rectangle.ContextFlyout>
                                    </Rectangle>
                                    <Rectangle Width='50' Height='10' Fill='Yellow'/>
                                </StackPanel>

                                <Border.ContextFlyout>
                                    <Flyout x:Name='panParentFlyout'>
                                        <StackPanel>
                                            <TextBlock Text='foo'/>
                                            <TextBlock Text='bar'/>
                                        </StackPanel>
                                    </Flyout>
                                </Border.ContextFlyout>
                            </Border>

                            {0}
                            {1}
                            {2}
                            {3}
                            {4}
                            {5}
                            {6}
                            {7}
                            {8}
                            {9}
                        </StackPanel>
                    </ScrollViewer>
                </Grid>
                ";

            string repeatedElementsXaml = @"
                <Border Width='200' Height='200' BorderThickness='2' Background='YellowGreen'>
                    <StackPanel Orientation='Horizontal'>
                        <Rectangle Width='50' Height='50' Fill='Red' Margin='10'/>
                        <Rectangle Width='50' Height='10' Fill='Yellow'/>
                    </StackPanel>
                </Border>
                ";

            string xaml = string.Format(
                xamlFormat,
                repeatedElementsXaml,
                repeatedElementsXaml,
                repeatedElementsXaml,
                repeatedElementsXaml,
                repeatedElementsXaml,
                repeatedElementsXaml,
                repeatedElementsXaml,
                repeatedElementsXaml,
                repeatedElementsXaml,
                repeatedElementsXaml);

            Log.Comment("Test XAML:");
            Log.Comment(xaml);

            return (Grid)XamlReader.Load(xaml);
        }

        private Grid SetupTestListView(bool canDrag, bool canReorder)
        {
            string xamlFormat = @"
                <Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>
                    <ListView x:Name='list' Width='200' HorizontalAlignment='Center' VerticalAlignment='Center' ItemContainerStyle='{{ThemeResource ListViewItemExpanded}}' {0} {1}>
                        <ListViewItem x:Name='item1' Background='Gray' Content='Item1'>
                            <ListViewItem.ContextFlyout>
                                <MenuFlyout x:Name='itemFlyout'>
                                    <MenuFlyoutItem>FOO</MenuFlyoutItem>
                                    <MenuFlyoutItem>BAR</MenuFlyoutItem>
                                </MenuFlyout>
                            </ListViewItem.ContextFlyout>
                        </ListViewItem>
                        <ListViewItem x:Name='item2' Background='Aqua'>Item2</ListViewItem>

                        <ListView.ContextFlyout>
                            <Flyout x:Name='listFlyout'>
                                <StackPanel>
                                    <TextBlock Text='foo'/>
                                    <TextBlock Text='bar'/>
                                </StackPanel>
                            </Flyout>
                        </ListView.ContextFlyout>
                    </ListView>
                </Grid>
                ";
            string xaml = string.Format(
                xamlFormat,
                canDrag ? "CanDragItems='true'" : string.Empty,
                canReorder ? "CanReorderItems='true'" : string.Empty);

            Log.Comment("Test ListView XAML:");
            Log.Comment(xaml);

            return (Grid)XamlReader.Load(xaml);
        }

        private Grid SetupTestListViewWithItemContainerStyle()
        {
            string xaml = @"
                <Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>
                    <ListView x:Name='list' Width='200' HorizontalAlignment='Center' VerticalAlignment='Center' CanReorderItems='true'>
                        <ListViewItem x:Name='item1' Background='Gray'>Item1</ListViewItem>
                        <ListViewItem x:Name='item2' Background='Aqua'>Item2</ListViewItem>

                        <ListView.ItemContainerStyle>
                            <Style TargetType='ListViewItem'>
                                <Setter Property='ContextFlyout'>
                                    <Setter.Value>
                                        <MenuFlyout>
                                            <MenuFlyoutItem>FOO</MenuFlyoutItem>
                                            <MenuFlyoutItem>BAR</MenuFlyoutItem>
                                        </MenuFlyout>
                                    </Setter.Value>
                                </Setter>
                            </Style>
                        </ListView.ItemContainerStyle>
                    </ListView>
                </Grid>
                ";

            Log.Comment("Test ListView XAML:");
            Log.Comment(xaml);

            return (Grid)XamlReader.Load(xaml);
        }

        private void HideFlyout(FlyoutBase flyout)
        {
            Log.Comment("Closing flyout");
            AutoResetEvent flyoutClosedEvent = new AutoResetEvent(false);
            EventHandler<object> flyoutClosedHandler = (s, e) =>
            {
                Log.Comment("Flyout Closed event fired");
                flyoutClosedEvent.Set();
            };
            UIExecutor.Execute(() =>
            {
                flyout.Closed += flyoutClosedHandler;
                flyout.Hide();
            });
            Verify.IsTrue(flyoutClosedEvent.WaitOne(TimeSpan.FromSeconds(5)), "Wait for Flyout.Closed event");
            UIExecutor.Execute(() =>
            {
                flyout.Closed -= flyoutClosedHandler;
            });
        }

        #endregion
    }
}
