// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Linq;
using Windows.Foundation;

using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using VirtualizingLayoutContext = Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext;

namespace MUXControlsTestApp.Samples
{
    // This is a sample layout that stacks elements one after
    // the other where each item can be of variable height. This is
    // also a virtualizing layout - we measure and arrange only elements
    // that are in the viewport. Not measuring/arranging all elements means
    // that we do not have the complete picture and need to estimate sometimes.
    // For example the size of the layout (extent) is an estimation based on the 
    // average heights we have seen so far. Also, if you drag the mouse thumb 
    // and yank it quickly, then we estimate what goes in the new viewport. 

    // The layout caches the bounds of everything that are in the current viewport.
    // During measure, we might get a suggested anchor (or start index), we use that
    // index to start and layout the rest of the items in the viewport relative to that
    // index. Note that since we are estimating, we can end up with negative origin when
    // the viewport is somewhere in the middle of the extent. This is achieved by setting the
    // LayoutOrigin property on the context. Once this is set, future viewport will account 
    // for the origin.
    public class VirtualizingStackLayout : VirtualizingLayout
    {
        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            var viewport = context.RealizationRect;
            DebugTrace("MeasureOverride: Viewport " + viewport);

            // Remove bounds for elements that are now outside the viewport.
            // Proactive recycling elements means we can reuse it during this measure pass again.
            RemoveCachedBoundsOutsideViewport(viewport);

            // Find the index of the element to start laying out from - the anchor 
            int startIndex = GetStartIndex(context, availableSize);

            // Measure and layout elements starting from the start index, forward and backward.
            Generate(context, availableSize, startIndex, forward:true);
            Generate(context, availableSize, startIndex, forward:false);

            // Estimate the extent size. Note that this can have a non 0 origin.
            m_lastExtent = EstimateExtent(context, availableSize);
            context.LayoutOrigin = new Point(m_lastExtent.X, m_lastExtent.Y);
            return new Size(m_lastExtent.Width, m_lastExtent.Height);
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {
            DebugTrace("ArrangeOverride: Viewport" + context.RealizationRect);
            for (int realizationIndex = 0; realizationIndex < m_realizedElementBounds.Count; realizationIndex++)
            {
                int currentDataIndex = m_firstRealizedDataIndex + realizationIndex;
                DebugTrace("Arranging " + currentDataIndex);

                // Arrange the child. If any alignment needs to be done, it
                // can be done here.
                var child = context.GetOrCreateElementAt(currentDataIndex);
                var arrangeBounds = m_realizedElementBounds[realizationIndex];
                arrangeBounds.X -= m_lastExtent.X;
                arrangeBounds.Y -= m_lastExtent.Y;
                child.Arrange(arrangeBounds);
            }

            return finalSize;
        }

        // The data collection has changed, since we are maintaining the bounds of elements
        // in the viewport, we will update the list to account for the collection change.
        protected override void OnItemsChangedCore(VirtualizingLayoutContext context, object source, NotifyCollectionChangedEventArgs args)
        {
            InvalidateMeasure();
            if (m_realizedElementBounds.Count > 0)
            {
                switch (args.Action)
                {
                    case NotifyCollectionChangedAction.Add:
                        OnItemsAdded(args.NewStartingIndex, args.NewItems.Count);
                        break;
                    case NotifyCollectionChangedAction.Replace:
                        OnItemsRemoved(args.OldStartingIndex, args.OldItems.Count);
                        OnItemsAdded(args.NewStartingIndex, args.NewItems.Count);
                        break;
                    case NotifyCollectionChangedAction.Remove:
                        OnItemsRemoved(args.OldStartingIndex, args.OldItems.Count);
                        break;
                    case NotifyCollectionChangedAction.Reset:
                        m_realizedElementBounds.Clear();
                        m_firstRealizedDataIndex = 0;
                        break;
                    default:
                        throw new NotImplementedException();
                }
            }
        }

