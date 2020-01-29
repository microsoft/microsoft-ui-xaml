// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using Windows.Foundation;
using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using VirtualizingLayoutContext = Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext;

namespace MUXControlsTestApp.Samples
{
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