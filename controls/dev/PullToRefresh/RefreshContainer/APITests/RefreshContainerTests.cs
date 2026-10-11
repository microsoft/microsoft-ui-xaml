// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Numerics;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Composition;
using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using Windows.Foundation;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class RefreshContainerTests : ApiTestBase
    {
        // Values mirrored from RefreshContainer.cpp / ScrollViewerIRefreshInfoProviderAdapter.cpp.
        private const double c_defaultPullDimension = 100.0;
        private const double c_containerSize = 300.0;
        private const double c_defaultExecutionRatio = 0.8;
        private const float c_tolerance = 0.001f;

        [TestMethod]
        [TestProperty("Description", "Before its template is applied a RefreshContainer pulls TopToBottom, has no Visualizer, uses a ScrollViewer adapter and ignores RequestRefresh.")]
        public void DefaultsBeforeTemplateAndRequestRefreshIsANoOp()
        {
            RunOnUIThread.Execute(() =>
            {
                RefreshContainer refreshContainer = new RefreshContainer();
                int refreshRequestedCount = 0;
                refreshContainer.RefreshRequested += (RefreshContainer sender, RefreshRequestedEventArgs args) => { refreshRequestedCount++; };

                Verify.AreEqual(RefreshPullDirection.TopToBottom, refreshContainer.PullDirection);
                Verify.IsNull(refreshContainer.Visualizer);
                Verify.IsTrue(((IRefreshContainerPrivate)refreshContainer).RefreshInfoProviderAdapter is ScrollViewerIRefreshInfoProviderAdapter,
                    "The default adapter adapts a ScrollViewer");

                refreshContainer.RequestRefresh();
                Verify.AreEqual(0, refreshRequestedCount, "Without a visualizer there is nothing to refresh");
                Verify.IsNull(refreshContainer.Visualizer);
            });
        }

        [TestMethod]
        [TestProperty("Description", "For each pull direction the default template places a default RefreshVisualizer at the pulled edge, sizes it 100 along the pull axis, rotates its indicator and adapts the content ScrollViewer into the visualizer's InfoProvider.")]
        public void DefaultTemplatePlacesSizesAndAdaptsForEachPullDirection()
        {
            VerifyDefaultTemplate(RefreshPullDirection.TopToBottom, VerticalAlignment.Top, HorizontalAlignment.Stretch, 0.0f);
            VerifyDefaultTemplate(RefreshPullDirection.BottomToTop, VerticalAlignment.Bottom, HorizontalAlignment.Stretch, 0.0f);
            VerifyDefaultTemplate(RefreshPullDirection.LeftToRight, VerticalAlignment.Stretch, HorizontalAlignment.Left, (float)(-Math.PI / 2));
            VerifyDefaultTemplate(RefreshPullDirection.RightToLeft, VerticalAlignment.Stretch, HorizontalAlignment.Right, (float)(Math.PI / 2));
        }

        [TestMethod]
        [TestProperty("Description", "Changing PullDirection after load moves and resizes the default visualizer and re-adapts the ScrollViewer into a new InfoProvider.")]
        public void PullDirectionChangeAfterLoadRepositionsVisualizerAndReadapts()
        {
            RefreshContainer refreshContainer = null;
            IRefreshInfoProvider firstProvider = null;

            RunOnUIThread.Execute(() =>
            {
                refreshContainer = CreateLoadedContainer(RefreshPullDirection.TopToBottom, CreateScrollViewer());
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                firstProvider = ((IRefreshVisualizerPrivate)refreshContainer.Visualizer).InfoProvider;
                Verify.IsNotNull(firstProvider);
                refreshContainer.PullDirection = RefreshPullDirection.LeftToRight;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Grid presenter = GetPresenter(refreshContainer);
                Verify.AreEqual(VerticalAlignment.Stretch, presenter.VerticalAlignment);
                Verify.AreEqual(HorizontalAlignment.Left, presenter.HorizontalAlignment);
                Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.Width);
                Verify.IsTrue(double.IsNaN(refreshContainer.Visualizer.Height), "Height is reset to Auto");
                Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.ActualWidth);
                Verify.AreEqual(c_containerSize, refreshContainer.Visualizer.ActualHeight);

                IRefreshInfoProvider secondProvider = ((IRefreshVisualizerPrivate)refreshContainer.Visualizer).InfoProvider;
                Verify.IsNotNull(secondProvider);
                Verify.IsFalse(object.ReferenceEquals(firstProvider, secondProvider), "The ScrollViewer is adapted again for the new direction");
                Verify.IsLessThan(Math.Abs(c_defaultExecutionRatio - secondProvider.ExecutionRatio), 0.0001, "ExecutionRatio");
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // BUG-PTR-4: changing PullDirection while the default visualizer is exactly 100x100 runs the container's own re-adapt path, which throws ArgumentException (E_INVALIDARG) out of the PullDirection setter.
        [TestProperty("Description", "When the default visualizer is exactly 100x100 a PullDirection change does not resize it (no SizeChanged), so the container itself must replace the default adapter and re-adapt the ScrollViewer for the new direction.")]
        public void PullDirectionChangeReadaptsSquareDefaultVisualizer()
        {
            RefreshContainer refreshContainer = null;
            int sizeChangedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                refreshContainer = new RefreshContainer() {
                    PullDirection = RefreshPullDirection.TopToBottom,
                    Width = c_defaultPullDimension,
                    Height = c_defaultPullDimension,
                    Content = CreateScrollViewer()
                };
                Content = refreshContainer;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                IRefreshContainerPrivate containerPrivate = (IRefreshContainerPrivate)refreshContainer;
                IRefreshVisualizerPrivate visualizerPrivate = (IRefreshVisualizerPrivate)refreshContainer.Visualizer;
                refreshContainer.Visualizer.SizeChanged += (object sender, SizeChangedEventArgs args) => { sizeChangedCount++; };

                foreach (RefreshPullDirection direction in new RefreshPullDirection[] { RefreshPullDirection.LeftToRight, RefreshPullDirection.BottomToTop })
                {
                    Log.Comment("PullDirection -> " + direction);
                    Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.ActualWidth, "Precondition: square default visualizer");
                    Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.ActualHeight, "Precondition: square default visualizer");
                    IRefreshInfoProviderAdapter oldAdapter = containerPrivate.RefreshInfoProviderAdapter;
                    IRefreshInfoProvider oldProvider = visualizerPrivate.InfoProvider;
                    Verify.IsNotNull(oldAdapter);
                    Verify.IsNotNull(oldProvider, "Precondition: the ScrollViewer is adapted");
                    // Identity is observable: without a change the same adapter and provider are returned.
                    Verify.IsTrue(object.ReferenceEquals(oldAdapter, containerPrivate.RefreshInfoProviderAdapter), "Adapter identity is stable");
                    Verify.IsTrue(object.ReferenceEquals(oldProvider, visualizerPrivate.InfoProvider), "Provider identity is stable");

                    refreshContainer.PullDirection = direction;

                    // The PullDirection change itself re-adapts (synchronously, before any layout pass).
                    IRefreshInfoProviderAdapter newAdapter = containerPrivate.RefreshInfoProviderAdapter;
                    Verify.IsTrue(newAdapter is ScrollViewerIRefreshInfoProviderAdapter, "A default ScrollViewer adapter is used");
                    Verify.IsFalse(object.ReferenceEquals(oldAdapter, newAdapter), "A new default adapter is created for " + direction);
                    IRefreshInfoProvider newProvider = visualizerPrivate.InfoProvider;
                    Verify.IsNotNull(newProvider, "The ScrollViewer is adapted again");
                    Verify.IsFalse(object.ReferenceEquals(oldProvider, newProvider), "The visualizer uses the new provider for " + direction);
                    Verify.IsLessThan(Math.Abs(c_defaultExecutionRatio - newProvider.ExecutionRatio), 0.0001, "ExecutionRatio");

                    Content.UpdateLayout();
                    Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.ActualWidth);
                    Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.ActualHeight);
                    Verify.IsTrue(object.ReferenceEquals(newProvider, visualizerPrivate.InfoProvider), "Layout does not adapt again");
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, sizeChangedCount, "The square visualizer is never resized, so SizeChanged cannot have re-adapted it");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Flipping PullDirection along the same axis (LeftToRight -> RightToLeft) keeps the 100x300 default visualizer size, moves it to the opposite edge and rotates its indicator; the square-visualizer re-adapt path is not taken for a non-square visualizer.")]
        public void SameAxisPullDirectionFlipMovesNonSquareVisualizer()
        {
            RefreshContainer refreshContainer = null;
            RunOnUIThread.Execute(() =>
            {
                refreshContainer = CreateLoadedContainer(RefreshPullDirection.LeftToRight, CreateScrollViewer());
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.ActualWidth, "Precondition: 100 wide");
                Verify.AreEqual(c_containerSize, refreshContainer.Visualizer.ActualHeight, "Precondition: not square");
                Verify.IsNotNull(((IRefreshVisualizerPrivate)refreshContainer.Visualizer).InfoProvider, "Precondition: the ScrollViewer is adapted");

                refreshContainer.PullDirection = RefreshPullDirection.RightToLeft;
                Content.UpdateLayout();

                Grid presenter = GetPresenter(refreshContainer);
                Verify.AreEqual(VerticalAlignment.Stretch, presenter.VerticalAlignment);
                Verify.AreEqual(HorizontalAlignment.Right, presenter.HorizontalAlignment);
                Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.Width);
                Verify.IsTrue(double.IsNaN(refreshContainer.Visualizer.Height), "Height stays Auto");
                Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.ActualWidth);
                Verify.AreEqual(c_containerSize, refreshContainer.Visualizer.ActualHeight);
                float rotation = ElementCompositionPreview.GetElementVisual(refreshContainer.Visualizer.Content).RotationAngle;
                Verify.IsLessThan(Math.Abs((float)(Math.PI / 2) - rotation), c_tolerance, "RightToLeft indicator rotation " + rotation);
                Verify.IsNotNull(((IRefreshVisualizerPrivate)refreshContainer.Visualizer).InfoProvider);
                Verify.AreEqual(RefreshVisualizerState.Idle, refreshContainer.Visualizer.State);
            });
        }

        [TestMethod]
        [TestProperty("Description", "RequestRefresh on the container (and on its visualizer) raises RefreshContainer.RefreshRequested once per handler, keeps the visualizer Refreshing until every deferral completes and then returns to Idle, for every pull direction.")]
        public void RequestRefreshCompletesOnlyAfterAllDeferralsComplete()
        {
            foreach (RefreshPullDirection direction in new RefreshPullDirection[] {
                RefreshPullDirection.TopToBottom, RefreshPullDirection.BottomToTop, RefreshPullDirection.LeftToRight, RefreshPullDirection.RightToLeft })
            {
                Log.Comment("PullDirection = " + direction);
                RefreshContainer refreshContainer = null;
                RunOnUIThread.Execute(() =>
                {
                    refreshContainer = CreateLoadedContainer(direction, CreateScrollViewer());
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    RefreshVisualizer visualizer = refreshContainer.Visualizer;
                    List<string> transitions = new List<string>();
                    visualizer.RefreshStateChanged += (RefreshVisualizer sender, RefreshStateChangedEventArgs args) => { transitions.Add(args.OldState + "->" + args.NewState); };

                    List<Deferral> first = new List<Deferral>();
                    List<Deferral> second = new List<Deferral>();
                    object observedSender = null;
                    RefreshVisualizerState stateDuringEvent = RefreshVisualizerState.Idle;
                    refreshContainer.RefreshRequested += (RefreshContainer sender, RefreshRequestedEventArgs args) =>
                    {
                        observedSender = sender;
                        stateDuringEvent = visualizer.State;
                        first.Add(args.GetDeferral());
                    };
                    refreshContainer.RefreshRequested += (RefreshContainer sender, RefreshRequestedEventArgs args) => { second.Add(args.GetDeferral()); };

                    Verify.AreEqual(RefreshVisualizerState.Idle, visualizer.State);
                    refreshContainer.RequestRefresh();
                    Verify.AreEqual(1, first.Count, "First handler raised once");
                    Verify.AreEqual(1, second.Count, "Second handler raised once");
                    Verify.IsTrue(object.ReferenceEquals(refreshContainer, observedSender), "Sender is the container");
                    Verify.AreEqual(RefreshVisualizerState.Refreshing, stateDuringEvent);
                    Verify.AreEqual(RefreshVisualizerState.Refreshing, visualizer.State);

                    first[0].Complete();
                    Verify.AreEqual(RefreshVisualizerState.Refreshing, visualizer.State, "One outstanding deferral keeps the refresh running");
                    second[0].Complete();
                    Verify.AreEqual(RefreshVisualizerState.Idle, visualizer.State);

                    // A refresh requested directly on the visualizer is forwarded to the container.
                    visualizer.RequestRefresh();
                    Verify.AreEqual(2, first.Count);
                    Verify.AreEqual(2, second.Count);
                    Verify.AreEqual(RefreshVisualizerState.Refreshing, visualizer.State);
                    second[1].Complete();
                    Verify.AreEqual(RefreshVisualizerState.Refreshing, visualizer.State);
                    first[1].Complete();
                    Verify.AreEqual(RefreshVisualizerState.Idle, visualizer.State);

                    Verify.AreEqual("Idle->Refreshing, Refreshing->Idle, Idle->Refreshing, Refreshing->Idle", string.Join(", ", transitions));
                });

                // Let the refresh-completed composition batch finish before the next iteration.
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(RefreshVisualizerState.Idle, refreshContainer.Visualizer.State);
                    Verify.IsNotNull(((IRefreshVisualizerPrivate)refreshContainer.Visualizer).InfoProvider);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Replacing the Visualizer after load hosts the new visualizer in the presenter, adapts the ScrollViewer for it, routes refreshes to it and does not force the default pull size onto it.")]
        public void ReplacingVisualizerAfterLoadHostsAndUsesTheNewVisualizer()
        {
            RefreshContainer refreshContainer = null;
            RefreshVisualizer original = null;
            RefreshVisualizer custom = null;

            RunOnUIThread.Execute(() =>
            {
                refreshContainer = CreateLoadedContainer(RefreshPullDirection.TopToBottom, CreateScrollViewer());
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                original = refreshContainer.Visualizer;
                custom = new RefreshVisualizer();
                refreshContainer.Visualizer = custom;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(object.ReferenceEquals(custom, refreshContainer.Visualizer));
                Grid presenter = GetPresenter(refreshContainer);
                Verify.AreEqual(1, presenter.Children.Count);
                Verify.IsTrue(object.ReferenceEquals(custom, presenter.Children[0]), "The new visualizer is hosted");
                Verify.IsNull(VisualTreeHelper.GetParent(original), "The previous visualizer is removed from the presenter");
                Verify.IsNotNull(((IRefreshVisualizerPrivate)custom).InfoProvider, "The ScrollViewer is adapted for the new visualizer");

                List<Deferral> deferrals = new List<Deferral>();
                refreshContainer.RefreshRequested += (RefreshContainer sender, RefreshRequestedEventArgs args) => { deferrals.Add(args.GetDeferral()); };
                refreshContainer.RequestRefresh();
                Verify.AreEqual(1, deferrals.Count);
                Verify.AreEqual(RefreshVisualizerState.Refreshing, custom.State);
                Verify.AreEqual(RefreshVisualizerState.Idle, original.State, "The previous visualizer is not refreshed");
                deferrals[0].Complete();
                Verify.AreEqual(RefreshVisualizerState.Idle, custom.State);

                // The container still positions the presenter but keeps the app-provided visualizer size.
                refreshContainer.PullDirection = RefreshPullDirection.LeftToRight;
                Content.UpdateLayout();
                Verify.AreEqual(HorizontalAlignment.Left, presenter.HorizontalAlignment);
                Verify.AreEqual(VerticalAlignment.Stretch, presenter.VerticalAlignment);
                Verify.IsTrue(double.IsNaN(custom.Width), "A custom visualizer is not resized to the default pull width");
                Verify.AreEqual(c_defaultPullDimension, custom.Height, "A custom visualizer keeps its own (style) height");
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // BUG-PTR-1: RefreshContainer never stores/removes its Visualizer event tokens, so a Visualizer set before the template is applied is subscribed twice and RefreshRequested is raised twice.
        [TestProperty("Description", "A Visualizer assigned before the template is applied raises RefreshContainer.RefreshRequested once per refresh.")]
        public void VisualizerSetBeforeLoadRaisesRefreshRequestedOnce()
        {
            RefreshContainer refreshContainer = null;
            RunOnUIThread.Execute(() =>
            {
                refreshContainer = new RefreshContainer() { Width = c_containerSize, Height = c_containerSize, Content = CreateScrollViewer() };
                refreshContainer.Visualizer = new RefreshVisualizer();
                Content = refreshContainer;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                int refreshRequestedCount = 0;
                refreshContainer.RefreshRequested += (RefreshContainer sender, RefreshRequestedEventArgs args) => { refreshRequestedCount++; };
                refreshContainer.RequestRefresh();
                Verify.AreEqual(1, refreshRequestedCount);
            });
        }

        [TestMethod]
        [TestProperty("Description", "A custom IRefreshInfoProviderAdapter is asked to adapt the template root with the visualizer size, its provider becomes the visualizer's InfoProvider, it receives SetAnimations, and it is not replaced when the visualizer is resized.")]
        public void CustomAdapterAdaptsTemplateRootAndIsKeptAcrossResize()
        {
            RefreshContainer refreshContainer = null;
            RecordingRefreshInfoProviderAdapter adapter = null;
            RefreshInfoProviderImpl provider = null;

            RunOnUIThread.Execute(() =>
            {
                refreshContainer = CreateLoadedContainer(RefreshPullDirection.TopToBottom, CreateScrollViewer());
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                provider = new RefreshInfoProviderImpl(ElementCompositionPreview.GetElementVisual(refreshContainer).Compositor);
                adapter = new RecordingRefreshInfoProviderAdapter(provider);
                ((IRefreshContainerPrivate)refreshContainer).RefreshInfoProviderAdapter = adapter;

                Verify.IsTrue(object.ReferenceEquals(adapter, ((IRefreshContainerPrivate)refreshContainer).RefreshInfoProviderAdapter));
                Verify.AreEqual(1, adapter.AdaptedRoots.Count);
                Grid root = adapter.AdaptedRoots[0] as Grid;
                Verify.IsNotNull(root);
                Verify.AreEqual("Root", root.Name);
                Verify.IsTrue(object.ReferenceEquals(refreshContainer, VisualTreeHelper.GetParent(root)), "The template root is adapted");
                Verify.AreEqual(refreshContainer.Visualizer.RenderSize, adapter.AdaptedSizes[0]);
                Verify.AreEqual(new Size(c_containerSize, c_defaultPullDimension), adapter.AdaptedSizes[0]);
                Verify.AreEqual(1, adapter.AnimatedContainers.Count);
                Verify.IsTrue(object.ReferenceEquals(refreshContainer.Visualizer, adapter.AnimatedContainers[0]), "SetAnimations receives the visualizer");
                Verify.IsTrue(object.ReferenceEquals(provider, ((IRefreshVisualizerPrivate)refreshContainer.Visualizer).InfoProvider));

                // The provider now drives the visualizer.
                provider.RaiseInteractionRatioChangedEvent(0.3f);
                Verify.AreEqual(RefreshVisualizerState.Peeking, refreshContainer.Visualizer.State);
                provider.RaiseInteractionRatioChangedEvent(0.0f);
                Verify.AreEqual(RefreshVisualizerState.Idle, refreshContainer.Visualizer.State);

                refreshContainer.PullDirection = RefreshPullDirection.LeftToRight;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(c_defaultPullDimension, refreshContainer.Visualizer.ActualWidth, "The visualizer was resized");
                Verify.IsTrue(object.ReferenceEquals(adapter, ((IRefreshContainerPrivate)refreshContainer).RefreshInfoProviderAdapter), "A custom adapter is not replaced on resize");
                Verify.AreEqual(1, adapter.AdaptedRoots.Count, "A custom adapter is not asked to adapt again on resize");
                Verify.IsTrue(object.ReferenceEquals(provider, ((IRefreshVisualizerPrivate)refreshContainer.Visualizer).InfoProvider));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Without a ScrollViewer to adapt, the container searches its tree (10 levels below the template root) for an element implementing IRefreshInfoProvider; deeper providers or a null adapter are handled.")]
        public void TreeSearchFindsContentProviderWithinTenLevels()
        {
            RefreshContainer reachable = null;
            RefreshInfoProviderGrid reachableProvider = null;
            RefreshContainer tooDeep = null;

            RunOnUIThread.Execute(() =>
            {
                // Root -> ContentPresenter -> 8 Borders -> provider: the provider is 10 levels below the template root.
                reachableProvider = new RefreshInfoProviderGrid();
                reachable = CreateLoadedContainer(RefreshPullDirection.TopToBottom, WrapInBorders(reachableProvider, 8));
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                IRefreshVisualizerPrivate visualizerPrivate = (IRefreshVisualizerPrivate)reachable.Visualizer;
                Verify.IsTrue(object.ReferenceEquals(reachableProvider, visualizerPrivate.InfoProvider), "The provider 10 levels deep is found");

                // The found provider drives the visualizer.
                reachableProvider.RaiseInteractionRatioChangedEvent(0.25f);
                Verify.AreEqual(RefreshVisualizerState.Peeking, reachable.Visualizer.State);
                reachableProvider.RaiseInteractionRatioChangedEvent(0.0f);
                Verify.AreEqual(RefreshVisualizerState.Idle, reachable.Visualizer.State);

                // Removing the adapter falls back to the same tree search, which re-assigns the provider.
                visualizerPrivate.InfoProvider = null;
                Verify.IsNull(visualizerPrivate.InfoProvider, "Precondition: the provider was cleared");
                ((IRefreshContainerPrivate)reachable).RefreshInfoProviderAdapter = null;
                Verify.IsNull(((IRefreshContainerPrivate)reachable).RefreshInfoProviderAdapter);
                Verify.IsTrue(object.ReferenceEquals(reachableProvider, visualizerPrivate.InfoProvider), "The tree search re-assigns the reachable provider");

                // One level deeper (11 levels) is beyond the search depth.
                RefreshInfoProviderGrid deepProvider = new RefreshInfoProviderGrid();
                tooDeep = CreateLoadedContainer(RefreshPullDirection.TopToBottom, WrapInBorders(deepProvider, 9));
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(((IRefreshVisualizerPrivate)tooDeep.Visualizer).InfoProvider, "A provider 11 levels deep is not found");
            });
        }

        private void VerifyDefaultTemplate(RefreshPullDirection direction, VerticalAlignment expectedVertical, HorizontalAlignment expectedHorizontal, float expectedRotation)
        {
            Log.Comment("PullDirection = " + direction);
            RefreshContainer refreshContainer = null;
            RunOnUIThread.Execute(() =>
            {
                refreshContainer = CreateLoadedContainer(direction, CreateScrollViewer());
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Grid root = (Grid)VisualTreeHelper.GetChild(refreshContainer, 0);
                Verify.AreEqual("Root", root.Name);
                Verify.IsTrue(ElementCompositionPreview.GetElementVisual(root).Clip is InsetClip, "The template root clips the pulled visualizer");

                RefreshVisualizer visualizer = refreshContainer.Visualizer;
                Verify.IsNotNull(visualizer, "A default visualizer is created");
                Grid presenter = GetPresenter(refreshContainer);
                Verify.AreEqual(1, presenter.Children.Count);
                Verify.AreEqual(expectedVertical, presenter.VerticalAlignment);
                Verify.AreEqual(expectedHorizontal, presenter.HorizontalAlignment);

                bool vertical = direction == RefreshPullDirection.TopToBottom || direction == RefreshPullDirection.BottomToTop;
                Verify.AreEqual(c_defaultPullDimension, vertical ? visualizer.Height : visualizer.Width);
                Verify.IsTrue(double.IsNaN(vertical ? visualizer.Width : visualizer.Height), "Cross-axis size is Auto");
                Verify.AreEqual(vertical ? c_containerSize : c_defaultPullDimension, visualizer.ActualWidth);
                Verify.AreEqual(vertical ? c_defaultPullDimension : c_containerSize, visualizer.ActualHeight);

                float rotation = ElementCompositionPreview.GetElementVisual(visualizer.Content).RotationAngle;
                Verify.IsLessThan(Math.Abs(expectedRotation - rotation), c_tolerance, "Indicator rotation " + rotation);

                IRefreshInfoProvider provider = ((IRefreshVisualizerPrivate)visualizer).InfoProvider;
                Verify.IsNotNull(provider, "The content ScrollViewer is adapted");
                Verify.IsLessThan(Math.Abs(c_defaultExecutionRatio - provider.ExecutionRatio), 0.0001, "ExecutionRatio");
                Verify.AreEqual("InteractionRatio", provider.InteractionRatioCompositionProperty);
                Verify.IsFalse(provider.IsInteractingForRefresh);
                Verify.IsNotNull(provider.CompositionProperties);
                Verify.AreEqual(RefreshVisualizerState.Idle, visualizer.State);
            });
        }

        private RefreshContainer CreateLoadedContainer(RefreshPullDirection direction, UIElement content)
        {
            RefreshContainer refreshContainer = new RefreshContainer() {
                PullDirection = direction,
                Width = c_containerSize,
                Height = c_containerSize,
                Content = content
            };
            Content = refreshContainer;
            Content.UpdateLayout();
            return refreshContainer;
        }

        private static ScrollViewer CreateScrollViewer()
        {
            return new ScrollViewer() {
                Content = new StackPanel() { Height = 1000, Background = new SolidColorBrush(Microsoft.UI.Colors.LightGray) }
            };
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

        private static Grid GetPresenter(RefreshContainer refreshContainer)
        {
            Grid presenter = VisualTreeHelper.GetParent(refreshContainer.Visualizer) as Grid;
            Verify.IsNotNull(presenter);
            Verify.AreEqual("RefreshVisualizerPresenter", presenter.Name);
            return presenter;
        }
    }

    partial class RecordingRefreshInfoProviderAdapter : IRefreshInfoProviderAdapter
    {
        private IRefreshInfoProvider provider;

        public List<UIElement> AdaptedRoots = new List<UIElement>();
        public List<Size> AdaptedSizes = new List<Size>();
        public List<UIElement> AnimatedContainers = new List<UIElement>();

        public RecordingRefreshInfoProviderAdapter(IRefreshInfoProvider provider)
        {
            this.provider = provider;
        }

        public IRefreshInfoProvider AdaptFromTree(UIElement root, Size visualizerSize)
        {
            AdaptedRoots.Add(root);
            AdaptedSizes.Add(visualizerSize);
            return provider;
        }

        public void SetAnimations(UIElement refreshVisualizerAnimatableContainer)
        {
            AnimatedContainers.Add(refreshVisualizerAnimatableContainer);
        }
    }

    // A content element that is itself an IRefreshInfoProvider, found by RefreshContainer's tree search.
    partial class RefreshInfoProviderGrid : Grid, IRefreshInfoProvider
    {
        private CompositionPropertySet compositionProperties;

        public RefreshInfoProviderGrid()
        {
            compositionProperties = ElementCompositionPreview.GetElementVisual(this).Compositor.CreatePropertySet();
            compositionProperties.InsertScalar(InteractionRatioCompositionProperty, 0.0f);
        }

        public void RaiseInteractionRatioChangedEvent(float interactionRatio)
        {
            if (this.InteractionRatioChanged != null)
            {
                this.InteractionRatioChanged.Invoke(this, PullToRefreshHelperTestApi.CreateRefreshInteractionRatioChangedEventArgsInstance(interactionRatio));
            }
        }

        public double ExecutionRatio
        {
            get { return 0.8; }
        }

        public CompositionPropertySet CompositionProperties
        {
            get { return compositionProperties; }
        }

        public string InteractionRatioCompositionProperty
        {
            get { return "InteractionRatio"; }
        }

        public bool IsInteractingForRefresh
        {
            get { return false; }
        }

        public event TypedEventHandler<IRefreshInfoProvider, RefreshInteractionRatioChangedEventArgs> InteractionRatioChanged;
#pragma warning disable CS0067 // Required by IRefreshInfoProvider; this provider never interacts.
        public event TypedEventHandler<IRefreshInfoProvider, object> IsInteractingForRefreshChanged;
#pragma warning restore CS0067
        public event TypedEventHandler<IRefreshInfoProvider, object> RefreshCompleted;
        public event TypedEventHandler<IRefreshInfoProvider, object> RefreshStarted;

        public void OnRefreshCompleted()
        {
            if (this.RefreshCompleted != null)
            {
                this.RefreshCompleted.Invoke(this, new EventArgs());
            }
        }

        public void OnRefreshStarted()
        {
            if (this.RefreshStarted != null)
            {
                this.RefreshStarted.Invoke(this, new EventArgs());
            }
        }
    }
}
