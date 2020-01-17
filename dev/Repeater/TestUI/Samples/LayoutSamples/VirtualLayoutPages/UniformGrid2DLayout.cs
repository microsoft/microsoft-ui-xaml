using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace Flick
{
    public class UniformGrid2DLayout : VirtualizingLayout
    {
        public double ItemWidth { get; set; }
        public double ItemHeight { get; set; }
        public double Spacing { get; set; }

        public double FocusedItemWidth { get; set; }
        public double FocusedItemHeight { get; set; }

        public bool EnlargeFocusedItem { get; set; }

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var realizationRect = context.RealizationRect;
            int itemCount = context.ItemCount;
            var firstRow = FirstRealizedRowIndexInRect(realizationRect, itemCount);
            var firstCol = FirsttRealizedColumnIndexInRect(realizationRect, itemCount);
            var lastRow = LastRealizedRowIndexInRect(realizationRect, itemCount);
            var lastCol = LastRealizedColumnIndexInRect(realizationRect, itemCount);

            Debug.WriteLine("Measure:" + realizationRect.ToString());

            // Viewport + Buffer Rect.
            for (int rowIndex = firstRow; rowIndex <= lastRow; rowIndex++)
            {
                for (int colIndex = firstCol; colIndex <= lastCol; colIndex++)
                {
                    var realIndex = GetElementIndex(rowIndex, colIndex, context.ItemCount);
                    if (realIndex >= 0 && realIndex < itemCount)
                    {
                        var element = context.GetOrCreateElementAt(realIndex);
                        if (EnlargeFocusedItem && FocusManager.GetFocusedElement() == element)
                        {
                            element.Measure(new Size(FocusedItemWidth, FocusedItemHeight));
                        }
                        else
                        {
                            element.Measure(new Size(ItemWidth, ItemHeight));
                        }
                    }
                }
            }

            int numRowsOrColumns = (int)Math.Sqrt(context.ItemCount);
            return new Size(ItemWidth * numRowsOrColumns, ItemHeight * numRowsOrColumns);
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {

            var realizationRect = context.RealizationRect;
            int itemCount = context.ItemCount;
            var firstRow = FirstRealizedRowIndexInRect(realizationRect, itemCount);
            var firstCol = FirsttRealizedColumnIndexInRect(realizationRect, itemCount);
            var lastRow = LastRealizedRowIndexInRect(realizationRect, itemCount);
            var lastCol = LastRealizedColumnIndexInRect(realizationRect, itemCount);

            Debug.WriteLine("Arrange:" + realizationRect.ToString());

            // Viewport + Buffer Rect.
            for (int rowIndex = firstRow; rowIndex <= lastRow; rowIndex++)
            {
                for (int colIndex = firstCol; colIndex <= lastCol; colIndex++)
                {
                    var realIndex = GetElementIndex(rowIndex, colIndex, context.ItemCount);
                    if (realIndex >= 0 && realIndex < itemCount)
                    {
                        var element = context.GetOrCreateElementAt(realIndex);
                        if (EnlargeFocusedItem && FocusManager.GetFocusedElement() == element)
                        {
                            var widthDiff = FocusedItemWidth - ItemWidth;
                            var heightDiff = FocusedItemHeight - ItemHeight;
                            var arrangeRect = new Rect(colIndex * (ItemWidth + Spacing) - widthDiff / 2, rowIndex * (ItemHeight + Spacing) - heightDiff / 2, FocusedItemWidth, FocusedItemHeight);
                            element.Arrange(arrangeRect);
                            Debug.WriteLine("   Arrange:" + realIndex + " :" + arrangeRect);
                            Canvas.SetZIndex(element, 1);
                        }
                        else
                        {
                            var arrangeRect = new Rect(colIndex * (ItemWidth + Spacing), rowIndex * (ItemHeight + Spacing), ItemWidth, ItemHeight);
                            element.Arrange(arrangeRect);
                            Debug.WriteLine("   Arrange:" + realIndex + " :" + arrangeRect);
                            Canvas.SetZIndex(element, 0);
                        }
                    }
                }
            }

            return finalSize;
        }

        #region Private Helper Methods

        private int GetElementIndex(int row, int col, int count)
        {
            int numRowsOrColumns = (int)Math.Sqrt(count);
            return (row * numRowsOrColumns + col);
        }

        private int FirstRealizedRowIndexInRect(Rect realizationRect, int itemCount)
        {
            return Clamp((int)(realizationRect.Y / (ItemHeight + Spacing)), itemCount);
        }

        private int LastRealizedRowIndexInRect(Rect realizationRect, int itemCount)
        {
            return Clamp((int)(realizationRect.Bottom / (ItemHeight + Spacing)), itemCount);
        }

        private int FirsttRealizedColumnIndexInRect(Rect realizationRect, int itemCount)
        {
            return Clamp((int)(realizationRect.X / (ItemWidth + Spacing)), itemCount);
        }

        private int LastRealizedColumnIndexInRect(Rect realizationRect, int itemCount)
        {
            return Clamp((int)(realizationRect.Right / (ItemWidth + Spacing)), itemCount);
        }

        private int Clamp(int index, int count)
        {
            return Math.Min(Math.Max(0, index), count - 1);
        }

        #endregion Private Methods
    }
}
