// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using Windows.Foundation;
using Windows.UI.Xaml;
using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using VirtualizingLayoutContext = Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext;

namespace MUXControlsTestApp.Samples
{
    /// <summary>
    ///  This is a custom layout that displays elements in two different sizes
    ///  wide (w) and narrow (n). There are two types of rows 
    ///  odd rows - narrow narrow wide
    ///  even rows - wide narrow narrow
    ///  This pattern repeats.
    /// </summary>
    public class ActivityFeedLayout : VirtualizingLayout
    {
        #region Layout parameters

        public double RowSpacing
        {
            get { return _rowSpacing; }
            set { SetValue(RowSpacingProperty, value); }
        }

        public static readonly DependencyProperty RowSpacingProperty =
            DependencyProperty.Register("RowSpacing", typeof(double), typeof(ActivityFeedLayout), new PropertyMetadata(0, OnPropertyChanged));

        public double ItemSpacing
        {
            get { return _itemSpacing; }
            set { SetValue(ItemSpacingProperty, value); }
        }

        public static readonly DependencyProperty ItemSpacingProperty =
            DependencyProperty.Register("ItemSpacing", typeof(double), typeof(ActivityFeedLayout), new PropertyMetadata(0, OnPropertyChanged));

        public Size MinItemSize
        {
            get { return _minItemSize; }
            set { SetValue(MinItemSizeProperty, value); }
        }

        public static readonly DependencyProperty MinItemSizeProperty =
            DependencyProperty.Register("MinItemSize", typeof(Size), typeof(ActivityFeedLayout), new PropertyMetadata(0, OnPropertyChanged));

        #endregion

        #region Setup / teardown

        protected override void InitializeForContextCore(VirtualizingLayoutContext context)
        {
            base.InitializeForContextCore(context);

            var state = context.LayoutState as ActivityFeedLayoutState;
            if (state == null)
            {
                // Store any state we might need since (in theory) the layout could be in use by multiple elements simultaneously
                // In reality for the Xbox Activity Feed there's probably only a single instance.
                context.LayoutState = new ActivityFeedLayoutState();
            }
        }

        protected override void UninitializeForContextCore(VirtualizingLayoutContext context)
        {
            base.UninitializeForContextCore(context);

            // clear any state
            context.LayoutState = null;
        }

        #endregion

        #region Layout

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            // In the Xbox scenario there isn't a user grabbing the window and resizing it.  Otherwise, it
            // might be useful to skip a full layout here and return a cached size until some condition is met
            // such as having enough room to fit another column.  That would require extra work like 
            // tracking some additional state, comparing the virtualizing rect with the cached value.
            var virtualizingContext = context as VirtualizingLayoutContext;
            var realizationRect = virtualizingContext.RealizationRect;

            // Determine which rows need to be realized
            var firstRowIndex = Math.Max((int)(realizationRect.Y / (this.MinItemSize.Height + this.RowSpacing)) - 1, 0);
            var lastRowIndex = Math.Min((int)(realizationRect.Bottom / (this.MinItemSize.Height + this.RowSpacing)) + 1, (int)(context.ItemCount / 3));

            // Determine which items fall on those rows and determine the rect for each item
            var state = context.LayoutState as ActivityFeedLayoutState;
            state.LayoutRects.Clear();
            state.FirstRealizedIndex = firstRowIndex * 3;

            for (int rowIndex = firstRowIndex; rowIndex < lastRowIndex; rowIndex++)
            {
                int firstItemIndex = rowIndex * 3;
                var boundsForCurrentRow = CalculateLayoutBoundsForRow(rowIndex);

                for (int columnIndex = 0; columnIndex < 3; columnIndex++)
                {
                    var index = firstItemIndex + columnIndex;
                    var rect = boundsForCurrentRow[index % 3];
                    var container = virtualizingContext.GetOrCreateElementAt(index);
                    container.Measure(new Size(boundsForCurrentRow[columnIndex].Width, boundsForCurrentRow[columnIndex].Height));
                    state.LayoutRects.Add(boundsForCurrentRow[columnIndex]);
                }
            }

