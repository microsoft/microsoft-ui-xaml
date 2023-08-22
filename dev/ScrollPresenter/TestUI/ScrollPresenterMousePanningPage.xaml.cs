// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System;
using System.Numerics;
using Windows.Foundation;
using Windows.UI.Input;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Input;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterMousePanningPage : TestPage
    {
        private bool _isMiddleButtonPressed = false;
        private bool _isInConstantVelocityPan = false;
        private Point _preConstantVelocityPanPosition;
        private bool _isInMousePan = false;
        private Point _preMousePanPosition;
        private double _preMousePanHorizontalOffset = 0.0;
        private double _preMousePanVerticalOffset = 0.0;

        public ScrollPresenterMousePanningPage()
        {
            InitializeComponent();

            btnClearEvents.Click += BtnClearEvents_Click;
            scrollPresenter.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler(ScrollPresenter_PointerPressed), true);
            scrollPresenter.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler(ScrollPresenter_PointerReleased), true);
            scrollPresenter.AddHandler(UIElement.PointerMovedEvent, new PointerEventHandler(ScrollPresenter_PointerMoved), true);
            scrollPresenter.AddHandler(UIElement.PointerCanceledEvent, new PointerEventHandler(ScrollPresenter_PointerCanceled), true);
            scrollPresenter.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler(ScrollPresenter_PointerCaptureLost), true);
        }

        private void ScrollPresenter_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            if (e.Pointer.PointerDeviceType == Windows.Devices.Input.PointerDeviceType.Mouse &&
                e.KeyModifiers == Windows.System.VirtualKeyModifiers.None)
            {
                if (_isMiddleButtonPressed || _isInConstantVelocityPan)
                {
                    (sender as UIElement).ReleasePointerCapture(e.Pointer);
                    StopConstantVelocityPan();
                    return;
                }

                if (!e.Handled)
                {
                    PointerPointProperties ppp = e.GetCurrentPoint(null).Properties;
                    Point position = e.GetCurrentPoint(scrollPresenter).Position;

                    if (ppp.IsLeftButtonPressed)
                    {
                        if (e.OriginalSource == rect)
                        {
                            // 'rect' element is meant to be dragged with the mouse using its
                            // left button, so do not initiate a mouse pan.
                            return;
                        }

                        if ((sender as UIElement).CapturePointer(e.Pointer))
                        {
                            Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                                Windows.UI.Core.CoreCursorType.Hand, 0);
                            _isInMousePan = true;
                            _preMousePanPosition = position;
                            _preMousePanHorizontalOffset = scrollPresenter.HorizontalOffset;
                            _preMousePanVerticalOffset = scrollPresenter.VerticalOffset;
                            LogEvent("Pointer capture initiated @ position " + _preMousePanPosition.ToString());
                        }
                        else
                        {
                            LogEvent("Pointer capture failed");
                        }
                    }
                    else if (ppp.IsMiddleButtonPressed)
                    {
                        if ((sender as UIElement).CapturePointer(e.Pointer))
                        {
                            Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                                Windows.UI.Core.CoreCursorType.SizeAll, 0);
                            _isMiddleButtonPressed = true;
                            _preConstantVelocityPanPosition = position;
                            fiConstantVelocityPanCenter.Visibility = Visibility.Visible;
                            fiConstantVelocityPanCenter.Margin = new Thickness(
                                position.X - 6,
                                position.Y - 6,
                                4, 4);
                            LogEvent("Middle button pressed @ position " + _preConstantVelocityPanPosition.ToString());
                        }
                        else
                        {
                            LogEvent("Pointer capture failed");
                        }
                    }
                }
            }
        }

        private void ScrollPresenter_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if (e.Pointer.PointerDeviceType == Windows.Devices.Input.PointerDeviceType.Mouse)
            {
                PointerPointProperties ppp = e.GetCurrentPoint(null).Properties;
                Point position = e.GetCurrentPoint(scrollPresenter).Position;

                if (_isInMousePan && !ppp.IsLeftButtonPressed)
                {
                    (sender as UIElement).ReleasePointerCapture(e.Pointer);
                    StopMousePan();
                    LogEvent("Pointer capture released @ position " + position.ToString());
                }
                else if (_isMiddleButtonPressed && !ppp.IsMiddleButtonPressed)
                {
                    _isMiddleButtonPressed = false;
                    LogEvent("Middle button released @ position " + position.ToString());
                    if (Math.Abs(position.X - _preConstantVelocityPanPosition.X) >= 4.0 ||
                        Math.Abs(position.Y - _preConstantVelocityPanPosition.Y) >= 4.0)
                    {
                        if (_isInConstantVelocityPan)
                        {
                            (sender as UIElement).ReleasePointerCapture(e.Pointer);
                            StopConstantVelocityPan();
                        }
                    }
                    else if (!_isInConstantVelocityPan)
                    {
                        // Unfortunately mouse capture is automatically lost and apparently cannot
                        // be regained. Xaml framework should allow retention of mouse capture
                        // beyond the PointerReleased event to correctly support middle-button-based panning.
                        StartConstantVelocityPan();
                    }
                }
            }
        }

        private void ScrollPresenter_PointerMoved(object sender, PointerRoutedEventArgs e)
        {
            if (e.Pointer.PointerDeviceType == Windows.Devices.Input.PointerDeviceType.Mouse)
            {
                Point position = e.GetCurrentPoint(scrollPresenter).Position;

                if (_isInMousePan)
                {
                    scrollPresenter.ScrollTo(
                        _preMousePanPosition.X - position.X + _preMousePanHorizontalOffset,
                        _preMousePanPosition.Y - position.Y + _preMousePanVerticalOffset,
                        new ScrollingScrollOptions(ScrollingAnimationMode.Disabled));
                }
                else
                {
                    if (_isMiddleButtonPressed)
                    {
                        if (Math.Abs(position.X - _preConstantVelocityPanPosition.X) >= 4.0 ||
                            Math.Abs(position.Y - _preConstantVelocityPanPosition.Y) >= 4.0)
                        {
                            if (!_isInConstantVelocityPan)
                            {
                                LogEvent("Pointer with pressed middle button moved to position " + position.ToString());
                                StartConstantVelocityPan();
                            }
                        }
                    }

                    if (_isInConstantVelocityPan)
                    {
                        Vector2 offsetsVelocity = new Vector2(
                            (float)(position.X - _preConstantVelocityPanPosition.X) / 2.0f,
                            (float)(position.Y - _preConstantVelocityPanPosition.Y) / 2.0f);

                        scrollPresenter.AddScrollVelocity(offsetsVelocity, inertiaDecayRate: Vector2.Zero);
                    }
                }
            }
        }

        private void ScrollPresenter_PointerCanceled(object sender, PointerRoutedEventArgs e)
        {
            LogEvent("Pointer canceled - id " + e.Pointer.PointerId);
            if (e.Pointer.PointerDeviceType == Windows.Devices.Input.PointerDeviceType.Mouse)
            {
                if (_isInMousePan)
                {
                    StopMousePan();
                }
                else if (_isMiddleButtonPressed || _isInConstantVelocityPan)
                {
                    StopConstantVelocityPan();
                }
            }
        }

        private void ScrollPresenter_PointerCaptureLost(object sender, PointerRoutedEventArgs e)
        {
            LogEvent("Pointer capture lost - id " + e.Pointer.PointerId);
        }

        private void StopMousePan()
        {
            Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                Windows.UI.Core.CoreCursorType.Arrow, 0);
            _isInMousePan = false;
            _preMousePanHorizontalOffset = 0.0;
            _preMousePanVerticalOffset = 0.0;
        }

        private void StartConstantVelocityPan()
        {
            _isInConstantVelocityPan = true;
            LogEvent("Constant velocity pan started");
        }

        private void StopConstantVelocityPan()
        {
            Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                Windows.UI.Core.CoreCursorType.Arrow, 0);
            _isInConstantVelocityPan = false;
            _isMiddleButtonPressed = false;
            fiConstantVelocityPanCenter.Visibility = Visibility.Collapsed;
            scrollPresenter.ScrollBy(0, 0);
            LogEvent("Constant velocity pan stopped");
        }

        private void BtnClearEvents_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            lstEvents.Items.Clear();
        }

        private void LogEvent(string log)
        {
            lstEvents.Items.Insert(0, log);
        }
    }
}
