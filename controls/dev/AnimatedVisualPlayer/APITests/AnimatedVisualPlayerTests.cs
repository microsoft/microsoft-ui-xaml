// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Common;
using Microsoft.UI.Composition;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using Windows.Foundation;
using Windows.Graphics;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class AnimatedVisualPlayerTests : ApiTestBase
    {
        private const int CompletionTimeoutMs = 5000;
        private static readonly TimeSpan ShortDuration = TimeSpan.FromMilliseconds(300);
        private static readonly TimeSpan HiddenPlayDuration = TimeSpan.FromMilliseconds(1500);
        private const int HostVisibilityTimeoutMs = 5000;

        [TestMethod]
        public void FallbackContentIsShownWhenSourceFailsAndRemovedWhenContentLoads()
        {
            AnimatedVisualPlayer player = null;
            AvpTestSource failingSource = null;
            AvpTestSource validSource = null;

            RunOnUIThread.Execute(() =>
            {
                failingSource = new AvpTestSource() { FailToCreate = true, DiagnosticsToReturn = "load failed" };
                player = CreatePlayer();
                player.FallbackContent = CreateFallbackTemplate("FallbackA", 100, 50);
                player.Source = failingSource;
            });

            LoadInHost(player);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, failingSource.CreateCount, "The failing source should have been asked once for content.");
                Verify.IsFalse(player.IsAnimatedVisualLoaded, "A source that fails to create content must not be reported as loaded.");
                Verify.AreEqual("load failed", player.Diagnostics as string, "Diagnostics from the failed load should be surfaced.");

                var fallback = GetSingleFallbackChild(player);
                Verify.AreEqual("FallbackA", fallback.Tag as string);
                Verify.AreEqual(100.0, player.DesiredSize.Width, "Desired width should come from the fallback content.");
                Verify.AreEqual(50.0, player.DesiredSize.Height, "Desired height should come from the fallback content.");
                Verify.AreEqual(100.0, player.ActualWidth, "Arranged width should come from the fallback content.");
                Verify.AreEqual(50.0, player.ActualHeight, "Arranged height should come from the fallback content.");

                validSource = new AvpTestSource() { Duration = TimeSpan.FromMilliseconds(750) };
                player.Source = validSource;
                Content.UpdateLayout();

                Verify.IsTrue(player.IsAnimatedVisualLoaded, "Valid content should load after a previous failure.");
                Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(player), "Fallback content should be removed when content loads.");
                Verify.IsNull(fallback.Parent, "The fallback element should be detached from the player.");
                Verify.AreEqual(TimeSpan.FromMilliseconds(750), player.Duration);
                Verify.IsNull(player.Diagnostics, "Diagnostics should reflect the latest (successful) load.");
            });
        }

        [TestMethod]
        public void FallbackContentChangesWhileFallenBackAreApplied()
        {
            AnimatedVisualPlayer player = null;

            RunOnUIThread.Execute(() =>
            {
                player = CreatePlayer();
                player.Source = new AvpTestSource() { FailToCreate = true };
            });

            LoadInHost(player);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(player.IsAnimatedVisualLoaded);
                Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(player), "No fallback template means no fallback content.");
                Verify.AreEqual(0.0, player.DesiredSize.Width);
                Verify.AreEqual(0.0, player.DesiredSize.Height);

                player.FallbackContent = CreateFallbackTemplate("FallbackA", 100, 50);
                Content.UpdateLayout();
                Verify.AreEqual("FallbackA", GetSingleFallbackChild(player).Tag as string, "Setting FallbackContent while fallen back should show it.");
                Verify.AreEqual(100.0, player.DesiredSize.Width);
                Verify.AreEqual(50.0, player.DesiredSize.Height);

                player.FallbackContent = CreateFallbackTemplate("FallbackB", 60, 30);
                Content.UpdateLayout();
                Verify.AreEqual("FallbackB", GetSingleFallbackChild(player).Tag as string, "Replacing FallbackContent while fallen back should replace the shown content.");
                Verify.AreEqual(60.0, player.DesiredSize.Width);
                Verify.AreEqual(30.0, player.DesiredSize.Height);

                player.FallbackContent = null;
                Content.UpdateLayout();
                Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(player), "Clearing FallbackContent while fallen back should remove the shown content.");
                Verify.AreEqual(0.0, player.DesiredSize.Width);
                Verify.AreEqual(0.0, player.DesiredSize.Height);
            });
        }

        [TestMethod]
        public void MeasureRespectsStretch()
        {
            AnimatedVisualPlayer player = null;
            AnimatedVisualPlayer emptyPlayer = null;
            AnimatedVisualPlayer noSourcePlayer = null;
            AvpTestSource emptySource = null;

            RunOnUIThread.Execute(() =>
            {
                player = CreatePlayer();
                player.Source = new AvpTestSource() { Size = new Vector2(200, 100) };

                emptySource = new AvpTestSource() { Size = Vector2.Zero };
                emptyPlayer = CreatePlayer();
                emptyPlayer.Source = emptySource;

                noSourcePlayer = CreatePlayer();
            });

            LoadInHost(() => new Grid() { Width = 600, Height = 600, Children = { player, emptyPlayer, noSourcePlayer } }, player);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(player.IsAnimatedVisualLoaded);

                VerifyMeasure(player, Stretch.None, new Size(150, 300), new Size(150, 100));
                VerifyMeasure(player, Stretch.None, new Size(400, 400), new Size(200, 100));

                VerifyMeasure(player, Stretch.Uniform, new Size(400, 400), new Size(400, 200));
                VerifyMeasure(player, Stretch.Uniform, new Size(100, double.PositiveInfinity), new Size(100, 50));
                VerifyMeasure(player, Stretch.Uniform, new Size(double.PositiveInfinity, 50), new Size(100, 50));

                VerifyMeasure(player, Stretch.Fill, new Size(300, 50), new Size(300, 50));
                VerifyMeasure(player, Stretch.Fill, new Size(100, double.PositiveInfinity), new Size(100, 50));

                VerifyMeasure(player, Stretch.UniformToFill, new Size(300, 60), new Size(300, 60));
                VerifyMeasure(player, Stretch.UniformToFill, new Size(100, double.PositiveInfinity), new Size(100, 50));

                Verify.AreEqual(1, emptySource.CreateCount);
                Verify.IsFalse(emptyPlayer.IsAnimatedVisualLoaded, "Zero-size content is treated as empty and not loaded.");
                VerifyMeasure(emptyPlayer, Stretch.Uniform, new Size(400, 400), new Size(0, 0));

                Verify.IsFalse(noSourcePlayer.IsAnimatedVisualLoaded);
                VerifyMeasure(noSourcePlayer, Stretch.Fill, new Size(400, 400), new Size(0, 0));
            });
        }

        [TestMethod]
        public void ArrangeScalesAndPositionsContentForEachStretch()
        {
            AnimatedVisualPlayer player = null;

            RunOnUIThread.Execute(() =>
            {
                player = CreatePlayer();
                player.Width = 400;
                player.Height = 400;
                player.Source = new AvpTestSource() { Size = new Vector2(200, 100) };
            });

            LoadInHost(player);

            RunOnUIThread.Execute(() =>
            {
                var rootVisual = ElementCompositionPreview.GetElementChildVisual(player);
                Verify.IsNotNull(rootVisual, "The player should host its content in a child visual once loaded.");

                Verify.AreEqual(Stretch.Uniform, player.Stretch, "Uniform is the default Stretch.");
                ArrangeWithStretch(player, Stretch.Uniform);
                VerifyVector3(new Vector3(2, 2, 1), rootVisual.Scale, "Uniform scale");
                VerifyVector3(new Vector3(0, 100, 0), rootVisual.Offset, "Uniform offset (centered vertically)");
                VerifyVector2(new Vector2(200, 100), rootVisual.Size, "Uniform size");
                VerifyVector2(Vector2.Zero, rootVisual.Clip.Offset, "Uniform clip offset");

                ArrangeWithStretch(player, Stretch.UniformToFill);
                VerifyVector3(new Vector3(4, 4, 1), rootVisual.Scale, "UniformToFill scale");
                VerifyVector3(new Vector3(-200, 0, 0), rootVisual.Offset, "UniformToFill offset (centered horizontally)");
                VerifyVector2(new Vector2(100, 100), rootVisual.Size, "UniformToFill size");
                VerifyVector2(new Vector2(50, 0), rootVisual.Clip.Offset, "UniformToFill clip offset");

                ArrangeWithStretch(player, Stretch.Fill);
                VerifyVector3(new Vector3(2, 4, 1), rootVisual.Scale, "Fill scale");
                VerifyVector3(Vector3.Zero, rootVisual.Offset, "Fill offset");
                VerifyVector2(new Vector2(200, 100), rootVisual.Size, "Fill size");
                VerifyVector2(Vector2.Zero, rootVisual.Clip.Offset, "Fill clip offset");

                ArrangeWithStretch(player, Stretch.None);
                VerifyVector3(new Vector3(1, 1, 1), rootVisual.Scale, "None scale");
                VerifyVector2(new Vector2(200, 100), rootVisual.Size, "None size");

                Verify.AreEqual(400.0, player.ActualWidth);
                Verify.AreEqual(400.0, player.ActualHeight);

                // A tall source (100x200) reverses which dimension limits the scale: the width scale (4) is
                // larger than the height scale (2).
                player.Source = new AvpTestSource() { Size = new Vector2(100, 200) };
                Verify.IsTrue(player.IsAnimatedVisualLoaded);
                Verify.IsTrue(ReferenceEquals(rootVisual, ElementCompositionPreview.GetElementChildVisual(player)), "The player keeps the same root visual for new content.");

                ArrangeWithStretch(player, Stretch.Uniform);
                VerifyVector3(new Vector3(2, 2, 1), rootVisual.Scale, "Tall Uniform scale (height limits)");
                VerifyVector3(new Vector3(100, 0, 0), rootVisual.Offset, "Tall Uniform offset (centered horizontally)");
                VerifyVector2(new Vector2(100, 200), rootVisual.Size, "Tall Uniform size");
                VerifyVector2(Vector2.Zero, rootVisual.Clip.Offset, "Tall Uniform clip offset");

                ArrangeWithStretch(player, Stretch.UniformToFill);
                VerifyVector3(new Vector3(4, 4, 1), rootVisual.Scale, "Tall UniformToFill scale (width fills)");
                VerifyVector3(new Vector3(0, -200, 0), rootVisual.Offset, "Tall UniformToFill offset (centered vertically)");
                VerifyVector2(new Vector2(100, 100), rootVisual.Size, "Tall UniformToFill size");
                VerifyVector2(new Vector2(0, 50), rootVisual.Clip.Offset, "Tall UniformToFill clip offset");
            });
        }

        // Scenario: a player shows content and plays it, then its Source is cleared.
        // Expected: the play completes, the content is closed and removed from the player's visual, and the
        //           content-derived properties (IsAnimatedVisualLoaded, Duration, Diagnostics, desired size) are reset.
        [TestMethod]
        public void ClearingSourceUnloadsContentAndResetsState()
        {
            AnimatedVisualPlayer player = null;
            AvpTestSource source = null;
            Task play = null;

            RunOnUIThread.Execute(() =>
            {
                source = new AvpTestSource() { Duration = TimeSpan.FromMilliseconds(750), DiagnosticsToReturn = "content diagnostics" };
                player = CreatePlayer();
                player.AutoPlay = false;
                player.Stretch = Stretch.None;
                player.Source = source;
            });

            LoadInHost(player);

            RunOnUIThread.Execute(() =>
            {
                var rootVisual = (ContainerVisual)ElementCompositionPreview.GetElementChildVisual(player);
                Verify.IsTrue(player.IsAnimatedVisualLoaded);
                Verify.AreEqual(TimeSpan.FromMilliseconds(750), player.Duration);
                Verify.AreEqual("content diagnostics", player.Diagnostics as string);
                Verify.AreEqual(200.0, player.DesiredSize.Width);
                Verify.AreEqual(100.0, player.DesiredSize.Height);
                Verify.AreEqual(1, rootVisual.Children.Count, "The content's root visual is hosted by the player.");
                Verify.IsFalse(source.LastVisual.IsDisposed);

                play = player.PlayAsync(0, 1, true).AsTask();
                Verify.IsTrue(player.IsPlaying);

                player.Source = null;
                Content.UpdateLayout();

                Verify.IsNull(player.Source);
                Verify.IsFalse(player.IsPlaying, "Clearing Source stops the current play.");
                Verify.IsFalse(player.IsAnimatedVisualLoaded, "Clearing Source unloads the content.");
                Verify.AreEqual(TimeSpan.Zero, player.Duration, "Duration is reset without content.");
                Verify.IsNull(player.Diagnostics, "Diagnostics are reset without content.");
                Verify.AreEqual(0.0, player.DesiredSize.Width, "A player without content has no desired width.");
                Verify.AreEqual(0.0, player.DesiredSize.Height, "A player without content has no desired height.");
                Verify.AreEqual(0, rootVisual.Children.Count, "The content's root visual is removed from the player.");
                Verify.IsTrue(source.LastVisual.IsDisposed, "The unloaded content is closed.");
                Verify.AreEqual(1, source.CreateCount, "Clearing Source does not ask the old source for content again.");
            });

            Verify.IsTrue(play.Wait(CompletionTimeoutMs), "The looped play is completed when Source is cleared.");
        }

        [TestMethod]
        public void AutoPlayStartsLoopedPlayOnlyWhenContentIsLoaded()
        {
            AnimatedVisualPlayer manualPlayer = null;
            AnimatedVisualPlayer autoPlayer = null;
            AnimatedVisualPlayer emptyPlayer = null;
            AvpTestDynamicSource lateSource = null;

            RunOnUIThread.Execute(() =>
            {
                manualPlayer = CreatePlayer();
                manualPlayer.AutoPlay = false;
                manualPlayer.Source = new AvpTestSource() { Duration = ShortDuration };

                autoPlayer = CreatePlayer();

                // A source whose content is empty until it announces new content.
                lateSource = new AvpTestDynamicSource() { Size = Vector2.Zero, Duration = ShortDuration };
                emptyPlayer = CreatePlayer();
                emptyPlayer.AutoPlay = false;
                emptyPlayer.Source = lateSource;
            });

            LoadInHost(() => new StackPanel() { Children = { manualPlayer, autoPlayer, emptyPlayer } }, manualPlayer);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(manualPlayer.IsPlaying, "AutoPlay=false must not start playing on load.");
                manualPlayer.AutoPlay = true;
                Verify.IsTrue(manualPlayer.IsPlaying, "Turning AutoPlay on with loaded content should start playing.");

                Verify.IsTrue(autoPlayer.AutoPlay, "AutoPlay defaults to true.");
                Verify.IsFalse(autoPlayer.IsPlaying);
                autoPlayer.Source = new AvpTestSource() { Duration = ShortDuration };
                Verify.IsTrue(autoPlayer.IsPlaying, "Content loaded while AutoPlay=true should start playing.");

                // Turning AutoPlay on before content exists must still play once the content loads.
                Verify.IsFalse(emptyPlayer.IsAnimatedVisualLoaded);
                emptyPlayer.AutoPlay = true;
                lateSource.Size = new Vector2(200, 100);
                lateSource.RaiseInvalidated();
                Verify.IsTrue(emptyPlayer.IsAnimatedVisualLoaded);
                Verify.IsTrue(emptyPlayer.IsPlaying, "Content that loads after AutoPlay was enabled should start playing.");
            });

            // The AutoPlay play is looped, so it continues past its duration until stopped.
            Thread.Sleep(ShortDuration * 3);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(manualPlayer.IsPlaying, "AutoPlay starts a looped play that keeps playing past its duration.");
                Verify.IsTrue(autoPlayer.IsPlaying, "AutoPlay starts a looped play that keeps playing past its duration.");
                Verify.IsTrue(emptyPlayer.IsPlaying, "AutoPlay starts a looped play that keeps playing past its duration.");
                manualPlayer.Stop();
                autoPlayer.Stop();
                emptyPlayer.Stop();
                Verify.IsFalse(manualPlayer.IsPlaying);
                Verify.IsFalse(autoPlayer.IsPlaying);
                Verify.IsFalse(emptyPlayer.IsPlaying);
            });
        }

        [TestMethod]
        public void AutoPlayDoesNotReplaceAPlayAlreadyInProgress()
        {
            AnimatedVisualPlayer player = null;
            Task play = null;

            RunOnUIThread.Execute(() =>
            {
                player = CreatePlayer();
                player.AutoPlay = false;
                player.Source = new AvpTestSource() { Duration = ShortDuration };
            });

            LoadInHost(player);

            RunOnUIThread.Execute(() =>
            {
                play = player.PlayAsync(0, 1, false).AsTask();
                Verify.IsTrue(player.IsPlaying);
                player.AutoPlay = true;
            });

            // If AutoPlay had replaced the play, the original action would be completed and a looped play would keep
            // IsPlaying true. Instead the original non-looped play runs to its end and then playing stops.
            Verify.IsTrue(play.Wait(CompletionTimeoutMs), "The original non-looped play should complete.");
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(player.AutoPlay);
                Verify.IsFalse(player.IsPlaying, "No looped AutoPlay play should have replaced the original play.");
            });
        }

        [TestMethod]
        public void DynamicSourceInvalidationReloadsContent()
        {
            AnimatedVisualPlayer player = null;
            AvpTestDynamicSource dynamicSource = null;
            AvpTestSource replacement = null;

            RunOnUIThread.Execute(() =>
            {
                dynamicSource = new AvpTestDynamicSource() { Size = Vector2.Zero };
                player = CreatePlayer();
                player.AutoPlay = false;
                player.Source = dynamicSource;
            });

            LoadInHost(player);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, dynamicSource.CreateCount);
                Verify.AreEqual(1, dynamicSource.SubscriberCount, "The player should subscribe to AnimatedVisualInvalidated.");
                Verify.IsFalse(player.IsAnimatedVisualLoaded, "Empty dynamic content is not loaded.");
                Verify.AreEqual(TimeSpan.Zero, player.Duration);

                dynamicSource.Size = new Vector2(200, 100);
                dynamicSource.Duration = TimeSpan.FromSeconds(1);
                dynamicSource.RaiseInvalidated();

                Verify.AreEqual(2, dynamicSource.CreateCount, "Invalidation should cause the player to recreate content.");
                Verify.IsTrue(player.IsAnimatedVisualLoaded, "Recreated non-empty content should be loaded.");
                Verify.AreEqual(TimeSpan.FromSeconds(1), player.Duration);
                var dynamicVisual = dynamicSource.LastVisual;
                Verify.IsFalse(dynamicVisual.IsDisposed);

                replacement = new AvpTestSource() { Duration = TimeSpan.FromMilliseconds(400) };
                player.Source = replacement;

                Verify.IsTrue(dynamicVisual.IsDisposed, "Content from the replaced source should be closed.");
                Verify.AreEqual(0, dynamicSource.SubscriberCount, "The player should unsubscribe from the replaced source.");

                dynamicSource.RaiseInvalidated();
                Verify.AreEqual(2, dynamicSource.CreateCount, "Invalidations from a replaced source must be ignored.");
                Verify.AreEqual(1, replacement.CreateCount, "Invalidations from a replaced source must not reload the current source.");
                Verify.IsTrue(player.IsAnimatedVisualLoaded);
                Verify.AreEqual(TimeSpan.FromMilliseconds(400), player.Duration);
            });
        }

        [TestMethod]
        public void AnimationOptimizationControlsAnimationCreation()
        {
            AnimatedVisualPlayer latencyPlayer = null;
            AnimatedVisualPlayer resourcesPlayer = null;
            AvpTestSource3 latencySource = null;
            AvpTestSource3 resourcesSource = null;

            RunOnUIThread.Execute(() =>
            {
                latencySource = new AvpTestSource3();
                latencyPlayer = CreatePlayer();
                latencyPlayer.AutoPlay = false;
                latencyPlayer.Source = latencySource;

                resourcesSource = new AvpTestSource3();
                resourcesPlayer = CreatePlayer();
                resourcesPlayer.AutoPlay = false;
                resourcesPlayer.AnimationOptimization = PlayerAnimationOptimization.Resources;
                resourcesPlayer.Source = resourcesSource;
            });

            LoadInHost(() => new StackPanel() { Children = { latencyPlayer, resourcesPlayer } }, latencyPlayer);

            AvpTestAnimatedVisual latencyVisual = null;
            AvpTestAnimatedVisual resourcesVisual = null;
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(PlayerAnimationOptimization.Latency, latencyPlayer.AnimationOptimization, "Latency is the default optimization.");
                Verify.AreEqual(1, latencySource.Source3CreateCount, "IAnimatedVisualSource3 should be preferred.");
                Verify.AreEqual(0, latencySource.LegacyCreateCount);
                Verify.AreEqual((bool?)true, latencySource.LastCreateAnimationsArgument, "Latency requests animations up front.");
                Verify.IsTrue(latencyPlayer.IsAnimatedVisualLoaded);

                Verify.AreEqual(1, resourcesSource.Source3CreateCount);
                Verify.AreEqual(0, resourcesSource.LegacyCreateCount);
                Verify.AreEqual((bool?)false, resourcesSource.LastCreateAnimationsArgument, "Resources defers animation creation.");

                latencyVisual = latencySource.LastVisual;
                resourcesVisual = resourcesSource.LastVisual;
                Verify.AreEqual(0, latencyVisual.CreateAnimationsCount);
                Verify.AreEqual(0, latencyVisual.DestroyAnimationsCount);

                latencyPlayer.AnimationOptimization = PlayerAnimationOptimization.Resources;
            });

            WaitForCondition(() => latencyVisual.DestroyAnimationsCount == 1, "Switching an idle player to Resources should destroy its animations.");

            RunOnUIThread.Execute(() =>
            {
                latencyPlayer.AnimationOptimization = PlayerAnimationOptimization.Latency;
                Verify.AreEqual(1, latencyVisual.CreateAnimationsCount, "Switching an idle player back to Latency should recreate its animations.");

                // Animations were never created for the Resources player, so SetProgress must create them
                // before it applies the progress, and then release them again because of Resources.
                Verify.AreEqual(0, resourcesVisual.CreateAnimationsCount);
                resourcesPlayer.SetProgress(0.5);
                Verify.AreEqual(1, resourcesVisual.CreateAnimationsCount, "SetProgress should create animations on demand.");
                Verify.AreEqual(0.5f, GetProgress(resourcesPlayer));
            });

            WaitForCondition(() => resourcesVisual.DestroyAnimationsCount == 1, "SetProgress with Resources should destroy the animations afterwards.");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, latencyVisual.DestroyAnimationsCount, "Latency must not destroy animations again.");
            });
        }

        [TestMethod]
        public void AnimationOptimizationChangeIsDeferredWhilePlaying()
        {
            AnimatedVisualPlayer player = null;
            AvpTestSource3 source = null;
            Task play = null;

            RunOnUIThread.Execute(() =>
            {
                source = new AvpTestSource3();
                player = CreatePlayer();
                player.AutoPlay = false;
                player.Source = source;
            });

            LoadInHost(player);

            AvpTestAnimatedVisual visual = null;
            RunOnUIThread.Execute(() =>
            {
                visual = source.LastVisual;
                play = player.PlayAsync(0, 1, true).AsTask();
                Verify.IsTrue(player.IsPlaying);
                player.AnimationOptimization = PlayerAnimationOptimization.Resources;
            });

            Verify.IsFalse(play.Wait(ShortDuration * 2), "A looped play does not complete on its own.");
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(player.IsPlaying);
                Verify.AreEqual(0, visual.DestroyAnimationsCount, "Animations must not be destroyed while a play is in progress.");
                player.SetProgress(0.75);
            });

            Verify.IsTrue(play.Wait(CompletionTimeoutMs), "SetProgress should complete the looped play.");
            WaitForCondition(() => visual.DestroyAnimationsCount == 1, "Once the play ends under Resources, animations should be destroyed.");
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(player.IsPlaying);
            });
        }

        // Scenario: an idle player whose animations exist switches AnimationOptimization to Resources and back to
        //           Latency in the same UI tick, before the deferred DestroyAnimations runs.
        // Expected: the destroy that Resources scheduled (after a compositor commit) is superseded by the switch back,
        //           so the content's animations are neither destroyed nor recreated. Switching to Resources alone on
        //           the same player does destroy them, which shows the observation window is long enough.
        [TestMethod]
        public void SwitchingBackToLatencyInTheSameTickCancelsTheDeferredDestroy()
        {
            AnimatedVisualPlayer player = null;
            AvpTestSource3 source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new AvpTestSource3();
                player = CreatePlayer();
                player.AutoPlay = false;
                player.Source = source;
            });

            LoadInHost(player);

            AvpTestAnimatedVisual visual = null;
            RunOnUIThread.Execute(() =>
            {
                visual = source.LastVisual;
                Verify.AreEqual((bool?)true, source.LastCreateAnimationsArgument, "Latency creates the animations with the content.");
                Verify.AreEqual(0, visual.CreateAnimationsCount);
                Verify.AreEqual(0, visual.DestroyAnimationsCount);

                player.AnimationOptimization = PlayerAnimationOptimization.Resources;
                player.AnimationOptimization = PlayerAnimationOptimization.Latency;

                Verify.AreEqual(0, visual.CreateAnimationsCount, "The animations still exist, so switching back must not recreate them.");
                Verify.AreEqual(0, visual.DestroyAnimationsCount, "Animations are destroyed only after a compositor commit.");
            });

            // The deferred destroy runs when the commit requested by the player completes. Wait for two later commits.
            WaitForCompositorCommit(player);
            WaitForCompositorCommit(player);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, visual.DestroyAnimationsCount, "A destroy superseded by switching back to Latency must not run.");
                Verify.AreEqual(0, visual.CreateAnimationsCount);
                Verify.AreEqual(PlayerAnimationOptimization.Latency, player.AnimationOptimization);

                // Contrast: Resources alone destroys the animations after the commit.
                player.AnimationOptimization = PlayerAnimationOptimization.Resources;
            });

            WaitForCondition(() => visual.DestroyAnimationsCount == 1, "Switching an idle player to Resources should destroy its animations.");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, visual.CreateAnimationsCount);
                player.AnimationOptimization = PlayerAnimationOptimization.Latency;
                Verify.AreEqual(1, visual.CreateAnimationsCount, "Switching back to Latency after the destroy recreates the animations.");
            });
        }

        [TestMethod]
        public void ResourcesOptimizationReleasesAnimationsAfterPlayCompletes()
        {
            AnimatedVisualPlayer player = null;
            AvpTestSource source = null;
            Task play = null;

            RunOnUIThread.Execute(() =>
            {
                source = new AvpTestSource() { Duration = ShortDuration };
                player = CreatePlayer();
                player.AutoPlay = false;
                player.AnimationOptimization = PlayerAnimationOptimization.Resources;
                player.Source = source;
            });

            LoadInHost(player);

            AvpTestAnimatedVisual visual = null;
            RunOnUIThread.Execute(() => visual = source.LastVisual);

            // Legacy sources always create animations, so Resources releases them right after loading.
            WaitForCondition(() => visual.DestroyAnimationsCount == 1, "Resources should release animations created by a legacy source.");

            RunOnUIThread.Execute(() =>
            {
                play = player.PlayAsync(0, 1, false).AsTask();
                Verify.AreEqual(1, visual.CreateAnimationsCount, "Playing should recreate the animations.");
                Verify.IsTrue(player.IsPlaying);
            });

            Verify.IsTrue(play.Wait(CompletionTimeoutMs), "A non-looped play should complete.");
            WaitForCondition(() => visual.DestroyAnimationsCount == 2, "Resources should release animations once the play completes.");
            RunOnUIThread.Execute(() => Verify.IsFalse(player.IsPlaying));
        }

        [TestMethod]
        public void SetProgressClampsAndCompletesTheCurrentPlay()
        {
            AnimatedVisualPlayer player = null;
            AnimatedVisualPlayer emptyPlayer = null;
            Task loopedPlay = null;
            Task pendingPlay = null;

            RunOnUIThread.Execute(() =>
            {
                player = CreatePlayer();
                player.AutoPlay = false;
                player.Source = new AvpTestSource() { Duration = ShortDuration };

                emptyPlayer = CreatePlayer();
                emptyPlayer.AutoPlay = false;
            });

            LoadInHost(() => new StackPanel() { Children = { player, emptyPlayer } }, player);

            RunOnUIThread.Execute(() =>
            {
                player.SetProgress(0.4);
                Verify.AreEqual(0.4f, GetProgress(player));
                player.SetProgress(-0.5);
                Verify.AreEqual(0.0f, GetProgress(player), "Progress below 0 is clamped to 0.");
                player.SetProgress(1.5);
                Verify.AreEqual(1.0f, GetProgress(player), "Progress above 1 is clamped to 1.");

                loopedPlay = player.PlayAsync(0, 1, true).AsTask();
                Verify.IsTrue(player.IsPlaying);

                emptyPlayer.SetProgress(2.0);
                Verify.AreEqual(1.0f, GetProgress(emptyPlayer), "Progress is clamped even without content.");
                pendingPlay = emptyPlayer.PlayAsync(0, 1, false).AsTask();
                Verify.IsFalse(emptyPlayer.IsPlaying, "A play without content does not start.");
            });

            Verify.IsFalse(loopedPlay.Wait(ShortDuration * 2), "A looped play does not complete on its own.");
            Verify.IsFalse(pendingPlay.Wait(ShortDuration), "A play without content stays pending.");

            RunOnUIThread.Execute(() =>
            {
                player.SetProgress(0.25);
                Verify.AreEqual(0.25f, GetProgress(player));
                emptyPlayer.Stop();
            });

            Verify.IsTrue(loopedPlay.Wait(CompletionTimeoutMs), "SetProgress should complete the current play.");
            Verify.IsTrue(pendingPlay.Wait(CompletionTimeoutMs), "Stop should complete a pending play.");
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(player.IsPlaying);
                Verify.IsFalse(emptyPlayer.IsPlaying);
            });
        }

        [TestMethod]
        public void PlayRequestedDuringContentLoadStartsOnceLoadedAndHonorsPause()
        {
            AnimatedVisualPlayer player = null;
            Task play = null;
            bool? isPlayingWhenRequested = null;
            bool? isLoadedWhenRequested = null;
            long callbackToken = 0;

            RunOnUIThread.Execute(() =>
            {
                player = CreatePlayer();
                player.AutoPlay = false;
            });

            LoadInHost(player);

            RunOnUIThread.Execute(() =>
            {
                // Duration is updated while the player is loading new content, before IsAnimatedVisualLoaded
                // becomes true. Requesting a play (and pausing it) from that callback exercises the path where
                // the player starts a play that was requested during loading.
                callbackToken = player.RegisterPropertyChangedCallback(AnimatedVisualPlayer.DurationProperty, (sender, dp) =>
                {
                    if (play == null && player.Duration > TimeSpan.Zero)
                    {
                        isLoadedWhenRequested = player.IsAnimatedVisualLoaded;
                        play = player.PlayAsync(0, 1, false).AsTask();
                        isPlayingWhenRequested = player.IsPlaying;
                        player.Pause();
                    }
                });

                player.Source = new AvpTestSource() { Duration = ShortDuration };
                player.UnregisterPropertyChangedCallback(AnimatedVisualPlayer.DurationProperty, callbackToken);

                Verify.IsNotNull(play, "The Duration callback should have requested a play.");
                Verify.AreEqual((bool?)false, isLoadedWhenRequested, "The play was requested before loading finished.");
                Verify.AreEqual((bool?)false, isPlayingWhenRequested, "The play cannot start before content is loaded.");
                Verify.IsTrue(player.IsAnimatedVisualLoaded);
                Verify.IsTrue(player.IsPlaying, "The requested play should start once content is loaded.");
            });

            Verify.IsFalse(play.Wait(ShortDuration * 3), "A play paused before it started must not progress to completion.");

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(player.IsPlaying);
                player.Resume();
            });

            Verify.IsTrue(play.Wait(CompletionTimeoutMs), "The play should complete after being resumed.");
            RunOnUIThread.Execute(() => Verify.IsFalse(player.IsPlaying));
        }

        [TestMethod]
        public void PlayAsyncStartedFromIsPlayingCallbackSupersedesTheOuterPlay()
        {
            AnimatedVisualPlayer player = null;
            Task firstPlay = null;
            Task outerPlay = null;
            Task reentrantPlay = null;
            long callbackToken = 0;

            RunOnUIThread.Execute(() =>
            {
                player = CreatePlayer();
                player.AutoPlay = false;
                player.Source = new AvpTestSource() { Duration = ShortDuration };
            });

            LoadInHost(player);

            RunOnUIThread.Execute(() =>
            {
                firstPlay = player.PlayAsync(0, 1, true).AsTask();
                Verify.IsTrue(player.IsPlaying);

                bool armed = true;
                callbackToken = player.RegisterPropertyChangedCallback(AnimatedVisualPlayer.IsPlayingProperty, (sender, dp) =>
                {
                    if (armed && !player.IsPlaying)
                    {
                        armed = false;
                        reentrantPlay = player.PlayAsync(0, 1, true).AsTask();
                    }
                });

                // Starting a new play completes the first one, which sets IsPlaying=false and re-enters PlayAsync.
                outerPlay = player.PlayAsync(0, 1, true).AsTask();
                player.UnregisterPropertyChangedCallback(AnimatedVisualPlayer.IsPlayingProperty, callbackToken);

                Verify.IsNotNull(reentrantPlay, "IsPlaying should have changed to false while starting the new play.");
                Verify.IsTrue(player.IsPlaying, "The reentrant play should be the one playing.");
            });

            Verify.IsTrue(firstPlay.Wait(CompletionTimeoutMs), "The first play is completed when a new play starts.");
            Verify.IsTrue(outerPlay.Wait(CompletionTimeoutMs), "A looped play overtaken by a reentrant play completes without playing.");
            Verify.IsFalse(reentrantPlay.Wait(ShortDuration * 2), "The reentrant looped play keeps playing.");

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(player.IsPlaying);
                player.Stop();
            });

            Verify.IsTrue(reentrantPlay.Wait(CompletionTimeoutMs), "Stop should complete the reentrant play.");
            RunOnUIThread.Execute(() => Verify.IsFalse(player.IsPlaying));
        }

        // Scenario: read every public static DependencyProperty identifier of AnimatedVisualPlayer, then for each
        //           property compare GetValue/SetValue/ClearValue with the CLR property on an unparented player.
        // Expected: identifiers are non-null, distinct and stable; GetValue matches the CLR default; values set through
        //           either surface are visible through the other; ClearValue restores the default. Read-only properties
        //           report their defaults. Setting Source through the DP reaches the player's change handler.
        [TestMethod]
        public void DependencyPropertyIdentifiersAndClrPropertiesAgree()
        {
            RunOnUIThread.Execute(() =>
            {
                var properties = new DependencyProperty[]
                {
                    AnimatedVisualPlayer.AutoPlayProperty,
                    AnimatedVisualPlayer.DiagnosticsProperty,
                    AnimatedVisualPlayer.DurationProperty,
                    AnimatedVisualPlayer.FallbackContentProperty,
                    AnimatedVisualPlayer.IsAnimatedVisualLoadedProperty,
                    AnimatedVisualPlayer.IsPlayingProperty,
                    AnimatedVisualPlayer.PlaybackRateProperty,
                    AnimatedVisualPlayer.SourceProperty,
                    AnimatedVisualPlayer.StretchProperty,
                    AnimatedVisualPlayer.AnimationOptimizationProperty,
                };
                VerifyIdentifiersAreDistinct(properties);
                Verify.IsTrue(ReferenceEquals(AnimatedVisualPlayer.SourceProperty, AnimatedVisualPlayer.SourceProperty), "Identifiers are stable.");
                Verify.IsTrue(ReferenceEquals(AnimatedVisualPlayer.AnimationOptimizationProperty, AnimatedVisualPlayer.AnimationOptimizationProperty), "Identifiers are stable.");

                var player = new AnimatedVisualPlayer();

                // Read-only properties: default value through both surfaces.
                Verify.IsNull(player.Diagnostics);
                Verify.IsNull(player.GetValue(AnimatedVisualPlayer.DiagnosticsProperty));
                Verify.AreEqual(TimeSpan.Zero, player.Duration);
                Verify.AreEqual(TimeSpan.Zero, (TimeSpan)player.GetValue(AnimatedVisualPlayer.DurationProperty));
                Verify.IsFalse(player.IsAnimatedVisualLoaded);
                Verify.IsFalse((bool)player.GetValue(AnimatedVisualPlayer.IsAnimatedVisualLoadedProperty));
                Verify.IsFalse(player.IsPlaying);
                Verify.IsFalse((bool)player.GetValue(AnimatedVisualPlayer.IsPlayingProperty));

                VerifyDependencyPropertyRoundTrip(player, AnimatedVisualPlayer.AutoPlayProperty, () => player.AutoPlay, v => player.AutoPlay = v, true, false);
                VerifyDependencyPropertyRoundTrip(player, AnimatedVisualPlayer.PlaybackRateProperty, () => player.PlaybackRate, v => player.PlaybackRate = v, 1.0, -0.5);
                VerifyDependencyPropertyRoundTrip(player, AnimatedVisualPlayer.StretchProperty, () => player.Stretch, v => player.Stretch = v, Stretch.Uniform, Stretch.UniformToFill);
                VerifyDependencyPropertyRoundTrip(player, AnimatedVisualPlayer.AnimationOptimizationProperty, () => player.AnimationOptimization, v => player.AnimationOptimization = v, PlayerAnimationOptimization.Latency, PlayerAnimationOptimization.Resources);
                VerifyDependencyPropertyRoundTrip(player, AnimatedVisualPlayer.FallbackContentProperty, () => player.FallbackContent, v => player.FallbackContent = v, null, CreateFallbackTemplate("Fallback", 10, 10));

                // Source: AutoPlay is turned off so loading content has no play side effects.
                player.AutoPlay = false;
                var source = new AvpTestSource();
                VerifyDependencyPropertyRoundTrip<IAnimatedVisualSource>(player, AnimatedVisualPlayer.SourceProperty, () => player.Source, v => player.Source = v, null, source);
                Verify.AreEqual(2, source.CreateCount, "Setting Source through the DP and through the CLR property should each load content.");
                Verify.IsFalse(player.IsAnimatedVisualLoaded, "Clearing Source unloads the content.");

                player.SetValue(AnimatedVisualPlayer.SourceProperty, source);
                Verify.IsTrue(player.IsAnimatedVisualLoaded, "Source set through the DP should load content without the player being in the tree.");
                Verify.AreEqual(source.Duration, (TimeSpan)player.GetValue(AnimatedVisualPlayer.DurationProperty), "Duration read through the DP reflects the loaded content.");
                Verify.IsTrue((bool)player.GetValue(AnimatedVisualPlayer.IsAnimatedVisualLoadedProperty));
                player.ClearValue(AnimatedVisualPlayer.SourceProperty);
                Verify.IsFalse(player.IsAnimatedVisualLoaded);
                Verify.AreEqual(TimeSpan.Zero, player.Duration);
                player.ClearValue(AnimatedVisualPlayer.AutoPlayProperty);
                Verify.IsTrue(player.AutoPlay);
            });
        }

        // Scenario: construct an AnimatedVisualPlayerAutomationPeer directly (as a derived control or custom peer
        //           would), and ask the framework for the player's peer.
        // Expected: both peers belong to the player, report the AnimatedVisualPlayer class name and the Image control
        //           type, and the framework returns an AnimatedVisualPlayerAutomationPeer.
        [TestMethod]
        public void AutomationPeerCreatedDirectlyAndByTheFramework()
        {
            RunOnUIThread.Execute(() =>
            {
                var player = CreatePlayer();

                var peer = new AnimatedVisualPlayerAutomationPeer(player);
                Verify.IsTrue(ReferenceEquals(player, peer.Owner), "The peer owner should be the player passed to the constructor.");
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer", peer.GetClassName());
                Verify.AreEqual(AutomationControlType.Image, peer.GetAutomationControlType());

                var frameworkPeer = FrameworkElementAutomationPeer.CreatePeerForElement(player);
                Verify.IsTrue(frameworkPeer is AnimatedVisualPlayerAutomationPeer, "The player should create an AnimatedVisualPlayerAutomationPeer.");
                Verify.IsTrue(ReferenceEquals(player, ((FrameworkElementAutomationPeer)frameworkPeer).Owner));
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer", frameworkPeer.GetClassName());
                Verify.AreEqual(AutomationControlType.Image, frameworkPeer.GetAutomationControlType());
                Verify.IsTrue(ReferenceEquals(frameworkPeer, FrameworkElementAutomationPeer.CreatePeerForElement(player)), "The framework peer is created once and reused.");
            });
        }

        // Scenario: a player in its own window plays a non-looped animation while the window is hidden and shown
        //           (AppWindow.Hide/Show flips XamlRoot.IsHostVisible and raises XamlRoot.Changed).
        // Expected: (a) while hidden the play does not progress to completion and IsPlaying stays true; (b) once shown
        //           it completes; (c) a play paused before hiding is not resumed by showing, only by Resume(); (d) a
        //           Resume() while hidden does not restart the play until the window is shown.
        [TestMethod]
        public void HidingTheHostWindowPausesPlayUntilTheWindowIsShown()
        {
            using (var host = new PlayerWindowHost(() =>
            {
                var player = CreatePlayer();
                player.AutoPlay = false;
                player.Source = new AvpTestSource() { Duration = HiddenPlayDuration };
                return player;
            }))
            {
                var player = host.Player;
                Task play = null;

                // The player acts only on IsHostVisible transitions it observes through XamlRoot.Changed, and it starts
                // out assuming the host is not visible. Resize the window so that the player observes the visible state
                // before the window is hidden. (Without this, the first hide after loading is not seen as a transition
                // and does not pause the play.)
                Verify.IsTrue(host.RunAndWaitForHostVisibility(() => host.ResizeBy(40), expectedVisible: true), "Resizing the visible window should raise XamlRoot.Changed.");

                // (a) A play started just before hiding does not progress while hidden.
                Verify.IsTrue(host.RunAndWaitForHostVisibility(() =>
                {
                    play = player.PlayAsync(0, 1, false).AsTask();
                    Verify.IsTrue(player.IsPlaying);
                    host.Window.AppWindow.Hide();
                }, expectedVisible: false), "Hiding the window should make XamlRoot.IsHostVisible false.");
                Verify.IsFalse(play.Wait(HiddenPlayDuration * 2), "A play must not progress to completion while the window is hidden.");
                RunOnUIThread.Execute(() => Verify.IsTrue(player.IsPlaying, "The play is still in progress while hidden."));

                // (b) Showing the window resumes the play, which then completes.
                Verify.IsTrue(host.RunAndWaitForHostVisibility(() => host.Window.AppWindow.Show(), expectedVisible: true), "Showing the window should make XamlRoot.IsHostVisible true.");
                Verify.IsTrue(play.Wait(CompletionTimeoutMs), "The play should resume and complete once the window is shown.");
                RunOnUIThread.Execute(() => Verify.IsFalse(player.IsPlaying));

                // (c) A play paused before hiding stays paused after the window is shown again.
                Verify.IsTrue(host.RunAndWaitForHostVisibility(() =>
                {
                    play = player.PlayAsync(0, 1, false).AsTask();
                    player.Pause();
                    host.Window.AppWindow.Hide();
                }, expectedVisible: false));
                Verify.IsTrue(host.RunAndWaitForHostVisibility(() => host.Window.AppWindow.Show(), expectedVisible: true));
                Verify.IsFalse(play.Wait(HiddenPlayDuration * 2), "Showing the window must not resume a play paused by the app.");
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(player.IsPlaying);
                    player.Resume();
                });
                Verify.IsTrue(play.Wait(CompletionTimeoutMs), "Resume should continue the play to completion.");

                // (d) Pause and Resume while hidden: the play stays paused until the window is shown.
                Verify.IsTrue(host.RunAndWaitForHostVisibility(() =>
                {
                    play = player.PlayAsync(0, 1, false).AsTask();
                    host.Window.AppWindow.Hide();
                }, expectedVisible: false));
                RunOnUIThread.Execute(() =>
                {
                    player.Pause();
                    player.Resume();
                });
                Verify.IsFalse(play.Wait(HiddenPlayDuration * 2), "Resume while hidden must not restart the play before the window is shown.");
                Verify.IsTrue(host.RunAndWaitForHostVisibility(() => host.Window.AppWindow.Show(), expectedVisible: true));
                Verify.IsTrue(play.Wait(CompletionTimeoutMs), "The play should complete once the window is shown.");
                RunOnUIThread.Execute(() => Verify.IsFalse(player.IsPlaying));
            }
        }

        // Scenario: an unparented player (never loaded, so never unloaded) loads content and starts a looped play,
        //           then the app releases every reference to the player.
        // Expected: while the player is referenced its looped play stays pending; once the player is released and
        //           collected, its destructor completes the pending play.
        [TestMethod]
        public void ReleasingAPlayingPlayerCompletesItsPlay()
        {
            var holder = new object[1];
            Task play = StartLoopedPlayOnUnparentedPlayer(holder);

            CollectGarbage();
            Verify.IsFalse(play.Wait(ShortDuration * 2), "A looped play of a referenced player does not complete on its own.");
            GC.KeepAlive(holder);

            holder[0] = null;
            Verify.IsTrue(WaitForCompletionAfterCollection(play), "Releasing the player should complete its pending play.");
        }

        // Scenario: get the activation factory of each AnimatedVisualPlayer/ProgressBar/ProgressRing class and of
        //           their automation peers directly from Microsoft.UI.Xaml.Controls.dll (DllGetActivationFactory).
        // Expected: each factory reports its class name through IInspectable::GetRuntimeClassName, and
        //           IActivationFactory::ActivateInstance returns E_NOTIMPL because these composable classes are created
        //           only through their factory interface's CreateInstance.
        [TestMethod]
        public void ActivationFactoriesReportClassNameAndRejectDefaultActivation()
        {
            var classNames = new string[]
            {
                "Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer",
                "Microsoft.UI.Xaml.Controls.ProgressBar",
                "Microsoft.UI.Xaml.Controls.ProgressRing",
                "Microsoft.UI.Xaml.Automation.Peers.AnimatedVisualPlayerAutomationPeer",
                "Microsoft.UI.Xaml.Automation.Peers.ProgressBarAutomationPeer",
                "Microsoft.UI.Xaml.Automation.Peers.ProgressRingAutomationPeer",
            };

            RunOnUIThread.Execute(() =>
            {
                // Creating a control guarantees that the controls DLL is loaded in the process.
                Verify.IsNotNull(new ProgressBar());
                IntPtr module = GetModuleHandleW("Microsoft.UI.Xaml.Controls.dll");
                Verify.AreNotEqual(IntPtr.Zero, module, "Microsoft.UI.Xaml.Controls.dll should be loaded.");
                IntPtr getActivationFactory = NativeLibrary.GetExport(module, "DllGetActivationFactory");

                foreach (var className in classNames)
                {
                    Log.Comment($"Activation factory for {className}");
                    VerifyActivationFactory(getActivationFactory, className);
                }
            });
        }

        private static AnimatedVisualPlayer CreatePlayer()
        {
            return new AnimatedVisualPlayer()
            {
                UseLayoutRounding = false,
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top,
            };
        }

        private void LoadInHost(FrameworkElement element)
        {
            LoadInHost(() => element, element);
        }

        private void LoadInHost(Func<FrameworkElement> createRoot, FrameworkElement elementToWaitFor)
        {
            var loaded = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                elementToWaitFor.Loaded += (sender, args) => loaded.Set();
                var root = createRoot();
                var host = root as Panel ?? new Grid() { Children = { root } };
                host.UseLayoutRounding = false;
                Content = host;
                Content.UpdateLayout();
            });
            TestUtilities.WaitForEvent(loaded);
            IdleSynchronizer.Wait();
        }

        private static DataTemplate CreateFallbackTemplate(string tag, double width, double height)
        {
            return (DataTemplate)XamlReader.Load(
                "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                $"<Border Tag='{tag}' Width='{width}' Height='{height}' Background='Red'/>" +
                "</DataTemplate>");
        }

        private static FrameworkElement GetSingleFallbackChild(AnimatedVisualPlayer player)
        {
            Verify.AreEqual(1, VisualTreeHelper.GetChildrenCount(player), "Expected exactly one fallback element.");
            var child = VisualTreeHelper.GetChild(player, 0) as FrameworkElement;
            Verify.IsNotNull(child);
            return child;
        }

        private static void VerifyMeasure(AnimatedVisualPlayer player, Stretch stretch, Size available, Size expected)
        {
            player.Stretch = stretch;
            player.InvalidateMeasure();
            player.Measure(available);
            Log.Comment($"Stretch={stretch} available={available} desired={player.DesiredSize}");
            Verify.IsLessThan(Math.Abs(expected.Width - player.DesiredSize.Width), 0.01, $"Desired width for Stretch={stretch}, available={available}");
            Verify.IsLessThan(Math.Abs(expected.Height - player.DesiredSize.Height), 0.01, $"Desired height for Stretch={stretch}, available={available}");
        }

        private void ArrangeWithStretch(AnimatedVisualPlayer player, Stretch stretch)
        {
            player.Stretch = stretch;
            player.InvalidateMeasure();
            player.InvalidateArrange();
            Content.UpdateLayout();
        }

        private static void VerifyVector3(Vector3 expected, Vector3 actual, string message)
        {
            Log.Comment($"{message}: expected {expected}, actual {actual}");
            Verify.IsLessThan(Vector3.Distance(expected, actual), 0.01f, message);
        }

        private static void VerifyVector2(Vector2 expected, Vector2 actual, string message)
        {
            Log.Comment($"{message}: expected {expected}, actual {actual}");
            Verify.IsLessThan(Vector2.Distance(expected, actual), 0.01f, message);
        }

        private static float GetProgress(AnimatedVisualPlayer player)
        {
            var progressPropertySet = (CompositionPropertySet)player.ProgressObject;
            var status = progressPropertySet.TryGetScalar("Progress", out float value);
            Verify.AreEqual(CompositionGetValueStatus.Succeeded, status);
            return value;
        }

        // Polls a UI-thread condition. Used for effects the player applies after a compositor commit
        // (DestroyAnimations runs from a RequestCommitAsync completion).
        private static void WaitForCondition(Func<bool> condition, string message)
        {
            var stopwatch = Stopwatch.StartNew();
            while (true)
            {
                bool satisfied = false;
                RunOnUIThread.Execute(() => satisfied = condition());
                if (satisfied)
                {
                    return;
                }

                if (stopwatch.ElapsedMilliseconds > CompletionTimeoutMs)
                {
                    Verify.Fail(message);
                    return;
                }

                Thread.Sleep(50);
            }
        }

        private static void WaitForCompositorCommit(UIElement element)
        {
            Task commit = null;
            RunOnUIThread.Execute(() => commit = ElementCompositionPreview.GetElementVisual(element).Compositor.RequestCommitAsync().AsTask());
            Verify.IsTrue(commit.Wait(CompletionTimeoutMs), "Waiting for a compositor commit.");
            IdleSynchronizer.Wait();
        }

        private static void VerifyIdentifiersAreDistinct(DependencyProperty[] properties)
        {
            for (int i = 0; i < properties.Length; i++)
            {
                Verify.IsNotNull(properties[i], "Dependency property identifier #" + i + " should not be null");
                for (int j = 0; j < i; j++)
                {
                    Verify.IsFalse(ReferenceEquals(properties[i], properties[j]), "Dependency property identifiers #" + j + " and #" + i + " should be distinct");
                }
            }
        }

        private static void VerifyDependencyPropertyRoundTrip<T>(DependencyObject owner, DependencyProperty property, Func<T> getClrValue, Action<T> setClrValue, T defaultValue, T newValue)
        {
            Verify.AreEqual(defaultValue, getClrValue(), "CLR default");
            Verify.AreEqual(defaultValue, (T)owner.GetValue(property), "GetValue default");

            owner.SetValue(property, newValue);
            Verify.AreEqual(newValue, getClrValue(), "A value set through the DP should be visible through the CLR property");
            owner.ClearValue(property);
            Verify.AreEqual(defaultValue, getClrValue(), "ClearValue should restore the default");

            setClrValue(newValue);
            Verify.AreEqual(newValue, (T)owner.GetValue(property), "A value set through the CLR property should be visible through the DP");
            owner.ClearValue(property);
            Verify.AreEqual(defaultValue, getClrValue(), "ClearValue should restore the default");
            Verify.AreEqual(defaultValue, (T)owner.GetValue(property), "ClearValue should restore the default");
        }

        // Starts a looped play on a player that is never added to the tree. The only reference to the player is
        // stored in holder[0], so the caller controls when it becomes unreachable.
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static Task StartLoopedPlayOnUnparentedPlayer(object[] holder)
        {
            Task play = null;
            RunOnUIThread.Execute(() =>
            {
                var player = CreatePlayer();
                player.AutoPlay = false;
                player.Source = new AvpTestSource();
                Verify.IsTrue(player.IsAnimatedVisualLoaded, "Content loads without the player being in the tree.");
                play = player.PlayAsync(0, 1, true).AsTask();
                Verify.IsTrue(player.IsPlaying);
                holder[0] = player;
            });
            return play;
        }

        private static void CollectGarbage()
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
        }

        // Collects garbage until the play completes. The player's final release happens on the UI thread after its
        // wrapper is finalized, so each collection is followed by a short wait that leaves the UI thread free.
        private static bool WaitForCompletionAfterCollection(Task play)
        {
            var stopwatch = Stopwatch.StartNew();
            while (stopwatch.ElapsedMilliseconds < CompletionTimeoutMs)
            {
                CollectGarbage();
                if (play.Wait(100))
                {
                    return true;
                }
            }

            return play.IsCompleted;
        }

        private static unsafe void VerifyActivationFactory(IntPtr getActivationFactory, string className)
        {
            const int E_NOTIMPL = unchecked((int)0x80004001);
            IntPtr classId = IntPtr.Zero;
            IntPtr factory = IntPtr.Zero;
            IntPtr runtimeClassName = IntPtr.Zero;
            IntPtr instance = IntPtr.Zero;
            try
            {
                Verify.AreEqual(0, WindowsCreateString(className, className.Length, out classId));
                int hr = ((delegate* unmanaged<IntPtr, IntPtr*, int>)getActivationFactory)(classId, &factory);
                Verify.AreEqual(0, hr, $"DllGetActivationFactory({className}) HRESULT");
                Verify.AreNotEqual(IntPtr.Zero, factory);

                // IInspectable: 3 GetIids, 4 GetRuntimeClassName, 5 GetTrustLevel; IActivationFactory: 6 ActivateInstance.
                IntPtr* vtable = *(IntPtr**)factory;
                hr = ((delegate* unmanaged<IntPtr, IntPtr*, int>)vtable[4])(factory, &runtimeClassName);
                Verify.AreEqual(0, hr, "GetRuntimeClassName HRESULT");
                uint length;
                char* buffer = WindowsGetStringRawBuffer(runtimeClassName, &length);
                Verify.AreEqual(className, new string(buffer, 0, (int)length), "The factory reports the runtime class name.");

                hr = ((delegate* unmanaged<IntPtr, IntPtr*, int>)vtable[6])(factory, &instance);
                Verify.AreEqual(E_NOTIMPL, hr, "ActivateInstance HRESULT");
                Verify.AreEqual(IntPtr.Zero, instance, "ActivateInstance must not return an instance.");
            }
            finally
            {
                if (instance != IntPtr.Zero)
                {
                    Marshal.Release(instance);
                }
                if (factory != IntPtr.Zero)
                {
                    Marshal.Release(factory);
                }
                if (runtimeClassName != IntPtr.Zero)
                {
                    WindowsDeleteString(runtimeClassName);
                }
                if (classId != IntPtr.Zero)
                {
                    WindowsDeleteString(classId);
                }
            }
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern IntPtr GetModuleHandleW(string moduleName);

        [DllImport("combase.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern int WindowsCreateString(string sourceString, int length, out IntPtr hstring);

        [DllImport("combase.dll", ExactSpelling = true)]
        private static extern int WindowsDeleteString(IntPtr hstring);

        [DllImport("combase.dll", ExactSpelling = true)]
        private static extern unsafe char* WindowsGetStringRawBuffer(IntPtr hstring, uint* length);

        // Hosts a player in a dedicated window so that hiding and showing never affect the shared test window.
        private sealed class PlayerWindowHost : IDisposable
        {
            public Window Window { get; private set; }
            public AnimatedVisualPlayer Player { get; private set; }

            public PlayerWindowHost(Func<AnimatedVisualPlayer> createPlayer)
            {
                var loaded = new AutoResetEvent(false);
                try
                {
                    RunOnUIThread.Execute(() =>
                    {
                        Window = new Window() { Title = "AnimatedVisualPlayer host" };
                        Player = createPlayer();
                        Player.Loaded += (sender, args) => loaded.Set();
                        Window.Content = new Grid() { Children = { Player } };
                        Window.Activate();
                    });
                    Verify.IsTrue(loaded.WaitOne(ApiTestBase.DefaultWaitTimeInMS), "Waiting for the player to load in its host window.");
                    IdleSynchronizer.Wait();
                }
                catch
                {
                    // The caller's using block never sees a host whose constructor threw, so close the window here.
                    Dispose();
                    throw;
                }
            }

            // Must be called on the UI thread.
            public void ResizeBy(int delta)
            {
                var size = Window.AppWindow.Size;
                Window.AppWindow.Resize(new SizeInt32(size.Width + delta, size.Height + delta));
            }

            // Runs action on the UI thread, then waits for the player's XamlRoot to raise Changed with
            // IsHostVisible == expectedVisible.
            public bool RunAndWaitForHostVisibility(Action action, bool expectedVisible)
            {
                var reached = new ManualResetEvent(false);
                XamlRoot xamlRoot = null;
                TypedEventHandler<XamlRoot, XamlRootChangedEventArgs> handler = (sender, args) =>
                {
                    if (sender.IsHostVisible == expectedVisible)
                    {
                        reached.Set();
                    }
                };

                RunOnUIThread.Execute(() =>
                {
                    xamlRoot = Player.XamlRoot;
                    xamlRoot.Changed += handler;
                    action();
                });
                bool result = reached.WaitOne(HostVisibilityTimeoutMs);
                RunOnUIThread.Execute(() => xamlRoot.Changed -= handler);
                return result;
            }

            public void Dispose()
            {
                RunOnUIThread.Execute(() =>
                {
                    Window?.Close();
                    MUXControlsTestApp.App.CurrentWindow.Activate();
                });
                IdleSynchronizer.Wait();
            }
        }
    }
}