        private void Generate(VirtualizingLayoutContext context, Size availableSize, int anchorDataIndex, bool forward)
        {
            // Generate forward or backward from anchorIndex until we hit the end of the viewport
            int step = forward ? 1 : -1;
            int previousDataIndex = anchorDataIndex;
            int currentDataIndex = previousDataIndex + step;
            var viewport = context.RealizationRect;
            while (IsDataIndexValid(currentDataIndex, context.ItemCount) &&
                ShouldContinueFillingUpSpace(previousDataIndex, forward, viewport))
            {
                EnsureRealized(currentDataIndex);
                DebugTrace("Measuring " + currentDataIndex);
                var desiredSize = MeasureElement(context, currentDataIndex, availableSize);
                var previousBounds = GetCachedBoundsForDataIndex(previousDataIndex);
                Rect currentBounds = new Rect(0,
                                              forward ? previousBounds.Y + previousBounds.Height : previousBounds.Y - desiredSize.Height,
                                              availableSize.Width,
                                              desiredSize.Height);
                SetCachedBoundsForDataIndex(currentDataIndex, currentBounds);
                previousDataIndex = currentDataIndex;
                currentDataIndex += step;
            }
        }

        // Remove bounds that are outside the viewport, leaving one extra since our
        // generate stops after generating one extra to know that we are outside the
        // viewport.
        private void RemoveCachedBoundsOutsideViewport(Rect viewport)
        {
            int firstRealizedIndexInViewport = 0;
            while (firstRealizedIndexInViewport < m_realizedElementBounds.Count &&
                   !Intersects(m_realizedElementBounds[firstRealizedIndexInViewport], viewport))
            {
                firstRealizedIndexInViewport++;
            }

            int lastRealizedIndexInViewport = m_realizedElementBounds.Count - 1;
            while (lastRealizedIndexInViewport >= 0 &&
                !Intersects(m_realizedElementBounds[lastRealizedIndexInViewport], viewport))
            {
                lastRealizedIndexInViewport--;
            }

            if (firstRealizedIndexInViewport > 0)
            {
                m_firstRealizedDataIndex += firstRealizedIndexInViewport;
                m_realizedElementBounds.RemoveRange(0, firstRealizedIndexInViewport);
            }

            if (lastRealizedIndexInViewport >= 0 && lastRealizedIndexInViewport < m_realizedElementBounds.Count - 2)
            {
                m_realizedElementBounds.RemoveRange(lastRealizedIndexInViewport + 2, m_realizedElementBounds.Count - lastRealizedIndexInViewport - 3);
            }
        }

        bool Intersects(Rect bounds, Rect viewport)
        {
            return !(bounds.Bottom < viewport.Top ||
                bounds.Top > viewport.Bottom);
        }

        private bool ShouldContinueFillingUpSpace(int dataIndex, bool forward, Rect viewport)
        {
            var bounds = GetCachedBoundsForDataIndex(dataIndex);
            return forward ?
                bounds.Y < viewport.Bottom :
                bounds.Y > viewport.Top;
        }

        private bool IsDataIndexValid(int currentDataIndex, int itemCount)
        {
            return currentDataIndex >= 0 && currentDataIndex < itemCount;
        }

        // Figure out which index to use as the anchor and start laying out around.
        private int GetStartIndex(VirtualizingLayoutContext context, Size availableSize)
        {
            int startDataIndex = -1;
            var anchorIndex = context.RecommendedAnchorIndex;
            bool isAnchorValid = anchorIndex != -1;

            if (isAnchorValid)
            {
                if (IsRealized(anchorIndex))
                {
                    startDataIndex = anchorIndex;
                }
                else
                {
                    ClearRealizedRange();
                    startDataIndex = anchorIndex;
                }
            }
            else
            {
                // find first realized element that is visible in the viewport.
                startDataIndex = GetFirstRealizedDataIndexInViewport(context.RealizationRect);
                if (startDataIndex < 0)
                {
                    startDataIndex = EstimateIndexForViewport(context.RealizationRect, context.ItemCount);
                    ClearRealizedRange();
                }
            }

            // We have an anchorIndex, realize and measure it and
            // figure out its bounds.
            if (startDataIndex != -1 & context.ItemCount > 0)
            {
                if (m_realizedElementBounds.Count == 0)
                {
                    m_firstRealizedDataIndex = startDataIndex;
                }

                var newAnchor = EnsureRealized(startDataIndex);
                DebugTrace("Measuring start index " + startDataIndex);
                var desiredSize = MeasureElement(context, startDataIndex, availableSize);

                var bounds = new Rect(
                    0,
                    newAnchor ?
                        (m_totalHeightForEstimation / m_numItemsUsedForEstimation) * startDataIndex :
                        GetCachedBoundsForDataIndex(startDataIndex).Y,
                    availableSize.Width,
                    desiredSize.Height);
                SetCachedBoundsForDataIndex(startDataIndex, bounds);
            }

            return startDataIndex;
        }

