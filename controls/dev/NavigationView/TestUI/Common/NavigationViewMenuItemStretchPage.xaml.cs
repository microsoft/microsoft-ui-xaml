// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Windows.Foundation;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewMenuItemStretchPage : TestPage
    {
        private readonly List<TeachingTipCloseReason> _outsideTipCloseReasons = new List<TeachingTipCloseReason>();
        private readonly List<TeachingTipCloseReason> _insideTipCloseReasons = new List<TeachingTipCloseReason>();

        public NavigationViewMenuItemStretchPage()
        {
            this.InitializeComponent();
            OutsideTip.Closed += (sender, args) => _outsideTipCloseReasons.Add(args.Reason);
            InsideTip.Closed += (sender, args) => _insideTipCloseReasons.Add(args.Reason);
        }

        private void CheckTeachingTipStateButton_Click(object sender, RoutedEventArgs e)
        {
            OutsideTipState.Text = DescribeTipState(OutsideTip, TipTargetOutsideNavView, _outsideTipCloseReasons);
            InsideTipState.Text = DescribeTipState(InsideTip, TipTargetInsideNavViewItem, _insideTipCloseReasons);
        }

        private static string DescribeTipState(TeachingTip tip, ButtonBase target, List<TeachingTipCloseReason> closeReasons)
        {
            Popup popup = TeachingTipTestHooks.GetPopup(tip);
            Rect targetBounds = target.TransformToVisual(null).TransformBounds(new Rect(0, 0, target.ActualWidth, target.ActualHeight));
            return $"IsPointerOver={target.IsPointerOver};IsOpen={tip.IsOpen};PopupOpen={popup != null && popup.IsOpen};" +
                $"Placement={TeachingTipTestHooks.GetEffectivePlacement(tip)};PopupOffset=({popup?.HorizontalOffset},{popup?.VerticalOffset});" +
                $"TargetIsLoaded={target.IsLoaded};TargetBounds={targetBounds};Closed=[{string.Join(",", closeReasons)}]";
        }
    }
}
