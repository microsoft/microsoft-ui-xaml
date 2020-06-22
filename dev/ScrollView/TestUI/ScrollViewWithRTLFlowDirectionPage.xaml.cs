// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollViewerWithRTLFlowDirectionPage : TestPage
    {
        public ScrollViewerWithRTLFlowDirectionPage()
        {
            this.InitializeComponent();
        }

        private void ChkScrollViewerFlowDirection_Checked(object sender, RoutedEventArgs e)
        {
            if (muxScrollViewer != null && wuxScrollViewer != null)
            {
                muxScrollViewer.FlowDirection = FlowDirection.RightToLeft;
                wuxScrollViewer.FlowDirection = FlowDirection.RightToLeft;
            }
        }

        private void ChkScrollViewerFlowDirection_Unchecked(object sender, RoutedEventArgs e)
        {
            if (muxScrollViewer != null && wuxScrollViewer != null)
            {
                muxScrollViewer.FlowDirection = FlowDirection.LeftToRight;
                wuxScrollViewer.FlowDirection = FlowDirection.LeftToRight;
            }
        }

        private void ChkScrollViewerContentFlowDirection_Checked(object sender, RoutedEventArgs e)
        {
            if (muxScrollViewer != null && wuxScrollViewer != null)
            {
                FrameworkElement content = muxScrollViewer.Content as FrameworkElement;

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

        private void ChkScrollViewerContentFlowDirection_Unchecked(object sender, RoutedEventArgs e)
        {
            if (muxScrollViewer != null && wuxScrollViewer != null)
            {
                FrameworkElement content = muxScrollViewer.Content as FrameworkElement;

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
