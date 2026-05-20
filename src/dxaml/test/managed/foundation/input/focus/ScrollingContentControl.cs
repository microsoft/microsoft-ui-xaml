// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace Microsoft.UI.Xaml.Tests.Focus
{
    public partial class ScrollingContentControl : ContentControl
    {
        private bool _isHorizontallyScrollable = false;
        private bool _isVerticallyScrollable = false;

        public ContentPresenter Presenter => ((ContentPresenter)VisualTreeHelper.GetChild(this, 0));


        public ScrollingContentControl()
        {
            this.IsTabStop = false;
            RegisterAsScrollPort(this);
        }

        public void UpdateConfiguration(bool isHorizontallyScrollable, bool isVerticallyScrollable)
        {
            _isHorizontallyScrollable = isHorizontallyScrollable;
            _isVerticallyScrollable = isVerticallyScrollable;

            // We updated a layout property, we need a new layout pass.
            InvalidateMeasure();
            UpdateLayout();
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            var measureSize = new Size(
                _isHorizontallyScrollable ? double.PositiveInfinity : availableSize.Width,
                _isVerticallyScrollable ? double.PositiveInfinity : availableSize.Height);
            Presenter.Measure(measureSize);
            return Presenter.DesiredSize;
        }
    }
}