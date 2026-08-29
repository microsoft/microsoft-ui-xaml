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

namespace Microsoft.UI.Xaml.Tests.Controls.ScrollViewerTests
{
    [TestClass]
    public class ScrollViewerAnchoringTests : XamlTestsBase
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
        [TestProperty("Description", "Verifies that updating CanBeScrollAnchor registers/unregisters the element on the anchor provider.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyAnchorCandidateRegistration()
        {
            ScrollViewer outerScrollViewer = null;
            ScrollViewer innerScrollViewer = null;
            StackPanel stackPanel = null;
            Button item = null;
            AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
            List<UIElement> anchorCandidates = new List<UIElement>();

            UIExecutor.Execute(() =>
            {
                outerScrollViewer = new ScrollViewer();
                outerScrollViewer.HorizontalAnchorRatio = 0.0;
                outerScrollViewer.VerticalAnchorRatio = 0.5;
                outerScrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                outerScrollViewer.VerticalScrollMode = ScrollMode.Enabled;
                outerScrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                outerScrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                outerScrollViewer.Loaded += (sender, args) =>
                {
                    scrollViewerLoadedEvent.Set();
                };

                innerScrollViewer = new ScrollViewer();
                innerScrollViewer.HorizontalAnchorRatio = 0.0;
                innerScrollViewer.VerticalAnchorRatio = 0.5;
                innerScrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                innerScrollViewer.VerticalScrollMode = ScrollMode.Enabled;
                innerScrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                innerScrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                innerScrollViewer.AnchorRequested += (sender, args) =>
                {
                    Verify.IsNull(args.Anchor);
                    anchorCandidates.Clear();
                    foreach(var candidate in args.AnchorCandidates)
                    {
                        anchorCandidates.Add(candidate);
                    }
                };

                outerScrollViewer.Content = innerScrollViewer;

                stackPanel = new StackPanel();
                innerScrollViewer.Content = stackPanel;

                Log.Comment("Setting window content");
                TestServices.WindowHelper.WindowContent = outerScrollViewer;
            });

            WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

            UIExecutor.Execute(() =>
            {
                item = new Button() { CanBeScrollAnchor = true };
                stackPanel.Children.Add(item);
                // Make sure that the inner ScrollViewer runs
                // arrange so that the anchor requested event is raised
                // where we can look at its current candidates.
                innerScrollViewer.InvalidateArrange();
                innerScrollViewer.UpdateLayout();
                Verify.AreEqual(1, anchorCandidates.Count);

                item.CanBeScrollAnchor = false;
                // Make sure that the inner ScrollViewer runs
                // arrange so that the anchor requested event is raised
                // where we can look at its current candidates.
                innerScrollViewer.InvalidateArrange();
                innerScrollViewer.UpdateLayout();
                Verify.AreEqual(0, anchorCandidates.Count);
            });
        }

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
            ScrollViewer scrollViewer = null;
            AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                scrollViewer = new ScrollViewer();
                scrollViewer.HorizontalAnchorRatio = 0.0;
                scrollViewer.VerticalAnchorRatio = 0.0;
                scrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                scrollViewer.VerticalScrollMode = ScrollMode.Enabled;
                scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                SetupDefaultAnchoringUI(orientation, scrollViewer, scrollViewerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

            UIExecutor.Execute(() =>
             {
                 Log.Comment("Inserting child at near edge");
                 InsertStackPanelChild((scrollViewer.Content as Border).Child as StackPanel, 1 /*operationCount*/, 0 /*newIndex*/, 1 /*newCount*/);
             });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("No scrollViewer offset change expected");
                if (orientation == Orientation.Vertical)
                {
                    Verify.AreEqual(scrollViewer.VerticalOffset, 0);
                }
                else
                {
                    Verify.AreEqual(scrollViewer.HorizontalOffset, 0);
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset growns to max value when inserting an item at the end (HorizontalAnchorRatio=1).")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void AnchoringAtRightEdgeWhileIncreasingContentWidth()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Horizontal, 0 /*viewportSizeChange*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset grows to max value when inserting an item at the end (VerticalAnchorRatio=1).")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void AnchoringAtBottomEdgeWhileIncreasingContentHeight()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Vertical, 0 /*viewportSizeChange*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset growns to max value when inserting an item at the end and growing viewport (HorizontalAnchorRatio=1).")]
        public void AnchoringAtRightEdgeWhileIncreasingContentAndViewportWidth()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Horizontal, 10 /*viewportSizeChange*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset grows to max value when inserting an item at the end and growning viewport (VerticalAnchorRatio=1).")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void AnchoringAtBottomEdgeWhileIncreasingContentAndViewportHeight()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Vertical, 10 /*viewportSizeChange*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset growns to max value when inserting an item at the end and shrinking viewport (HorizontalAnchorRatio=1).")]
        public void AnchoringAtRightEdgeWhileIncreasingContentAndDecreasingViewportWidth()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Horizontal, -10 /*viewportSizeChange*/);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset grows to max value when inserting an item at the end and shrinking viewport (VerticalAnchorRatio=1).")]
        public void AnchoringAtBottomEdgeWhileIncreasingContentAndDecreasingViewportHeight()
        {
            AnchoringAtFarEdgeWhileIncreasingContent(Orientation.Vertical, -10 /*viewportSizeChange*/);
        }

        private void AnchoringAtFarEdgeWhileIncreasingContent(Orientation orientation, double viewportSizeChange)
        {
            ScrollViewer scrollViewer = null;
            AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollViewerViewChangedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
             {
                 scrollViewer = new ScrollViewer();
                 scrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                 scrollViewer.VerticalScrollMode = ScrollMode.Enabled;
                 scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                 scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                 SetupDefaultAnchoringUI(orientation, scrollViewer, scrollViewerLoadedEvent);
             });

            WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Starting View - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}, ExtentWidth={3}, ExtentHeight={4}",
                         scrollViewer.HorizontalOffset, scrollViewer.VerticalOffset, scrollViewer.ZoomFactor, scrollViewer.ExtentWidth, scrollViewer.ExtentHeight);
            });

            double horizontalOffset = 0.0;
            double verticalOffset = 0.0;

            UIExecutor.Execute(() =>
             {
                 if (orientation == Orientation.Vertical)
                 {
                     verticalOffset = scrollViewer.ExtentHeight * 2.0 - scrollViewer.Height;
                     scrollViewer.HorizontalAnchorRatio = 0.0;
                     scrollViewer.VerticalAnchorRatio = 1.0;
                 }
                 else
                 {
                     horizontalOffset = scrollViewer.ExtentWidth * 2.0 - scrollViewer.Width;
                     scrollViewer.HorizontalAnchorRatio = 1.0;
                     scrollViewer.VerticalAnchorRatio = 0.0;
                 }
             });

            ChangeView(scrollViewer, null /* horizontalOffset */, null /* verticalOffset */, 2.0f /* zoomFactor */);

            ChangeView(scrollViewer, horizontalOffset, verticalOffset, null /* zoomFactor */);

            UIExecutor.Execute(() =>
             {
                 scrollViewer.ViewChanged += delegate (object sender, ScrollViewerViewChangedEventArgs args)
                 {
                     var sv = sender as ScrollViewer;
                     Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}, ExtentWidth={3}, ExtentHeight={4}",
                         sv.HorizontalOffset, sv.VerticalOffset, sv.ZoomFactor, sv.ExtentWidth, sv.ExtentHeight);
                     scrollViewerViewChangedEvent.Set();
                 };

                 Log.Comment("Inserting child at far edge");
                 InsertStackPanelChild((scrollViewer.Content as Border).Child as StackPanel, 1 /*operationCount*/, c_defaultAnchoringUIStackPanelChildrenCount /*newIndex*/, 1 /*newCount*/);

                 if (viewportSizeChange != 0)
                 {
                     if (orientation == Orientation.Vertical)
                     {
                         Log.Comment("Changing viewport height");
                         scrollViewer.Height += viewportSizeChange;
                     }
                     else
                     {
                         Log.Comment("Changing viewport width");
                         scrollViewer.Width += viewportSizeChange;
                     }
                 }
             });

            WaitForEvent("Waiting for scrollViewer.ViewChanged event", scrollViewerViewChangedEvent);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
             {
                 if (orientation == Orientation.Vertical)
                 {
                     verticalOffset = scrollViewer.ExtentHeight - scrollViewer.Height;
                 }
                 else
                 {
                     horizontalOffset = scrollViewer.ExtentWidth - scrollViewer.Width;
                 }

                 Log.Comment("scrollViewer offset change expected");
                 Verify.AreEqual(scrollViewer.HorizontalOffset, horizontalOffset);
                 Verify.AreEqual(scrollViewer.VerticalOffset, verticalOffset);
             });
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
            ScrollViewer scrollViewer = null;
            AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollViewerViewChangedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
             {
                 scrollViewer = new ScrollViewer();
                 scrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                 scrollViewer.VerticalScrollMode = ScrollMode.Enabled;
                 scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                 scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                 SetupDefaultAnchoringUI(orientation, scrollViewer, scrollViewerLoadedEvent);
             });

            WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

            double horizontalOffset = 0.0;
            double verticalOffset = 0.0;

            UIExecutor.Execute(() =>
            {
                if (orientation == Orientation.Vertical)
                {
                    verticalOffset = scrollViewer.ExtentHeight * 2.0 - scrollViewer.Height;
                    scrollViewer.HorizontalAnchorRatio = 0.0;
                    scrollViewer.VerticalAnchorRatio = 1.0;
                }
                else
                {
                    horizontalOffset = scrollViewer.ExtentWidth * 2.0 - scrollViewer.Width;
                    scrollViewer.HorizontalAnchorRatio = 1.0;
                    scrollViewer.VerticalAnchorRatio = 0.0;
                }
            });

            ChangeView(scrollViewer, 0.0f, 0.0f, 2.0f);

            ChangeView(scrollViewer, horizontalOffset, verticalOffset, null);

            UIExecutor.Execute(() =>
             {
                 scrollViewer.ViewChanged += delegate (object sender, ScrollViewerViewChangedEventArgs args)
                 {
                     var sv = sender as ScrollViewer;
                     Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                         sv.HorizontalOffset, sv.VerticalOffset, sv.ZoomFactor);
                     scrollViewerViewChangedEvent.Set();
                 };

                 if (orientation == Orientation.Vertical)
                 {
                     Log.Comment("Decreasing viewport height");
                     scrollViewer.Height -= 100;
                 }
                 else
                 {
                     Log.Comment("Decreasing viewport width");
                     scrollViewer.Width -= 100;
                 }
             });

            WaitForEvent("Waiting for scrollViewer.ViewChanged event", scrollViewerViewChangedEvent);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
             {
                 if (orientation == Orientation.Vertical)
                 {
                     verticalOffset = scrollViewer.ExtentHeight - scrollViewer.Height;
                 }
                 else
                 {
                     horizontalOffset = scrollViewer.ExtentWidth - scrollViewer.Width;
                 }

                 Log.Comment("scrollViewer offset change expected");
                 Verify.AreEqual(scrollViewer.HorizontalOffset, horizontalOffset);
                 Verify.AreEqual(scrollViewer.VerticalOffset, verticalOffset);
             });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset growns when inserting an item at the beginning (HorizontalAnchorRatio=0).")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void AnchoringAtAlmostLeftEdge()
        {
            AnchoringAtAlmostNearEdge(Orientation.Horizontal);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset grows when inserting an item at the beginning (VerticalAnchorRatio=0).")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void AnchoringAtAlmostTopEdge()
        {
            AnchoringAtAlmostNearEdge(Orientation.Vertical);
        }

        private void AnchoringAtAlmostNearEdge(Orientation orientation)
        {
            ScrollViewer scrollViewer = null;
            AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollViewerViewChangedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
             {
                 scrollViewer = new ScrollViewer();
                 scrollViewer.HorizontalAnchorRatio = 0.0;
                 scrollViewer.VerticalAnchorRatio = 0.0;
                 scrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                 scrollViewer.VerticalScrollMode = ScrollMode.Enabled;
                 scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                 scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                 SetupDefaultAnchoringUI(orientation, scrollViewer, scrollViewerLoadedEvent);
             });

            WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

            double horizontalOffset = orientation == Orientation.Vertical ? 0.0 : 1.0;
            double verticalOffset = orientation == Orientation.Vertical ? 1.0 : 0.0;

            ChangeView(scrollViewer, horizontalOffset, verticalOffset, null);

            UIExecutor.Execute(() =>
             {
                 scrollViewer.ViewChanged += delegate (object sender, ScrollViewerViewChangedEventArgs args)
                 {
                     var sv = sender as ScrollViewer;
                     Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                         sv.HorizontalOffset, sv.VerticalOffset, sv.ZoomFactor);
                     scrollViewerViewChangedEvent.Set();
                 };

                 Log.Comment("Inserting child at near edge");
                 InsertStackPanelChild((scrollViewer.Content as Border).Child as StackPanel, 1 /*operationCount*/, 0 /*newIndex*/, 1 /*newCount*/);
             });

            WaitForEvent("Waiting for scrollViewer.ViewChanged event", scrollViewerViewChangedEvent);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
             {
                 Log.Comment("scrollViewer offset change expected");
                 if (orientation == Orientation.Vertical)
                 {
                     Verify.AreEqual(scrollViewer.VerticalOffset, 127.0);
                 }
                 else
                 {
                     Verify.AreEqual(scrollViewer.HorizontalOffset, 127.0);
                 }
             });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset does not change when inserting an item at the end (HorizontalAnchorRatio=1).")]
        public void AnchoringAtAlmostRightEdge()
        {
            AnchoringAtAlmostFarEdge(Orientation.Horizontal);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset does not change when inserting an item at the end (VerticalAnchorRatio=1).")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void AnchoringAtAlmostBottomEdge()
        {
            AnchoringAtAlmostFarEdge(Orientation.Vertical);
        }

        private void AnchoringAtAlmostFarEdge(Orientation orientation)
        {
            ScrollViewer scrollViewer = null;
            AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
             {
                 scrollViewer = new ScrollViewer();
                 scrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                 scrollViewer.VerticalScrollMode = ScrollMode.Enabled;
                 scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                 scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                 SetupDefaultAnchoringUI(orientation, scrollViewer, scrollViewerLoadedEvent);
             });

            WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

            double horizontalOffset = 0.0;
            double verticalOffset = 0.0;

            UIExecutor.Execute(() =>
             {
                 if (orientation == Orientation.Vertical)
                 {
                     verticalOffset = scrollViewer.ExtentHeight * 2.0 - scrollViewer.Height - 1.0;
                     scrollViewer.HorizontalAnchorRatio = 0.0;
                     scrollViewer.VerticalAnchorRatio = 1.0;
                 }
                 else
                 {
                     horizontalOffset = scrollViewer.ExtentWidth * 2.0 - scrollViewer.Width - 1.0;
                     scrollViewer.HorizontalAnchorRatio = 1.0;
                     scrollViewer.VerticalAnchorRatio = 0.0;
                 }
             });

            ChangeView(scrollViewer, 0.0f, 0.0f, 2.0f);
            ChangeView(scrollViewer, horizontalOffset, verticalOffset, null);

            UIExecutor.Execute(() =>
             {
                 Log.Comment("Inserting child at far edge");
                 InsertStackPanelChild((scrollViewer.Content as Border).Child as StackPanel, 1 /*operationCount*/, c_defaultAnchoringUIStackPanelChildrenCount /*newIndex*/, 1 /*newCount*/);
             });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
             {
                 Log.Comment("No scrollViewer offset change expected");
                 if (orientation == Orientation.Vertical)
                 {
                     Verify.AreEqual(scrollViewer.VerticalOffset, verticalOffset);
                 }
                 else
                 {
                     Verify.AreEqual(scrollViewer.HorizontalOffset, horizontalOffset);
                 }
             });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalOffset increases when shrinking the viewport width (HorizontalAnchorRatio=0.5).")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void AnchoringElementWithShrinkingViewport()
        {
            AnchoringElementWithResizedViewport(Orientation.Horizontal, -100.0);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies VerticalOffset decreases when growning the viewport height (VerticalAnchorRatio=0.5).")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void AnchoringElementWithGrowningViewport()
        {
            AnchoringElementWithResizedViewport(Orientation.Vertical, 100.0);
        }

        private void AnchoringElementWithResizedViewport(Orientation orientation, double viewportSizeChange)
        {
            ScrollViewer scrollViewer = null;
            AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollViewerViewChangedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
             {
                 scrollViewer = new ScrollViewer();
                 scrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                 scrollViewer.VerticalScrollMode = ScrollMode.Enabled;
                 scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                 scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                 SetupDefaultAnchoringUI(orientation, scrollViewer, scrollViewerLoadedEvent);
             });

            WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

            double horizontalOffset = 0.0;
            double verticalOffset = 0.0;
            UIExecutor.Execute(() =>
             {
                 if (orientation == Orientation.Vertical)
                 {
                     verticalOffset = (scrollViewer.ExtentHeight * 2.0 - scrollViewer.Height) / 2.0;
                     scrollViewer.HorizontalAnchorRatio = 0.0;
                     scrollViewer.VerticalAnchorRatio = 0.5;
                 }
                 else
                 {
                     horizontalOffset = (scrollViewer.ExtentWidth * 2.0 - scrollViewer.Width) / 2.0;
                     scrollViewer.HorizontalAnchorRatio = 0.5;
                     scrollViewer.VerticalAnchorRatio = 0.0;
                 }
             });

            ChangeView(scrollViewer, 0.0f, 0.0f, 2.0f);
            ChangeView(scrollViewer, horizontalOffset, verticalOffset, 2.0f);

            UIExecutor.Execute(() =>
             {
                 Log.Comment("scrollViewer view prior to viewport size change: HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                         scrollViewer.HorizontalOffset, scrollViewer.VerticalOffset, scrollViewer.ZoomFactor);

                 scrollViewer.ViewChanged += delegate (object sender, ScrollViewerViewChangedEventArgs args)
                 {
                     var sv = sender as ScrollViewer;
                     Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                         sv.HorizontalOffset, sv.VerticalOffset, sv.ZoomFactor);
                     scrollViewerViewChangedEvent.Set();
                 };

                 if (orientation == Orientation.Vertical)
                 {
                     Log.Comment("Changing viewport height");
                     scrollViewer.Height += viewportSizeChange;
                 }
                 else
                 {
                     Log.Comment("Changing viewport width");
                     scrollViewer.Width += viewportSizeChange;
                 }
             });

            WaitForEvent("Waiting for scrollViewer.ViewChanged event", scrollViewerViewChangedEvent);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
             {
                 Log.Comment("scrollViewer view after viewport size change: HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                         scrollViewer.HorizontalOffset, scrollViewer.VerticalOffset, scrollViewer.ZoomFactor);
                 Log.Comment("Expecting offset change equal to half the viewport size change");
                 if (orientation == Orientation.Vertical)
                 {
                     Verify.AreEqual(scrollViewer.VerticalOffset, verticalOffset - viewportSizeChange / 2.0);
                 }
                 else
                 {
                     Verify.AreEqual(scrollViewer.HorizontalOffset, horizontalOffset - viewportSizeChange / 2.0);
                 }
             });
        }

        private void SetupDefaultAnchoringUI(
            Orientation orientation,
            ScrollViewer scrollViewer,
            AutoResetEvent scrollViewerLoadedEvent)
        {
            Log.Comment("Setting up default anchoring UI with scrollViewer");

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

            Verify.IsNotNull(scrollViewer);
            scrollViewer.Name = "scrollViewer";
            if (orientation == Orientation.Vertical)
            {
                scrollViewer.Width = c_defaultAnchoringUIscrollViewerConstrainedSize;
                scrollViewer.Height = c_defaultAnchoringUIscrollViewerNonConstrainedSize;
            }
            else
            {
                scrollViewer.Width = c_defaultAnchoringUIscrollViewerNonConstrainedSize;
                scrollViewer.Height = c_defaultAnchoringUIscrollViewerConstrainedSize;
            }

            scrollViewer.Background = new SolidColorBrush(Colors.AliceBlue);
            scrollViewer.Content = border;

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, 0 /*newIndex*/, c_defaultAnchoringUIStackPanelChildrenCount /*newCount*/);

            if (scrollViewerLoadedEvent != null)
            {
                scrollViewer.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("scrollViewer.Loaded event handler");
                    scrollViewerLoadedEvent.Set();
                };
            }

            scrollViewer.AnchorRequested += (ScrollViewer sender, AnchorRequestedEventArgs args) =>
            {
                Log.Comment("scrollViewer.AnchorRequested event handler");

                Verify.IsNull(args.Anchor);
                Verify.AreEqual(args.AnchorCandidates.Count, 0);

                StackPanel sp = (sender.Content as Border).Child as StackPanel;
                foreach (Border b in sp.Children)
                {
                    args.AnchorCandidates.Add(b);
                }
            };

            Log.Comment("Setting window content");
            TestServices.WindowHelper.WindowContent = scrollViewer;
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
        [TestProperty("Hosting:Mode", "UAP")]
        public void AnchoringWithReducedExtent()
        {
            AnchoringWithOffsetCoercion(false /*reduceAnchorOffset*/);
        }

        //[TestMethod] Bug. We don't seem to track extent change and viewport change them together.
        [TestProperty("Description", "Verifies vertical offset does not exceed its max value because of anchoring, when reducing the extent height and anchor offset.")]
        public void AnchoringWithReducedExtentAndAnchorOffset()
        {
            AnchoringWithOffsetCoercion(true /*reduceAnchorOffset*/);
        }

        private void AnchoringWithOffsetCoercion(bool reduceAnchorOffset)
        {
            ScrollViewer scrollViewer = null;
            Border anchorElement = null;
            AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollViewerViewChangedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollViewerAnchorRequestedEvent = new AutoResetEvent(false);

            // This test validates that the scrollViewer accounts for maximum vertical offset (based on viewport and child extent) 
            // when calculating the vertical offset shift for anchoring. The vertical offset cannot exceed child extent - viewport.

            UIExecutor.Execute(() =>
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

                 scrollViewer = new ScrollViewer
                 {
                     Content = grid,
                     Width = 200,
                     Height = 200
                 };

                 scrollViewer.HorizontalAnchorRatio = 0.0;
                 scrollViewer.VerticalAnchorRatio = 0.0;
                 scrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                 scrollViewer.VerticalScrollMode = ScrollMode.Enabled;
                 scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                 scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;

                 scrollViewer.Loaded += (object sender, RoutedEventArgs e) =>
                 {
                     Log.Comment("scrollViewer.Loaded event handler");
                     scrollViewerLoadedEvent.Set();
                 };

                 scrollViewer.ViewChanged += delegate (object sender, ScrollViewerViewChangedEventArgs args)
                 {
                     var sv = sender as ScrollViewer;
                     Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                         sv.HorizontalOffset, sv.VerticalOffset, sv.ZoomFactor);
                     if ((reduceAnchorOffset && sv.VerticalOffset == 400) ||
                         (!reduceAnchorOffset && sv.VerticalOffset == 500))
                     {
                         scrollViewerViewChangedEvent.Set();
                     }
                 };

                 scrollViewer.AnchorRequested += delegate (ScrollViewer sender, AnchorRequestedEventArgs args)
                 {
                     Log.Comment("scrollViewer.AnchorRequested event handler. Forcing the red Border to be the scrollViewer anchor.");
                     args.Anchor = anchorElement;
                     scrollViewerAnchorRequestedEvent.Set();
                 };

                 Log.Comment("Setting window content");
                 TestServices.WindowHelper.WindowContent = scrollViewer;
             });

            WaitForEvent("Waiting for scrollViewer.Loaded event", scrollViewerLoadedEvent);
            TestServices.WindowHelper.WaitForIdle();

            ChangeView(scrollViewer, 0.0, 600.0, null);

            UIExecutor.Execute(() =>
             {
                 Verify.AreEqual(600, scrollViewer.VerticalOffset);

                 Log.Comment("scrollViewer.Child height is reduced by 300px. scrollViewer.VerticalOffset is expected to be reduced by 100px (600 -> 500).");
                 (scrollViewer.Content as Grid).Height = 700;
                 if (reduceAnchorOffset)
                 {
                     Log.Comment("Tracked element is shifted up by 200px within the scrollViewer.Child (600 -> 400). Anchoring is expected to reduce the VerticalOffset by half of that (500 -> 400).");
                     anchorElement.Margin = new Thickness(0, 400, 0, 0);
                 }
                 scrollViewerViewChangedEvent.Reset();
             });

            WaitForEvent("Waiting for scrollViewer.ViewChanged event", scrollViewerViewChangedEvent);
            WaitForEvent("Waiting for scrollViewer.AnchorRequested event", scrollViewerAnchorRequestedEvent);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
             {
                 Verify.AreEqual(reduceAnchorOffset ? 400 : 500, scrollViewer.VerticalOffset);
             });
        }

        private void ChangeView(
            ScrollViewer scrollViewer,
            Nullable<double> horizontalOffset = null,
            Nullable<double> verticalOffset = null,
            Nullable<float> zoomFactor = null)
        {
            bool disableAnimations = true;
            AutoResetEvent viewChanged = new AutoResetEvent(false);
            EventHandler<ScrollViewerViewChangedEventArgs> viewChangedHandler = null;
            Log.Comment("Attempting ChangeView to - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            horizontalOffset, verticalOffset, zoomFactor);

            UIExecutor.Execute(() =>
            {
                viewChangedHandler = (object sender, ScrollViewerViewChangedEventArgs args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        var sv = sender as ScrollViewer;
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}, ExtentWidth={3}, ExtentHeight={4}",
                            sv.HorizontalOffset, sv.VerticalOffset, sv.ZoomFactor, sv.ExtentWidth, sv.ExtentHeight);
                        sv.ViewChanged -= viewChangedHandler;
                        viewChanged.Set();
                    }
                };

                scrollViewer.ViewChanged += viewChangedHandler;
                scrollViewer.ChangeView(horizontalOffset, verticalOffset, zoomFactor, disableAnimations);
            });

            WaitForEvent("Waiting for view change completion", viewChanged);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}, ExtentWidth={3}, ExtentHeight={4}",
                    scrollViewer.HorizontalOffset, scrollViewer.VerticalOffset, scrollViewer.ZoomFactor, scrollViewer.ExtentWidth, scrollViewer.ExtentHeight);

                if(horizontalOffset != null)
                {
                    Verify.AreEqual(horizontalOffset, scrollViewer.HorizontalOffset);
                }

                if (verticalOffset != null)
                {
                    Verify.AreEqual(verticalOffset, scrollViewer.VerticalOffset);
                }

                if (zoomFactor != null)
                {
                    Verify.AreEqual(zoomFactor, scrollViewer.ZoomFactor);
                }
            });
        }

        private void WaitForEvent(string logComment, EventWaitHandle eventWaitHandle)
        {
            Log.Comment(logComment);

            if (global::System.Diagnostics.Debugger.IsAttached)
            {
               // Avoid timing out while debugging.
               eventWaitHandle.WaitOne();
            }
            else if (!eventWaitHandle.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration)))
            {
                throw new Exception("Timeout expiration in WaitForEvent.");
            }
        }

        private const int c_MaxWaitDuration = 5000;
        private const double c_defaultAnchoringUIscrollViewerNonConstrainedSize = 600.0;
        private const double c_defaultAnchoringUIscrollViewerConstrainedSize = 300.0;
        private const int c_defaultAnchoringUIStackPanelChildrenCount = 16;
        private const int c_defaultAnchoringUIRepeaterChildrenCount = 16;
    }
}
