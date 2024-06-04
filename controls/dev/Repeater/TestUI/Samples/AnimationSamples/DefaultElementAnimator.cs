// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Numerics;
using Windows.Foundation;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using System.Collections.Generic;
using System.Xml.Linq;
using System.Linq;

namespace MUXControlsTestApp.Utils
{
    // TODO: port to native.
    public class DefaultItemCollectionTransitionProvider : ItemCollectionTransitionProvider
    {
        private const double DefaultAnimationDurationInMs = 300.0;
        public static double AnimationSlowdownFactor { get; set; }

        static DefaultItemCollectionTransitionProvider()
        {
            AnimationSlowdownFactor = 1.0;
        }

        protected override bool ShouldAnimateCore(ItemCollectionTransition transition)
        {
            return true;
        }

        protected override void StartTransitions(IList<ItemCollectionTransition> transitions)
        {
            var addTransitions = transitions.Where(transition => transition.Operation == ItemCollectionTransitionOperation.Add).ToList();
            var removeTransitions = transitions.Where(transition => transition.Operation == ItemCollectionTransitionOperation.Remove).ToList();
            var moveTransitions = transitions.Where(transition => transition.Operation == ItemCollectionTransitionOperation.Move).ToList();

            StartAddTransitions(addTransitions, removeTransitions.Count > 0, moveTransitions.Count > 0);
            StartRemoveTransitions(removeTransitions);
            StartMoveTransitions(moveTransitions, removeTransitions.Count > 0);
        }

        private static void StartAddTransitions(IList<ItemCollectionTransition> transitions, bool hasRemoveTransitions, bool hasMoveTransitions)
        {
            foreach (var transition in transitions)
            {
                var progress = transition.Start();
                var visual = ElementCompositionPreview.GetElementVisual(progress.Element);
                var compositor = visual.Compositor;

                var fadeInAnimation = compositor.CreateScalarKeyFrameAnimation();
                fadeInAnimation.InsertKeyFrame(0.0f, 0.0f);

                if (hasMoveTransitions && hasRemoveTransitions)
                {
                    fadeInAnimation.InsertKeyFrame(0.66f, 0.0f);
                }
                else if (hasMoveTransitions || hasRemoveTransitions)
                {
                    fadeInAnimation.InsertKeyFrame(0.5f, 0.0f);
                }

                fadeInAnimation.InsertKeyFrame(1.0f, 1.0f);
                fadeInAnimation.Duration = TimeSpan.FromMilliseconds(
                    DefaultAnimationDurationInMs * ((hasRemoveTransitions ? 1 : 0) + (hasMoveTransitions ? 1 : 0) + 1) * AnimationSlowdownFactor);

                var batch = compositor.CreateScopedBatch(CompositionBatchTypes.Animation);
                visual.StartAnimation("Opacity", fadeInAnimation);
                batch.End();
                batch.Completed += delegate { progress.Complete(); };
            }
        }

        private static void StartRemoveTransitions(IList<ItemCollectionTransition> transitions)
        {
            foreach (var transition in transitions)
            {
                var progress = transition.Start();
                var visual = ElementCompositionPreview.GetElementVisual(progress.Element);
                var compositor = visual.Compositor;

                var fadeOutAnimation = compositor.CreateScalarKeyFrameAnimation();
                fadeOutAnimation.InsertExpressionKeyFrame(0.0f, "this.CurrentValue");
                fadeOutAnimation.InsertKeyFrame(1.0f, 0.0f);
                fadeOutAnimation.Duration = TimeSpan.FromMilliseconds(DefaultAnimationDurationInMs * AnimationSlowdownFactor);

                var batch = compositor.CreateScopedBatch(CompositionBatchTypes.Animation);
                visual.StartAnimation("Opacity", fadeOutAnimation);
                batch.End();
                batch.Completed += delegate {
                    visual.Opacity = 1.0f;
                    progress.Complete();
                };
            }
        }

        private static void StartMoveTransitions(IList<ItemCollectionTransition> transitions, bool hasRemoveAnimations)
        {
            foreach (var transition in transitions)
            {
                var progress = transition.Start();
                var visual = ElementCompositionPreview.GetElementVisual(progress.Element);
                var compositor = visual.Compositor;
                var batch = compositor.CreateScopedBatch(CompositionBatchTypes.Animation);

                // Animate offset.
                if (transition.OldBounds.X != transition.NewBounds.X ||
                    transition.OldBounds.Y != transition.NewBounds.Y)
                {
                    AnimateOffset(visual, compositor, transition.OldBounds, transition.NewBounds, hasRemoveAnimations);
                }

                batch.End();
                batch.Completed += delegate { progress.Complete(); };
            }
        }

        private static void AnimateOffset(Visual visual, Compositor compositor, Rect oldBounds, Rect newBounds, bool hasRemoveAnimations)
        {
            var offsetAnimation = compositor.CreateVector2KeyFrameAnimation();

            offsetAnimation.SetVector2Parameter("delta", new Vector2(
                (float)(oldBounds.X - newBounds.X),
                (float)(oldBounds.Y - newBounds.Y)));
            offsetAnimation.SetVector2Parameter("final", new Vector2());
            offsetAnimation.InsertExpressionKeyFrame(0.0f, "this.CurrentValue + delta");
            if (hasRemoveAnimations)
            {
                offsetAnimation.InsertExpressionKeyFrame(0.5f, "delta");
            }
            offsetAnimation.InsertExpressionKeyFrame(1.0f, "final");
            offsetAnimation.Duration = TimeSpan.FromMilliseconds(
                DefaultAnimationDurationInMs * ((hasRemoveAnimations ? 1 : 0) + 1) * AnimationSlowdownFactor);

            visual.StartAnimation("TransformMatrix._41_42", offsetAnimation);
        }
    }
}
