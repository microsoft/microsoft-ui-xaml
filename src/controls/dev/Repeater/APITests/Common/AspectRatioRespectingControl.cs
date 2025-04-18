﻿using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.UI.Xaml.Data;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;

namespace MUXControls.ApiTests.RepeaterTests.Common
{
    [Microsoft.UI.Xaml.Data.Bindable]
    public partial class AspectRatioRespectingControl : ContentPresenter
    {
        protected override Size MeasureOverride(Size availableSize)
        {
            return TwoByOneifySize(availableSize);
        }

        /// <inheritdoc/>
        protected override Size ArrangeOverride(Size finalSize)
        {
            return TwoByOneifySize(finalSize);
        }

        private Size TwoByOneifySize(Size initialSize)
        {
            if (Double.IsNaN(initialSize.Height) || Double.IsInfinity(initialSize.Height))
            {
                return new Size(initialSize.Width/ 2, initialSize.Width);
            }
            if (Double.IsNaN(initialSize.Width) || Double.IsInfinity(initialSize.Width))
            {
                return new Size(initialSize.Height / 2, initialSize.Height);
            }
            return new Size(Math.Min(100,initialSize.Height / 2), Math.Min(100,initialSize.Height));
        }
    }
}
