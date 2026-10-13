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
using Microsoft.UI.Xaml.Hosting;
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

        // Scenario: create an adapter and call AdaptFromTree on a loaded tree without a ScrollViewer, then on a tree
        //           with a ScrollViewer nested in a Border.
        // Expected: the adapter is created; the first call returns null; the second returns a provider with
        //           ExecutionRatio 0.8.
        // A failure means: the ScrollViewer adapter cannot be created, adapts something that is not a ScrollViewer, or
        //                  misses a nested one.
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

        // Scenario: call AdaptFromTree on a Grid whose ScrollViewer is exactly 10 levels down, on one where it is 11
        //           levels down, and on a root that is itself a ScrollViewer containing another ScrollViewer.
        // Expected: the 10-level ScrollViewer is adapted (ExecutionRatio 0.8); the 11-level one is not (null); the root
        //           ScrollViewer is adapted itself, and SetAnimations animates its own content, not the nested
        //           ScrollViewer's.
        // A failure means: the tree search depth changed, or the wrong ScrollViewer is adapted when a ScrollViewer is
        //                  the root.
        [TestMethod]
        [TestProperty("Description", "AdaptFromTree searches at most 10 levels below a root that is not itself a ScrollViewer: a ScrollViewer exactly 10 levels down is adapted, one 11 levels down is not. A root that is a ScrollViewer is adapted itself, not a ScrollViewer nested in its content.")]
        public void AdaptFromTreeSearchesTenLevelsBelowTheRoot()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollViewer reachable = new ScrollViewer() { Content = new Button() };
                Grid reachableRoot = new Grid();
                // Grid -> 9 Borders -> ScrollViewer: the ScrollViewer is 10 levels below the root.
                reachableRoot.Children.Add(WrapInBorders(reachable, 9));
                Content = reachableRoot;
                Content.UpdateLayout();
                Verify.AreEqual(10, DistanceFromAncestor(reachable, reachableRoot), "Precondition: depth 10");
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                IRefreshInfoProvider provider = adapter.AdaptFromTree(reachableRoot, new Size(1.0, 1.0));
                Verify.IsNotNull(provider, "A ScrollViewer 10 levels below the root is adapted");
                Verify.IsLessThan(Math.Abs(c_defaultExecutionRatio - provider.ExecutionRatio), 0.0001, "ExecutionRatio");

                ScrollViewer tooDeep = new ScrollViewer() { Content = new Button() };
                Grid tooDeepRoot = new Grid();
                tooDeepRoot.Children.Add(WrapInBorders(tooDeep, 10));
                Content = tooDeepRoot;
                Content.UpdateLayout();
                Verify.AreEqual(11, DistanceFromAncestor(tooDeep, tooDeepRoot), "Precondition: depth 11");
                ScrollViewerIRefreshInfoProviderAdapter secondAdapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                Verify.IsNull(secondAdapter.AdaptFromTree(tooDeepRoot, new Size(1.0, 1.0)), "A ScrollViewer 11 levels below the root is beyond the search depth");

                // A root that is itself a ScrollViewer is adapted directly, even when its content contains another ScrollViewer.
                ScrollViewer nested = new ScrollViewer() { Content = new Button() { Content = "nested" } };
                StackPanel rootContent = new StackPanel();
                rootContent.Children.Add(nested);
                ScrollViewer rootScrollViewer = new ScrollViewer() { Content = rootContent };
                Content = rootScrollViewer;
                Content.UpdateLayout();
                RecordingAnimationHandler handler = new RecordingAnimationHandler();
                ScrollViewerIRefreshInfoProviderAdapter thirdAdapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, handler);
                IRefreshInfoProvider rootProvider = thirdAdapter.AdaptFromTree(rootScrollViewer, new Size(1.0, 1.0));
                Verify.IsNotNull(rootProvider, "A root ScrollViewer is adapted");
                Verify.IsLessThan(Math.Abs(c_defaultExecutionRatio - rootProvider.ExecutionRatio), 0.0001, "ExecutionRatio");
                thirdAdapter.SetAnimations(new Border());
                Verify.AreEqual(1, handler.TrackerInfoProviders.Count);
                Verify.IsTrue(object.ReferenceEquals(rootContent, handler.TrackerInfoProviders[0]), "The root ScrollViewer's content is animated, not the nested ScrollViewer's");
            });
        }

        private static UIElement WrapInBorders(UIElement element, int count)
        {
            UIElement current = element;
            for (int i = 0; i < count; i++)
            {
                current = new Border() { Child = current };
            }
            return current;
        }

        private static int DistanceFromAncestor(DependencyObject element, DependencyObject ancestor)
        {
            int distance = 0;
            DependencyObject current = element;
            while (current != null && !object.ReferenceEquals(current, ancestor))
            {
                current = Microsoft.UI.Xaml.Media.VisualTreeHelper.GetParent(current);
                distance++;
            }
            return current == null ? -1 : distance;
        }

        // Scenario: call Adapt(null).
        // Expected: an ArgumentException is thrown.
        // A failure means: invalid input is accepted silently or crashes instead of being reported.
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

        // Scenario: load a ScrollViewer whose Content is the integer 1 and call Adapt on it.
        // Expected: an ArgumentException is thrown.
        // A failure means: a ScrollViewer that cannot be animated (non-UIElement content) is accepted silently or
        //                  crashes.
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

        // Scenario: load a ScrollViewer without content and call Adapt on it.
        // Expected: Content is null and an ArgumentException is thrown.
        // A failure means: a ScrollViewer without content is accepted silently or crashes.
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

        // Scenario: load a ScrollViewer with a Button as content and adapt it.
        // Expected: the provider is not null, has ExecutionRatio 0.8, is not interacting, publishes the
        //           "InteractionRatio" composition property and has a CompositionProperties set.
        // A failure means: adapting a normal ScrollViewer does not produce a usable pull-to-refresh provider.
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

        // Scenario: adapt a ScrollViewer before it is loaded, then load it, drop every reference, remove it and force a
        //           GC.
        // Expected: both the ScrollViewer and the adapter are collected.
        // A failure means: adapting a ScrollViewer before it loads leaks it or the adapter (for example through its
        //                  pending Loaded handler).
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

        // Scenario: for TopToBottom and LeftToRight, adapt one ScrollViewer with a recording animation handler, call
        //           SetAnimations, start and complete a refresh through the provider, call SetAnimations(null), then
        //           adapt a second ScrollViewer and refresh through both providers.
        // Expected: Adapt alone does not animate; SetAnimations hands the handler the visualizer stand-in, the
        //           ScrollViewer content and an InteractionTracker (min/max position zero, scale 1, one source with
        //           CapableTouchpadOnly redirection, scale disabled, position input with inertia and chaining Never
        //           only on the pull axis, chaining Auto on the other); refresh started/completed reach the handler
        //           with the content and ExecutionRatio 0.8; SetAnimations(null) throws ArgumentException without
        //           reaching the handler; after re-adapting, only the new provider drives the handler.
        // A failure means: the pull is not wired to the content animation, touch input is configured for the wrong
        //                  axis, or an old ScrollViewer keeps driving the animation.
        [TestMethod]
        [TestProperty("Description", "The adapter hands its InteractionTracker and the ScrollViewer content to the animation handler, configures the interaction source for the pull axis, forwards refresh started/completed with the execution ratio, rejects a null animation container, and stops forwarding from a provider it no longer adapts.")]
        public void AdapterDrivesAnimationHandlerForEachPullAxis()
        {
            VerifyAnimationHandlerContract(RefreshPullDirection.TopToBottom);
            VerifyAnimationHandlerContract(RefreshPullDirection.LeftToRight);
        }

        // Scenario: call SetAnimations before any Adapt with a recording handler; then use the default handler on
        //           another container before and after adapting a ScrollViewer.
        // Expected: the recording handler gets the container with no content and no tracker; the default handler
        //           enables a zero Translation on the container but not on the content before Adapt, and on the content
        //           after Adapt.
        // A failure means: setting up animations before adapting crashes, or the default animation handler animates the
        //                  wrong elements.
        [TestMethod]
        [TestProperty("Description", "SetAnimations before any Adapt hands the animation handler the container with no content and no InteractionTracker; the default handler then only enables Translation on the container, and animates the content once a ScrollViewer is adapted.")]
        public void SetAnimationsBeforeAdaptHasNoContentOrTracker()
        {
            RunOnUIThread.Execute(() =>
            {
                Border recordedContainer = new Border();
                Border defaultContainer = new Border();
                Button content = new Button() { Content = "content" };
                ScrollViewer sv = new ScrollViewer() { Content = content };
                StackPanel panel = new StackPanel();
                panel.Children.Add(recordedContainer);
                panel.Children.Add(defaultContainer);
                panel.Children.Add(sv);
                Content = panel;
                Content.UpdateLayout();

                RecordingAnimationHandler handler = new RecordingAnimationHandler();
                ScrollViewerIRefreshInfoProviderAdapter recordedAdapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, handler);
                recordedAdapter.SetAnimations(recordedContainer);
                Verify.AreEqual(1, handler.CallCount);
                Verify.IsTrue(object.ReferenceEquals(recordedContainer, handler.TrackerVisualizers[0]));
                Verify.IsNull(handler.TrackerInfoProviders[0], "No ScrollViewer has been adapted, so there is no content");
                Verify.IsNull(handler.Trackers[0], "No ScrollViewer has been adapted, so there is no InteractionTracker");

                // The default animation handler (null handler argument).
                Visual containerVisual = ElementCompositionPreview.GetElementVisual(defaultContainer);
                Visual contentVisual = ElementCompositionPreview.GetElementVisual(content);
                Vector3 translation;
                Verify.AreEqual(CompositionGetValueStatus.NotFound, containerVisual.Properties.TryGetVector3("Translation", out translation), "Precondition: container Translation not enabled");
                Verify.AreEqual(CompositionGetValueStatus.NotFound, contentVisual.Properties.TryGetVector3("Translation", out translation), "Precondition: content Translation not enabled");
                ScrollViewerIRefreshInfoProviderAdapter defaultAdapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                defaultAdapter.SetAnimations(defaultContainer);
                Verify.AreEqual(CompositionGetValueStatus.Succeeded, containerVisual.Properties.TryGetVector3("Translation", out translation), "The container is prepared for animation");
                Verify.AreEqual(Vector3.Zero, translation);
                Verify.AreEqual(CompositionGetValueStatus.NotFound, contentVisual.Properties.TryGetVector3("Translation", out translation), "No content is animated before Adapt");

                IRefreshInfoProvider provider = defaultAdapter.Adapt(sv, new Size(100.0, 100.0));
                Verify.IsNotNull(provider);
                defaultAdapter.SetAnimations(defaultContainer);
                Verify.AreEqual(CompositionGetValueStatus.Succeeded, contentVisual.Properties.TryGetVector3("Translation", out translation), "The adapted content is animated");
                Verify.AreEqual(Vector3.Zero, translation);
            });
        }

        // Scenario: adapt a ScrollViewer that is not loaded yet, then adapt a loaded one and call SetAnimations;
        //           afterwards load the first ScrollViewer.
        // Expected: both Adapt calls return a provider; the tracker has one interaction source and animates the loaded
        //           ScrollViewer's content; after the first ScrollViewer loads (Loaded raised once) the tracker still
        //           has exactly 1 interaction source.
        // A failure means: a stale Loaded handler from an earlier Adapt hooks a ScrollViewer that is no longer adapted
        //                  into the pull input.
        [TestMethod]
        [TestProperty("Description", "Adapting another ScrollViewer before the first one is loaded drops the pending Loaded handling: when the first ScrollViewer loads later, no interaction source is added to the adapter's InteractionTracker.")]
        public void ReadaptBeforeLoadedDropsThePendingLoadedHandler()
        {
            StackPanel panel = null;
            ScrollViewer notYetLoaded = null;
            InteractionTracker tracker = null;
            int notYetLoadedLoadedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                Border container = new Border();
                Button loadedContent = new Button() { Content = "loaded" };
                ScrollViewer loaded = new ScrollViewer() { Content = loadedContent };
                panel = new StackPanel();
                panel.Children.Add(container);
                panel.Children.Add(loaded);
                Content = panel;
                Content.UpdateLayout();

                notYetLoaded = new ScrollViewer() { Content = new Button() { Content = "later" } };
                notYetLoaded.Loaded += (object sender, RoutedEventArgs e) => { notYetLoadedLoadedCount++; };

                RecordingAnimationHandler handler = new RecordingAnimationHandler();
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, handler);
                Verify.IsNotNull(adapter.Adapt(notYetLoaded, new Size(100.0, 100.0)), "Adapting a ScrollViewer that is not loaded yet defers the interaction source");
                Verify.IsNotNull(adapter.Adapt(loaded, new Size(100.0, 100.0)));
                adapter.SetAnimations(container);
                tracker = handler.Trackers.Single();
                Verify.IsTrue(object.ReferenceEquals(loadedContent, handler.TrackerInfoProviders.Single()));
                Verify.AreEqual(1, tracker.InteractionSources.Count, "Precondition: one interaction source for the loaded ScrollViewer");

                panel.Children.Add(notYetLoaded);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, notYetLoadedLoadedCount, "Precondition: the first ScrollViewer has now loaded");
                Verify.AreEqual(1, tracker.InteractionSources.Count, "The first ScrollViewer's Loaded no longer reaches the adapter");
            });
        }

        // Scenario: adapt a ScrollViewer for a 100x100 visualizer, move its InteractionTracker to y = -80, -50, -150
        //           and 0, then run a 200 ms custom tracker animation to y = -80.
        // Expected: the published ratio is 0.8, 0.5, 1.0 (clamped) and 0; InteractionRatioChanged is raised only for
        //           0.8 and 0 (exactly 2 events); IsInteractingForRefresh stays false; the animation reaches ratio 0.8
        //           without raising IsInteractingForRefreshChanged.
        // A failure means: the pull distance is converted to the wrong ratio, the event throttling changed, or
        //                  programmatic tracker moves are treated as a user pull.
        [TestMethod]
        [TestProperty("Description", "InteractionTracker position changes are converted to an InteractionRatio (pull distance / visualizer size, clamped to 1) published in the composition property set; InteractionRatioChanged is throttled except near 0 and the execution ratio. A custom tracker animation drives the ratio without being treated as user interaction.")]
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

            // A custom (programmatic) tracker animation also drives the ratio, and is not user interaction either.
            int interactingChanges = 0;
            RunOnUIThread.Execute(() =>
            {
                provider.IsInteractingForRefreshChanged += (IRefreshInfoProvider sender, object args) => { interactingChanges++; };
                Vector3KeyFrameAnimation animation = tracker.Compositor.CreateVector3KeyFrameAnimation();
                animation.InsertKeyFrame(1.0f, new Vector3(0.0f, -80.0f, 0.0f));
                animation.Duration = TimeSpan.FromMilliseconds(200);
                tracker.TryUpdatePositionWithAnimation(animation);
            });
            WaitForRatio(provider, 0.8f, "custom tracker animation to y=-80");
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(provider.IsInteractingForRefresh, "A custom tracker animation is not user interaction");
                Verify.AreEqual(0, interactingChanges, "IsInteractingForRefreshChanged is not raised by a custom animation");
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
            WaitForRatio(provider, expectedRatio, "tracker position " + y);
        }

        private static void WaitForRatio(IRefreshInfoProvider provider, float expectedRatio, string context)
        {
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
            Log.Comment(context + " -> InteractionRatio " + ratio);
            Verify.IsLessThan(Math.Abs(ratio - expectedRatio), c_tolerance, "InteractionRatio for " + context);
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
