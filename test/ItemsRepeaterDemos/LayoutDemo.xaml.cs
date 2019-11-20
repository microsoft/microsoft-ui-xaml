using Microsoft.UI.Xaml.Controls;
using System;
using System.Diagnostics;
using System.Linq;
using Windows.Foundation;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    public sealed partial class LayoutDemo : Page
    {
        public LayoutDemo()
        {
            this.InitializeComponent();
            repeater.ItemsSource = Enumerable.Range(0, 100);
        }
    }




    public class NonVirtualStackLayout : NonVirtualizingLayout
    {
        protected override Size MeasureOverride(NonVirtualizingLayoutContext context, Size availableSize)
        {
            double extentHeight = 0.0;
            double extentWidth = 0.0;
            foreach (var element in context.Children)
            {
                element.Measure(availableSize);
                extentHeight += element.DesiredSize.Height;
                extentWidth = Math.Max(extentWidth, element.DesiredSize.Width);
            }

            return new Size(extentWidth, extentHeight);
        }

        protected override Size ArrangeOverride(NonVirtualizingLayoutContext context, Size finalSize)
        {
            double offset = 0.0;
            foreach (var element in context.Children)
            {
                element.Arrange(new Rect(0, offset, element.DesiredSize.Width, element.DesiredSize.Height));
                offset += element.DesiredSize.Height;
            }

            return finalSize;
        }
    }

    public class VirtualizingUniformStackLayout : VirtualizingLayout
    {
        public double ItemWidth { get; set; }
        public double ItemHeight { get; set; }

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
                var element = context.GetOrCreateElementAt(currentIndex);
                element.Measure(new Size(ItemWidth, ItemHeight));
            }

            // Anchor 
            var anchorIndex = context.RecommendedAnchorIndex;
            if (anchorIndex >= 0)
            {
                var anchorElement = context.GetOrCreateElementAt(anchorIndex);
                anchorElement.Measure(new Size(ItemWidth, ItemHeight));
            }

            return new Size(ItemWidth, context.ItemCount * ItemHeight);
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
                var element = context.GetOrCreateElementAt(currentIndex);
                var arrangeRect = new Rect(0, currentIndex * ItemHeight, ItemWidth, ItemHeight);
                element.Arrange(arrangeRect);
                Debug.WriteLine("   Arrange:" + currentIndex + " :" + arrangeRect);
            }

            // Anchor 
            var anchorIndex = context.RecommendedAnchorIndex;
            if (anchorIndex >= 0)
            {
                var anchor = context.GetOrCreateElementAt(anchorIndex);
                var arrangeRect = new Rect(0, anchorIndex * ItemHeight, ItemWidth, ItemHeight);
                anchor.Arrange(arrangeRect);
                Debug.WriteLine("   Arrange:" + anchorIndex + " :" + arrangeRect);
            }

            return finalSize;
        }

        #region Private Helper Methods

        private int FirstRealizedIndexInRect(Rect realizationRect, int itemCount)
        {
            return Math.Max(0, Math.Min((int)(realizationRect.Y / ItemHeight), itemCount - 1));
        }

        private int LastRealizedIndexInRect(Rect realizationRect, int itemCount)
        {
            int index = realizationRect.Bottom == double.PositiveInfinity ? itemCount - 1 : (int)(realizationRect.Bottom / ItemHeight);
            return Math.Max(0, Math.Min(index, itemCount - 1));
        }

        #endregion Private Methods
    }

}
