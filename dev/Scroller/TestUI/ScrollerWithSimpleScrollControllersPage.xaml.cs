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
using Windows.UI.Xaml.Media;

#if !BUILD_WINDOWS
using IScrollController = Microsoft.UI.Xaml.Controls.Primitives.IScrollController;
using ScrollControllerInteractionRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerInteractionRequestedEventArgs;
using ScrollControllerOffsetChangeRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerOffsetChangeRequestedEventArgs;
using ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs;
using ScrollerRailingMode = Microsoft.UI.Xaml.Controls.ScrollerRailingMode;
using ScrollerViewChangeResult = Microsoft.UI.Xaml.Controls.ScrollerViewChangeResult;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerWithSimpleScrollControllersPage : TestPage
    {
        private CanvasScrollControllerConsumer canvasScrollControllerConsumer = null;
        private IScrollController horizontalScrollController = null;
        private IScrollController verticalScrollController = null;

        public ScrollerWithSimpleScrollControllersPage()
        {
            this.InitializeComponent();
            this.Loaded += ScrollerWithSimpleScrollControllersPageOnLoaded;
        }

        private void LogMessage(string message)
        {
            if (chkLog.IsChecked.Value)
            {
                lstLog.Items.Add(message);
            }
        }

        private void ScrollerWithSimpleScrollControllersPageOnLoaded(object sender, RoutedEventArgs e)
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
                if (scroller.HorizontalScrollController != horizontalScrollController)
                {
                    scroller.HorizontalScrollController = horizontalScrollController;
                    LogMessage("Scroller.HorizontalScrollController set");
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
                if (scroller.VerticalScrollController != verticalScrollController)
                {
                    scroller.VerticalScrollController = verticalScrollController;
                    LogMessage("Scroller.VerticalScrollController set");
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
            if (scroller != null && scroller.HorizontalScrollController != null)
            {
                scroller.HorizontalScrollController = null;
                LogMessage("Scroller.HorizontalScrollController reset");
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
            if (scroller != null && scroller.VerticalScrollController != null)
            {
                scroller.VerticalScrollController = null;
                LogMessage("Scroller.VerticalScrollController reset");
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
                    scroller.Visibility = Visibility.Visible;
                    canvas.Visibility = Visibility.Collapsed;
                }
                else
                {
                    scroller.Visibility = Visibility.Collapsed;
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

                LogMessage(cmbIScrollControllerConsumer.SelectedIndex == 0 ? "Scroller is consumer" : "CanvasScrollControllerConsumer is consumer");
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
            horizontalPS.InsertScalar("CustomViewport", 0.0f);

            verticalPS = compositor.CreatePropertySet();
            verticalPS.InsertScalar("CustomMinOffset", 0.0f);
            verticalPS.InsertScalar("CustomMaxOffset", 0.0f);
            verticalPS.InsertScalar("CustomOffset", 0.0f);
            verticalPS.InsertScalar("CustomViewport", 0.0f);

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

        private void SetHorizontalInfo(double maxOffset, double viewport)
        {
            horizontalPS.InsertScalar("CustomMaxOffset", (float)maxOffset);
            horizontalPS.InsertScalar("CustomViewport", (float)viewport);
        }

        private double GetHorizontalViewport()
        {
            float viewport = 0.0f;
            horizontalPS.TryGetScalar("CustomViewport", out viewport);
            return (double)viewport;
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

        private void SetVerticalInfo(double maxOffset, double viewport)
        {
            verticalPS.InsertScalar("CustomMaxOffset", (float)maxOffset);
            verticalPS.InsertScalar("CustomViewport", (float)viewport);
        }

        private double GetVerticalViewport()
        {
            float viewport = 0.0f;
            verticalPS.TryGetScalar("CustomViewport", out viewport);
            return (double)viewport;
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
                        horizontalScrollController.InteractionRequested -= OnInteractionRequested;
                        horizontalScrollController.InteractionInfoChanged -= OnInteractionInfoChanged;
                        horizontalScrollController.OffsetChangeRequested -= OnOffsetChangeRequested;
                        horizontalScrollController.OffsetChangeWithAdditionalVelocityRequested -= OnOffsetChangeWithAdditionalVelocityRequested;
                        LogMessage("CanvasScrollControllerConsumer: old HorizontalScrollController events unhooked");
                    }

                    horizontalScrollController = value;

                    if (horizontalScrollController != null)
                    {
                        horizontalScrollController.InteractionRequested += OnInteractionRequested;
                        horizontalScrollController.InteractionInfoChanged += OnInteractionInfoChanged;
                        horizontalScrollController.OffsetChangeRequested += OnOffsetChangeRequested;
                        horizontalScrollController.OffsetChangeWithAdditionalVelocityRequested += OnOffsetChangeWithAdditionalVelocityRequested;
                        LogMessage("CanvasScrollControllerConsumer: new HorizontalScrollController events hooked");

                        horizontalScrollController.SetValues(
                            0.0                      /*minOffset*/,
                            GetHorizontalMaxOffset() /*maxOffset*/,
                            GetHorizontalOffset()    /*offset*/,
                            GetHorizontalViewport()  /*viewport*/);
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
                        verticalScrollController.InteractionRequested -= OnInteractionRequested;
                        verticalScrollController.InteractionInfoChanged -= OnInteractionInfoChanged;
                        verticalScrollController.OffsetChangeRequested -= OnOffsetChangeRequested;
                        verticalScrollController.OffsetChangeWithAdditionalVelocityRequested -= OnOffsetChangeWithAdditionalVelocityRequested;
                        LogMessage("CanvasScrollControllerConsumer: old VerticalScrollController events unhooked");
                    }

                    verticalScrollController = value;

                    if (verticalScrollController != null)
                    {
                        verticalScrollController.InteractionRequested += OnInteractionRequested;
                        verticalScrollController.InteractionInfoChanged += OnInteractionInfoChanged;
                        verticalScrollController.OffsetChangeRequested += OnOffsetChangeRequested;
                        verticalScrollController.OffsetChangeWithAdditionalVelocityRequested += OnOffsetChangeWithAdditionalVelocityRequested;
                        LogMessage("CanvasScrollControllerConsumer: new VerticalScrollController events hooked");

                        verticalScrollController.SetValues(
                            0.0                    /*minOffset*/,
                            GetVerticalMaxOffset() /*maxOffset*/,
                            GetVerticalOffset()    /*offset*/,
                            GetVerticalViewport()  /*viewport*/);
                    }
                }
            }
        }

        private void OnInteractionRequested(IScrollController sender, ScrollControllerInteractionRequestedEventArgs e)
        {
            if (sender == horizontalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnInteractionRequested for HorizontalScrollController");
            }
            else if (sender == verticalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnInteractionRequested for VerticalScrollController");
            }
            else
            {
                LogMessage("CanvasScrollControllerConsumer: OnInteractionRequested for unknown sender");
            }
        }

        private void OnInteractionInfoChanged(IScrollController sender, object e)
        {
            if (sender == horizontalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnInteractionInfoChanged for HorizontalScrollController");
            }
            else if (sender == verticalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnInteractionInfoChanged for VerticalScrollController");
            }
            else
            {
                LogMessage("CanvasScrollControllerConsumer: OnInteractionInfoChanged for unknown sender");
            }
        }

        private void OnOffsetChangeRequested(IScrollController sender, ScrollControllerOffsetChangeRequestedEventArgs e)
        {
            if (sender == horizontalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnOffsetChangeRequested for HorizontalScrollController");
                LogMessage("Offset=" + e.Offset + ", OffsetKind=" + e.OffsetKind + ", OffsetChangeKind=" + e.OffsetChangeKind);

                switch (e.OffsetKind)
                {
                    case ScrollerViewKind.Absolute:
                    {
                        switch (e.OffsetChangeKind)
                        {
                            case ScrollerViewChangeKind.DisableAnimation:
                                Canvas.SetLeft(scrolledElement, -e.Offset);
                                break;
                        }
                        break;
                    }
                }
            }
            else if (sender == verticalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnOffsetChangeRequested for VerticalScrollController");
                LogMessage("Offset=" + e.Offset + ", OffsetKind=" + e.OffsetKind + ", OffsetChangeKind=" + e.OffsetChangeKind);

                switch (e.OffsetKind)
                {
                    case ScrollerViewKind.Absolute:
                    {
                        switch (e.OffsetChangeKind)
                        {
                            case ScrollerViewChangeKind.DisableAnimation:
                                Canvas.SetTop(scrolledElement, -e.Offset);
                                break;
                        }
                        break;
                    }
                }
            }
            else
            {
                LogMessage("CanvasScrollControllerConsumer: OnOffsetChangeRequested for unknown sender");
            }
        }

        private void OnOffsetChangeWithAdditionalVelocityRequested(IScrollController sender, ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs e)
        {
            if (sender == horizontalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnOffsetChangeWithAdditionalVelocityRequested for HorizontalScrollController");
            }
            else if (sender == verticalScrollController)
            {
                LogMessage("CanvasScrollControllerConsumer: OnOffsetChangeWithAdditionalVelocityRequested for VerticalScrollController");
            }
            else
            {
                LogMessage("CanvasScrollControllerConsumer: OnOffsetChangeWithAdditionalVelocityRequested for unknown sender");
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
                    0.0                      /*minOffset*/,
                    GetHorizontalMaxOffset() /*maxOffset*/,
                    GetHorizontalOffset()    /*offset*/,
                    GetHorizontalViewport()  /*viewport*/);

            if (verticalScrollController != null)
                verticalScrollController.SetValues(
                    0.0                    /*minOffset*/,
                    GetVerticalMaxOffset() /*maxOffset*/,
                    GetVerticalOffset()    /*offset*/,
                    GetVerticalViewport()  /*viewport*/);
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
        private List<int> lstOffsetChangeIds = new List<int>();
        private List<int> lstOffsetChangeWithAdditionalVelocityIds = new List<int>();

        public event TypedEventHandler<IScrollController, ScrollControllerInteractionRequestedEventArgs> InteractionRequested;
        public event TypedEventHandler<IScrollController, object> InteractionInfoChanged;
        public event TypedEventHandler<IScrollController, ScrollControllerOffsetChangeRequestedEventArgs> OffsetChangeRequested;
        public event TypedEventHandler<IScrollController, ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs> OffsetChangeWithAdditionalVelocityRequested;

        public ScrollBarController(ScrollBar scrollBar, ListBox logList, bool isLogging)
        {
            this.scrollBar = scrollBar;
            this.IsLogging = isLogging;

            AreScrollerInteractionsAllowed = true;

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

        public bool AreScrollerInteractionsAllowed
        {
            get;
            private set;
        }

        public bool IsInteracting
        {
            get;
            private set;
        }

        public Visual InteractionVisual
        {
            get
            {
                return null;
            }
        }

        public Orientation InteractionVisualScrollOrientation
        {
            get
            {
                return Orientation;
            }
        }

        public ScrollerRailingMode InteractionVisualScrollRailingMode
        {
            get
            {
                // Unused because InteractionVisual returns null.
                return ScrollerRailingMode.Disabled;
            }
        }

        public void SetExpressionAnimationSources(
            CompositionPropertySet propertySet,
            string minOffsetPropertyName,
            string maxOffsetPropertyName,
            string offsetPropertyName,
            string multiplierPropertyName)
        {
            /*
            double scrollBarTrackDim = 0.0f;

            if (Orientation == Orientation.Horizontal)
            {
                scrollBarTrackDim = scrollBar.ActualWidth - 32.0;
            }
            else
            {
                scrollBarTrackDim = scrollBar.ActualHeight - 32.0;
            }
            scrollBarTrackDim *= (1.0 - (scrollBar.ViewportSize / (scrollBar.Maximum - scrollBar.Minimum + scrollBar.ViewportSize)));

            If InteractionVisual returned a non-null value, the multiplierPropertyName scalar would have to be set to this value:
                (float)((scrollBar.Maximum - scrollBar.Minimum) / scrollBarTrackDim);
            */
        }

        public void SetValues(
            double minOffset,
            double maxOffset,
            double offset,
            double viewport)
        {
            LogMessage("ScrollBarController: SetValues for Orientation=" + Orientation + " with maxOffset=" + maxOffset + ", offset=" + offset + ", viewport=" + viewport);

            if (operationsCount > 0)
            {
                LogMessage("ScrollBarController: SetValues ignored during operation");
                return;
            }

            scrollBar.ViewportSize = viewport;
            scrollBar.Minimum = minOffset;
            scrollBar.Maximum = maxOffset;
            scrollBar.Value = offset;
            scrollBar.LargeChange = viewport;
        }

        public CompositionAnimation GetOffsetChangeAnimation(
            Int32 offsetChangeId,
            Vector2 currentPosition,
            CompositionAnimation defaultAnimation)
        {
            LogMessage(
                "ScrollBarController: GetOffsetChangeAnimation for Orientation=" + Orientation +
                " with offsetChangeId=" + offsetChangeId + ", currentPosition=" + currentPosition);
            return null;
        }

        public void OnOffsetChangeCompleted(
            Int32 offsetChangeId,
            ScrollerViewChangeResult result)
        {
            LogMessage(
                "ScrollBarController: OnOffsetChangeCompleted for Orientation=" + Orientation +
                " with offsetChangeId=" + offsetChangeId + ", result=" + result);

            if (lstOffsetChangeIds.Contains(offsetChangeId))
            {
                lstOffsetChangeIds.Remove(offsetChangeId);
                operationsCount--;
            }
            else if (lstOffsetChangeWithAdditionalVelocityIds.Contains(offsetChangeId))
            {
                lstOffsetChangeWithAdditionalVelocityIds.Remove(offsetChangeId);
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
                    AreScrollerInteractionsAllowed = true;
                    break;
                case ScrollEventType.LargeDecrement:
                case ScrollEventType.LargeIncrement:
                case ScrollEventType.SmallDecrement:
                case ScrollEventType.SmallIncrement:
                case ScrollEventType.ThumbPosition:
                case ScrollEventType.ThumbTrack:

                    if (e.ScrollEventType == ScrollEventType.ThumbTrack && AreScrollerInteractionsAllowed)
                    {
                        AreScrollerInteractionsAllowed = false;
                    }

                    int offsetChangeId = RaiseOffsetChangeRequested(
                        e.NewValue,
                        ScrollerViewKind.Absolute,
                        ScrollerViewChangeKind.DisableAnimation);
                    break;
            }
        }

        private void RaiseInteractionInfoChanged()
        {
            LogMessage("ScrollBarController: RaiseInteractionInfoChanged for Orientation=" + Orientation);
            if (InteractionInfoChanged != null)
            {
                InteractionInfoChanged(this, null);
            }
        }

        private void RaiseInteractionRequested(PointerPoint pointerPoint)
        {
            LogMessage("ScrollBarController: RaiseInteractionRequested for Orientation=" + Orientation + " with pointerPoint=" + pointerPoint);
            if (InteractionRequested != null)
            {
                InteractionRequested(this, new ScrollControllerInteractionRequestedEventArgs(pointerPoint));
            }
        }

        private int RaiseOffsetChangeRequested(
            double offset,
            ScrollerViewKind offsetKind,
            ScrollerViewChangeKind offsetChangeKind)
        {
            LogMessage("ScrollBarController: RaiseOffsetChangeRequested for Orientation=" + Orientation + " with offset =" + offset + ", offsetKind=" + offsetKind + ", offsetChangeKind=" + offsetChangeKind);
            if (OffsetChangeRequested != null)
            {
                ScrollControllerOffsetChangeRequestedEventArgs e = 
                    new ScrollControllerOffsetChangeRequestedEventArgs(
                        offset,
                        offsetKind,
                        offsetChangeKind);
                OffsetChangeRequested(this, e);
                if (e.ViewChangeId != -1 && !lstOffsetChangeIds.Contains(e.ViewChangeId))
                {
                    operationsCount++;
                    lstOffsetChangeIds.Add(e.ViewChangeId);
                }
                return e.ViewChangeId;
            }
            return -1;
        }

        private int RaiseOffsetChangeWithAdditionalVelocityRequested(
            float additionalVelocity, float? inertiaDecayRate)
        {
            LogMessage("ScrollBarController: RaiseOffsetChangeWithAdditionalVelocityRequested for Orientation=" + Orientation + " with additionalVelocity=" + additionalVelocity + ", inertiaDecayRate=" + inertiaDecayRate);
            if (OffsetChangeWithAdditionalVelocityRequested != null)
            {
                ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs e = 
                    new ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs(
                        additionalVelocity,
                        inertiaDecayRate);
                OffsetChangeWithAdditionalVelocityRequested(this, e);
                if (e.ViewChangeId != -1 && !lstOffsetChangeWithAdditionalVelocityIds.Contains(e.ViewChangeId))
                {
                    operationsCount++;
                    lstOffsetChangeWithAdditionalVelocityIds.Add(e.ViewChangeId);
                }
                return e.ViewChangeId;
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
