using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using Windows.Foundation;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    public class TableRowLayout : VirtualizingLayoutBase
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
            OnLayoutInvalidated();
        }

        protected override Size MeasureLayoutCore(Size availableSize, LayoutContext context)
        {
            Size size = new Size(0, 0);
            for (int i = 0; i < context.ItemCount; i++)
            {
                var child = context.GetElementAt(i);
                child.Measure(availableSize);
                size.Width = size.Width + child.DesiredSize.Width;
                size.Height = Math.Max(size.Height, child.DesiredSize.Height);
                if (!ColumnWidths.ContainsKey(i))
                {
                    ColumnWidths.Add(i, child.DesiredSize.Width);
                }
                else
                {
                    if (child.DesiredSize.Width > ColumnWidths[i])
                    {
                        ColumnWidths[i] = child.DesiredSize.Width;
                        // Layout needs to run again since the column size needs to change.
                        OnLayoutInvalidated();
                    }
                    else
                    {
                        size.Width = ColumnWidths[i];
                    }
                }
            }

            return size;
        }

        protected override Size ArrangeLayoutCore(Size finalSize, LayoutContext context)
        {
            double x = 0;
            for (int i = 0; i < context.ItemCount; i++)
            {
                var child = context.GetElementAt(i);
                var childRect = new Rect(x, 0, ColumnWidths[i], finalSize.Height);
                child.Arrange(childRect);
                x += childRect.Width;
            }

            return finalSize;
        }
    }
}
