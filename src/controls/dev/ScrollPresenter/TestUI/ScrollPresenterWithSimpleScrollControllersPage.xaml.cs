﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.Foundation;
using Microsoft.UI.Composition;
using Microsoft.UI.Input;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterWithSimpleScrollControllersPage : TestPage
    {
        private CanvasScrollControllerConsumer canvasScrollControllerConsumer = null;
        private IScrollController horizontalScrollController = null;
        private IScrollController verticalScrollController = null;

        public ScrollPresenterWithSimpleScrollControllersPage()
        {
            this.InitializeComponent();
            this.Loaded += ScrollPresenterWithSimpleScrollControllersPageOnLoaded;
        }

        private void LogMessage(string message)
        {
            if (chkLog.IsChecked.Value)
            {
                lstLog.Items.Add(message);
            }
        }

        private void ScrollPresenterWithSimpleScrollControllersPageOnLoaded(object sender, RoutedEventArgs e)
        {
            Rect rect = new Rect(0.0, 0.0, 300.0, 500.0);
            RectangleGeometry rectGeo = new RectangleGeometry();
            rectGeo.Rect = rect;
            this.canvas.Clip = rectGeo;

            CmbIScrollControllerConsumer_SelectionChanged(null, null);
        }

        private void BtnSetHorizontalScrollController_Click(object sender, RoutedEventArgs e)
        {
            if (horizontalScrollController == null)
            {
                horizontalScrollController = new ScrollBarController(horizontalScrollBar, lstLog, chkLog.IsChecked.Value);
            }

            ((ScrollBarController)horizontalScrollController).IsEnabled = true;

            if (cmbIScrollControllerConsumer.SelectedIndex == 0)
            {
                if (scrollPresenter.HorizontalScrollController != horizontalScrollController)
                {
                    scrollPresenter.HorizontalScrollController = horizontalScrollController;
                    LogMessage("ScrollPresenter.HorizontalScrollController set");
                }
            }
            else if (canvasScrollControllerConsumer.HorizontalScrollController != horizontalScrollController)
            {
                canvasScrollControllerConsumer.HorizontalScrollController = horizontalScrollController;
                LogMessage("CanvasScrollControllerConsumer.HorizontalScrollController set");
            }
        }

        private void BtnSetVerticalScrollController_Click(object sender, RoutedEventArgs e)
        {
            if (verticalScrollController == null)
            {
                verticalScrollController = new ScrollBarController(verticalScrollBar, lstLog, chkLog.IsChecked.Value);
            }

            ((ScrollBarController)verticalScrollController).IsEnabled = true;

            if (cmbIScrollControllerConsumer.SelectedIndex == 0)
            {
                if (scrollPresenter.VerticalScrollController != verticalScrollController)
                {
                    scrollPresenter.VerticalScrollController = verticalScrollController;
                    LogMessage("ScrollPresenter.VerticalScrollController set");
                }
            }
            else if (canvasScrollControllerConsumer.VerticalScrollController != verticalScrollController)
            {
                canvasScrollControllerConsumer.VerticalScrollController = verticalScrollController;
                LogMessage("CanvasScrollControllerConsumer.VerticalScrollController set");
            }
        }

        private void BtnResetHorizontalScrollController_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter != null && scrollPresenter.HorizontalScrollController != null)
            {
                scrollPresenter.HorizontalScrollController = null;
                LogMessage("ScrollPresenter.HorizontalScrollController reset");
            }

            if (canvasScrollControllerConsumer != null && canvasScrollControllerConsumer.HorizontalScrollController != null)
            {
                canvasScrollControllerConsumer.HorizontalScrollController = null;
                LogMessage("CanvasScrollControllerConsumer.HorizontalScrollController reset");
            }

            if (horizontalScrollController != null)
                ((ScrollBarController)horizontalScrollController).IsEnabled = false;
        }

        private void BtnResetVerticalScrollController_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter != null && scrollPresenter.VerticalScrollController != null)
            {
                scrollPresenter.VerticalScrollController = null;
                LogMessage("ScrollPresenter.VerticalScrollController reset");
            }
            if (canvasScrollControllerConsumer != null && canvasScrollControllerConsumer.VerticalScrollController != null)
            {
                canvasScrollControllerConsumer.VerticalScrollController = null;
                LogMessage("CanvasScrollControllerConsumer.VerticalScrollController reset");
            }

            if (verticalScrollController != null)
                ((ScrollBarController)verticalScrollController).IsEnabled = false;
        }

        private void BtnClearLog_Click(object sender, RoutedEventArgs e)
        {
            lstLog.Items.Clear();
        }

        private void CmbIScrollControllerConsumer_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (lstLog != null)
            {
                BtnResetHorizontalScrollController_Click(null, null);
                BtnResetVerticalScrollController_Click(null, null);

                if (cmbIScrollControllerConsumer.SelectedIndex == 0)
                {
                    scrollPresenter.Visibility = Visibility.Visible;
                    canvas.Visibility = Visibility.Collapsed;
                }
                else
                {
                    scrollPresenter.Visibility = Visibility.Collapsed;
                    canvas.Visibility = Visibility.Visible;
                }

                if (cmbIScrollControllerConsumer.SelectedIndex == 1)
                {
                    if (canvasScrollControllerConsumer == null)
                    {
                        canvasScrollControllerConsumer = new CanvasScrollControllerConsumer(lstLog, canvas, img2, chkLog.IsChecked.Value);
                    }

                    canvasScrollControllerConsumer.IsEnabled = true;
                }
                else
                {
                    canvasScrollControllerConsumer.IsEnabled = false;
                }

                LogMessage(cmbIScrollControllerConsumer.SelectedIndex == 0 ? "ScrollPresenter is consumer" : "CanvasScrollControllerConsumer is consumer");
            }
        }

        private void ChkLog_Checked(object sender, RoutedEventArgs e)
        {
            if (canvasScrollControllerConsumer != null)
                canvasScrollControllerConsumer.IsLogging = true;
            if (horizontalScrollController != null)
                ((ScrollBarController)horizontalScrollController).IsLogging = true;
            if (verticalScrollController != null)
                ((ScrollBarController)verticalScrollController).IsLogging = true;
        }

        private void ChkLog_Unchecked(object sender, RoutedEventArgs e)
        {
            if (canvasScrollControllerConsumer != null)
                canvasScrollControllerConsumer.IsLogging = false;
            if (horizontalScrollController != null)
                ((ScrollBarController)horizontalScrollController).IsLogging = false;
            if (verticalScrollController != null)
                ((ScrollBarController)verticalScrollController).IsLogging = false;
        }
    }

    public class CanvasScrollControllerConsumer
    {
        private CompositionPropertySet horizontalPS = null;
        private CompositionPropertySet verticalPS = null;
        private IScrollController horizontalScrollController = null;
        private IScrollController verticalScrollController = null;
        private ListBox lstLog = null;
        private Canvas canvas = null;
        private FrameworkElement scrolledElement = null;
        private double preManipulationHorizontalOffset = 0.0;
        private double preManipulationVerticalOffset = 0.0;
        private bool isEnabled = true;

        public CanvasScrollControllerConsumer(ListBox lstLog, Canvas canvas, FrameworkElement scrolledElement, bool isLogging)
        {
            this.lstLog = lstLog;
            this.canvas = canvas;
            this.scrolledElement = scrolledElement;
            this.IsLogging = isLogging;

            scrolledElement.ManipulationStarting += ScrolledElementManipulationStarting;
            scrolledElement.ManipulationDelta += ScrolledElementManipulationDelta;
            scrolledElement.ManipulationCompleted += ScrolledElementManipulationCompleted;

            Visual visual = ElementCompositionPreview.GetElementVisual(lstLog);
            Compositor compositor = visual.Compositor;

            horizontalPS = compositor.CreatePropertySet();
            horizontalPS.InsertScalar("CustomMinOffset", 0.0f);
            horizontalPS.InsertScalar("CustomMaxOffset", 0.0f);
            horizontalPS.InsertScalar("CustomOffset", 0.0f);
            horizontalPS.InsertScalar("CustomViewportLength", 0.0f);

            verticalPS = compositor.CreatePropertySet();
            verticalPS.InsertScalar("CustomMinOffset", 0.0f);
            verticalPS.InsertScalar("CustomMaxOffset", 0.0f);
            verticalPS.InsertScalar("CustomOffset", 0.0f);
            verticalPS.InsertScalar("CustomViewportLength", 0.0f);

            SetHorizontalInfo(Math.Max(0.0, scrolledElement.ActualWidth - canvas.ActualWidth), canvas.ActualWidth);
            SetVerticalInfo(Math.Max(0.0, scrolledElement.ActualHeight - canvas.ActualHeight), canvas.ActualWidth);
        }

        public bool IsLogging
        {
            get;
            set;
        }

        public bool IsEnabled
        {
            get
            {
                return isEnabled;
            }
            set
            {
                isEnabled = value;

                if (value)
                {
                    SetHorizontalOffset(-Canvas.GetLeft(scrolledElement));
                    SetVerticalOffset(-Canvas.GetTop(scrolledElement));
                }
            }
        }

        private void SetHorizontalInfo(double maxOffset, double viewportLength)
        {
            horizontalPS.InsertScalar("CustomMaxOffset", (float)maxOffset);
            horizontalPS.InsertScalar("CustomViewportLength", (float)viewportLength);
        }

        private double GetHorizontalViewportLength()
        {
            float viewportLength = 0.0f;
            horizontalPS.TryGetScalar("CustomViewportLength", out viewportLength);
            return (double)viewportLength;
        }

        private double GetHorizontalMaxOffset()
        {
            float maxOffset = 0.0f;
            horizontalPS.TryGetScalar("CustomMaxOffset", out maxOffset);
            return (double)maxOffset;
        }

        private void SetHorizontalOffset(double offset)
        {
            horizontalPS.InsertScalar("CustomOffset", (float)offset);
        }

        private double GetHorizontalOffset()
        {
            float offset = 0.0f;
            horizontalPS.TryGetScalar("CustomOffset", out offset);
            return (double)offset;
        }

        private void SetVerticalInfo(double maxOffset, double viewportLength)
        {
            verticalPS.InsertScalar("CustomMaxOffset", (float)maxOffset);
            verticalPS.InsertScalar("CustomViewportLength", (float)viewportLength);
        }

        private double GetVerticalViewportLength()
        {
            float viewportLength = 0.0f;
            verticalPS.TryGetScalar("CustomViewportLength", out viewportLength);
            return (double)viewportLength;
        }

        private double GetVerticalMaxOffset()
        {
            float maxOffset = 0.0f;
            verticalPS.TryGetScalar("CustomMaxOffset", out maxOffset);
            return (double)maxOffset;
        }

        private void SetVerticalOffset(double offset)
        {
            verticalPS.InsertScalar("CustomOffset", (float)offset);
        }

        private double GetVerticalOffset()
        {
            float offset = 0.0f;
            verticalPS.TryGetScalar("CustomOffset", out offset);
            return (double)offset;
        }

        public IScrollController HorizontalScrollController
        {
            get
            {
                return horizontalScrollController;
            }
            set
            {
                if (horizontalScrollController != value)
                {
                    if (horizontalScrollController != null)
                    {
                        horizontalScrollController.CanScrollChanged -= OnCanScrollChanged;
                        horizontalScrollController.IsScrollingWithMouseChanged -= OnIsScrollingWithMouseChanged;
                        horizontalScrollController.ScrollToRequested -= OnScrollToRequested;
                        horizontalScrollController.ScrollByRequested -= OnScrollByRequested;
                        horizontalScrollController.AddScrollVelocityRequested -= OnAddScrollVelocityRequested;
                        LogMessage("CanvasScrollControllerConsumer: old HorizontalScrollController events unhooked");
                    }

                    horizontalScrollController = value;

                    if (horizontalScrollController != null)
                    {
                        horizontalScrollController.CanScrollChanged += OnCanScrollChanged;
                        horizontalScrollController.IsScrollingWithMouseChanged += OnIsScrollingWithMouseChanged;
                        horizontalScrollController.ScrollToRequested += OnScrollToRequested;
                        horizontalScrollController.ScrollByRequested += OnScrollByRequested;
                        horizontalScrollController.AddScrollVelocityRequested += OnAddScrollVelocityRequested;
                        LogMessage("CanvasScrollControllerConsumer: new HorizontalScrollController events hooked");

                        horizontalScrollController.SetValues(
                            0.0                           /*minOffset*/,
                            GetHorizontalMaxOffset()      /*maxOffset*/,
                            GetHorizontalOffset()         /*offset*/,
                            GetHorizontalViewportLength() /*viewportLength*/);
                    }
                }
            }
        }

        public IScrollController VerticalScrollController
        {
            get
            {
                return verticalScrollController;
            }
            set
            {
                if (verticalScrollController != value)
                {
                    if (verticalScrollController != null)
                    {
                        verticalScrollController.CanScrollChanged -= OnCanScrollChanged;
                        verticalScrollController.IsScrollingWithMouseChanged -= OnIsScrollingWithMouseChanged;
                        verticalScrollController.ScrollToRequested -= OnScrollToRequested;
                        verticalScrollController.ScrollByRequested -= OnScrollByRequested;
                        verticalScrollController.AddScrollVelocityRequested -= OnAddScrollVelocityRequested;
                        LogMessage("CanvasScrollControllerConsumer: old VerticalScrollController events unhooked");
                    }

                    verticalScrollController = value;

                    if (verticalScrollController != null)
                    {
                        verticalScrollController.CanScrollChanged += OnCanScrollChanged;
                        verticalScrollController.IsScrollingWithMouseChanged += OnIsScrollingWithMouseChanged;
                        verticalScrollController.ScrollToRequested += OnScrollToRequested;
                        verticalScrollController.ScrollByRequested += OnScrollByRequested;
                        verticalScrollController.AddScrollVelocityRequested += OnAddScrollVelocityRequested;
                        LogMessage("CanvasScrollControllerConsumer: new VerticalScrollController events hooked");

                        verticalScrollController.SetValues(
                            0.0                         /*minOffset*/,
                            GetVerticalMaxOffset()      /*maxOffset*/,
                            GetVerticalOffset()         /*offset*/,
                            GetVerticalViewportLength() /*viewportLength*/);
                    }
                }
            }
        }

        private void OnCanScrollChanged(IScrollController sender, object e)
        {
            if (sender == horizontalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnCanScrollChanged for HorizontalScrollController");
            }
            else if (sender == verticalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnCanScrollChanged for VerticalScrollController");
            }
            else
            {
                LogMessage("CanvasScrollControllerConsumer: OnCanScrollChanged for unknown sender");
            }
        }

        private void OnIsScrollingWithMouseChanged(IScrollController sender, object e)
        {
            if (sender == horizontalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnIsScrollingWithMouseChanged for HorizontalScrollController");
            }
            else if (sender == verticalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnIsScrollingWithMouseChanged for VerticalScrollController");
            }
            else
            {
                LogMessage("CanvasScrollControllerConsumer: OnIsScrollingWithMouseChanged for unknown sender");
            }
        }

        private void OnScrollToRequested(IScrollController sender, ScrollControllerScrollToRequestedEventArgs e)
        {
            if (sender == horizontalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnScrollToRequested for HorizontalScrollController");
                LogMessage("Offset=" + e.Offset + ", AnimationMode=" + e.Options.AnimationMode + ", SnapPointsMode=" + e.Options.SnapPointsMode);

                if (e.Options.AnimationMode == ScrollingAnimationMode.Disabled)
                {
                    Canvas.SetLeft(scrolledElement, -e.Offset);
                }
            }
            else if (sender == verticalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnScrollToRequested for VerticalScrollController");
                LogMessage("Offset=" + e.Offset + ", AnimationMode=" + e.Options.AnimationMode + ", SnapPointsMode=" + e.Options.SnapPointsMode);

                if (e.Options.AnimationMode == ScrollingAnimationMode.Disabled)
                {
                    Canvas.SetTop(scrolledElement, -e.Offset);
                }
            }
            else
            {
                LogMessage("CanvasScrollControllerConsumer: OnScrollToRequested for unknown sender");
            }
        }

        private void OnScrollByRequested(IScrollController sender, ScrollControllerScrollByRequestedEventArgs e)
        {
            if (sender == horizontalScrollController || sender == verticalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnScrollByRequested for " + (sender == horizontalScrollController ? "HorizontalScrollController" : "VerticalScrollController"));
                LogMessage("OffsetDelta=" + e.OffsetDelta + ", AnimationMode=" + e.Options.AnimationMode + ", SnapPointsMode=" + e.Options.SnapPointsMode);
            }
            else
            {
                LogMessage("CanvasScrollControllerConsumer: OnScrollByRequested for unknown sender");
            }
        }

        private void OnAddScrollVelocityRequested(IScrollController sender, ScrollControllerAddScrollVelocityRequestedEventArgs e)
        {
            if (sender == horizontalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnAddScrollVelocityRequested for HorizontalScrollController");
            }
            else if (sender == verticalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnAddScrollVelocityRequested for VerticalScrollController");
            }
            else
            {
                LogMessage("CanvasScrollControllerConsumer: OnAddScrollVelocityRequested for unknown sender");
            }
        }

        private void ScrolledElementManipulationStarting(object sender, ManipulationStartingRoutedEventArgs e)
        {
            preManipulationHorizontalOffset = -Canvas.GetLeft(scrolledElement);
            preManipulationVerticalOffset = -Canvas.GetTop(scrolledElement);
        }

        private void ScrolledElementManipulationDelta(object sender, ManipulationDeltaRoutedEventArgs e)
        {
            UpdateOffsets(e.Cumulative.Translation);
        }

        private void ScrolledElementManipulationCompleted(object sender, ManipulationCompletedRoutedEventArgs e)
        {
            UpdateOffsets(e.Cumulative.Translation, true);

            preManipulationHorizontalOffset = 0.0;
            preManipulationVerticalOffset = 0.0;
        }

        private void UpdateOffsets(Point cumulativeTranslation, bool clamp = false)
        {
            double newHorizontalOffset = preManipulationHorizontalOffset - cumulativeTranslation.X;
            double newVerticalOffset = preManipulationVerticalOffset - cumulativeTranslation.Y;

            if (clamp)
            {
                newHorizontalOffset = Math.Max(0.0, newHorizontalOffset);
                newVerticalOffset = Math.Max(0.0, newVerticalOffset);

                newHorizontalOffset = Math.Min(GetHorizontalMaxOffset(), newHorizontalOffset);
                newVerticalOffset = Math.Min(GetVerticalMaxOffset(), newVerticalOffset);
            }

            Canvas.SetLeft(scrolledElement, -newHorizontalOffset);
            Canvas.SetTop(scrolledElement, -newVerticalOffset);

            SetHorizontalOffset(newHorizontalOffset);
            SetVerticalOffset(newVerticalOffset);

            if (horizontalScrollController != null)
                horizontalScrollController.SetValues(
                    0.0                           /*minOffset*/,
                    GetHorizontalMaxOffset()      /*maxOffset*/,
                    GetHorizontalOffset()         /*offset*/,
                    GetHorizontalViewportLength() /*viewportLength*/);

            if (verticalScrollController != null)
                verticalScrollController.SetValues(
                    0.0                         /*minOffset*/,
                    GetVerticalMaxOffset()      /*maxOffset*/,
                    GetVerticalOffset()         /*offset*/,
                    GetVerticalViewportLength() /*viewportLength*/);
        }

        private void LogMessage(string message)
        {
            if (IsLogging)
            {
                lstLog.Items.Add(message);
            }
        }
    }

    public class ScrollBarController : IScrollController
    {
        private ListBox lstLog = null;
        private ScrollBar scrollBar = null;
        private int operationsCount = 0;
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private List<int> lstScrollToCorrelationIds = new List<int>();
        private List<int> lstScrollByCorrelationIds = new List<int>();
        private List<int> lstAddScrollVelocityCorrelationIds = new List<int>();

        public event TypedEventHandler<IScrollController, object> CanScrollChanged;
        public event TypedEventHandler<IScrollController, object> IsScrollingWithMouseChanged;
        public event TypedEventHandler<IScrollController, ScrollControllerScrollToRequestedEventArgs> ScrollToRequested;
        public event TypedEventHandler<IScrollController, ScrollControllerScrollByRequestedEventArgs> ScrollByRequested;
        public event TypedEventHandler<IScrollController, ScrollControllerAddScrollVelocityRequestedEventArgs> AddScrollVelocityRequested;

        public ScrollBarController(ScrollBar scrollBar, ListBox logList, bool isLogging)
        {
            this.scrollBar = scrollBar;
            this.IsLogging = isLogging;
            this.IsScrollingWithMouse = false;

            lstLog = logList;
            LogMessage("ScrollBarController: constructor for Orientation=" + Orientation);

            scrollBar.SmallChange = 16.0;
            scrollBar.ValueChanged += ScrollBarValueChanged;
            scrollBar.Scroll += ScrollBarScroll;
        }

        public bool IsLogging
        {
            get;
            set;
        }

        public bool IsEnabled
        {
            get
            {
                return scrollBar.IsEnabled;
            }
            set
            {
                scrollBar.IsEnabled = value;
                if (!value)
                    scrollBar.Value = 0.0;
            }
        }

        public Orientation Orientation
        {
            get
            {
                return scrollBar.Orientation;
            }
        }

        public IScrollControllerPanningInfo PanningInfo
        {
            get
            {
                return null;
            }
        }

        public bool CanScroll
        {
            get;
            private set;
        }

        public bool IsScrollingWithMouse
        {
            get;
            private set;
        }

        public void SetIsScrollable(bool isScrollable)
        {
            LogMessage("ScrollBarController: SetIsScrollable for Orientation=" + Orientation + " with isScrollable=" + isScrollable);
            bool oldCanScroll = CanScroll;

            CanScroll = isScrollable && IsEnabled;

            if (oldCanScroll != CanScroll)
            {
                RaiseCanScrollChanged();
            }
        }

        public void SetValues(
            double minOffset,
            double maxOffset,
            double offset,
            double viewportLength)
        {
            LogMessage("ScrollBarController: SetValues for Orientation=" + Orientation + " with maxOffset=" + maxOffset + ", offset=" + offset + ", viewportLength=" + viewportLength);

            if (operationsCount > 0)
            {
                LogMessage("ScrollBarController: SetValues ignored during operation");
                return;
            }

            scrollBar.ViewportSize = viewportLength;
            scrollBar.Minimum = minOffset;
            scrollBar.Maximum = maxOffset;
            scrollBar.Value = offset;
            scrollBar.LargeChange = viewportLength;
        }

        public CompositionAnimation GetScrollAnimation(
            int correlationId,
            Vector2 startPosition,
            Vector2 endPosition,
            CompositionAnimation defaultAnimation)
        {
            LogMessage(
                "ScrollBarController: GetScrollAnimation for Orientation=" + Orientation +
                " with offsetChangeCorrelationId=" + correlationId + ", startPosition=" + startPosition + ", endPosition=" + endPosition);
            return null;
        }

        public void NotifyRequestedScrollCompleted(
            int correlationId)
        {
            int offsetChangeCorrelationId = correlationId;

            LogMessage(
                "ScrollBarController: NotifyRequestedScrollCompleted for Orientation=" + Orientation +
                " with offsetChangeCorrelationId=" + offsetChangeCorrelationId);

            if (lstScrollToCorrelationIds.Contains(offsetChangeCorrelationId))
            {
                lstScrollToCorrelationIds.Remove(offsetChangeCorrelationId);
                operationsCount--;
            }
            else if (lstScrollByCorrelationIds.Contains(offsetChangeCorrelationId))
            {
                lstScrollByCorrelationIds.Remove(offsetChangeCorrelationId);
                operationsCount--;
            }
            else if (lstAddScrollVelocityCorrelationIds.Contains(offsetChangeCorrelationId))
            {
                lstAddScrollVelocityCorrelationIds.Remove(offsetChangeCorrelationId);
                operationsCount--;
            }
        }

        private void ScrollBarValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            LogMessage("ScrollBarController: ScrollBarValueChanged for Orientation=" + Orientation + " with Value=" + scrollBar.Value);
        }

        private void ScrollBarScroll(object sender, ScrollEventArgs e)
        {
            LogMessage("ScrollBarController: ScrollBarScroll for Orientation=" + Orientation + " with ScrollEventType=" + e.ScrollEventType + ", NewValue = " + e.NewValue);

            switch (e.ScrollEventType)
            {
                case ScrollEventType.EndScroll:
                    if (IsScrollingWithMouse)
                    {
                        IsScrollingWithMouse = false;
                        RaiseIsScrollingWithMouseChanged();
                    }
                    break;
                case ScrollEventType.LargeDecrement:
                case ScrollEventType.LargeIncrement:
                case ScrollEventType.SmallDecrement:
                case ScrollEventType.SmallIncrement:
                case ScrollEventType.ThumbPosition:
                case ScrollEventType.ThumbTrack:

                    if (e.ScrollEventType == ScrollEventType.ThumbTrack && !IsScrollingWithMouse)
                    {
                        IsScrollingWithMouse = true;
                        RaiseIsScrollingWithMouseChanged();
                    }

                    int offsetChangeCorrelationId = RaiseScrollToRequested(
                        e.NewValue,
                        ScrollingAnimationMode.Disabled);
                    break;
            }
        }

        private void RaiseCanScrollChanged()
        {
            LogMessage("ScrollBarController: RaiseCanScrollChanged for Orientation=" + Orientation);
            if (CanScrollChanged != null)
            {
                CanScrollChanged(this, null);
            }
        }

        private void RaiseIsScrollingWithMouseChanged()
        {
            LogMessage("ScrollBarController: RaiseIsScrollingWithMouseChanged for Orientation=" + Orientation);
            if (IsScrollingWithMouseChanged != null)
            {
                IsScrollingWithMouseChanged(this, null);
            }
        }

        private int RaiseScrollToRequested(
            double offset,
            ScrollingAnimationMode animationMode)
        {
            LogMessage("ScrollBarController: RaiseScrollToRequested for Orientation=" + Orientation + " with offset=" + offset + ", animationMode=" + animationMode);
            if (ScrollToRequested != null)
            {
                ScrollControllerScrollToRequestedEventArgs e = 
                    new ScrollControllerScrollToRequestedEventArgs(
                        offset,
                        new ScrollingScrollOptions(animationMode, ScrollingSnapPointsMode.Ignore));
                ScrollToRequested(this, e);
                if (e.CorrelationId != -1 && !lstScrollToCorrelationIds.Contains(e.CorrelationId))
                {
                    operationsCount++;
                    lstScrollToCorrelationIds.Add(e.CorrelationId);
                }
                return e.CorrelationId;
            }
            return -1;
        }

        private int RaiseScrollByRequested(
            double offsetDelta,
            ScrollingAnimationMode animationMode)
        {
            LogMessage("ScrollBarController: RaiseScrollByRequested for Orientation=" + Orientation + " with offsetDelta=" + offsetDelta + ", animationMode=" + animationMode);
            if (ScrollToRequested != null)
            {
                ScrollControllerScrollByRequestedEventArgs e =
                    new ScrollControllerScrollByRequestedEventArgs(
                        offsetDelta,
                        new ScrollingScrollOptions(animationMode, ScrollingSnapPointsMode.Ignore));
                ScrollByRequested(this, e);
                if (e.CorrelationId != -1 && !lstScrollByCorrelationIds.Contains(e.CorrelationId))
                {
                    operationsCount++;
                    lstScrollByCorrelationIds.Add(e.CorrelationId);
                }
                return e.CorrelationId;
            }
            return -1;
        }

        private int RaiseAddScrollVelocityRequested(
            float offsetVelocity, float? inertiaDecayRate)
        {
            LogMessage("ScrollBarController: RaiseAddScrollVelocityRequested for Orientation=" + Orientation + " with offsetVelocity=" + offsetVelocity + ", inertiaDecayRate=" + inertiaDecayRate);
            if (AddScrollVelocityRequested != null)
            {
                ScrollControllerAddScrollVelocityRequestedEventArgs e = 
                    new ScrollControllerAddScrollVelocityRequestedEventArgs(
                        offsetVelocity,
                        inertiaDecayRate);
                AddScrollVelocityRequested(this, e);
                if (e.CorrelationId != -1 && !lstAddScrollVelocityCorrelationIds.Contains(e.CorrelationId))
                {
                    operationsCount++;
                    lstAddScrollVelocityCorrelationIds.Add(e.CorrelationId);
                }
                return e.CorrelationId;
            }
            return -1;
        }

        private void LogMessage(string message)
        {
            if (IsLogging)
            {
                lstLog.Items.Add(message);
            }
        }
    }
}
