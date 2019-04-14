using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation;

namespace Flick
{
    public class UniformGrid2DLayout : VirtualizingLayout
    {
        public double ItemWidth { get; set; }
        public double ItemHeight { get; set; }
        public double Spacing { get; set; }

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var realizationRect = context.RealizationRect;
            int itemCount = context.ItemCount;
            var firstRow = FirstRealizedRowIndexInRect(realizationRect);
            var firstCol = FirsttRealizedColumnIndexInRect(realizationRect);
            var lastRow = LastRealizedRowIndexInRect(realizationRect);
            var lastCol = LastRealizedColumnIndexInRect(realizationRect);

            Debug.WriteLine("Measure:" + realizationRect.ToString());

            // Viewport + Buffer Rect.
            for (int rowIndex = firstRow; rowIndex <= lastRow; rowIndex++)
            {
                for (int colIndex = firstCol; colIndex <= lastCol; colIndex++)
                {
                    var realIndex = GetElementIndex(rowIndex, colIndex, context.ItemCount);
                    var element = context.GetOrCreateElementAt(realIndex);
                    element.Measure(new Size(ItemWidth, ItemHeight));
                }
            }

            int numRowsOrColumns = (int)Math.Sqrt(context.ItemCount);
            return new Size(ItemWidth * numRowsOrColumns * 1000, ItemHeight * numRowsOrColumns * 1000);
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {

            var realizationRect = context.RealizationRect;
            int itemCount = context.ItemCount;
            var firstRow = FirstRealizedRowIndexInRect(realizationRect);
            var firstCol = FirsttRealizedColumnIndexInRect(realizationRect);
            var lastRow = LastRealizedRowIndexInRect(realizationRect);
            var lastCol = LastRealizedColumnIndexInRect(realizationRect);

            Debug.WriteLine("Arrange:" + realizationRect.ToString());

            // Viewport + Buffer Rect.
            for (int rowIndex = firstRow; rowIndex <= lastRow; rowIndex++)
            {
                for (int colIndex = firstCol; colIndex <= lastCol; colIndex++)
                {
                    var realIndex = GetElementIndex(rowIndex, colIndex, context.ItemCount);
                    var element = context.GetOrCreateElementAt(realIndex);
                    var arrangeRect = new Rect(colIndex * (ItemWidth + Spacing), rowIndex * (ItemHeight + Spacing), ItemWidth, ItemHeight);
                    element.Arrange(arrangeRect);
                    Debug.WriteLine("   Arrange:" + realIndex + " :" + arrangeRect);
                }
            }

            return finalSize;
        }

        #region Private Helper Methods

        private int GetElementIndex(int row, int col, int count)
        {
            int numRowsOrColumns = (int)Math.Sqrt(count);
            return (row * numRowsOrColumns + col) % count;
        }

        private int FirstRealizedRowIndexInRect(Rect realizationRect)
        {
            return (int)(realizationRect.Y /(ItemHeight + Spacing));
        }

        private int LastRealizedRowIndexInRect(Rect realizationRect)
        {
            return (int)(realizationRect.Bottom / (ItemHeight + Spacing));
        }

        private int FirsttRealizedColumnIndexInRect(Rect realizationRect)
        {
            return (int)(realizationRect.X / (ItemWidth + Spacing));
        }

        private int LastRealizedColumnIndexInRect(Rect realizationRect)
        {
            return (int)(realizationRect.Right / (ItemWidth + Spacing));
        }

        #endregion Private Methods
    }
}