        private int EstimateIndexForViewport(Rect viewport, int dataCount)
        {
            double averageHeight = m_totalHeightForEstimation / m_numItemsUsedForEstimation;
            int estimatedIndex = (int)(viewport.Top / averageHeight);
            // clamp to an index within the collection
            estimatedIndex = Math.Max(0, Math.Min(estimatedIndex, dataCount));
            return estimatedIndex;
        }

        private int GetFirstRealizedDataIndexInViewport(Rect viewport)
        {
            int index = -1;
            if (m_realizedElementBounds.Count > 0)
            {
                for (int i = 0; i < m_realizedElementBounds.Count; i++)
                {
                    if (m_realizedElementBounds[i].Y < viewport.Bottom &&
                       m_realizedElementBounds[i].Bottom > viewport.Top)
                    {
                        index = m_firstRealizedDataIndex + i;
                        break;
                    }
                }
            }

            return index;
        }

        private Size MeasureElement(VirtualizingLayoutContext context, int index, Size availableSize)
        {
            var child = context.GetOrCreateElementAt(index);
            child.Measure(availableSize);

            int estimationBufferIndex = index % m_estimationBuffer.Count;
            bool alreadyMeasured = m_estimationBuffer[estimationBufferIndex] != 0;
            if (!alreadyMeasured)
            {
                m_numItemsUsedForEstimation++;
            }

            m_totalHeightForEstimation -= m_estimationBuffer[estimationBufferIndex];
            m_totalHeightForEstimation += child.DesiredSize.Height;
            m_estimationBuffer[estimationBufferIndex] = child.DesiredSize.Height;

            return child.DesiredSize;
        }

        private bool EnsureRealized(int dataIndex)
        {
            if (!IsRealized(dataIndex))
            {
                int realizationIndex = RealizationIndex(dataIndex);
                Debug.Assert(dataIndex == m_firstRealizedDataIndex - 1 ||
                    dataIndex == m_firstRealizedDataIndex + m_realizedElementBounds.Count ||
                    m_realizedElementBounds.Count == 0);

                if (realizationIndex == -1)
                {
                    m_realizedElementBounds.Insert(0, new Rect());
                }
                else
                {
                    m_realizedElementBounds.Add(new Rect());
                }

                if (m_firstRealizedDataIndex > dataIndex)
                {
                    m_firstRealizedDataIndex = dataIndex;
                }

                return true;
            }

            return false;
        }

        // Figure out the extent of the layout by getting the number of items remaining
        // above and below the realized elements and getting an estimation based on 
        // average item heights seen so far.
        private Rect EstimateExtent(VirtualizingLayoutContext context, Size availableSize)
        {
            double averageHeight = m_totalHeightForEstimation / m_numItemsUsedForEstimation;

            Rect extent = new Rect(0, 0, availableSize.Width, context.ItemCount * averageHeight);

            if (context.ItemCount > 0 && m_realizedElementBounds.Count > 0)
            {
                extent.Y = m_firstRealizedDataIndex == 0 ?
                                m_realizedElementBounds[0].Y :
                                m_realizedElementBounds[0].Y - (m_firstRealizedDataIndex - 1) * averageHeight;

                int lastRealizedIndex = m_firstRealizedDataIndex + m_realizedElementBounds.Count;
                if (lastRealizedIndex == context.ItemCount - 1)
                {
                    var lastBounds = m_realizedElementBounds[m_realizedElementBounds.Count - 1];
                    extent.Y = lastBounds.Bottom;
                }
                else
                {
                    var lastBounds = m_realizedElementBounds[m_realizedElementBounds.Count - 1];
                    int lastRealizedDataIndex = m_firstRealizedDataIndex + m_realizedElementBounds.Count;
                    int numItemsAfterLastRealizedIndex = context.ItemCount - lastRealizedDataIndex;
                    extent.Height = lastBounds.Bottom + numItemsAfterLastRealizedIndex * averageHeight - extent.Y;
                }
            }

            DebugTrace("Extent " + extent + " with average height " + averageHeight);
            return extent;
        }

