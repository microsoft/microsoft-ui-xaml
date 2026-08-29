using Microsoft.UI.Xaml.Controls;
using System;
using System.Diagnostics;
using Windows.Foundation;

namespace Flick
{
    public class VirtualizingUniformCarouselStackLayout : VirtualizingLayout
    {
        public double ItemWidth { get; set; }
        public double ItemHeight { get; set; }
        public double Spacing { get; set; }

        // Number of times to repeat the count to give the 
        // illusion of infinite scrolling.
        public int RepeatCount { get; private set; } = 500;

        public bool Repeat { get; set; } = false;

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var realizationRect = context.RealizationRect;
            int itemCount = context.ItemCount;

            if (realizationRect.Width == 0 || realizationRect.Width < itemCount * (ItemWidth + Spacing)) // TODO: Better math
            {
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
            else
            {
                for(int i=0; i< itemCount; i++)
                {
                    var element = context.GetOrCreateElementAt(i);
                    element.Measure(new Size(ItemWidth, ItemHeight));
                }

                return new Size((ItemWidth + Spacing) * context.ItemCount - Spacing, ItemHeight);
            }
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {
            var realizationRect = context.RealizationRect;
            int itemCount = context.ItemCount;

            if (realizationRect.Width < itemCount * (ItemWidth + Spacing)) // TODO: Better math
            {
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
            }
            else
            {
                for (int i = 0; i < itemCount; i++)
                {
                    var element = context.GetOrCreateElementAt(i);
                    var arrangeRect = new Rect(i * (ItemWidth + Spacing), 0, ItemWidth, ItemHeight);
                    element.Arrange(arrangeRect);
                    Debug.WriteLine("   Arrange:" + i + " :" + arrangeRect);
                }
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
