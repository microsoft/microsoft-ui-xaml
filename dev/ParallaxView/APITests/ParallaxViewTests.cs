// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;

using System;
using System.Numerics;
using System.Threading;

using Windows.Foundation;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ParallaxSourceOffsetKind = Microsoft.UI.Xaml.Controls.ParallaxSourceOffsetKind;
using ParallaxView = Microsoft.UI.Xaml.Controls.ParallaxView;
using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using ScrollingAnimationMode = Microsoft.UI.Xaml.Controls.ScrollingAnimationMode;
using ScrollingSnapPointsMode = Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;
using ScrollingZoomOptions = Microsoft.UI.Xaml.Controls.ScrollingZoomOptions;
using ScrollingInteractionState = Microsoft.UI.Xaml.Controls.ScrollingInteractionState;
using ScrollingZoomMode = Microsoft.UI.Xaml.Controls.ScrollingZoomMode;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ParallaxViewTests : ApiTestBase
    {
        private const float c_shiftTolerance = 0.0001f;
        private const int c_MaxWaitDuration = 5000;

        private const ParallaxSourceOffsetKind c_defaultHorizontalSourceOffsetKind = ParallaxSourceOffsetKind.Relative;
        private const double c_defaultHorizontalSourceStartOffset = 0.0;
        private const double c_defaultHorizontalSourceEndOffset = 0.0;
        private const double c_defaultMaxHorizontalShiftRatio = 1.0;
        private const double c_defaultHorizontalShift = 0.0;
        private const bool c_defaultIsHorizontalShiftClamped = true;
        private const ParallaxSourceOffsetKind c_defaultVerticalSourceOffsetKind = ParallaxSourceOffsetKind.Relative;
        private const double c_defaultVerticalSourceStartOffset = 0.0;
        private const double c_defaultVerticalSourceEndOffset = 0.0;
        private const double c_defaultMaxVerticalShiftRatio = 1.0;
        private const double c_defaultVerticalShift = 0.0;
        private const bool c_defaultIsVerticalShiftClamped = true;

        private const double c_scrollViewerMaxOverpanRatio = 0.1;  // ScrollViewer's maximum underpan/viewport and overpan/viewport ratio
        private const double c_scrollViewerMaxOverpanRatioWithZoom = 1.0;  // ScrollViewer's maximum underpan/viewport and overpan/viewport ratio when ZoomMode is Enabled
        private const double c_scrollPresenterMaxOverpanRatio = 1.0;  // ScrollPresenter's maximum underpan/viewport and overpan/viewport ratio
        private const double c_defaultUIScrollPresenterWidth = 200.0;
        private const double c_defaultUIScrollPresenterHeight = 100.0;
        private const double c_defaultUIScrollPresenterContentWidth = 1200.0;
        private const double c_defaultUIScrollPresenterContentHeight = 600.0;
        private const double c_defaultUIFinalScrollPresenterHorizontalOffset = 100.0;
        private const double c_defaultUIFinalScrollPresenterVerticalOffset = 50.0;
        private const double c_defaultUIScrollViewerWidth = 200.0;
        private const double c_defaultUIScrollViewerHeight = 100.0;
        private const double c_defaultUIScrollViewerContentWidth = 1200.0;
        private const double c_defaultUIScrollViewerContentHeight = 600.0;
        private const double c_defaultUIFinalScrollViewerHorizontalOffset = 100.0;
        private const double c_defaultUIFinalScrollViewerVerticalOffset = 50.0;
        private const double c_defaultUIHorizontalShift = 100.0;
        private const double c_defaultUIVerticalShift = 50.0;

        // Enum used in tests that use ChangePropertyAfterParallaxViewDisposal
        private enum PropertyId
        {
            ParallaxViewChildVerticalAlignment,
            ScrollViewerContentHeight,
        }

        private enum SourceType
        {
            ScrollViewer,
            ScrollPresenter,
        }

        [TestMethod]
        [Description("Verifies the ParallaxView default properties.")]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                ParallaxView parallaxView = new ParallaxView();
                Verify.IsNotNull(parallaxView);

                Log.Comment("Verifying ParallaxView default property values");
                Verify.IsNull(parallaxView.Child);
                Verify.IsNull(parallaxView.Source);
                Verify.AreEqual(parallaxView.HorizontalSourceOffsetKind, c_defaultHorizontalSourceOffsetKind);
                Verify.AreEqual(parallaxView.HorizontalSourceStartOffset, c_defaultHorizontalSourceStartOffset);
                Verify.AreEqual(parallaxView.HorizontalSourceEndOffset, c_defaultHorizontalSourceEndOffset);
                Verify.AreEqual(parallaxView.MaxHorizontalShiftRatio, c_defaultMaxHorizontalShiftRatio);
                Verify.AreEqual(parallaxView.HorizontalShift, c_defaultHorizontalShift);
                Verify.AreEqual(parallaxView.IsHorizontalShiftClamped, c_defaultIsHorizontalShiftClamped);
                Verify.AreEqual(parallaxView.VerticalSourceOffsetKind, c_defaultVerticalSourceOffsetKind);
                Verify.AreEqual(parallaxView.VerticalSourceStartOffset, c_defaultVerticalSourceStartOffset);
                Verify.AreEqual(parallaxView.VerticalSourceEndOffset, c_defaultVerticalSourceEndOffset);
                Verify.AreEqual(parallaxView.MaxVerticalShiftRatio, c_defaultMaxVerticalShiftRatio);
                Verify.AreEqual(parallaxView.VerticalShift, c_defaultVerticalShift);
                Verify.AreEqual(parallaxView.IsVerticalShiftClamped, c_defaultIsVerticalShiftClamped);
            });
        }

        [TestMethod]
        [Description("Exercises the ParallaxView property setters and getters for non-default values.")]
        public void VerifyPropertyGettersAndSetters()
        {
            ParallaxView parallaxView = null;
            Rectangle rectangle = null;
            ScrollViewer scrollViewer = null;

            RunOnUIThread.Execute(() =>
            {
                parallaxView = new ParallaxView();
                Verify.IsNotNull(parallaxView);

                rectangle = new Rectangle();
                Verify.IsNotNull(rectangle);

                scrollViewer = new ScrollViewer();
                Verify.IsNotNull(scrollViewer);

                Log.Comment("Setting ParallaxView properties to non-default values");
                parallaxView.Child = rectangle;
                parallaxView.Source = scrollViewer;
                parallaxView.HorizontalSourceOffsetKind = ParallaxSourceOffsetKind.Absolute;
                parallaxView.HorizontalSourceStartOffset = 11.0;
                parallaxView.HorizontalSourceEndOffset = 22.0;
                parallaxView.MaxHorizontalShiftRatio = 0.123;
                parallaxView.HorizontalShift = 321.0;
                parallaxView.IsHorizontalShiftClamped = false;
                parallaxView.VerticalSourceOffsetKind = ParallaxSourceOffsetKind.Absolute;
                parallaxView.VerticalSourceStartOffset = 4.5;
                parallaxView.VerticalSourceEndOffset = 5.4;
                parallaxView.MaxVerticalShiftRatio = 0.321;
                parallaxView.VerticalShift = 45.6;
                parallaxView.IsVerticalShiftClamped = false;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verifying ParallaxView non-default property values");
                Verify.AreEqual(parallaxView.Child, rectangle);
                Verify.AreEqual(parallaxView.Source, scrollViewer);
                Verify.AreEqual(parallaxView.HorizontalSourceOffsetKind, ParallaxSourceOffsetKind.Absolute);
                Verify.AreEqual(parallaxView.HorizontalSourceStartOffset, 11.0);
                Verify.AreEqual(parallaxView.HorizontalSourceEndOffset, 22.0);
                Verify.AreEqual(parallaxView.MaxHorizontalShiftRatio, 0.123);
                Verify.AreEqual(parallaxView.HorizontalShift, 321.0);
                Verify.IsFalse(parallaxView.IsHorizontalShiftClamped);
                Verify.AreEqual(parallaxView.VerticalSourceOffsetKind, ParallaxSourceOffsetKind.Absolute);
                Verify.AreEqual(parallaxView.VerticalSourceStartOffset, 4.5);
                Verify.AreEqual(parallaxView.VerticalSourceEndOffset, 5.4);
                Verify.AreEqual(parallaxView.MaxVerticalShiftRatio, 0.321);
                Verify.AreEqual(parallaxView.VerticalShift, 45.6);
                Verify.IsFalse(parallaxView.IsVerticalShiftClamped);
            });
        }

        [TestMethod]
        [Description("Parallaxes a Rectangle based on a ListView's ScrollViewer.")]
        public void VerifyBasicParallaxingWithListView()
        {
            ParallaxView parallaxView = null;
            Rectangle rectanglePVChild = null;
            ListView listView = null;
            ScrollViewer scrollViewer = null;
            Visual visualPPChild = null;
            AutoResetEvent parallaxViewLoadedEvent = null;
            AutoResetEvent sourceLoadedEvent = null;
            float? finalHorizontalShift = 0f;
            float? finalVerticalShift = 0f;
            double expectedVerticalShift = 0.0;

            RunOnUIThread.Execute(() =>
            {
                parallaxViewLoadedEvent = new AutoResetEvent(false);
                sourceLoadedEvent = new AutoResetEvent(false);
                listView = new ListView();
                rectanglePVChild = new Rectangle();
                parallaxView = new ParallaxView();

                SetupDefaultUIWithListView(
                    parallaxView, rectanglePVChild, listView,
                    parallaxViewLoadedEvent, sourceLoadedEvent);

                parallaxView.VerticalSourceOffsetKind = c_defaultVerticalSourceOffsetKind;
                parallaxView.VerticalSourceStartOffset = c_defaultVerticalSourceStartOffset;
                parallaxView.VerticalSourceEndOffset = c_defaultVerticalSourceEndOffset;
                parallaxView.MaxVerticalShiftRatio = c_defaultMaxVerticalShiftRatio;
                parallaxView.IsVerticalShiftClamped = c_defaultIsVerticalShiftClamped;
            });

            Log.Comment("Waiting for Loaded events");
            parallaxViewLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            sourceLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("Default UI set up");

            RunOnUIThread.Execute(() =>
            {
                // Locate ListView's inner ScrollViewer
                scrollViewer = FindInnerScrollViewer(listView);

                expectedVerticalShift =
                    (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerVerticalOffset) * c_defaultUIVerticalShift /
                    (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio * 2.0 + ((scrollViewer.Content as FrameworkElement).ActualHeight - c_defaultUIScrollViewerHeight));

                Log.Comment("Setting up spy property set");
                visualPPChild = ElementCompositionPreview.GetElementVisual(rectanglePVChild);

                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            ChangeScrollViewerView(
                scrollViewer: scrollViewer,
                horizontalOffset: 0.0,
                verticalOffset: c_defaultUIFinalScrollViewerVerticalOffset,
                zoomFactor: 1.0f,
                disableAnimation: false,
                visualPPChild: visualPPChild,
                finalHorizontalShift: ref finalHorizontalShift,
                finalVerticalShift: ref finalVerticalShift);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Validating the final parallax amount of the target Rectangle.");
                Log.Comment("expectedHorizontalShift=0, finalHorizontalShift={0}", finalHorizontalShift);
                Log.Comment("expectedVerticalShift={0}, finalVerticalShift={1}", -expectedVerticalShift, finalVerticalShift);
                Verify.AreEqual(finalHorizontalShift, 0);
                Verify.IsTrue(Math.Abs((double)finalVerticalShift + expectedVerticalShift) < 0.00001);
            });
        }

        [TestMethod]
        [Description("Parallaxes a Rectangle based on another Rectangle in a ScrollViewer.")]
        public void VerifyBasicParallaxingWithScrollViewer()
        {
            const double expectedHorizontalShift =
                (c_defaultUIScrollViewerWidth * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerHorizontalOffset) * c_defaultUIHorizontalShift /
                (c_defaultUIScrollViewerWidth * c_scrollViewerMaxOverpanRatio * 2.0 + (c_defaultUIScrollViewerContentWidth - c_defaultUIScrollViewerWidth));
            const double expectedVerticalShift =
                (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerVerticalOffset) * c_defaultUIVerticalShift /
                (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio * 2.0 + (c_defaultUIScrollViewerContentHeight - c_defaultUIScrollViewerHeight));

            VerifyParallaxingWithDefaultSourceUI(
                sourceType: SourceType.ScrollViewer,
                horizontalSourceOffsetKind: c_defaultHorizontalSourceOffsetKind,
                horizontalSourceStartOffset: c_defaultHorizontalSourceStartOffset,
                horizontalSourceEndOffset: c_defaultHorizontalSourceEndOffset,
                maxHorizontalShiftRatio: c_defaultMaxHorizontalShiftRatio,
                isHorizontalShiftClamped: c_defaultIsHorizontalShiftClamped,
                expectedHorizontalShift: expectedHorizontalShift,
                verticalSourceOffsetKind: c_defaultVerticalSourceOffsetKind,
                verticalSourceStartOffset: c_defaultVerticalSourceStartOffset,
                verticalSourceEndOffset: c_defaultVerticalSourceEndOffset,
                maxVerticalShiftRatio: c_defaultMaxVerticalShiftRatio,
                isVerticalShiftClamped: c_defaultIsVerticalShiftClamped,
                expectedVerticalShift: expectedVerticalShift);
        }

        [TestMethod]
        [Description("Parallaxes a Rectangle based on another Rectangle in a ScrollPresenter.")]
        public void VerifyBasicParallaxingWithScrollPresenter()
        {
            const double expectedHorizontalShift =
                (c_defaultUIScrollPresenterWidth * c_scrollPresenterMaxOverpanRatio + c_defaultUIFinalScrollPresenterHorizontalOffset) * c_defaultUIHorizontalShift /
                (c_defaultUIScrollPresenterWidth * c_scrollPresenterMaxOverpanRatio * 2.0 + (c_defaultUIScrollPresenterContentWidth - c_defaultUIScrollPresenterWidth));
            const double expectedVerticalShift =
                (c_defaultUIScrollPresenterHeight * c_scrollPresenterMaxOverpanRatio + c_defaultUIFinalScrollPresenterVerticalOffset) * c_defaultUIVerticalShift /
                (c_defaultUIScrollPresenterHeight * c_scrollPresenterMaxOverpanRatio * 2.0 + (c_defaultUIScrollPresenterContentHeight - c_defaultUIScrollPresenterHeight));

            VerifyParallaxingWithDefaultSourceUI(
                sourceType: SourceType.ScrollPresenter,
                horizontalSourceOffsetKind: c_defaultHorizontalSourceOffsetKind,
                horizontalSourceStartOffset: c_defaultHorizontalSourceStartOffset,
                horizontalSourceEndOffset: c_defaultHorizontalSourceEndOffset,
                maxHorizontalShiftRatio: c_defaultMaxHorizontalShiftRatio,
                isHorizontalShiftClamped: c_defaultIsHorizontalShiftClamped,
                expectedHorizontalShift: expectedHorizontalShift,
                verticalSourceOffsetKind: c_defaultVerticalSourceOffsetKind,
                verticalSourceStartOffset: c_defaultVerticalSourceStartOffset,
                verticalSourceEndOffset: c_defaultVerticalSourceEndOffset,
                maxVerticalShiftRatio: c_defaultMaxVerticalShiftRatio,
                isVerticalShiftClamped: c_defaultIsVerticalShiftClamped,
                expectedVerticalShift: expectedVerticalShift);
        }

        [TestMethod]
        [Description("Parallaxes a Rectangle based on another Rectangle in a ScrollViewer, using the Absolute HorizontalSourceOffsetKind/VerticalSourceOffsetKind value.")]
        public void VerifyParallaxingWithAbsoluteExtremes()
        {
            ParallaxView parallaxView = null;
            Rectangle rectanglePVChild = null;
            ScrollViewer scrollViewer = null;
            Rectangle rectangleSVContent = null;
            Visual visualPPChild = null;
            AutoResetEvent parallaxViewLoadedEvent = null;
            AutoResetEvent scrollViewerLoadedEvent = null;
            float? finalHorizontalShift = null;
            float? finalVerticalShift = null;

            RunOnUIThread.Execute(() =>
            {
                parallaxViewLoadedEvent = new AutoResetEvent(false);
                scrollViewerLoadedEvent = new AutoResetEvent(false);
                rectangleSVContent = new Rectangle();
                scrollViewer = new ScrollViewer();
                rectanglePVChild = new Rectangle();
                parallaxView = new ParallaxView();

                SetupDefaultUIWithScrollViewer(
                    parallaxView, rectanglePVChild, scrollViewer, rectangleSVContent,
                    parallaxViewLoadedEvent, scrollViewerLoadedEvent);

                parallaxView.HorizontalSourceOffsetKind = ParallaxSourceOffsetKind.Absolute;
                parallaxView.HorizontalSourceStartOffset = 40.0;
                parallaxView.HorizontalSourceEndOffset = 920.0;
                parallaxView.VerticalSourceOffsetKind = ParallaxSourceOffsetKind.Absolute;
                parallaxView.VerticalSourceStartOffset = -30.0;
                parallaxView.VerticalSourceEndOffset = 560.0;
            });

            Log.Comment("Waiting for Loaded events");
            parallaxViewLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            scrollViewerLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("Default UI set up");

            ChangeScrollViewerView(
                scrollViewer: scrollViewer,
                horizontalOffset: c_defaultUIFinalScrollViewerHorizontalOffset,
                verticalOffset: c_defaultUIFinalScrollViewerVerticalOffset,
                zoomFactor: 1.0f,
                disableAnimation: false,
                visualPPChild: null,
                finalHorizontalShift: ref finalHorizontalShift,
                finalVerticalShift: ref finalVerticalShift);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Setting up spy property set");
                visualPPChild = ElementCompositionPreview.GetElementVisual(rectanglePVChild);

                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                // Validate the final parallax amount of the target Rectangle.
                double expectedHorizontalShift =
                    (scrollViewer.HorizontalOffset - parallaxView.HorizontalSourceStartOffset) * parallaxView.HorizontalShift / (parallaxView.HorizontalSourceEndOffset - parallaxView.HorizontalSourceStartOffset);
                double expectedVerticalShift =
                    (scrollViewer.VerticalOffset - parallaxView.VerticalSourceStartOffset) * parallaxView.VerticalShift / (parallaxView.VerticalSourceEndOffset - parallaxView.VerticalSourceStartOffset);

                float offset;
                CompositionGetValueStatus status;

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, horizontal offset={1}", status, offset);
                Log.Comment("expectedHorizontalShift={0}", -expectedHorizontalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedHorizontalShift, c_shiftTolerance));

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, vertical offset={1}", status, offset);
                Log.Comment("expectedVerticalShift={0}", -expectedVerticalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedVerticalShift, c_shiftTolerance));
            });
        }

        [TestMethod]
        [Description("Parallaxes a Rectangle based on another Rectangle in a ScrollViewer, while ScrollViewer.ZoomMode is Enabled.")]
        public void VerifyParallaxingWithZooming()
        {
            ParallaxView parallaxView = null;
            Rectangle rectanglePVChild = null;
            ScrollViewer scrollViewer = null;
            Rectangle rectangleSVContent = null;
            Visual visualPPChild = null;
            AutoResetEvent parallaxViewLoadedEvent = null;
            AutoResetEvent scrollViewerLoadedEvent = null;
            float? finalHorizontalShift = null;
            float? finalVerticalShift = null;

            RunOnUIThread.Execute(() =>
            {
                parallaxViewLoadedEvent = new AutoResetEvent(false);
                scrollViewerLoadedEvent = new AutoResetEvent(false);
                rectangleSVContent = new Rectangle();
                scrollViewer = new ScrollViewer();
                rectanglePVChild = new Rectangle();
                parallaxView = new ParallaxView();

                SetupDefaultUIWithScrollViewer(
                    parallaxView, rectanglePVChild, scrollViewer, rectangleSVContent,
                    parallaxViewLoadedEvent, scrollViewerLoadedEvent);

                scrollViewer.ZoomMode = Windows.UI.Xaml.Controls.ZoomMode.Enabled;
            });

            Log.Comment("Waiting for Loaded events");
            parallaxViewLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            scrollViewerLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("Default UI set up");

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Setting up spy property set");
                visualPPChild = ElementCompositionPreview.GetElementVisual(rectanglePVChild);

                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                // Validate the initial parallax amount of the target Rectangle.
                const double expectedHorizontalShift =
                    c_defaultUIScrollViewerWidth * c_scrollViewerMaxOverpanRatioWithZoom * c_defaultUIHorizontalShift /
                    (c_defaultUIScrollViewerWidth * c_scrollViewerMaxOverpanRatioWithZoom * 2.0 + (c_defaultUIScrollViewerContentWidth - c_defaultUIScrollViewerWidth));
                const double expectedVerticalShift =
                    c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatioWithZoom * c_defaultUIVerticalShift /
                    (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatioWithZoom * 2.0 + (c_defaultUIScrollViewerContentHeight - c_defaultUIScrollViewerHeight));

                float offset;
                CompositionGetValueStatus status;

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out offset);
                Log.Comment("Target transform before view change: status={0}, horizontal offset={1}", status, offset);
                Log.Comment("expectedHorizontalShift={0}", -expectedHorizontalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedHorizontalShift, c_shiftTolerance));

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out offset);
                Log.Comment("Target transform before view change: status={0}, vertical offset={1}", status, offset);
                Log.Comment("expectedVerticalShift={0}", -expectedVerticalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedVerticalShift, c_shiftTolerance));

                Log.Comment("Start spying again");
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            ChangeScrollViewerView(
                scrollViewer: scrollViewer,
                horizontalOffset: c_defaultUIFinalScrollViewerHorizontalOffset,
                verticalOffset: c_defaultUIFinalScrollViewerVerticalOffset,
                zoomFactor: 2.0f,
                disableAnimation: false,
                visualPPChild: null,
                finalHorizontalShift: ref finalHorizontalShift,
                finalVerticalShift: ref finalVerticalShift);

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                // Validate the final parallax amount of the target Rectangle.
                const double expectedHorizontalShift =
                    (c_defaultUIScrollViewerWidth * c_scrollViewerMaxOverpanRatioWithZoom + c_defaultUIFinalScrollViewerHorizontalOffset) * c_defaultUIHorizontalShift /
                    (c_defaultUIScrollViewerWidth * c_scrollViewerMaxOverpanRatioWithZoom * 2.0 + (c_defaultUIScrollViewerContentWidth * 2.0 - c_defaultUIScrollViewerWidth));
                const double expectedVerticalShift =
                    (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatioWithZoom + c_defaultUIFinalScrollViewerVerticalOffset) * c_defaultUIVerticalShift /
                    (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatioWithZoom * 2.0 + (c_defaultUIScrollViewerContentHeight * 2.0 - c_defaultUIScrollViewerHeight));

                float offset;
                CompositionGetValueStatus status;

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, horizontal offset={1}", status, offset);
                Log.Comment("expectedHorizontalShift={0}", -expectedHorizontalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedHorizontalShift, c_shiftTolerance));

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, vertical offset={1}", status, offset);
                Log.Comment("expectedVerticalShift={0}", -expectedVerticalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedVerticalShift, c_shiftTolerance));
            });
        }

        [TestMethod]
        [Description("Parallaxes a Rectangle based on another Rectangle in a ScrollViewer, using the MaxHorizontalShiftRatio/MaxVerticalShiftRatio properties.")]
        public void VerifyParallaxingWithMaxRatios()
        {
            const double maxHorizontalShiftRatio = 0.01;
            const double maxVerticalShiftRatio = 0.02;

            const double expectedHorizontalShift =
                (c_defaultUIScrollViewerWidth * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerHorizontalOffset) * maxHorizontalShiftRatio;
            const double expectedVerticalShift =
                (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerVerticalOffset) * maxVerticalShiftRatio;

            VerifyParallaxingWithDefaultSourceUI(
                sourceType: SourceType.ScrollViewer,
                horizontalSourceOffsetKind: c_defaultHorizontalSourceOffsetKind,
                horizontalSourceStartOffset: c_defaultHorizontalSourceStartOffset,
                horizontalSourceEndOffset: c_defaultHorizontalSourceEndOffset,
                maxHorizontalShiftRatio: maxHorizontalShiftRatio,
                isHorizontalShiftClamped: c_defaultIsHorizontalShiftClamped,
                expectedHorizontalShift: expectedHorizontalShift,
                verticalSourceOffsetKind: c_defaultVerticalSourceOffsetKind,
                verticalSourceStartOffset: c_defaultVerticalSourceStartOffset,
                verticalSourceEndOffset: c_defaultVerticalSourceEndOffset,
                maxVerticalShiftRatio: maxVerticalShiftRatio,
                isVerticalShiftClamped: c_defaultIsVerticalShiftClamped,
                expectedVerticalShift: expectedVerticalShift);
        }

        [TestMethod]
        [Description("Parallaxes a Rectangle based on another Rectangle in a ScrollViewer, without clamping, using the IsHorizontalShiftClamped/IsVerticalShiftClamped properties.")]
        public void VerifyParallaxingWithoutClamping()
        {
            const double horizontalSourceStartOffset = 40.0;
            const double horizontalSourceEndOffset = 60.0;
            const double maxHorizontalShiftRatio = 5.0;
            const double verticalSourceStartOffset = 10.0;
            const double verticalSourceEndOffset = 30.0;
            const double maxVerticalShiftRatio = 2.5;

            const double expectedHorizontalShift =
                (c_defaultUIFinalScrollViewerHorizontalOffset - horizontalSourceStartOffset) * c_defaultUIHorizontalShift / (horizontalSourceEndOffset - horizontalSourceStartOffset);
            const double expectedVerticalShift =
                (c_defaultUIFinalScrollViewerVerticalOffset - verticalSourceStartOffset) * c_defaultUIVerticalShift / (verticalSourceEndOffset - verticalSourceStartOffset);

            VerifyParallaxingWithDefaultSourceUI(
                sourceType: SourceType.ScrollViewer,
                horizontalSourceOffsetKind: ParallaxSourceOffsetKind.Absolute,
                horizontalSourceStartOffset: horizontalSourceStartOffset,
                horizontalSourceEndOffset: horizontalSourceEndOffset,
                maxHorizontalShiftRatio: maxHorizontalShiftRatio,
                isHorizontalShiftClamped: false,
                expectedHorizontalShift: expectedHorizontalShift,
                verticalSourceOffsetKind: ParallaxSourceOffsetKind.Absolute,
                verticalSourceStartOffset: verticalSourceStartOffset,
                verticalSourceEndOffset: verticalSourceEndOffset,
                maxVerticalShiftRatio: maxVerticalShiftRatio,
                isVerticalShiftClamped: false,
                expectedVerticalShift: expectedVerticalShift);
        }

        [TestMethod]
        [Description("Parallaxes a Rectangle based on another Rectangle in a ScrollViewer, with clamping, using the IsHorizontalShiftClamped/IsVerticalShiftClamped properties.")]
        public void VerifyParallaxingWithClamping()
        {
            VerifyParallaxingWithDefaultSourceUI(
                sourceType: SourceType.ScrollViewer,
                horizontalSourceOffsetKind: ParallaxSourceOffsetKind.Absolute,
                horizontalSourceStartOffset: 40.0,
                horizontalSourceEndOffset: 60.0,
                maxHorizontalShiftRatio: 5.0 ,
                isHorizontalShiftClamped: true,
                expectedHorizontalShift: c_defaultUIHorizontalShift,
                verticalSourceOffsetKind: ParallaxSourceOffsetKind.Absolute,
                verticalSourceStartOffset: 10.0,
                verticalSourceEndOffset: 30.0,
                maxVerticalShiftRatio: 2.5,
                isVerticalShiftClamped: true,
                expectedVerticalShift: c_defaultUIVerticalShift);
        }

        [TestMethod]
        [Description("Parallaxes a Rectangle based on another Rectangle in a ScrollViewer, and changes the sizes of the ScrollViewer & its content.")]
        public void VerifyParallaxingWithSizeChanges()
        {
            ParallaxView parallaxView = null;
            Rectangle rectanglePVChild = null;
            ScrollViewer scrollViewer = null;
            Rectangle rectangleSVContent = null;
            Visual visualPPChild = null;
            AutoResetEvent parallaxViewLoadedEvent = null;
            AutoResetEvent scrollViewerLoadedEvent = null;
            float? finalHorizontalShift = null;
            float? finalVerticalShift = null;

            RunOnUIThread.Execute(() =>
            {
                parallaxViewLoadedEvent = new AutoResetEvent(false);
                scrollViewerLoadedEvent = new AutoResetEvent(false);
                rectangleSVContent = new Rectangle();
                scrollViewer = new ScrollViewer();
                rectanglePVChild = new Rectangle();
                parallaxView = new ParallaxView();

                SetupDefaultUIWithScrollViewer(
                    parallaxView, rectanglePVChild, scrollViewer, rectangleSVContent,
                    parallaxViewLoadedEvent, scrollViewerLoadedEvent);
            });

            Log.Comment("Waiting for Loaded events");
            parallaxViewLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            scrollViewerLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("Default UI set up");

            ChangeScrollViewerView(
                scrollViewer: scrollViewer,
                horizontalOffset: c_defaultUIFinalScrollViewerHorizontalOffset,
                verticalOffset: c_defaultUIFinalScrollViewerVerticalOffset,
                zoomFactor: 1.0f,
                disableAnimation: false,
                visualPPChild: null,
                finalHorizontalShift: ref finalHorizontalShift,
                finalVerticalShift: ref finalVerticalShift);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Setting up spy property set");
                visualPPChild = ElementCompositionPreview.GetElementVisual(rectanglePVChild);

                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                // Validate the parallax amount of the target Rectangle.
                const double expectedHorizontalShift =
                    (c_defaultUIScrollViewerWidth * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerHorizontalOffset) * c_defaultUIHorizontalShift /
                    (c_defaultUIScrollViewerWidth * c_scrollViewerMaxOverpanRatio * 2.0 + (c_defaultUIScrollViewerContentWidth - c_defaultUIScrollViewerWidth));
                const double expectedVerticalShift =
                    (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerVerticalOffset) * c_defaultUIVerticalShift /
                    (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio * 2.0 + (c_defaultUIScrollViewerContentHeight - c_defaultUIScrollViewerHeight));

                float offset;
                CompositionGetValueStatus status;

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, horizontal offset={1}", status, offset);
                Log.Comment("expectedHorizontalShift={0}", -expectedHorizontalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedHorizontalShift, c_shiftTolerance));

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, vertical offset={1}", status, offset);
                Log.Comment("expectedVerticalShift={0}", -expectedVerticalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedVerticalShift, c_shiftTolerance));

                Log.Comment("Decreasing the ScrollViewer's Width and increasing its Height");
                scrollViewer.Width -= 20.0;
                scrollViewer.Height += 20.0;

                Log.Comment("Setting up spy property set");
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                const double expectedHorizontalShift =
                    ((c_defaultUIScrollViewerWidth - 20.0) * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerHorizontalOffset) * c_defaultUIHorizontalShift /
                    ((c_defaultUIScrollViewerWidth - 20.0) * c_scrollViewerMaxOverpanRatio * 2.0 + (c_defaultUIScrollViewerContentWidth - (c_defaultUIScrollViewerWidth - 20.0)));
                const double expectedVerticalShift =
                    ((c_defaultUIScrollViewerHeight + 20.0) * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerVerticalOffset) * c_defaultUIVerticalShift /
                    ((c_defaultUIScrollViewerHeight + 20.0) * c_scrollViewerMaxOverpanRatio * 2.0 + (c_defaultUIScrollViewerContentHeight - (c_defaultUIScrollViewerHeight + 20.0)));

                // Validate the parallax amount of the target Rectangle.
                float offset;
                CompositionGetValueStatus status;

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, horizontal offset={1}", status, offset);
                Log.Comment("expectedHorizontalShift={0}", -expectedHorizontalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedHorizontalShift, c_shiftTolerance));

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, vertical offset={1}", status, offset);
                Log.Comment("expectedVerticalShift={0}", -expectedVerticalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedVerticalShift, c_shiftTolerance));

                Log.Comment("Decreasing the ScrollViewer.Content's Width and increasing its Height");
                rectangleSVContent.Width -= 20.0;
                rectangleSVContent.Height += 20.0;

                Log.Comment("Setting up spy property set");
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                const double expectedHorizontalShift =
                    ((c_defaultUIScrollViewerWidth - 20.0) * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerHorizontalOffset) * c_defaultUIHorizontalShift /
                    ((c_defaultUIScrollViewerWidth - 20.0) * c_scrollViewerMaxOverpanRatio * 2.0 + ((c_defaultUIScrollViewerContentWidth - 20.0) - (c_defaultUIScrollViewerWidth - 20.0)));
                const double expectedVerticalShift =
                    ((c_defaultUIScrollViewerHeight + 20.0) * c_scrollViewerMaxOverpanRatio + c_defaultUIFinalScrollViewerVerticalOffset) * c_defaultUIVerticalShift /
                    ((c_defaultUIScrollViewerHeight + 20.0) * c_scrollViewerMaxOverpanRatio * 2.0 + ((c_defaultUIScrollViewerContentHeight + 20.0) - (c_defaultUIScrollViewerHeight + 20.0)));

                // Validate the parallax amount of the target Rectangle.
                float offset;
                CompositionGetValueStatus status;

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, horizontal offset={1}", status, offset);
                Log.Comment("expectedHorizontalShift={0}", -expectedHorizontalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedHorizontalShift, c_shiftTolerance));

                status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after view change: status={0}, vertical offset={1}", status, offset);
                Log.Comment("expectedVerticalShift={0}", -expectedVerticalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedVerticalShift, c_shiftTolerance));
            });
        }

        [TestMethod]
        [Description("Regression test for MSFT:11812935 - Validates that event handlers in ScrollInputHelper are unhooked when the ParallaxView element is disposed. Changes the ScrollViewer.Content size after the ParallaxView's garbage-collection.")]
        public void ChangeSizeOfDisconnectedScrollViewerContent()
        {
            ChangePropertyAfterParallaxViewDisposal(PropertyId.ScrollViewerContentHeight);
        }

        [TestMethod]
        [Description("Validates that event handlers in ParallaxView element are unhooked when it is disposed. Changes the old ParallaxView.Child's VerticalAlignment after the ParallaxView's garbage-collection.")]
        public void ChangeAlignmentOfDisconnectedParallaxViewChild()
        {
            ChangePropertyAfterParallaxViewDisposal(PropertyId.ParallaxViewChildVerticalAlignment);
        }

        [TestMethod]
        [Description("Basic parallaxing of a ParallaxView inside a ScrollViewer.")]
        public void VerifyBasicParallaxingInsideScrollViewer()
        {
            double expectedVerticalShift =
                c_defaultUIVerticalShift * (1.0 - (c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio / ((2.0 * c_scrollViewerMaxOverpanRatio + 1.5) * c_defaultUIScrollViewerHeight)));

            VerifyParallaxingWithParallaxViewInsideScrollViewer(
                verticalSourceOffsetKind: ParallaxSourceOffsetKind.Relative,
                verticalSourceStartOffset: 0.0,
                verticalSourceEndOffset: 0.0,
                maxVerticalShiftRatio: 1.0,
                verticalParallaxViewOffset: 0.0,
                isVerticalShiftClamped: true,
                expectedVerticalShift: expectedVerticalShift);

            const double verticalParallaxViewOffset = 75.0;
            expectedVerticalShift =
                c_defaultUIVerticalShift * (1.0 - ((c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio + verticalParallaxViewOffset) / ((2.0 * c_scrollViewerMaxOverpanRatio + 1.5) * c_defaultUIScrollViewerHeight)));

            VerifyParallaxingWithParallaxViewInsideScrollViewer(
                verticalSourceOffsetKind: ParallaxSourceOffsetKind.Relative,
                verticalSourceStartOffset: 0.0,
                verticalSourceEndOffset: 0.0,
                maxVerticalShiftRatio: 1.0,
                verticalParallaxViewOffset: verticalParallaxViewOffset,
                isVerticalShiftClamped: true,
                expectedVerticalShift: expectedVerticalShift);
        }

        [TestMethod]
        [Description("Basic parallaxing of a ParallaxView inside a ScrollPresenter.")]
        public void VerifyBasicParallaxingInsideScrollPresenter()
        {
            VerifyParallaxingWithParallaxViewInsideScrollPresenter(
                scrollPresenterHorizontalOffset: 0.0,
                scrollPresenterVerticalOffset: 0.0,
                expectedHorizontalShift: c_defaultUIHorizontalShift / 4.5,
                expectedVerticalShift: c_defaultUIVerticalShift / 4.5);

            VerifyParallaxingWithParallaxViewInsideScrollPresenter(
                scrollPresenterHorizontalOffset: c_defaultUIFinalScrollPresenterHorizontalOffset,
                scrollPresenterVerticalOffset: c_defaultUIFinalScrollPresenterVerticalOffset,
                expectedHorizontalShift: (c_defaultUIHorizontalShift + c_defaultUIFinalScrollPresenterHorizontalOffset / 4.0) / 4.5,
                expectedVerticalShift: (c_defaultUIVerticalShift + c_defaultUIFinalScrollPresenterVerticalOffset / 4.0) / 4.5);
        }

        [TestMethod]
        [Description("Basic parallaxing of a ParallaxView used as a header inside a ScrollViewer.")]
        public void VerifyHeaderParallaxingInsideScrollViewer()
        {
            VerifyParallaxingWithParallaxViewInsideScrollViewer(
                verticalSourceOffsetKind: ParallaxSourceOffsetKind.Absolute,
                verticalSourceStartOffset: 0.0,
                verticalSourceEndOffset: c_defaultUIScrollViewerHeight / 2.0,
                maxVerticalShiftRatio: 1.0,
                verticalParallaxViewOffset: 0.0,
                isVerticalShiftClamped: true,
                expectedVerticalShift: c_defaultUIVerticalShift);
        }

        [TestMethod]
        [Description("Changing ParallaxView offset inside a ScrollViewer.")]
        public void VerifyParallaxViewOffsetChangeInsideScrollViewer()
        {
            const double verticalParallaxViewOffset1 = 15.0;
            const double verticalParallaxViewOffset2 = 75.0;
            const double expectedVerticalShift1 =
                c_defaultUIVerticalShift * (1.0 - ((c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio + verticalParallaxViewOffset1) / ((2.0 * c_scrollViewerMaxOverpanRatio + 1.5) * c_defaultUIScrollViewerHeight)));
            const double expectedVerticalShift2 =
                c_defaultUIVerticalShift * (1.0 - ((c_defaultUIScrollViewerHeight * c_scrollViewerMaxOverpanRatio + verticalParallaxViewOffset2) / ((2.0 * c_scrollViewerMaxOverpanRatio + 1.5) * c_defaultUIScrollViewerHeight)));

            ParallaxView parallaxView = null;
            Rectangle rectanglePVChild = null;
            ScrollViewer scrollViewer = null;
            StackPanel stackPanelSVContent = null;
            Visual visualPPChild = null;
            AutoResetEvent parallaxViewLoadedEvent = null;
            AutoResetEvent scrollViewerLoadedEvent = null;
            float? finalHorizontalShift = null;
            float? finalVerticalShift = 0f;

            RunOnUIThread.Execute(() =>
            {
                parallaxViewLoadedEvent = new AutoResetEvent(false);
                scrollViewerLoadedEvent = new AutoResetEvent(false);
                stackPanelSVContent = new StackPanel();
                scrollViewer = new ScrollViewer();
                rectanglePVChild = new Rectangle();
                parallaxView = new ParallaxView();

                SetupUIWithParallaxViewInsideScrollViewer(
                    verticalParallaxViewOffset1,
                    parallaxView, rectanglePVChild, scrollViewer, stackPanelSVContent,
                    parallaxViewLoadedEvent, scrollViewerLoadedEvent);
            });

            Log.Comment("Waiting for Loaded events");
            parallaxViewLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            scrollViewerLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("Default UI set up");

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scrollViewer.ActualWidth, c_defaultUIScrollViewerWidth);
                Verify.AreEqual(scrollViewer.ActualHeight, c_defaultUIScrollViewerHeight);
                Verify.AreEqual(stackPanelSVContent.ActualWidth, c_defaultUIScrollViewerWidth);
                Verify.AreEqual(stackPanelSVContent.ActualHeight, c_defaultUIScrollViewerHeight * 3.5 + verticalParallaxViewOffset1);
                Verify.AreEqual(parallaxView.ActualWidth, c_defaultUIScrollViewerWidth);
                Verify.AreEqual(parallaxView.ActualHeight, c_defaultUIScrollViewerHeight / 2.0);
                Verify.AreEqual(rectanglePVChild.ActualWidth, c_defaultUIScrollViewerWidth);
                Verify.AreEqual(rectanglePVChild.ActualHeight, c_defaultUIScrollViewerHeight / 2.0 + c_defaultUIVerticalShift);

                Log.Comment("Setting up spy property set");
                visualPPChild = ElementCompositionPreview.GetElementVisual(rectanglePVChild);

                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            ChangeScrollViewerView(
                scrollViewer: scrollViewer,
                horizontalOffset: null,
                verticalOffset: c_defaultUIFinalScrollViewerVerticalOffset,
                zoomFactor: null,
                disableAnimation: false,
                visualPPChild: visualPPChild,
                finalHorizontalShift: ref finalHorizontalShift,
                finalVerticalShift: ref finalVerticalShift);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Validating the parallax amount of the target Rectangle.");
                Log.Comment("expectedVerticalShift1={0}, finalVerticalShift={1}", -expectedVerticalShift1, finalVerticalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance((float)finalVerticalShift, (float)-expectedVerticalShift1, c_shiftTolerance));

                Log.Comment("Changing the vertical offset of the ParallaxView within the ScrollViewer.Content");
                Rectangle rectangle = stackPanelSVContent.Children[0] as Rectangle;
                rectangle.Height = verticalParallaxViewOffset2;

                Log.Comment("Invoking ParallaxView.RefreshAutomaticVerticalOffsets() to update the vertical shift.");
                parallaxView.RefreshAutomaticVerticalOffsets();

                Log.Comment("Start spying again");
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(stackPanelSVContent.ActualHeight, c_defaultUIScrollViewerHeight * 3.5 + verticalParallaxViewOffset2);

                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                // Validate the final parallax amount of the target Rectangle.
                float offset;
                CompositionGetValueStatus status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out offset);
                Log.Comment("Target transform after offset change: status={0}, vertical offset={1}", status, offset);
                Log.Comment("expectedVerticalShift2={0}", -expectedVerticalShift2);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance(offset, (float)-expectedVerticalShift2, c_shiftTolerance));
            });
        }

        private void VerifyParallaxingWithDefaultSourceUI(
            SourceType sourceType,
            ParallaxSourceOffsetKind horizontalSourceOffsetKind,
            double horizontalSourceStartOffset,
            double horizontalSourceEndOffset,
            double maxHorizontalShiftRatio,
            bool isHorizontalShiftClamped,
            double expectedHorizontalShift,
            ParallaxSourceOffsetKind verticalSourceOffsetKind,
            double verticalSourceStartOffset,
            double verticalSourceEndOffset,
            double maxVerticalShiftRatio,
            bool isVerticalShiftClamped,
            double expectedVerticalShift)
        {
            ParallaxView parallaxView = null;
            Rectangle rectanglePVChild = null;
            ScrollViewer scrollViewer = null;
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleSVContent = null;
            Visual visualPPChild = null;
            AutoResetEvent parallaxViewLoadedEvent = null;
            AutoResetEvent sourceLoadedEvent = null;
            float? finalHorizontalShift = 0f;
            float? finalVerticalShift = 0f;

            RunOnUIThread.Execute(() =>
            {
                parallaxViewLoadedEvent = new AutoResetEvent(false);
                sourceLoadedEvent = new AutoResetEvent(false);
                rectangleSVContent = new Rectangle();
                rectanglePVChild = new Rectangle();
                parallaxView = new ParallaxView();

                if (sourceType == SourceType.ScrollViewer)
                {
                    scrollViewer = new ScrollViewer();
                    SetupDefaultUIWithScrollViewer(
                        parallaxView, rectanglePVChild, scrollViewer, rectangleSVContent,
                        parallaxViewLoadedEvent, sourceLoadedEvent);
                }
                else
                {
                    scrollPresenter = new ScrollPresenter();
                    SetupDefaultUIWithScrollPresenter(
                        parallaxView, rectanglePVChild, scrollPresenter, rectangleSVContent,
                        parallaxViewLoadedEvent, sourceLoadedEvent);
                }

                parallaxView.HorizontalSourceOffsetKind = horizontalSourceOffsetKind;
                parallaxView.HorizontalSourceStartOffset = horizontalSourceStartOffset;
                parallaxView.HorizontalSourceEndOffset = horizontalSourceEndOffset;
                parallaxView.MaxHorizontalShiftRatio = maxHorizontalShiftRatio;
                parallaxView.IsHorizontalShiftClamped = isHorizontalShiftClamped;
                parallaxView.VerticalSourceOffsetKind = verticalSourceOffsetKind;
                parallaxView.VerticalSourceStartOffset = verticalSourceStartOffset;
                parallaxView.VerticalSourceEndOffset = verticalSourceEndOffset;
                parallaxView.MaxVerticalShiftRatio = maxVerticalShiftRatio;
                parallaxView.IsVerticalShiftClamped = isVerticalShiftClamped;
            });

            Log.Comment("Waiting for Loaded events");
            parallaxViewLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            sourceLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("Default UI set up");

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Setting up spy property set");
                visualPPChild = ElementCompositionPreview.GetElementVisual(rectanglePVChild);

                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());

                if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                {
                    // visualPPChild.TransformMatrix.M41 & M42 are expected to be 0.
                    Matrix4x4 m = visualPPChild.TransformMatrix;
                    Log.Comment("Target TransformMatrix prior to view change: {0}, {1}", m.M41, m.M42);
                    Verify.AreEqual(m.M41, 0.0f);
                    Verify.AreEqual(m.M42, 0.0f);
                }
            });

            if (sourceType == SourceType.ScrollViewer)
            {
                ChangeScrollViewerView(
                    scrollViewer: scrollViewer,
                    horizontalOffset: c_defaultUIFinalScrollViewerHorizontalOffset,
                    verticalOffset: c_defaultUIFinalScrollViewerVerticalOffset,
                    zoomFactor: 1.0f,
                    disableAnimation: false,
                    visualPPChild: visualPPChild,
                    finalHorizontalShift: ref finalHorizontalShift,
                    finalVerticalShift: ref finalVerticalShift);
            }
            else
            {
                ChangeScrollPresenterView(
                    scrollPresenter: scrollPresenter,
                    horizontalOffset: c_defaultUIFinalScrollPresenterHorizontalOffset,
                    verticalOffset: c_defaultUIFinalScrollPresenterVerticalOffset,
                    zoomFactor: 1.0f,
                    disableAnimation: false,
                    visualPPChild: visualPPChild,
                    finalHorizontalShift: ref finalHorizontalShift,
                    finalVerticalShift: ref finalVerticalShift);
            }

            RunOnUIThread.Execute(() =>
            {
                if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                {
                    // visualPPChild.TransformMatrix.M41 & M42 are still being animated, so their reported values are expected to be 0.
                    Matrix4x4 m = visualPPChild.TransformMatrix;
                    Log.Comment("Target TransformMatrix after view change: {0}, {1}", m.M41, m.M42);
                    Verify.AreEqual(m.M41, 0.0f);
                    Verify.AreEqual(m.M42, 0.0f);
                }

                Log.Comment("Validating the final parallax amount of the target Rectangle.");
                Log.Comment("expectedHorizontalShift={0}, finalHorizontalShift={1}", -expectedHorizontalShift, finalHorizontalShift);
                Log.Comment("expectedVerticalShift={0}, finalVerticalShift={1}", -expectedVerticalShift, finalVerticalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance((float)finalHorizontalShift, (float)-expectedHorizontalShift, c_shiftTolerance));
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance((float)finalVerticalShift, (float)-expectedVerticalShift, c_shiftTolerance));
            });
        }

        private void VerifyParallaxingWithParallaxViewInsideScrollViewer(
            ParallaxSourceOffsetKind verticalSourceOffsetKind,
            double verticalSourceStartOffset,
            double verticalSourceEndOffset,
            double maxVerticalShiftRatio,
            double verticalParallaxViewOffset,
            bool isVerticalShiftClamped,
            double expectedVerticalShift)
        {
            ParallaxView parallaxView = null;
            Rectangle rectanglePVChild = null;
            ScrollViewer scrollViewer = null;
            StackPanel stackPanelSVContent = null;
            Visual visualPPChild = null;
            AutoResetEvent parallaxViewLoadedEvent = null;
            AutoResetEvent scrollViewerLoadedEvent = null;
            float? finalHorizontalShift = null;
            float? finalVerticalShift = 0f;

            RunOnUIThread.Execute(() =>
            {
                parallaxViewLoadedEvent = new AutoResetEvent(false);
                scrollViewerLoadedEvent = new AutoResetEvent(false);
                stackPanelSVContent = new StackPanel();
                scrollViewer = new ScrollViewer();
                rectanglePVChild = new Rectangle();
                parallaxView = new ParallaxView();

                SetupUIWithParallaxViewInsideScrollViewer(
                    verticalParallaxViewOffset,
                    parallaxView, rectanglePVChild, scrollViewer, stackPanelSVContent,
                    parallaxViewLoadedEvent, scrollViewerLoadedEvent);

                parallaxView.VerticalSourceOffsetKind = verticalSourceOffsetKind;
                parallaxView.VerticalSourceStartOffset = verticalSourceStartOffset;
                parallaxView.VerticalSourceEndOffset = verticalSourceEndOffset;
                parallaxView.MaxVerticalShiftRatio = maxVerticalShiftRatio;
                parallaxView.IsVerticalShiftClamped = isVerticalShiftClamped;
            });

            Log.Comment("Waiting for Loaded events");
            parallaxViewLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            scrollViewerLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("Default UI set up");

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scrollViewer.ActualWidth, c_defaultUIScrollViewerWidth);
                Verify.AreEqual(scrollViewer.ActualHeight, c_defaultUIScrollViewerHeight);
                Verify.AreEqual(stackPanelSVContent.ActualWidth, c_defaultUIScrollViewerWidth);
                Verify.AreEqual(stackPanelSVContent.ActualHeight, c_defaultUIScrollViewerHeight * 3.5 + verticalParallaxViewOffset);
                Verify.AreEqual(parallaxView.ActualWidth, c_defaultUIScrollViewerWidth);
                Verify.AreEqual(parallaxView.ActualHeight, c_defaultUIScrollViewerHeight / 2.0);
                Verify.AreEqual(rectanglePVChild.ActualWidth, c_defaultUIScrollViewerWidth);
                Verify.AreEqual(rectanglePVChild.ActualHeight, c_defaultUIScrollViewerHeight / 2.0 + c_defaultUIVerticalShift);

                Log.Comment("Setting up spy property set");
                visualPPChild = ElementCompositionPreview.GetElementVisual(rectanglePVChild);

                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());

                if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                {
                    // visualPPChild.TransformMatrix.M41 & M42 are expected to be 0.
                    Matrix4x4 m = visualPPChild.TransformMatrix;
                    Log.Comment("Target TransformMatrix prior to view change: {0}, {1}", m.M41, m.M42);
                    Verify.AreEqual(m.M41, 0.0f);
                    Verify.AreEqual(m.M42, 0.0f);
                }
            });

            ChangeScrollViewerView(
                scrollViewer: scrollViewer,
                horizontalOffset: null,
                verticalOffset: c_defaultUIFinalScrollViewerVerticalOffset,
                zoomFactor: null,
                disableAnimation: false,
                visualPPChild: visualPPChild,
                finalHorizontalShift: ref finalHorizontalShift,
                finalVerticalShift: ref finalVerticalShift);

            RunOnUIThread.Execute(() =>
            {
                if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                {
                    // visualPPChild.TransformMatrix.M42 is still being animated, so its reported value is expected to be 0.
                    Matrix4x4 m = visualPPChild.TransformMatrix;
                    Log.Comment("Target TransformMatrix after view change: {0}, {1}", m.M41, m.M42);
                    Verify.AreEqual(m.M41, 0.0f);
                    Verify.AreEqual(m.M42, 0.0f);
                }

                Log.Comment("Validating the final parallax amount of the target Rectangle.");
                Log.Comment("expectedVerticalShift={0}, finalVerticalShift={1}", -expectedVerticalShift, finalVerticalShift);
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance((float)finalVerticalShift, (float)-expectedVerticalShift, c_shiftTolerance));
            });
        }

        private void VerifyParallaxingWithParallaxViewInsideScrollPresenter(
            double scrollPresenterHorizontalOffset,
            double scrollPresenterVerticalOffset,
            double expectedHorizontalShift,
            double expectedVerticalShift)
        {
            ParallaxView parallaxView = null;
            Rectangle rectanglePVChild = null;
            ScrollPresenter scrollPresenter = null;
            Visual visualPPChild = null;
            AutoResetEvent parallaxViewLoadedEvent = null;
            AutoResetEvent scrollPresenterLoadedEvent = null;
            float? finalHorizontalShift = 0f;
            float? finalVerticalShift = 0f;

            RunOnUIThread.Execute(() =>
            {
                parallaxViewLoadedEvent = new AutoResetEvent(false);
                scrollPresenterLoadedEvent = new AutoResetEvent(false);
                scrollPresenter = new ScrollPresenter();
                rectanglePVChild = new Rectangle();
                parallaxView = new ParallaxView();

                SetupUIWithParallaxViewInsideScrollPresenter(
                    parallaxView,
                    rectanglePVChild,
                    scrollPresenter,
                    parallaxViewLoadedEvent,
                    scrollPresenterLoadedEvent);
            });

            Log.Comment("Waiting for Loaded events");
            parallaxViewLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            scrollPresenterLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("Default UI set up");

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scrollPresenter Actual Size={0}, {1}", scrollPresenter.ActualWidth, scrollPresenter.ActualHeight);
                Log.Comment("parallaxView Actual Size={0}, {1}", parallaxView.ActualWidth, parallaxView.ActualHeight);
                Log.Comment("rectanglePVChild Actual Size={0}, {1}", rectanglePVChild.ActualWidth, rectanglePVChild.ActualHeight);

                Verify.AreEqual(scrollPresenter.ActualWidth, c_defaultUIScrollPresenterWidth);
                Verify.AreEqual(scrollPresenter.ActualHeight, c_defaultUIScrollPresenterHeight);

                Verify.AreEqual(parallaxView.ActualWidth, c_defaultUIScrollPresenterContentWidth);
                Verify.AreEqual(parallaxView.ActualHeight, c_defaultUIScrollPresenterContentHeight);
                
                Verify.AreEqual(rectanglePVChild.ActualWidth, c_defaultUIScrollPresenterContentWidth + c_defaultUIHorizontalShift);
                Verify.AreEqual(rectanglePVChild.ActualHeight, c_defaultUIScrollPresenterContentHeight + c_defaultUIVerticalShift);

                Log.Comment("Setting up spy property set");
                visualPPChild = ElementCompositionPreview.GetElementVisual(rectanglePVChild);

                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                CompositionPropertySpy.StartSpyingScalarProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
            });

            if (scrollPresenterHorizontalOffset != 0.0 || scrollPresenterVerticalOffset != 0.0)
            {
                ChangeScrollPresenterView(
                    scrollPresenter: scrollPresenter,
                    horizontalOffset: scrollPresenterHorizontalOffset,
                    verticalOffset: scrollPresenterVerticalOffset,
                    zoomFactor: null,
                    disableAnimation: false,
                    visualPPChild: visualPPChild,
                    finalHorizontalShift: ref finalHorizontalShift,
                    finalVerticalShift: ref finalVerticalShift);
            }
            else
            {
                Log.Comment("Waiting for captured properties to be updated");
                CompositionPropertySpy.SynchronouslyTickUIThread(10);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Cancelling spying");
                    CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                    CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
                });

                Log.Comment("Waiting for captured properties to be updated");
                CompositionPropertySpy.SynchronouslyTickUIThread(10);

                float horizontalShift = 0f;
                float verticalShift = 0f;

                RunOnUIThread.Execute(() =>
                {
                    CompositionGetValueStatus status;

                    Log.Comment("Accessing final shift values");

                        status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out horizontalShift);
                        Log.Comment("Target transform after view change: status={0}, horizontal offset={1}", status, horizontalShift);
                        status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out verticalShift);
                        Log.Comment("Target transform after view change: status={0}, vertical offset={1}", status, verticalShift);
                });

                finalHorizontalShift = horizontalShift;
                finalVerticalShift = verticalShift;
            }

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Validating the final parallax amount of the target Rectangle.");
                Log.Comment("expectedHorizontalShift={0}, finalHorizontalShift={1}", -expectedHorizontalShift, finalHorizontalShift);
                Log.Comment("expectedVerticalShift={0}, finalVerticalShift={1}", -expectedVerticalShift, finalVerticalShift);

                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance((float)finalHorizontalShift, (float)-expectedHorizontalShift, c_shiftTolerance));
                Verify.IsTrue(CompositionPropertyLogger.AreFloatsEqualWithTolerance((float)finalVerticalShift, (float)-expectedVerticalShift, c_shiftTolerance));
            });
        }        

        private void SetupDefaultUIWithListView(
            ParallaxView parallaxView,
            Rectangle rectanglePVChild,
            ListView listView,
            AutoResetEvent parallaxViewLoadedEvent,
            AutoResetEvent listViewLoadedEvent)
        {
            Log.Comment("Setting up default UI with ParallaxView and ListView");

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            Verify.IsNotNull(listView);
            if (string.IsNullOrEmpty(listView.Name))
            {
                listView.Name = "listView";
            }
            listView.Width = c_defaultUIScrollViewerWidth;
            listView.Height = c_defaultUIScrollViewerHeight;

            for (int listViewItemIndex = 0; listViewItemIndex < 32; listViewItemIndex++)
            {
                listView.Items.Add("Item #" + listViewItemIndex);
            }

            Verify.IsNotNull(rectanglePVChild);
            rectanglePVChild.Width = c_defaultUIScrollViewerWidth + c_defaultHorizontalShift;
            rectanglePVChild.Height = c_defaultUIScrollViewerHeight + c_defaultVerticalShift;
            rectanglePVChild.Fill = twoColorLGB;

            Verify.IsNotNull(parallaxView);
            parallaxView.Width = c_defaultUIScrollViewerWidth;
            parallaxView.Height = c_defaultUIScrollViewerHeight;
            parallaxView.Child = rectanglePVChild;
            parallaxView.VerticalShift = c_defaultUIVerticalShift;

            StackPanel stackPanel = new StackPanel();
            Verify.IsNotNull(stackPanel);
            stackPanel.Children.Add(parallaxView);
            stackPanel.Children.Add(listView);

            if (parallaxViewLoadedEvent != null)
            {
                parallaxView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ParallaxView.Loaded event handler");
                    parallaxViewLoadedEvent.Set();
                };
            }

            if (listViewLoadedEvent != null)
            {
                listView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ListView.Loaded event handler");
                    parallaxView.Source = listView;
                    listViewLoadedEvent.Set();
                };
            }

            Log.Comment("Setting window content");
            Content = stackPanel;
        }

        private void SetupDefaultUIWithScrollViewer(
            ParallaxView parallaxView,
            Rectangle rectanglePVChild,
            ScrollViewer scrollViewer,
            Rectangle rectangleSVContent,
            AutoResetEvent parallaxViewLoadedEvent,
            AutoResetEvent scrollViewerLoadedEvent)
        {
            Log.Comment("Setting up default UI with ParallaxView, ScrollViewer and Rectangles");

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            Verify.IsNotNull(rectangleSVContent);
            rectangleSVContent.Width = c_defaultUIScrollViewerContentWidth;
            rectangleSVContent.Height = c_defaultUIScrollViewerContentHeight;
            rectangleSVContent.Fill = twoColorLGB;

            Verify.IsNotNull(scrollViewer);
            if (string.IsNullOrEmpty(scrollViewer.Name))
            {
                scrollViewer.Name = "scrollViewer";
            }
            scrollViewer.Width = c_defaultUIScrollViewerWidth;
            scrollViewer.Height = c_defaultUIScrollViewerHeight;
            scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden;
            scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;
            scrollViewer.ZoomMode = Windows.UI.Xaml.Controls.ZoomMode.Disabled;
            scrollViewer.Content = rectangleSVContent;

            Verify.IsNotNull(rectanglePVChild);
            rectanglePVChild.Width = c_defaultUIScrollViewerWidth + c_defaultHorizontalShift;
            rectanglePVChild.Height = c_defaultUIScrollViewerHeight + c_defaultVerticalShift;
            rectanglePVChild.Fill = twoColorLGB;

            Verify.IsNotNull(parallaxView);
            parallaxView.Width = c_defaultUIScrollViewerWidth;
            parallaxView.Height = c_defaultUIScrollViewerHeight;
            parallaxView.Child = rectanglePVChild;
            parallaxView.HorizontalShift = c_defaultUIHorizontalShift;
            parallaxView.VerticalShift = c_defaultUIVerticalShift;

            StackPanel stackPanel = new StackPanel();
            Verify.IsNotNull(stackPanel);
            stackPanel.Children.Add(parallaxView);
            stackPanel.Children.Add(scrollViewer);

            if (parallaxViewLoadedEvent != null)
            {
                parallaxView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ParallaxView.Loaded event handler");
                    parallaxViewLoadedEvent.Set();
                };
            }

            if (scrollViewerLoadedEvent != null)
            {
                scrollViewer.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollViewer.Loaded event handler");
                    parallaxView.Source = scrollViewer;
                    scrollViewerLoadedEvent.Set();
                };
            }

            Log.Comment("Setting window content");
            Content = stackPanel;
        }

        private void SetupDefaultUIWithScrollPresenter(
            ParallaxView parallaxView,
            Rectangle rectanglePVChild,
            ScrollPresenter scrollPresenter,
            Rectangle rectangleScrollPresenterContent,
            AutoResetEvent parallaxViewLoadedEvent,
            AutoResetEvent scrollPresenterLoadedEvent)
        {
            Log.Comment("Setting up default UI with ParallaxView, ScrollPresenter and Rectangles");

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            Verify.IsNotNull(rectangleScrollPresenterContent);
            rectangleScrollPresenterContent.Width = c_defaultUIScrollPresenterContentWidth;
            rectangleScrollPresenterContent.Height = c_defaultUIScrollPresenterContentHeight;
            rectangleScrollPresenterContent.Fill = twoColorLGB;

            Verify.IsNotNull(scrollPresenter);
            if (string.IsNullOrEmpty(scrollPresenter.Name))
            {
                scrollPresenter.Name = "scrollPresenter";
            }
            scrollPresenter.Width = c_defaultUIScrollPresenterWidth;
            scrollPresenter.Height = c_defaultUIScrollPresenterHeight;
            scrollPresenter.ZoomMode = ScrollingZoomMode.Disabled;
            scrollPresenter.Content = rectangleScrollPresenterContent;

            Verify.IsNotNull(rectanglePVChild);
            rectanglePVChild.Width = c_defaultUIScrollPresenterWidth + c_defaultHorizontalShift;
            rectanglePVChild.Height = c_defaultUIScrollPresenterHeight + c_defaultVerticalShift;
            rectanglePVChild.Fill = twoColorLGB;

            Verify.IsNotNull(parallaxView);
            parallaxView.Width = c_defaultUIScrollPresenterWidth;
            parallaxView.Height = c_defaultUIScrollPresenterHeight;
            parallaxView.Child = rectanglePVChild;
            parallaxView.HorizontalShift = c_defaultUIHorizontalShift;
            parallaxView.VerticalShift = c_defaultUIVerticalShift;

            StackPanel stackPanel = new StackPanel();
            Verify.IsNotNull(stackPanel);
            stackPanel.Children.Add(parallaxView);
            stackPanel.Children.Add(scrollPresenter);

            if (parallaxViewLoadedEvent != null)
            {
                parallaxView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ParallaxView.Loaded event handler");
                    parallaxViewLoadedEvent.Set();
                };
            }

            if (scrollPresenterLoadedEvent != null)
            {
                scrollPresenter.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollPresenter.Loaded event handler");
                    parallaxView.Source = scrollPresenter;
                    scrollPresenterLoadedEvent.Set();
                };
            }

            Log.Comment("Setting window content");
            Content = stackPanel;
        }

        private void SetupUIWithParallaxViewInsideScrollPresenter(
            ParallaxView parallaxView,
            Rectangle rectanglePVChild,
            ScrollPresenter scrollPresenter,
            AutoResetEvent parallaxViewLoadedEvent,
            AutoResetEvent scrollPresenterLoadedEvent)
        {
            Log.Comment("Setting up UI with ParallaxView inside ScrollPresenter");

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            Verify.IsNotNull(scrollPresenter);
            if (string.IsNullOrEmpty(scrollPresenter.Name))
            {
                scrollPresenter.Name = "scrollPresenter";
            }
            scrollPresenter.Width = c_defaultUIScrollPresenterWidth;
            scrollPresenter.Height = c_defaultUIScrollPresenterHeight;
            scrollPresenter.ZoomMode = ScrollingZoomMode.Disabled;

            Grid gridScrollPresenterContent = new Grid();
            scrollPresenter.Content = gridScrollPresenterContent;

            Verify.IsNotNull(rectanglePVChild);
            rectanglePVChild.Width = c_defaultUIScrollPresenterContentWidth + c_defaultUIHorizontalShift;
            rectanglePVChild.Height = c_defaultUIScrollPresenterContentHeight + c_defaultUIVerticalShift;
            rectanglePVChild.Fill = twoColorLGB;

            Verify.IsNotNull(parallaxView);
            parallaxView.Width = c_defaultUIScrollPresenterContentWidth;
            parallaxView.Height = c_defaultUIScrollPresenterContentHeight;
            parallaxView.Child = rectanglePVChild;
            parallaxView.HorizontalShift = c_defaultUIHorizontalShift;
            parallaxView.VerticalShift = c_defaultUIVerticalShift;

            gridScrollPresenterContent.Children.Add(parallaxView);

            if (parallaxViewLoadedEvent != null)
            {
                parallaxView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ParallaxView.Loaded event handler");
                    parallaxViewLoadedEvent.Set();
                };
            }

            if (scrollPresenterLoadedEvent != null)
            {
                scrollPresenter.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollPresenter.Loaded event handler");
                    parallaxView.Source = scrollPresenter;
                    scrollPresenterLoadedEvent.Set();
                };
            }

            Log.Comment("Setting window content");
            Content = scrollPresenter;
        }

        private void SetupUIWithParallaxViewInsideScrollViewer(
            double verticalParallaxViewOffset,
            ParallaxView parallaxView,
            Rectangle rectanglePVChild,
            ScrollViewer scrollViewer,
            StackPanel stackPanelSVContent,
            AutoResetEvent parallaxViewLoadedEvent,
            AutoResetEvent scrollViewerLoadedEvent)
        {
            Log.Comment("Setting up UI with ParallaxView inside ScrollViewer");

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            Verify.IsNotNull(stackPanelSVContent);
            stackPanelSVContent.Width = c_defaultUIScrollViewerWidth;

            if (verticalParallaxViewOffset > 0.0)
            {
                Rectangle rectangleChild = new Rectangle() { Width = c_defaultUIScrollViewerWidth, Height = verticalParallaxViewOffset, Fill = twoColorLGB };
                stackPanelSVContent.Children.Add(rectangleChild);
            }

            Verify.IsNotNull(rectanglePVChild);
            rectanglePVChild.Width = c_defaultUIScrollViewerWidth;
            rectanglePVChild.Height = c_defaultUIScrollViewerHeight / 2.0 + c_defaultUIVerticalShift;
            rectanglePVChild.Fill = twoColorLGB;

            Verify.IsNotNull(parallaxView);
            parallaxView.Width = c_defaultUIScrollViewerWidth;
            parallaxView.Height = c_defaultUIScrollViewerHeight / 2.0;
            parallaxView.Child = rectanglePVChild;
            parallaxView.VerticalShift = c_defaultUIVerticalShift;
            parallaxView.MaxVerticalShiftRatio = c_defaultMaxVerticalShiftRatio;

            stackPanelSVContent.Children.Add(parallaxView);

            for (int childRect = 0; childRect < 3; childRect++)
            {
                Rectangle rectangleChild = new Rectangle() { Width = c_defaultUIScrollViewerWidth, Height = c_defaultUIScrollViewerHeight, Fill = twoColorLGB };
                stackPanelSVContent.Children.Add(rectangleChild);
            }

            Verify.IsNotNull(scrollViewer);
            scrollViewer.Width = c_defaultUIScrollViewerWidth;
            scrollViewer.Height = c_defaultUIScrollViewerHeight;
            scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden;
            scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;
            scrollViewer.ZoomMode = Windows.UI.Xaml.Controls.ZoomMode.Disabled;
            scrollViewer.Content = stackPanelSVContent;

            if (parallaxViewLoadedEvent != null)
            {
                parallaxView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ParallaxView.Loaded event handler");
                    parallaxViewLoadedEvent.Set();
                };
            }

            if (scrollViewerLoadedEvent != null)
            {
                scrollViewer.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollViewer.Loaded event handler");
                    parallaxView.Source = scrollViewer;
                    scrollViewerLoadedEvent.Set();
                };
            }

            Log.Comment("Setting window content");
            Content = scrollViewer;
        }

        private void ChangePropertyAfterParallaxViewDisposal(PropertyId propertyToChange)
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Bug 12582949 - Skipping failing test on RS1.");
                return;
            }

            ParallaxView parallaxView = null;
            Rectangle rectanglePVChild = null;
            ScrollViewer scrollViewer = null;
            Rectangle rectangleSVContent = null;
            AutoResetEvent parallaxViewLoadedEvent = null;
            AutoResetEvent scrollViewerLoadedEvent = null;

            RunOnUIThread.Execute(() =>
            {
                parallaxViewLoadedEvent = new AutoResetEvent(false);
                scrollViewerLoadedEvent = new AutoResetEvent(false);
                rectangleSVContent = new Rectangle();
                scrollViewer = new ScrollViewer();
                rectanglePVChild = new Rectangle();
                parallaxView = new ParallaxView();

                scrollViewer.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollViewer.Loaded event handler");
                    scrollViewerLoadedEvent.Set();
                };

                SetupDefaultUIWithScrollViewer(
                    parallaxView, rectanglePVChild, scrollViewer, rectangleSVContent,
                    parallaxViewLoadedEvent, scrollViewerLoadedEvent: null);

                parallaxView.Source = scrollViewer;
            });

            Log.Comment("Waiting for Loaded events");
            parallaxViewLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            scrollViewerLoadedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("Default UI set up");

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Disposing the ParallaxView");
                (Content as StackPanel).Children.Remove(parallaxView);
                WeakReference parallaxViewWeakReference = new WeakReference(parallaxView);
                parallaxView = null;
                Log.Comment("Garbage-collecting the ParallaxView");
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                Verify.IsNull(parallaxViewWeakReference.Target);

                switch (propertyToChange)
                {
                    case PropertyId.ParallaxViewChildVerticalAlignment:
                        Log.Comment("Changing prior ParallaxView.Child's VerticalAlignment");
                        rectanglePVChild.VerticalAlignment = VerticalAlignment.Bottom;
                        break;
                    case PropertyId.ScrollViewerContentHeight:
                        Log.Comment("Changing ScrollViewer.Content.Height");
                        rectangleSVContent.Height += 10;
                        break;
                }
            });
        }

        private static void ChangeScrollViewerView(
            ScrollViewer scrollViewer,
            double? horizontalOffset,
            double? verticalOffset,
            float? zoomFactor,
            bool disableAnimation,
            Visual visualPPChild,
            ref float? finalHorizontalShift,
            ref float? finalVerticalShift)
        {
            AutoResetEvent scrollViewerViewChangedEvent = null;
            double? targetHorizontalOffset = horizontalOffset;
            double? targetVerticalOffset = verticalOffset;
            float? targetZoomFactor = zoomFactor;
            bool spyOnHorizontalShift = visualPPChild != null && finalHorizontalShift != null;
            bool spyOnVerticalShift = visualPPChild != null && finalVerticalShift != null;
            float horizontalShift = 0f;
            float verticalShift = 0f;

            RunOnUIThread.Execute(() =>
            {
                scrollViewerViewChangedEvent = new AutoResetEvent(false);

                scrollViewer.ViewChanged += (object sender, ScrollViewerViewChangedEventArgs e) =>
                {
                    try
                    {
                        Log.Comment("ScrollViewer.ViewChanged - IsIntermediate={0}, View=({1}, {2}, {3})",
                            e.IsIntermediate, scrollViewer.HorizontalOffset, scrollViewer.VerticalOffset, scrollViewer.ZoomFactor);

                        CompositionGetValueStatus status;

                        if (spyOnHorizontalShift)
                        {
                            status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out horizontalShift);
                            Log.Comment("Target transform during view change: status={0}, horizontal offset={1}", status, horizontalShift);
                        }

                        if (spyOnVerticalShift)
                        {
                            status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out verticalShift);
                            Log.Comment("Target transform during view change: status={0}, vertical offset={1}", status, verticalShift);
                        }

                        if (!e.IsIntermediate)
                        {
                            Log.Comment("ScrollViewer - final notification. Final view: {0}, {1}, {2}", scrollViewer.HorizontalOffset, scrollViewer.VerticalOffset, scrollViewer.ZoomFactor);
                            scrollViewerViewChangedEvent.Set();
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Error("Exception in ScrollViewer.ViewChanged: " + ex.ToString());
                    }
                };

                Log.Comment("Changing ScrollViewer view to ({0}, {1}, {2}) with disableAnimation={3}", targetHorizontalOffset, targetVerticalOffset, targetZoomFactor, disableAnimation);
                bool result = scrollViewer.ChangeView(targetHorizontalOffset, targetVerticalOffset, targetZoomFactor, disableAnimation);
                Log.Comment("View change result: {0}", result);
                Verify.IsTrue(result);
            });

            Log.Comment("Waiting for scrollViewerViewChangedEvent event");
            scrollViewerViewChangedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("First view change completed");

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            if (spyOnHorizontalShift || spyOnVerticalShift)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Cancelling spying");

                    if (spyOnHorizontalShift)
                    {
                        CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                    }
                    if (spyOnVerticalShift)
                    {
                        CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
                    }
                });

                Log.Comment("Waiting for captured properties to be updated");
                CompositionPropertySpy.SynchronouslyTickUIThread(10);

                RunOnUIThread.Execute(() =>
                {
                    CompositionGetValueStatus status;

                    Log.Comment("Accessing final shift values");

                    if (spyOnHorizontalShift)
                    {
                        status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out horizontalShift);
                        Log.Comment("Target transform after view change: status={0}, horizontal offset={1}", status, horizontalShift);
                    }

                    if (spyOnVerticalShift)
                    {
                        status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out verticalShift);
                        Log.Comment("Target transform after view change: status={0}, vertical offset={1}", status, verticalShift);
                    }
                });

                if (spyOnHorizontalShift)
                {
                    finalHorizontalShift = horizontalShift;
                }

                if (spyOnVerticalShift)
                {
                    finalVerticalShift = verticalShift;
                }
            }
        }

        private static void ChangeScrollPresenterView(
            ScrollPresenter scrollPresenter,
            double? horizontalOffset,
            double? verticalOffset,
            float? zoomFactor,
            bool disableAnimation,
            Visual visualPPChild,
            ref float? finalHorizontalShift,
            ref float? finalVerticalShift)
        {
            bool isPrivateLoggingEnabled = false;
            AutoResetEvent scrollPresenterStateChangedEvent = null;
            bool spyOnHorizontalShift = visualPPChild != null && finalHorizontalShift != null;
            bool spyOnVerticalShift = visualPPChild != null && finalVerticalShift != null;
            float horizontalShift = 0f;
            float verticalShift = 0f;

            RunOnUIThread.Execute(() =>
            {
                if (isPrivateLoggingEnabled)
                {
                    // Turn on private tracing for ScrollPresenter control
                    MUXControlsTestHooks.SetOutputDebugStringLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
                }

                scrollPresenterStateChangedEvent = new AutoResetEvent(false);

                scrollPresenter.ViewChanged += (ScrollPresenter sender, object e) =>
                {
                    try
                    { 
                        Log.Comment("ScrollPresenter.ViewChanged - View=({0}, {1}, {2})", scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                        CompositionGetValueStatus status;

                        if (spyOnHorizontalShift)
                        {
                            status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out horizontalShift);
                            Log.Comment("Target transform during view change: status={0}, horizontal offset={1}", status, horizontalShift);
                        }

                        if (spyOnVerticalShift)
                        {
                            status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out verticalShift);
                            Log.Comment("Target transform during view change: status={0}, vertical offset={1}", status, verticalShift);
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Error("Exception in ScrollPresenter.ViewChanged: " + ex.ToString());
                    }
                };

                scrollPresenter.StateChanged += (ScrollPresenter sender, object e) =>
                {
                    try
                    { 
                        Log.Comment("ScrollPresenter.StateChanged - State={0}", scrollPresenter.State);
                        if (scrollPresenter.State == ScrollingInteractionState.Idle)
                        {
                            Log.Comment("ScrollPresenter - idling notification. Final view: {0}, {1}, {2}", scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);
                            scrollPresenterStateChangedEvent.Set();
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Error("Exception in ScrollPresenter.StateChanged: " + ex.ToString());
                    }
                };

                Log.Comment("Changing ScrollPresenter view to ({0}, {1}, {2}) with disableAnimation={3}", horizontalOffset, verticalOffset, zoomFactor, disableAnimation);
                if (zoomFactor != null && (float)zoomFactor != scrollPresenter.ZoomFactor)
                {
                    int viewChangeCorrelationId = scrollPresenter.ZoomTo(
                        (float)zoomFactor,
                        Vector2.Zero,
                        new ScrollingZoomOptions(
                            disableAnimation ? ScrollingAnimationMode.Disabled : ScrollingAnimationMode.Enabled,
                            ScrollingSnapPointsMode.Ignore));
                    Verify.IsGreaterThan(viewChangeCorrelationId, 0);
                }

                if ((horizontalOffset != null && (double)horizontalOffset != scrollPresenter.HorizontalOffset) || (verticalOffset != null && (double)verticalOffset != scrollPresenter.VerticalOffset))
                {
                    Log.Comment("Invoking ScrollPresenter.ChangeOffsets");
                    int viewChangeCorrelationId = scrollPresenter.ScrollTo(
                        horizontalOffset == null ? scrollPresenter.HorizontalOffset : (double)horizontalOffset,
                        verticalOffset == null ? scrollPresenter.VerticalOffset : (double)verticalOffset,
                        new ScrollingScrollOptions(
                            disableAnimation ? ScrollingAnimationMode.Disabled : ScrollingAnimationMode.Enabled,
                            ScrollingSnapPointsMode.Ignore));
                    Verify.IsGreaterThan(viewChangeCorrelationId, 0);
                }
            });

            Log.Comment("Waiting for scrollPresenterStateChangedEvent event");
            scrollPresenterStateChangedEvent.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration));
            Log.Comment("First view change completed");

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            if (spyOnHorizontalShift || spyOnVerticalShift)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Cancelling spying");

                    if (spyOnHorizontalShift)
                    {
                        CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualHorizontalTargetedPropertyName());
                    }
                    if (spyOnVerticalShift)
                    {
                        CompositionPropertySpy.StopSpyingProperty(visualPPChild, GetVisualVerticalTargetedPropertyName());
                    }
                });

                Log.Comment("Waiting for captured properties to be updated");
                CompositionPropertySpy.SynchronouslyTickUIThread(10);

                RunOnUIThread.Execute(() =>
                {
                    CompositionGetValueStatus status;

                    Log.Comment("Accessing final shift values");

                    if (spyOnHorizontalShift)
                    {
                        status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualHorizontalTargetedPropertyName(), out horizontalShift);
                        Log.Comment("Target transform after view change: status={0}, horizontal offset={1}", status, horizontalShift);
                    }

                    if (spyOnVerticalShift)
                    {
                        status = CompositionPropertySpy.TryGetScalar(visualPPChild, GetVisualVerticalTargetedPropertyName(), out verticalShift);
                        Log.Comment("Target transform after view change: status={0}, vertical offset={1}", status, verticalShift);
                    }

                    if (isPrivateLoggingEnabled)
                    {
                        // Turn off private tracing for ScrollPresenter control
                        MUXControlsTestHooks.SetOutputDebugStringLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                        MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                        MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
                    }
                });

                if (spyOnHorizontalShift)
                {
                    finalHorizontalShift = horizontalShift;
                }

                if (spyOnVerticalShift)
                {
                    finalVerticalShift = verticalShift;
                }
            }
        }

        private static string GetVisualHorizontalTargetedPropertyName()
        {
            return PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2) ? "Translation.X" : "TransformMatrix._41";
        }

        private static string GetVisualVerticalTargetedPropertyName()
        {
            return PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2) ? "Translation.Y" : "TransformMatrix._42";
        }

        private static void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string senderName = string.Empty;
            ScrollPresenter scr = sender as ScrollPresenter;

            if (scr != null)
            {
                senderName = "s:" + scr.Name + ", ";
            }

            if (args.IsVerboseLevel)
            {
                Log.Comment("  Verbose: " + senderName + "m:" + msg);
            }
            else
            {
                Log.Comment("  Info:    " + senderName + "m:" + msg);
            }
        }

        private static ScrollViewer FindInnerScrollViewer(ItemsControl ic)
        {
            Panel p = ic.ItemsPanelRoot;

            UIElement parent = VisualTreeHelper.GetParent(p) as UIElement;
            while (parent != null && !(parent is ScrollViewer))
                parent = VisualTreeHelper.GetParent(parent) as UIElement;

            return parent as ScrollViewer;
        }
    }
}
