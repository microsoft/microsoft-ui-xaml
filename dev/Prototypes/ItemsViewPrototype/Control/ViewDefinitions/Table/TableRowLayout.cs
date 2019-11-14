using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using Windows.Foundation;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class TableRowLayout : VirtualizingLayout
    {
        private Dictionary<int, double> _columnWidths;

        public Dictionary<int, double> ColumnWidths
        {
            get
            {
                if (_columnWidths == null)
                {
                    _columnWidths = new Dictionary<int, double>();
                }

                return _columnWidths;
            }
        }

        public void InvalidateLayout()
        {
           InvalidateMeasure();
        }

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            Size size = new Size(0, 0);
            for (int i = 0; i < context.ItemCount; i++)
            {
                var child = context.GetOrCreateElementAt(i);
                child.Measure(availableSize);
                if (!ColumnWidths.ContainsKey(i))
                {
                    ColumnWidths.Add(i, child.DesiredSize.Width);
                }
                else
                {
                    if (child.DesiredSize.Width > ColumnWidths[i])
                    {
                        size.Width += child.DesiredSize.Width;
                        ColumnWidths[i] = child.DesiredSize.Width;

                        // Invalidate all listening containers to use the updated column size
                        this.InvalidateMeasure();
                    }
                    else
                    {
                        size.Width += ColumnWidths[i];
                    }
                }

                size.Height = Math.Max(size.Height, child.DesiredSize.Height);
            }

            return size;
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {
            double x = 0;
            for (int i = 0; i < context.ItemCount; i++)
            {
                var child = context.GetOrCreateElementAt(i);
                var childRect = new Rect(x, 0, ColumnWidths[i], finalSize.Height);
                child.Arrange(childRect);
                x += childRect.Width;
            }

            return finalSize;
        }
    }
}
