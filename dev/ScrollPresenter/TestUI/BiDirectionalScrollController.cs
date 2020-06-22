// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.Foundation;
using Windows.UI.Composition;
using Windows.UI.Input;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;

using IScrollController = Microsoft.UI.Xaml.Controls.Primitives.IScrollController;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ScrollInfo = Microsoft.UI.Xaml.Controls.ScrollInfo;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;
using ScrollControllerInteractionRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerInteractionRequestedEventArgs;
using ScrollControllerScrollToRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerScrollToRequestedEventArgs;
using ScrollControllerScrollByRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerScrollByRequestedEventArgs;
using ScrollControllerScrollFromRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerScrollFromRequestedEventArgs;

namespace MUXControlsTestApp.Utilities
{
    public class BiDirectionalScrollControllerScrollCompletedEventArgs
    {
        internal BiDirectionalScrollControllerScrollCompletedEventArgs(int offsetsChangeId)
        {
            OffsetsChangeId = offsetsChangeId;
        }

        public int OffsetsChangeId
        {
            get;
            set;
        }
    }

    public sealed class BiDirectionalScrollController : ContentControl
    {
        private class UniScrollControllerScrollCompletedEventArgs
        {
            public UniScrollControllerScrollCompletedEventArgs(int offsetChangeId)
            {
                OffsetChangeId = offsetChangeId;
            }

            public int OffsetChangeId
            {
                get;
                set;
            }
        }

        private class UniScrollController : IScrollController
        {
            public event TypedEventHandler<IScrollController, object> InteractionInfoChanged;
            public event TypedEventHandler<IScrollController, ScrollControllerInteractionRequestedEventArgs> InteractionRequested;
            public event TypedEventHandler<IScrollController, ScrollControllerScrollToRequestedEventArgs> ScrollToRequested;
            public event TypedEventHandler<IScrollController, ScrollControllerScrollByRequestedEventArgs> ScrollByRequested;
            public event TypedEventHandler<IScrollController, ScrollControllerScrollFromRequestedEventArgs> ScrollFromRequested;
            public event TypedEventHandler<IScrollController, UniScrollControllerScrollCompletedEventArgs> ScrollCompleted;

            public UniScrollController(BiDirectionalScrollController owner, Orientation orientation)
            {
                Owner = owner;
                Orientation = orientation;
                MinOffset = 0.0;
                MaxOffset = 0.0;
                Offset = 0.0;
                Viewport = 0.0;
            }

            public bool AreScrollControllerInteractionsAllowed
            {
                get;
                private set;
            }

            public bool AreScrollerInteractionsAllowed
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_AreScrollerInteractionsAllowed for Orientation=" + Orientation);
                    return Owner.AreScrollerInteractionsAllowed;
                }
            }

