// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.Foundation;

using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using VirtualizingLayoutContext = Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext;

namespace MUXControlsTestApp.Samples
{
    class CircleLayout : VirtualizingLayout
    {
        public double ItemSize { get; set; }

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var itemCount = context.ItemCount;
            for(int i=0; i< context.ItemCount; i++)
            {
                var container = context.GetOrCreateElementAt(i);
                container.Measure(new Size(ItemSize, ItemSize));
            }

            return availableSize;
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {
            var itemCount = context.ItemCount;
            var radius = GetCircleRadius(finalSize) - ItemSize / 2.0;
            var angleIncrement = 2 * Math.PI / itemCount;
            var angle = 0.0;

            for (int i = 0; i < context.ItemCount; i++)
            {
                var container = context.GetOrCreateElementAt(i);
                var x = Math.Sin(angle) * radius - ItemSize / 2.0 + finalSize.Width / 2.0;
                var y = -Math.Cos(angle) * radius - ItemSize / 2.0 + finalSize.Height / 2.0;
                container.Arrange(new Rect(x, y, ItemSize, ItemSize));
                angle += angleIncrement;
            }

            return finalSize;
        }

        private double GetCircleRadius(Size availableSize)
        {
            return Math.Min(availableSize.Width, availableSize.Height) / 2;
        }
    }
}