        private bool IsRealized(int dataIndex)
        {
            int realizationIndex = dataIndex - m_firstRealizedDataIndex;
            return realizationIndex >= 0 && realizationIndex < m_realizedElementBounds.Count;
        }

        // Index in the m_realizedElementBounds collection
        private int RealizationIndex(int dataIndex)
        {
            return dataIndex - m_firstRealizedDataIndex;
        }

        private void OnItemsAdded(int index, int count)
        {
            // Using the old indexes here (before it was updated by the collection change)
            // if the insert data index is between the first and last realized data index, we need
            // to insert items.
            int lastRealizedDataIndex = m_firstRealizedDataIndex + m_realizedElementBounds.Count - 1;
            int newStartingIndex = index;
            if (newStartingIndex > m_firstRealizedDataIndex &&
                newStartingIndex <= lastRealizedDataIndex)
            {
                // Inserted within the realized range
                int insertRangeStartIndex = newStartingIndex - m_firstRealizedDataIndex;
                for (int i = 0; i < count; i++)
                {
                    // Insert null (sentinel) here instead of an element, that way we do not 
                    // end up creating a lot of elements only to be thrown out in the next layout.
                    int insertRangeIndex = insertRangeStartIndex + i;
                    int dataIndex = newStartingIndex + i;
                    // This is to keep the contiguousness of the mapping
                    m_realizedElementBounds.Insert(insertRangeIndex, new Rect());
                }
            }
            else if (index <= m_firstRealizedDataIndex)
            {
                // Items were inserted before the realized range.
                // We need to update m_firstRealizedDataIndex;
                m_firstRealizedDataIndex += count;
            }
        }

        private void OnItemsRemoved(int index, int count)
        {
            int lastRealizedDataIndex = m_firstRealizedDataIndex + m_realizedElementBounds.Count - 1;
            int startIndex = Math.Max(m_firstRealizedDataIndex, index);
            int endIndex = Math.Min(lastRealizedDataIndex, index + count - 1);
            bool removeAffectsFirstRealizedDataIndex = (index <= m_firstRealizedDataIndex);

            if (endIndex >= startIndex)
            {
                ClearRealizedRange(RealizationIndex(startIndex), endIndex - startIndex + 1);
            }

            if (removeAffectsFirstRealizedDataIndex &&
                m_firstRealizedDataIndex != -1)
            {
                m_firstRealizedDataIndex -= count;
            }
        }

        private void ClearRealizedRange(int startRealizedIndex, int count)
        {
            m_realizedElementBounds.RemoveRange(startRealizedIndex, count);
            if (startRealizedIndex == 0)
            {
                m_firstRealizedDataIndex = m_realizedElementBounds.Count == 0 ? 0 : m_firstRealizedDataIndex + count;
            }
        }

        private void ClearRealizedRange()
        {
            m_realizedElementBounds.Clear();
            m_firstRealizedDataIndex = 0;
        }

        private Rect GetCachedBoundsForDataIndex(int dataIndex)
        {
            return m_realizedElementBounds[RealizationIndex(dataIndex)];
        }

        private void SetCachedBoundsForDataIndex(int dataIndex, Rect bounds)
        {
            m_realizedElementBounds[RealizationIndex(dataIndex)] = bounds;
        }

        private Rect GetCachedBoundsForRealizationIndex(int relativeIndex)
        {
            return m_realizedElementBounds[relativeIndex];
        }

        void DebugTrace(string message, params object[] args)
        {
            Debug.WriteLine(message, args);
        }

        // Estimation state
        List<double> m_estimationBuffer = Enumerable.Repeat(0d, 100).ToList();
        int m_numItemsUsedForEstimation = 0;
        double m_totalHeightForEstimation = 0;

        // State to keep track of realized bounds
        int m_firstRealizedDataIndex = 0;
        List<Rect> m_realizedElementBounds = new List<Rect>();

        Rect m_lastExtent = new Rect();
    }
}
