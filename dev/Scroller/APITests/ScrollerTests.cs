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

#if !BUILD_WINDOWS
using InteractionState = Microsoft.UI.Xaml.Controls.InteractionState;
using ScrollerScrollMode = Microsoft.UI.Xaml.Controls.ScrollerScrollMode;
using ScrollerZoomMode = Microsoft.UI.Xaml.Controls.ScrollerZoomMode;
using ScrollerChainingMode = Microsoft.UI.Xaml.Controls.ScrollerChainingMode;
using ScrollerRailingMode = Microsoft.UI.Xaml.Controls.ScrollerRailingMode;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
using ScrollerInputKind = Microsoft.UI.Xaml.Controls.ScrollerInputKind;
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class ScrollerTests
    {
        private const InteractionState c_defaultState = InteractionState.Idle;
        private const ScrollerChainingMode c_defaultHorizontalScrollChainingMode = ScrollerChainingMode.Auto;
        private const ScrollerChainingMode c_defaultVerticalScrollChainingMode = ScrollerChainingMode.Auto;
        private const ScrollerRailingMode c_defaultHorizontalScrollRailingMode = ScrollerRailingMode.Enabled;
        private const ScrollerRailingMode c_defaultVerticalScrollRailingMode = ScrollerRailingMode.Enabled;
        private const ScrollerScrollMode c_defaultHorizontalScrollMode = ScrollerScrollMode.Auto;
        private const ScrollerScrollMode c_defaultVerticalScrollMode = ScrollerScrollMode.Auto;
        private const ScrollerChainingMode c_defaultZoomChainingMode = ScrollerChainingMode.Auto;
        private const ScrollerZoomMode c_defaultZoomMode = ScrollerZoomMode.Disabled;
        private const ScrollerInputKind c_defaultInputKind = ScrollerInputKind.All;
        private const bool c_defaultIsChildAvailableWidthConstrained = false;
        private const bool c_defaultIsChildAvailableHeightConstrained = false;
        private const bool c_defaultIsAnchoredAtExtent = true;
        private const double c_defaultMinZoomFactor = 0.1;
        private const double c_defaultZoomFactor = 1.0;
        private const double c_defaultMaxZoomFactor = 10.0;
        private const double c_defaultHorizontalOffset = 0.0;
        private const double c_defaultVerticalOffset = 0.0;
        private const double c_defaultAnchorRatio = 0.0;

        private const double c_defaultUIScrollerChildWidth = 1200.0;
        private const double c_defaultUIScrollerChildHeight = 600.0;
        private const double c_defaultUIScrollerWidth = 300.0;
        private const double c_defaultUIScrollerHeight = 200.0;
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
        [TestProperty("Description", "Verifies the Scroller default properties.")]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                Scroller scroller = new Scroller();
                Verify.IsNotNull(scroller);

                Log.Comment("Verifying Scroller default property values");
                Verify.IsNull(scroller.HorizontalScrollController);
                Verify.IsNull(scroller.VerticalScrollController);
                Verify.IsNull(scroller.Child);
                Verify.IsNotNull(scroller.ExpressionAnimationSources);
                Verify.AreEqual(scroller.State, c_defaultState);
                Verify.AreEqual(scroller.HorizontalScrollChainingMode, c_defaultHorizontalScrollChainingMode);
                Verify.AreEqual(scroller.VerticalScrollChainingMode, c_defaultVerticalScrollChainingMode);
                Verify.AreEqual(scroller.HorizontalScrollRailingMode, c_defaultHorizontalScrollRailingMode);
                Verify.AreEqual(scroller.VerticalScrollRailingMode, c_defaultVerticalScrollRailingMode);
                Verify.AreEqual(scroller.HorizontalScrollMode, c_defaultHorizontalScrollMode);
                Verify.AreEqual(scroller.VerticalScrollMode, c_defaultVerticalScrollMode);
                Verify.AreEqual(scroller.ZoomChainingMode, c_defaultZoomChainingMode);
                Verify.AreEqual(scroller.IsChildAvailableWidthConstrained, c_defaultIsChildAvailableWidthConstrained);
                Verify.AreEqual(scroller.IsChildAvailableHeightConstrained, c_defaultIsChildAvailableHeightConstrained);
                Verify.AreEqual(scroller.ZoomMode, c_defaultZoomMode);
                Verify.AreEqual(scroller.InputKind, c_defaultInputKind);
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);
                Verify.AreEqual(scroller.ZoomFactor, c_defaultZoomFactor);
                Verify.AreEqual(scroller.HorizontalOffset, c_defaultHorizontalOffset);
                Verify.AreEqual(scroller.VerticalOffset, c_defaultVerticalOffset);
                Verify.AreEqual(scroller.HorizontalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scroller.VerticalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scroller.IsAnchoredAtHorizontalExtent, c_defaultIsAnchoredAtExtent);
                Verify.AreEqual(scroller.IsAnchoredAtVerticalExtent, c_defaultIsAnchoredAtExtent);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Exercises the Scroller property setters and getters for non-default values.")]
        public void VerifyPropertyGettersAndSetters()
        {
            Scroller scroller = null;
            Rectangle rectangle = null;

            RunOnUIThread.Execute(() =>
            {
                scroller = new Scroller();
                Verify.IsNotNull(scroller);

                rectangle = new Rectangle();
                Verify.IsNotNull(rectangle);

                Log.Comment("Setting Scroller properties to non-default values");
                scroller.Child = rectangle;
                scroller.HorizontalScrollChainingMode = ScrollerChainingMode.Always;
                scroller.VerticalScrollChainingMode = ScrollerChainingMode.Never;
                scroller.HorizontalScrollRailingMode = ScrollerRailingMode.Disabled;
                scroller.VerticalScrollRailingMode = ScrollerRailingMode.Disabled;
                scroller.HorizontalScrollMode = ScrollerScrollMode.Enabled;
                scroller.VerticalScrollMode = ScrollerScrollMode.Disabled;
                scroller.ZoomChainingMode = ScrollerChainingMode.Never;
                scroller.ZoomMode = ScrollerZoomMode.Enabled;
                scroller.InputKind = ScrollerInputKind.MouseWheel;
                scroller.IsChildAvailableWidthConstrained = !c_defaultIsChildAvailableWidthConstrained;
                scroller.IsChildAvailableHeightConstrained = !c_defaultIsChildAvailableHeightConstrained;
                scroller.MinZoomFactor = 0.5f;
                scroller.MaxZoomFactor = 2.0f;
                scroller.HorizontalAnchorRatio = 0.25f;
                scroller.VerticalAnchorRatio = 0.75f;
                scroller.IsAnchoredAtHorizontalExtent = !c_defaultIsAnchoredAtExtent;
                scroller.IsAnchoredAtVerticalExtent = !c_defaultIsAnchoredAtExtent;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verifying Scroller non-default property values");
                Verify.AreEqual(scroller.Child, rectangle);
                Verify.AreEqual(scroller.State, c_defaultState);
                Verify.AreEqual(scroller.HorizontalScrollChainingMode, ScrollerChainingMode.Always);
                Verify.AreEqual(scroller.VerticalScrollChainingMode, ScrollerChainingMode.Never);
                Verify.AreEqual(scroller.HorizontalScrollRailingMode, ScrollerRailingMode.Disabled);
                Verify.AreEqual(scroller.VerticalScrollRailingMode, ScrollerRailingMode.Disabled);
                Verify.AreEqual(scroller.HorizontalScrollMode, ScrollerScrollMode.Enabled);
                Verify.AreEqual(scroller.VerticalScrollMode, ScrollerScrollMode.Disabled);
                Verify.AreEqual(scroller.ZoomChainingMode, ScrollerChainingMode.Never);
                Verify.AreEqual(scroller.ZoomMode, ScrollerZoomMode.Enabled);
                Verify.AreEqual(scroller.InputKind, ScrollerInputKind.MouseWheel);
                Verify.AreEqual(scroller.IsChildAvailableWidthConstrained, !c_defaultIsChildAvailableWidthConstrained);
                Verify.AreEqual(scroller.IsChildAvailableHeightConstrained, !c_defaultIsChildAvailableHeightConstrained);
                Verify.AreEqual(scroller.MinZoomFactor, 0.5f);
                Verify.AreEqual(scroller.MaxZoomFactor, 2.0f);
                Verify.AreEqual(scroller.HorizontalAnchorRatio, 0.25f);
                Verify.AreEqual(scroller.VerticalAnchorRatio, 0.75f);
                Verify.AreEqual(scroller.IsAnchoredAtHorizontalExtent, !c_defaultIsAnchoredAtExtent);
                Verify.AreEqual(scroller.IsAnchoredAtVerticalExtent, !c_defaultIsAnchoredAtExtent);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts to set invalid Scroller.MinZoomFactor values.")]
        public void SetInvalidMinZoomFactorValues()
        {
            RunOnUIThread.Execute(() =>
            {
                Scroller scroller = new Scroller();

                Log.Comment("Attempting to set Scroller.MinZoomFactor to double.NaN");
                try
                {
                    scroller.MinZoomFactor = double.NaN;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);

                Log.Comment("Attempting to set Scroller.MinZoomFactor to double.NegativeInfinity");
                try
                {
                    scroller.MinZoomFactor = double.NegativeInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);

                Log.Comment("Attempting to set Scroller.MinZoomFactor to double.PositiveInfinity");
                try
                {
                    scroller.MinZoomFactor = double.PositiveInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts to set invalid Scroller.MaxZoomFactor values.")]
        public void SetInvalidMaxZoomFactorValues()
        {
            RunOnUIThread.Execute(() =>
            {
                Scroller scroller = new Scroller();

                Log.Comment("Attempting to set Scroller.MaxZoomFactor to double.NaN");
                try
                {
                    scroller.MaxZoomFactor = double.NaN;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);

                Log.Comment("Attempting to set Scroller.MaxZoomFactor to double.NegativeInfinity");
                try
                {
                    scroller.MaxZoomFactor = double.NegativeInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);

                Log.Comment("Attempting to set Scroller.MaxZoomFactor to double.PositiveInfinity");
                try
                {
                    scroller.MaxZoomFactor = double.PositiveInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the InteractionTracker's VisualInteractionSource properties get set according to Scroller properties.")]
        public void VerifyInteractionSourceSettings()
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(enableAnchorNotifications: false, enableInteractionSourcesNotifications: true))
            {
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
                    CompositionInteractionSourceCollection interactionSources = scrollerTestHooksHelper.GetInteractionSources(scroller);
                    Verify.IsNotNull(interactionSources);
                    ScrollerTestHooksHelper.LogInteractionSources(interactionSources);
                    Verify.AreEqual(interactionSources.Count, 1);
                    IEnumerator<ICompositionInteractionSource> sourcesEnumerator = (interactionSources as IEnumerable<ICompositionInteractionSource>).GetEnumerator();
                    sourcesEnumerator.MoveNext();
                    VisualInteractionSource visualInteractionSource = sourcesEnumerator.Current as VisualInteractionSource;
                    Verify.IsNotNull(visualInteractionSource);

                    Verify.AreEqual(visualInteractionSource.ManipulationRedirectionMode,
                        PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4) ? VisualInteractionSourceRedirectionMode.CapableTouchpadAndPointerWheel : VisualInteractionSourceRedirectionMode.CapableTouchpadOnly);
                    Verify.IsTrue(visualInteractionSource.IsPositionXRailsEnabled);
                    Verify.IsTrue(visualInteractionSource.IsPositionYRailsEnabled);
                    Verify.AreEqual(visualInteractionSource.PositionXChainingMode, InteractionChainingMode.Auto);
                    Verify.AreEqual(visualInteractionSource.PositionYChainingMode, InteractionChainingMode.Auto);
                    Verify.AreEqual(visualInteractionSource.ScaleChainingMode, InteractionChainingMode.Auto);
                    Verify.AreEqual(visualInteractionSource.PositionXSourceMode, InteractionSourceMode.EnabledWithInertia);
                    Verify.AreEqual(visualInteractionSource.PositionYSourceMode, InteractionSourceMode.EnabledWithInertia);
                    Verify.AreEqual(visualInteractionSource.ScaleSourceMode, InteractionSourceMode.Disabled);
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                    {
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.PositionXSourceMode, InteractionSourceRedirectionMode.Enabled);
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.PositionYSourceMode, InteractionSourceRedirectionMode.Enabled);
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.ScaleSourceMode, InteractionSourceRedirectionMode.Enabled);
                    }

                    Log.Comment("Changing Scroller properties that affect the primary VisualInteractionSource");
                    scroller.HorizontalScrollChainingMode = ScrollerChainingMode.Always;
                    scroller.VerticalScrollChainingMode = ScrollerChainingMode.Never;
                    scroller.HorizontalScrollRailingMode = ScrollerRailingMode.Disabled;
                    scroller.VerticalScrollRailingMode = ScrollerRailingMode.Disabled;
                    scroller.HorizontalScrollMode = ScrollerScrollMode.Enabled;
                    scroller.VerticalScrollMode = ScrollerScrollMode.Disabled;
                    scroller.ZoomChainingMode = ScrollerChainingMode.Never;
                    scroller.ZoomMode = ScrollerZoomMode.Enabled;
                    scroller.InputKind = ScrollerInputKind.Touch;
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    CompositionInteractionSourceCollection interactionSources = scrollerTestHooksHelper.GetInteractionSources(scroller);
                    Verify.IsNotNull(interactionSources);
                    ScrollerTestHooksHelper.LogInteractionSources(interactionSources);
                    Verify.AreEqual(interactionSources.Count, 1);
                    IEnumerator<ICompositionInteractionSource> sourcesEnumerator = (interactionSources as IEnumerable<ICompositionInteractionSource>).GetEnumerator();
                    sourcesEnumerator.MoveNext();
                    VisualInteractionSource visualInteractionSource = sourcesEnumerator.Current as VisualInteractionSource;
                    Verify.IsNotNull(visualInteractionSource);

                    Verify.AreEqual(visualInteractionSource.ManipulationRedirectionMode, VisualInteractionSourceRedirectionMode.CapableTouchpadOnly);
                    Verify.IsFalse(visualInteractionSource.IsPositionXRailsEnabled);
                    Verify.IsFalse(visualInteractionSource.IsPositionYRailsEnabled);
                    Verify.AreEqual(visualInteractionSource.PositionXChainingMode, InteractionChainingMode.Always);
                    Verify.AreEqual(visualInteractionSource.PositionYChainingMode, InteractionChainingMode.Never);
                    Verify.AreEqual(visualInteractionSource.ScaleChainingMode, InteractionChainingMode.Never);
                    Verify.AreEqual(visualInteractionSource.PositionXSourceMode, InteractionSourceMode.EnabledWithInertia);
                    Verify.AreEqual(visualInteractionSource.PositionYSourceMode, InteractionSourceMode.Disabled);
                    Verify.AreEqual(visualInteractionSource.ScaleSourceMode, InteractionSourceMode.EnabledWithInertia);
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                    {
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.PositionXSourceMode, InteractionSourceRedirectionMode.Enabled);
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.PositionYSourceMode, InteractionSourceRedirectionMode.Enabled);
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.ScaleSourceMode, InteractionSourceRedirectionMode.Enabled);
                    }
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Decreases the Scroller.MaxZoomFactor property and verifies the Scroller.ZoomFactor value decreases accordingly. Verifies the impact on the Scroller.Child Visual.")]
        public void PinchChildThroughMaxZoomFactor()
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

            const double newMaxZoomFactor = 0.5;
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            Visual visualScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(
                    scroller, rectangleScrollerChild, scrollerLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(newMaxZoomFactor < c_defaultZoomFactor);
                Verify.IsTrue(newMaxZoomFactor < c_defaultMaxZoomFactor);
                Verify.IsTrue(newMaxZoomFactor > c_defaultMinZoomFactor);
                scroller.MaxZoomFactor = newMaxZoomFactor;
            });

            IdleSynchronizer.Wait();

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy on translation and scale facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(rectangleScrollerChild, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(rectangleScrollerChild, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy property set");
                    visualScrollerChild = ElementCompositionPreview.GetElementVisual(rectangleScrollerChild);

                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerChild, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerChild, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerChild, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);


            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(rectangleScrollerChild);
                    CompositionPropertySpy.StopSpyingScaleFacade(rectangleScrollerChild);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Cancelling spying");
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerChild, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerChild, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerChild, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);
                Verify.AreEqual(scroller.ZoomFactor, newMaxZoomFactor);
                Verify.AreEqual(scroller.MaxZoomFactor, newMaxZoomFactor);
            });

            Log.Comment("Validating final transform of Scroller.Child's Visual after MaxZoomFactor change");
            CompositionGetValueStatus status;
            float offset;
            float zoomFactor;
            
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation;
                    status = CompositionPropertySpy.TryGetTranslationFacade(rectangleScrollerChild, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    Verify.AreEqual(translation.X, c_defaultHorizontalOffset);
                    Verify.AreEqual(translation.Y, c_defaultVerticalOffset);

                    Vector3 scale;
                    status = CompositionPropertySpy.TryGetScaleFacade(rectangleScrollerChild, out scale);
                    Log.Comment("status={0}, vertical offset={1}", status, scale.X);
                    Verify.AreEqual(scale.X, newMaxZoomFactor);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollerChild, c_visualHorizontalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, horizontal offset={1}", status, offset);
                    Verify.AreEqual(offset, c_defaultHorizontalOffset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerChild, c_visualVerticalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, vertical offset={1}", status, offset);
                    Verify.AreEqual(offset, c_defaultVerticalOffset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerChild, c_visualScaleTargetedPropertyName, out zoomFactor);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                    Verify.AreEqual(zoomFactor, newMaxZoomFactor);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Increases the Scroller.MinZoomFactor property and verifies the Scroller.ZoomFactor value increases accordingly. Verifies the impact on the Scroller.Child Visual.")]
        public void StretchChildThroughMinZoomFactor()
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

            const double newMinZoomFactor = 2.0;
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            Visual visualScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(
                    scroller, rectangleScrollerChild, scrollerLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(newMinZoomFactor > c_defaultZoomFactor);
                Verify.IsTrue(newMinZoomFactor > c_defaultMinZoomFactor);
                Verify.IsTrue(newMinZoomFactor < c_defaultMaxZoomFactor);
                scroller.MinZoomFactor = newMinZoomFactor;
            });

            IdleSynchronizer.Wait();

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy on translation and scale facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(rectangleScrollerChild, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(rectangleScrollerChild, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy property set");
                    visualScrollerChild = ElementCompositionPreview.GetElementVisual(rectangleScrollerChild);

                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerChild, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerChild, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerChild, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);
            Log.Comment("Cancelling spying");

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(rectangleScrollerChild);
                    CompositionPropertySpy.StopSpyingScaleFacade(rectangleScrollerChild);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerChild, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerChild, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerChild, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.MinZoomFactor, newMinZoomFactor);
                Verify.AreEqual(scroller.ZoomFactor, newMinZoomFactor);
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);
            });
            
            Log.Comment("Validating final transform of Scroller.Child's Visual after MinZoomFactor change");
            CompositionGetValueStatus status;
            float offset;
            float zoomFactor;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation;
                    status = CompositionPropertySpy.TryGetTranslationFacade(rectangleScrollerChild, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    Verify.AreEqual(translation.X, c_defaultHorizontalOffset);
                    Verify.AreEqual(translation.Y, c_defaultVerticalOffset);

                    Vector3 scale;
                    status = CompositionPropertySpy.TryGetScaleFacade(rectangleScrollerChild, out scale);
                    Log.Comment("status={0}, vertical offset={1}", status, scale.X);
                    Verify.AreEqual(scale.X, newMinZoomFactor);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollerChild, c_visualHorizontalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, horizontal offset={1}", status, offset);
                    Verify.AreEqual(offset, c_defaultHorizontalOffset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerChild, c_visualVerticalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, vertical offset={1}", status, offset);
                    Verify.AreEqual(offset, c_defaultVerticalOffset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerChild, c_visualScaleTargetedPropertyName, out zoomFactor);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                    Verify.AreEqual(zoomFactor, newMinZoomFactor);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Reads the properties exposed by the Scroller.ExpressionAnimationSources CompositionPropertySet.")]
        public void ReadExpressionAnimationSources()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(
                    scroller, rectangleScrollerChild, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Setting up spy property set");
                CompositionPropertySpy.StartSpyingVector2Property(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingScalarProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName, 1.0f);
            });

            Log.Comment("Jumping to absolute zoomFactor");
            ChangeZoomFactor(scroller, 2.0f, 100.0f, 200.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName);
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Validating final property values of Scroller.ExpressionAnimationSources");
                CompositionGetValueStatus status;
                float zoomFactor;
                Vector2 offset;
                Vector2 position;
                Vector2 minPosition;
                Vector2 maxPosition;
                Vector2 extent;
                Vector2 viewport;

                Log.Comment("Validating final zoomFactor");
                status = CompositionPropertySpy.TryGetScalar(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName, out zoomFactor);
                Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                Verify.AreEqual(zoomFactor, 2.0f);

                Log.Comment("Validating final offset");
                status = CompositionPropertySpy.TryGetVector2(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName, out offset);
                Log.Comment("status={0}, offset={1}", status, offset);
                Verify.AreEqual(offset.X, 0.0f);
                Verify.AreEqual(offset.Y, 0.0f);

                Log.Comment("Validating final position");
                status = CompositionPropertySpy.TryGetVector2(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName, out position);
                Log.Comment("status={0}, position={1}", status, position);
                Verify.AreEqual(position.X, 100.0f);
                Verify.AreEqual(position.Y, 200.0f);

                Log.Comment("Validating final minPosition");
                status = CompositionPropertySpy.TryGetVector2(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName, out minPosition);
                Log.Comment("status={0}, minPosition={1}", status, minPosition);
                Verify.AreEqual(minPosition.X, 0.0f);
                Verify.AreEqual(minPosition.Y, 0.0f);

                Log.Comment("Validating final maxPosition");
                status = CompositionPropertySpy.TryGetVector2(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName, out maxPosition);
                Log.Comment("status={0}, maxPosition={1}", status, maxPosition);
                Verify.AreEqual(maxPosition.X, 2100.0f);
                Verify.AreEqual(maxPosition.Y, 1000.0f);

                Log.Comment("Validating final extent");
                status = scroller.ExpressionAnimationSources.TryGetVector2(c_expressionAnimationSourcesExtentPropertyName, out extent);
                Log.Comment("status={0}, extent={1}", status, extent);
                Verify.AreEqual(extent.X, c_defaultUIScrollerChildWidth);
                Verify.AreEqual(extent.Y, c_defaultUIScrollerChildHeight);

                Log.Comment("Validating final viewport");
                status = scroller.ExpressionAnimationSources.TryGetVector2(c_expressionAnimationSourcesViewportPropertyName, out viewport);
                Log.Comment("status={0}, viewport={1}", status, viewport);
                Verify.AreEqual(viewport.X, c_defaultUIScrollerWidth);
                Verify.AreEqual(viewport.Y, c_defaultUIScrollerHeight);
            });
        }

        [TestMethod]
        public void ValidateXYFocusNavigation()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Skipped ValidateXYFocusNavigation because XYFocus support for third party scrollers is only available in RS4+.");
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

                        <controlsPrimitives:Scroller x:Name='scroller' Width='200' Height='200' HorizontalAlignment='Center' VerticalAlignment='Center'>
                            <Grid Width='600' Height='600' Background='Gray'>
                                
                                <!-- Inner buttons are larger than the outer so that they get ranked better by the XY focus algorithm. -->
                                <Button Content='Inner Left Button' HorizontalAlignment='Left' VerticalAlignment='Center'  Width='180' Height='180' />
                                <Button Content='Inner Top Button' HorizontalAlignment='Center' VerticalAlignment='Top' Width='180' Height='180' />
                                <Button Content='Inner Right Button' HorizontalAlignment='Right' VerticalAlignment='Center' Width='180' Height='180' />
                                <Button Content='Inner Bottom Button' HorizontalAlignment='Center' VerticalAlignment='Bottom' Width='180' Height='180' />

                                <Button x:Name='innerCenterButton' Content='Inner Center Button' HorizontalAlignment='Center' VerticalAlignment='Center' />
                            </Grid>
                        </controlsPrimitives:Scroller>
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
        [TestProperty("Description", "Listens to the Scroller.Child.EffectiveViewportChanged event and expects it to be raised while changing offsets.")]
        public void ListenToChildEffectiveViewportChanged()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                // Skipping this test on pre-RS5 since it uses RS5's FrameworkElement.EffectiveViewportChanged event.
                return;
            }

            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            int effectiveViewportChangedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);

                rectangleScrollerChild.EffectiveViewportChanged += (FrameworkElement sender, EffectiveViewportChangedEventArgs args) =>
                {
                    Log.Comment("Scroller.Child.EffectiveViewportChanged: BringIntoViewDistance=" +
                        args.BringIntoViewDistanceX + "," + args.BringIntoViewDistanceY + ", EffectiveViewport=" +
                        args.EffectiveViewport.ToString() + ", MaxViewport=" + args.MaxViewport.ToString());
                    effectiveViewportChangedCount++;
                };
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            ChangeOffsets(
                scroller,
                horizontalOffset: 250,
                verticalOffset: 150,
                viewKind: ScrollerViewKind.Absolute,
                snapPointRespect: ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                viewChangeKind: ScrollerViewChangeKind.AllowAnimation,
                hookViewChanged: false);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Expect multiple EffectiveViewportChanged occurrences during the animated offsets change.");
                Verify.IsGreaterThanOrEqual(effectiveViewportChangedCount, 1);
            });
        }

        private void SetupDefaultUI(
            Scroller scroller,
            Rectangle rectangleScrollerChild,
            AutoResetEvent scrollerLoadedEvent,
            bool setAsContentRoot = true)
        {
            Log.Comment("Setting up default UI with Scroller" + (rectangleScrollerChild == null ? "" : " and Rectangle"));

            if (rectangleScrollerChild != null)
            {
                LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

                GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
                twoColorLGB.GradientStops.Add(brownGS);

                GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
                twoColorLGB.GradientStops.Add(orangeGS);

                rectangleScrollerChild.Width = c_defaultUIScrollerChildWidth;
                rectangleScrollerChild.Height = c_defaultUIScrollerChildHeight;
                rectangleScrollerChild.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scroller);
            scroller.Width = c_defaultUIScrollerWidth;
            scroller.Height = c_defaultUIScrollerHeight;
            if (rectangleScrollerChild != null)
            {
                scroller.Child = rectangleScrollerChild;
            }

            if (scrollerLoadedEvent != null)
            {
                scroller.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Loaded event handler");
                    scrollerLoadedEvent.Set();
                };
            }

            if (setAsContentRoot)
            {
                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scroller;
            }
        }

        private void SpyTranslationAndScale(Scroller scroller, Compositor compositor, out float horizontalOffset, out float verticalOffset, out float zoomFactor)
        {
            Verify.IsTrue(PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2));

            float horizontalOffsetTmp = 0.0f;
            float verticalOffsetTmp = 0.0f;
            float zoomFactorTmp = 1.0f;

            horizontalOffset = verticalOffset = 0.0f;
            zoomFactor = 0.0f;
            
            Visual visualScrollerChild = null;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(scroller);
                    Verify.IsNotNull(scroller.Child);

                    Log.Comment("Setting up spying on facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(scroller.Child, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(scroller.Child, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(scroller);
                    Verify.IsNotNull(scroller.Child);

                    Log.Comment("Setting up spy property set");
                    visualScrollerChild = ElementCompositionPreview.GetElementVisual(scroller.Child);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerChild, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerChild, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerChild, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            Log.Comment("Cancelling spying");
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(scroller.Child);
                    CompositionPropertySpy.StopSpyingScaleFacade(scroller.Child);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerChild, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerChild, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerChild, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);


            Log.Comment("Reading Scroller.Child's Visual Transform");
            CompositionGetValueStatus status;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation = Vector3.Zero;
                    status = CompositionPropertySpy.TryGetTranslationFacade(scroller.Child, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    horizontalOffsetTmp = translation.X;
                    verticalOffsetTmp = translation.Y;

                    Vector3 scale = Vector3.One;
                    status = CompositionPropertySpy.TryGetScaleFacade(scroller.Child, out scale);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactorTmp);
                    zoomFactorTmp = scale.X;
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollerChild, c_visualHorizontalOffsetTargetedPropertyName, out horizontalOffsetTmp);
                    Log.Comment("status={0}, horizontal offset={1}", status, horizontalOffsetTmp);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerChild, c_visualVerticalOffsetTargetedPropertyName, out verticalOffsetTmp);
                    Log.Comment("status={0}, vertical offset={1}", status, verticalOffsetTmp);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerChild, c_visualScaleTargetedPropertyName, out zoomFactorTmp);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactorTmp);
                });
            }

            horizontalOffset = horizontalOffsetTmp;
            verticalOffset = verticalOffsetTmp;
            zoomFactor = zoomFactorTmp;
        }
    }
}