            // estimate the extent by finding the last item and getting its bottom/right position
            var extentHeight = ((int)(context.ItemCount / 3) - 1) * (this.MinItemSize.Height + this.RowSpacing) + this.MinItemSize.Height;

            // Report a desired size for the layout
            return new Size(this.MinItemSize.Width * 4 + this.ItemSpacing * 2, extentHeight);
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {
            Debug.WriteLine("ActivityFeedLayout.ArrangeOverride:");
            // walk through the cache of containers and arrange
            var state = context.LayoutState as ActivityFeedLayoutState;
            var virtualContext = context as VirtualizingLayoutContext;
            int currentIndex = state.FirstRealizedIndex;
            foreach(var arrangeRect in state.LayoutRects)
            {
                Debug.WriteLine("ActivityFeedLayout.ArrangeOverride  Arranging " + currentIndex + " at " + arrangeRect);
                var container = virtualContext.GetOrCreateElementAt(currentIndex);
                container.Arrange(arrangeRect);
                currentIndex++;
            }

            return finalSize;
        }

        #endregion

        private Rect[] CalculateLayoutBoundsForRow(int rowIndex)
        {
            var boundsForRow = new Rect[3];

            var yoffset = rowIndex * (this.MinItemSize.Height + this.RowSpacing);
            boundsForRow[0].Y = boundsForRow[1].Y = boundsForRow[2].Y = yoffset;
            boundsForRow[0].Height = boundsForRow[1].Height = boundsForRow[2].Height = this.MinItemSize.Height;

            if (rowIndex % 2 == 0)
            {
                // Left tile (narrow)
                boundsForRow[0].X = 0;
                boundsForRow[0].Width = this.MinItemSize.Width;
                // Middle tile (narrow)
                boundsForRow[1].X = boundsForRow[0].Right + this.ItemSpacing;
                boundsForRow[1].Width = this.MinItemSize.Width;
                // Right tile (wide)
                boundsForRow[2].X = boundsForRow[1].Right + this.ItemSpacing;
                boundsForRow[2].Width = this.MinItemSize.Width * 2 + this.ItemSpacing;
            }
            else
            {
                // Left tile (wide)
                boundsForRow[0].X = 0;
                boundsForRow[0].Width = (this.MinItemSize.Width * 2 + this.ItemSpacing);
                // Middle tile (narrow)
                boundsForRow[1].X = boundsForRow[0].Right + this.ItemSpacing;
                boundsForRow[1].Width = this.MinItemSize.Width;
                // Right tile (narrow)
                boundsForRow[2].X = boundsForRow[1].Right + this.ItemSpacing;
                boundsForRow[2].Width = this.MinItemSize.Width;
            }

            return boundsForRow;
        }

        private static void OnPropertyChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            var layout = obj as ActivityFeedLayout;
            if (args.Property == RowSpacingProperty)
            {
                layout._rowSpacing = (double)args.NewValue;
            }
            else if (args.Property == ItemSpacingProperty)
            {
                layout._itemSpacing = (double)args.NewValue;
            }
            else if (args.Property == MinItemSizeProperty)
            {
                layout._minItemSize = (Size)args.NewValue;
            }
            else
            {
                throw new InvalidOperationException("Don't know what you are talking about !");
            }

            layout.InvalidateMeasure();
        }

        // Cached copy of dependency properties so that we do not have to do GetValue in the host path during
        // layout. This can be quite expensive due to the number of times we end up calling these.
        private double _rowSpacing;
        private double _itemSpacing;
        private Size _minItemSize;
    }

    internal class ActivityFeedLayoutState
    {
        public int FirstRealizedIndex { get; set; }
        /// <summary>
        /// List of indicies and their layout bounds.
        /// </summary>
        public List<Rect> LayoutRects
        {
            get
            {
                if(_layoutRects == null)
                {
                    _layoutRects = new List<Rect>();
                }

                return _layoutRects;
            }
        }

        private List<Rect> _layoutRects;
    }
}
