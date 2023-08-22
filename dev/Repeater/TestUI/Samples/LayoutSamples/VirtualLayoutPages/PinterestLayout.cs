// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Diagnostics;
using Windows.Foundation;

using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using VirtualizingLayoutContext = Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext;

namespace MUXControlsTestApp.Samples
{
    public class PinterestLayout : VirtualizingLayout
    {
        public PinterestLayout()
        {
            Width = 150.0;
        }

        public double Width { get; set; }

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var viewport = context.RealizationRect;
            Debug.WriteLine("Measure: " + viewport);

            if (availableSize.Width != m_lastAvailableWidth)
            {
                UpdateCachedBounds(availableSize);
                m_lastAvailableWidth = availableSize.Width;
            }

            // Initialize column offsets
            int numColumns = (int)(availableSize.Width / Width);
            if (m_columnOffsets.Count == 0)
            {
                for (int i = 0; i < numColumns; i++)
                {
                    m_columnOffsets.Add(0);
                }
            }

            m_firstIndex = GetStartIndex(viewport);
            int currentIndex = m_firstIndex;
            double nextOffset = -1.0;

            // Measure items from start index to when we hit the end of the viewport.
            while (currentIndex < context.ItemCount && nextOffset < viewport.Bottom)
            {
                Debug.WriteLine("Measuring " + currentIndex);
                var child = context.GetOrCreateElementAt(currentIndex);
                child.Measure(new Size(Width, availableSize.Height));

                if (currentIndex >= m_cachedBounds.Count)
                {
                    // We do not have bounds for this index. Lay it out and cache it.
                    int columnIndex = GetIndexOfLowestColumn(m_columnOffsets, out nextOffset);
                    m_cachedBounds.Add(new Rect(columnIndex * Width, nextOffset, Width, child.DesiredSize.Height));
                    m_columnOffsets[columnIndex] += child.DesiredSize.Height;
                }
                else
                {
                    if (currentIndex + 1 == m_cachedBounds.Count)
                    {
                        // Last element. Use the next offset.
                        GetIndexOfLowestColumn(m_columnOffsets, out nextOffset);
                    }
                    else
                    {
                        nextOffset = m_cachedBounds[currentIndex + 1].Top;
                    }
                }

                child.Arrange(m_cachedBounds[currentIndex]);

                m_lastIndex = currentIndex;
                currentIndex++;
            }

            var extent = GetExtentSize(availableSize);
            return extent;
        }

        // The children are arranged during measure, so ArrangeOverride can be a no-op.
        //protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        //{
        //    Debug.WriteLine("Arrange: " + context.RealizationRect);
        //    for (int index = m_firstIndex; index <= m_lastIndex; index++)
        //    {
        //        Debug.WriteLine("Arranging " + index);
        //        var child = context.GetElementAt(index);
        //        child.Arrange(m_cachedBounds[index]);
        //    }
        //    return finalSize;
        //}

        private void UpdateCachedBounds(Size availableSize)
        {
            int numColumns = (int)(availableSize.Width / Width);
            m_columnOffsets.Clear();
            for (int i = 0; i < numColumns; i++)
            {
                m_columnOffsets.Add(0);
            }

            for (int index = 0; index < m_cachedBounds.Count; index++)
            {
                double nextOffset = 0.0;
                int columnIndex = GetIndexOfLowestColumn(m_columnOffsets, out nextOffset);
                var oldHeight = m_cachedBounds[index].Height;
                m_cachedBounds[index] = new Rect(columnIndex * Width, nextOffset, Width, oldHeight);
                m_columnOffsets[columnIndex] += oldHeight;
            }
        }

        private int GetStartIndex(Rect viewport)
        {
            int startIndex = 0;
            if (m_cachedBounds.Count == 0)
            {
                startIndex = 0;
            }
            else
            {
                // find first index that intersects the viewport
                // perhaps this can be done more efficiently than walking
                // from the start of the list.
                for (int i = 0; i < m_cachedBounds.Count; i++)
                {
                    var currentBounds = m_cachedBounds[i];
                    if (currentBounds.Y < viewport.Bottom &&
                        currentBounds.Bottom > viewport.Top)
                    {
                        startIndex = i;
                        break;
                    }
                }
            }

            return startIndex;
        }

        private int GetIndexOfLowestColumn(List<double> columnOffsets, out double lowestOffset)
        {
            int lowestIndex = 0;
            lowestOffset = columnOffsets[lowestIndex];
            for (int index = 0; index < columnOffsets.Count; index++)
            {
                var currentOffset = columnOffsets[index];
                if (lowestOffset > currentOffset)
                {
                    lowestOffset = currentOffset;
                    lowestIndex = index;
                }
            }

            return lowestIndex;
        }

        private Size GetExtentSize(Size availableSize)
        {
            double largestColumnOffset = m_columnOffsets[0];
            for (int index = 0; index < m_columnOffsets.Count; index++)
            {
                var currentOffset = m_columnOffsets[index];
                if (largestColumnOffset < currentOffset)
                {
                    largestColumnOffset = currentOffset;
                }
            }

            return new Size(availableSize.Width, largestColumnOffset);
        }

        int m_firstIndex = 0;
        int m_lastIndex = 0;
        double m_lastAvailableWidth = 0.0;
        List<double> m_columnOffsets = new List<double>();
        List<Rect> m_cachedBounds = new List<Rect>();
    }
}
