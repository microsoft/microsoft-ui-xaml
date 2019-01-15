// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Threading;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Composition;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollerTests
    {
        private enum BiDirectionalAlignment
        {
            Near,
            Center,
            Stretch,
            Far
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Child.Margin and verifies InteractionTracker.MaxPosition.")]
        public void BasicMargin()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1.");
                return;
            }

            const double c_Margin = 50.0;
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding positive Margin to Scroller.Child");
                rectangleScrollerChild.Margin = new Thickness(c_Margin);
            });

            // Try to jump beyond maximum offsets
            ChangeOffsets(
                scroller,
                c_defaultUIScrollerChildWidth + 2 * c_Margin - c_defaultUIScrollerWidth + 10.0,
                c_defaultUIScrollerChildHeight + 2 * c_Margin - c_defaultUIScrollerHeight + 10.0,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation,
                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                true /*hookViewChanged*/,
                c_defaultUIScrollerChildWidth + 2 * c_Margin - c_defaultUIScrollerWidth,
                c_defaultUIScrollerChildHeight + 2 * c_Margin - c_defaultUIScrollerHeight);

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding negative Margin to Scroller.Child");
                rectangleScrollerChild.Margin = new Thickness(-c_Margin);
            });

            // Try to jump beyond maximum offsets
            ChangeOffsets(
                scroller,
                c_defaultUIScrollerChildWidth - 2 * c_Margin - c_defaultUIScrollerWidth + 10.0,
                c_defaultUIScrollerChildHeight - 2 * c_Margin - c_defaultUIScrollerHeight + 10.0,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation,
                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                false /*hookViewChanged*/,
                c_defaultUIScrollerChildWidth - 2 * c_Margin - c_defaultUIScrollerWidth,
                c_defaultUIScrollerChildHeight - 2 * c_Margin - c_defaultUIScrollerHeight);
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Child.Padding and verifies InteractionTracker.MaxPosition.")]
        public void BasicPadding()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1 because test sometimes hangs indefinitely."); // Bug 12286203
                return;
            }

            const double c_Padding = 50.0;
            Scroller scroller = null;
            Border borderScrollerChild = null;
            Rectangle rectangle = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                borderScrollerChild = new Border();
                rectangle = new Rectangle();
                scroller = new Scroller();

                borderScrollerChild.Width = c_defaultUIScrollerChildWidth;
                borderScrollerChild.Height = c_defaultUIScrollerChildHeight;
                borderScrollerChild.Child = rectangle;
                scroller.Child = borderScrollerChild;

                SetupDefaultUI(scroller, null /*rectangleScrollerChild*/, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding Padding to Scroller.Child");
                borderScrollerChild.Padding = new Thickness(c_Padding);
            });

            // Try to jump beyond maximum offsets
            ChangeOffsets(
                scroller,
                c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth + 10.0,
                c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight + 10.0,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation,
                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                true /*hookViewChanged*/,
                c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth,
                c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight);
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Child.HorizontalAlignment/VerticalAlignment and verifies child positioning.")]
        public void BasicAlignment()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX Scroller tests fail on RS5_Release
                return;
            }

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.15f;
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute small zoomFactor to make the child smaller than the viewport.
            ChangeZoomFactor(scroller, c_smallZoomFactor, 0.0f, 0.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Stretch/Strech alignments");
                Verify.AreEqual(rectangleScrollerChild.HorizontalAlignment, HorizontalAlignment.Stretch);
                Verify.AreEqual(rectangleScrollerChild.VerticalAlignment, VerticalAlignment.Stretch);
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - (float)(c_defaultUIScrollerWidth - c_defaultUIScrollerChildWidth * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - (float)(c_defaultUIScrollerHeight - c_defaultUIScrollerChildHeight * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(zoomFactor, c_smallZoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Left/Top alignments");
                rectangleScrollerChild.HorizontalAlignment = HorizontalAlignment.Left;
                rectangleScrollerChild.VerticalAlignment = VerticalAlignment.Top;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.AreEqual(horizontalOffset, 0.0f);
            Verify.AreEqual(verticalOffset, 0.0f);
            Verify.AreEqual(zoomFactor, c_smallZoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Right/Bottom alignments");
                rectangleScrollerChild.HorizontalAlignment = HorizontalAlignment.Right;
                rectangleScrollerChild.VerticalAlignment = VerticalAlignment.Bottom;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - (float)(c_defaultUIScrollerWidth - c_defaultUIScrollerChildWidth * c_smallZoomFactor)) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - (float)(c_defaultUIScrollerHeight - c_defaultUIScrollerChildHeight * c_smallZoomFactor)) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(zoomFactor, c_smallZoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Center/Center alignments");
                rectangleScrollerChild.HorizontalAlignment = HorizontalAlignment.Center;
                rectangleScrollerChild.VerticalAlignment = VerticalAlignment.Center;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - (float)(c_defaultUIScrollerWidth - c_defaultUIScrollerChildWidth * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - (float)(c_defaultUIScrollerHeight - c_defaultUIScrollerChildHeight * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(zoomFactor, c_smallZoomFactor);
        }

        [TestMethod]
        [TestProperty("Description", "Uses a StackPanel with Stretch alignment as Scroller.Child to verify it stretched to the size of the Scroller.")]
        public void StretchAlignment()
        {
            Scroller scroller = null;
            StackPanel stackPanel = null;
            Rectangle rectangle = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangle = new Rectangle();
                rectangle.Height = c_defaultUIScrollerChildHeight;
                stackPanel = new StackPanel();
                stackPanel.Children.Add(rectangle);
                scroller = new Scroller();
                scroller.Width = c_defaultUIScrollerWidth;
                scroller.Height = c_defaultUIScrollerHeight;
                scroller.Child = stackPanel;

                scroller.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Loaded event handler");
                    scrollerLoadedEvent.Set();
                };

                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scroller;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Checking Stretch/Strech alignments");
                Verify.AreEqual(stackPanel.HorizontalAlignment, HorizontalAlignment.Stretch);
                Verify.AreEqual(stackPanel.VerticalAlignment, VerticalAlignment.Stretch);

                Log.Comment("Checking StackPanel size");
                Verify.AreEqual(stackPanel.ActualWidth, c_defaultUIScrollerWidth);
                Verify.AreEqual(stackPanel.ActualHeight, c_defaultUIScrollerChildHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Child.HorizontalAlignment/VerticalAlignment and Scroller.Child.Margin and verifies child positioning.")]
        public void BasicMarginAndAlignment()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX Scroller tests fail on RS5_Release
                return;
            }

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.15f;
            const double c_Margin = 40.0;
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);

                Log.Comment("Adding positive Margin to Scroller.Child");
                rectangleScrollerChild.Margin = new Thickness(c_Margin);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute small zoomFactor to make the child smaller than the viewport.
            ChangeZoomFactor(scroller, c_smallZoomFactor, 0.0f, 0.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Stretch/Strech alignments");
                Verify.AreEqual(rectangleScrollerChild.HorizontalAlignment, HorizontalAlignment.Stretch);
                Verify.AreEqual(rectangleScrollerChild.VerticalAlignment, VerticalAlignment.Stretch);

            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - 20.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - 15.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(zoomFactor, c_smallZoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Left/Top alignments");
                rectangleScrollerChild.HorizontalAlignment = HorizontalAlignment.Left;
                rectangleScrollerChild.VerticalAlignment = VerticalAlignment.Top;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset + 34.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset + 34.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(zoomFactor, c_smallZoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Right/Bottom alignments");
                rectangleScrollerChild.HorizontalAlignment = HorizontalAlignment.Right;
                rectangleScrollerChild.VerticalAlignment = VerticalAlignment.Bottom;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - 74.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - 64.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(zoomFactor, c_smallZoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Center/Center alignments");
                rectangleScrollerChild.HorizontalAlignment = HorizontalAlignment.Center;
                rectangleScrollerChild.VerticalAlignment = VerticalAlignment.Center;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - 20.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - 15.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(zoomFactor, c_smallZoomFactor);
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Child to Image with unnatural size and verifies InteractionTracker.MaxPosition.")]
        public void ImageWithUnnaturalSize()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1.");
                return;
            }

            const double c_UnnaturalImageWidth = 1200.0;
            const double c_UnnaturalImageHeight = 1000.0;
            Scroller scroller = null;
            Image imageScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                imageScrollerChild = new Image();
                scroller = new Scroller();

                Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                Verify.IsNotNull(uri);
                imageScrollerChild.Source = new BitmapImage(uri);
                imageScrollerChild.Width = c_UnnaturalImageWidth;
                imageScrollerChild.Height = c_UnnaturalImageHeight;
                scroller.Child = imageScrollerChild;

                SetupDefaultUI(scroller, null /*rectangleScrollerChild*/, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Try to jump beyond maximum offsets
            ChangeOffsets(
                scroller,
                c_UnnaturalImageWidth - c_defaultUIScrollerWidth + 10.0,
                c_UnnaturalImageHeight - c_defaultUIScrollerHeight + 10.0,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation,
                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                true /*hookViewChanged*/,
                c_UnnaturalImageWidth - c_defaultUIScrollerWidth,
                c_UnnaturalImageHeight - c_defaultUIScrollerHeight);
        }

        [TestMethod]
        [TestProperty("Description", 
            "Sets Scroller.IsChildAvailableWidthConstrained to True and verifies Image positioning for various alignments and zoom factors.")]
        public void ImageWithConstrainedWidth()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX Scroller tests fail on RS5_Release
                return;
            }

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.5f;
            const float c_largeZoomFactor = 2.0f;
            const double c_imageHeight = 300.0;
            const double c_scrollerWidth = 200.0;
            const double c_leftMargin = 20.0;
            const double c_rightMargin = 30.0;
            Scroller scroller = null;
            Image imageScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                imageScrollerChild = new Image();
                scroller = new Scroller();

                Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                Verify.IsNotNull(uri);
                imageScrollerChild.Source = new BitmapImage(uri);
                imageScrollerChild.Margin = new Thickness(c_leftMargin, 0, c_rightMargin, 0);
                scroller.Child = imageScrollerChild;
                scroller.Background = new Media.SolidColorBrush(Colors.Chartreuse);

                SetupDefaultUI(scroller, null /*rectangleScrollerChild*/, scrollerLoadedEvent);

                // Constraining the Image width and making the Scroller smaller than the Image
                imageScrollerChild.Height = c_imageHeight;
                scroller.IsChildAvailableWidthConstrained = true;
                scroller.Width = c_scrollerWidth;
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute small zoomFactor to make the child smaller than the viewport.
            ChangeZoomFactor(scroller, c_smallZoomFactor, 0.0f, 0.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);

            ValidateChildWithConstrainedWidth(
                compositor,
                scroller,
                child: imageScrollerChild,
                horizontalAlignment: HorizontalAlignment.Stretch,
                expectedVerticalOffset: (float)(c_defaultUIScrollerHeight - c_imageHeight * c_smallZoomFactor) / 2.0f, // (200 - 300 * 0.5) / 2 = 25
                expectedMinPosition: (float)(-c_scrollerWidth * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            ValidateChildWithConstrainedWidth(
                compositor,
                scroller,
                child: imageScrollerChild,
                horizontalAlignment: HorizontalAlignment.Left,
                expectedVerticalOffset: (float)(c_defaultUIScrollerHeight - c_imageHeight * c_smallZoomFactor) / 2.0f, // (200 - 300 * 0.5) / 2 = 25
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_smallZoomFactor);

            ValidateChildWithConstrainedWidth(
                compositor,
                scroller,
                child: imageScrollerChild,
                horizontalAlignment: HorizontalAlignment.Right,
                expectedVerticalOffset: (float)(c_defaultUIScrollerHeight - c_imageHeight * c_smallZoomFactor) / 2.0f, // (200 - 300 * 0.5) / 2 = 25
                expectedMinPosition: (float)(-c_scrollerWidth * c_smallZoomFactor), // -100
                expectedZoomFactor: c_smallZoomFactor);

            ValidateChildWithConstrainedWidth(
                compositor,
                scroller,
                child: imageScrollerChild,
                horizontalAlignment: HorizontalAlignment.Center,
                expectedVerticalOffset: (float)(c_defaultUIScrollerHeight - c_imageHeight * c_smallZoomFactor) / 2.0f, // (200 - 300 * 0.5) / 2 = 25
                expectedMinPosition: (float)(-c_scrollerWidth * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            // Jump to absolute large zoomFactor to make the child larger than the viewport.
            ChangeZoomFactor(scroller, c_largeZoomFactor, 0.0f, 0.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);

            ValidateChildWithConstrainedWidth(
                compositor,
                scroller,
                child: imageScrollerChild,
                horizontalAlignment: HorizontalAlignment.Stretch,
                expectedVerticalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateChildWithConstrainedWidth(
                compositor,
                scroller,
                child: imageScrollerChild,
                horizontalAlignment: HorizontalAlignment.Left,
                expectedVerticalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateChildWithConstrainedWidth(
                compositor,
                scroller,
                child: imageScrollerChild,
                horizontalAlignment: HorizontalAlignment.Right,
                expectedVerticalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateChildWithConstrainedWidth(
                compositor,
                scroller,
                child: imageScrollerChild,
                horizontalAlignment: HorizontalAlignment.Center,
                expectedVerticalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);
        }

        [TestMethod]
        [TestProperty("Description",
            "Sets Scroller.IsChildAvailableHeightConstrained to True and verifies Image positioning for various alignments and zoom factors.")]
        public void ImageWithConstrainedHeight()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX Scroller tests fail on RS5_Release
                return;
            }

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
            Scroller scroller = null;
            Image imageScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                imageScrollerChild = new Image();
                scroller = new Scroller();

                Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                Verify.IsNotNull(uri);
                imageScrollerChild.Source = new BitmapImage(uri);
                imageScrollerChild.Margin = new Thickness(0, c_topMargin, 0, c_bottomMargin);
                scroller.Child = imageScrollerChild;
                scroller.Background = new Media.SolidColorBrush(Colors.Chartreuse);

                SetupDefaultUI(scroller, rectangleScrollerChild: null, scrollerLoadedEvent: scrollerLoadedEvent);

                // Constraining the Image height and making the Scroller smaller than the Image
                imageScrollerChild.Width = c_imageWidth;
                scroller.IsChildAvailableHeightConstrained = true;
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute small zoomFactor to make the child smaller than the viewport.
            ChangeZoomFactor(scroller, c_smallZoomFactor, 0.0f, 0.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);

            ValidateChildWithConstrainedHeight(
                compositor,
                scroller,
                child: imageScrollerChild,
                verticalAlignment: VerticalAlignment.Stretch,
                expectedHorizontalOffset: (float)(c_defaultUIScrollerWidth - c_imageWidth * c_smallZoomFactor) / 2.0f, // 87.5
                expectedMinPosition: (float)(-c_defaultUIScrollerHeight * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            ValidateChildWithConstrainedHeight(
                compositor,
                scroller,
                child: imageScrollerChild,
                verticalAlignment: VerticalAlignment.Top,
                expectedHorizontalOffset: (float)(c_defaultUIScrollerWidth - c_imageWidth * c_smallZoomFactor) / 2.0f, // 87.5
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_smallZoomFactor);

            ValidateChildWithConstrainedHeight(
                compositor,
                scroller,
                child: imageScrollerChild,
                verticalAlignment: VerticalAlignment.Bottom,
                expectedHorizontalOffset: (float)(c_defaultUIScrollerWidth - c_imageWidth * c_smallZoomFactor) / 2.0f, // 87.5
                expectedMinPosition: (float)(-c_defaultUIScrollerHeight * c_smallZoomFactor), // -100
                expectedZoomFactor: c_smallZoomFactor);

            ValidateChildWithConstrainedHeight(
                compositor,
                scroller,
                child: imageScrollerChild,
                verticalAlignment: VerticalAlignment.Center,
                expectedHorizontalOffset: (float)(c_defaultUIScrollerWidth - c_imageWidth * c_smallZoomFactor) / 2.0f, // 87.5
                expectedMinPosition: (float)(-c_defaultUIScrollerHeight * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            // Jump to absolute large zoomFactor to make the child larger than the viewport.
            ChangeZoomFactor(scroller, c_largeZoomFactor, 0.0f, 0.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);

            ValidateChildWithConstrainedHeight(
                compositor,
                scroller,
                child: imageScrollerChild,
                verticalAlignment: VerticalAlignment.Stretch,
                expectedHorizontalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateChildWithConstrainedHeight(
                compositor,
                scroller,
                child: imageScrollerChild,
                verticalAlignment: VerticalAlignment.Top,
                expectedHorizontalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateChildWithConstrainedHeight(
                compositor,
                scroller,
                child: imageScrollerChild,
                verticalAlignment: VerticalAlignment.Bottom,
                expectedHorizontalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateChildWithConstrainedHeight(
                compositor,
                scroller,
                child: imageScrollerChild,
                verticalAlignment: VerticalAlignment.Center,
                expectedHorizontalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);
        }

        private void ValidateChildWithConstrainedWidth(
            Compositor compositor,
            Scroller scroller,
            FrameworkElement child,
            HorizontalAlignment horizontalAlignment,
            float expectedVerticalOffset,
            float expectedMinPosition,
            float expectedZoomFactor)
        {
            const double c_leftMargin = 20.0;
            const double c_rightMargin = 30.0;
            const double c_scrollerWidth = 200.0;

            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            double arrangeRenderSizesDelta = 0.0;
            double expectedHorizontalOffset = 0.0;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering alignment " + horizontalAlignment.ToString());
                child.HorizontalAlignment = horizontalAlignment;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(child.HorizontalAlignment, horizontalAlignment);
                Verify.AreEqual(child.DesiredSize.Width, c_scrollerWidth); // 200
                Verify.AreEqual(child.RenderSize.Width, c_scrollerWidth - c_leftMargin - c_rightMargin); // 200 - 20 - 30 = 150
                arrangeRenderSizesDelta = GetArrangeRenderSizesDelta(
                    BiDirectionalAlignmentFromHorizontalAlignment(horizontalAlignment),
                    extentSize: c_scrollerWidth,
                    renderSize: c_scrollerWidth - c_leftMargin - c_rightMargin,
                    nearMargin: c_leftMargin,
                    farMargin: c_rightMargin);
                Verify.AreEqual(arrangeRenderSizesDelta, 20.0);
                expectedHorizontalOffset = -expectedMinPosition + (expectedZoomFactor - 1.0f) * arrangeRenderSizesDelta;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Log.Comment("horizontalOffset={0}, verticalOffset={1}, zoomFactor={2}",
                horizontalOffset, verticalOffset, zoomFactor);
            Log.Comment("expectedHorizontalOffset={0}, expectedVerticalOffset={1}, expectedZoomFactor={2}",
                expectedHorizontalOffset, expectedVerticalOffset, expectedZoomFactor);
            Verify.AreEqual(horizontalOffset, expectedHorizontalOffset);
            Verify.AreEqual(verticalOffset, expectedVerticalOffset);
            Verify.AreEqual(zoomFactor, expectedZoomFactor);
        }

        private void ValidateChildWithConstrainedHeight(
            Compositor compositor,
            Scroller scroller,
            FrameworkElement child,
            VerticalAlignment verticalAlignment,
            float expectedHorizontalOffset,
            float expectedMinPosition,
            float expectedZoomFactor)
        {
            const double c_topMargin = 40.0;
            const double c_bottomMargin = 10.0;

            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            double arrangeRenderSizesDelta = 0.0;
            double expectedVerticalOffset = 0.0;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering alignment " + verticalAlignment.ToString());
                child.VerticalAlignment = verticalAlignment;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(child.VerticalAlignment, verticalAlignment);
                Verify.AreEqual(child.DesiredSize.Height, c_defaultUIScrollerHeight); // 200
                Verify.AreEqual(child.RenderSize.Height, c_defaultUIScrollerHeight - c_topMargin - c_bottomMargin); // 200 - 40 - 10 = 150
                arrangeRenderSizesDelta = GetArrangeRenderSizesDelta(
                    BiDirectionalAlignmentFromVerticalAlignment(verticalAlignment),
                    extentSize: c_defaultUIScrollerHeight,
                    renderSize: c_defaultUIScrollerHeight - c_topMargin - c_bottomMargin,
                    nearMargin: c_topMargin,
                    farMargin: c_bottomMargin);
                Verify.AreEqual(arrangeRenderSizesDelta, 40.0);
                expectedVerticalOffset = -expectedMinPosition + (expectedZoomFactor - 1.0f) * arrangeRenderSizesDelta;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Log.Comment("horizontalOffset={0}, verticalOffset={1}, zoomFactor={2}",
                horizontalOffset, verticalOffset, zoomFactor);
            Log.Comment("expectedHorizontalOffset={0}, expectedVerticalOffset={1}, expectedZoomFactor={2}",
                expectedHorizontalOffset, expectedVerticalOffset, expectedZoomFactor);
            Verify.AreEqual(horizontalOffset, expectedHorizontalOffset);
            Verify.AreEqual(verticalOffset, expectedVerticalOffset);
            Verify.AreEqual(zoomFactor, expectedZoomFactor);
        }

        private double GetArrangeRenderSizesDelta(
            BiDirectionalAlignment alignment,
            double extentSize,
            double renderSize,
            double nearMargin,
            double farMargin)
        {
            double delta = (alignment == BiDirectionalAlignment.Stretch) ? 0.0 : extentSize - renderSize;

            if (alignment == BiDirectionalAlignment.Center ||
                alignment == BiDirectionalAlignment.Far)
            {
                delta -= nearMargin + farMargin;
            }

            if (alignment == BiDirectionalAlignment.Center ||
                alignment == BiDirectionalAlignment.Stretch)
            {
                delta /= 2.0f;
            }
            else if (alignment == BiDirectionalAlignment.Near)
            {
                delta = 0.0f;
            }

            delta += nearMargin;

            Log.Comment("GetArrangeRenderSizesDelta returns {0}.", delta);
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
