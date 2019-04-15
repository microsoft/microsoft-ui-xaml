using System;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;

namespace Flick
{
    public class CircleLayout : NonVirtualizingLayout
    {
        public double ItemSize { get; set; }

        protected override Size MeasureOverride(NonVirtualizingLayoutContext context, Size availableSize)
        {
            foreach(var container in context.Children)
            {
                container.Measure(new Size(ItemSize, ItemSize));
            }

            return availableSize;
        }

        protected override Size ArrangeOverride(NonVirtualizingLayoutContext context, Size finalSize)
        {
            var itemCount = context.Children.Count;
            var radius = GetCircleRadius(finalSize) - ItemSize / 2.0;
            var angleIncrement = 2 * Math.PI / itemCount;
            var angle = 0.0;

            foreach(var container in context.Children)
            { 
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