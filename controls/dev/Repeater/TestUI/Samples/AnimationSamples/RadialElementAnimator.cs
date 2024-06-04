// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Numerics;
using Windows.Foundation;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Media.Animation;
using System.Linq;

namespace MUXControlsTestApp.Samples
{
    public class RadialItemCollectionTransitionProvider : ItemCollectionTransitionProvider
    {
        private const double DefaultAnimationDurationInMs = 600.0;

        private static double _animationSlowdownFactor = 1.0;
        public static double AnimationSlowdownFactor
        {
            get
            {
                return _animationSlowdownFactor;
            }
            set
            {
                _animationSlowdownFactor = value;
            }
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

            StartAddTransitions(addTransitions);
            StartRemoveTransitions(removeTransitions);
            StartMoveTransitions(moveTransitions);
        }

        private static void StartAddTransitions(IList<ItemCollectionTransition> transitions)
        {
            foreach (var transition in transitions)
            {
                var progress = transition.Start();
                var visual = ElementCompositionPreview.GetElementVisual(progress.Element);
                var compositor = visual.Compositor;

                var parent = (FrameworkElement)VisualTreeHelper.GetParent(progress.Element);
                var to = LayoutInformation.GetLayoutSlot((FrameworkElement)progress.Element);
                var from = to;
                from.X = (parent.ActualWidth - ((FrameworkElement)progress.Element).ActualWidth) / 2;
                from.Y = (parent.ActualHeight - ((FrameworkElement)progress.Element).ActualHeight) / 2;

                var batch = compositor.CreateScopedBatch(CompositionBatchTypes.Animation);
                AnimateOffset(visual, compositor, from, to);
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
                batch.Completed += delegate { progress.Complete(); };
            }
        }

        private static void StartMoveTransitions(IList<ItemCollectionTransition> transitions)
        {
            foreach (var transition in transitions)
            {
                var progress = transition.Start();
                var outerVisual = ElementCompositionPreview.GetElementVisual(progress.Element);
                var innerVisual = ElementCompositionPreview.GetElementVisual((UIElement)VisualTreeHelper.GetChild(progress.Element, 0));
                var compositor = outerVisual.Compositor;

                var parent = (FrameworkElement)VisualTreeHelper.GetParent(progress.Element);
                var center = new Point(parent.ActualWidth / 2, parent.ActualHeight / 2);
                var from = new Point((transition.OldBounds.Left + transition.OldBounds.Right) / 2 - center.X, -(transition.OldBounds.Top + transition.OldBounds.Bottom) / 2 + center.Y);
                var to = new Point((transition.NewBounds.Left + transition.NewBounds.Right) / 2 - center.X, -(transition.NewBounds.Top + transition.NewBounds.Bottom) / 2 + center.Y);
                var fromLength = Math.Sqrt(from.X * from.X + from.Y * from.Y);
                var toLength = Math.Sqrt(to.X * to.X + to.Y * to.Y);
                from.X /= fromLength;
                from.Y /= fromLength;
                to.X /= toLength;
                to.Y /= toLength;

                var angle = Math.Acos(Math.Min(1, to.X * from.X + to.Y * from.Y));
                var sign = Math.Sign(to.X * from.Y - to.Y * from.X);
                if (sign == 0) sign = Math.Sign(from.Y - to.Y);
                if (double.IsNaN(angle)) angle = 0;

                outerVisual.CenterPoint = new Vector3((float)(center.X - transition.NewBounds.X), (float)(center.Y - transition.NewBounds.Y), 0);
                innerVisual.CenterPoint = new Vector3((float)(transition.NewBounds.Width / 2), (float)(transition.NewBounds.Height / 2), 0);

                var outerRotate = compositor.CreateScalarKeyFrameAnimation();
                {
                    outerRotate.InsertExpressionKeyFrame(0.0f, (-sign * angle).ToString());
                    outerRotate.InsertKeyFrame(1.0f, 0.0f);
                    outerRotate.Duration = TimeSpan.FromMilliseconds(DefaultAnimationDurationInMs * AnimationSlowdownFactor);
                }

                var innerRotate = compositor.CreateScalarKeyFrameAnimation();
                {
                    innerRotate.InsertExpressionKeyFrame(0.0f, (sign * angle).ToString());
                    innerRotate.InsertKeyFrame(1.0f, 0.0f);
                    innerRotate.Duration = TimeSpan.FromMilliseconds(DefaultAnimationDurationInMs * AnimationSlowdownFactor);
                }

                var batch = compositor.CreateScopedBatch(CompositionBatchTypes.Animation);
                outerVisual.StartAnimation("RotationAngle", outerRotate);
                innerVisual.StartAnimation("RotationAngle", innerRotate);
                batch.End();
                batch.Completed += delegate { progress.Complete(); };
            }
        }

        private static void AnimateOffset(Visual visual, Compositor compositor, Rect oldBounds, Rect newBounds)
        {
            var offsetAnimation = compositor.CreateVector2KeyFrameAnimation();

            offsetAnimation.SetVector2Parameter("delta", new Vector2(
                (float)(oldBounds.X - newBounds.X),
                (float)(oldBounds.Y - newBounds.Y)));
            offsetAnimation.SetVector2Parameter("final", new Vector2());
            offsetAnimation.InsertExpressionKeyFrame(0.0f, "this.CurrentValue + delta");
            offsetAnimation.InsertExpressionKeyFrame(1.0f, "final");
            offsetAnimation.Duration = TimeSpan.FromMilliseconds(
                DefaultAnimationDurationInMs * AnimationSlowdownFactor);

            visual.StartAnimation("TransformMatrix._41_42", offsetAnimation);
        }
    }
}
