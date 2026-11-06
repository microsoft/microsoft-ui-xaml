// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Private.Controls;
using Windows.Foundation;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml.Hosting;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class RefreshVisualizerTests : ApiTestBase
    {
        TimeSpan c_MaxWaitDuration = TimeSpan.FromMilliseconds(5000);

        // Values mirrored from RefreshVisualizer.cpp.
        private const float c_minimumIndicatorOpacity = 0.4f;
        private const float c_fullIndicatorOpacity = 1.0f;
        private const float c_tolerance = 0.001f;
        private const float c_halfPi = (float)(Math.PI / 2);

        // Scenario: put a default RefreshVisualizer in the test window and wait for its Loaded event.
        // Expected: Loaded is raised within the wait time; State is Idle, Orientation is Auto, there is no
        //           InfoProvider, the default style height is 100, and the default content is a SymbolIcon showing
        //           Symbol.Refresh at the idle indicator opacity 0.4.
        // A failure means: the visualizer cannot be created or loaded, or its defaults (state, orientation, height,
        //                  refresh icon, idle opacity) changed.
        [TestMethod]
        public void CanInstantiate()
        {
            AutoResetEvent refreshVisualizerLoadedEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                refreshVisualizer.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("RefreshVisualizer.Loaded event handler");
                    refreshVisualizerLoadedEvent.Set();
                };
                Content = refreshVisualizer;
            });

            Log.Comment("Waiting for Loaded event");
            Verify.IsTrue(refreshVisualizerLoadedEvent.WaitOne(c_MaxWaitDuration), "RefreshVisualizer.Loaded should be raised");
            Log.Comment("Default UI set up");

            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = Content as RefreshVisualizer;
                Verify.IsNotNull(refreshVisualizer);
                Verify.AreEqual(RefreshVisualizerState.Idle, refreshVisualizer.State);
                Verify.AreEqual(RefreshVisualizerOrientation.Auto, refreshVisualizer.Orientation);
                Verify.IsNull(((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider);
                Verify.AreEqual(100.0, refreshVisualizer.Height, "Default style height");
                Verify.AreEqual(Symbol.Refresh, ((SymbolIcon)refreshVisualizer.Content).Symbol);
                VerifyClose(c_minimumIndicatorOpacity, ElementCompositionPreview.GetElementVisual(refreshVisualizer.Content).Opacity, "Idle indicator opacity");
            });
        }

        // Scenario: give an untemplated visualizer an IRefreshInfoProvider test double and call RequestRefresh with no
        //           RefreshRequested handler.
        // Expected: the provider's OnRefreshStarted and OnRefreshCompleted go from 0 calls to exactly 1 call each.
        // A failure means: the visualizer no longer tells its info provider (for example the ScrollViewer adapter) when
        //                  a refresh starts and ends, so the pulled content is not held and released around the
        //                  refresh.
        [TestMethod]
        public void RefreshPropagatesToInfoProvider()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Compositor compositor = ElementCompositionPreview.GetElementVisual(refreshVisualizer).Compositor;
                RefreshInfoProviderImpl refreshInfoProviderImpl = new RefreshInfoProviderImpl(compositor);
                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = refreshInfoProviderImpl;
                Verify.AreEqual<int>(0, refreshInfoProviderImpl.OnRefreshStartedCalls);
                Verify.AreEqual<int>(0, refreshInfoProviderImpl.OnRefreshCompletedCalls);
                refreshVisualizer.RequestRefresh();
                Verify.AreEqual<int>(1, refreshInfoProviderImpl.OnRefreshStartedCalls);
                Verify.AreEqual<int>(1, refreshInfoProviderImpl.OnRefreshCompletedCalls);
            });
        }

        // Scenario: create a visualizer, load it, then replace its default indicator with SymbolIcon(Cancel).
        // Expected: Content is null before the template is applied and the default SymbolIcon(Refresh) after load;
        //           after the replacement the template's Root grid hosts exactly the new content (one child, the same
        //           instance), the previous indicator has no parent, and the new content is centered horizontally and
        //           vertically.
        // A failure means: replacing the refresh indicator does not show the app's content in its place, leaves the old
        //                  indicator behind, or positions the new content wrongly.
        [TestMethod]
        public void ContentIsUpdatable()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                //The default Progress Indicator is consturcted in the OnApplyTemplate method, since that is not called in this test
                //we expect the Content to be null.
                Verify.IsNull(refreshVisualizer.Content);
                Content = refreshVisualizer;
                Content.UpdateLayout();
                Verify.IsNotNull(refreshVisualizer.Content);
                Verify.AreEqual<Symbol>(Symbol.Refresh, ((SymbolIcon)(refreshVisualizer.Content)).Symbol);
                SymbolIcon defaultIndicator = (SymbolIcon)refreshVisualizer.Content;
                refreshVisualizer.Content = new SymbolIcon(Symbol.Cancel);
                Verify.AreEqual<Symbol>(Symbol.Cancel, ((SymbolIcon)(refreshVisualizer.Content)).Symbol);

                // The replacement is what the template hosts, centered, and the default indicator is removed.
                Grid root = (Grid)VisualTreeHelper.GetChild(refreshVisualizer, 0);
                Verify.AreEqual("Root", root.Name);
                Verify.AreEqual(1, root.Children.Count, "Only the new content is hosted");
                Verify.IsTrue(object.ReferenceEquals(refreshVisualizer.Content, root.Children[0]), "Root hosts the replaced content");
                Verify.IsNull(VisualTreeHelper.GetParent(defaultIndicator), "The previous indicator is removed from Root");
                Verify.AreEqual(HorizontalAlignment.Center, ((FrameworkElement)refreshVisualizer.Content).HorizontalAlignment);
                Verify.AreEqual(VerticalAlignment.Center, ((FrameworkElement)refreshVisualizer.Content).VerticalAlignment);
            });
        }

        // Scenario: set Content to a 50x30 Border before the visualizer is in the tree, then load it.
        // Expected: before load the Border has no parent and Stretch alignment; after load Root hosts only that Border
        //           (no default indicator), Content is still the Border, it is centered both ways and keeps its 50x30
        //           actual size.
        // A failure means: content set before the template is applied is lost, shown next to a default indicator, or
        //                  not centered or sized as the app set it.
        [TestMethod]
        [TestProperty("Description", "Content assigned before the template is applied is hosted, centered, in the template's Root instead of a default indicator.")]
        public void ContentSetBeforeTemplateIsCenteredInRoot()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Border content = new Border() { Width = 50, Height = 30 };
                refreshVisualizer.Content = content;
                Verify.IsNull(VisualTreeHelper.GetParent(content), "Precondition: not hosted before the template is applied");
                Verify.AreEqual(HorizontalAlignment.Stretch, content.HorizontalAlignment, "Precondition: default alignment");

                Content = refreshVisualizer;
                Content.UpdateLayout();

                Grid root = (Grid)VisualTreeHelper.GetChild(refreshVisualizer, 0);
                Verify.AreEqual("Root", root.Name);
                Verify.AreEqual(1, root.Children.Count, "No default indicator is added next to the app content");
                Verify.IsTrue(object.ReferenceEquals(content, root.Children[0]), "The early content is hosted in Root");
                Verify.IsTrue(object.ReferenceEquals(content, refreshVisualizer.Content));
                Verify.AreEqual(HorizontalAlignment.Center, content.HorizontalAlignment);
                Verify.AreEqual(VerticalAlignment.Center, content.VerticalAlignment);
                Verify.AreEqual(50.0, content.ActualWidth);
                Verify.AreEqual(30.0, content.ActualHeight);
            });
        }

        // Scenario: never add the visualizer to the tree (no template), give it a 30x30 Border as Content and a
        //           provider test double, start interacting at ratio 0.1, drop the ratio to 0 and release.
        // Expected: Interacting at 0.1, Idle at 0 and after release, indicator opacity 0.4 throughout, transitions
        //           exactly [Idle->Interacting, Interacting->Idle], and the visualizer still has no template children.
        // A failure means: the pull state machine or the indicator fade depends on the template being applied, so an
        //                  untemplated visualizer crashes or skips states.
        [TestMethod]
        [TestProperty("Description", "Without a template the visualizer still runs its state machine on its explicit Content: interacting below the execution ratio is Interacting with the minimum indicator opacity.")]
        public void InteractingWithoutTemplateFadesIndicator()
        {
            RunOnUIThread.Execute(() =>
            {
                // Never added to the tree, so OnApplyTemplate never runs and there is no Root.
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Border content = new Border() { Width = 30, Height = 30 };
                refreshVisualizer.Content = content;
                RefreshInfoProviderImpl provider = new RefreshInfoProviderImpl(ElementCompositionPreview.GetElementVisual(refreshVisualizer).Compositor);
                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = provider;
                List<string> transitions = RecordTransitions(refreshVisualizer);
                Visual indicator = ElementCompositionPreview.GetElementVisual(content);
                Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(refreshVisualizer), "Precondition: no template");

                SetInteracting(provider, true);
                provider.RaiseInteractionRatioChangedEvent(0.1f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Interacting, c_minimumIndicatorOpacity);

                provider.RaiseInteractionRatioChangedEvent(0.0f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);
                SetInteracting(provider, false);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);

                VerifyTransitions(new string[] { "Idle->Interacting", "Interacting->Idle" }, transitions);
                Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(refreshVisualizer), "Still untemplated");
            });
        }

        // Scenario: give the provider an out-of-range ExecutionRatio of -0.5, start interacting and send the ratios
        //           0.1, -0.2, 0, 0.1, exactly 0, 0.1 and -0.6, then release.
        // Expected: Pending (opacity 1.0) at 0.1; Idle at -0.2 and at 0; Pending at 0.1; Idle at exactly 0 from
        //           Pending; Pending at 0.1; Interacting at -0.6 (at or below the execution ratio); Idle on release; no
        //           refresh is started; transitions exactly [Idle->Pending, Pending->Idle, Idle->Pending,
        //           Pending->Idle, Idle->Pending, Pending->Interacting, Interacting->Idle].
        // A failure means: pulling back from an armed refresh no longer returns to the right state (Idle once the ratio
        //                  is zero or below, Interacting at or below the execution ratio).
        [TestMethod]
        [TestProperty("Description", "With an (out-of-range) negative ExecutionRatio, a Pending pull falls back to Idle once the ratio drops to zero or below while staying above the execution ratio, and to Interacting at or below the execution ratio.")]
        public void NegativeExecutionRatioFallsBackFromPendingToIdle()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Content = refreshVisualizer;
                Content.UpdateLayout();

                RefreshInfoProviderImpl provider = new RefreshInfoProviderImpl(ElementCompositionPreview.GetElementVisual(refreshVisualizer).Compositor);
                // The visualizer reads ExecutionRatio when the provider is assigned.
                provider.ExecutionRatio = -0.5;
                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = provider;
                List<string> transitions = RecordTransitions(refreshVisualizer);
                Visual indicator = ElementCompositionPreview.GetElementVisual(refreshVisualizer.Content);
                SetInteracting(provider, true);

                // From zero, any ratio above -0.5 is past the execution ratio.
                provider.RaiseInteractionRatioChangedEvent(0.1f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Pending, c_fullIndicatorOpacity);
                // Above the execution ratio but not positive: Idle.
                provider.RaiseInteractionRatioChangedEvent(-0.2f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);

                // Back at zero, then Pending again; from Pending a ratio of exactly 0 (above the execution ratio, not
                // positive) is Idle; at or below the execution ratio it is Interacting instead.
                provider.RaiseInteractionRatioChangedEvent(0.0f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);
                provider.RaiseInteractionRatioChangedEvent(0.1f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Pending, c_fullIndicatorOpacity);
                provider.RaiseInteractionRatioChangedEvent(0.0f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);
                provider.RaiseInteractionRatioChangedEvent(0.1f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Pending, c_fullIndicatorOpacity);
                provider.RaiseInteractionRatioChangedEvent(-0.6f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Interacting, c_minimumIndicatorOpacity);

                SetInteracting(provider, false);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);
                Verify.AreEqual(0, provider.OnRefreshStartedCalls, "No refresh was requested");
                VerifyTransitions(new string[] {
                    "Idle->Pending", "Pending->Idle", "Idle->Pending", "Pending->Idle", "Idle->Pending", "Pending->Interacting", "Interacting->Idle" }, transitions);
            });
        }

        // Scenario: use the private PullToRefreshHelperTestApi to create RefreshStateChangedEventArgs (Idle->Pending
        //           and Refreshing->Peeking) and RefreshRequestedEventArgs that wrap a Deferral, take two deferrals
        //           from those args and complete them one after the other.
        // Expected: OldState/NewState are exactly the pairs given; the wrapped deferral has completed 0 times after the
        //           first deferral completes and exactly once after the second.
        // A failure means: the test helper builds event args with wrong or swapped states, or RefreshRequestedEventArgs
        //                  completes the refresh before every deferral taken from it is done (a refresh would end
        //                  early).
        [TestMethod]
        [TestProperty("Description", "PullToRefreshHelperTestApi creates RefreshStateChangedEventArgs with exactly the given states and RefreshRequestedEventArgs whose deferral completes only after every deferral taken from it is completed.")]
        public void HelperTestApiCreatesEventArgsWithTheGivenValues()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshStateChangedEventArgs idleToPending = PullToRefreshHelperTestApi.CreateRefreshStateChangedEventArgsInstance(RefreshVisualizerState.Idle, RefreshVisualizerState.Pending);
                Verify.AreEqual(RefreshVisualizerState.Idle, idleToPending.OldState);
                Verify.AreEqual(RefreshVisualizerState.Pending, idleToPending.NewState);
                RefreshStateChangedEventArgs refreshingToPeeking = PullToRefreshHelperTestApi.CreateRefreshStateChangedEventArgsInstance(RefreshVisualizerState.Refreshing, RefreshVisualizerState.Peeking);
                Verify.AreEqual(RefreshVisualizerState.Refreshing, refreshingToPeeking.OldState);
                Verify.AreEqual(RefreshVisualizerState.Peeking, refreshingToPeeking.NewState);

                int completed = 0;
                RefreshRequestedEventArgs args = PullToRefreshHelperTestApi.CreateRefreshRequestedEventArgsInstance(new Deferral(() => { completed++; }));
                Verify.IsNotNull(args);
                Deferral first = args.GetDeferral();
                Deferral second = args.GetDeferral();
                first.Complete();
                Verify.AreEqual(0, completed, "One deferral is still outstanding");
                second.Complete();
                Verify.AreEqual(1, completed, "The supplied deferral completes once the last deferral is completed");
            });
        }

        // Scenario: load a visualizer, set Content to SymbolIcon(Cancel), then set it to null and request a refresh.
        // Expected: Root hosts the Cancel icon; after null the Content property stays null but Root hosts exactly one
        //           default 30x30 SymbolIcon(Refresh); the refresh completes synchronously with transitions exactly
        //           [Idle->Refreshing, Refreshing->Idle].
        // A failure means: clearing the content crashes, leaves no indicator or two indicators, or breaks refreshing.
        [TestMethod]
        [TestProperty("Description", "Setting Content to null after the template is applied keeps the property null but hosts a default refresh indicator, and refreshes still cycle Idle -> Refreshing -> Idle.")]
        public void NullContentDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Content = refreshVisualizer;
                Content.UpdateLayout();
                Grid root = (Grid)VisualTreeHelper.GetChild(refreshVisualizer, 0);
                Verify.AreEqual("Root", root.Name);

                refreshVisualizer.Content = new SymbolIcon(Symbol.Cancel);
                Verify.AreEqual(1, root.Children.Count);
                Verify.AreEqual(Symbol.Cancel, ((SymbolIcon)root.Children[0]).Symbol);

                refreshVisualizer.Content = null;
                Verify.IsNull(refreshVisualizer.Content, "The Content property keeps the value the app set");
                Verify.AreEqual(1, root.Children.Count, "A single indicator is hosted");
                SymbolIcon fallback = root.Children[0] as SymbolIcon;
                Verify.IsNotNull(fallback, "A default indicator replaces the removed content");
                Verify.AreEqual(Symbol.Refresh, fallback.Symbol);
                Verify.AreEqual(30.0, fallback.Width);
                Verify.AreEqual(30.0, fallback.Height);

                List<string> transitions = RecordTransitions(refreshVisualizer);
                refreshVisualizer.RequestRefresh();
                Verify.AreEqual(RefreshVisualizerState.Idle, refreshVisualizer.State, "No deferral was taken, so the refresh completes synchronously");
                VerifyTransitions(new string[] { "Idle->Refreshing", "Refreshing->Idle" }, transitions);
            });
        }

        // Scenario: refresh once with a provider test double, clear InfoProvider, refresh again, then raise a ratio
        //           change (0.5) on the removed provider.
        // Expected: the provider sees exactly 1 start and 1 completion (the first refresh only); InfoProvider is null;
        //           the second refresh goes [Idle->Refreshing, Refreshing->Idle]; the removed provider's event changes
        //           nothing (still Idle, still 2 transitions).
        // A failure means: clearing the info provider breaks refreshing, keeps notifying the old provider, or keeps
        //                  reacting to its events.
        [TestMethod]
        [TestProperty("Description", "After the InfoProvider is cleared, refreshes still complete and the previous provider is no longer notified.")]
        public void NullInfoProviderDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Compositor compositor = ElementCompositionPreview.GetElementVisual(refreshVisualizer).Compositor;
                RefreshInfoProviderImpl refreshInfoProviderImpl = new RefreshInfoProviderImpl(compositor);
                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = refreshInfoProviderImpl;
                refreshVisualizer.RequestRefresh();
                Verify.AreEqual(1, refreshInfoProviderImpl.OnRefreshStartedCalls);
                Verify.AreEqual(1, refreshInfoProviderImpl.OnRefreshCompletedCalls);

                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = null;
                Verify.IsNull(((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider);
                List<string> transitions = RecordTransitions(refreshVisualizer);
                refreshVisualizer.RequestRefresh();
                VerifyTransitions(new string[] { "Idle->Refreshing", "Refreshing->Idle" }, transitions);
                Verify.AreEqual(1, refreshInfoProviderImpl.OnRefreshStartedCalls, "Removed provider must not be notified");
                Verify.AreEqual(1, refreshInfoProviderImpl.OnRefreshCompletedCalls, "Removed provider must not be notified");

                // Events raised by the removed provider no longer reach the visualizer.
                refreshInfoProviderImpl.RaiseInteractionRatioChangedEvent(0.5f);
                Verify.AreEqual(RefreshVisualizerState.Idle, refreshVisualizer.State);
                Verify.AreEqual(2, transitions.Count);
            });
        }

        // Scenario: call RequestRefresh on a visualizer whose RefreshRequested handler takes no deferral.
        // Expected: the event is raised exactly once with the visualizer as sender, non-null args and State Refreshing
        //           during the event; afterwards State is back to Idle.
        // A failure means: apps are not told that a refresh was requested, see the wrong sender or state, or the
        //                  refresh does not complete when no deferral is taken.
        [TestMethod]
        public void RefreshRequestedEventFires()
        {
            int refreshRequestedCount = 0;
            object observedSender = null;
            RefreshVisualizerState stateDuringEvent = RefreshVisualizerState.Idle;
            RefreshVisualizer visualizer = null;

            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                visualizer = refreshVisualizer;
                refreshVisualizer.RefreshRequested += (RefreshVisualizer sender, RefreshRequestedEventArgs args) =>
                {
                    refreshRequestedCount++;
                    observedSender = sender;
                    stateDuringEvent = sender.State;
                    Verify.IsNotNull(args);
                };
                refreshVisualizer.RequestRefresh();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(1, refreshRequestedCount);
                Verify.IsTrue(object.ReferenceEquals(visualizer, observedSender), "Sender is the visualizer");
                Verify.AreEqual(RefreshVisualizerState.Refreshing, stateDuringEvent, "State is Refreshing while RefreshRequested is raised");
                Verify.AreEqual(RefreshVisualizerState.Idle, visualizer.State, "Refresh completes when no deferral is taken");
            });
        }

        // Scenario: give a visualizer that is never added to the tree a counting IRefreshInfoProvider that the test
        //           keeps alive, drop the visualizer and force garbage collection (up to 5 passes, waiting for the UI
        //           thread to go idle).
        // Expected: while the visualizer lives the provider has 1 InteractionRatioChanged and 1
        //           IsInteractingForRefreshChanged subscriber; after release the visualizer's Tag sentinel is
        //           collected, both counts are 0 and exactly 2 handlers were removed (the InfoProvider never changes,
        //           so only the destructor can remove them).
        // A failure means: a destroyed RefreshVisualizer leaves its handlers on a provider that outlives it, which can
        //                  later call into freed memory.
        [TestMethod]
        [TestProperty("Description", "A RefreshVisualizer that is released while its InfoProvider is still alive unsubscribes from the provider's IsInteractingForRefreshChanged and InteractionRatioChanged events when it is destroyed.")]
        // Runs in its own test host. In the shared API-test host this test twice crashed the host during its first forced GC,
        // with a framework assert: 0xC0000420 STATUS_ASSERTION_FAILURE in CImageSource::SetBitmapState (Microsoft.UI.Xaml.dll;
        // validation runs h1 and m2). That GC also releases objects left over by earlier tests. Taking the compositor from
        // GetCompositorForCurrentThread() instead of an orphan element's visual (below) is a mitigation, not the root cause.
        // The release cannot be made deterministic without the GC: the cast to IRefreshVisualizerPrivate caches its own
        // native reference in the projected wrapper, and CsWinRT does not expose that cache for disposal.
        [TestProperty("IsolationLevel", "Method")]
        public void ReleasedVisualizerUnsubscribesFromALiveInfoProvider()
        {
            CountingRefreshInfoProvider provider = null;
            WeakReference visualizerSentinel = null;

            RunOnUIThread.Execute(() =>
            {
                // The thread's compositor, not GetElementVisual(new Border()): no hand-off visual on an orphan element
                // becomes garbage during the forced collections below.
                provider = new CountingRefreshInfoProvider(global::Microsoft.UI.Xaml.Media.CompositionTarget.GetCompositorForCurrentThread());
                visualizerSentinel = CreateVisualizerUsingProvider(provider);
                Verify.AreEqual(1, provider.InteractionRatioChangedSubscribers, "Precondition: the visualizer listens to InteractionRatioChanged");
                Verify.AreEqual(1, provider.IsInteractingForRefreshChangedSubscribers, "Precondition: the visualizer listens to IsInteractingForRefreshChanged");
            });
            IdleSynchronizer.Wait();

            // Only the provider is kept alive; the visualizer (never in the tree, no other references) is collected and its
            // final release is completed on the UI thread.
            for (int i = 0; i < 5 && visualizerSentinel.IsAlive; i++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                IdleSynchronizer.Wait();
                Log.Comment(string.Format("Iteration {0}: visualizer alive={1}, InteractionRatioChanged subscribers={2}, IsInteractingForRefreshChanged subscribers={3}",
                    i, visualizerSentinel.IsAlive, Volatile.Read(ref provider.InteractionRatioChangedSubscribers), Volatile.Read(ref provider.IsInteractingForRefreshChangedSubscribers)));
            }
            IdleSynchronizer.Wait();

            Verify.IsFalse(visualizerSentinel.IsAlive, "The visualizer was released");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, provider.InteractionRatioChangedSubscribers, "InteractionRatioChanged handler removed");
                Verify.AreEqual(0, provider.IsInteractingForRefreshChangedSubscribers, "IsInteractingForRefreshChanged handler removed");
                Verify.AreEqual(2, provider.TotalRemoves, "Exactly the two handlers added by the visualizer were removed");
            });
            GC.KeepAlive(provider);
        }

        // Kept out of line so that no reference to the visualizer survives in the caller's frame.
        [global::System.Runtime.CompilerServices.MethodImpl(global::System.Runtime.CompilerServices.MethodImplOptions.NoInlining)]
        private static WeakReference CreateVisualizerUsingProvider(IRefreshInfoProvider provider)
        {
            // The managed Tag value is held by the native visualizer, so it stays alive as long as the visualizer does.
            object sentinel = new object();
            RefreshVisualizer refreshVisualizer = new RefreshVisualizer() { Tag = sentinel };
            ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = provider;
            return new WeakReference(sentinel);
        }

        // Scenario: place a visualizer in the test window with a provider test double and a RefreshRequested handler
        //           that takes a deferral and completes it on a thread-pool thread, then request a refresh.
        // Expected: OnRefreshStarted is called once and OnRefreshCompleted is never called: a deferral completed off
        //           the UI thread does not complete the refresh (the current, documented-by-test behaviour).
        // A failure means: the behaviour of a deferral completed off the UI thread changed (the refresh now completes,
        //                  or its start is not reported).
        [TestMethod]
        public void RefreshCompletedDeferralCompletedOffUIThread()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Compositor compositor = ElementCompositionPreview.GetElementVisual(refreshVisualizer).Compositor;
                RefreshInfoProviderImpl refreshInfoProviderImpl = new RefreshInfoProviderImpl(compositor);
                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = refreshInfoProviderImpl;
                refreshVisualizer.RefreshRequested += (RefreshVisualizer sender, RefreshRequestedEventArgs args) => 
                {
                    Deferral def = args.GetDeferral();

                    global::Windows.System.Threading.ThreadPool.RunAsync(
                        (IAsyncAction action) =>
                        {
                            Log.Comment("RefreshVisualizer.RefreshRequested event handler");
                            def.Complete();
                        }).AsTask().Wait();
                };

                Verify.AreEqual<int>(0, refreshInfoProviderImpl.OnRefreshStartedCalls);
                Verify.AreEqual<int>(0, refreshInfoProviderImpl.OnRefreshCompletedCalls);

                Content = refreshVisualizer;
                
                refreshVisualizer.RequestRefresh();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = (RefreshVisualizer)Content;
                RefreshInfoProviderImpl refreshInfoProviderImpl = (RefreshInfoProviderImpl)((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider;

                Verify.AreEqual<int>(1, refreshInfoProviderImpl.OnRefreshStartedCalls);
                //Since we are completing the deferral off the UI thread the refresh never completed.
                Verify.AreEqual<int>(0, refreshInfoProviderImpl.OnRefreshCompletedCalls);
            });

        }

        // Scenario: subscribe a RefreshStateChanged handler and request a refresh; remove the handler and refresh
        //           again; then add a new recording handler and refresh a third time.
        // Expected: the first refresh raises exactly 2 events [Idle->Refreshing, Refreshing->Idle] with State already
        //           equal to NewState in each; after removal the count stays 2 while the refresh still completes; the
        //           new handler sees exactly [Idle->Refreshing, Refreshing->Idle].
        // A failure means: state-change notifications are missing, out of order or raised before State is updated, or
        //                  removing a handler does not stop it.
        [TestMethod]
        public void RefreshStateChangedEventFires()
        {
            int refreshStateChangedCount = 0;
            List<string> transitions = new List<string>();
            RefreshVisualizer visualizer = null;
            TypedEventHandler<RefreshVisualizer, RefreshStateChangedEventArgs> handler = (RefreshVisualizer sender, RefreshStateChangedEventArgs args) =>
            {
                refreshStateChangedCount++;
                transitions.Add(args.OldState + "->" + args.NewState);
                Verify.AreEqual(args.NewState, sender.State, "State already reflects NewState when the event is raised");
            };

            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                visualizer = refreshVisualizer;
                refreshVisualizer.RefreshStateChanged += handler;
                refreshVisualizer.RequestRefresh();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual<int>(2, refreshStateChangedCount);
                VerifyTransitions(new string[] { "Idle->Refreshing", "Refreshing->Idle" }, transitions);

                // A removed handler is no longer invoked; a handler added afterwards is.
                visualizer.RefreshStateChanged -= handler;
                visualizer.RequestRefresh();
                Verify.AreEqual(RefreshVisualizerState.Idle, visualizer.State, "The refresh completed");
                Verify.AreEqual<int>(2, refreshStateChangedCount, "The removed handler was not invoked");

                List<string> laterTransitions = RecordTransitions(visualizer);
                visualizer.RequestRefresh();
                VerifyTransitions(new string[] { "Idle->Refreshing", "Refreshing->Idle" }, laterTransitions);
                Verify.AreEqual<int>(2, refreshStateChangedCount, "The removed handler stays removed");
            });
        }

        // Scenario: load a visualizer and drive a provider test double (ExecutionRatio 0.8) through a full pull: start
        //           interacting, ratios 0.5, 0.9, 0.6, 0, 0.9, release (the RefreshRequested handler takes a deferral),
        //           ratio 0.3 plus another press and release while refreshing, complete the deferral, then
        //           not-interacting ratios 0.2 and 0, a pull to 0.5 that is released, and a new press that starts at
        //           ratio 0.4.
        // Expected: every step has the expected state and indicator opacity (0.4 for Idle/Interacting, 1.0 for
        //           Pending/Refreshing/Peeking); no refresh while still interacting; releasing in Pending starts
        //           exactly one refresh (1 deferral, OnRefreshStarted 1) that ratio changes and another release do not
        //           interrupt; completing the deferral returns to Idle with OnRefreshCompleted 1; the 13 transitions
        //           match exactly.
        // A failure means: the pull-to-refresh state machine or its indicator feedback changed, for example a refresh
        //                  starts too early, twice, or not on release.
        [TestMethod]
        [TestProperty("Description", "Drives the interaction state machine through an IRefreshInfoProvider test double and verifies every transition, the RefreshStateChanged arguments, the indicator opacity of each state and the refresh started on release from Pending.")]
        public void InteractionRatioAndInteractingChangesDriveRefreshStates()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Content = refreshVisualizer;
                Content.UpdateLayout();

                RefreshInfoProviderImpl provider = new RefreshInfoProviderImpl(ElementCompositionPreview.GetElementVisual(refreshVisualizer).Compositor);
                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = provider;
                List<string> transitions = RecordTransitions(refreshVisualizer);
                List<Deferral> deferrals = new List<Deferral>();
                refreshVisualizer.RefreshRequested += (RefreshVisualizer sender, RefreshRequestedEventArgs args) => { deferrals.Add(args.GetDeferral()); };
                Visual indicator = ElementCompositionPreview.GetElementVisual(refreshVisualizer.Content);

                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);

                SetInteracting(provider, true);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);

                provider.RaiseInteractionRatioChangedEvent(0.5f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Interacting, c_minimumIndicatorOpacity);

                provider.RaiseInteractionRatioChangedEvent(0.9f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Pending, c_fullIndicatorOpacity);

                provider.RaiseInteractionRatioChangedEvent(0.6f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Interacting, c_minimumIndicatorOpacity);

                provider.RaiseInteractionRatioChangedEvent(0.0f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);

                // From zero straight past the execution ratio (0.8) goes directly to Pending.
                provider.RaiseInteractionRatioChangedEvent(0.9f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Pending, c_fullIndicatorOpacity);
                Verify.AreEqual(0, deferrals.Count, "No refresh is requested while the user is still interacting");
                Verify.AreEqual(0, provider.OnRefreshStartedCalls);

                // Releasing in Pending requests a refresh.
                SetInteracting(provider, false);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Refreshing, c_fullIndicatorOpacity);
                Verify.AreEqual(1, deferrals.Count);
                Verify.AreEqual(1, provider.OnRefreshStartedCalls);

                // A refresh in progress is not interrupted by ratio changes or another release.
                provider.RaiseInteractionRatioChangedEvent(0.3f);
                SetInteracting(provider, true);
                SetInteracting(provider, false);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Refreshing, c_fullIndicatorOpacity);
                Verify.AreEqual(1, deferrals.Count, "Release while Refreshing must not request another refresh");

                deferrals[0].Complete();
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);
                Verify.AreEqual(1, provider.OnRefreshCompletedCalls);

                // Not interacting: a positive ratio peeks and zero returns to Idle.
                provider.RaiseInteractionRatioChangedEvent(0.2f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Peeking, c_fullIndicatorOpacity);
                provider.RaiseInteractionRatioChangedEvent(0.0f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);

                // Releasing while Interacting returns to Idle without a refresh.
                SetInteracting(provider, true);
                provider.RaiseInteractionRatioChangedEvent(0.5f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Interacting, c_minimumIndicatorOpacity);
                SetInteracting(provider, false);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);

                // Interacting again from a non-zero ratio peeks instead of starting a new pull.
                SetInteracting(provider, true);
                provider.RaiseInteractionRatioChangedEvent(0.4f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Peeking, c_fullIndicatorOpacity);
                SetInteracting(provider, false);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);

                Verify.AreEqual(1, deferrals.Count);
                Verify.AreEqual(1, provider.OnRefreshStartedCalls);
                Verify.AreEqual(1, provider.OnRefreshCompletedCalls);
                VerifyTransitions(new string[] {
                    "Idle->Interacting", "Interacting->Pending", "Pending->Interacting", "Interacting->Idle",
                    "Idle->Pending", "Pending->Refreshing", "Refreshing->Idle",
                    "Idle->Peeking", "Peeking->Idle",
                    "Idle->Interacting", "Interacting->Idle",
                    "Idle->Peeking", "Peeking->Idle" }, transitions);
            });
        }

        // Scenario: with the default ExecutionRatio 0.8 (exactly 0.8f), send the ratios 0.8 (from Idle), 0.8 again,
        //           0.81, 0.8, 0, then 0.81 from zero, 0 and 0.
        // Expected: exactly 0.8 is Interacting every time (from Idle, from Interacting and when falling back from
        //           Pending); only 0.81 is Pending; transitions exactly [Idle->Interacting, Interacting->Pending,
        //           Pending->Interacting, Interacting->Idle, Idle->Pending, Pending->Interacting, Interacting->Idle].
        // A failure means: a pull arms the refresh at the threshold instead of only beyond it (an off-by-one in the
        //                  comparison).
        [TestMethod]
        [TestProperty("Description", "The ExecutionRatio is an exclusive threshold: a ratio exactly equal to it is Interacting (from zero, from Interacting and when falling back from Pending); only a larger ratio is Pending.")]
        public void ExecutionRatioIsAnExclusivePendingThreshold()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Content = refreshVisualizer;
                Content.UpdateLayout();

                RefreshInfoProviderImpl provider = new RefreshInfoProviderImpl(ElementCompositionPreview.GetElementVisual(refreshVisualizer).Compositor);
                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = provider;
                Verify.AreEqual((double)0.8f, provider.ExecutionRatio, "Precondition: the double's ExecutionRatio is exactly 0.8f");
                List<string> transitions = RecordTransitions(refreshVisualizer);
                Visual indicator = ElementCompositionPreview.GetElementVisual(refreshVisualizer.Content);
                SetInteracting(provider, true);

                // From Idle at zero, exactly the execution ratio is not past it.
                provider.RaiseInteractionRatioChangedEvent(0.8f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Interacting, c_minimumIndicatorOpacity);
                // Interacting stays Interacting at exactly the execution ratio.
                provider.RaiseInteractionRatioChangedEvent(0.8f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Interacting, c_minimumIndicatorOpacity);
                // Just above it is Pending.
                provider.RaiseInteractionRatioChangedEvent(0.81f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Pending, c_fullIndicatorOpacity);
                // Back to exactly the execution ratio leaves Pending.
                provider.RaiseInteractionRatioChangedEvent(0.8f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Interacting, c_minimumIndicatorOpacity);
                provider.RaiseInteractionRatioChangedEvent(0.0f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);
                // From zero, just above the execution ratio goes straight to Pending.
                provider.RaiseInteractionRatioChangedEvent(0.81f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Pending, c_fullIndicatorOpacity);
                provider.RaiseInteractionRatioChangedEvent(0.0f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Interacting, c_minimumIndicatorOpacity);
                provider.RaiseInteractionRatioChangedEvent(0.0f);
                VerifyState(refreshVisualizer, indicator, RefreshVisualizerState.Idle, c_minimumIndicatorOpacity);

                VerifyTransitions(new string[] {
                    "Idle->Interacting", "Interacting->Pending", "Pending->Interacting", "Interacting->Idle",
                    "Idle->Pending", "Pending->Interacting", "Interacting->Idle" }, transitions);
            });
        }

        // Scenario: assign provider A and then provider B (ExecutionRatio 0.5); drive A (interacting, 0.6), then B
        //           (interacting, 0.6 then 0.45); clear InfoProvider, release B and request a refresh.
        // Expected: A's events change nothing (Idle, no transitions); with B, 0.6 is Pending and 0.45 Interacting (B's
        //           0.5 is used, not 0.8); after clearing, B's release no longer changes the state; RequestRefresh
        //           still completes and neither provider is told; transitions exactly [Idle->Pending,
        //           Pending->Interacting, Interacting->Refreshing, Refreshing->Idle].
        // A failure means: the visualizer keeps listening to a provider it no longer uses, or uses a stale execution
        //                  ratio.
        [TestMethod]
        [TestProperty("Description", "The visualizer uses the ExecutionRatio of its current InfoProvider and ignores events from a provider it no longer uses.")]
        public void ReplacedInfoProviderIsIgnoredAndNewExecutionRatioIsUsed()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Content = refreshVisualizer;
                Content.UpdateLayout();
                Compositor compositor = ElementCompositionPreview.GetElementVisual(refreshVisualizer).Compositor;

                RefreshInfoProviderImpl first = new RefreshInfoProviderImpl(compositor);
                RefreshInfoProviderImpl second = new RefreshInfoProviderImpl(compositor);
                second.ExecutionRatio = 0.5;

                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = first;
                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = second;
                Verify.IsTrue(object.ReferenceEquals(second, ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider));
                List<string> transitions = RecordTransitions(refreshVisualizer);

                SetInteracting(first, true);
                first.RaiseInteractionRatioChangedEvent(0.6f);
                Verify.AreEqual(RefreshVisualizerState.Idle, refreshVisualizer.State, "Events from the replaced provider are ignored");
                Verify.AreEqual(0, transitions.Count);

                // 0.6 is below the default ratio (0.8) but above the new provider's ratio (0.5): Pending, not Interacting.
                SetInteracting(second, true);
                second.RaiseInteractionRatioChangedEvent(0.6f);
                Verify.AreEqual(RefreshVisualizerState.Pending, refreshVisualizer.State);
                second.RaiseInteractionRatioChangedEvent(0.45f);
                Verify.AreEqual(RefreshVisualizerState.Interacting, refreshVisualizer.State);

                ((IRefreshVisualizerPrivate)refreshVisualizer).InfoProvider = null;
                SetInteracting(second, false);
                Verify.AreEqual(RefreshVisualizerState.Interacting, refreshVisualizer.State, "A cleared provider no longer drives the state");
                refreshVisualizer.RequestRefresh();
                Verify.AreEqual(RefreshVisualizerState.Idle, refreshVisualizer.State);
                Verify.AreEqual(0, first.OnRefreshStartedCalls);
                Verify.AreEqual(0, second.OnRefreshStartedCalls);
                VerifyTransitions(new string[] { "Idle->Pending", "Pending->Interacting", "Interacting->Refreshing", "Refreshing->Idle" }, transitions);
            });
        }

        // Scenario: load a visualizer and set combinations of Orientation and the internal pull direction.
        // Expected: indicator RotationAngle: Auto gives 0 for TopToBottom and BottomToTop, -pi/2 for LeftToRight and
        //           +pi/2 for RightToLeft; Normal with LeftToRight gives 0; Rotate270DegreesCounterclockwise gives
        //           -pi/2 and Rotate90DegreesCounterclockwise +pi/2 (TopToBottom); changing only Orientation to Normal
        //           gives 0; the indicator's CenterPoint is half its rendered width and height.
        // A failure means: the refresh icon points the wrong way for the pull direction or Orientation, or rotates
        //                  around the wrong point.
        [TestMethod]
        [TestProperty("Description", "Orientation and the internal pull direction determine the idle rotation of the refresh indicator.")]
        public void OrientationAndPullDirectionDetermineIndicatorRotation()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Content = refreshVisualizer;
                Content.UpdateLayout();
                IRefreshVisualizerPrivate visualizerPrivate = (IRefreshVisualizerPrivate)refreshVisualizer;
                Visual indicator = ElementCompositionPreview.GetElementVisual(refreshVisualizer.Content);

                VerifyRotation(refreshVisualizer, indicator, RefreshVisualizerOrientation.Auto, RefreshPullDirection.TopToBottom, 0.0f);
                VerifyRotation(refreshVisualizer, indicator, RefreshVisualizerOrientation.Auto, RefreshPullDirection.LeftToRight, -c_halfPi);
                VerifyRotation(refreshVisualizer, indicator, RefreshVisualizerOrientation.Auto, RefreshPullDirection.RightToLeft, c_halfPi);
                VerifyRotation(refreshVisualizer, indicator, RefreshVisualizerOrientation.Auto, RefreshPullDirection.BottomToTop, 0.0f);
                VerifyRotation(refreshVisualizer, indicator, RefreshVisualizerOrientation.Normal, RefreshPullDirection.LeftToRight, 0.0f);
                VerifyRotation(refreshVisualizer, indicator, RefreshVisualizerOrientation.Rotate270DegreesCounterclockwise, RefreshPullDirection.TopToBottom, -c_halfPi);
                VerifyRotation(refreshVisualizer, indicator, RefreshVisualizerOrientation.Rotate90DegreesCounterclockwise, RefreshPullDirection.TopToBottom, c_halfPi);

                // Changing only the Orientation property updates the rotation too.
                refreshVisualizer.Orientation = RefreshVisualizerOrientation.Normal;
                VerifyClose(0.0f, indicator.RotationAngle, "RotationAngle after Orientation=Normal");

                // The default (square, 30x30 DIP) indicator rotates around the center of its rendered size
                // (derived from RenderSize, which is rounded to physical pixels at non-100% scale).
                VerifyCenterPoint(indicator, refreshVisualizer.Content, "Default indicator");
            });
        }

        // Scenario: load a visualizer (height 100, stretched width) with a provider (ExecutionRatio 0.8) and a handler
        //           that takes a deferral; for each pull direction request a refresh and complete it; finally clear the
        //           provider and refresh TopToBottom.
        // Expected: the indicator Translation is zero while idle; while refreshing it is (1 - 0.8) / 2 = 0.1 of the
        //           visualizer size along the pull axis (+0.1 x height TopToBottom, -0.1 x height BottomToTop, +0.1 x
        //           width LeftToRight, -0.1 x width RightToLeft); it returns to zero after the refresh; without a
        //           provider the full height is used.
        // A failure means: the indicator is placed wrongly while a refresh runs, or is not reset afterwards.
        [TestMethod]
        [TestProperty("Description", "While refreshing, the indicator is translated along the pull axis by (1 - ExecutionRatio) / 2 of the visualizer size (negative for far pull directions) and reset when idle.")]
        public void RefreshingTranslationFollowsExecutionRatioAndPullDirection()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                Content = refreshVisualizer;
                Content.UpdateLayout();
                Grid root = (Grid)VisualTreeHelper.GetChild(refreshVisualizer, 0);
                float height = (float)root.ActualHeight;
                float width = (float)root.ActualWidth;
                Verify.AreEqual(100.0f, height);
                Verify.IsTrue(width > 100.0f, "Visualizer stretches horizontally");

                IRefreshVisualizerPrivate visualizerPrivate = (IRefreshVisualizerPrivate)refreshVisualizer;
                RefreshInfoProviderImpl provider = new RefreshInfoProviderImpl(ElementCompositionPreview.GetElementVisual(refreshVisualizer).Compositor);
                visualizerPrivate.InfoProvider = provider;
                List<Deferral> deferrals = new List<Deferral>();
                refreshVisualizer.RefreshRequested += (RefreshVisualizer sender, RefreshRequestedEventArgs args) => { deferrals.Add(args.GetDeferral()); };
                Visual indicator = ElementCompositionPreview.GetElementVisual(refreshVisualizer.Content);

                // ExecutionRatio 0.8 => (1 - 0.8) * 0.5 = 0.1 of the visualizer size.
                VerifyRefreshingTranslation(refreshVisualizer, indicator, deferrals, RefreshPullDirection.TopToBottom, new Vector3(0.0f, 0.1f * height, 0.0f));
                VerifyRefreshingTranslation(refreshVisualizer, indicator, deferrals, RefreshPullDirection.BottomToTop, new Vector3(0.0f, -0.1f * height, 0.0f));
                VerifyRefreshingTranslation(refreshVisualizer, indicator, deferrals, RefreshPullDirection.LeftToRight, new Vector3(0.1f * width, 0.0f, 0.0f));
                VerifyRefreshingTranslation(refreshVisualizer, indicator, deferrals, RefreshPullDirection.RightToLeft, new Vector3(-0.1f * width, 0.0f, 0.0f));

                // Without an InfoProvider the full size is used.
                visualizerPrivate.InfoProvider = null;
                VerifyRefreshingTranslation(refreshVisualizer, indicator, deferrals, RefreshPullDirection.TopToBottom, new Vector3(0.0f, height, 0.0f));
            });
        }

        // Scenario: use a 40x20 Grid as Content, load the visualizer and set the pull direction to TopToBottom.
        // Expected: (correct behaviour) the indicator CenterPoint is (rendered width / 2, rendered height / 2, 0).
        // A failure means: the bug is still present: a non-square indicator rotates and scales around the wrong point.
        // Ignored: reproduces BUG-PTR-2 (RefreshVisualizer sets the CenterPoint to (Height/2, Width/2), swapping the
        //          axes); remove the Ignore once fixed.
        [TestMethod]
        [TestProperty("Ignore", "True")] // BUG-PTR-2: RefreshVisualizer sets the indicator CenterPoint to (Height/2, Width/2); non-square content pivots around the wrong point.
        [TestProperty("Description", "A non-square indicator should rotate and scale around its center (Width/2, Height/2).")]
        public void NonSquareIndicatorRotatesAroundItsCenter()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshVisualizer refreshVisualizer = new RefreshVisualizer();
                refreshVisualizer.Content = new Grid() { Width = 40, Height = 20 };
                Content = refreshVisualizer;
                Content.UpdateLayout();
                ((IRefreshVisualizerPrivate)refreshVisualizer).SetInternalPullDirection(RefreshPullDirection.TopToBottom);
                Visual indicator = ElementCompositionPreview.GetElementVisual(refreshVisualizer.Content);
                Size size = ((FrameworkElement)refreshVisualizer.Content).RenderSize;
                Verify.IsGreaterThan(Math.Abs(size.Width - size.Height), 1.0, "Precondition: the indicator is not square, so swapped axes are detectable");
                VerifyCenterPoint(indicator, refreshVisualizer.Content, "Non-square indicator");
            });
        }

        private static void VerifyClose(float expected, float actual, string context)
        {
            Verify.IsLessThan(Math.Abs(expected - actual), c_tolerance, context + ": expected " + expected + ", actual " + actual);
        }

        private static void VerifyCenterPoint(Visual indicator, UIElement content, string context)
        {
            Size size = ((FrameworkElement)content).RenderSize;
            Verify.IsTrue(size.Width > 0 && size.Height > 0, context + ": indicator has been laid out");
            Vector3 centerPoint = indicator.CenterPoint;
            VerifyClose((float)(size.Width / 2), centerPoint.X, context + " CenterPoint.X (half the rendered width)");
            VerifyClose((float)(size.Height / 2), centerPoint.Y, context + " CenterPoint.Y (half the rendered height)");
            VerifyClose(0.0f, centerPoint.Z, context + " CenterPoint.Z");
        }

        private static void SetInteracting(RefreshInfoProviderImpl provider, bool isInteracting)
        {
            provider.SetCustomIsInteractingForRefresh(isInteracting);
            provider.RaiseIsInteractionForRefreshChangedEvent();
        }

        private static List<string> RecordTransitions(RefreshVisualizer refreshVisualizer)
        {
            List<string> transitions = new List<string>();
            refreshVisualizer.RefreshStateChanged += (RefreshVisualizer sender, RefreshStateChangedEventArgs args) =>
            {
                transitions.Add(args.OldState + "->" + args.NewState);
            };
            return transitions;
        }

        private static void VerifyTransitions(string[] expected, List<string> actual)
        {
            Verify.AreEqual(string.Join(", ", expected), string.Join(", ", actual), "RefreshStateChanged sequence");
        }

        private static void VerifyState(RefreshVisualizer refreshVisualizer, Visual indicator, RefreshVisualizerState expectedState, float expectedOpacity)
        {
            Verify.AreEqual(expectedState, refreshVisualizer.State);
            VerifyClose(expectedOpacity, indicator.Opacity, "Indicator opacity in state " + expectedState);
        }

        private static void VerifyRotation(RefreshVisualizer refreshVisualizer, Visual indicator, RefreshVisualizerOrientation orientation, RefreshPullDirection pullDirection, float expectedAngle)
        {
            refreshVisualizer.Orientation = orientation;
            ((IRefreshVisualizerPrivate)refreshVisualizer).SetInternalPullDirection(pullDirection);
            VerifyClose(expectedAngle, indicator.RotationAngle, "RotationAngle for " + orientation + " / " + pullDirection);
        }

        private static void VerifyRefreshingTranslation(RefreshVisualizer refreshVisualizer, Visual indicator, List<Deferral> deferrals, RefreshPullDirection pullDirection, Vector3 expected)
        {
            ((IRefreshVisualizerPrivate)refreshVisualizer).SetInternalPullDirection(pullDirection);
            VerifyTranslation(indicator, Vector3.Zero, "Idle " + pullDirection);

            int deferralCount = deferrals.Count;
            refreshVisualizer.RequestRefresh();
            Verify.AreEqual(RefreshVisualizerState.Refreshing, refreshVisualizer.State);
            Verify.AreEqual(deferralCount + 1, deferrals.Count);
            VerifyTranslation(indicator, expected, "Refreshing " + pullDirection);

            deferrals[deferrals.Count - 1].Complete();
            Verify.AreEqual(RefreshVisualizerState.Idle, refreshVisualizer.State);
            VerifyTranslation(indicator, Vector3.Zero, "Idle after refresh " + pullDirection);
        }

        private static void VerifyTranslation(Visual indicator, Vector3 expected, string context)
        {
            Vector3 actual;
            Verify.AreEqual(CompositionGetValueStatus.Succeeded, indicator.Properties.TryGetVector3("Translation", out actual), context);
            VerifyClose(expected.X, actual.X, context + " X");
            VerifyClose(expected.Y, actual.Y, context + " Y");
            VerifyClose(expected.Z, actual.Z, context + " Z");
        }
    }

    // An IRefreshInfoProvider that counts the handlers added to and removed from its events.
    partial class CountingRefreshInfoProvider : IRefreshInfoProvider
    {
        private readonly CompositionPropertySet compositionProperties;
        private TypedEventHandler<IRefreshInfoProvider, RefreshInteractionRatioChangedEventArgs> interactionRatioChanged;
        private TypedEventHandler<IRefreshInfoProvider, object> isInteractingForRefreshChanged;

        public int InteractionRatioChangedSubscribers;
        public int IsInteractingForRefreshChangedSubscribers;
        public int TotalRemoves;

        public CountingRefreshInfoProvider(Compositor compositor)
        {
            compositionProperties = compositor.CreatePropertySet();
            compositionProperties.InsertScalar(InteractionRatioCompositionProperty, 0.0f);
        }

        public double ExecutionRatio { get { return 0.8; } }
        public CompositionPropertySet CompositionProperties { get { return compositionProperties; } }
        public string InteractionRatioCompositionProperty { get { return "InteractionRatio"; } }
        public bool IsInteractingForRefresh { get { return false; } }

        public event TypedEventHandler<IRefreshInfoProvider, RefreshInteractionRatioChangedEventArgs> InteractionRatioChanged
        {
            add { interactionRatioChanged += value; InteractionRatioChangedSubscribers++; }
            remove { interactionRatioChanged -= value; InteractionRatioChangedSubscribers--; TotalRemoves++; }
        }

        public event TypedEventHandler<IRefreshInfoProvider, object> IsInteractingForRefreshChanged
        {
            add { isInteractingForRefreshChanged += value; IsInteractingForRefreshChangedSubscribers++; }
            remove { isInteractingForRefreshChanged -= value; IsInteractingForRefreshChangedSubscribers--; TotalRemoves++; }
        }

#pragma warning disable CS0067 // Required by IRefreshInfoProvider; this provider never refreshes.
        public event TypedEventHandler<IRefreshInfoProvider, object> RefreshCompleted;
        public event TypedEventHandler<IRefreshInfoProvider, object> RefreshStarted;
#pragma warning restore CS0067

        public void OnRefreshCompleted() { }
        public void OnRefreshStarted() { }
    }

    partial class RefreshInfoProviderImpl : IRefreshInfoProvider
    {
        private double executionRatio;
        private CompositionPropertySet compositionProperties;
        private bool isInteractingForRefresh;

        public int OnRefreshCompletedCalls;
        public int OnRefreshStartedCalls;

        public RefreshInfoProviderImpl(Compositor compositor)
        {
            executionRatio = 0.8f;
            compositionProperties = compositor.CreatePropertySet();
            compositionProperties.InsertScalar(InteractionRatioCompositionProperty, 0.0f);
            isInteractingForRefresh = false;
            OnRefreshCompletedCalls = 0;
            OnRefreshStartedCalls = 0;
        }

        public void SetCustomInteractionRatio(float value)
        {
            compositionProperties.InsertScalar(InteractionRatioCompositionProperty, value);
        }

        public void SetCustomIsInteractingForRefresh(bool value)
        {
            isInteractingForRefresh = value;
        }

        public void RaiseInteractionRatioChangedEvent(float interactionRatio)
        {
            if (this.InteractionRatioChanged != null)
            {
                this.InteractionRatioChanged.Invoke(this, PullToRefreshHelperTestApi.CreateRefreshInteractionRatioChangedEventArgsInstance(interactionRatio));
            }
        }

        public void RaiseIsInteractionForRefreshChangedEvent()
        {
            if (this.IsInteractingForRefreshChanged != null)
            {
                this.IsInteractingForRefreshChanged.Invoke(this, new EventArgs());
            }
        }

        public double ExecutionRatio
        {
            get
            {
                return executionRatio;
            }

            set
            {
                executionRatio = value;
            }
        }

        public CompositionPropertySet CompositionProperties
        {
            get
            {
                return compositionProperties;
            }
        }

        public string InteractionRatioCompositionProperty
        {
            get
            {
                return "InteractionRatio";
            }
        }

        public bool IsInteractingForRefresh
        {
            get
            {
                return isInteractingForRefresh;
            }
        }

        public event TypedEventHandler<IRefreshInfoProvider, RefreshInteractionRatioChangedEventArgs> InteractionRatioChanged;
        public event TypedEventHandler<IRefreshInfoProvider, object> IsInteractingForRefreshChanged;
        public event TypedEventHandler<IRefreshInfoProvider, object> RefreshCompleted;
        public event TypedEventHandler<IRefreshInfoProvider, object> RefreshStarted;

        public void OnRefreshCompleted()
        {
            OnRefreshCompletedCalls++;

            if (this.RefreshCompleted != null)
            {
                RefreshCompleted.Invoke(this, new EventArgs());
            }
        }

        public void OnRefreshStarted()
        {
            OnRefreshStartedCalls++;
            if (this.RefreshStarted != null)
            {
                RefreshStarted.Invoke(this, new EventArgs());
            }
        }
    }
}
