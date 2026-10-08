// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Private.Controls;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    // Validates the anchoring surface exposed by ItemsRepeaterScrollHost: the public CurrentAnchor,
    // HorizontalAnchorRatio and VerticalAnchorRatio APIs, and the IRepeaterScrollingSurface interface it implements.
    // The scroll host content is a StackPanel of fixed-size Borders so that each candidate's offset is known
    // independently of the product: candidate i is located at Y = i * c_itemHeight.
    [TestClass]
    public class ItemsRepeaterScrollHostTests : ApiTestBase
    {
        private const double c_itemHeight = 100.0;
        private const double c_viewportWidth = 200.0;
        private const double c_viewportHeight = 300.0;
        private const int c_itemCount = 10;

        // Scenario: create an ItemsRepeaterScrollHost and set HorizontalAnchorRatio and VerticalAnchorRatio to several
        //           values, including NaN.
        // Expected: both ratios default to 0 and read back exactly the value that was set.
        // A failure means: apps cannot rely on the anchor ratios they configure to scroll anchoring.
        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalAnchorRatio and VerticalAnchorRatio default to 0 and round-trip set values, including NaN.")]
        public void VerifyAnchorRatioPropertiesRoundTrip()
        {
            RunOnUIThread.Execute(() =>
            {
                var scrollHost = new ItemsRepeaterScrollHost();

                Verify.AreEqual(0.0, scrollHost.HorizontalAnchorRatio);
                Verify.AreEqual(0.0, scrollHost.VerticalAnchorRatio);

                scrollHost.HorizontalAnchorRatio = 0.5;
                scrollHost.VerticalAnchorRatio = 1.0;
                Verify.AreEqual(0.5, scrollHost.HorizontalAnchorRatio);
                Verify.AreEqual(1.0, scrollHost.VerticalAnchorRatio);

                scrollHost.HorizontalAnchorRatio = double.NaN;
                scrollHost.VerticalAnchorRatio = double.NaN;
                Verify.IsTrue(double.IsNaN(scrollHost.HorizontalAnchorRatio));
                Verify.IsTrue(double.IsNaN(scrollHost.VerticalAnchorRatio));
            });
        }

        // Scenario: assign a ScrollViewer to the host, then assign a second ScrollViewer.
        // Expected: the host shows exactly one child, the current ScrollViewer, and the previous one is removed from
        //           the tree.
        // A failure means: the host could show two scroll viewers or keep a stale one, breaking scrolling of the
        //                  repeater.
        [TestMethod]
        [TestProperty("Description", "Verifies that setting ScrollViewer hosts it as the only child and that a new ScrollViewer replaces the previous one.")]
        public void VerifyScrollViewerPropertyReplacesChild()
        {
            RunOnUIThread.Execute(() =>
            {
                var scrollHost = new ItemsRepeaterScrollHost();
                Verify.IsNull(scrollHost.ScrollViewer);
                Content = scrollHost;

                var scrollViewer1 = new ScrollViewer();
                var scrollViewer2 = new ScrollViewer();

                scrollHost.ScrollViewer = scrollViewer1;
                Content.UpdateLayout();
                Verify.AreSame(scrollViewer1, scrollHost.ScrollViewer);
                Verify.AreEqual(1, VisualTreeHelper.GetChildrenCount(scrollHost));
                Verify.AreSame(scrollHost, VisualTreeHelper.GetParent(scrollViewer1));

                scrollHost.ScrollViewer = scrollViewer2;
                Content.UpdateLayout();
                Verify.AreSame(scrollViewer2, scrollHost.ScrollViewer);
                Verify.AreEqual(1, VisualTreeHelper.GetChildrenCount(scrollHost));
                Verify.AreSame(scrollViewer2, VisualTreeHelper.GetChild(scrollHost, 0));
                Verify.IsNull(VisualTreeHelper.GetParent(scrollViewer1));
            });
        }

        // Scenario: register several stacked items as anchor candidates with both anchor ratios set to 0, 0.5 and 1.
        // Expected: CurrentAnchor (and AnchorElement) is the candidate whose ratio point is closest to the same point
        //           of the viewport.
        // A failure means: scrolled content can jump when items above the viewport change size, because the wrong item
        //                  is used as anchor.
        [TestMethod]
        [TestProperty("Description", "Verifies that CurrentAnchor is the registered candidate whose anchor-ratio edge is closest to the same edge of the viewport.")]
        public void VerifyCurrentAnchorIsCandidateClosestToViewportEdge()
        {
            // ratio -> expected anchor index. The viewport edge is at ratio * viewportHeight and the candidate edge is
            // at i * itemHeight + ratio * itemHeight, so 0 -> item 0 (edge 0), 0.5 -> item 1 (edge 150), 1 -> item 2 (edge 300).
            // Note (PC-SCROLLHOST-ANCHOR-AXIS): ItemsRepeaterScrollHost currently computes the vertical anchor edge from
            // HorizontalAnchorRatio. Both ratios are set to the same value so these expectations hold with or without a fix.
            var expectations = new Dictionary<double, int>() { { 0.0, 0 }, { 0.5, 1 }, { 1.0, 2 } };

            foreach (var expectation in expectations)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"AnchorRatio={expectation.Key}, expected anchor index={expectation.Value}");
                    var scrollHost = CreateScrollHost(out var scrollViewer, out var items);
                    SetAnchorRatios(scrollHost, expectation.Key);
                    var surface = AsScrollingSurface(scrollHost);

                    Verify.IsNull(scrollHost.CurrentAnchor, "No anchor is expected before any candidate is registered.");

                    foreach (var item in items)
                    {
                        surface.RegisterAnchorCandidate(item);
                    }

                    Verify.AreSame(items[expectation.Value], scrollHost.CurrentAnchor);
                    Verify.AreSame(items[expectation.Value], surface.AnchorElement, "AnchorElement must match CurrentAnchor.");
                    Verify.IsTrue(surface.IsHorizontallyScrollable);
                    Verify.IsTrue(surface.IsVerticallyScrollable);

                    Content = null;
                });
            }
        }

        // Scenario: register six anchor candidates, then unregister the anchor, an unknown element and the rest.
        // Expected: the anchor moves to the next closest candidate, unknown elements are ignored, and finally no anchor
        //           remains.
        // A failure means: removed items could stay the scroll anchor and cause wrong scroll offsets.
        [TestMethod]
        [TestProperty("Description", "Verifies that unregistering the current anchor recomputes the anchor and that unregistering unknown elements has no effect.")]
        public void VerifyUnregisterAnchorCandidateRecomputesAnchor()
        {
            RunOnUIThread.Execute(() =>
            {
                var scrollHost = CreateScrollHost(out var scrollViewer, out var items);
                SetAnchorRatios(scrollHost, 0.5);
                var surface = AsScrollingSurface(scrollHost);

                for (int i = 0; i < 6; i++)
                {
                    surface.RegisterAnchorCandidate(items[i]);
                }

                // Viewport edge is at 150, item 1's center is at 150.
                Verify.AreSame(items[1], scrollHost.CurrentAnchor);

                // Items 0 (center 50) and 2 (center 250) are now equidistant; the first registered candidate wins.
                surface.UnregisterAnchorCandidate(items[1]);
                Verify.AreSame(items[0], scrollHost.CurrentAnchor);

                // Unregistering an element that was never registered must not change the anchor.
                surface.UnregisterAnchorCandidate(new Border());
                surface.UnregisterAnchorCandidate(items[1]);
                Verify.AreSame(items[0], scrollHost.CurrentAnchor);

                surface.UnregisterAnchorCandidate(items[0]);
                Verify.AreSame(items[2], scrollHost.CurrentAnchor);

                for (int i = 2; i < 6; i++)
                {
                    surface.UnregisterAnchorCandidate(items[i]);
                }

                Verify.IsNull(scrollHost.CurrentAnchor, "No anchor is expected once all candidates are unregistered.");
            });
        }

        // Scenario: set both anchor ratios to NaN and register anchor candidates.
        // Expected: CurrentAnchor stays null, i.e. scroll anchoring is turned off.
        // A failure means: apps that disable anchoring with NaN would still get anchoring scroll adjustments.
        [TestMethod]
        [TestProperty("Description", "Verifies that a NaN anchor ratio produces no anchor even when candidates are registered.")]
        public void VerifyNaNAnchorRatioYieldsNoAnchor()
        {
            RunOnUIThread.Execute(() =>
            {
                var scrollHost = CreateScrollHost(out var scrollViewer, out var items);
                SetAnchorRatios(scrollHost, double.NaN);
                var surface = AsScrollingSurface(scrollHost);

                foreach (var item in items)
                {
                    surface.RegisterAnchorCandidate(item);
                }

                Verify.IsNull(scrollHost.CurrentAnchor);
            });
        }

        // Scenario: query GetRelativeViewport for items before and after scrolling the ScrollViewer down by 200px.
        // Expected: the relative viewport reflects the scroll offset and the anchor is the item at the top of the
        //           scrolled viewport.
        // A failure means: the repeater would realize the wrong items or pick the wrong anchor after the user scrolls.
        [TestMethod]
        [TestProperty("Description", "Verifies the anchor selection and GetRelativeViewport account for the ScrollViewer vertical offset.")]
        public void VerifyAnchorAndRelativeViewportUseScrollOffset()
        {
            ItemsRepeaterScrollHost scrollHost = null;
            ScrollViewer scrollViewer = null;
            List<Border> items = null;
            var viewChangedEvent = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                scrollHost = CreateScrollHost(out scrollViewer, out items);
                SetAnchorRatios(scrollHost, 0.0);
                var surface = AsScrollingSurface(scrollHost);

                // Unscrolled: item 3 starts 300px below the top of the viewport.
                VerifyRect(new Rect(0, -300, c_viewportWidth, c_viewportHeight), surface.GetRelativeViewport(items[3]), "unscrolled item 3");
                VerifyRect(new Rect(0, 0, c_viewportWidth, c_viewportHeight), surface.GetRelativeViewport(items[0]), "unscrolled item 0");

                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChangedEvent.Set();
                    }
                };

                Verify.IsTrue(scrollViewer.ChangeView(null, 200.0, null, disableAnimation: true));
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(DefaultWaitTimeInMS), "Waiting for ViewChanged.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(200.0, scrollViewer.VerticalOffset);
                var surface = AsScrollingSurface(scrollHost);

                // Item 3 is now 100px below the top of the viewport, item 1 is 100px above it.
                VerifyRect(new Rect(0, -100, c_viewportWidth, c_viewportHeight), surface.GetRelativeViewport(items[3]), "scrolled item 3");
                VerifyRect(new Rect(0, 100, c_viewportWidth, c_viewportHeight), surface.GetRelativeViewport(items[1]), "scrolled item 1");

                // With a 0 ratio the viewport top edge (offset 200) coincides with item 2's top edge.
                foreach (var item in items)
                {
                    surface.RegisterAnchorCandidate(item);
                }

                Verify.AreSame(items[2], scrollHost.CurrentAnchor);
            });
        }

        // Scenario: use the host as a scrolling surface before any ScrollViewer is assigned.
        // Expected: anchor candidates are ignored, there is no anchor and the relative viewport is empty, without any
        //           exception.
        // A failure means: a host without a ScrollViewer could crash or report a bogus viewport to the repeater.
        [TestMethod]
        [TestProperty("Description", "Verifies the scrolling surface behavior when no ScrollViewer is hosted: candidates are ignored and the relative viewport is empty.")]
        public void VerifyScrollingSurfaceWithoutScrollViewer()
        {
            RunOnUIThread.Execute(() =>
            {
                var scrollHost = new ItemsRepeaterScrollHost() { Width = c_viewportWidth, Height = c_viewportHeight };
                var element = new Border() { Width = 50, Height = 50 };
                Content = scrollHost;
                Content.UpdateLayout();

                var surface = AsScrollingSurface(scrollHost);
                Verify.IsNull(scrollHost.ScrollViewer);
                Verify.IsTrue(surface.IsHorizontallyScrollable);
                Verify.IsTrue(surface.IsVerticallyScrollable);

                surface.RegisterAnchorCandidate(element);
                Verify.IsNull(scrollHost.CurrentAnchor);
                Verify.IsNull(surface.AnchorElement);
                VerifyRect(new Rect(0, 0, 0, 0), surface.GetRelativeViewport(element), "no ScrollViewer");

                // The candidate was not recorded, so hosting a ScrollViewer afterwards must not surface it as an anchor.
                var scrollViewer = new ScrollViewer() { Content = new StackPanel() };
                scrollHost.ScrollViewer = scrollViewer;
                Content.UpdateLayout();
                Verify.IsNull(scrollHost.CurrentAnchor);
            });
        }

        // Scenario: subscribe to the ViewportChanged, PostArrange and ConfigurationChanged events of the host's scrolling
        //           surface, scroll and re-arrange the host, then unsubscribe.
        // Expected: subscribing and unsubscribing succeed, and none of the three events is raised while subscribed: the host
        //           accepts ConfigurationChanged as a no-op, never raises PostArrange, and raises ViewportChanged only from
        //           code paths that nothing calls (see untestable.md).
        // A failure means: the scroll host started raising these events, so components that track it must be revalidated.
        [TestMethod]
        [TestProperty("Description", "Verifies the ItemsRepeaterScrollHost scrolling-surface events accept handler registration and removal and are not raised.")]
        public void VerifyScrollingSurfaceEventsCanBeSubscribedAndRemoved()
        {
            ItemsRepeaterScrollHost scrollHost = null;
            ScrollViewer scrollViewer = null;
            var viewChangedEvent = new ManualResetEvent(false);
            int viewportChangedCount = 0, postArrangeCount = 0, configurationChangedCount = 0;
            ViewportChangedEventHandler viewportChanged = (sender, isFinal) => viewportChangedCount++;
            PostArrangeEventHandler postArrange = (sender) => postArrangeCount++;
            ConfigurationChangedEventHandler configurationChanged = (sender) => configurationChangedCount++;

            RunOnUIThread.Execute(() =>
            {
                scrollHost = CreateScrollHost(out scrollViewer, out var items);
                var surface = AsScrollingSurface(scrollHost);
                surface.ViewportChanged += viewportChanged;
                surface.PostArrange += postArrange;
                surface.ConfigurationChanged += configurationChanged;
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChangedEvent.Set();
                    }
                };
                Verify.IsTrue(scrollViewer.ChangeView(null, 150.0, null, disableAnimation: true));
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(DefaultWaitTimeInMS), "Waiting for ViewChanged.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Re-arrange while still subscribed: an arrange pass is where PostArrange would be raised.
                scrollHost.InvalidateArrange();
                scrollHost.UpdateLayout();
                Log.Comment($"Raised while subscribed: ViewportChanged={viewportChangedCount}, PostArrange={postArrangeCount}, ConfigurationChanged={configurationChangedCount}");
                Verify.AreEqual(0, viewportChangedCount, "The scroll host does not raise ViewportChanged during a scroll and layout.");
                Verify.AreEqual(0, postArrangeCount, "The scroll host does not raise PostArrange.");
                Verify.AreEqual(0, configurationChangedCount, "The scroll host does not report configuration changes.");
                var surface = AsScrollingSurface(scrollHost);
                surface.ViewportChanged -= viewportChanged;
                surface.PostArrange -= postArrange;
                surface.ConfigurationChanged -= configurationChanged;
                Verify.AreEqual(150.0, scrollViewer.VerticalOffset);
            });
        }

        private ItemsRepeaterScrollHost CreateScrollHost(out ScrollViewer scrollViewer, out List<Border> items)
        {
            items = Enumerable.Range(0, c_itemCount).Select(i => new Border() {
                Width = c_viewportWidth,
                Height = c_itemHeight,
                Tag = i
            }).ToList();

            var panel = new StackPanel() { Width = c_viewportWidth };
            foreach (var item in items)
            {
                panel.Children.Add(item);
            }

            scrollViewer = new ScrollViewer() {
                Content = panel,
                HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled,
                VerticalScrollBarVisibility = ScrollBarVisibility.Hidden,
            };

            var scrollHost = new ItemsRepeaterScrollHost() {
                Width = c_viewportWidth,
                Height = c_viewportHeight,
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top,
                ScrollViewer = scrollViewer
            };

            Content = scrollHost;
            Content.UpdateLayout();

            Verify.AreEqual(c_viewportHeight, scrollViewer.ViewportHeight);
            Verify.AreEqual(c_viewportWidth, scrollViewer.ViewportWidth);
            Verify.AreEqual(c_itemHeight * c_itemCount, scrollViewer.ExtentHeight);

            return scrollHost;
        }

        // Sets both anchor ratios to the same value. The product currently derives the vertical anchor edge from
        // HorizontalAnchorRatio (PC-SCROLLHOST-ANCHOR-AXIS); setting both keeps the tests valid if that is fixed.
        private static void SetAnchorRatios(ItemsRepeaterScrollHost scrollHost, double ratio)
        {
            scrollHost.HorizontalAnchorRatio = ratio;
            scrollHost.VerticalAnchorRatio = ratio;
        }

        private static IRepeaterScrollingSurface AsScrollingSurface(ItemsRepeaterScrollHost scrollHost)
        {
            var surface = ((object)scrollHost) as IRepeaterScrollingSurface;
            Verify.IsNotNull(surface, "ItemsRepeaterScrollHost is expected to implement IRepeaterScrollingSurface.");
            return surface;
        }

        private static void VerifyRect(Rect expected, Rect actual, string context)
        {
            Log.Comment($"{context}: expected {expected}, actual {actual}");
            Verify.IsLessThan(Math.Abs(expected.X - actual.X), 0.5, context + " X");
            Verify.IsLessThan(Math.Abs(expected.Y - actual.Y), 0.5, context + " Y");
            Verify.IsLessThan(Math.Abs(expected.Width - actual.Width), 0.5, context + " Width");
            Verify.IsLessThan(Math.Abs(expected.Height - actual.Height), 0.5, context + " Height");
        }
    }
}
