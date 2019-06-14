using Microsoft.UI.Xaml.Controls;
using System;
using System.Diagnostics;
using Windows.Foundation;
using Windows.UI.Xaml;

namespace Flick
{
    public class VirtualizingUniformCarouselStackLayout : VirtualizingLayout
    {
        public double ItemWidth { get; set; }
        public double ItemHeight { get; set; }
        public double Spacing { get; set; }

        public Double HorizontalCacheLength
        {
            get { return (Double)GetValue(HorizontalCacheLengthProperty); }
            set { SetValue(HorizontalCacheLengthProperty, value); }
        }

        public static readonly DependencyProperty HorizontalCacheLengthProperty = DependencyProperty.Register(
            "HorizontalCacheLength", typeof(Double), typeof(VirtualizingUniformCarouselStackLayout), new PropertyMetadata(0.0));

        // Number of times to repeat the count to give the 
        // illusion of infinite scrolling.
        public int RepeatCount
        {
            get { return (int)GetValue(RepeatCountProperty); }
            set
            {
                if (value < 0)
                {
                    throw new ArgumentException(String.Format("{0} must be a non-negative integer", "RepeatCount"));
                }

                SetValue(RepeatCountProperty, value);
            }
        }

        public static readonly DependencyProperty RepeatCountProperty = DependencyProperty.Register(
            "RepeatCount", typeof(int), typeof(VirtualizingUniformCarouselStackLayout), new PropertyMetadata(500));

        public static readonly DependencyProperty VerticalScrollModeProperty = DependencyProperty.Register(
            "VerticalScrollMode", typeof(ScrollMode), typeof(VirtualizingUniformCarouselStackLayout), new PropertyMetadata(ScrollMode.Disabled));

        public Thickness Margin
        {
            get { return (Thickness)GetValue(MarginProperty); }
            set { SetValue(MarginProperty, value); }
        }

        public static readonly DependencyProperty MarginProperty = DependencyProperty.Register(
            "Margin", typeof(Thickness), typeof(VirtualizingUniformCarouselStackLayout), new PropertyMetadata(new Thickness(0)));

        public bool Repeat { get; set; } = false;

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var realizationRect = context.RealizationRect;
            var viewportWidth = realizationRect.Width / (1 + HorizontalCacheLength);
            var viewportHeight = realizationRect.Height;
            var viewportXCoordinate = realizationRect.X + (realizationRect.Width - viewportWidth) / 2;
            var viewportYCoordinate = realizationRect.Y;
            var viewportRect = new Rect(viewportXCoordinate, viewportYCoordinate, viewportWidth, viewportHeight);
            int itemCount = context.ItemCount;

            if (realizationRect.Width == 0 || itemCount == 0)
            {
                return new Size(0, ItemHeight);
            }

            if (itemCount == 1)
            {
                var marginLeftRight = (viewportRect.Width - ItemWidth) / 2;
                var marginTopBottom = 0;

                if (Margin.Left != marginLeftRight
                    || Margin.Top != marginTopBottom
                    || Margin.Right != marginLeftRight
                    || Margin.Bottom != marginTopBottom)
                {
                    Margin = new Thickness(marginLeftRight, marginTopBottom, marginLeftRight, marginTopBottom);
                }

                return new Size(Margin.Left + ItemWidth + Margin.Right, ItemHeight);
            }

            if (realizationRect.Width < itemCount * (ItemWidth + Spacing))
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
                for (int i = 0; i < itemCount; i++)
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
            var viewportWidth = realizationRect.Width / (1 + HorizontalCacheLength);
            var viewportHeight = realizationRect.Height;
            var viewportXCoordinate = realizationRect.X + (realizationRect.Width - viewportWidth) / 2;
            var viewportYCoordinate = realizationRect.Y;
            var viewportRect = new Rect(viewportXCoordinate, viewportYCoordinate, viewportWidth, viewportHeight);
            int itemCount = context.ItemCount;

            if (itemCount == 1)
            {
                var marginLeftRight = (viewportRect.Width - ItemWidth) / 2;
                var marginTopBottom = 0;

                if (Margin.Left != marginLeftRight
                    || Margin.Top != marginTopBottom
                    || Margin.Right != marginLeftRight
                    || Margin.Bottom != marginTopBottom)
                {
                    Margin = new Thickness(marginLeftRight, marginTopBottom, marginLeftRight, marginTopBottom);
                }

                var realIndex = 0;
                var element = context.GetOrCreateElementAt(realIndex);
                var arrangeRect = new Rect(Margin.Left, 0, ItemWidth, ItemHeight);
                element.Arrange(arrangeRect);
                Debug.WriteLine("   Arrange:" + realIndex + " :" + arrangeRect);
                return finalSize;
            }
            else if (realizationRect.Width < itemCount * (ItemWidth + Spacing)) // TODO: Better math
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