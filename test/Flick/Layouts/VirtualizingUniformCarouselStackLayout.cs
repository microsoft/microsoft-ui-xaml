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
        public Rect RealizationRect { get; set; }
        public Rect ViewportRect
        {
            get
            {
                var viewportWidth = RealizationRect.Width / (1 + HorizontalCacheLength);
                var viewportHeight = RealizationRect.Height;
                var viewportXCoordinate = RealizationRect.X + (RealizationRect.Width - viewportWidth) / 2;
                var viewportYCoordinate = RealizationRect.Y;
                return new Rect(viewportXCoordinate, viewportYCoordinate, viewportWidth, viewportHeight);
            }
        }

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

        // Ratio to scale deselected items relative to the selected item
        public Double ItemScaleRatio
        {
            get { return (Double)GetValue(ItemScaleRatioProperty); }
            set
            {
                if (value <= 0 || value > 1)
                {
                    throw new ArgumentException(String.Format("{0} must be a number x where 0 < x <= 1", "ItemScaleRatio"));
                }

                SetValue(ItemScaleRatioProperty, value);
            }
        }

        public static readonly DependencyProperty ItemScaleRatioProperty = DependencyProperty.Register(
            "ItemScaleRatio", typeof(Double), typeof(VirtualizingUniformCarouselStackLayout), new PropertyMetadata(1.0));

        public Thickness Margin
        {
            get { return (Thickness)GetValue(MarginProperty); }
            set { SetValue(MarginProperty, value); }
        }

        public static readonly DependencyProperty MarginProperty = DependencyProperty.Register(
            "Margin", typeof(Thickness), typeof(VirtualizingUniformCarouselStackLayout), new PropertyMetadata(new Thickness(0)));

        public int MaxNumberOfItemsThatCanFitInViewport { get; set; } = 0;

        public float FirstSnapPointOffset { get; private set; } = 0.0f;

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var realizationRect = context.RealizationRect;

            if (RealizationRect.X != realizationRect.X
                || RealizationRect.Y != realizationRect.Y
                || RealizationRect.Width != realizationRect.Width
                || RealizationRect.Height != realizationRect.Height)
            {
                RealizationRect = new Rect(realizationRect.X, realizationRect.Y, realizationRect.Width, realizationRect.Height);
            }

            int itemCount = context.ItemCount;

            // Max number of items that can fit in the viewport after animations have been applied
            // (e.g. if ItemScaleRatio < 1.0 then more items will be able to fit in the viewport than could fit if the items weren't animated)
            int maxNumberOfItemsThatCanFitInViewport = 1 + (int)Math.Ceiling((ViewportRect.Width - ItemWidth - Spacing) / (ItemWidth * ItemScaleRatio + Spacing));

            if (maxNumberOfItemsThatCanFitInViewport != MaxNumberOfItemsThatCanFitInViewport)
            {
                MaxNumberOfItemsThatCanFitInViewport = maxNumberOfItemsThatCanFitInViewport;
            }

            if (realizationRect.Width == 0 || itemCount == 0)
            {
                return new Size(0, ItemHeight);
            }
            else if (itemCount == 1)
            {
                var marginLeftRight = (ViewportRect.Width - ItemWidth) / 2;
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
            else if (itemCount < maxNumberOfItemsThatCanFitInViewport)
            {
                var scrollViewerExtentWidthWithoutMargin = (((ItemWidth + Spacing) * itemCount) - Spacing);
                var differenceBetweenExtentWidthWithoutMarginAndViewportWidth = ViewportRect.Width - scrollViewerExtentWidthWithoutMargin;
                double marginLeftRight;
                var marginTopBottom = 0;

                if ((itemCount % 2) == 0)
                {
                    marginLeftRight = ((((ItemWidth + Spacing) / 2) + (((itemCount / 2) - 1) * (ItemWidth + Spacing))) + (differenceBetweenExtentWidthWithoutMarginAndViewportWidth / 2));
                }
                else
                {
                    marginLeftRight = (((itemCount / 2) * (ItemWidth + Spacing)) + (differenceBetweenExtentWidthWithoutMarginAndViewportWidth / 2));
                }

                if (Margin.Left != marginLeftRight
                    || Margin.Top != marginTopBottom
                    || Margin.Right != marginLeftRight
                    || Margin.Bottom != marginTopBottom)
                {
                    Margin = new Thickness(marginLeftRight, marginTopBottom, marginLeftRight, marginTopBottom);
                }

                for (int i = 0; i < itemCount; ++i)
                {
                    var element = context.GetOrCreateElementAt(i);
                    element.Measure(new Size(ItemWidth, ItemHeight));
                }

                return new Size(Margin.Left + ((ItemWidth + Spacing) * itemCount) - Spacing + Margin.Right, ItemHeight);
            }
            else
            {
                var marginLeftRight = 0;
                var marginTopBottom = 0;

                if (Margin.Left != marginLeftRight
                    || Margin.Top != marginTopBottom
                    || Margin.Right != marginLeftRight
                    || Margin.Bottom != marginTopBottom)
                {
                    Margin = new Thickness(marginLeftRight, marginTopBottom, marginLeftRight, marginTopBottom);
                }

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

                return new Size(((ItemWidth + Spacing) * context.ItemCount * RepeatCount) - Spacing, ItemHeight);
            }
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {
            var realizationRect = context.RealizationRect;

            if (RealizationRect.X != realizationRect.X
                || RealizationRect.Y != realizationRect.Y
                || RealizationRect.Width != realizationRect.Width
                || RealizationRect.Height != realizationRect.Height)
            {
                RealizationRect = new Rect(realizationRect.X, realizationRect.Y, realizationRect.Width, realizationRect.Height);
            }

            int itemCount = context.ItemCount;

            // Max number of items that can fit in the viewport after animations have been applied
            // (e.g. if ItemScaleRatio < 1.0 then more items will be able to fit in the viewport than could fit if the items weren't animated)
            int maxNumberOfItemsThatCanFitInViewport = 1 + (int)Math.Ceiling((ViewportRect.Width - ItemWidth - Spacing) / (ItemWidth * ItemScaleRatio + Spacing));

            if (maxNumberOfItemsThatCanFitInViewport != MaxNumberOfItemsThatCanFitInViewport)
            {
                MaxNumberOfItemsThatCanFitInViewport = maxNumberOfItemsThatCanFitInViewport;
            }

            if (itemCount == 1)
            {
                var marginLeftRight = (ViewportRect.Width - ItemWidth) / 2;
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

                float firstSnapPointOffset = (float)(arrangeRect.X + (ItemWidth / 2));

                if (FirstSnapPointOffset != firstSnapPointOffset)
                {
                    FirstSnapPointOffset = firstSnapPointOffset;
                }

                element.Arrange(arrangeRect);
                Debug.WriteLine("   Arrange:" + realIndex + " :" + arrangeRect);
                return finalSize;
            }
            else if (itemCount < maxNumberOfItemsThatCanFitInViewport)
            {
                var scrollViewerExtentWidthWithoutMargin = (((ItemWidth + Spacing) * itemCount) - Spacing);
                var differenceBetweenExtentWidthWithoutMarginAndViewportWidth = ViewportRect.Width - scrollViewerExtentWidthWithoutMargin;
                double marginLeftRight;
                var marginTopBottom = 0;

                if ((itemCount % 2) == 0)
                {
                    marginLeftRight = ((((ItemWidth + Spacing) / 2) + (((itemCount / 2) - 1) * (ItemWidth + Spacing))) + (differenceBetweenExtentWidthWithoutMarginAndViewportWidth / 2));
                }
                else
                {
                    marginLeftRight = (((itemCount / 2) * (ItemWidth + Spacing)) + (differenceBetweenExtentWidthWithoutMarginAndViewportWidth / 2));
                }

                if (Margin.Left != marginLeftRight
                    || Margin.Top != marginTopBottom
                    || Margin.Right != marginLeftRight
                    || Margin.Bottom != marginTopBottom)
                {
                    Margin = new Thickness(marginLeftRight, marginTopBottom, marginLeftRight, marginTopBottom);
                }

                for (int i = 0; i < itemCount; ++i)
                {
                    var element = context.GetOrCreateElementAt(i);
                    double arrangeRectX = (finalSize.Width / 2); // center of extent

                    if ((itemCount % 2) == 0)
                    {
                        if (i < (itemCount / 2))
                        {
                            arrangeRectX -= (((Spacing / 2) + ItemWidth) + ((((itemCount / 2) - 1) - i) * (ItemWidth + Spacing)));
                        }
                        else
                        {
                            arrangeRectX += ((Spacing / 2) + ((i - (itemCount / 2)) * (ItemWidth + Spacing)));
                        }
                    }
                    else
                    {
                        if (i == (itemCount / 2))
                        {
                            arrangeRectX -= (ItemWidth / 2);
                        }
                        else if (i < (itemCount / 2))
                        {
                            arrangeRectX -= ((ItemWidth / 2) + (((itemCount / 2) - i) * (ItemWidth + Spacing)));
                        }
                        else
                        {
                            arrangeRectX += (((ItemWidth / 2) + Spacing) + (((i - (itemCount / 2)) - 1) * (ItemWidth + Spacing)));
                        }
                    }

                    if (i == 0)
                    {
                        float firstSnapPointOffset = (float)(arrangeRectX + (ItemWidth / 2));

                        if (FirstSnapPointOffset != firstSnapPointOffset)
                        {
                            FirstSnapPointOffset = firstSnapPointOffset;
                        }
                    }

                    var arrangeRect = new Rect(arrangeRectX, 0, ItemWidth, ItemHeight);
                    element.Arrange(arrangeRect);
                    Debug.WriteLine("   Arrange:" + i + " :" + arrangeRect);
                }
            }
            else
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

                float firstSnapPointOffset = (float)(Margin.Left + (ItemWidth / 2));

                if (FirstSnapPointOffset != firstSnapPointOffset)
                {
                    FirstSnapPointOffset = firstSnapPointOffset;
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