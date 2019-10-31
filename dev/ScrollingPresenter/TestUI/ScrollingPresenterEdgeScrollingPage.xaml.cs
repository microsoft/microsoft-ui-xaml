// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System;
using System.Numerics;
using Windows.Devices.Input;
using Windows.Foundation;
using Windows.UI.Input;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresenterEdgeScrollingPage : TestPage
    {
        public ScrollingPresenterEdgeScrollingPage()
        {
            InitializeComponent();

            scrollingPresenter.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler(ScrollingPresenter_PointerPressed), true);
            scrollingPresenter.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler(ScrollingPresenter_PointerReleased), true);
            scrollingPresenter.AddHandler(UIElement.PointerMovedEvent, new PointerEventHandler(ScrollingPresenter_PointerMoved), true);
            scrollingPresenter.AddHandler(UIElement.PointerCanceledEvent, new PointerEventHandler(ScrollingPresenter_PointerCanceled), true);
            scrollingPresenter.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler(ScrollingPresenter_PointerCaptureLost), true);
        }

        private void ScrollingPresenter_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
        }

        private void ScrollingPresenter_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
        }

        private void ScrollingPresenter_PointerMoved(object sender, PointerRoutedEventArgs e)
        {
        }

        private void ScrollingPresenter_PointerCanceled(object sender, PointerRoutedEventArgs e)
        {
        }

        private void ScrollingPresenter_PointerCaptureLost(object sender, PointerRoutedEventArgs e)
        {
            LogEvent("Pointer capture lost - id " + e.Pointer.PointerId);
        }

        private void BtnUpdateHorizontalMouseEdgeScrolling_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Point pointerPositionAdjustment = new Point(
                    Convert.ToDouble(txtHorizontalMousePositionAdjustmentX.Text),
                    Convert.ToDouble(txtHorizontalMousePositionAdjustmentY.Text));

                Double leftEdgeApplicableRange = Convert.ToDouble(txtMouseLeftEdgeApplicableRange.Text);
                Double rightEdgeApplicableRange = Convert.ToDouble(txtMouseRightEdgeApplicableRange.Text);

                Single leftEdgeVelocity = Convert.ToSingle(txtMouseLeftEdgeVelocity.Text);
                Single rightEdgeVelocity = Convert.ToSingle(txtMouseRightEdgeVelocity.Text);

                LogEvent($"Activating Mouse with pointerPositionAdjustment:{pointerPositionAdjustment}, leftEdgeApplicableRange:{leftEdgeApplicableRange}, rightEdgeApplicableRange:{rightEdgeApplicableRange}, leftEdgeVelocity:{leftEdgeVelocity}, rightEdgeVelocity:{rightEdgeVelocity}");

                scrollingPresenter.SetHorizontalEdgeScrolling(
                    PointerDeviceType.Mouse,
                    pointerPositionAdjustment,
                    leftEdgeApplicableRange,
                    rightEdgeApplicableRange,
                    leftEdgeVelocity,
                    rightEdgeVelocity);

                btnCancelHorizontalMouseEdgeScrolling.IsEnabled = true;
                chkIsHorizontalMouseActive.IsChecked = true;
            }
            catch (Exception ex)
            {
                LogEvent(ex.ToString());
            }
        }

        private void BtnCancelHorizontalMouseEdgeScrolling_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                LogEvent($"Deactivating Mouse");

                scrollingPresenter.SetHorizontalEdgeScrolling(
                    pointerDeviceType: PointerDeviceType.Mouse,
                    pointerPositionAdjustment: new Point(0, 0),
                    leftEdgeApplicableRange: 0.0,
                    rightEdgeApplicableRange: 0.0,
                    leftEdgeVelocity: 0.0f,
                    rightEdgeVelocity: 0.0f);

                btnCancelHorizontalMouseEdgeScrolling.IsEnabled = false;
                chkIsHorizontalMouseActive.IsChecked = false;
            }
            catch (Exception ex)
            {
                LogEvent(ex.ToString());
            }
        }

        private void TxtMouseEdgeVelocity_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                Single leftEdgeVelocity = Convert.ToSingle(txtMouseLeftEdgeVelocity.Text);
                Single rightEdgeVelocity = Convert.ToSingle(txtMouseRightEdgeVelocity.Text);

                btnUpdateHorizontalMouseEdgeScrolling.IsEnabled = leftEdgeVelocity != 0.0f || rightEdgeVelocity != 0.0f;
            }
            catch (Exception ex)
            {
                LogEvent(ex.ToString());
            }
        }

        private void BtnClearEvents_Click(object sender, RoutedEventArgs e)
        {
            lstEvents.Items.Clear();
        }

        private void LogEvent(string log)
        {
            lstEvents.Items.Insert(0, log);
        }
    }
}
