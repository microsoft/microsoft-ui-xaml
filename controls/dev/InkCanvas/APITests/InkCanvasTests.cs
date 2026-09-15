// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Markup;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using InkPresenter = global::Microsoft.UI.Xaml.Controls.InkPresenter;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // The InkCanvas public surface intentionally mirrors the classic
    // Windows.UI.Xaml.Controls.InkCanvas exactly: a default constructor plus a
    // single read-only InkPresenter property. All ink configuration flows through
    // the InkPresenter, so these tests only validate construction and that the
    // control participates in the visual tree.
    [TestClass]
    public class InkCanvasTests : ApiTestBase
    {
        [TestMethod]
        public void InkCanvasConstructionTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();
                Verify.IsNotNull(inkCanvas, "InkCanvas should construct without throwing.");
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")]
        public void InkCanvasInVisualTreeTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <controls:InkCanvas x:Name='TestInkCanvas'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            Width='400' Height='300' />
                    </Grid>");

                Content = root;
                Content.UpdateLayout();

                var inkCanvas = (InkCanvas)root.FindName("TestInkCanvas");
                Verify.IsNotNull(inkCanvas, "InkCanvas should be found in visual tree.");
                Verify.AreEqual(400.0, inkCanvas.Width, "Width should be 400.");
                Verify.AreEqual(300.0, inkCanvas.Height, "Height should be 300.");
            });
        }

        [TestMethod]
        public void InkCanvasInkPresenterAccessorTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // The InkPresenter is created on a dedicated ink thread and may not be
                // available synchronously right after construction. The accessor must
                // simply be callable without throwing.
                InkPresenter presenter = inkCanvas.InkPresenter;
                Log.Comment("InkPresenter getter is accessible (value may be null before the ink thread initializes).");
            });
        }

        // The canvas renders through a composition visual rather than XAML children, so the peer has
        // to supply bounds itself. Without that it reports an empty rect and offscreen, and assistive
        // technology skips the canvas entirely.
        [TestMethod]
        public void InkCanvasAutomationPeerReportsBounds()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas { Width = 400, Height = 300 };

                Content = inkCanvas;
                Content.UpdateLayout();

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(inkCanvas);
                Verify.IsNotNull(peer, "InkCanvas should create an automation peer.");

                var bounds = peer.GetBoundingRectangle();
                Log.Comment($"Bounding rectangle: {bounds.X},{bounds.Y} {bounds.Width}x{bounds.Height}");

                Verify.IsGreaterThan(bounds.Width, 0.0, "Bounding rectangle should have a non-zero width.");
                Verify.IsGreaterThan(bounds.Height, 0.0, "Bounding rectangle should have a non-zero height.");
                Verify.IsFalse(peer.IsOffscreen(), "A visible InkCanvas should not report itself as offscreen.");
            });
        }

        // The positive test above only proves the peer stopped reporting a blanket "offscreen";
        // these are the cases where it still has to say true, so that assistive technology keeps
        // skipping a canvas the user genuinely cannot reach.
        [TestMethod]
        public void InkCanvasAutomationPeerReportsOffscreen()
        {
            RunOnUIThread.Execute(() =>
            {
                var collapsedCanvas = new InkCanvas
                {
                    Width = 400,
                    Height = 300,
                    Visibility = Visibility.Collapsed
                };

                Content = collapsedCanvas;
                Content.UpdateLayout();

                var collapsedPeer = FrameworkElementAutomationPeer.CreatePeerForElement(collapsedCanvas);
                Verify.IsNotNull(collapsedPeer, "InkCanvas should create an automation peer.");
                Verify.IsTrue(collapsedPeer.IsOffscreen(), "A collapsed InkCanvas should report itself as offscreen.");

                // Positioned far past the right edge of the content area, so no part of it intersects
                // the XamlRoot - this is the "beyond the app boundary" case.
                var offscreenCanvas = new InkCanvas { Width = 400, Height = 300 };
                var canvasHost = new Canvas();
                canvasHost.Children.Add(offscreenCanvas);
                Canvas.SetLeft(offscreenCanvas, 20000);

                Content = canvasHost;
                Content.UpdateLayout();

                var offscreenPeer = FrameworkElementAutomationPeer.CreatePeerForElement(offscreenCanvas);
                Verify.IsNotNull(offscreenPeer, "InkCanvas should create an automation peer.");

                var offscreenBounds = offscreenPeer.GetBoundingRectangle();
                Log.Comment($"Offscreen bounding rectangle: {offscreenBounds.X},{offscreenBounds.Y} {offscreenBounds.Width}x{offscreenBounds.Height}");
                Verify.IsTrue(offscreenPeer.IsOffscreen(), "An InkCanvas positioned outside the content area should report itself as offscreen.");
            });
        }

        // A canvas is routinely larger than the viewport that scrolls it. The reported rectangle has
        // to be the visible part, not the full layout size, otherwise assistive technology highlights
        // an area far bigger than the user can actually see.
        [TestMethod]
        public void InkCanvasAutomationPeerClipsBoundsToScrollViewport()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas { Width = 400, Height = 2000 };
                var scrollViewer = new ScrollViewer
                {
                    Width = 300,
                    Height = 200,
                    Content = inkCanvas
                };

                Content = scrollViewer;
                Content.UpdateLayout();

                var canvasPeer = FrameworkElementAutomationPeer.CreatePeerForElement(inkCanvas);
                var scrollPeer = FrameworkElementAutomationPeer.CreatePeerForElement(scrollViewer);
                Verify.IsNotNull(canvasPeer, "InkCanvas should create an automation peer.");
                Verify.IsNotNull(scrollPeer, "ScrollViewer should create an automation peer.");

                var canvasBounds = canvasPeer.GetBoundingRectangle();
                var scrollBounds = scrollPeer.GetBoundingRectangle();
                Log.Comment($"Canvas bounds: {canvasBounds.Width}x{canvasBounds.Height}, viewport bounds: {scrollBounds.Width}x{scrollBounds.Height}");

                // Both rectangles come back in the same physical units, so comparing them keeps this
                // assertion independent of the display scale the test happens to run at.
                Verify.IsLessThanOrEqual(canvasBounds.Height, scrollBounds.Height + 1.0, "Reported height should be clipped to the scroll viewport.");
                Verify.IsLessThanOrEqual(canvasBounds.Width, scrollBounds.Width + 1.0, "Reported width should be clipped to the scroll viewport.");
                Verify.IsGreaterThan(canvasBounds.Height, 0.0, "A partially visible canvas should still report bounds.");
                Verify.IsFalse(canvasPeer.IsOffscreen(), "A partially visible InkCanvas should not report itself as offscreen.");
            });
        }

        // The clip case Harshit asked about: the canvas is inside the tree and visible by its own
        // Visibility, but an ancestor clip excludes it entirely, so it must report offscreen.
        [TestMethod]
        public void InkCanvasAutomationPeerReportsOffscreenWhenClippedOut()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas { Width = 400, Height = 300 };
                var host = new Grid
                {
                    Width = 400,
                    Height = 300,
                    // Clip to a region the canvas cannot reach.
                    Clip = new RectangleGeometry { Rect = new Windows.Foundation.Rect(0, 0, 0, 0) }
                };

                host.Children.Add(inkCanvas);

                Content = host;
                Content.UpdateLayout();

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(inkCanvas);
                Verify.IsNotNull(peer, "InkCanvas should create an automation peer.");

                var bounds = peer.GetBoundingRectangle();
                Log.Comment($"Clipped bounding rectangle: {bounds.X},{bounds.Y} {bounds.Width}x{bounds.Height}");

                Verify.IsTrue(peer.IsOffscreen(), "An InkCanvas clipped out by an ancestor should report itself as offscreen.");
                Verify.AreEqual(0.0, bounds.Width, "An offscreen InkCanvas should report an empty rectangle.");
                Verify.AreEqual(0.0, bounds.Height, "An offscreen InkCanvas should report an empty rectangle.");
            });
        }

        [TestMethod]
        public void InkCanvasMultipleInstancesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var canvas1 = new InkCanvas();
                var canvas2 = new InkCanvas();

                Verify.IsNotNull(canvas1, "Canvas1 should construct.");
                Verify.IsNotNull(canvas2, "Canvas2 should construct.");
                Verify.AreNotSame(canvas1, canvas2, "Instances should be independent.");
            });
        }

        // The following tests exercise the InkCanvas weak-reference / callback lifetime behavior.
        //
        // In OnLoaded, InkCanvas wires up XamlRoot.Changed and SizeChanged handlers that capture a
        // weak reference to itself. Those weak references are created through the safe
        // make_weak(static_cast<winrt::InkCanvas>(*this)) pattern (rather than the projected get_weak())
        // to avoid the C++/WinRT over-release bug (cppwinrt #1431) for composed/aggregated objects.
        //
        // TestInkCanvas (a managed subclass, below) forces exactly that composed/aggregation scenario:
        // the CLR creates an outer object that aggregates the native InkCanvas across a module boundary,
        // which is where the projected get_weak() bug would manifest. These tests load and then destroy
        // such instances while the callbacks may still be pending and verify no crash / refcount imbalance.

        // Managed subclass so the native InkCanvas is composed/aggregated by an outer object in another
        // module - the exact scenario affected by the get_weak() over-release bug.
        public class TestInkCanvas : InkCanvas
        {
        }

        [TestMethod]
        [TestProperty("Ignore", "True")]
        public void InkCanvasLoadUnloadDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                Content = new TestInkCanvas();
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Unload the InkCanvas. This revokes the XamlRoot.Changed and SizeChanged handlers
                // that were established during load.
                Content = null;
                Log.Comment("InkCanvas unloaded without crashing.");
            });

            IdleSynchronizer.Wait();
        }

        [TestMethod]
        public void InkCanvasDestroyedBeforePendingCallbacksDoesNotCrash()
        {
            // Create, load, and then immediately drop and destroy several aggregated InkCanvas instances.
            // Destroying the instance while load-time callbacks may still be pending exercises the
            // weak-capture-fails-safely path. If the weak reference had been created with the buggy
            // projected get_weak(), destroying the aggregated outer object here could over-release it and
            // lead to a use-after-free / access violation.
            for (int i = 0; i < 5; i++)
            {
                RunOnUIThread.Execute(() =>
                {
                    Content = new TestInkCanvas();
                    Content.UpdateLayout();

                    // Immediately remove from the tree while callbacks may still be pending.
                    Content = null;
                });

                IdleSynchronizer.Wait();

                // Force the managed wrapper (and, transitively, the native InkCanvas) to be collected so
                // that any callback that fires afterwards must safely observe a dropped weak reference.
                RunOnUIThread.Execute(() =>
                {
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    GC.Collect();
                });

                IdleSynchronizer.Wait();
            }

            Log.Comment("Repeated create/load/unload/collect cycles completed without crashing.");
        }
    }
}
