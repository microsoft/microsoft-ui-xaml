// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Numerics;
using System.Threading;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Composition.Interactions;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Input;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using InteractionState = Microsoft.UI.Xaml.Controls.InteractionState;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class ScrollingPresenterTests
    {
        private const InteractionState c_defaultState = InteractionState.Idle;
        private const ChainingMode c_defaultHorizontalScrollChainingMode = ChainingMode.Auto;
        private const ChainingMode c_defaultVerticalScrollChainingMode = ChainingMode.Auto;
        private const RailingMode c_defaultHorizontalScrollRailingMode = RailingMode.Enabled;
        private const RailingMode c_defaultVerticalScrollRailingMode = RailingMode.Enabled;
#if USE_SCROLLMODE_AUTO
        private const ScrollMode c_defaultHorizontalScrollMode = ScrollMode.Auto;
        private const ScrollMode c_defaultVerticalScrollMode = ScrollMode.Auto;
#else
        private const ScrollMode c_defaultHorizontalScrollMode = ScrollMode.Enabled;
        private const ScrollMode c_defaultVerticalScrollMode = ScrollMode.Enabled;
#endif
        private const ChainingMode c_defaultZoomChainingMode = ChainingMode.Auto;
        private const ZoomMode c_defaultZoomMode = ZoomMode.Disabled;
        private const InputKind c_defaultIgnoredInputKind = InputKind.None;
        private const ContentOrientation c_defaultContentOrientation = ContentOrientation.None;
        private const double c_defaultMinZoomFactor = 0.1;
        private const double c_defaultZoomFactor = 1.0;
        private const double c_defaultMaxZoomFactor = 10.0;
        private const double c_defaultHorizontalOffset = 0.0;
        private const double c_defaultVerticalOffset = 0.0;
        private const double c_defaultAnchorRatio = 0.0;

        private const double c_defaultUIScrollingPresenterContentWidth = 1200.0;
        private const double c_defaultUIScrollingPresenterContentHeight = 600.0;
        private const double c_defaultUIScrollingPresenterWidth = 300.0;
        private const double c_defaultUIScrollingPresenterHeight = 200.0;
        private const double c_defaultUIScrollControllerThickness = 44.0;

        private const float c_defaultOffsetResultTolerance = 0.0001f;

        private const string c_visualHorizontalOffsetTargetedPropertyName = "Translation.X";
        private const string c_visualVerticalOffsetTargetedPropertyName = "Translation.Y";
        private const string c_visualScaleTargetedPropertyName = "Scale.X";

        private const string c_expressionAnimationSourcesExtentPropertyName = "Extent";
        private const string c_expressionAnimationSourcesViewportPropertyName = "Viewport";
        private const string c_expressionAnimationSourcesOffsetPropertyName = "Offset";
        private const string c_expressionAnimationSourcesPositionPropertyName = "Position";
        private const string c_expressionAnimationSourcesMinPositionPropertyName = "MinPosition";
        private const string c_expressionAnimationSourcesMaxPositionPropertyName = "MaxPosition";
        private const string c_expressionAnimationSourcesZoomFactorPropertyName = "ZoomFactor";

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollingPresenter default properties.")]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollingPresenter scrollingPresenter = new ScrollingPresenter();
                Verify.IsNotNull(scrollingPresenter);

                Log.Comment("Verifying ScrollingPresenter default property values");
                Verify.IsNull(scrollingPresenter.HorizontalScrollController);
                Verify.IsNull(scrollingPresenter.VerticalScrollController);
                Verify.IsNull(scrollingPresenter.Content);
                Verify.IsNotNull(scrollingPresenter.ExpressionAnimationSources);
                Verify.AreEqual(c_defaultState, scrollingPresenter.State);
                Verify.AreEqual(c_defaultHorizontalScrollChainingMode, scrollingPresenter.HorizontalScrollChainingMode);
                Verify.AreEqual(c_defaultVerticalScrollChainingMode, scrollingPresenter.VerticalScrollChainingMode);
                Verify.AreEqual(c_defaultHorizontalScrollRailingMode, scrollingPresenter.HorizontalScrollRailingMode);
                Verify.AreEqual(c_defaultVerticalScrollRailingMode, scrollingPresenter.VerticalScrollRailingMode);
                Verify.AreEqual(c_defaultHorizontalScrollMode, scrollingPresenter.HorizontalScrollMode);
                Verify.AreEqual(c_defaultVerticalScrollMode, scrollingPresenter.VerticalScrollMode);
                Verify.AreEqual(c_defaultZoomChainingMode, scrollingPresenter.ZoomChainingMode);
                Verify.AreEqual(c_defaultContentOrientation, scrollingPresenter.ContentOrientation);
                Verify.AreEqual(c_defaultZoomMode, scrollingPresenter.ZoomMode);
                Verify.AreEqual(c_defaultIgnoredInputKind, scrollingPresenter.IgnoredInputKind);
                Verify.AreEqual(c_defaultMinZoomFactor, scrollingPresenter.MinZoomFactor);
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollingPresenter.MaxZoomFactor);
                Verify.AreEqual(c_defaultZoomFactor, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(c_defaultHorizontalOffset, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(c_defaultVerticalOffset, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(c_defaultAnchorRatio, scrollingPresenter.HorizontalAnchorRatio);
                Verify.AreEqual(c_defaultAnchorRatio, scrollingPresenter.VerticalAnchorRatio);
                Verify.AreEqual(0.0, scrollingPresenter.ExtentWidth);
                Verify.AreEqual(0.0, scrollingPresenter.ExtentHeight);
                Verify.AreEqual(0.0, scrollingPresenter.ViewportWidth);
                Verify.AreEqual(0.0, scrollingPresenter.ViewportHeight);
                Verify.AreEqual(0.0, scrollingPresenter.ScrollableWidth);
                Verify.AreEqual(0.0, scrollingPresenter.ScrollableHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Exercises the ScrollingPresenter property setters and getters for non-default values.")]
        public void VerifyPropertyGettersAndSetters()
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangle = null;

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter = new ScrollingPresenter();
                Verify.IsNotNull(scrollingPresenter);

                rectangle = new Rectangle();
                Verify.IsNotNull(rectangle);

                Log.Comment("Setting ScrollingPresenter properties to non-default values");
                scrollingPresenter.Content = rectangle;
                scrollingPresenter.HorizontalScrollChainingMode = ChainingMode.Always;
                scrollingPresenter.VerticalScrollChainingMode = ChainingMode.Never;
                scrollingPresenter.HorizontalScrollRailingMode = RailingMode.Disabled;
                scrollingPresenter.VerticalScrollRailingMode = RailingMode.Disabled;
                scrollingPresenter.HorizontalScrollMode = ScrollMode.Disabled;
                scrollingPresenter.VerticalScrollMode = ScrollMode.Disabled;
                scrollingPresenter.ZoomChainingMode = ChainingMode.Never;
                scrollingPresenter.ZoomMode = ZoomMode.Enabled;
                scrollingPresenter.IgnoredInputKind = InputKind.MouseWheel;
                scrollingPresenter.ContentOrientation = ContentOrientation.Horizontal;
                scrollingPresenter.MinZoomFactor = 0.5f;
                scrollingPresenter.MaxZoomFactor = 2.0f;
                scrollingPresenter.HorizontalAnchorRatio = 0.25f;
                scrollingPresenter.VerticalAnchorRatio = 0.75f;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verifying ScrollingPresenter non-default property values");
                Verify.AreEqual(rectangle, scrollingPresenter.Content);
                Verify.AreEqual(c_defaultState, scrollingPresenter.State);
                Verify.AreEqual(ChainingMode.Always, scrollingPresenter.HorizontalScrollChainingMode);
                Verify.AreEqual(ChainingMode.Never, scrollingPresenter.VerticalScrollChainingMode);
                Verify.AreEqual(RailingMode.Disabled, scrollingPresenter.HorizontalScrollRailingMode);
                Verify.AreEqual(RailingMode.Disabled, scrollingPresenter.VerticalScrollRailingMode);
                Verify.AreEqual(ScrollMode.Disabled, scrollingPresenter.HorizontalScrollMode);
                Verify.AreEqual(ScrollMode.Disabled, scrollingPresenter.VerticalScrollMode);
                Verify.AreEqual(ChainingMode.Never, scrollingPresenter.ZoomChainingMode);
                Verify.AreEqual(ZoomMode.Enabled, scrollingPresenter.ZoomMode);
                Verify.AreEqual(InputKind.MouseWheel, scrollingPresenter.IgnoredInputKind);
                Verify.AreEqual(ContentOrientation.Horizontal, scrollingPresenter.ContentOrientation);
                Verify.AreEqual(0.5f, scrollingPresenter.MinZoomFactor);
                Verify.AreEqual(2.0f, scrollingPresenter.MaxZoomFactor);
                Verify.AreEqual(0.25f, scrollingPresenter.HorizontalAnchorRatio);
                Verify.AreEqual(0.75f, scrollingPresenter.VerticalAnchorRatio);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollingPresenter ExtentWidth/Height, ViewportWidth/Height and ScrollableWidth/Height properties.")]
        public void VerifyExtentAndViewportProperties()
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent, setAsContentRoot: true);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(c_defaultUIScrollingPresenterContentWidth, scrollingPresenter.ExtentWidth);
                Verify.AreEqual(c_defaultUIScrollingPresenterContentHeight, scrollingPresenter.ExtentHeight);
                Verify.AreEqual(c_defaultUIScrollingPresenterWidth, scrollingPresenter.ViewportWidth);
                Verify.AreEqual(c_defaultUIScrollingPresenterHeight, scrollingPresenter.ViewportHeight);
                Verify.AreEqual(c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth, scrollingPresenter.ScrollableWidth);
                Verify.AreEqual(c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight, scrollingPresenter.ScrollableHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts to set invalid ScrollingPresenter.MinZoomFactor values.")]
        public void SetInvalidMinZoomFactorValues()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollingPresenter scrollingPresenter = new ScrollingPresenter();

                Log.Comment("Attempting to set ScrollingPresenter.MinZoomFactor to double.NaN");
                try
                {
                    scrollingPresenter.MinZoomFactor = double.NaN;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMinZoomFactor, scrollingPresenter.MinZoomFactor);

                Log.Comment("Attempting to set ScrollingPresenter.MinZoomFactor to double.NegativeInfinity");
                try
                {
                    scrollingPresenter.MinZoomFactor = double.NegativeInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMinZoomFactor, scrollingPresenter.MinZoomFactor);

                Log.Comment("Attempting to set ScrollingPresenter.MinZoomFactor to double.PositiveInfinity");
                try
                {
                    scrollingPresenter.MinZoomFactor = double.PositiveInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMinZoomFactor, scrollingPresenter.MinZoomFactor);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts to set invalid ScrollingPresenter.MaxZoomFactor values.")]
        public void SetInvalidMaxZoomFactorValues()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollingPresenter scrollingPresenter = new ScrollingPresenter();

                Log.Comment("Attempting to set ScrollingPresenter.MaxZoomFactor to double.NaN");
                try
                {
                    scrollingPresenter.MaxZoomFactor = double.NaN;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollingPresenter.MaxZoomFactor);

                Log.Comment("Attempting to set ScrollingPresenter.MaxZoomFactor to double.NegativeInfinity");
                try
                {
                    scrollingPresenter.MaxZoomFactor = double.NegativeInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollingPresenter.MaxZoomFactor);

                Log.Comment("Attempting to set ScrollingPresenter.MaxZoomFactor to double.PositiveInfinity");
                try
                {
                    scrollingPresenter.MaxZoomFactor = double.PositiveInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollingPresenter.MaxZoomFactor);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the InteractionTracker's VisualInteractionSource properties get set according to ScrollingPresenter properties.")]
        public void VerifyInteractionSourceSettings()
        {
            using (ScrollingPresenterTestHooksHelper scrollingPresenterTestHooksHelper = new ScrollingPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications : false))
            {
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
                    CompositionInteractionSourceCollection interactionSources = scrollingPresenterTestHooksHelper.GetInteractionSources(scrollingPresenter);
                    Verify.IsNotNull(interactionSources);
                    ScrollingPresenterTestHooksHelper.LogInteractionSources(interactionSources);
                    Verify.AreEqual(1, interactionSources.Count);
                    IEnumerator<ICompositionInteractionSource> sourcesEnumerator = (interactionSources as IEnumerable<ICompositionInteractionSource>).GetEnumerator();
                    sourcesEnumerator.MoveNext();
                    VisualInteractionSource visualInteractionSource = sourcesEnumerator.Current as VisualInteractionSource;
                    Verify.IsNotNull(visualInteractionSource);

                    Verify.AreEqual(PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5) ? VisualInteractionSourceRedirectionMode.CapableTouchpadAndPointerWheel : VisualInteractionSourceRedirectionMode.CapableTouchpadOnly,
                        visualInteractionSource.ManipulationRedirectionMode);
                    Verify.IsTrue(visualInteractionSource.IsPositionXRailsEnabled);
                    Verify.IsTrue(visualInteractionSource.IsPositionYRailsEnabled);
                    Verify.AreEqual(InteractionChainingMode.Auto, visualInteractionSource.PositionXChainingMode);
                    Verify.AreEqual(InteractionChainingMode.Auto, visualInteractionSource.PositionYChainingMode);
                    Verify.AreEqual(InteractionChainingMode.Auto, visualInteractionSource.ScaleChainingMode);
                    Verify.AreEqual(InteractionSourceMode.EnabledWithInertia, visualInteractionSource.PositionXSourceMode);
                    Verify.AreEqual(InteractionSourceMode.EnabledWithInertia, visualInteractionSource.PositionYSourceMode);
                    Verify.AreEqual(InteractionSourceMode.Disabled, visualInteractionSource.ScaleSourceMode);
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                    {
                        Verify.AreEqual(InteractionSourceRedirectionMode.Enabled, visualInteractionSource.PointerWheelConfig.PositionXSourceMode);
                        Verify.AreEqual(InteractionSourceRedirectionMode.Enabled, visualInteractionSource.PointerWheelConfig.PositionYSourceMode);
                        Verify.AreEqual(InteractionSourceRedirectionMode.Enabled, visualInteractionSource.PointerWheelConfig.ScaleSourceMode);
                    }

                    Log.Comment("Changing ScrollingPresenter properties that affect the primary VisualInteractionSource");
                    scrollingPresenter.HorizontalScrollChainingMode = ChainingMode.Always;
                    scrollingPresenter.VerticalScrollChainingMode = ChainingMode.Never;
                    scrollingPresenter.HorizontalScrollRailingMode = RailingMode.Disabled;
                    scrollingPresenter.VerticalScrollRailingMode = RailingMode.Disabled;
                    scrollingPresenter.HorizontalScrollMode = ScrollMode.Enabled;
                    scrollingPresenter.VerticalScrollMode = ScrollMode.Disabled;
                    scrollingPresenter.ZoomChainingMode = ChainingMode.Never;
                    scrollingPresenter.ZoomMode = ZoomMode.Enabled;
                    scrollingPresenter.IgnoredInputKind = InputKind.All & ~InputKind.Touch;
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    CompositionInteractionSourceCollection interactionSources = scrollingPresenterTestHooksHelper.GetInteractionSources(scrollingPresenter);
                    Verify.IsNotNull(interactionSources);
                    ScrollingPresenterTestHooksHelper.LogInteractionSources(interactionSources);
                    Verify.AreEqual(1, interactionSources.Count);
                    IEnumerator<ICompositionInteractionSource> sourcesEnumerator = (interactionSources as IEnumerable<ICompositionInteractionSource>).GetEnumerator();
                    sourcesEnumerator.MoveNext();
                    VisualInteractionSource visualInteractionSource = sourcesEnumerator.Current as VisualInteractionSource;
                    Verify.IsNotNull(visualInteractionSource);

                    Verify.AreEqual(VisualInteractionSourceRedirectionMode.CapableTouchpadOnly, visualInteractionSource.ManipulationRedirectionMode);
                    Verify.IsFalse(visualInteractionSource.IsPositionXRailsEnabled);
                    Verify.IsFalse(visualInteractionSource.IsPositionYRailsEnabled);
                    Verify.AreEqual(InteractionChainingMode.Always, visualInteractionSource.PositionXChainingMode);
                    Verify.AreEqual(InteractionChainingMode.Never, visualInteractionSource.PositionYChainingMode);
                    Verify.AreEqual(InteractionChainingMode.Never, visualInteractionSource.ScaleChainingMode);
                    Verify.AreEqual(InteractionSourceMode.EnabledWithInertia, visualInteractionSource.PositionXSourceMode);
                    Verify.AreEqual(InteractionSourceMode.Disabled, visualInteractionSource.PositionYSourceMode);
                    Verify.AreEqual(InteractionSourceMode.EnabledWithInertia, visualInteractionSource.ScaleSourceMode);
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                    {
                        Verify.AreEqual(InteractionSourceRedirectionMode.Enabled, visualInteractionSource.PointerWheelConfig.PositionXSourceMode);
                        Verify.AreEqual(InteractionSourceRedirectionMode.Enabled, visualInteractionSource.PointerWheelConfig.PositionYSourceMode);
                        Verify.AreEqual(InteractionSourceRedirectionMode.Enabled, visualInteractionSource.PointerWheelConfig.ScaleSourceMode);
                    }
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Decreases the ScrollingPresenter.MaxZoomFactor property and verifies the ScrollingPresenter.ZoomFactor value decreases accordingly. Verifies the impact on the ScrollingPresenter.Content Visual.")]
        public void PinchContentThroughMaxZoomFactor()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX ScrollingPresenter tests fail on RS5_Release
                return;
            }

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const double newMaxZoomFactor = 0.5;
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            Visual visualScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(
                    scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(newMaxZoomFactor < c_defaultZoomFactor);
                Verify.IsTrue(newMaxZoomFactor < c_defaultMaxZoomFactor);
                Verify.IsTrue(newMaxZoomFactor > c_defaultMinZoomFactor);
                scrollingPresenter.MaxZoomFactor = newMaxZoomFactor;
            });

            IdleSynchronizer.Wait();

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy on translation and scale facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(rectangleScrollingPresenterContent, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(rectangleScrollingPresenterContent, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy property set");
                    visualScrollingPresenterContent = ElementCompositionPreview.GetElementVisual(rectangleScrollingPresenterContent);

                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollingPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollingPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollingPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);


            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(rectangleScrollingPresenterContent);
                    CompositionPropertySpy.StopSpyingScaleFacade(rectangleScrollingPresenterContent);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Cancelling spying");
                    CompositionPropertySpy.StopSpyingProperty(visualScrollingPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollingPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollingPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(c_defaultMinZoomFactor, scrollingPresenter.MinZoomFactor);
                Verify.AreEqual(newMaxZoomFactor, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(newMaxZoomFactor, scrollingPresenter.MaxZoomFactor);
            });

            Log.Comment("Validating final transform of ScrollingPresenter.Content's Visual after MaxZoomFactor change");
            CompositionGetValueStatus status;
            float offset;
            float zoomFactor;
            
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation;
                    status = CompositionPropertySpy.TryGetTranslationFacade(rectangleScrollingPresenterContent, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    Verify.AreEqual(c_defaultHorizontalOffset, translation.X);
                    Verify.AreEqual(c_defaultVerticalOffset, translation.Y);

                    Vector3 scale;
                    status = CompositionPropertySpy.TryGetScaleFacade(rectangleScrollingPresenterContent, out scale);
                    Log.Comment("status={0}, vertical offset={1}", status, scale.X);
                    Verify.AreEqual(newMaxZoomFactor, scale.X);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollingPresenterContent, c_visualHorizontalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, horizontal offset={1}", status, offset);
                    Verify.AreEqual(c_defaultHorizontalOffset, offset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollingPresenterContent, c_visualVerticalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, vertical offset={1}", status, offset);
                    Verify.AreEqual(c_defaultVerticalOffset, offset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollingPresenterContent, c_visualScaleTargetedPropertyName, out zoomFactor);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                    Verify.AreEqual(newMaxZoomFactor, zoomFactor);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Increases the ScrollingPresenter.MinZoomFactor property and verifies the ScrollingPresenter.ZoomFactor value increases accordingly. Verifies the impact on the ScrollingPresenter.Content Visual.")]
        public void StretchContentThroughMinZoomFactor()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX ScrollingPresenter tests fail on RS5_Release
                return;
            }

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const double newMinZoomFactor = 2.0;
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            Visual visualScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(
                    scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(newMinZoomFactor > c_defaultZoomFactor);
                Verify.IsTrue(newMinZoomFactor > c_defaultMinZoomFactor);
                Verify.IsTrue(newMinZoomFactor < c_defaultMaxZoomFactor);
                scrollingPresenter.MinZoomFactor = newMinZoomFactor;
            });

            IdleSynchronizer.Wait();

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy on translation and scale facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(rectangleScrollingPresenterContent, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(rectangleScrollingPresenterContent, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy property set");
                    visualScrollingPresenterContent = ElementCompositionPreview.GetElementVisual(rectangleScrollingPresenterContent);

                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollingPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollingPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollingPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);
            Log.Comment("Cancelling spying");

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(rectangleScrollingPresenterContent);
                    CompositionPropertySpy.StopSpyingScaleFacade(rectangleScrollingPresenterContent);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingProperty(visualScrollingPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollingPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollingPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(newMinZoomFactor, scrollingPresenter.MinZoomFactor);
                Verify.AreEqual(newMinZoomFactor, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollingPresenter.MaxZoomFactor);
            });
            
            Log.Comment("Validating final transform of ScrollingPresenter.Content's Visual after MinZoomFactor change");
            CompositionGetValueStatus status;
            float offset;
            float zoomFactor;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation;
                    status = CompositionPropertySpy.TryGetTranslationFacade(rectangleScrollingPresenterContent, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    Verify.AreEqual(c_defaultHorizontalOffset, translation.X);
                    Verify.AreEqual(c_defaultVerticalOffset, translation.Y);

                    Vector3 scale;
                    status = CompositionPropertySpy.TryGetScaleFacade(rectangleScrollingPresenterContent, out scale);
                    Log.Comment("status={0}, vertical offset={1}", status, scale.X);
                    Verify.AreEqual(newMinZoomFactor, scale.X);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollingPresenterContent, c_visualHorizontalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, horizontal offset={1}", status, offset);
                    Verify.AreEqual(c_defaultHorizontalOffset, offset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollingPresenterContent, c_visualVerticalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, vertical offset={1}", status, offset);
                    Verify.AreEqual(c_defaultVerticalOffset, offset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollingPresenterContent, c_visualScaleTargetedPropertyName, out zoomFactor);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                    Verify.AreEqual(newMinZoomFactor, zoomFactor);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Reads the properties exposed by the ScrollingPresenter.ExpressionAnimationSources CompositionPropertySet.")]
        public void ReadExpressionAnimationSources()
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(
                    scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Setting up spy property set");
                CompositionPropertySpy.StartSpyingVector2Property(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingScalarProperty(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName, 1.0f);
            });

            Log.Comment("Jumping to absolute zoomFactor");
            ZoomTo(scrollingPresenter, 2.0f, 100.0f, 200.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);            

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName);
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Validating final property values of ScrollingPresenter.ExpressionAnimationSources");
                CompositionGetValueStatus status;
                float zoomFactor;
                Vector2 offset;
                Vector2 position;
                Vector2 minPosition;
                Vector2 maxPosition;
                Vector2 extent;
                Vector2 viewport;

                Log.Comment("Validating final zoomFactor");
                status = CompositionPropertySpy.TryGetScalar(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName, out zoomFactor);
                Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                Verify.AreEqual(2.0f, zoomFactor);

                Log.Comment("Validating final offset");
                status = CompositionPropertySpy.TryGetVector2(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName, out offset);
                Log.Comment("status={0}, offset={1}", status, offset);
                Verify.AreEqual(0.0f, offset.X);
                Verify.AreEqual(0.0f, offset.Y);

                Log.Comment("Validating final position");
                status = CompositionPropertySpy.TryGetVector2(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName, out position);
                Log.Comment("status={0}, position={1}", status, position);
                Verify.AreEqual(100.0f, position.X);
                Verify.AreEqual(200.0f, position.Y);

                Log.Comment("Validating final minPosition");
                status = CompositionPropertySpy.TryGetVector2(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName, out minPosition);
                Log.Comment("status={0}, minPosition={1}", status, minPosition);
                Verify.AreEqual(0.0f, minPosition.X);
                Verify.AreEqual(0.0f, minPosition.Y);

                Log.Comment("Validating final maxPosition");
                status = CompositionPropertySpy.TryGetVector2(scrollingPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName, out maxPosition);
                Log.Comment("status={0}, maxPosition={1}", status, maxPosition);
                Verify.AreEqual(2100.0f, maxPosition.X);
                Verify.AreEqual(1000.0f, maxPosition.Y);

                Log.Comment("Validating final extent");
                status = scrollingPresenter.ExpressionAnimationSources.TryGetVector2(c_expressionAnimationSourcesExtentPropertyName, out extent);
                Log.Comment("status={0}, extent={1}", status, extent);
                Verify.AreEqual(c_defaultUIScrollingPresenterContentWidth, extent.X);
                Verify.AreEqual(c_defaultUIScrollingPresenterContentHeight, extent.Y);

                Log.Comment("Validating final viewport");
                status = scrollingPresenter.ExpressionAnimationSources.TryGetVector2(c_expressionAnimationSourcesViewportPropertyName, out viewport);
                Log.Comment("status={0}, viewport={1}", status, viewport);
                Verify.AreEqual(c_defaultUIScrollingPresenterWidth, viewport.X);
                Verify.AreEqual(c_defaultUIScrollingPresenterHeight, viewport.Y);
            });
        }

        [TestMethod]
        public void ValidateXYFocusNavigation()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Skipped ValidateXYFocusNavigation because XYFocus support for third party scrollingPresenters is only available in RS4+.");
                return;
            }

            Button innerCenterButton = null;

            RunOnUIThread.Execute(() =>
            {
                var rootPanel = (Grid)XamlReader.Load(TestUtilities.ProcessTestXamlForRepo(
                    @"<Grid Width='600' Height='600' 
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controlsPrimitives='using:Microsoft.UI.Xaml.Controls.Primitives'>
                        <Button Content='Outer Left Button' HorizontalAlignment='Left' VerticalAlignment='Center' />
                        <Button Content='Outer Top Button' HorizontalAlignment='Center' VerticalAlignment='Top' />
                        <Button Content='Outer Right Button' HorizontalAlignment='Right' VerticalAlignment='Center' />
                        <Button Content='Outer Bottom Button' HorizontalAlignment='Center' VerticalAlignment='Bottom' />

                        <controlsPrimitives:ScrollingPresenter x:Name='scrollingPresenter' Width='200' Height='200' HorizontalAlignment='Center' VerticalAlignment='Center'>
                            <Grid Width='600' Height='600' Background='Gray'>
                                
                                <!-- Inner buttons are larger than the outer so that they get ranked better by the XY focus algorithm. -->
                                <Button Content='Inner Left Button' HorizontalAlignment='Left' VerticalAlignment='Center'  Width='180' Height='180' />
                                <Button Content='Inner Top Button' HorizontalAlignment='Center' VerticalAlignment='Top' Width='180' Height='180' />
                                <Button Content='Inner Right Button' HorizontalAlignment='Right' VerticalAlignment='Center' Width='180' Height='180' />
                                <Button Content='Inner Bottom Button' HorizontalAlignment='Center' VerticalAlignment='Bottom' Width='180' Height='180' />

                                <Button x:Name='innerCenterButton' Content='Inner Center Button' HorizontalAlignment='Center' VerticalAlignment='Center' />
                            </Grid>
                        </controlsPrimitives:ScrollingPresenter>
                    </Grid>"));

                innerCenterButton = (Button)rootPanel.FindName("innerCenterButton");
                MUXControlsTestApp.App.TestContentRoot = rootPanel;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Ensure inner center button has keyboard focus.");
                innerCenterButton.Focus(FocusState.Keyboard);
                Verify.AreEqual(innerCenterButton, FocusManager.GetFocusedElement());

                Verify.AreEqual("Inner Right Button", ((Button)FocusManager.FindNextFocusableElement(FocusNavigationDirection.Right)).Content);
                Verify.AreEqual("Inner Left Button", ((Button)FocusManager.FindNextFocusableElement(FocusNavigationDirection.Left)).Content);
                Verify.AreEqual("Inner Top Button", ((Button)FocusManager.FindNextFocusableElement(FocusNavigationDirection.Up)).Content);
                Verify.AreEqual("Inner Bottom Button", ((Button)FocusManager.FindNextFocusableElement(FocusNavigationDirection.Down)).Content);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Listens to the ScrollingPresenter.Content.EffectiveViewportChanged event and expects it to be raised while changing offsets.")]
        public void ListenToContentEffectiveViewportChanged()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                // Skipping this test on pre-RS5 since it uses RS5's FrameworkElement.EffectiveViewportChanged event.
                return;
            }

            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            int effectiveViewportChangedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);

                rectangleScrollingPresenterContent.EffectiveViewportChanged += (FrameworkElement sender, EffectiveViewportChangedEventArgs args) =>
                {
                    Log.Comment("ScrollingPresenter.Content.EffectiveViewportChanged: BringIntoViewDistance=" +
                        args.BringIntoViewDistanceX + "," + args.BringIntoViewDistanceY + ", EffectiveViewport=" +
                        args.EffectiveViewport.ToString() + ", MaxViewport=" + args.MaxViewport.ToString());
                    effectiveViewportChangedCount++;
                };
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            ScrollTo(
                scrollingPresenter,
                horizontalOffset: 250,
                verticalOffset: 150,
                animationMode: AnimationMode.Enabled,
                snapPointsMode: SnapPointsMode.Ignore,
                hookViewChanged: false);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Expect multiple EffectiveViewportChanged occurrences during the animated offsets change.");
                Verify.IsGreaterThanOrEqual(effectiveViewportChangedCount, 1);
            });
        }

        private void SetupDefaultUI(
            ScrollingPresenter scrollingPresenter,
            Rectangle rectangleScrollingPresenterContent,
            AutoResetEvent scrollingPresenterLoadedEvent,
            bool setAsContentRoot = true)
        {
            Log.Comment("Setting up default UI with ScrollingPresenter" + (rectangleScrollingPresenterContent == null ? "" : " and Rectangle"));

            if (rectangleScrollingPresenterContent != null)
            {
                LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

                GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
                twoColorLGB.GradientStops.Add(brownGS);

                GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
                twoColorLGB.GradientStops.Add(orangeGS);

                rectangleScrollingPresenterContent.Width = c_defaultUIScrollingPresenterContentWidth;
                rectangleScrollingPresenterContent.Height = c_defaultUIScrollingPresenterContentHeight;
                rectangleScrollingPresenterContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scrollingPresenter);
            scrollingPresenter.Width = c_defaultUIScrollingPresenterWidth;
            scrollingPresenter.Height = c_defaultUIScrollingPresenterHeight;
            if (rectangleScrollingPresenterContent != null)
            {
                scrollingPresenter.Content = rectangleScrollingPresenterContent;
            }

            if (scrollingPresenterLoadedEvent != null)
            {
                scrollingPresenter.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollingPresenter.Loaded event handler");
                    scrollingPresenterLoadedEvent.Set();
                };
            }

            if (setAsContentRoot)
            {
                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scrollingPresenter;
            }
        }

        private void SpyTranslationAndScale(ScrollingPresenter scrollingPresenter, Compositor compositor, out float horizontalOffset, out float verticalOffset, out float zoomFactor)
        {
            Verify.IsTrue(PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2));

            float horizontalOffsetTmp = 0.0f;
            float verticalOffsetTmp = 0.0f;
            float zoomFactorTmp = 1.0f;

            horizontalOffset = verticalOffset = 0.0f;
            zoomFactor = 0.0f;
            
            Visual visualScrollingPresenterContent = null;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(scrollingPresenter);
                    Verify.IsNotNull(scrollingPresenter.Content);

                    Log.Comment("Setting up spying on facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(scrollingPresenter.Content, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(scrollingPresenter.Content, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(scrollingPresenter);
                    Verify.IsNotNull(scrollingPresenter.Content);

                    Log.Comment("Setting up spy property set");
                    visualScrollingPresenterContent = ElementCompositionPreview.GetElementVisual(scrollingPresenter.Content);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollingPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollingPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollingPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            Log.Comment("Cancelling spying");
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(scrollingPresenter.Content);
                    CompositionPropertySpy.StopSpyingScaleFacade(scrollingPresenter.Content);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingProperty(visualScrollingPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollingPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollingPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            Log.Comment("Reading ScrollingPresenter.Content's Visual Transform");
            CompositionGetValueStatus status;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation = Vector3.Zero;
                    status = CompositionPropertySpy.TryGetTranslationFacade(scrollingPresenter.Content, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    horizontalOffsetTmp = translation.X;
                    verticalOffsetTmp = translation.Y;

                    Vector3 scale = Vector3.One;
                    status = CompositionPropertySpy.TryGetScaleFacade(scrollingPresenter.Content, out scale);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactorTmp);
                    zoomFactorTmp = scale.X;
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollingPresenterContent, c_visualHorizontalOffsetTargetedPropertyName, out horizontalOffsetTmp);
                    Log.Comment("status={0}, horizontal offset={1}", status, horizontalOffsetTmp);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollingPresenterContent, c_visualVerticalOffsetTargetedPropertyName, out verticalOffsetTmp);
                    Log.Comment("status={0}, vertical offset={1}", status, verticalOffsetTmp);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollingPresenterContent, c_visualScaleTargetedPropertyName, out zoomFactorTmp);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactorTmp);
                });
            }

            horizontalOffset = horizontalOffsetTmp;
            verticalOffset = verticalOffsetTmp;
            zoomFactor = zoomFactorTmp;
        }
    }
}
