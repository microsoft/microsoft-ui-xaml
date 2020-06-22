// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using System.Collections.Specialized;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ScrollerAnchorRequestedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerAnchorRequestedEventArgs;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ItemsSourceView = Microsoft.UI.Xaml.Controls.ItemsSourceView;
using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using RecyclePool = Microsoft.UI.Xaml.Controls.RecyclePool;
using UniformGridLayout = Microsoft.UI.Xaml.Controls.UniformGridLayout;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollerTests : ApiTestBase
    {
        private const double c_defaultAnchoringUIScrollerNonConstrainedSize = 600.0;
        private const double c_defaultAnchoringUIScrollerConstrainedSize = 300.0;
        private const int c_defaultAnchoringUIStackPanelChildrenCount = 16;
        private const int c_defaultAnchoringUIRepeaterChildrenCount = 16;

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset remains at 0 when inserting an item at the beginning (HorizontalAnchorRatio=0).")]
        public void AnchoringAtLeftEdge()
        {
            AnchoringAtNearEdge(Orientation.Horizontal);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset remains at 0 when inserting an item at the beginning (VerticalAnchorRatio=0).")]
        public void AnchoringAtTopEdge()
        {
            AnchoringAtNearEdge(Orientation.Vertical);
        }

        private void AnchoringAtNearEdge(Orientation orientation)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: true,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications: false))
            {
                Scroller scroller = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    scroller = new Scroller();

                    SetupDefaultAnchoringUI(orientation, scroller, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Inserting child at near edge");
                    InsertStackPanelChild((scroller.Content as Border).Child as StackPanel, 1 /*operationCount*/, 0 /*newIndex*/, 1 /*newCount*/);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("No Scroller offset change expected");
                    if (orientation == Orientation.Vertical)
                    {
                        Verify.AreEqual(0, scroller.VerticalOffset);
                    }
                    else
                    {
                        Verify.AreEqual(0, scroller.HorizontalOffset);
                    }
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset growns to max value when inserting an item at the end (HorizontalAnchorRatio=1).")]
        public void AnchoringAtRightEdgeWhileIncreasingContentWidth()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Horizontal, 0 /*viewportSizeChange*/, 3876 /*expectedFinalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset grows to max value when inserting an item at the end (VerticalAnchorRatio=1).")]
        public void AnchoringAtBottomEdgeWhileIncreasingContentHeight()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Vertical, 0 /*viewportSizeChange*/, 3876 /*expectedFinalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset growns to max value when inserting an item at the end and growing viewport (HorizontalAnchorRatio=1).")]
        public void AnchoringAtRightEdgeWhileIncreasingContentAndViewportWidth()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Horizontal, 10 /*viewportSizeChange*/, 3866 /*expectedFinalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset grows to max value when inserting an item at the end and growning viewport (VerticalAnchorRatio=1).")]
        public void AnchoringAtBottomEdgeWhileIncreasingContentAndViewportHeight()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Vertical, 10 /*viewportSizeChange*/, 3866 /*expectedFinalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset growns to max value when inserting an item at the end and shrinking viewport (HorizontalAnchorRatio=1).")]
        public void AnchoringAtRightEdgeWhileIncreasingContentAndDecreasingViewportWidth()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Horizontal, -10 /*viewportSizeChange*/, 3886 /*expectedFinalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset grows to max value when inserting an item at the end and shrinking viewport (VerticalAnchorRatio=1).")]
        public void AnchoringAtBottomEdgeWhileIncreasingContentAndDecreasingViewportHeight()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Vertical, -10 /*viewportSizeChange*/, 3886 /*expectedFinalOffset*/);
        }

        private void AnchoringAtFarEdgeWhileIncreasingContent(Orientation orientation, double viewportSizeChange, double expectedFinalOffset)
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1 where InteractionTracker's AdjustPositionXIfGreaterThanThreshold/AdjustPositionYIfGreaterThanThreshold are ineffective in this scenario.");
                return;
            }

            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: true,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications: false))
            {
                Scroller scroller = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    scroller = new Scroller();

                    SetupDefaultAnchoringUI(orientation, scroller, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                ZoomTo(scroller, 2.0f, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

                double horizontalOffset = 0.0;
                double verticalOffset = 0.0;

                RunOnUIThread.Execute(() =>
                {
                    if (orientation == Orientation.Vertical)
                    {
                        verticalOffset = scroller.ExtentHeight * 2.0 - scroller.Height;
                        scroller.VerticalAnchorRatio = 1.0;
                    }
                    else
                    {
                        horizontalOffset = scroller.ExtentWidth * 2.0 - scroller.Width;
                        scroller.HorizontalAnchorRatio = 1.0;
                    }
                });

                ScrollTo(scroller, horizontalOffset, verticalOffset, AnimationMode.Disabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

                RunOnUIThread.Execute(() =>
                {
                    scroller.ViewChanged += delegate (Scroller sender, object args)
                    {
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                        if ((orientation == Orientation.Vertical && expectedFinalOffset == sender.VerticalOffset) ||
                            (orientation == Orientation.Horizontal && expectedFinalOffset == sender.HorizontalOffset))
                        {
                            scrollerViewChangedEvent.Set();
                        }                        
                    };

                    Log.Comment("Inserting child at far edge");
                    InsertStackPanelChild((scroller.Content as Border).Child as StackPanel, 1 /*operationCount*/, c_defaultAnchoringUIStackPanelChildrenCount /*newIndex*/, 1 /*newCount*/);

                    if (viewportSizeChange != 0)
                    {
                        if (orientation == Orientation.Vertical)
                        {
                            Log.Comment("Changing viewport height");
                            scroller.Height += viewportSizeChange;
                        }
                        else
                        {
                            Log.Comment("Changing viewport width");
                            scroller.Width += viewportSizeChange;
                        }
                    }
                });

                WaitForEvent("Waiting for Scroller.ViewChanged event", scrollerViewChangedEvent);
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    if (orientation == Orientation.Vertical)
                    {
                        verticalOffset = scroller.ExtentHeight * 2.0 - scroller.Height;
                    }
                    else
                    {
                        horizontalOffset = scroller.ExtentWidth * 2.0 - scroller.Width;
                    }

                    Log.Comment("Scroller offset change expected");
                    Verify.AreEqual(scroller.HorizontalOffset, horizontalOffset);
                    Verify.AreEqual(scroller.VerticalOffset, verticalOffset);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset shrinks to max value when decreasing viewport width (HorizontalAnchorRatio=1).")]
        public void AnchoringAtRightEdgeWhileDecreasingViewportWidth()
        {
            AnchoringAtFarEdgeWhileDecreasingViewport(Orientation.Horizontal);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset shrinks to max value when decreasing viewport height (VerticalAnchorRatio=1).")]
        public void AnchoringAtBottomEdgeWhileDecreasingViewportHeight()
        {
            AnchoringAtFarEdgeWhileDecreasingViewport(Orientation.Vertical);
        }

        private void AnchoringAtFarEdgeWhileDecreasingViewport(Orientation orientation)
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1 where InteractionTracker's AdjustPositionXIfGreaterThanThreshold/AdjustPositionYIfGreaterThanThreshold are ineffective in this scenario.");
                return;
            }

            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: true,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications: false))
            {
                Scroller scroller = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    scroller = new Scroller();

                    SetupDefaultAnchoringUI(orientation, scroller, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                ZoomTo(scroller, 2.0f, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

                double horizontalOffset = 0.0;
                double verticalOffset = 0.0;

                RunOnUIThread.Execute(() =>
                {
                    if (orientation == Orientation.Vertical)
                    {
                        verticalOffset = scroller.ExtentHeight * 2.0 - scroller.Height;
                        scroller.VerticalAnchorRatio = 1.0;
                    }
                    else
                    {
                        horizontalOffset = scroller.ExtentWidth * 2.0 - scroller.Width;
                        scroller.HorizontalAnchorRatio = 1.0;
                    }
                });

                ScrollTo(scroller, horizontalOffset, verticalOffset, AnimationMode.Disabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

                RunOnUIThread.Execute(() =>
                {
                    scroller.ViewChanged += delegate (Scroller sender, object args)
                    {
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                        scrollerViewChangedEvent.Set();
                    };

                    if (orientation == Orientation.Vertical)
                    {
                        Log.Comment("Decreasing viewport height");
                        scroller.Height -= 100;
                    }
                    else
                    {
                        Log.Comment("Decreasing viewport width");
                        scroller.Width -= 100;
                    }
                });

                WaitForEvent("Waiting for Scroller.ViewChanged event", scrollerViewChangedEvent);
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    if (orientation == Orientation.Vertical)
                    {
                        verticalOffset = scroller.ExtentHeight * 2.0 - scroller.Height;
                    }
                    else
                    {
                        horizontalOffset = scroller.ExtentWidth * 2.0 - scroller.Width;
                    }

                    Log.Comment("Scroller offset change expected");
                    Verify.AreEqual(scroller.HorizontalOffset, horizontalOffset);
                    Verify.AreEqual(scroller.VerticalOffset, verticalOffset);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset growns when inserting an item at the beginning (HorizontalAnchorRatio=0).")]
        public void AnchoringAtAlmostLeftEdge()
        {
            AnchoringAtAlmostNearEdge(Orientation.Horizontal);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset grows when inserting an item at the beginning (VerticalAnchorRatio=0).")]
        public void AnchoringAtAlmostTopEdge()
        {
            AnchoringAtAlmostNearEdge(Orientation.Vertical);
        }

        private void AnchoringAtAlmostNearEdge(Orientation orientation)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: true,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications: false))
            {
                Scroller scroller = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    scroller = new Scroller();

                    SetupDefaultAnchoringUI(orientation, scroller, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                double horizontalOffset = orientation == Orientation.Vertical ? 0.0 : 1.0;
                double verticalOffset = orientation == Orientation.Vertical ? 1.0 : 0.0;

                ScrollTo(scroller, horizontalOffset, verticalOffset, AnimationMode.Disabled, SnapPointsMode.Ignore);

                RunOnUIThread.Execute(() =>
                {
                    scroller.ViewChanged += delegate (Scroller sender, object args)
                    {
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                        scrollerViewChangedEvent.Set();
                    };

                    Log.Comment("Inserting child at near edge");
                    InsertStackPanelChild((scroller.Content as Border).Child as StackPanel, 1 /*operationCount*/, 0 /*newIndex*/, 1 /*newCount*/);
                });

                WaitForEvent("Waiting for Scroller.ViewChanged event", scrollerViewChangedEvent);
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Scroller offset change expected");
                    if (orientation == Orientation.Vertical)
                    {
                        Verify.AreEqual(127.0, scroller.VerticalOffset);
                    }
                    else
                    {
                        Verify.AreEqual(127.0, scroller.HorizontalOffset);
                    }
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset does not change when inserting an item at the end (HorizontalAnchorRatio=1).")]
        public void AnchoringAtAlmostRightEdge()
        {
            AnchoringAtAlmostFarEdge(Orientation.Horizontal);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset does not change when inserting an item at the end (VerticalAnchorRatio=1).")]
        public void AnchoringAtAlmostBottomEdge()
        {
            AnchoringAtAlmostFarEdge(Orientation.Vertical);
        }

        private void AnchoringAtAlmostFarEdge(Orientation orientation)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: true,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications: false))
            {
                Scroller scroller = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    scroller = new Scroller();

                    SetupDefaultAnchoringUI(orientation, scroller, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                ZoomTo(scroller, 2.0f, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

                double horizontalOffset = 0.0;
                double verticalOffset = 0.0;

                RunOnUIThread.Execute(() =>
                {
                    if (orientation == Orientation.Vertical)
                    {
                        verticalOffset = scroller.ExtentHeight * 2.0 - scroller.Height - 1.0;
                        scroller.VerticalAnchorRatio = 1.0;
                    }
                    else
                    {
                        horizontalOffset = scroller.ExtentWidth * 2.0 - scroller.Width - 1.0;
                        scroller.HorizontalAnchorRatio = 1.0;
                    }
                });

                ScrollTo(scroller, horizontalOffset, verticalOffset, AnimationMode.Disabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Inserting child at far edge");
                    InsertStackPanelChild((scroller.Content as Border).Child as StackPanel, 1 /*operationCount*/, c_defaultAnchoringUIStackPanelChildrenCount /*newIndex*/, 1 /*newCount*/);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("No Scroller offset change expected");
                    if (orientation == Orientation.Vertical)
                    {
                        Verify.AreEqual(scroller.VerticalOffset, verticalOffset);
                    }
                    else
                    {
                        Verify.AreEqual(scroller.HorizontalOffset, horizontalOffset);
                    }
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset increases when shrinking the viewport width (HorizontalAnchorRatio=0.5).")]
        public void AnchoringElementWithShrinkingViewport()
        {
            AnchoringElementWithResizedViewport(Orientation.Horizontal, -100.0);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset decreases when growning the viewport height (VerticalAnchorRatio=0.5).")]
        public void AnchoringElementWithGrowningViewport()
        {
            AnchoringElementWithResizedViewport(Orientation.Vertical, 100.0);
        }

        private void AnchoringElementWithResizedViewport(Orientation orientation, double viewportSizeChange)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: true,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications: false))
            {
                Scroller scroller = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    scroller = new Scroller();

                    SetupDefaultAnchoringUI(orientation, scroller, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                ZoomTo(scroller, 2.0f, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

                double horizontalOffset = 0.0;
                double verticalOffset = 0.0;

                RunOnUIThread.Execute(() =>
                {
                    if (orientation == Orientation.Vertical)
                    {
                        verticalOffset = (scroller.ExtentHeight * 2.0 - scroller.Height) / 2.0;
                        scroller.VerticalAnchorRatio = 0.5;
                    }
                    else
                    {
                        horizontalOffset = (scroller.ExtentWidth * 2.0 - scroller.Width) / 2.0;
                        scroller.HorizontalAnchorRatio = 0.5;
                    }
                });

                ScrollTo(scroller, horizontalOffset, verticalOffset, AnimationMode.Disabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Scroller view prior to viewport size change: HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                    scroller.ViewChanged += delegate (Scroller sender, object args)
                    {
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                        scrollerViewChangedEvent.Set();
                    };

                    if (orientation == Orientation.Vertical)
                    {
                        Log.Comment("Changing viewport height");
                        scroller.Height += viewportSizeChange;
                    }
                    else
                    {
                        Log.Comment("Changing viewport width");
                        scroller.Width += viewportSizeChange;
                    }
                });

                WaitForEvent("Waiting for Scroller.ViewChanged event", scrollerViewChangedEvent);
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Scroller view after viewport size change: HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);
                    Log.Comment("Expecting offset change equal to half the viewport size change");
                    if (orientation == Orientation.Vertical)
                    {
                        Verify.AreEqual(scroller.VerticalOffset, verticalOffset - viewportSizeChange / 2.0);
                    }
                    else
                    {
                        Verify.AreEqual(scroller.HorizontalOffset, horizontalOffset - viewportSizeChange / 2.0);
                    }
                });
            }
        }

        private void SetupDefaultAnchoringUI(
            Orientation orientation,
            Scroller scroller,
            AutoResetEvent scrollerLoadedEvent)
        {
            Log.Comment("Setting up default anchoring UI with Scroller");

            StackPanel stackPanel = new StackPanel();
            stackPanel.Name = "stackPanel";
            stackPanel.Orientation = orientation;
            stackPanel.Margin = new Thickness(30);

            Border border = new Border();
            border.Name = "border";
            border.BorderThickness = new Thickness(3);
            border.BorderBrush = new SolidColorBrush(Colors.Chartreuse);
            border.Margin = new Thickness(15);
            border.Background = new SolidColorBrush(Colors.Beige);
            border.Child = stackPanel;

            Verify.IsNotNull(scroller);
            scroller.Name = "scroller";
            if (orientation == Orientation.Vertical)
            {
                scroller.ContentOrientation = ContentOrientation.Vertical;
                scroller.Width = c_defaultAnchoringUIScrollerConstrainedSize;
                scroller.Height = c_defaultAnchoringUIScrollerNonConstrainedSize;
            }
            else
            {
                scroller.ContentOrientation = ContentOrientation.Horizontal;
                scroller.Width = c_defaultAnchoringUIScrollerNonConstrainedSize;
                scroller.Height = c_defaultAnchoringUIScrollerConstrainedSize;
            }
            scroller.Background = new SolidColorBrush(Colors.AliceBlue);
            scroller.Content = border;

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, 0 /*newIndex*/, c_defaultAnchoringUIStackPanelChildrenCount /*newCount*/);

            if (scrollerLoadedEvent != null)
            {
                scroller.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Loaded event handler");
                    scrollerLoadedEvent.Set();
                };
            }

            scroller.AnchorRequested += (Scroller sender, ScrollerAnchorRequestedEventArgs args) =>
            {
                Log.Comment("Scroller.AnchorRequested event handler");

                Verify.IsNull(args.AnchorElement);
                Verify.AreEqual(0, args.AnchorCandidates.Count);

                StackPanel sp = (sender.Content as Border).Child as StackPanel;
                foreach (Border b in sp.Children)
                {
                    args.AnchorCandidates.Add(b);
                }
            };

            Log.Comment("Setting window content");
            Content = scroller;
        }

        private void InsertStackPanelChild(StackPanel stackPanel, int operationCount, int newIndex, int newCount, string namePrefix = "")
        {
            if (newIndex < 0 || newIndex > stackPanel.Children.Count || newCount <= 0)
            {
                throw new ArgumentException();
            }

            SolidColorBrush chartreuseBrush = new SolidColorBrush(Colors.Chartreuse);
            SolidColorBrush blanchedAlmondBrush = new SolidColorBrush(Colors.BlanchedAlmond);

            for (int i = 0; i < newCount; i++)
            {
                TextBlock textBlock = new TextBlock();
                textBlock.Text = "TB#" + stackPanel.Children.Count + "_" + operationCount;
                textBlock.Name = namePrefix + "textBlock" + stackPanel.Children.Count + "_" + operationCount;
                textBlock.HorizontalAlignment = HorizontalAlignment.Center;
                textBlock.VerticalAlignment = VerticalAlignment.Center;

                Border border = new Border();
                border.Name = namePrefix + "border" + stackPanel.Children.Count + "_" + operationCount;
                border.BorderThickness = border.Margin = new Thickness(3);
                border.BorderBrush = chartreuseBrush;
                border.Background = blanchedAlmondBrush;
                border.Width = stackPanel.Orientation == Orientation.Vertical ? 170 : 120;
                border.Height = stackPanel.Orientation == Orientation.Vertical ? 120 : 170;
                border.Child = textBlock;

                stackPanel.Children.Insert(newIndex + i, border);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies vertical offset does not exceed its max value because of anchoring, when reducing the extent height.")]
        public void AnchoringWithReducedExtent()
        {
            AnchoringWithOffsetCoercion(false /*reduceAnchorOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies vertical offset does not exceed its max value because of anchoring, when reducing the extent height and anchor offset.")]
        public void AnchoringWithReducedExtentAndAnchorOffset()
        {
            AnchoringWithOffsetCoercion(true /*reduceAnchorOffset*/);
        }

        private void AnchoringWithOffsetCoercion(bool reduceAnchorOffset)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: true,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications: false))
            {
                Scroller scroller = null;
                Border anchorElement = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerAnchorRequestedEvent = new AutoResetEvent(false);

                // This test validates that the Scroller accounts for maximum vertical offset (based on viewport and content extent) 
                // when calculating the vertical offset shift for anchoring. The vertical offset cannot exceed content extent - viewport.

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Visual tree setup");
                    anchorElement = new Border
                    {
                        Width = 100,
                        Height = 100,
                        Background = new SolidColorBrush(Colors.Red),
                        Margin = new Thickness(0, 600, 0, 0),
                        VerticalAlignment = VerticalAlignment.Top
                    };

                    Grid grid = new Grid();
                    grid.Children.Add(anchorElement);
                    grid.Width = 200;
                    grid.Height = 1000;
                    grid.Background = new SolidColorBrush(Colors.Gray);

                    scroller = new Scroller
                    {
                        Content = grid,
                        Width = 200,
                        Height = 200
                    };

                    scroller.Loaded += (object sender, RoutedEventArgs e) =>
                    {
                        Log.Comment("Scroller.Loaded event handler");
                        scrollerLoadedEvent.Set();
                    };

                    scroller.ViewChanged += delegate (Scroller sender, object args)
                    {
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                        if ((reduceAnchorOffset && sender.VerticalOffset == 400) ||
                            (!reduceAnchorOffset && sender.VerticalOffset == 500))
                        {
                            scrollerViewChangedEvent.Set();
                        }
                    };

                    scroller.AnchorRequested += delegate (Scroller sender, ScrollerAnchorRequestedEventArgs args)
                    {
                        Log.Comment("Scroller.AnchorRequested event handler. Forcing the red Border to be the Scroller anchor.");
                        args.AnchorElement = anchorElement;
                        scrollerAnchorRequestedEvent.Set();
                    };

                    Log.Comment("Setting window content");
                    Content = scroller;
                });

                WaitForEvent("Waiting for Scroller.Loaded event", scrollerLoadedEvent);
                IdleSynchronizer.Wait();

                ScrollTo(scroller, 0.0, 600.0, AnimationMode.Disabled, SnapPointsMode.Ignore);

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(600, scroller.VerticalOffset);

                    Log.Comment("Scroller.Content height is reduced by 300px. Scroller.VerticalOffset is expected to be reduced by 100px (600 -> 500).");
                    (scroller.Content as Grid).Height = 700;
                    if (reduceAnchorOffset)
                    {
                        Log.Comment("Tracked element is shifted up by 200px within the Scroller.Content (600 -> 400). Anchoring is expected to reduce the VerticalOffset by half of that (500 -> 400).");
                        anchorElement.Margin = new Thickness(0, 400, 0, 0);
                    }
                    scrollerViewChangedEvent.Reset();
                });

                WaitForEvent("Waiting for Scroller.ViewChanged event", scrollerViewChangedEvent);
                WaitForEvent("Waiting for Scroller.AnchorRequested event", scrollerAnchorRequestedEvent);
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(reduceAnchorOffset ? 400 : 500, scroller.VerticalOffset);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset adjusts when inserting and removing items at the beginning (VerticalAnchorRatio=0.5).")]
        public void AnchoringAtRepeaterMiddle()
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: true,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications: false))
            {
                using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("Scroller"))
                {
                    Scroller scroller = null;
                    AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                    AutoResetEvent scrollerViewChangedEvent = new AutoResetEvent(false);

                    RunOnUIThread.Execute(() =>
                    {
                        scroller = new Scroller();

                        SetupRepeaterAnchoringUI(scroller, scrollerLoadedEvent);

                        scroller.HorizontalAnchorRatio = double.NaN;
                        scroller.VerticalAnchorRatio = 0.5;
                    });

                    WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                    ZoomTo(scroller, 2.0f, 0.0f, 0.0f, AnimationMode.Enabled, SnapPointsMode.Ignore);
                    ScrollTo(scroller, 0.0, 250.0, AnimationMode.Enabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

                    ItemsRepeater repeater = null;
                    TestDataSource dataSource = null;

                    RunOnUIThread.Execute(() =>
                    {
                        repeater = (scroller.Content as Border).Child as ItemsRepeater;
                        dataSource = repeater.ItemsSource as TestDataSource;

                        scroller.ViewChanged += delegate (Scroller sender, object args) {
                            scrollerViewChangedEvent.Set();
                        };

                        Log.Comment("Inserting items at the beginning");
                        dataSource.Insert(0 /*index*/, 2 /*count*/);
                    });

                    WaitForEvent("Waiting for Scroller.ViewChanged event", scrollerViewChangedEvent);

                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment("Scroller offset change expected");
                        Verify.AreEqual(520.0, scroller.VerticalOffset);
                    });

                    RunOnUIThread.Execute(() =>
                    {
                        scrollerViewChangedEvent.Reset();

                        Log.Comment("Removing items from the beginning");
                        dataSource.Remove(0 /*index*/, 2 /*count*/);
                    });

                    WaitForEvent("Waiting for Scroller.ViewChanged event", scrollerViewChangedEvent);

                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment("Scroller offset change expected");
                        Verify.AreEqual(250.0, scroller.VerticalOffset);
                    });
                }
            }
        }

        private void SetupRepeaterAnchoringUI(
            Scroller scroller,
            AutoResetEvent scrollerLoadedEvent)
        {
            Log.Comment("Setting up ItemsRepeater anchoring UI with Scroller and ItemsRepeater");

            TestDataSource dataSource = new TestDataSource(
                Enumerable.Range(0, c_defaultAnchoringUIRepeaterChildrenCount).Select(i => string.Format("Item #{0}", i)).ToList());

            RecyclingElementFactory elementFactory = new RecyclingElementFactory();
            elementFactory.RecyclePool = new RecyclePool();
            elementFactory.Templates["Item"] = XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <Border BorderThickness='3' BorderBrush='Chartreuse' Width='100' Height='100' Margin='3' Background='BlanchedAlmond'>
                          <TextBlock Text='{Binding}' HorizontalAlignment='Center' VerticalAlignment='Center'/>
                        </Border>
                      </DataTemplate>") as DataTemplate;

            ItemsRepeater repeater = new ItemsRepeater()
            {
                Name = "repeater",
                ItemsSource = dataSource,
                ItemTemplate = elementFactory,
                Layout = new UniformGridLayout()
                {
                    MinItemWidth = 125,
                    MinItemHeight = 125,
                    MinRowSpacing = 10,
                    MinColumnSpacing = 10
                },
                Margin = new Thickness(30)
            };

            Border border = new Border()
            {
                Name = "border",
                BorderThickness = new Thickness(3),
                BorderBrush = new SolidColorBrush(Colors.Chartreuse),
                Margin = new Thickness(15),
                Background = new SolidColorBrush(Colors.Beige),
                Child = repeater
            };

            Verify.IsNotNull(scroller);
            scroller.Name = "scroller";
            scroller.ContentOrientation = ContentOrientation.Vertical;
            scroller.Width = 400;
            scroller.Height = 600;
            scroller.Background = new SolidColorBrush(Colors.AliceBlue);
            scroller.Content = border;

            if (scrollerLoadedEvent != null)
            {
                scroller.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Loaded event handler");
                    scrollerLoadedEvent.Set();
                };
            }

            scroller.AnchorRequested += (Scroller sender, ScrollerAnchorRequestedEventArgs args) =>
            {
                Log.Comment("Scroller.AnchorRequested event handler");

                Verify.IsNull(args.AnchorElement);
                Verify.IsGreaterThan(args.AnchorCandidates.Count, 0);
            };

            Log.Comment("Setting window content");
            Content = scroller;
        }

        private class TestDataSource : CustomItemsSourceViewWithUniqueIdMapping
        {
            public TestDataSource(List<string> source)
            {
                Inner = source;
            }

            public List<string> Inner
            {
                get;
                set;
            }

            protected override int GetSizeCore()
            {
                return Inner.Count;
            }

            protected override object GetAtCore(int index)
            {
                return Inner[index];
            }

            protected override string KeyFromIndexCore(int index)
            {
                return Inner[index].ToString();
            }

            public void Insert(int index, int count)
            {
                for (int i = 0; i < count; i++)
                {
                    Inner.Insert(index + i, string.Format("ItemI #{0}", Inner.Count));
                }

                OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                    NotifyCollectionChangedAction.Add,
                    oldStartingIndex: -1,
                    oldItemsCount: 0,
                    newStartingIndex: index,
                    newItemsCount: count));
            }

            public void Remove(int index, int count)
            {
                for (int i = 0; i < count; i++)
                {
                    Inner.RemoveAt(index);
                }

                OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                    NotifyCollectionChangedAction.Remove,
                    oldStartingIndex: index,
                    oldItemsCount: count,
                    newStartingIndex: -1,
                    newItemsCount: 0));
            }
        }
    }
}
