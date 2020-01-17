// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingViewWithRTLFlowDirectionPage : TestPage
    {
        public ScrollingViewWithRTLFlowDirectionPage()
        {
            this.InitializeComponent();
        }

        private void ChkScrollingViewFlowDirection_Checked(object sender, RoutedEventArgs e)
        {
            if (muxScrollingView != null && wuxScrollViewer != null)
            {
                muxScrollingView.FlowDirection = FlowDirection.RightToLeft;
                wuxScrollViewer.FlowDirection = FlowDirection.RightToLeft;
            }
        }

        private void ChkScrollingViewFlowDirection_Unchecked(object sender, RoutedEventArgs e)
        {
            if (muxScrollingView != null && wuxScrollViewer != null)
            {
                muxScrollingView.FlowDirection = FlowDirection.LeftToRight;
                wuxScrollViewer.FlowDirection = FlowDirection.LeftToRight;
            }
        }

        private void ChkScrollingViewContentFlowDirection_Checked(object sender, RoutedEventArgs e)
        {
            if (muxScrollingView != null && wuxScrollViewer != null)
            {
                FrameworkElement content = muxScrollingView.Content as FrameworkElement;

                if (content != null)
                {
                    content.FlowDirection = FlowDirection.RightToLeft;
                }

                content = wuxScrollViewer.Content as FrameworkElement;

                if (content != null)
                {
                    content.FlowDirection = FlowDirection.RightToLeft;
                }
            }
        }

        private void ChkScrollingViewContentFlowDirection_Unchecked(object sender, RoutedEventArgs e)
        {
            if (muxScrollingView != null && wuxScrollViewer != null)
            {
                FrameworkElement content = muxScrollingView.Content as FrameworkElement;

                if (content != null)
                {
                    content.FlowDirection = FlowDirection.LeftToRight;
                }

                content = wuxScrollViewer.Content as FrameworkElement;

                if (content != null)
                {
                    content.FlowDirection = FlowDirection.LeftToRight;
                }
            }
        }
    }
}
