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
using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class ScrollPresenterTests : ApiTestBase
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

        private const double c_defaultUIScrollPresenterContentWidth = 1200.0;
        private const double c_defaultUIScrollPresenterContentHeight = 600.0;
        private const double c_defaultUIScrollPresenterWidth = 300.0;
        private const double c_defaultUIScrollPresenterHeight = 200.0;
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
        [TestProperty("Description", "Verifies the ScrollPresenter default properties.")]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollPresenter scrollPresenter = new ScrollPresenter();
                Verify.IsNotNull(scrollPresenter);

                Log.Comment("Verifying ScrollPresenter default property values");
                Verify.IsNull(scrollPresenter.HorizontalScrollController);
                Verify.IsNull(scrollPresenter.VerticalScrollController);
                Verify.IsNull(scrollPresenter.Content);
                Verify.IsNotNull(scrollPresenter.ExpressionAnimationSources);
                Verify.AreEqual(c_defaultState, scrollPresenter.State);
                Verify.AreEqual(c_defaultHorizontalScrollChainingMode, scrollPresenter.HorizontalScrollChainingMode);
                Verify.AreEqual(c_defaultVerticalScrollChainingMode, scrollPresenter.VerticalScrollChainingMode);
                Verify.AreEqual(c_defaultHorizontalScrollRailingMode, scrollPresenter.HorizontalScrollRailingMode);
                Verify.AreEqual(c_defaultVerticalScrollRailingMode, scrollPresenter.VerticalScrollRailingMode);
                Verify.AreEqual(c_defaultHorizontalScrollMode, scrollPresenter.HorizontalScrollMode);
                Verify.AreEqual(c_defaultVerticalScrollMode, scrollPresenter.VerticalScrollMode);
                Verify.AreEqual(c_defaultZoomChainingMode, scrollPresenter.ZoomChainingMode);
                Verify.AreEqual(c_defaultContentOrientation, scrollPresenter.ContentOrientation);
                Verify.AreEqual(c_defaultZoomMode, scrollPresenter.ZoomMode);
                Verify.AreEqual(c_defaultIgnoredInputKind, scrollPresenter.IgnoredInputKind);
                Verify.AreEqual(c_defaultMinZoomFactor, scrollPresenter.MinZoomFactor);
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollPresenter.MaxZoomFactor);
                Verify.AreEqual(c_defaultZoomFactor, scrollPresenter.ZoomFactor);
                Verify.AreEqual(c_defaultHorizontalOffset, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(c_defaultVerticalOffset, scrollPresenter.VerticalOffset);
                Verify.AreEqual(c_defaultAnchorRatio, scrollPresenter.HorizontalAnchorRatio);
                Verify.AreEqual(c_defaultAnchorRatio, scrollPresenter.VerticalAnchorRatio);
                Verify.AreEqual(0.0, scrollPresenter.ExtentWidth);
                Verify.AreEqual(0.0, scrollPresenter.ExtentHeight);
                Verify.AreEqual(0.0, scrollPresenter.ViewportWidth);
                Verify.AreEqual(0.0, scrollPresenter.ViewportHeight);
                Verify.AreEqual(0.0, scrollPresenter.ScrollableWidth);
                Verify.AreEqual(0.0, scrollPresenter.ScrollableHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Exercises the ScrollPresenter property setters and getters for non-default values.")]
        public void VerifyPropertyGettersAndSetters()
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangle = null;

            RunOnUIThread.Execute(() =>
            {
                scrollPresenter = new ScrollPresenter();
                Verify.IsNotNull(scrollPresenter);

                rectangle = new Rectangle();
                Verify.IsNotNull(rectangle);

                Log.Comment("Setting ScrollPresenter properties to non-default values");
                scrollPresenter.Content = rectangle;
                scrollPresenter.HorizontalScrollChainingMode = ChainingMode.Always;
                scrollPresenter.VerticalScrollChainingMode = ChainingMode.Never;
                scrollPresenter.HorizontalScrollRailingMode = RailingMode.Disabled;
                scrollPresenter.VerticalScrollRailingMode = RailingMode.Disabled;
                scrollPresenter.HorizontalScrollMode = ScrollMode.Disabled;
                scrollPresenter.VerticalScrollMode = ScrollMode.Disabled;
                scrollPresenter.ZoomChainingMode = ChainingMode.Never;
                scrollPresenter.ZoomMode = ZoomMode.Enabled;
                scrollPresenter.IgnoredInputKind = InputKind.MouseWheel;
                scrollPresenter.ContentOrientation = ContentOrientation.Horizontal;
                scrollPresenter.MinZoomFactor = 0.5f;
                scrollPresenter.MaxZoomFactor = 2.0f;
                scrollPresenter.HorizontalAnchorRatio = 0.25f;
                scrollPresenter.VerticalAnchorRatio = 0.75f;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verifying ScrollPresenter non-default property values");
                Verify.AreEqual(rectangle, scrollPresenter.Content);
                Verify.AreEqual(c_defaultState, scrollPresenter.State);
                Verify.AreEqual(ChainingMode.Always, scrollPresenter.HorizontalScrollChainingMode);
                Verify.AreEqual(ChainingMode.Never, scrollPresenter.VerticalScrollChainingMode);
                Verify.AreEqual(RailingMode.Disabled, scrollPresenter.HorizontalScrollRailingMode);
                Verify.AreEqual(RailingMode.Disabled, scrollPresenter.VerticalScrollRailingMode);
                Verify.AreEqual(ScrollMode.Disabled, scrollPresenter.HorizontalScrollMode);
                Verify.AreEqual(ScrollMode.Disabled, scrollPresenter.VerticalScrollMode);
                Verify.AreEqual(ChainingMode.Never, scrollPresenter.ZoomChainingMode);
                Verify.AreEqual(ZoomMode.Enabled, scrollPresenter.ZoomMode);
                Verify.AreEqual(InputKind.MouseWheel, scrollPresenter.IgnoredInputKind);
                Verify.AreEqual(ContentOrientation.Horizontal, scrollPresenter.ContentOrientation);
                Verify.AreEqual(0.5f, scrollPresenter.MinZoomFactor);
                Verify.AreEqual(2.0f, scrollPresenter.MaxZoomFactor);
                Verify.AreEqual(0.25f, scrollPresenter.HorizontalAnchorRatio);
                Verify.AreEqual(0.75f, scrollPresenter.VerticalAnchorRatio);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollPresenter ExtentWidth/Height, ViewportWidth/Height and ScrollableWidth/Height properties.")]
        public void VerifyExtentAndViewportProperties()
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent, setAsContentRoot: true);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(c_defaultUIScrollPresenterContentWidth, scrollPresenter.ExtentWidth);
                Verify.AreEqual(c_defaultUIScrollPresenterContentHeight, scrollPresenter.ExtentHeight);
                Verify.AreEqual(c_defaultUIScrollPresenterWidth, scrollPresenter.ViewportWidth);
                Verify.AreEqual(c_defaultUIScrollPresenterHeight, scrollPresenter.ViewportHeight);
                Verify.AreEqual(c_defaultUIScrollPresenterContentWidth - c_defaultUIScrollPresenterWidth, scrollPresenter.ScrollableWidth);
                Verify.AreEqual(c_defaultUIScrollPresenterContentHeight - c_defaultUIScrollPresenterHeight, scrollPresenter.ScrollableHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts to set invalid ScrollPresenter.MinZoomFactor values.")]
        public void SetInvalidMinZoomFactorValues()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollPresenter scrollPresenter = new ScrollPresenter();

                Log.Comment("Attempting to set ScrollPresenter.MinZoomFactor to double.NaN");
                try
                {
                    scrollPresenter.MinZoomFactor = double.NaN;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMinZoomFactor, scrollPresenter.MinZoomFactor);

                Log.Comment("Attempting to set ScrollPresenter.MinZoomFactor to double.NegativeInfinity");
                try
                {
                    scrollPresenter.MinZoomFactor = double.NegativeInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMinZoomFactor, scrollPresenter.MinZoomFactor);

                Log.Comment("Attempting to set ScrollPresenter.MinZoomFactor to double.PositiveInfinity");
                try
                {
                    scrollPresenter.MinZoomFactor = double.PositiveInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMinZoomFactor, scrollPresenter.MinZoomFactor);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts to set invalid ScrollPresenter.MaxZoomFactor values.")]
        public void SetInvalidMaxZoomFactorValues()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollPresenter scrollPresenter = new ScrollPresenter();

                Log.Comment("Attempting to set ScrollPresenter.MaxZoomFactor to double.NaN");
                try
                {
                    scrollPresenter.MaxZoomFactor = double.NaN;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollPresenter.MaxZoomFactor);

                Log.Comment("Attempting to set ScrollPresenter.MaxZoomFactor to double.NegativeInfinity");
                try
                {
                    scrollPresenter.MaxZoomFactor = double.NegativeInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollPresenter.MaxZoomFactor);

                Log.Comment("Attempting to set ScrollPresenter.MaxZoomFactor to double.PositiveInfinity");
                try
                {
                    scrollPresenter.MaxZoomFactor = double.PositiveInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollPresenter.MaxZoomFactor);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the InteractionTracker's VisualInteractionSource properties get set according to ScrollPresenter properties.")]
        public void VerifyInteractionSourceSettings()
        {
            using (ScrollPresenterTestHooksHelper scrollPresenterTestHooksHelper = new ScrollPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications : false))
            {
                ScrollPresenter scrollPresenter = null;
                Rectangle rectangleScrollPresenterContent = null;
                AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollPresenterContent = new Rectangle();
                    scrollPresenter = new ScrollPresenter();

                    SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    CompositionInteractionSourceCollection interactionSources = scrollPresenterTestHooksHelper.GetInteractionSources(scrollPresenter);
                    Verify.IsNotNull(interactionSources);
                    ScrollPresenterTestHooksHelper.LogInteractionSources(interactionSources);
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

                    Log.Comment("Changing ScrollPresenter properties that affect the primary VisualInteractionSource");
                    scrollPresenter.HorizontalScrollChainingMode = ChainingMode.Always;
                    scrollPresenter.VerticalScrollChainingMode = ChainingMode.Never;
                    scrollPresenter.HorizontalScrollRailingMode = RailingMode.Disabled;
                    scrollPresenter.VerticalScrollRailingMode = RailingMode.Disabled;
                    scrollPresenter.HorizontalScrollMode = ScrollMode.Enabled;
                    scrollPresenter.VerticalScrollMode = ScrollMode.Disabled;
                    scrollPresenter.ZoomChainingMode = ChainingMode.Never;
                    scrollPresenter.ZoomMode = ZoomMode.Enabled;
                    scrollPresenter.IgnoredInputKind = InputKind.All & ~InputKind.Touch;
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    CompositionInteractionSourceCollection interactionSources = scrollPresenterTestHooksHelper.GetInteractionSources(scrollPresenter);
                    Verify.IsNotNull(interactionSources);
                    ScrollPresenterTestHooksHelper.LogInteractionSources(interactionSources);
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
        [TestProperty("Description", "Decreases the ScrollPresenter.MaxZoomFactor property and verifies the ScrollPresenter.ZoomFactor value decreases accordingly. Verifies the impact on the ScrollPresenter.Content Visual.")]
        public void PinchContentThroughMaxZoomFactor()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX ScrollPresenter tests fail on RS5_Release
                return;
            }

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const double newMaxZoomFactor = 0.5;
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            Visual visualScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(
                    scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(newMaxZoomFactor < c_defaultZoomFactor);
                Verify.IsTrue(newMaxZoomFactor < c_defaultMaxZoomFactor);
                Verify.IsTrue(newMaxZoomFactor > c_defaultMinZoomFactor);
                scrollPresenter.MaxZoomFactor = newMaxZoomFactor;
            });

            IdleSynchronizer.Wait();

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy on translation and scale facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(rectangleScrollPresenterContent, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(rectangleScrollPresenterContent, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy property set");
                    visualScrollPresenterContent = ElementCompositionPreview.GetElementVisual(rectangleScrollPresenterContent);

                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);


            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(rectangleScrollPresenterContent);
                    CompositionPropertySpy.StopSpyingScaleFacade(rectangleScrollPresenterContent);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Cancelling spying");
                    CompositionPropertySpy.StopSpyingProperty(visualScrollPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(c_defaultMinZoomFactor, scrollPresenter.MinZoomFactor);
                Verify.AreEqual(newMaxZoomFactor, scrollPresenter.ZoomFactor);
                Verify.AreEqual(newMaxZoomFactor, scrollPresenter.MaxZoomFactor);
            });

            Log.Comment("Validating final transform of ScrollPresenter.Content's Visual after MaxZoomFactor change");
            CompositionGetValueStatus status;
            float offset;
            float zoomFactor;
            
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation;
                    status = CompositionPropertySpy.TryGetTranslationFacade(rectangleScrollPresenterContent, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    Verify.AreEqual(c_defaultHorizontalOffset, translation.X);
                    Verify.AreEqual(c_defaultVerticalOffset, translation.Y);

                    Vector3 scale;
                    status = CompositionPropertySpy.TryGetScaleFacade(rectangleScrollPresenterContent, out scale);
                    Log.Comment("status={0}, vertical offset={1}", status, scale.X);
                    Verify.AreEqual(newMaxZoomFactor, scale.X);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollPresenterContent, c_visualHorizontalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, horizontal offset={1}", status, offset);
                    Verify.AreEqual(c_defaultHorizontalOffset, offset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollPresenterContent, c_visualVerticalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, vertical offset={1}", status, offset);
                    Verify.AreEqual(c_defaultVerticalOffset, offset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollPresenterContent, c_visualScaleTargetedPropertyName, out zoomFactor);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                    Verify.AreEqual(newMaxZoomFactor, zoomFactor);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Increases the ScrollPresenter.MinZoomFactor property and verifies the ScrollPresenter.ZoomFactor value increases accordingly. Verifies the impact on the ScrollPresenter.Content Visual.")]
        public void StretchContentThroughMinZoomFactor()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX ScrollPresenter tests fail on RS5_Release
                return;
            }

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const double newMinZoomFactor = 2.0;
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            Visual visualScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(
                    scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(newMinZoomFactor > c_defaultZoomFactor);
                Verify.IsTrue(newMinZoomFactor > c_defaultMinZoomFactor);
                Verify.IsTrue(newMinZoomFactor < c_defaultMaxZoomFactor);
                scrollPresenter.MinZoomFactor = newMinZoomFactor;
            });

            IdleSynchronizer.Wait();

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy on translation and scale facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(rectangleScrollPresenterContent, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(rectangleScrollPresenterContent, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy property set");
                    visualScrollPresenterContent = ElementCompositionPreview.GetElementVisual(rectangleScrollPresenterContent);

                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);
            Log.Comment("Cancelling spying");

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(rectangleScrollPresenterContent);
                    CompositionPropertySpy.StopSpyingScaleFacade(rectangleScrollPresenterContent);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingProperty(visualScrollPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(newMinZoomFactor, scrollPresenter.MinZoomFactor);
                Verify.AreEqual(newMinZoomFactor, scrollPresenter.ZoomFactor);
                Verify.AreEqual(c_defaultMaxZoomFactor, scrollPresenter.MaxZoomFactor);
            });
            
            Log.Comment("Validating final transform of ScrollPresenter.Content's Visual after MinZoomFactor change");
            CompositionGetValueStatus status;
            float offset;
            float zoomFactor;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation;
                    status = CompositionPropertySpy.TryGetTranslationFacade(rectangleScrollPresenterContent, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    Verify.AreEqual(c_defaultHorizontalOffset, translation.X);
                    Verify.AreEqual(c_defaultVerticalOffset, translation.Y);

                    Vector3 scale;
                    status = CompositionPropertySpy.TryGetScaleFacade(rectangleScrollPresenterContent, out scale);
                    Log.Comment("status={0}, vertical offset={1}", status, scale.X);
                    Verify.AreEqual(newMinZoomFactor, scale.X);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollPresenterContent, c_visualHorizontalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, horizontal offset={1}", status, offset);
                    Verify.AreEqual(c_defaultHorizontalOffset, offset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollPresenterContent, c_visualVerticalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, vertical offset={1}", status, offset);
                    Verify.AreEqual(c_defaultVerticalOffset, offset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollPresenterContent, c_visualScaleTargetedPropertyName, out zoomFactor);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                    Verify.AreEqual(newMinZoomFactor, zoomFactor);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Reads the properties exposed by the ScrollPresenter.ExpressionAnimationSources CompositionPropertySet.")]
        public void ReadExpressionAnimationSources()
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(
                    scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Setting up spy property set");
                CompositionPropertySpy.StartSpyingVector2Property(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingScalarProperty(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName, 1.0f);
            });

            Log.Comment("Jumping to absolute zoomFactor");
            ZoomTo(scrollPresenter, 2.0f, 100.0f, 200.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);            

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName);
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Validating final property values of ScrollPresenter.ExpressionAnimationSources");
                CompositionGetValueStatus status;
                float zoomFactor;
                Vector2 offset;
                Vector2 position;
                Vector2 minPosition;
                Vector2 maxPosition;
                Vector2 extent;
                Vector2 viewport;

                Log.Comment("Validating final zoomFactor");
                status = CompositionPropertySpy.TryGetScalar(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName, out zoomFactor);
                Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                Verify.AreEqual(2.0f, zoomFactor);

                Log.Comment("Validating final offset");
                status = CompositionPropertySpy.TryGetVector2(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName, out offset);
                Log.Comment("status={0}, offset={1}", status, offset);
                Verify.AreEqual(0.0f, offset.X);
                Verify.AreEqual(0.0f, offset.Y);

                Log.Comment("Validating final position");
                status = CompositionPropertySpy.TryGetVector2(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName, out position);
                Log.Comment("status={0}, position={1}", status, position);
                Verify.AreEqual(100.0f, position.X);
                Verify.AreEqual(200.0f, position.Y);

                Log.Comment("Validating final minPosition");
                status = CompositionPropertySpy.TryGetVector2(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName, out minPosition);
                Log.Comment("status={0}, minPosition={1}", status, minPosition);
                Verify.AreEqual(0.0f, minPosition.X);
                Verify.AreEqual(0.0f, minPosition.Y);

                Log.Comment("Validating final maxPosition");
                status = CompositionPropertySpy.TryGetVector2(scrollPresenter.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName, out maxPosition);
                Log.Comment("status={0}, maxPosition={1}", status, maxPosition);
                Verify.AreEqual(2100.0f, maxPosition.X);
                Verify.AreEqual(1000.0f, maxPosition.Y);

                Log.Comment("Validating final extent");
                status = scrollPresenter.ExpressionAnimationSources.TryGetVector2(c_expressionAnimationSourcesExtentPropertyName, out extent);
                Log.Comment("status={0}, extent={1}", status, extent);
                Verify.AreEqual(c_defaultUIScrollPresenterContentWidth, extent.X);
                Verify.AreEqual(c_defaultUIScrollPresenterContentHeight, extent.Y);

                Log.Comment("Validating final viewport");
                status = scrollPresenter.ExpressionAnimationSources.TryGetVector2(c_expressionAnimationSourcesViewportPropertyName, out viewport);
                Log.Comment("status={0}, viewport={1}", status, viewport);
                Verify.AreEqual(c_defaultUIScrollPresenterWidth, viewport.X);
                Verify.AreEqual(c_defaultUIScrollPresenterHeight, viewport.Y);
            });
        }

        [TestMethod]
        public void ValidateXYFocusNavigation()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Skipped ValidateXYFocusNavigation because XYFocus support for third party scrollPresenters is only available in RS4+.");
                return;
            }
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

                        <controlsPrimitives:ScrollPresenter x:Name='scrollPresenter' Width='200' Height='200' HorizontalAlignment='Center' VerticalAlignment='Center'>
                            <Grid Width='600' Height='600' Background='Gray'>
                                
                                <!-- Inner buttons are larger than the outer so that they get ranked better by the XY focus algorithm. -->
                                <Button Content='Inner Left Button' HorizontalAlignment='Left' VerticalAlignment='Center'  Width='180' Height='180' />
                                <Button Content='Inner Top Button' HorizontalAlignment='Center' VerticalAlignment='Top' Width='180' Height='180' />
                                <Button Content='Inner Right Button' HorizontalAlignment='Right' VerticalAlignment='Center' Width='180' Height='180' />
                                <Button Content='Inner Bottom Button' HorizontalAlignment='Center' VerticalAlignment='Bottom' Width='180' Height='180' />

                                <Button x:Name='innerCenterButton' Content='Inner Center Button' HorizontalAlignment='Center' VerticalAlignment='Center' />
                            </Grid>
                        </controlsPrimitives:ScrollPresenter>
                    </Grid>"));

                var innerCenterButton = (Button)rootPanel.FindName("innerCenterButton");
                Content = rootPanel;
                Content.UpdateLayout();

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
        [TestProperty("Description", "Listens to the ScrollPresenter.Content.EffectiveViewportChanged event and expects it to be raised while changing offsets.")]
        public void ListenToContentEffectiveViewportChanged()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                // Skipping this test on pre-RS5 since it uses RS5's FrameworkElement.EffectiveViewportChanged event.
                return;
            }

            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            int effectiveViewportChangedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);

                rectangleScrollPresenterContent.EffectiveViewportChanged += (FrameworkElement sender, EffectiveViewportChangedEventArgs args) =>
                {
                    Log.Comment("ScrollPresenter.Content.EffectiveViewportChanged: BringIntoViewDistance=" +
                        args.BringIntoViewDistanceX + "," + args.BringIntoViewDistanceY + ", EffectiveViewport=" +
                        args.EffectiveViewport.ToString() + ", MaxViewport=" + args.MaxViewport.ToString());
                    effectiveViewportChangedCount++;
                };
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            ScrollTo(
                scrollPresenter,
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
            ScrollPresenter scrollPresenter,
            Rectangle rectangleScrollPresenterContent,
            AutoResetEvent scrollPresenterLoadedEvent,
            bool setAsContentRoot = true)
        {
            Log.Comment("Setting up default UI with ScrollPresenter" + (rectangleScrollPresenterContent == null ? "" : " and Rectangle"));

            if (rectangleScrollPresenterContent != null)
            {
                LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

                GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
                twoColorLGB.GradientStops.Add(brownGS);

                GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
                twoColorLGB.GradientStops.Add(orangeGS);

                rectangleScrollPresenterContent.Width = c_defaultUIScrollPresenterContentWidth;
                rectangleScrollPresenterContent.Height = c_defaultUIScrollPresenterContentHeight;
                rectangleScrollPresenterContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scrollPresenter);
            scrollPresenter.Width = c_defaultUIScrollPresenterWidth;
            scrollPresenter.Height = c_defaultUIScrollPresenterHeight;
            if (rectangleScrollPresenterContent != null)
            {
                scrollPresenter.Content = rectangleScrollPresenterContent;
            }

            if (scrollPresenterLoadedEvent != null)
            {
                scrollPresenter.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollPresenter.Loaded event handler");
                    scrollPresenterLoadedEvent.Set();
                };
            }

            if (setAsContentRoot)
            {
                Log.Comment("Setting window content");
                Content = scrollPresenter;
            }
        }

        private void SpyTranslationAndScale(ScrollPresenter scrollPresenter, Compositor compositor, out float horizontalOffset, out float verticalOffset, out float zoomFactor)
        {
            Verify.IsTrue(PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2));

            float horizontalOffsetTmp = 0.0f;
            float verticalOffsetTmp = 0.0f;
            float zoomFactorTmp = 1.0f;

            horizontalOffset = verticalOffset = 0.0f;
            zoomFactor = 0.0f;
            
            Visual visualScrollPresenterContent = null;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(scrollPresenter);
                    Verify.IsNotNull(scrollPresenter.Content);

                    Log.Comment("Setting up spying on facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(scrollPresenter.Content, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(scrollPresenter.Content, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(scrollPresenter);
                    Verify.IsNotNull(scrollPresenter.Content);

                    Log.Comment("Setting up spy property set");
                    visualScrollPresenterContent = ElementCompositionPreview.GetElementVisual(scrollPresenter.Content);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            Log.Comment("Cancelling spying");
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(scrollPresenter.Content);
                    CompositionPropertySpy.StopSpyingScaleFacade(scrollPresenter.Content);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingProperty(visualScrollPresenterContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollPresenterContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollPresenterContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            Log.Comment("Reading ScrollPresenter.Content's Visual Transform");
            CompositionGetValueStatus status;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation = Vector3.Zero;
                    status = CompositionPropertySpy.TryGetTranslationFacade(scrollPresenter.Content, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    horizontalOffsetTmp = translation.X;
                    verticalOffsetTmp = translation.Y;

                    Vector3 scale = Vector3.One;
                    status = CompositionPropertySpy.TryGetScaleFacade(scrollPresenter.Content, out scale);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactorTmp);
                    zoomFactorTmp = scale.X;
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollPresenterContent, c_visualHorizontalOffsetTargetedPropertyName, out horizontalOffsetTmp);
                    Log.Comment("status={0}, horizontal offset={1}", status, horizontalOffsetTmp);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollPresenterContent, c_visualVerticalOffsetTargetedPropertyName, out verticalOffsetTmp);
                    Log.Comment("status={0}, vertical offset={1}", status, verticalOffsetTmp);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollPresenterContent, c_visualScaleTargetedPropertyName, out zoomFactorTmp);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactorTmp);
                });
            }

            horizontalOffset = horizontalOffsetTmp;
            verticalOffset = verticalOffsetTmp;
            zoomFactor = zoomFactorTmp;
        }
    }
}