            public bool IsInteracting
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_IsInteracting for Orientation=" + Orientation);
                    return Owner.IsInteracting;
                }
            }

            public bool IsInteractionVisualRailEnabled
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_IsInteractionVisualRailEnabled for Orientation=" + Orientation);
                    return Owner.IsRailing;
                }
            }

            public Visual InteractionVisual
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_InteractionVisual for Orientation=" + Orientation);
                    return Owner.GetParentInteractionVisual();
                }
            }

            public Orientation InteractionVisualScrollOrientation
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_InteractionVisualScrollOrientation for Orientation=" + Orientation);
                    return Orientation;
                }
            }

            internal double MinOffset
            {
                get;
                private set;
            }

            internal double MaxOffset
            {
                get;
                private set;
            }

            internal double Offset
            {
                get;
                private set;
            }

            internal double Viewport
            {
                get;
                private set;
            }

            private ScrollMode ScrollMode
            {
                get;
                set;
            }

            private Orientation Orientation
            {
                get;
                set;
            }

            private BiDirectionalScrollController Owner
            {
                get;
                set;
            }

            private string MinOffsetPropertyName
            {
                get;
                set;
            }

            private string MaxOffsetPropertyName
            {
                get;
                set;
            }

            private string OffsetPropertyName
            {
                get;
                set;
            }

            private string MultiplierPropertyName
            {
                get;
                set;
            }

            private CompositionPropertySet ExpressionAnimationSources
            {
                get;
                set;
            }

            private ExpressionAnimation ThumbOffsetAnimation
            {
                get;
                set;
            }

            public void SetExpressionAnimationSources(
                CompositionPropertySet propertySet, string minOffsetPropertyName, string maxOffsetPropertyName, string offsetPropertyName, string multiplierPropertyName)
            {
                RaiseLogMessage(
                    "UniScrollController: SetExpressionAnimationSources for Orientation=" + Orientation +
                    " with minOffsetPropertyName=" + minOffsetPropertyName +
                    ", maxOffsetPropertyName=" + maxOffsetPropertyName +
                    ", offsetPropertyName=" + offsetPropertyName +
                    ", multiplierPropertyName=" + multiplierPropertyName);
                ExpressionAnimationSources = propertySet;
                if (ExpressionAnimationSources != null)
                {
                    MinOffsetPropertyName = minOffsetPropertyName.Trim();
                    MaxOffsetPropertyName = maxOffsetPropertyName.Trim();
                    OffsetPropertyName = offsetPropertyName.Trim();
                    MultiplierPropertyName = multiplierPropertyName.Trim();

                    UpdateInteractionVisualScrollMultiplier();

                    if (ThumbOffsetAnimation == null)
                    {
                        EnsureThumbAnimation();
                        StartThumbAnimation();
                    }
                }
                else
                {
                    MinOffsetPropertyName =
                    MaxOffsetPropertyName =
                    OffsetPropertyName =
                    MultiplierPropertyName = string.Empty;

                    ThumbOffsetAnimation = null;
                    StopThumbAnimation();
                }
            }

            public void SetScrollMode(ScrollMode scrollMode)
            {
                RaiseLogMessage(
                    "UniScrollController: SetScrollMode for Orientation=" + Orientation +
                    " with scrollMode=" + scrollMode);
                ScrollMode = scrollMode;
                UpdateAreScrollControllerInteractionsAllowed();
            }

            public void SetValues(double minOffset, double maxOffset, double offset, double viewport)
            {
                RaiseLogMessage(
                    "UniScrollController: SetValues for Orientation=" + Orientation +
                    " with minOffset=" + minOffset +
                    ", maxOffset=" + maxOffset +
                    ", offset=" + offset +
                    ", viewport=" + viewport);

                if (maxOffset < minOffset)
                {
                    throw new ArgumentOutOfRangeException("maxOffset");
                }

                if (viewport < 0.0)
                {
                    throw new ArgumentOutOfRangeException("viewport");
                }

                offset = Math.Max(minOffset, offset);
                offset = Math.Min(maxOffset, offset);

                double offsetTarget = 0.0;

                if (Owner.OperationsCount == 0)
                {
                    offsetTarget = offset;
                }
                else
                {
                    offsetTarget = Math.Max(minOffset, offsetTarget);
                    offsetTarget = Math.Min(maxOffset, offsetTarget);
                }

                if (Orientation == Orientation.Horizontal)
                    Owner.OffsetTarget = new Point(offsetTarget, Owner.OffsetTarget.Y);
                else
                    Owner.OffsetTarget = new Point(Owner.OffsetTarget.X, offsetTarget);

                bool updateThumbSize =
                    MinOffset != minOffset ||
                    MaxOffset != maxOffset ||
                    Viewport != viewport;

                MinOffset = minOffset;
                Offset = offset;
                MaxOffset = maxOffset;
                Viewport = viewport;

                if (updateThumbSize && !Owner.UpdateThumbSize())
                {
                    UpdateInteractionVisualScrollMultiplier();
                }
            }

            public CompositionAnimation GetScrollAnimation(
                ScrollInfo info,
                Vector2 currentPosition,
                CompositionAnimation defaultAnimation)
            {
                RaiseLogMessage(
                    "UniScrollController: GetScrollAnimation for Orientation=" + Orientation +
                    " with OffsetsChangeId=" + info.OffsetsChangeId + ", currentPosition=" + currentPosition);
                return defaultAnimation;
            }

            public void OnScrollCompleted(
                ScrollInfo info)
            {
                RaiseLogMessage(
                    "UniScrollController: OnScrollCompleted for Orientation=" + Orientation +
                    " with OffsetsChangeId=" + info.OffsetsChangeId);

                ScrollCompleted?.Invoke(this, new UniScrollControllerScrollCompletedEventArgs(info.OffsetsChangeId));
            }

            internal bool UpdateAreScrollControllerInteractionsAllowed()
            {
                bool oldAreScrollControllerInteractionsAllowed = AreScrollControllerInteractionsAllowed;

                AreScrollControllerInteractionsAllowed = ScrollMode != ScrollMode.Disabled && Owner.IsEnabled;

                if (oldAreScrollControllerInteractionsAllowed != AreScrollControllerInteractionsAllowed)
                {
                    RaiseInteractionInfoChanged();
                    return true;
                }
                return false;
            }

            internal void UpdateInteractionVisualScrollMultiplier()
            {
                if (ExpressionAnimationSources != null && !string.IsNullOrWhiteSpace(MultiplierPropertyName))
                {
                    float interactionVisualScrollMultiplier = Owner.GetInteractionVisualScrollMultiplier(Orientation, MaxOffset, MinOffset);

                    RaiseLogMessage("UniScrollController: UpdateInteractionVisualScrollMultiplier for Orientation=" + Orientation + ", InteractionVisualScrollMultiplier=" + interactionVisualScrollMultiplier);
                    ExpressionAnimationSources.InsertScalar(MultiplierPropertyName, interactionVisualScrollMultiplier);
                }
            }

            internal void RaiseInteractionInfoChanged()
            {
                RaiseLogMessage("UniScrollController: RaiseInteractionInfoChanged for Orientation=" + Orientation);

                if (InteractionInfoChanged != null)
                {
                    InteractionInfoChanged(this, null);
                }
            }

            internal bool RaiseInteractionRequested(PointerPoint pointerPoint)
            {
                RaiseLogMessage("UniScrollController: RaiseInteractionRequested for Orientation=" + Orientation + " with pointerPoint=" + pointerPoint);

                if (InteractionRequested != null)
                {
                    ScrollControllerInteractionRequestedEventArgs args = new ScrollControllerInteractionRequestedEventArgs(pointerPoint);
                    InteractionRequested(this, args);
                    RaiseLogMessage("UniScrollController: RaiseInteractionRequested result Handled=" + args.Handled);
                    return args.Handled;
                }
                return false;
            }

            internal int RaiseScrollToRequested(
                double offset,
                AnimationMode animationMode)
            {
                RaiseLogMessage("UniScrollController: RaiseScrollToRequested for Orientation=" + Orientation + " with offset=" + offset + ", animationMode=" + animationMode);

                if (ScrollToRequested != null)
                {
                    ScrollControllerScrollToRequestedEventArgs e =
                        new ScrollControllerScrollToRequestedEventArgs(
                            offset,
                            new ScrollingScrollOptions(animationMode, SnapPointsMode.Ignore));
                    ScrollToRequested(this, e);
                    return e.Info.OffsetsChangeId;
                }
                return -1;
            }

            internal int RaiseScrollByRequested(
                double offsetDelta,
                AnimationMode animationMode)
            {
                RaiseLogMessage("UniScrollController: RaiseScrollByRequested for Orientation=" + Orientation + " with offsetDelta=" + offsetDelta + ", animationMode=" + animationMode);

                if (ScrollByRequested != null)
                {
                    ScrollControllerScrollByRequestedEventArgs e =
                        new ScrollControllerScrollByRequestedEventArgs(
                            offsetDelta,
                            new ScrollingScrollOptions(animationMode, SnapPointsMode.Ignore));
                    ScrollByRequested(this, e);
                    return e.Info.OffsetsChangeId;
                }
                return -1;
            }

            internal int RaiseScrollFromRequested(
                float offsetVelocity, float? inertiaDecayRate)
            {
                RaiseLogMessage("UniScrollController: RaiseScrollFromRequested for Orientation=" + Orientation + " with offsetVelocity=" + offsetVelocity + ", inertiaDecayRate=" + inertiaDecayRate);

                if (ScrollFromRequested != null)
                {
                    ScrollControllerScrollFromRequestedEventArgs e =
                        new ScrollControllerScrollFromRequestedEventArgs(
                            offsetVelocity,
                            inertiaDecayRate);
                    ScrollFromRequested(this, e);
                    return e.Info.OffsetsChangeId;
                }
                return -1;
            }

            private void EnsureThumbAnimation()
            {
                if (ThumbOffsetAnimation == null && ExpressionAnimationSources != null &&
                    !string.IsNullOrWhiteSpace(MultiplierPropertyName) &&
                    !string.IsNullOrWhiteSpace(OffsetPropertyName) &&
                    !string.IsNullOrWhiteSpace(MinOffsetPropertyName) &&
                    !string.IsNullOrWhiteSpace(MaxOffsetPropertyName))
                {
                    ThumbOffsetAnimation = ExpressionAnimationSources.Compositor.CreateExpressionAnimation();
                    ThumbOffsetAnimation.Expression =
                        "min(eas." + MaxOffsetPropertyName + ",max(eas." + MinOffsetPropertyName + ",eas." + OffsetPropertyName + "))/(-eas." + MultiplierPropertyName + ")";
                    ThumbOffsetAnimation.SetReferenceParameter("eas", ExpressionAnimationSources);
                }
            }

            private void StartThumbAnimation()
            {
                if (Owner.InteractionVisual != null && ThumbOffsetAnimation != null)
                {
                    if (Orientation == Orientation.Horizontal)
                    {
                        Owner.InteractionVisual.StartAnimation("Translation.X", ThumbOffsetAnimation);
                    }
                    else
                    {
                        Owner.InteractionVisual.StartAnimation("Translation.Y", ThumbOffsetAnimation);
                    }
                }
            }

            private void StopThumbAnimation()
            {
                if (Owner.InteractionVisual != null)
                {
                    if (Orientation == Orientation.Horizontal)
                    {
                        Owner.InteractionVisual.StopAnimation("Translation.X");
                    }
                    else
                    {
                        Owner.InteractionVisual.StopAnimation("Translation.Y");
                    }
                }
            }

            private void RaiseLogMessage(string message)
            {
                Owner.RaiseLogMessage(message);
            }
        }

        private struct OperationInfo
        {
            public int Id;
            public Point RelativeOffsetChange;
            public Point OffsetTarget;

            public OperationInfo(int id, Point relativeOffsetChange, Point offsetTarget) : this()
            {
                Id = id;
                RelativeOffsetChange = relativeOffsetChange;
                OffsetTarget = offsetTarget;
            }
        }

        private const float SmallChangeAdditionalVelocity = 144.0f;
        private const float SmallChangeInertiaDecayRate = 0.975f;

        private List<string> lstAsyncEventMessage = new List<string>();
        private List<int> lstViewChangeIds = new List<int>();
        private List<int> lstScrollToIds = new List<int>();
        private List<int> lstScrollByIds = new List<int>();
        private List<int> lstScrollFromIds = new List<int>();
        private Dictionary<int, OperationInfo> operations = new Dictionary<int, OperationInfo>();
        private UniScrollController horizontalScrollController = null;
        private UniScrollController verticalScrollController = null;
        private RepeatButton biDecrementRepeatButton = null;
        private RepeatButton biIncrementRepeatButton = null;
        private RepeatButton horizontalIncrementVerticalDecrementRepeatButton = null;
        private RepeatButton horizontalDecrementVerticalIncrementRepeatButton = null;
        private RepeatButton horizontalDecrementRepeatButton = null;
        private RepeatButton horizontalIncrementRepeatButton = null;
        private RepeatButton verticalDecrementRepeatButton = null;
        private RepeatButton verticalIncrementRepeatButton = null;
        private bool isThumbDragged = false;
        private bool isRailing = true;
        private Point preManipulationThumbOffset;

        public event TypedEventHandler<BiDirectionalScrollController, string> LogMessage;
        public event TypedEventHandler<BiDirectionalScrollController, BiDirectionalScrollControllerScrollCompletedEventArgs> ScrollCompleted;

        public BiDirectionalScrollController()
        {
            this.DefaultStyleKey = typeof(BiDirectionalScrollController);
            AreScrollerInteractionsAllowed = true;
            this.horizontalScrollController = new UniScrollController(this, Orientation.Horizontal);
            this.verticalScrollController = new UniScrollController(this, Orientation.Vertical);

            this.horizontalScrollController.ScrollCompleted += UniScrollController_ScrollCompleted;
            this.verticalScrollController.ScrollCompleted += UniScrollController_ScrollCompleted;

            IsEnabledChanged += BiDirectionalScrollController_IsEnabledChanged;
            SizeChanged += BiDirectionalScrollController_SizeChanged;
        }

        public bool IsRailing
        {
            get
            {
                return isRailing;
            }
            set
            {
                if (isRailing != value)
                {
                    isRailing = value;

                    if (Thumb != null)
                    {
                        Thumb.ManipulationMode = ManipulationModes.TranslateX | ManipulationModes.TranslateY;
                        if (IsRailing)
                        {
                            Thumb.ManipulationMode |= ManipulationModes.TranslateRailsX | ManipulationModes.TranslateRailsY;
                        }
                    }

                    RaiseInteractionInfoChanged();
                }
            }
        }

        public IScrollController HorizontalScrollController
        {
            get
            {
                return horizontalScrollController;
            }
        }

        public IScrollController VerticalScrollController
        {
            get
            {
                return verticalScrollController;
            }
        }

        public int ScrollTo(double horizontalOffset, double verticalOffset, AnimationMode animationMode)
        {
            int viewChangeId = RaiseScrollToRequested(
                new Point(horizontalOffset, verticalOffset),
                animationMode, false /*hookupCompletion*/);
            lstViewChangeIds.Add(viewChangeId);
            return viewChangeId;
        }

        public int ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, AnimationMode animationMode)
        {
            int viewChangeId = RaiseScrollByRequested(
                new Point(horizontalOffsetDelta, verticalOffsetDelta),
                animationMode, false /*hookupCompletion*/);
            lstViewChangeIds.Add(viewChangeId);
            return viewChangeId;
        }

        public int ScrollFrom(Vector2 offsetVelocity, Vector2? inertiaDecayRate)
        {
            int viewChangeId = RaiseScrollFromRequested(
                offsetVelocity, inertiaDecayRate, false /*hookupCompletion*/);
            lstViewChangeIds.Add(viewChangeId);
            return viewChangeId;
        }

        protected override void OnApplyTemplate()
        {
            UnhookHandlers();

            base.OnApplyTemplate();

            Thumb = GetTemplateChild("Thumb") as FrameworkElement;
            biDecrementRepeatButton = GetTemplateChild("BiDecrementRepeatButton") as RepeatButton;
            biIncrementRepeatButton = GetTemplateChild("BiIncrementRepeatButton") as RepeatButton;
            horizontalIncrementVerticalDecrementRepeatButton = GetTemplateChild("HorizontalIncrementVerticalDecrementRepeatButton") as RepeatButton;
            horizontalDecrementVerticalIncrementRepeatButton = GetTemplateChild("HorizontalDecrementVerticalIncrementRepeatButton") as RepeatButton;
            horizontalDecrementRepeatButton = GetTemplateChild("HorizontalDecrementRepeatButton") as RepeatButton;
            horizontalIncrementRepeatButton = GetTemplateChild("HorizontalIncrementRepeatButton") as RepeatButton;
            verticalDecrementRepeatButton = GetTemplateChild("VerticalDecrementRepeatButton") as RepeatButton;
            verticalIncrementRepeatButton = GetTemplateChild("VerticalIncrementRepeatButton") as RepeatButton;

            if (Thumb != null)
            {
                Thumb.ManipulationMode = ManipulationModes.TranslateX | ManipulationModes.TranslateY;
                if (IsRailing)
                {
                    Thumb.ManipulationMode |= ManipulationModes.TranslateRailsX | ManipulationModes.TranslateRailsY;
                }

                InteractionVisual = ElementCompositionPreview.GetElementVisual(Thumb);
                ElementCompositionPreview.SetIsTranslationEnabled(Thumb, true);
            }
            else
            {
                InteractionVisual = null;
            }

            HookHandlers();

            RaiseInteractionInfoChanged();
        }

        internal Visual GetParentInteractionVisual()
        {
            RaiseLogMessage("BiDirectionalScrollController: GetParentInteractionVisual");
            return (Thumb != null && Thumb.Parent != null) ?
                ElementCompositionPreview.GetElementVisual(Thumb.Parent as UIElement) : null;
        }

        internal bool AreScrollerInteractionsAllowed
        {
            get;
            private set;
        }

        internal bool IsInteracting
        {
            get;
            private set;
        }

        internal Visual InteractionVisual
        {
            get;
            private set;
        }

        internal Point OffsetTarget
        {
            get;
            set;
        }

        private FrameworkElement Thumb
        {
            get;
            set;
        }

        private ExpressionAnimation HorizontalThumbOffsetAnimation
        {
            get;
            set;
        }

        private ExpressionAnimation VerticalThumbOffsetAnimation
        {
            get;
            set;
        }

        private double HorizontalThumbOffset
        {
            get
            {
                if (Thumb != null)
                {
                    double parentWidth = 0.0;
                    FrameworkElement parent = Thumb.Parent as FrameworkElement;
                    if (parent != null)
                    {
                        parentWidth = parent.ActualWidth;
                    }
                    if (horizontalScrollController.MaxOffset != horizontalScrollController.MinOffset)
                    {
                        return horizontalScrollController.Offset / (horizontalScrollController.MaxOffset - horizontalScrollController.MinOffset) * (parentWidth - Thumb.Width);
                    }
                }

                return 0.0;
            }
        }

        private double VerticalThumbOffset
        {
            get
            {
                if (Thumb != null)
                {
                    double parentHeight = 0.0;
                    FrameworkElement parent = Thumb.Parent as FrameworkElement;
                    if (parent != null)
                    {
                        parentHeight = parent.ActualHeight;
                    }
                    if (verticalScrollController.MaxOffset != verticalScrollController.MinOffset)
                    {
                        return verticalScrollController.Offset / (verticalScrollController.MaxOffset - verticalScrollController.MinOffset) * (parentHeight - Thumb.Height);
                    }
                }

                return 0.0;
            }
        }

        private void HookHandlers()
        {
            if (biDecrementRepeatButton != null)
            {
                biDecrementRepeatButton.Click += BiDecrementRepeatButton_Click;
            }

            if (biIncrementRepeatButton != null)
            {
                biIncrementRepeatButton.Click += BiIncrementRepeatButton_Click;
            }

            if (horizontalIncrementVerticalDecrementRepeatButton != null)
            {
                horizontalIncrementVerticalDecrementRepeatButton.Click += HorizontalIncrementVerticalDecrementRepeatButton_Click;
            }

            if (horizontalDecrementVerticalIncrementRepeatButton != null)
            {
                horizontalDecrementVerticalIncrementRepeatButton.Click += HorizontalDecrementVerticalIncrementRepeatButton_Click;
            }

            if (horizontalDecrementRepeatButton != null)
            {
                horizontalDecrementRepeatButton.Click += HorizontalDecrementRepeatButton_Click;
            }

            if (horizontalIncrementRepeatButton != null)
            {
                horizontalIncrementRepeatButton.Click += HorizontalIncrementRepeatButton_Click;
            }

            if (verticalDecrementRepeatButton != null)
            {
                verticalDecrementRepeatButton.Click += VerticalDecrementRepeatButton_Click;
            }

            if (verticalIncrementRepeatButton != null)
            {
                verticalIncrementRepeatButton.Click += VerticalIncrementRepeatButton_Click;
            }

            if (Thumb != null)
            {
                Thumb.PointerPressed += Thumb_PointerPressed;
                Thumb.ManipulationStarting += Thumb_ManipulationStarting;
                Thumb.ManipulationDelta += Thumb_ManipulationDelta;
                Thumb.ManipulationCompleted += Thumb_ManipulationCompleted;

                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parent.PointerPressed += Parent_PointerPressed;
                }
            }
        }

        private void UnhookHandlers()
        {
            if (biDecrementRepeatButton != null)
            {
                biDecrementRepeatButton.Click -= BiDecrementRepeatButton_Click;
            }

            if (biIncrementRepeatButton != null)
            {
                biIncrementRepeatButton.Click -= BiIncrementRepeatButton_Click;
            }

            if (horizontalIncrementVerticalDecrementRepeatButton != null)
            {
                horizontalIncrementVerticalDecrementRepeatButton.Click -= HorizontalIncrementVerticalDecrementRepeatButton_Click;
            }

            if (horizontalDecrementVerticalIncrementRepeatButton != null)
            {
                horizontalDecrementVerticalIncrementRepeatButton.Click -= HorizontalDecrementVerticalIncrementRepeatButton_Click;
            }

            if (horizontalDecrementRepeatButton != null)
            {
                horizontalDecrementRepeatButton.Click -= HorizontalDecrementRepeatButton_Click;
            }

            if (horizontalIncrementRepeatButton != null)
            {
                horizontalIncrementRepeatButton.Click -= HorizontalIncrementRepeatButton_Click;
            }

            if (verticalDecrementRepeatButton != null)
            {
                verticalDecrementRepeatButton.Click -= VerticalDecrementRepeatButton_Click;
            }

            if (verticalIncrementRepeatButton != null)
            {
                verticalIncrementRepeatButton.Click -= VerticalIncrementRepeatButton_Click;
            }

            if (Thumb != null)
            {
                Thumb.PointerPressed -= Thumb_PointerPressed;
                Thumb.ManipulationStarting -= Thumb_ManipulationStarting;
                Thumb.ManipulationDelta -= Thumb_ManipulationDelta;
                Thumb.ManipulationCompleted -= Thumb_ManipulationCompleted;

                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parent.PointerPressed -= Parent_PointerPressed;
                }
            }
        }

        private void BiDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: BiDecrementRepeatButton_Click");

            int viewChangeId =
                RaiseScrollFromRequested(new Vector2(-SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void BiIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: BiIncrementRepeatButton_Click");

            int viewChangeId =
                RaiseScrollFromRequested(new Vector2(SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalIncrementVerticalDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalIncrementVerticalDecrementRepeatButton_Click");

            int viewChangeId =
                RaiseScrollFromRequested(new Vector2(SmallChangeAdditionalVelocity, -SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalDecrementVerticalIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalDecrementVerticalIncrementRepeatButton_Click");

            int viewChangeId =
                RaiseScrollFromRequested(new Vector2(-SmallChangeAdditionalVelocity, SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalDecrementRepeatButton_Click");

            int viewChangeId =
                RaiseScrollFromRequested(new Vector2(-SmallChangeAdditionalVelocity, 0), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalIncrementRepeatButton_Click");

            int viewChangeId =
                RaiseScrollFromRequested(new Vector2(SmallChangeAdditionalVelocity, 0), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void VerticalDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: VerticalDecrementRepeatButton_Click");

            int viewChangeId =
                RaiseScrollFromRequested(new Vector2(0, -SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void VerticalIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: VerticalIncrementRepeatButton_Click");

            int viewChangeId =
                RaiseScrollFromRequested(new Vector2(0, SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void Thumb_PointerPressed(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            Point pt = e.GetCurrentPoint(sender as UIElement).Position;
            RaiseLogMessage("BiDirectionalScrollController: Thumb_PointerPressed with position=" + pt);

            switch (e.Pointer.PointerDeviceType)
            {
                case Windows.Devices.Input.PointerDeviceType.Touch:
                case Windows.Devices.Input.PointerDeviceType.Pen:
                    RaiseInteractionRequested(e.GetCurrentPoint(null));
                    break;
                case Windows.Devices.Input.PointerDeviceType.Mouse:
                    isThumbDragged = true;
                    AreScrollerInteractionsAllowed = false;
                    break;
            }
        }

        private void Parent_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            Point pt = e.GetCurrentPoint(sender as UIElement).Position;
            RaiseLogMessage("BiDirectionalScrollController: Parent_PointerPressed with position=" + pt);

            Point maxThumbOffset = MaxThumbOffset();
            double targetThumbHorizontalOffset = pt.X - Thumb.ActualWidth / 2.0;
            double targetThumbVerticalOffset = pt.Y - Thumb.ActualHeight / 2.0;

            targetThumbHorizontalOffset = Math.Max(targetThumbHorizontalOffset, 0.0);
            targetThumbVerticalOffset = Math.Max(targetThumbVerticalOffset, 0.0);

            targetThumbHorizontalOffset = Math.Min(targetThumbHorizontalOffset, maxThumbOffset.X);
            targetThumbVerticalOffset = Math.Min(targetThumbVerticalOffset, maxThumbOffset.Y);

            Point targetThumbOffset = new Point(targetThumbHorizontalOffset, targetThumbVerticalOffset);
            Point targetScrollPresenterOffset = ScrollPresenterOffsetFromThumbOffset(targetThumbOffset);

            int viewChangeId = RaiseScrollToRequested(targetScrollPresenterOffset, AnimationMode.Auto, true /*hookupCompletion*/);
            if (viewChangeId != -1 && !operations.ContainsKey(viewChangeId))
            {
                operations.Add(
                    viewChangeId, 
                    new OperationInfo(viewChangeId, new Point(targetScrollPresenterOffset.X - HorizontalThumbOffset, targetScrollPresenterOffset.Y - VerticalThumbOffset), targetScrollPresenterOffset));
            }
        }

        private void Thumb_ManipulationStarting(object sender, ManipulationStartingRoutedEventArgs e)
        {
            if (isThumbDragged)
            {
                preManipulationThumbOffset = new Point(HorizontalThumbOffset, VerticalThumbOffset);
            }
        }

        private void Thumb_ManipulationDelta(object sender, ManipulationDeltaRoutedEventArgs e)
        {
            if (isThumbDragged)
            {
                Point targetThumbOffset = new Point(
                    preManipulationThumbOffset.X + e.Cumulative.Translation.X,
                    preManipulationThumbOffset.Y + e.Cumulative.Translation.Y);
                Point scrollPresenterOffset = ScrollPresenterOffsetFromThumbOffset(targetThumbOffset);

                int viewChangeId = RaiseScrollToRequested(
                    scrollPresenterOffset, AnimationMode.Disabled, true /*hookupCompletion*/);
            }
        }

        private void Thumb_ManipulationCompleted(object sender, ManipulationCompletedRoutedEventArgs e)
        {
            if (isThumbDragged)
            {
                isThumbDragged = false;
                AreScrollerInteractionsAllowed = true;
            }
        }

        private void BiDirectionalScrollController_IsEnabledChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: IsEnabledChanged with IsEnabled=" + IsEnabled);
            if (!horizontalScrollController.UpdateAreScrollControllerInteractionsAllowed() ||
                !verticalScrollController.UpdateAreScrollControllerInteractionsAllowed())
            {
                RaiseInteractionInfoChanged();
            }
        }

        private void BiDirectionalScrollController_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateThumbSize();
        }

        private bool UpdateThumbSize()
        {
            if (Thumb != null)
            {
                Size parentSize;
                FrameworkElement parent = Thumb.Parent as FrameworkElement;

                if (parent != null)
                {
                    parentSize = new Size(parent.ActualWidth, parent.ActualHeight);
                }

                Size newSize;
                if (horizontalScrollController.Viewport == 0.0)
                {
                    newSize.Width = 40.0;
                }
                else
                {
                    newSize.Width = Math.Max(Math.Min(40.0, parentSize.Width), horizontalScrollController.Viewport / (horizontalScrollController.MaxOffset - horizontalScrollController.MinOffset + horizontalScrollController.Viewport) * parentSize.Width);
                }
                if (verticalScrollController.Viewport == 0.0)
                {
                    newSize.Height = 40.0;
                }
                else
                {
                    newSize.Height = Math.Max(Math.Min(40.0, parentSize.Height), verticalScrollController.Viewport / (verticalScrollController.MaxOffset - verticalScrollController.MinOffset + verticalScrollController.Viewport) * parentSize.Height);
                }
                if (newSize.Width != Thumb.Width)
                {
                    RaiseLogMessage("BiDirectionalScrollController: UpdateThumbSize for Orientation=Horizontal, setting Width=" + newSize.Width);
                    Thumb.Width = newSize.Width;
                    var ignored = Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Low, UpdateHorizontalInteractionVisualScrollMultiplier);
                    return true;
                }
                if (newSize.Height != Thumb.Height)
                {
                    RaiseLogMessage("BiDirectionalScrollController: UpdateThumbSize for Orientation=Vertical, setting Height=" + newSize.Height);
                    Thumb.Height = newSize.Height;
                    var ignored = Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Low, UpdateVerticalInteractionVisualScrollMultiplier);
                    return true;
                }
            }
            return false;
        }

        private void UpdateHorizontalInteractionVisualScrollMultiplier()
        {
            horizontalScrollController.UpdateInteractionVisualScrollMultiplier();
        }

        private void UpdateVerticalInteractionVisualScrollMultiplier()
        {
            verticalScrollController.UpdateInteractionVisualScrollMultiplier();
        }

        private Point ScrollPresenterOffsetFromThumbOffset(Point thumbOffset)
        {
            Point scrollPresenterOffset = new Point();

            if (Thumb != null)
            {
                Size parentSize;
                Size thumbSize = new Size(Thumb.ActualWidth, Thumb.ActualHeight);
                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parentSize = new Size(parent.ActualWidth, parent.ActualHeight);
                }
                if (parentSize.Width != thumbSize.Width || parentSize.Height != thumbSize.Height)
                {
                    scrollPresenterOffset = new Point(
                        parentSize.Width == thumbSize.Width ? 0 : (thumbOffset.X * (horizontalScrollController.MaxOffset - horizontalScrollController.MinOffset) / (parentSize.Width - thumbSize.Width)),
                        parentSize.Height == thumbSize.Height ? 0 : (thumbOffset.Y * (verticalScrollController.MaxOffset - verticalScrollController.MinOffset) / (parentSize.Height - thumbSize.Height)));
                }
            }

            return scrollPresenterOffset;
        }

        private Point MaxThumbOffset()
        {
            Point maxThumbOffset = new Point();

            if (Thumb != null)
            {
                Size parentSize;
                Size thumbSize = new Size(Thumb.ActualWidth, Thumb.ActualHeight);
                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parentSize = new Size(parent.ActualWidth, parent.ActualHeight);
                }
                maxThumbOffset = new Point(
                    Math.Max(0.0, parentSize.Width - thumbSize.Width),
                    Math.Max(0.0, parentSize.Height - thumbSize.Height));
            }

            return maxThumbOffset;
        }

        private void UniScrollController_ScrollCompleted(IScrollController sender, UniScrollControllerScrollCompletedEventArgs args)
        {
            if (lstScrollToIds.Contains(args.OffsetChangeId))
            {
                lstScrollToIds.Remove(args.OffsetChangeId);

                Point relativeOffsetChange;

                if (operations.ContainsKey(args.OffsetChangeId))
                {
                    OperationInfo oi = operations[args.OffsetChangeId];
                    relativeOffsetChange = oi.RelativeOffsetChange;
                    operations.Remove(args.OffsetChangeId);
                }

                RaiseLogMessage("BiDirectionalScrollController: ScrollToRequest completed. OffsetChangeId=" + args.OffsetChangeId);
            }
            else if (lstScrollByIds.Contains(args.OffsetChangeId))
            {
                lstScrollByIds.Remove(args.OffsetChangeId);

                Point relativeOffsetChange;

                if (operations.ContainsKey(args.OffsetChangeId))
                {
                    OperationInfo oi = operations[args.OffsetChangeId];
                    relativeOffsetChange = oi.RelativeOffsetChange;
                    operations.Remove(args.OffsetChangeId);
                }

                RaiseLogMessage("BiDirectionalScrollController: ScrollByRequest completed. OffsetChangeId=" + args.OffsetChangeId);
            }
            else if (lstScrollFromIds.Contains(args.OffsetChangeId))
            {
                lstScrollFromIds.Remove(args.OffsetChangeId);

                RaiseLogMessage("BiDirectionalScrollController: ScrollFromRequest completed. OffsetChangeId=" + args.OffsetChangeId);
            }

            if (lstViewChangeIds.Contains(args.OffsetChangeId))
            {
                lstViewChangeIds.Remove(args.OffsetChangeId);

                RaiseScrollCompleted(args.OffsetChangeId);
            }
        }

        private void RaiseInteractionInfoChanged()
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseInteractionInfoChanged");
            horizontalScrollController.RaiseInteractionInfoChanged();
            verticalScrollController.RaiseInteractionInfoChanged();
        }

        private void RaiseInteractionRequested(PointerPoint pointerPoint)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseInteractionRequested with pointerPoint=" + pointerPoint);
            bool handled = horizontalScrollController.RaiseInteractionRequested(pointerPoint);
            RaiseLogMessage("BiDirectionalScrollController: RaiseInteractionRequested result Handled=" + handled);
        }

        private int RaiseScrollToRequested(
            Point offset,
            AnimationMode animationMode,
            bool hookupCompletion)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseScrollToRequested with offset=" + offset + ", animationMode=" + animationMode);

            int horizontalOffsetChangeId = horizontalScrollController.RaiseScrollToRequested(
                offset.X, animationMode);

            if (horizontalOffsetChangeId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Horizontal ScrollToRequest started. Id=" + horizontalOffsetChangeId);

            int verticalOffsetChangeId = verticalScrollController.RaiseScrollToRequested(
                offset.Y, animationMode);

            if (verticalOffsetChangeId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Vertical ScrollToRequest started. Id=" + verticalOffsetChangeId);

            int offsetChangeId = -1;

            if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId == verticalOffsetChangeId)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId != verticalOffsetChangeId)
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollToRequest Ids do not match.");
            }
            else if (horizontalOffsetChangeId != -1)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (verticalOffsetChangeId != -1)
            {
                offsetChangeId = verticalOffsetChangeId;
            }
            else
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollToRequest Ids are -1.");
            }

            if (hookupCompletion && offsetChangeId != -1 && !lstScrollToIds.Contains(offsetChangeId))
            {
                lstScrollToIds.Add(offsetChangeId);
            }

            return offsetChangeId;
        }

        private int RaiseScrollByRequested(
            Point offsetDelta,
            AnimationMode animationMode,
            bool hookupCompletion)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseScrollByRequested with offsetDelta=" + offsetDelta + ", animationMode=" + animationMode);

            int horizontalOffsetChangeId = horizontalScrollController.RaiseScrollByRequested(
                offsetDelta.X, animationMode);

            if (horizontalOffsetChangeId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Horizontal ScrollByRequest started. Id=" + horizontalOffsetChangeId);

            int verticalOffsetChangeId = verticalScrollController.RaiseScrollByRequested(
                offsetDelta.Y, animationMode);

            if (verticalOffsetChangeId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Vertical ScrollByRequest started. Id=" + verticalOffsetChangeId);

            int offsetChangeId = -1;

            if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId == verticalOffsetChangeId)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId != verticalOffsetChangeId)
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollByRequest Ids do not match.");
            }
            else if (horizontalOffsetChangeId != -1)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (verticalOffsetChangeId != -1)
            {
                offsetChangeId = verticalOffsetChangeId;
            }
            else
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollByRequest Ids are -1.");
            }

            if (hookupCompletion && offsetChangeId != -1 && !lstScrollByIds.Contains(offsetChangeId))
            {
                lstScrollByIds.Add(offsetChangeId);
            }

            return offsetChangeId;
        }

        private int RaiseScrollFromRequested(
            Vector2 additionalVelocity, Vector2? inertiaDecayRate, bool hookupCompletion)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseScrollFromRequested with additionalVelocity=" + additionalVelocity + ", inertiaDecayRate=" + inertiaDecayRate);

            int horizontalOffsetChangeId = -1;
            int verticalOffsetChangeId = -1;

            if (additionalVelocity.X != 0.0f)
            {
                horizontalOffsetChangeId = horizontalScrollController.RaiseScrollFromRequested(
                    additionalVelocity.X, inertiaDecayRate == null ? (float?)null : inertiaDecayRate.Value.X);

                if (horizontalOffsetChangeId != -1)
                    RaiseLogMessage("BiDirectionalScrollController: ScrollFromRequest started. Id=" + horizontalOffsetChangeId);
            }

            if (additionalVelocity.Y != 0.0f)
            {
                verticalOffsetChangeId = verticalScrollController.RaiseScrollFromRequested(
                    additionalVelocity.Y, inertiaDecayRate == null ? (float?)null : inertiaDecayRate.Value.Y);

                if (verticalOffsetChangeId != -1)
                    RaiseLogMessage("BiDirectionalScrollController: ScrollFromRequest started. Id=" + verticalOffsetChangeId);
            }

            int offsetChangeId = -1;

            if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId == verticalOffsetChangeId)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId != verticalOffsetChangeId)
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollFromRequest operations do not match.");
            }
            else if (horizontalOffsetChangeId != -1)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (verticalOffsetChangeId != -1)
            {
                offsetChangeId = verticalOffsetChangeId;
            }
            else
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollFromRequest operations are null.");
            }

            if (hookupCompletion && offsetChangeId != -1 && !lstScrollFromIds.Contains(offsetChangeId))
            {
                lstScrollFromIds.Add(offsetChangeId);
            }

            return offsetChangeId;
        }

        internal int OperationsCount
        {
            get
            {
                return operations.Count;
            }
        }

        internal float GetInteractionVisualScrollMultiplier(Orientation orientation, double maxOffset, double minOffset)
        {
            if (Thumb != null)
            {
                Thumb.UpdateLayout();

                double parentDim = 0.0;
                double thumbDim =
                    orientation == Orientation.Horizontal ? Thumb.ActualWidth : Thumb.ActualHeight;
                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parentDim = orientation == Orientation.Horizontal ? parent.ActualWidth : parent.ActualHeight;
                }
                if (parentDim != thumbDim)
                {
                    return (float)((maxOffset - minOffset) / (thumbDim - parentDim));
                }
            }

            return 0.0f;
        }

        internal void RaiseLogMessage(string message)
        {
            if (LogMessage != null)
            {
                LogMessage(this, message);
            }
        }

        private void RaiseScrollCompleted(int viewChangeId)
        {
            if (ScrollCompleted != null)
            {
                BiDirectionalScrollControllerScrollCompletedEventArgs args = new BiDirectionalScrollControllerScrollCompletedEventArgs(viewChangeId);

                ScrollCompleted(this, args);
            }
        }
    }
}
