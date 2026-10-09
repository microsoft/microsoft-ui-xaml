// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using System.Threading.Tasks;
using System.Threading;
using Microsoft.UI.Xaml.Media;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using System.Xml.Linq;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class ItemCollectionTransitionProviderTests : ApiTestBase
    {
        
        [TestMethod]
        [TestProperty("Ignore", "True")] // Task 35797826: ElementAnimatorTests.ValidateElementAnimator test disabled
        public void ValidateItemCollectionTransitionProvider()
        {
            ItemsRepeater repeater = null;
            ItemCollectionTransitionProviderDerived transitionProvider = null;
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => string.Format("Item #{0}", i)));
            var renderingEvent = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var elementFactory = new RecyclingElementFactory();
                elementFactory.RecyclePool = new RecyclePool();
                elementFactory.Templates["Item"] = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> 
                          <TextBlock Text='{Binding}' Height='50' />
                      </DataTemplate>");

                CompositionTarget.Rendering += (sender, args) =>
                {
                    renderingEvent.Set();
                };

                repeater = new ItemsRepeater()
                {
                    ItemsSource = data,
                    ItemTemplate = elementFactory,
                };

                Content = new ItemsRepeaterScrollHost()
                {
                    Width = 400,
                    Height = 800,
                    ScrollViewer = new ScrollViewer
                    {
                        Content = repeater
                    }
                };
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(renderingEvent.WaitOne(), "Waiting for rendering event");

            List<CallInfo> addCalls = new();
            List<CallInfo> removeCalls = new();
            List<CallInfo> moveCalls = new();
            RunOnUIThread.Execute(() =>
            {
                transitionProvider = new ItemCollectionTransitionProviderDerived()
                {
                    ShouldAnimateFunc = (ItemCollectionTransition transition) => true,
                    StartTransitionsFunc = (IList<ItemCollectionTransition> transitions) =>
                    {
                        foreach (var transition in transitions)
                        {
                            var progress = transition.Start();

                            switch (transition.Operation)
                            {
                                case ItemCollectionTransitionOperation.Add:
                                    addCalls.Add(new CallInfo(repeater.GetElementIndex(progress.Element), transition));
                                    break;
                                case ItemCollectionTransitionOperation.Remove:
                                    removeCalls.Add(new CallInfo(repeater.GetElementIndex(progress.Element), transition));
                                    break;
                                case ItemCollectionTransitionOperation.Move:
                                    moveCalls.Add(new CallInfo(repeater.GetElementIndex(progress.Element), transition));
                                    break;
                            }

                            progress.Complete();
                        }
                    }
                };
                repeater.ItemTransitionProvider = transitionProvider;

                renderingEvent.Reset();
                data.Insert(0, "new item");
                data.RemoveAt(2);
            });

            Verify.IsTrue(renderingEvent.WaitOne(), "Waiting for rendering event");
            IdleSynchronizer.Wait();

            Verify.AreEqual(1, addCalls.Count);
            var call = addCalls[0];
            Verify.AreEqual(0, call.Index);
            Verify.AreEqual(ItemCollectionTransitionTriggers.CollectionChangeAdd, call.Transition.Triggers);

            Verify.AreEqual(1, removeCalls.Count);
            call = removeCalls[0];
            Verify.AreEqual(-1, call.Index); // not in the repeater anymore
            Verify.AreEqual(ItemCollectionTransitionTriggers.CollectionChangeRemove, call.Transition.Triggers);

            Verify.AreEqual(1, moveCalls.Count);
            call = moveCalls[0];
            Verify.AreEqual(1, call.Index);
            Verify.AreEqual(ItemCollectionTransitionTriggers.CollectionChangeAdd | ItemCollectionTransitionTriggers.CollectionChangeRemove, call.Transition.Triggers);
            Verify.AreEqual(0, call.Transition.OldBounds.Y);
            Verify.AreEqual(50, call.Transition.NewBounds.Y);

            addCalls.Clear();
            removeCalls.Clear();
            moveCalls.Clear();

            // Hookup just for show animations and validate.
            RunOnUIThread.Execute(() =>
            {
                transitionProvider.ShouldAnimateFunc = (ItemCollectionTransition transition) => transition.Operation == ItemCollectionTransitionOperation.Add;

                renderingEvent.Reset();
                data.Insert(0, "new item");
                data.RemoveAt(2);
            });

            Verify.IsTrue(renderingEvent.WaitOne(), "Waiting for rendering event");
            IdleSynchronizer.Wait();

            Verify.AreEqual(1, addCalls.Count);
            call = addCalls[0];
            Verify.AreEqual(0, call.Index);
            Verify.AreEqual(ItemCollectionTransitionTriggers.CollectionChangeAdd, call.Transition.Triggers);

            Verify.AreEqual(0, removeCalls.Count);
            Verify.AreEqual(0, moveCalls.Count);
        }

        // Scenario: give an ItemsRepeater a transition provider that declines to animate, insert an item, then start and
        //           complete the reported transition by hand, and complete it again after unsubscribing.
        // Expected: TransitionCompleted reports an Add transition for the new element without it being started; Start()
        //           marks it started and returns a progress object for the same transition and element whose Complete()
        //           raises TransitionCompleted again; a removed handler is not called; every transition object reports its
        //           runtime class name.
        // A failure means: apps animating item additions could miss completion notifications and leave items stuck.
        [TestMethod]
        [TestProperty("Description", "Verifies the ItemCollectionTransition lifecycle: TransitionCompleted, HasStarted, Start/Complete and handler removal.")]
        public void VerifyTransitionLifecycleWithoutAnimation()
        {
            ItemsRepeater repeater = null;
            ItemCollectionTransitionProviderDerived provider = null;
            var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
            var completed = new List<ItemCollectionTransitionCompletedEventArgs>();
            var completedEvent = new AutoResetEvent(false);
            UIElement insertedElement = null;
            TypedEventHandler<ItemCollectionTransitionProvider, ItemCollectionTransitionCompletedEventArgs> onCompleted = (sender, args) =>
            {
                completed.Add(args);
                if (insertedElement != null && args.Element == insertedElement)
                {
                    completedEvent.Set();
                }
            };

            RunOnUIThread.Execute(() =>
            {
                provider = new ItemCollectionTransitionProviderDerived() { ShouldAnimateFunc = (transition) => false };
                provider.TransitionCompleted += onCompleted;
                repeater = new ItemsRepeater() {
                    ItemsSource = data,
                    ItemTransitionProvider = provider,
                    ItemTemplate = (DataTemplate)XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding}' Height='20'/></DataTemplate>"),
                };
                Content = repeater;
                Content.UpdateLayout();
                Verify.AreSame(provider, repeater.ItemTransitionProvider);
                completed.Clear();
                data.Insert(0, "New");
                Content.UpdateLayout();
                insertedElement = repeater.TryGetElement(0);
                Verify.IsNotNull(insertedElement);
            });

            // Bounded wait: if TransitionCompleted never arrives (for example because the provider never schedules its batch),
            // log it and fail with an explicit message instead of continuing.
            bool insertCompleted = completedEvent.WaitOne(DefaultWaitTimeInMS);
            Log.Comment("TransitionCompleted for the inserted element arrived: " + insertCompleted);
            Verify.IsTrue(insertCompleted, "TransitionCompleted was not raised for the inserted element within " + DefaultWaitTimeInMS + "ms.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Transitions queued for the initially loaded items may complete in the same frame, so look for the one
                // reported for the inserted element.
                var add = completed.FirstOrDefault(a => a.Transition.Operation == ItemCollectionTransitionOperation.Add && a.Element == insertedElement);
                Verify.IsNotNull(add, "An Add transition completed for the inserted element.");
                var transition = add.Transition;
                var element = add.Element;
                Verify.IsTrue((transition.Triggers & ItemCollectionTransitionTriggers.CollectionChangeAdd) != 0);
                Verify.IsFalse(transition.HasStarted, "A transition that was not animated was never started.");
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.ItemCollectionTransition", GetNativeRuntimeClassName(transition));
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.ItemCollectionTransitionCompletedEventArgs", GetNativeRuntimeClassName(add));

                var progress = transition.Start();
                Verify.IsTrue(transition.HasStarted);
                Verify.AreSame(transition, progress.Transition);
                Verify.AreSame(element, progress.Element);
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProgress", GetNativeRuntimeClassName(progress));
                int countBeforeComplete = completed.Count;
                progress.Complete();
                Verify.AreEqual(countBeforeComplete + 1, completed.Count, "Complete() raises TransitionCompleted synchronously.");
                Verify.AreSame(transition, completed.Last().Transition);

                provider.TransitionCompleted -= onCompleted;
                int countAfterRemoval = completed.Count;
                progress.Complete();
                Verify.AreEqual(countAfterRemoval, completed.Count, "A removed TransitionCompleted handler is not called.");
                Content = null;
            });
        }

        // Scenario: take a real Add transition reported by the transition provider of an ItemsRepeater and queue it on a
        //           second, unattached provider with QueueTransition - first declining to animate it, then accepting.
        // Expected: nothing is raised synchronously; on the next frame the second provider raises TransitionCompleted for
        //           that same transition and element; StartTransitions receives it only when the provider wants to animate
        //           it and system animations are on (it is never asked otherwise).
        // A failure means: custom transition providers that queue their own transitions never see them start or complete.
        [TestMethod]
        [TestProperty("Description", "Verifies ItemCollectionTransitionProvider.QueueTransition schedules StartTransitions and TransitionCompleted on the next frame.")]
        public void VerifyQueueTransitionSchedulesTransitionsOnNextFrame()
        {
            ItemsRepeater repeater = null;
            ItemCollectionTransitionProviderDerived repeaterProvider = null;
            ItemCollectionTransitionProviderDerived queueingProvider = null;
            var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
            var repeaterCompletedEvent = new AutoResetEvent(false);
            var queuedCompletedEvent = new AutoResetEvent(false);
            var queuedCompleted = new List<ItemCollectionTransitionCompletedEventArgs>();
            var shouldAnimateCalls = new List<ItemCollectionTransition>();
            var startedTransitions = new List<ItemCollectionTransition>();
            UIElement insertedElement = null;
            ItemCollectionTransition transition = null;
            bool animationsEnabled = false;

            RunOnUIThread.Execute(() =>
            {
                animationsEnabled = new global::Windows.UI.ViewManagement.UISettings().AnimationsEnabled;
                Log.Comment("System animations enabled: " + animationsEnabled);
                repeaterProvider = new ItemCollectionTransitionProviderDerived() { ShouldAnimateFunc = (t) => false };
                repeaterProvider.TransitionCompleted += (sender, args) =>
                {
                    if (insertedElement != null && args.Element == insertedElement && transition == null)
                    {
                        transition = args.Transition;
                        repeaterCompletedEvent.Set();
                    }
                };
                repeater = new ItemsRepeater() {
                    ItemsSource = data,
                    ItemTransitionProvider = repeaterProvider,
                    ItemTemplate = (DataTemplate)XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding}' Height='20'/></DataTemplate>"),
                };
                Content = repeater;
                Content.UpdateLayout();
                data.Insert(0, "New");
                Content.UpdateLayout();
                insertedElement = repeater.TryGetElement(0);
                Verify.IsNotNull(insertedElement);
            });

            bool repeaterCompleted = repeaterCompletedEvent.WaitOne(DefaultWaitTimeInMS);
            Log.Comment("The repeater's Add transition completed: " + repeaterCompleted);
            Verify.IsTrue(repeaterCompleted, "The repeater's Add transition did not complete within " + DefaultWaitTimeInMS + "ms.");
            IdleSynchronizer.Wait();

            for (int pass = 0; pass < 2; pass++)
            {
                bool wantsAnimation = pass == 1;
                RunOnUIThread.Execute(() =>
                {
                    if (queueingProvider == null)
                    {
                        queueingProvider = new ItemCollectionTransitionProviderDerived();
                        queueingProvider.TransitionCompleted += (sender, args) =>
                        {
                            queuedCompleted.Add(args);
                            queuedCompletedEvent.Set();
                        };
                    }
                    queueingProvider.ShouldAnimateFunc = (t) => { shouldAnimateCalls.Add(t); return wantsAnimation; };
                    // The transitions are deliberately not started, so the provider completes them itself.
                    queueingProvider.StartTransitionsFunc = (transitions) => startedTransitions.AddRange(transitions);
                    queuedCompleted.Clear();
                    shouldAnimateCalls.Clear();
                    startedTransitions.Clear();

                    queueingProvider.QueueTransition(transition);

                    Verify.AreEqual(0, queuedCompleted.Count, "TransitionCompleted is not raised synchronously by QueueTransition.");
                    Verify.AreEqual(0, startedTransitions.Count, "StartTransitions is not called synchronously by QueueTransition.");
                });

                bool queuedTransitionCompleted = queuedCompletedEvent.WaitOne(DefaultWaitTimeInMS);
                Log.Comment("Queued transition completed (pass " + pass + "): " + queuedTransitionCompleted);
                Verify.IsTrue(queuedTransitionCompleted, "QueueTransition did not lead to TransitionCompleted within " + DefaultWaitTimeInMS + "ms (pass " + pass + ").");
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(1, queuedCompleted.Count, "One TransitionCompleted for the queued transition.");
                    Verify.AreSame(transition, queuedCompleted[0].Transition);
                    Verify.AreSame(insertedElement, queuedCompleted[0].Element);
                    Verify.IsFalse(transition.HasStarted, "The queued transition was not started.");
                    // ShouldAnimateCore is only consulted when system animations are on.
                    Verify.AreEqual(animationsEnabled ? 1 : 0, shouldAnimateCalls.Count, "ShouldAnimateCore calls");
                    Verify.IsTrue(shouldAnimateCalls.All(t => t == transition));
                    if (wantsAnimation && animationsEnabled)
                    {
                        Verify.AreEqual(1, startedTransitions.Count, "StartTransitions receives the transition to animate.");
                        Verify.AreSame(transition, startedTransitions[0]);
                    }
                    else
                    {
                        Verify.AreEqual(0, startedTransitions.Count, "StartTransitions is not called when nothing is animated.");
                    }
                });
            }

            RunOnUIThread.Execute(() => { Content = null; });
        }

        private static string GetNativeRuntimeClassName(object instance)
        {
            var objRef = ((global::WinRT.IWinRTObject)instance).NativeObject;
            return global::WinRT.IInspectable.FromAbi(objRef.ThisPtr).GetRuntimeClassName(false);
        }

        struct CallInfo
        {
            public CallInfo(int index, ItemCollectionTransition transition)
            {
                Index = index;
                Transition = transition;
            }

            public int Index { get; set; }
            public ItemCollectionTransition Transition { get; set; }

            public override string ToString()
            {
                return $"Index: {Index} Operation: {Transition.Operation} Triggers: {Transition.Triggers} OldBounds: {Transition.OldBounds} NewBounds {Transition.NewBounds}";
            }
        };
    }
}
