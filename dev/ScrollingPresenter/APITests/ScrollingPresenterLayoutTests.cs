// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Private.Controls;
using MUXControlsTestApp.Utilities;
using System;
using System.Numerics;
using System.Threading;
using Windows.UI.Composition;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Shapes;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollingPresenterTests
    {
        private enum BiDirectionalAlignment
        {
            Near,
            Center,
            Stretch,
            Far
        }

        [TestMethod]
        [TestProperty("Description", "Sets ScrollingPresenter.Content.Margin and verifies InteractionTracker.MaxPosition.")]
        public void BasicMargin()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1.");
                return;
            }

            const double c_Margin = 50.0;
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding positive Margin to ScrollingPresenter.Content");
                rectangleScrollingPresenterContent.Margin = new Thickness(c_Margin);
            });

            // Try to jump beyond maximum offsets
            ScrollTo(
                scrollingPresenter,
                c_defaultUIScrollingPresenterContentWidth + 2 * c_Margin - c_defaultUIScrollingPresenterWidth + 10.0,
                c_defaultUIScrollingPresenterContentHeight + 2 * c_Margin - c_defaultUIScrollingPresenterHeight + 10.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: c_defaultUIScrollingPresenterContentWidth + 2 * c_Margin - c_defaultUIScrollingPresenterWidth,
                expectedFinalVerticalOffset: c_defaultUIScrollingPresenterContentHeight + 2 * c_Margin - c_defaultUIScrollingPresenterHeight);

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding negative Margin to ScrollingPresenter.Content");
                rectangleScrollingPresenterContent.Margin = new Thickness(-c_Margin);
            });

            // Try to jump beyond maximum offsets
            ScrollTo(
                scrollingPresenter,
                c_defaultUIScrollingPresenterContentWidth - 2 * c_Margin - c_defaultUIScrollingPresenterWidth + 10.0,
                c_defaultUIScrollingPresenterContentHeight - 2 * c_Margin - c_defaultUIScrollingPresenterHeight + 10.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: c_defaultUIScrollingPresenterContentWidth - 2 * c_Margin - c_defaultUIScrollingPresenterWidth,
                expectedFinalVerticalOffset: c_defaultUIScrollingPresenterContentHeight - 2 * c_Margin - c_defaultUIScrollingPresenterHeight);
        }

        [TestMethod]
        [TestProperty("Description", "Sets ScrollingPresenter.Content.Padding and verifies InteractionTracker.MaxPosition.")]
        public void BasicPadding()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1 because test sometimes hangs indefinitely."); // Bug 12286203
                return;
            }

            const double c_Padding = 50.0;
            ScrollingPresenter scrollingPresenter = null;
            Border borderScrollingPresenterContent = null;
            Rectangle rectangle = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                borderScrollingPresenterContent = new Border();
                rectangle = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                borderScrollingPresenterContent.Width = c_defaultUIScrollingPresenterContentWidth;
                borderScrollingPresenterContent.Height = c_defaultUIScrollingPresenterContentHeight;
                borderScrollingPresenterContent.Child = rectangle;
                scrollingPresenter.Content = borderScrollingPresenterContent;

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent: null, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding Padding to ScrollingPresenter.Content");
                borderScrollingPresenterContent.Padding = new Thickness(c_Padding);
            });

            // Try to jump beyond maximum offsets
            ScrollTo(
                scrollingPresenter,
                c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth + 10.0,
                c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight + 10.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth,
                expectedFinalVerticalOffset: c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight);
        }

        [TestMethod]
        [TestProperty("Description", "Sets ScrollingPresenter.Content.HorizontalAlignment/VerticalAlignment and verifies content positioning.")]
        public void BasicAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.15f;
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);
            IdleSynchronizer.Wait();

            // Jump to absolute small zoomFactor to make the content smaller than the viewport.
            ZoomTo(scrollingPresenter, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Stretch/Strech alignments");
                Verify.AreEqual(HorizontalAlignment.Stretch, rectangleScrollingPresenterContent.HorizontalAlignment);
                Verify.AreEqual(VerticalAlignment.Stretch, rectangleScrollingPresenterContent.VerticalAlignment);
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - (float)(c_defaultUIScrollingPresenterWidth - c_defaultUIScrollingPresenterContentWidth * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - (float)(c_defaultUIScrollingPresenterHeight - c_defaultUIScrollingPresenterContentHeight * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Left/Top alignments");
                rectangleScrollingPresenterContent.HorizontalAlignment = HorizontalAlignment.Left;
                rectangleScrollingPresenterContent.VerticalAlignment = VerticalAlignment.Top;
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.AreEqual(0.0f, horizontalOffset);
            Verify.AreEqual(0.0f, verticalOffset);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Right/Bottom alignments");
                rectangleScrollingPresenterContent.HorizontalAlignment = HorizontalAlignment.Right;
                rectangleScrollingPresenterContent.VerticalAlignment = VerticalAlignment.Bottom;
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - (float)(c_defaultUIScrollingPresenterWidth - c_defaultUIScrollingPresenterContentWidth * c_smallZoomFactor)) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - (float)(c_defaultUIScrollingPresenterHeight - c_defaultUIScrollingPresenterContentHeight * c_smallZoomFactor)) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Center/Center alignments");
                rectangleScrollingPresenterContent.HorizontalAlignment = HorizontalAlignment.Center;
                rectangleScrollingPresenterContent.VerticalAlignment = VerticalAlignment.Center;
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - (float)(c_defaultUIScrollingPresenterWidth - c_defaultUIScrollingPresenterContentWidth * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - (float)(c_defaultUIScrollingPresenterHeight - c_defaultUIScrollingPresenterContentHeight * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);
        }

        [TestMethod]
        [TestProperty("Description", "Validates ScrollingPresenter.ViewportHeight for various layouts.")]
        public void ViewportHeight()
        {
            for (double scrollingPresenterContentHeight = 50.0; scrollingPresenterContentHeight <= 350.0; scrollingPresenterContentHeight += 300.0)
            {
                ViewportHeight(
                    isScrollingPresenterParentSizeSet: false,
                    isScrollingPresenterParentMaxSizeSet: false,
                    scrollingPresenterVerticalAlignment: VerticalAlignment.Top,
                    scrollingPresenterContentHeight: scrollingPresenterContentHeight);
                ViewportHeight(
                    isScrollingPresenterParentSizeSet: true,
                    isScrollingPresenterParentMaxSizeSet: false,
                    scrollingPresenterVerticalAlignment: VerticalAlignment.Top,
                    scrollingPresenterContentHeight: scrollingPresenterContentHeight);
                ViewportHeight(
                    isScrollingPresenterParentSizeSet: false,
                    isScrollingPresenterParentMaxSizeSet: true,
                    scrollingPresenterVerticalAlignment: VerticalAlignment.Top,
                    scrollingPresenterContentHeight: scrollingPresenterContentHeight);

                ViewportHeight(
                    isScrollingPresenterParentSizeSet: false,
                    isScrollingPresenterParentMaxSizeSet: false,
                    scrollingPresenterVerticalAlignment: VerticalAlignment.Center,
                    scrollingPresenterContentHeight: scrollingPresenterContentHeight);
                ViewportHeight(
                    isScrollingPresenterParentSizeSet: true,
                    isScrollingPresenterParentMaxSizeSet: false,
                    scrollingPresenterVerticalAlignment: VerticalAlignment.Center,
                    scrollingPresenterContentHeight: scrollingPresenterContentHeight);
                ViewportHeight(
                    isScrollingPresenterParentSizeSet: false,
                    isScrollingPresenterParentMaxSizeSet: true,
                    scrollingPresenterVerticalAlignment: VerticalAlignment.Center,
                    scrollingPresenterContentHeight: scrollingPresenterContentHeight);

                ViewportHeight(
                    isScrollingPresenterParentSizeSet: false,
                    isScrollingPresenterParentMaxSizeSet: false,
                    scrollingPresenterVerticalAlignment: VerticalAlignment.Stretch,
                    scrollingPresenterContentHeight: scrollingPresenterContentHeight);
                ViewportHeight(
                    isScrollingPresenterParentSizeSet: true,
                    isScrollingPresenterParentMaxSizeSet: false,
                    scrollingPresenterVerticalAlignment: VerticalAlignment.Stretch,
                    scrollingPresenterContentHeight: scrollingPresenterContentHeight);
                ViewportHeight(
                    isScrollingPresenterParentSizeSet: false,
                    isScrollingPresenterParentMaxSizeSet: true,
                    scrollingPresenterVerticalAlignment: VerticalAlignment.Stretch,
                    scrollingPresenterContentHeight: scrollingPresenterContentHeight);
            }
        }

        private void ViewportHeight(
            bool isScrollingPresenterParentSizeSet,
            bool isScrollingPresenterParentMaxSizeSet,
            VerticalAlignment scrollingPresenterVerticalAlignment,
            double scrollingPresenterContentHeight)
        {
            Log.Comment($"ViewportHeight test case - isScrollingPresenterParentSizeSet: {isScrollingPresenterParentSizeSet}, isScrollingPresenterParentMaxSizeSet: {isScrollingPresenterParentMaxSizeSet}, scrollingPresenterVerticalAlignment: {scrollingPresenterVerticalAlignment}, scrollingPresenterContentHeight: {scrollingPresenterContentHeight}");

            Border border = null;
            ScrollingPresenter scrollingPresenter = null;
            StackPanel stackPanel = null;
            Rectangle rectangle = null;
            AutoResetEvent borderLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangle = new Rectangle()
                {
                    Width = 30,
                    Height = scrollingPresenterContentHeight
                };

                stackPanel = new StackPanel()
                {
                    BorderThickness = new Thickness(5),
                    Margin = new Thickness(7),
                    VerticalAlignment = VerticalAlignment.Top
                };
                stackPanel.Children.Add(rectangle);

                scrollingPresenter = new ScrollingPresenter()
                {
                    Content = stackPanel,
                    ContentOrientation = ContentOrientation.Vertical,
                    VerticalAlignment = scrollingPresenterVerticalAlignment
                };

                border = new Border()
                {
                    BorderThickness = new Thickness(2),
                    Margin = new Thickness(3),
                    VerticalAlignment = VerticalAlignment.Center,
                    Child = scrollingPresenter
                };
                if (isScrollingPresenterParentSizeSet)
                {
                    border.Width = 300.0;
                    border.Height = 200.0;
                }
                if (isScrollingPresenterParentMaxSizeSet)
                {
                    border.MaxWidth = 300.0;
                    border.MaxHeight = 200.0;
                }

                border.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Border.Loaded event handler");
                    borderLoadedEvent.Set();
                };

                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = border;
            });

            WaitForEvent("Waiting for Border.Loaded event", borderLoadedEvent);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                double expectedViewportHeight =
                    rectangle.Height + stackPanel.BorderThickness.Top + stackPanel.BorderThickness.Bottom +
                    stackPanel.Margin.Top + stackPanel.Margin.Bottom;

                double borderChildAvailableHeight = border.ActualHeight - border.BorderThickness.Top - border.BorderThickness.Bottom;

                if (expectedViewportHeight > borderChildAvailableHeight || scrollingPresenterVerticalAlignment == VerticalAlignment.Stretch)
                {
                    expectedViewportHeight = borderChildAvailableHeight;
                }

                Log.Comment($"border.ActualWidth: {border.ActualWidth}, border.ActualHeight: {border.ActualHeight}");
                Log.Comment($"Checking ViewportWidth - scrollingPresenter.ViewportWidth: {scrollingPresenter.ViewportWidth}, scrollingPresenter.ActualWidth: {scrollingPresenter.ActualWidth}");
                Verify.AreEqual(scrollingPresenter.ViewportWidth, scrollingPresenter.ActualWidth);

                Log.Comment($"Checking ViewportHeight - expectedViewportHeight: {expectedViewportHeight}, scrollingPresenter.ViewportHeight: {scrollingPresenter.ViewportHeight}, scrollingPresenter.ActualHeight: {scrollingPresenter.ActualHeight}");
                Verify.AreEqual(expectedViewportHeight, scrollingPresenter.ViewportHeight);
                Verify.AreEqual(scrollingPresenter.ViewportHeight, scrollingPresenter.ActualHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Uses a StackPanel with Stretch alignment as ScrollingPresenter.Content to verify it stretched to the size of the ScrollingPresenter.")]
        public void StretchAlignment()
        {
            ScrollingPresenter scrollingPresenter = null;
            StackPanel stackPanel = null;
            Rectangle rectangle = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangle = new Rectangle();
                rectangle.Height = c_defaultUIScrollingPresenterContentHeight;
                stackPanel = new StackPanel();
                stackPanel.Children.Add(rectangle);
                scrollingPresenter = new ScrollingPresenter();
                scrollingPresenter.Width = c_defaultUIScrollingPresenterWidth;
                scrollingPresenter.Height = c_defaultUIScrollingPresenterHeight;
                scrollingPresenter.Content = stackPanel;

                scrollingPresenter.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollingPresenter.Loaded event handler");
                    scrollingPresenterLoadedEvent.Set();
                };

                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scrollingPresenter;
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Checking Stretch/Strech alignments");
                Verify.AreEqual(HorizontalAlignment.Stretch, stackPanel.HorizontalAlignment);
                Verify.AreEqual(VerticalAlignment.Stretch, stackPanel.VerticalAlignment);

                Log.Comment("Checking StackPanel size");
                Verify.AreEqual(c_defaultUIScrollingPresenterWidth, stackPanel.ActualWidth);
                Verify.AreEqual(c_defaultUIScrollingPresenterContentHeight, stackPanel.ActualHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Sets ScrollingPresenter.Content.HorizontalAlignment/VerticalAlignment and ScrollingPresenter.Content.Margin and verifies content positioning.")]
        public void BasicMarginAndAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.15f;
            const double c_Margin = 40.0;
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);

                Log.Comment("Adding positive Margin to ScrollingPresenter.Content");
                rectangleScrollingPresenterContent.Margin = new Thickness(c_Margin);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);
            IdleSynchronizer.Wait();

            // Jump to absolute small zoomFactor to make the content smaller than the viewport.
            ZoomTo(scrollingPresenter, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Stretch/Strech alignments");
                Verify.AreEqual(HorizontalAlignment.Stretch, rectangleScrollingPresenterContent.HorizontalAlignment);
                Verify.AreEqual(VerticalAlignment.Stretch, rectangleScrollingPresenterContent.VerticalAlignment);

            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - 20.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - 15.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Left/Top alignments");
                rectangleScrollingPresenterContent.HorizontalAlignment = HorizontalAlignment.Left;
                rectangleScrollingPresenterContent.VerticalAlignment = VerticalAlignment.Top;
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset + 34.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset + 34.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Right/Bottom alignments");
                rectangleScrollingPresenterContent.HorizontalAlignment = HorizontalAlignment.Right;
                rectangleScrollingPresenterContent.VerticalAlignment = VerticalAlignment.Bottom;
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - 74.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - 64.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Center/Center alignments");
                rectangleScrollingPresenterContent.HorizontalAlignment = HorizontalAlignment.Center;
                rectangleScrollingPresenterContent.VerticalAlignment = VerticalAlignment.Center;
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - 20.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - 15.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);
        }

        [TestMethod]
        [TestProperty("Description", "Sets ScrollingPresenter.Content to an unsized and stretched Image, verifies resulting ScrollingPresenter extents.")]
        public void StretchedImage()
        {
            ScrollingPresenter scrollingPresenter = null;
            Image imageScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            const double margin = 10.0;

            RunOnUIThread.Execute(() =>
            {
                Uri uri = new Uri("ms-appx:/Assets/ingredient8.png");
                Verify.IsNotNull(uri);
                imageScrollingPresenterContent = new Image();
                imageScrollingPresenterContent.Source = new BitmapImage(uri);
                imageScrollingPresenterContent.Margin = new Thickness(margin);

                scrollingPresenter = new ScrollingPresenter();
                scrollingPresenter.Content = imageScrollingPresenterContent;

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent: null, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scrollingPresenter.ContentOrientation, ContentOrientation.None);
                // Image is unconstrained and stretches to largest square contained in the 300 x 200 viewport: 200 x 200.
                ValidateStretchedImageSize(
                    scrollingPresenter,
                    imageScrollingPresenterContent,
                    desiredSize: 80.0 + 2.0 * margin /*natural size + margins*/,
                    actualSize: c_defaultUIScrollingPresenterHeight - 2.0 * margin,
                    extentSize: c_defaultUIScrollingPresenterHeight);

                Log.Comment("Changing ScrollingPresenter.ContentOrientation to Vertical.");
                scrollingPresenter.ContentOrientation = ContentOrientation.Vertical;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scrollingPresenter.ContentOrientation, ContentOrientation.Vertical);
                // Image is constrained horizontally to 300 and stretches to the 300 x 300 square.
                ValidateStretchedImageSize(
                    scrollingPresenter,
                    imageScrollingPresenterContent,
                    desiredSize: c_defaultUIScrollingPresenterWidth,
                    actualSize: c_defaultUIScrollingPresenterWidth - 2.0 * margin,
                    extentSize: c_defaultUIScrollingPresenterWidth);

                Log.Comment("Changing ScrollingPresenter.ContentOrientation to Horizontal.");
                scrollingPresenter.ContentOrientation = ContentOrientation.Horizontal;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scrollingPresenter.ContentOrientation, ContentOrientation.Horizontal);
                // Image is constrained vertically to 200 and stretches to the 200 x 200 square.
                ValidateStretchedImageSize(
                    scrollingPresenter,
                    imageScrollingPresenterContent,
                    desiredSize: c_defaultUIScrollingPresenterHeight,
                    actualSize: c_defaultUIScrollingPresenterHeight - 2.0 * margin,
                    extentSize: c_defaultUIScrollingPresenterHeight);
            });
        }

        private void ValidateStretchedImageSize(
            ScrollingPresenter scrollingPresenter,
            Image imageScrollingPresenterContent,
            double desiredSize,
            double actualSize,
            double extentSize)
        {
            Log.Comment($"Sizes with ScrollingPresenter.ContentOrientation={scrollingPresenter.ContentOrientation}");
            Log.Comment($"Image DesiredSize=({imageScrollingPresenterContent.DesiredSize.Width} x {imageScrollingPresenterContent.DesiredSize.Height})");
            Log.Comment($"Image RenderSize=({imageScrollingPresenterContent.RenderSize.Width} x {imageScrollingPresenterContent.RenderSize.Height})");
            Log.Comment($"Image ActualSize=({imageScrollingPresenterContent.ActualWidth} x {imageScrollingPresenterContent.ActualHeight})");
            Log.Comment($"ScrollingPresenter ExtentSize=({scrollingPresenter.ExtentWidth} x {scrollingPresenter.ExtentHeight})");

            Verify.AreEqual(imageScrollingPresenterContent.DesiredSize.Width, desiredSize);
            Verify.AreEqual(imageScrollingPresenterContent.DesiredSize.Height, desiredSize);
            Verify.AreEqual(imageScrollingPresenterContent.RenderSize.Width, actualSize);
            Verify.AreEqual(imageScrollingPresenterContent.RenderSize.Height, actualSize);
            Verify.AreEqual(imageScrollingPresenterContent.ActualWidth, actualSize);
            Verify.AreEqual(imageScrollingPresenterContent.ActualHeight, actualSize);
            Verify.AreEqual(scrollingPresenter.ExtentWidth, extentSize);
            Verify.AreEqual(scrollingPresenter.ExtentHeight, extentSize);
        }

        [TestMethod]
        [TestProperty("Description", "Sets ScrollingPresenter.Content to Image with unnatural size and verifies InteractionTracker.MaxPosition.")]
        public void ImageWithUnnaturalSize()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1.");
                return;
            }

            const double c_UnnaturalImageWidth = 1200.0;
            const double c_UnnaturalImageHeight = 1000.0;
            ScrollingPresenter scrollingPresenter = null;
            Image imageScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                imageScrollingPresenterContent = new Image();
                scrollingPresenter = new ScrollingPresenter();

                Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                Verify.IsNotNull(uri);
                imageScrollingPresenterContent.Source = new BitmapImage(uri);
                imageScrollingPresenterContent.Width = c_UnnaturalImageWidth;
                imageScrollingPresenterContent.Height = c_UnnaturalImageHeight;
                scrollingPresenter.Content = imageScrollingPresenterContent;

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent: null, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            // Try to jump beyond maximum offsets
            ScrollTo(
                scrollingPresenter,
                c_UnnaturalImageWidth - c_defaultUIScrollingPresenterWidth + 10.0,
                c_UnnaturalImageHeight - c_defaultUIScrollingPresenterHeight + 10.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: c_UnnaturalImageWidth - c_defaultUIScrollingPresenterWidth,
                expectedFinalVerticalOffset: c_UnnaturalImageHeight - c_defaultUIScrollingPresenterHeight);
        }

        [TestMethod]
        [TestProperty("Description",
            "Sets ScrollingPresenter.ContentOrientation to Vertical and verifies Image positioning for Stretch alignment.")]
        public void BasicImageWithConstrainedWidth()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollingPresenter"))
            {
                const double c_imageHeight = 300.0;
                const double c_scrollingPresenterWidth = 200.0;
                ScrollingPresenter scrollingPresenter = null;
                Image imageScrollingPresenterContent = null;
                AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
                Compositor compositor = null;

                RunOnUIThread.Execute(() =>
                {
                    imageScrollingPresenterContent = new Image();
                    scrollingPresenter = new ScrollingPresenter();

                    Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                    Verify.IsNotNull(uri);
                    imageScrollingPresenterContent.Source = new BitmapImage(uri);
                    scrollingPresenter.Content = imageScrollingPresenterContent;
                    scrollingPresenter.Background = new Media.SolidColorBrush(Colors.Chartreuse);

                    SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent: null, scrollingPresenterLoadedEvent);

                // Constraining the Image width and making the ScrollingPresenter smaller than the Image
                imageScrollingPresenterContent.Height = c_imageHeight;
                    scrollingPresenter.ContentOrientation = ContentOrientation.Vertical;
                    scrollingPresenter.Width = c_scrollingPresenterWidth;
                    compositor = Window.Current.Compositor;
                });

                WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

                ValidateContentWithConstrainedWidth(
                    compositor,
                    scrollingPresenter,
                    content: imageScrollingPresenterContent,
                    horizontalAlignment: HorizontalAlignment.Stretch,
                    leftMargin: 0.0,
                    rightMargin: 0.0,
                    expectedMinPosition: 0.0f,
                    expectedZoomFactor: 1.0f);
            }
        }

        [TestMethod]
        [TestProperty("Description",
            "Sets ScrollingPresenter.ContentOrientation to Vertical and verifies Image positioning for various alignments and zoom factors.")]
        public void ImageWithConstrainedWidth()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.5f;
            const float c_largeZoomFactor = 2.0f;
            const double c_imageHeight = 300.0;
            const double c_scrollingPresenterWidth = 200.0;
            const double c_leftMargin = 20.0;
            const double c_rightMargin = 30.0;
            ScrollingPresenter scrollingPresenter = null;
            Image imageScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                imageScrollingPresenterContent = new Image();
                scrollingPresenter = new ScrollingPresenter();

                Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                Verify.IsNotNull(uri);
                imageScrollingPresenterContent.Source = new BitmapImage(uri);
                imageScrollingPresenterContent.Margin = new Thickness(c_leftMargin, 0, c_rightMargin, 0);
                scrollingPresenter.Content = imageScrollingPresenterContent;
                scrollingPresenter.Background = new Media.SolidColorBrush(Colors.Chartreuse);

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent: null, scrollingPresenterLoadedEvent);

                // Constraining the Image width and making the ScrollingPresenter smaller than the Image
                imageScrollingPresenterContent.Height = c_imageHeight;
                scrollingPresenter.ContentOrientation = ContentOrientation.Vertical;
                scrollingPresenter.Width = c_scrollingPresenterWidth;
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            ValidateContentWithConstrainedWidth(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                horizontalAlignment: HorizontalAlignment.Stretch,
                leftMargin: c_leftMargin,
                rightMargin: c_rightMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: 1.0f);

            // Jump to absolute small zoomFactor to make the content smaller than the viewport.
            ZoomTo(scrollingPresenter, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

            ValidateContentWithConstrainedWidth(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                horizontalAlignment: HorizontalAlignment.Stretch,
                leftMargin: c_leftMargin,
                rightMargin: c_rightMargin,
                expectedMinPosition: (float)(-c_scrollingPresenterWidth * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                horizontalAlignment: HorizontalAlignment.Left,
                leftMargin: c_leftMargin,
                rightMargin: c_rightMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                horizontalAlignment: HorizontalAlignment.Right,
                leftMargin: c_leftMargin,
                rightMargin: c_rightMargin,
                expectedMinPosition: (float)(-c_scrollingPresenterWidth * c_smallZoomFactor), // -100
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                horizontalAlignment: HorizontalAlignment.Center,
                leftMargin: c_leftMargin,
                rightMargin: c_rightMargin,
                expectedMinPosition: (float)(-c_scrollingPresenterWidth * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            // Jump to absolute large zoomFactor to make the content larger than the viewport.
            ZoomTo(scrollingPresenter, c_largeZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);

            ValidateContentWithConstrainedWidth(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                horizontalAlignment: HorizontalAlignment.Stretch,
                leftMargin: c_leftMargin,
                rightMargin: c_rightMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                horizontalAlignment: HorizontalAlignment.Left,
                leftMargin: c_leftMargin,
                rightMargin: c_rightMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                horizontalAlignment: HorizontalAlignment.Right,
                leftMargin: c_leftMargin,
                rightMargin: c_rightMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                horizontalAlignment: HorizontalAlignment.Center,
                leftMargin: c_leftMargin,
                rightMargin: c_rightMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);
        }

        [TestMethod]
        [TestProperty("Description",
            "Sets ScrollingPresenter.ContentOrientation to Horizontal and verifies Image positioning for Stretch alignment.")]
        public void BasicImageWithConstrainedHeight()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollingPresenter"))
            {
                const float c_smallZoomFactor = 0.5f;
                const double c_imageWidth = 250.0;
                ScrollingPresenter scrollingPresenter = null;
                Image imageScrollingPresenterContent = null;
                AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
                Compositor compositor = null;

                RunOnUIThread.Execute(() =>
                {
                    imageScrollingPresenterContent = new Image();
                    scrollingPresenter = new ScrollingPresenter();

                    Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                    Verify.IsNotNull(uri);
                    imageScrollingPresenterContent.Source = new BitmapImage(uri);
                    scrollingPresenter.Content = imageScrollingPresenterContent;
                    scrollingPresenter.Background = new Media.SolidColorBrush(Colors.Chartreuse);

                    SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent: null, scrollingPresenterLoadedEvent);

                // Constraining the Image height and making the ScrollingPresenter smaller than the Image
                imageScrollingPresenterContent.Width = c_imageWidth;
                    scrollingPresenter.ContentOrientation = ContentOrientation.Horizontal;
                    compositor = Window.Current.Compositor;
                });

                WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

                ValidateContentWithConstrainedHeight(
                    compositor,
                    scrollingPresenter,
                    content: imageScrollingPresenterContent,
                    verticalAlignment: VerticalAlignment.Stretch,
                    topMargin: 0.0,
                    bottomMargin: 0.0,
                    expectedMinPosition: 0f,
                    expectedZoomFactor: 1.0f);

                // Jump to absolute small zoomFactor to make the content smaller than the viewport.
                ZoomTo(scrollingPresenter, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

                ValidateContentWithConstrainedHeight(
                    compositor,
                    scrollingPresenter,
                    content: imageScrollingPresenterContent,
                    verticalAlignment: VerticalAlignment.Stretch,
                    topMargin: 0.0,
                    bottomMargin: 0.0,
                    expectedMinPosition: (float)(-c_defaultUIScrollingPresenterHeight * c_smallZoomFactor / 2.0), // -50
                    expectedZoomFactor: c_smallZoomFactor);
            }
        }

        [TestMethod]
        [TestProperty("Description",
            "Sets ScrollingPresenter.ContentOrientation to Horizontal and verifies Image positioning for various alignments and zoom factors.")]
        public void ImageWithConstrainedHeight()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.5f;
            const float c_largeZoomFactor = 2.0f;
            const double c_imageWidth = 250.0;
            const double c_topMargin = 40.0;
            const double c_bottomMargin = 10.0;
            ScrollingPresenter scrollingPresenter = null;
            Image imageScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                imageScrollingPresenterContent = new Image();
                scrollingPresenter = new ScrollingPresenter();

                Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                Verify.IsNotNull(uri);
                imageScrollingPresenterContent.Source = new BitmapImage(uri);
                imageScrollingPresenterContent.Margin = new Thickness(0, c_topMargin, 0, c_bottomMargin);
                scrollingPresenter.Content = imageScrollingPresenterContent;
                scrollingPresenter.Background = new Media.SolidColorBrush(Colors.Chartreuse);

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent: null, scrollingPresenterLoadedEvent);

                // Constraining the Image height and making the ScrollingPresenter smaller than the Image
                imageScrollingPresenterContent.Width = c_imageWidth;
                scrollingPresenter.ContentOrientation = ContentOrientation.Horizontal;
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            ValidateContentWithConstrainedHeight(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                verticalAlignment: VerticalAlignment.Stretch,
                topMargin: c_topMargin,
                bottomMargin: c_bottomMargin,
                expectedMinPosition: 0f,
                expectedZoomFactor: 1.0f);

            // Jump to absolute small zoomFactor to make the content smaller than the viewport.
            ZoomTo(scrollingPresenter, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

            ValidateContentWithConstrainedHeight(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                verticalAlignment: VerticalAlignment.Stretch,
                topMargin: c_topMargin,
                bottomMargin: c_bottomMargin,
                expectedMinPosition: (float)(-c_defaultUIScrollingPresenterHeight * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                verticalAlignment: VerticalAlignment.Top,
                topMargin: c_topMargin,
                bottomMargin: c_bottomMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                verticalAlignment: VerticalAlignment.Bottom,
                topMargin: c_topMargin,
                bottomMargin: c_bottomMargin,
                expectedMinPosition: (float)(-c_defaultUIScrollingPresenterHeight * c_smallZoomFactor), // -100
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                verticalAlignment: VerticalAlignment.Center,
                topMargin: c_topMargin,
                bottomMargin: c_bottomMargin,
                expectedMinPosition: (float)(-c_defaultUIScrollingPresenterHeight * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            // Jump to absolute large zoomFactor to make the content larger than the viewport.
            ZoomTo(scrollingPresenter, c_largeZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);

            ValidateContentWithConstrainedHeight(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                verticalAlignment: VerticalAlignment.Stretch,
                topMargin: c_topMargin,
                bottomMargin: c_bottomMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                verticalAlignment: VerticalAlignment.Top,
                topMargin: c_topMargin,
                bottomMargin: c_bottomMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                verticalAlignment: VerticalAlignment.Bottom,
                topMargin: c_topMargin,
                bottomMargin: c_bottomMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scrollingPresenter,
                content: imageScrollingPresenterContent,
                verticalAlignment: VerticalAlignment.Center,
                topMargin: c_topMargin,
                bottomMargin: c_bottomMargin,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);
        }

        [TestMethod]
        [TestProperty("Description",
            "Sets ScrollingPresenter.ContentOrientation to Both and verifies Image positioning for various alignments and zoom factors.")]
        public void ImageWithConstrainedSize()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollingPresenter"))
            {
                const float c_smallZoomFactor = 0.5f;
                const float c_largeZoomFactor = 2.0f;
                const double c_imageWidth = 2400;
                const double c_imageHeight = 1400.0;
                const double c_scrollingPresenterWidth = 314.0;
                const double c_scrollingPresenterHeight = 210.0;
                const double c_leftMargin = 20.0;
                const double c_rightMargin = 30.0;
                const double c_topMargin = 40.0;
                const double c_bottomMargin = 10.0;
                ScrollingPresenter scrollingPresenter = null;
                Image imageScrollingPresenterContent = null;
                AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
                Compositor compositor = null;

                RunOnUIThread.Execute(() =>
                {
                    imageScrollingPresenterContent = new Image();
                    scrollingPresenter = new ScrollingPresenter();

                    Uri uri = new Uri("ms-appx:/Assets/LargeWisteria.jpg");
                    Verify.IsNotNull(uri);
                    imageScrollingPresenterContent.Source = new BitmapImage(uri);
                    imageScrollingPresenterContent.Margin = new Thickness(c_leftMargin, c_topMargin, c_rightMargin, c_bottomMargin);
                    scrollingPresenter.Content = imageScrollingPresenterContent;
                    scrollingPresenter.Background = new Media.SolidColorBrush(Colors.Chartreuse);

                    SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent: null, scrollingPresenterLoadedEvent);

                // Constraining the Image width and height, and making the ScrollingPresenter smaller than the Image
                scrollingPresenter.ContentOrientation = ContentOrientation.Both;
                    scrollingPresenter.Width = c_scrollingPresenterWidth;
                    scrollingPresenter.Height = c_scrollingPresenterHeight;
                    compositor = Window.Current.Compositor;
                });

                WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

                ValidateContentWithConstrainedSize(
                    compositor,
                    scrollingPresenter,
                    content: imageScrollingPresenterContent,
                    horizontalAlignment: HorizontalAlignment.Stretch,
                    verticalAlignment: VerticalAlignment.Stretch,
                    expectedMinPositionX: 0.0f,
                    expectedMinPositionY: (float)-(c_scrollingPresenterHeight - (c_scrollingPresenterWidth - c_leftMargin - c_rightMargin) * c_imageHeight / c_imageWidth - c_topMargin - c_bottomMargin) / 2.0f, //-3
                    expectedZoomFactor: 1.0f);

                // Jump to absolute small zoomFactor to make the content smaller than the viewport.
                ZoomTo(scrollingPresenter, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

                ValidateContentWithConstrainedSize(
                    compositor,
                    scrollingPresenter,
                    content: imageScrollingPresenterContent,
                    horizontalAlignment: HorizontalAlignment.Stretch,
                    verticalAlignment: VerticalAlignment.Stretch,
                    expectedMinPositionX: (float)-c_scrollingPresenterWidth / 4.0f, // -78.5
                    expectedMinPositionY: (float)-(c_scrollingPresenterHeight - ((c_scrollingPresenterWidth - c_leftMargin - c_rightMargin) * c_imageHeight / c_imageWidth + c_topMargin + c_bottomMargin) * c_smallZoomFactor) / 2.0f, //-54
                    expectedZoomFactor: c_smallZoomFactor);

                // Jump to absolute large zoomFactor to make the content larger than the viewport.
                ZoomTo(scrollingPresenter, c_largeZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);

                ValidateContentWithConstrainedSize(
                    compositor,
                    scrollingPresenter,
                    content: imageScrollingPresenterContent,
                    horizontalAlignment: HorizontalAlignment.Stretch,
                    verticalAlignment: VerticalAlignment.Stretch,
                    expectedMinPositionX: 0.0f,
                    expectedMinPositionY: 0.0f,
                    expectedZoomFactor: c_largeZoomFactor);
            }
        }

        private void ValidateContentWithConstrainedWidth(
            Compositor compositor,
            ScrollingPresenter scrollingPresenter,
            FrameworkElement content,
            HorizontalAlignment horizontalAlignment,
            double leftMargin,
            double rightMargin,
            float expectedMinPosition,
            float expectedZoomFactor)
        {
            const double c_scrollingPresenterWidth = 200.0;
            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            float expectedHorizontalOffset = 0.0f;
            float expectedVerticalOffset = 0.0f;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"Covering alignment {horizontalAlignment.ToString()}");
                content.HorizontalAlignment = horizontalAlignment;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Vector2 minPosition = ScrollingPresenterTestHooks.GetMinPosition(scrollingPresenter);
                Vector2 arrangeRenderSizesDelta = ScrollingPresenterTestHooks.GetArrangeRenderSizesDelta(scrollingPresenter);
                Log.Comment($"MinPosition {minPosition.ToString()}");
                Log.Comment($"ArrangeRenderSizesDelta {arrangeRenderSizesDelta.ToString()}");
                Log.Comment($"Content.DesiredSize {content.DesiredSize.ToString()}");
                Log.Comment($"Content.RenderSize {content.RenderSize.ToString()}");

                Verify.AreEqual(expectedMinPosition, minPosition.X);
                Verify.AreEqual(horizontalAlignment, content.HorizontalAlignment);
                Verify.AreEqual(c_scrollingPresenterWidth, content.DesiredSize.Width); // 200
                Verify.AreEqual(c_scrollingPresenterWidth - leftMargin - rightMargin, content.RenderSize.Width);
                double arrangeRenderSizesDeltaX = GetArrangeRenderSizesDelta(
                    BiDirectionalAlignmentFromHorizontalAlignment(horizontalAlignment),
                    extentSize: c_scrollingPresenterWidth,
                    renderSize: c_scrollingPresenterWidth - leftMargin - rightMargin,
                    nearMargin: leftMargin,
                    farMargin: rightMargin);
                Verify.AreEqual(leftMargin, arrangeRenderSizesDeltaX);
                Verify.AreEqual(arrangeRenderSizesDeltaX, arrangeRenderSizesDelta.X);
                expectedHorizontalOffset = -minPosition.X + (expectedZoomFactor - 1.0f) * arrangeRenderSizesDelta.X;
                expectedVerticalOffset = -minPosition.Y + (expectedZoomFactor - 1.0f) * arrangeRenderSizesDelta.Y;
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"horizontalOffset={horizontalOffset}, verticalOffset={verticalOffset}, zoomFactor={zoomFactor}");
                Log.Comment($"expectedHorizontalOffset={expectedHorizontalOffset}, expectedVerticalOffset={expectedVerticalOffset}, expectedZoomFactor={expectedZoomFactor}");
                Verify.AreEqual(expectedHorizontalOffset, horizontalOffset);
                Verify.AreEqual(expectedVerticalOffset, verticalOffset);
                Verify.AreEqual(expectedZoomFactor, zoomFactor);
            });
        }

        private void ValidateContentWithConstrainedHeight(
            Compositor compositor,
            ScrollingPresenter scrollingPresenter,
            FrameworkElement content,
            VerticalAlignment verticalAlignment,
            double topMargin,
            double bottomMargin,
            float expectedMinPosition,
            float expectedZoomFactor)
        {
            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            float expectedHorizontalOffset = 0.0f;
            float expectedVerticalOffset = 0.0f;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"Covering alignment {verticalAlignment.ToString()}");
                content.VerticalAlignment = verticalAlignment;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Vector2 minPosition = ScrollingPresenterTestHooks.GetMinPosition(scrollingPresenter);
                Vector2 arrangeRenderSizesDelta = ScrollingPresenterTestHooks.GetArrangeRenderSizesDelta(scrollingPresenter);
                Log.Comment($"MinPosition {minPosition.ToString()}");
                Log.Comment($"ArrangeRenderSizesDelta {arrangeRenderSizesDelta.ToString()}");
                Log.Comment($"Content.DesiredSize {content.DesiredSize.ToString()}");
                Log.Comment($"Content.RenderSize {content.RenderSize.ToString()}");

                Verify.AreEqual(expectedMinPosition, minPosition.Y);
                Verify.AreEqual(verticalAlignment, content.VerticalAlignment);
                Verify.AreEqual(c_defaultUIScrollingPresenterHeight, content.DesiredSize.Height);
                Verify.AreEqual(c_defaultUIScrollingPresenterHeight - topMargin - bottomMargin, content.RenderSize.Height);
                double arrangeRenderSizesDeltaY = GetArrangeRenderSizesDelta(
                    BiDirectionalAlignmentFromVerticalAlignment(verticalAlignment),
                    extentSize: c_defaultUIScrollingPresenterHeight,
                    renderSize: c_defaultUIScrollingPresenterHeight - topMargin - bottomMargin,
                    nearMargin: topMargin,
                    farMargin: bottomMargin);
                Verify.AreEqual(topMargin, arrangeRenderSizesDeltaY);
                Verify.AreEqual(arrangeRenderSizesDeltaY, arrangeRenderSizesDelta.Y);
                expectedVerticalOffset = -minPosition.Y + (expectedZoomFactor - 1.0f) * arrangeRenderSizesDelta.Y;
                expectedHorizontalOffset = -minPosition.X + (expectedZoomFactor - 1.0f) * arrangeRenderSizesDelta.X;
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"horizontalOffset={horizontalOffset}, verticalOffset={verticalOffset}, zoomFactor={zoomFactor}");
                Log.Comment($"expectedHorizontalOffset={expectedHorizontalOffset}, expectedVerticalOffset={expectedVerticalOffset}, expectedZoomFactor={expectedZoomFactor}");
                Verify.AreEqual(expectedHorizontalOffset, horizontalOffset);
                Verify.AreEqual(expectedVerticalOffset, verticalOffset);
                Verify.AreEqual(expectedZoomFactor, zoomFactor);
            });
        }

        private void ValidateContentWithConstrainedSize(
            Compositor compositor,
            ScrollingPresenter scrollingPresenter,
            FrameworkElement content,
            HorizontalAlignment horizontalAlignment,
            VerticalAlignment verticalAlignment,
            float expectedMinPositionX,
            float expectedMinPositionY,
            float expectedZoomFactor)
        {
            const double c_leftMargin = 20.0;
            const double c_rightMargin = 30.0;
            const double c_topMargin = 40.0;
            const double c_bottomMargin = 10.0;
            const double c_scrollingPresenterWidth = 314.0;
            const double c_scrollingPresenterHeight = 210.0;
            const double c_imageWidth = 2400;
            const double c_imageHeight = 1400.0;

            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            double horizontalArrangeRenderSizesDelta = 0.0;
            double verticalArrangeRenderSizesDelta = 0.0;
            double expectedHorizontalOffset = 0.0;
            double expectedVerticalOffset = 0.0;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"Covering alignments {horizontalAlignment.ToString()} and {verticalAlignment.ToString()}");
                content.HorizontalAlignment = horizontalAlignment;
                content.VerticalAlignment = verticalAlignment;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(horizontalAlignment, content.HorizontalAlignment);
                Verify.AreEqual(verticalAlignment, content.VerticalAlignment);

                Vector2 minPosition = ScrollingPresenterTestHooks.GetMinPosition(scrollingPresenter);
                Vector2 arrangeRenderSizesDelta = ScrollingPresenterTestHooks.GetArrangeRenderSizesDelta(scrollingPresenter);
                Log.Comment($"MinPosition {minPosition.ToString()}");
                Log.Comment($"ArrangeRenderSizesDelta {arrangeRenderSizesDelta.ToString()}");
                Log.Comment($"Content.DesiredSize {content.DesiredSize.ToString()}");
                Log.Comment($"Content.RenderSize {content.RenderSize.ToString()}");

                Verify.AreEqual(expectedMinPositionX, minPosition.X);
                Verify.AreEqual(expectedMinPositionY, minPosition.Y);
                Verify.AreEqual(c_scrollingPresenterWidth, content.DesiredSize.Width); // 314
                Verify.AreEqual((c_scrollingPresenterWidth - c_leftMargin - c_rightMargin) * c_imageHeight / c_imageWidth + c_topMargin + c_bottomMargin, content.DesiredSize.Height); // 204
                Verify.AreEqual(c_scrollingPresenterWidth - c_leftMargin - c_rightMargin, content.RenderSize.Width); // 264
                Verify.AreEqual((c_scrollingPresenterWidth - c_leftMargin - c_rightMargin) * c_imageHeight / c_imageWidth, content.RenderSize.Height); // 154

                horizontalArrangeRenderSizesDelta = GetArrangeRenderSizesDelta(
                    BiDirectionalAlignmentFromHorizontalAlignment(horizontalAlignment),
                    extentSize: c_scrollingPresenterWidth,
                    renderSize: c_scrollingPresenterWidth - c_leftMargin - c_rightMargin,
                    nearMargin: c_leftMargin,
                    farMargin: c_rightMargin);
                Log.Comment($"horizontalArrangeRenderSizesDelta {horizontalArrangeRenderSizesDelta}");
                Verify.AreEqual(c_leftMargin, horizontalArrangeRenderSizesDelta);
                Verify.AreEqual(arrangeRenderSizesDelta.X, horizontalArrangeRenderSizesDelta);
                expectedHorizontalOffset = -minPosition.X + (expectedZoomFactor - 1.0f) * horizontalArrangeRenderSizesDelta;

                verticalArrangeRenderSizesDelta = GetArrangeRenderSizesDelta(
                    BiDirectionalAlignmentFromVerticalAlignment(verticalAlignment),
                    extentSize: c_scrollingPresenterHeight,
                    renderSize: c_scrollingPresenterHeight - c_topMargin - c_bottomMargin,
                    nearMargin: c_topMargin,
                    farMargin: c_bottomMargin);
                Log.Comment($"verticalArrangeRenderSizesDelta {verticalArrangeRenderSizesDelta}");
                Verify.AreEqual(c_topMargin, verticalArrangeRenderSizesDelta);
                Verify.AreEqual(arrangeRenderSizesDelta.Y, verticalArrangeRenderSizesDelta);
                expectedVerticalOffset = -minPosition.Y + (expectedZoomFactor - 1.0f) * verticalArrangeRenderSizesDelta;
            });

            SpyTranslationAndScale(scrollingPresenter, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"horizontalOffset={horizontalOffset}, verticalOffset={verticalOffset}, zoomFactor={zoomFactor}");
                Log.Comment($"expectedHorizontalOffset={expectedHorizontalOffset}, expectedVerticalOffset={expectedVerticalOffset}, expectedZoomFactor={expectedZoomFactor}");
                Verify.AreEqual(expectedHorizontalOffset, horizontalOffset);
                Verify.AreEqual(expectedVerticalOffset, verticalOffset);
                Verify.AreEqual(expectedZoomFactor, zoomFactor);
            });
        }

        private double GetArrangeRenderSizesDelta(
            BiDirectionalAlignment alignment,
            double extentSize,
            double renderSize,
            double nearMargin,
            double farMargin)
        {
            double delta = extentSize - renderSize;

            if (alignment == BiDirectionalAlignment.Near)
            {
                delta = 0.0;
            }
            else
            {
                delta -= nearMargin + farMargin;
            }

            if (alignment == BiDirectionalAlignment.Center ||
                alignment == BiDirectionalAlignment.Stretch)
            {
                delta /= 2.0;
            }

            delta += nearMargin;

            Log.Comment($"GetArrangeRenderSizesDelta returns {delta}.");
            return delta;
        }

        private BiDirectionalAlignment BiDirectionalAlignmentFromHorizontalAlignment(HorizontalAlignment horizontalAlignment)
        {
            switch (horizontalAlignment)
            {
                case HorizontalAlignment.Stretch:
                    return BiDirectionalAlignment.Stretch;
                case HorizontalAlignment.Left:
                    return BiDirectionalAlignment.Near;
                case HorizontalAlignment.Right:
                    return BiDirectionalAlignment.Far;
                default:
                    return BiDirectionalAlignment.Center;
            }
        }

        private BiDirectionalAlignment BiDirectionalAlignmentFromVerticalAlignment(VerticalAlignment verticalAlignment)
        {
            switch (verticalAlignment)
            {
                case VerticalAlignment.Stretch:
                    return BiDirectionalAlignment.Stretch;
                case VerticalAlignment.Top:
                    return BiDirectionalAlignment.Near;
                case VerticalAlignment.Bottom:
                    return BiDirectionalAlignment.Far;
                default:
                    return BiDirectionalAlignment.Center;
            }
        }
    }
}
