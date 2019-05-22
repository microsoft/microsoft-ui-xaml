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
    public class VirtualizingUniformCarousalStackLayout : VirtualizingLayout
    {
        public double ItemWidth { get; set; }
        public double ItemHeight { get; set; }
        public double Spacing { get; set; }

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var realizationRect = context.RealizationRect;
            int itemCount = context.ItemCount;
            int firstRealizedIndex = FirstRealizedIndexInRect(realizationRect, itemCount);
            int lastRealizedIndex = LastRealizedIndexInRect(realizationRect, itemCount);
            Debug.WriteLine("Measure:" + realizationRect.ToString());

            // Viewport + Buffer Rect.
            for (int currentIndex = firstRealizedIndex; currentIndex <= lastRealizedIndex; currentIndex++)
            {
                var realIndex = Math.Abs(currentIndex % context.ItemCount);
                var element = context.GetOrCreateElementAt(realIndex);
                element.Measure(new Size(ItemWidth, ItemHeight));
            }

            return new Size(((ItemWidth + Spacing) * context.ItemCount * 1000) - Spacing, ItemHeight);
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {
            var realizationRect = context.RealizationRect;
            int itemCount = context.ItemCount;
            int firstRealizedIndex = FirstRealizedIndexInRect(realizationRect, itemCount);
            int lastRealizedIndex = LastRealizedIndexInRect(realizationRect, itemCount);
            Debug.WriteLine("Arrange:" + realizationRect.ToString());

            // Viewport + Buffer Rect.
            for (int currentIndex = firstRealizedIndex; currentIndex <= lastRealizedIndex; currentIndex++)
            {
                var realIndex = Math.Abs(currentIndex % context.ItemCount);
                var element = context.GetOrCreateElementAt(realIndex);
                var arrangeRect = new Rect(currentIndex * (ItemWidth + Spacing), 0, ItemWidth, ItemHeight);
                element.Arrange(arrangeRect);
                Debug.WriteLine("   Arrange:" + currentIndex + " :" + arrangeRect);
            }

            return finalSize;
        }

        #region Private Helper Methods

        private int FirstRealizedIndexInRect(Rect realizationRect, int itemCount)
        {
            return (int)(realizationRect.X / (ItemWidth + Spacing));
        }

        private int LastRealizedIndexInRect(Rect realizationRect, int itemCount)
        {
            int index = (int)(realizationRect.Right / (ItemWidth + Spacing));
            return index;
        }

        #endregion Private Methods
    }
}
