// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Composition;
using Microsoft.UI.Composition.Interactions;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Private.Controls;
using Windows.Foundation;
using System.Threading;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ScrollViewerAdapterTests : ApiTestBase
    {
        private const double c_defaultExecutionRatio = 0.8;
        private const float c_tolerance = 0.001f;

        [TestMethod]
        [TestProperty("Description", "An adapter adapts the first ScrollViewer found in a tree and returns null when the tree has none.")]
        public void CanInstantiate()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                Verify.IsNotNull(adapter);

                Grid withoutScrollViewer = new Grid();
                withoutScrollViewer.Children.Add(new Border() { Child = new Button() });
                Content = withoutScrollViewer;
                Content.UpdateLayout();
                Verify.IsNull(adapter.AdaptFromTree(withoutScrollViewer, new Size(1.0, 1.0)), "Nothing to adapt without a ScrollViewer");

                ScrollViewer sv = new ScrollViewer() { Content = new Button() };
                Grid withScrollViewer = new Grid();
                withScrollViewer.Children.Add(new Border() { Child = sv });
                Content = withScrollViewer;
                Content.UpdateLayout();
                IRefreshInfoProvider provider = adapter.AdaptFromTree(withScrollViewer, new Size(1.0, 1.0));
                Verify.IsNotNull(provider, "The nested ScrollViewer is found and adapted");
                Verify.IsLessThan(Math.Abs(c_defaultExecutionRatio - provider.ExecutionRatio), 0.0001, "ExecutionRatio");
            });
        }

        [TestMethod]
        public void AdaptOnNullThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                Verify.IsNotNull(adapter);
                Verify.Throws<ArgumentException>(() => { adapter.Adapt(null, new Size(1.0, 1.0)); });
            });
        }

        [TestMethod]
        public void AdaptOnSVWithNonUIElementContentThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = new ScrollViewer();
                sv.Content = 1;
                Content = sv;
                Content.UpdateLayout();
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                Verify.IsNotNull(adapter);
                Verify.Throws<ArgumentException>(() => { adapter.Adapt(sv, new Size(1.0, 1.0)); });
            });
        }

        [TestMethod]
        public void AdaptOnSVWithoutContentThrows()
        {
            var resetEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = new ScrollViewer();
                sv.Loaded += (object sender, RoutedEventArgs e) => { resetEvent.Set(); };
                Content = sv;
                Content.UpdateLayout();
                Verify.IsNull(sv.Content);
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                Verify.IsNotNull(adapter);
                Verify.Throws<ArgumentException>(() => { adapter.Adapt(sv, new Size(1.0, 1.0)); });
            });
        }

        [TestMethod]
        [TestProperty("Description", "Adapting a loaded ScrollViewer returns a provider that is not interacting, uses the default execution ratio and publishes the InteractionRatio composition property.")]
        public void AdaptOnSV()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = new ScrollViewer();
                sv.Content = new Button();

                Content = sv;
                Content.UpdateLayout();
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                IRefreshInfoProvider provider = adapter.Adapt(sv, new Size(1.0, 1.0));
                Verify.IsNotNull(provider);
                Verify.IsLessThan(Math.Abs(c_defaultExecutionRatio - provider.ExecutionRatio), 0.0001, "ExecutionRatio");
                Verify.IsFalse(provider.IsInteractingForRefresh);
                Verify.AreEqual("InteractionRatio", provider.InteractionRatioCompositionProperty);
                Verify.IsNotNull(provider.CompositionProperties);
            });
        }

        [TestMethod]
        public void AdaptOnSVWithoutWaitingForLoaded()
        {
            var objects = new Dictionary<string, WeakReference>();
            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = new ScrollViewer();
                objects["sv"] = new WeakReference(sv);
                sv.Content = new Button();
                var adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                objects["adapter"] = new WeakReference(adapter);
                adapter.Adapt(sv, new Size(1.0, 1.0));
                Content = sv;
                Content.UpdateLayout();

                adapter = null;
                Content = null;
            });
            
            IdleSynchronizer.Wait();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            RunOnUIThread.Execute(() => CheckLeaks(objects));
        }

        [TestMethod]
        [TestProperty("Description", "The adapter hands its InteractionTracker and the ScrollViewer content to the animation handler, configures the interaction source for the pull axis, forwards refresh started/completed with the execution ratio, rejects a null animation container, and stops forwarding from a provider it no longer adapts.")]
        public void AdapterDrivesAnimationHandlerForEachPullAxis()
        {
            VerifyAnimationHandlerContract(RefreshPullDirection.TopToBottom);
            VerifyAnimationHandlerContract(RefreshPullDirection.LeftToRight);
        }

        [TestMethod]
        [TestProperty("Description", "InteractionTracker position changes are converted to an InteractionRatio (pull distance / visualizer size, clamped to 1) published in the composition property set; InteractionRatioChanged is throttled except near 0 and the execution ratio.")]
        public void InteractionRatioFollowsTrackerPositionAndIsThrottled()
        {
            RecordingAnimationHandler handler = new RecordingAnimationHandler();
            IRefreshInfoProvider provider = null;
            InteractionTracker tracker = null;
            List<double> raised = new List<double>();

            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = new ScrollViewer() { Content = new Button() { Content = "content" } };
                Border container = new Border();
                StackPanel panel = new StackPanel();
                panel.Children.Add(container);
                panel.Children.Add(sv);
                Content = panel;
                Content.UpdateLayout();

                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, handler);
                provider = adapter.Adapt(sv, new Size(100.0, 100.0));
                provider.InteractionRatioChanged += (IRefreshInfoProvider sender, RefreshInteractionRatioChangedEventArgs args) => { raised.Add(args.InteractionRatio); };
                adapter.SetAnimations(container);
                tracker = handler.Trackers.Single();
                tracker.MinPosition = new Vector3(0.0f, -200.0f, 0.0f);
            });
            IdleSynchronizer.Wait();

            int start = 0;
            RunOnUIThread.Execute(() => { start = raised.Count; });

            // Exactly at the execution ratio: always raised.
            MoveTrackerAndWaitForRatio(tracker, provider, -80.0f, 0.8f);
            // Halfway: published but throttled.
            MoveTrackerAndWaitForRatio(tracker, provider, -50.0f, 0.5f);
            // Beyond the visualizer size: clamped to 1, throttled.
            MoveTrackerAndWaitForRatio(tracker, provider, -150.0f, 1.0f);
            // Back to zero: always raised.
            MoveTrackerAndWaitForRatio(tracker, provider, 0.0f, 0.0f);

            RunOnUIThread.Execute(() =>
            {
                List<double> observed = raised.Skip(start).ToList();
                Log.Comment("Raised ratios: " + string.Join(", ", observed));
                Verify.AreEqual(2, observed.Count, "Only the ratios near the execution ratio and zero are raised");
                Verify.IsLessThan(Math.Abs(observed[0] - 0.8), 0.001);
                Verify.IsLessThan(Math.Abs(observed[1] - 0.0), 0.001);
                Verify.IsFalse(provider.IsInteractingForRefresh, "Programmatic tracker moves are not user interaction");
            });
        }

        private void VerifyAnimationHandlerContract(RefreshPullDirection direction)
        {
            Log.Comment("PullDirection = " + direction);
            RunOnUIThread.Execute(() =>
            {
                RecordingAnimationHandler handler = new RecordingAnimationHandler();
                Button firstContent = new Button() { Content = "first" };
                Button secondContent = new Button() { Content = "second" };
                ScrollViewer first = new ScrollViewer() { Content = firstContent };
                ScrollViewer second = new ScrollViewer() { Content = secondContent };
                Border visualizerStandIn = new Border();
                StackPanel panel = new StackPanel();
                panel.Children.Add(visualizerStandIn);
                panel.Children.Add(first);
                panel.Children.Add(second);
                Content = panel;
                Content.UpdateLayout();

                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(direction, handler);
                IRefreshInfoProvider provider = adapter.Adapt(first, new Size(100.0, 100.0));
                Verify.IsNotNull(provider);
                int started = 0;
                int completed = 0;
                provider.RefreshStarted += (IRefreshInfoProvider sender, object args) => { started++; };
                provider.RefreshCompleted += (IRefreshInfoProvider sender, object args) => { completed++; };
                Verify.AreEqual(0, handler.CallCount, "Adapt alone does not animate");

                adapter.SetAnimations(visualizerStandIn);
                Verify.AreEqual(1, handler.Trackers.Count);
                Verify.IsTrue(object.ReferenceEquals(visualizerStandIn, handler.TrackerVisualizers[0]));
                Verify.IsTrue(object.ReferenceEquals(firstContent, handler.TrackerInfoProviders[0]), "The ScrollViewer content is animated");
                InteractionTracker tracker = handler.Trackers[0];
                Verify.IsNotNull(tracker);
                Verify.AreEqual(Vector3.Zero, tracker.MinPosition);
                Verify.AreEqual(Vector3.Zero, tracker.MaxPosition);
                Verify.AreEqual(1.0f, tracker.MinScale);
                Verify.AreEqual(1.0f, tracker.MaxScale);
                Verify.AreEqual(1, tracker.InteractionSources.Count);
                VisualInteractionSource source = (VisualInteractionSource)tracker.InteractionSources.First();
                bool vertical = direction == RefreshPullDirection.TopToBottom || direction == RefreshPullDirection.BottomToTop;
                Verify.AreEqual(VisualInteractionSourceRedirectionMode.CapableTouchpadOnly, source.ManipulationRedirectionMode);
                Verify.AreEqual(InteractionSourceMode.Disabled, source.ScaleSourceMode);
                Verify.AreEqual(vertical ? InteractionSourceMode.EnabledWithInertia : InteractionSourceMode.Disabled, source.PositionYSourceMode);
                Verify.AreEqual(vertical ? InteractionSourceMode.Disabled : InteractionSourceMode.EnabledWithInertia, source.PositionXSourceMode);
                Verify.AreEqual(vertical ? InteractionChainingMode.Never : InteractionChainingMode.Auto, source.PositionYChainingMode);
                Verify.AreEqual(vertical ? InteractionChainingMode.Auto : InteractionChainingMode.Never, source.PositionXChainingMode);

                provider.OnRefreshStarted();
                Verify.AreEqual(1, started);
                Verify.AreEqual(1, handler.RequestedInfoProviders.Count);
                Verify.IsNull(handler.RequestedVisualizers[0]);
                Verify.IsTrue(object.ReferenceEquals(firstContent, handler.RequestedInfoProviders[0]));
                Verify.IsLessThan(Math.Abs(c_defaultExecutionRatio - handler.RequestedExecutionRatios[0]), 0.0001, "ExecutionRatio");

                provider.OnRefreshCompleted();
                Verify.AreEqual(1, completed);
                Verify.AreEqual(1, handler.CompletedInfoProviders.Count);
                Verify.IsNull(handler.CompletedVisualizers[0]);
                Verify.IsTrue(object.ReferenceEquals(firstContent, handler.CompletedInfoProviders[0]));

                Verify.Throws<ArgumentException>(() => { adapter.SetAnimations(null); });
                Verify.AreEqual(1, handler.Trackers.Count, "A rejected SetAnimations does not reach the handler");

                // Re-adapting to another ScrollViewer creates a new provider; the old one no longer drives the handler.
                IRefreshInfoProvider secondProvider = adapter.Adapt(second, new Size(100.0, 100.0));
                Verify.IsNotNull(secondProvider);
                Verify.IsFalse(object.ReferenceEquals(provider, secondProvider));
                provider.OnRefreshStarted();
                Verify.AreEqual(2, started, "The old provider still raises its own event");
                Verify.AreEqual(1, handler.RequestedInfoProviders.Count, "The adapter no longer listens to the old provider");
                secondProvider.OnRefreshStarted();
                Verify.AreEqual(2, handler.RequestedInfoProviders.Count);
                Verify.IsTrue(object.ReferenceEquals(secondContent, handler.RequestedInfoProviders[1]));
                secondProvider.OnRefreshCompleted();
                Verify.AreEqual(2, handler.CompletedInfoProviders.Count);
                Verify.IsTrue(object.ReferenceEquals(secondContent, handler.CompletedInfoProviders[1]));
            });
        }

        private static void MoveTrackerAndWaitForRatio(InteractionTracker tracker, IRefreshInfoProvider provider, float y, float expectedRatio)
        {
            RunOnUIThread.Execute(() => { tracker.TryUpdatePosition(new Vector3(0.0f, y, 0.0f)); });
            float ratio = float.NaN;
            DateTime deadline = DateTime.Now.AddSeconds(5);
            while (DateTime.Now < deadline)
            {
                RunOnUIThread.Execute(() =>
                {
                    float value;
                    if (provider.CompositionProperties.TryGetScalar(provider.InteractionRatioCompositionProperty, out value) == CompositionGetValueStatus.Succeeded)
                    {
                        ratio = value;
                    }
                });
                if (!float.IsNaN(ratio) && Math.Abs(ratio - expectedRatio) < c_tolerance)
                {
                    break;
                }
                Thread.Sleep(50);
            }
            Log.Comment("Tracker y=" + y + " -> InteractionRatio " + ratio);
            Verify.IsLessThan(Math.Abs(ratio - expectedRatio), c_tolerance, "InteractionRatio for tracker position " + y);
            IdleSynchronizer.Wait();
        }

        void CheckLeaks(Dictionary<string, WeakReference> objects)
        {
            foreach (var pair in objects)
            {
                Log.Comment("Checking if object {0} has been collected", pair.Key);
                object target = pair.Value.Target;
                if (target != null)
                {
                    Verify.DisableVerifyFailureExceptions = true; // throwing exceptions makes running this in TDP harder
                    Verify.Fail(String.Format("Object {0} is still alive when it should not be.", target));
                    Verify.DisableVerifyFailureExceptions = false;
                }
            }
            objects.Clear();
        }
    }

    partial class RecordingAnimationHandler : IAdapterAnimationHandler
    {
        public int CallCount;
        public List<UIElement> TrackerVisualizers = new List<UIElement>();
        public List<UIElement> TrackerInfoProviders = new List<UIElement>();
        public List<InteractionTracker> Trackers = new List<InteractionTracker>();
        public List<UIElement> RequestedVisualizers = new List<UIElement>();
        public List<UIElement> RequestedInfoProviders = new List<UIElement>();
        public List<double> RequestedExecutionRatios = new List<double>();
        public List<UIElement> CompletedVisualizers = new List<UIElement>();
        public List<UIElement> CompletedInfoProviders = new List<UIElement>();

        public void InteractionTrackerAnimation(UIElement refreshVisualizer, UIElement infoProvider, InteractionTracker interactionTracker)
        {
            CallCount++;
            TrackerVisualizers.Add(refreshVisualizer);
            TrackerInfoProviders.Add(infoProvider);
            Trackers.Add(interactionTracker);
        }

        public void RefreshRequestedAnimation(UIElement refreshVisualizer, UIElement infoProvider, double executionRatio)
        {
            CallCount++;
            RequestedVisualizers.Add(refreshVisualizer);
            RequestedInfoProviders.Add(infoProvider);
            RequestedExecutionRatios.Add(executionRatio);
        }

        public void RefreshCompletedAnimation(UIElement refreshVisualizer, UIElement infoProvider)
        {
            CallCount++;
            CompletedVisualizers.Add(refreshVisualizer);
            CompletedInfoProviders.Add(infoProvider);
        }
    }
}
