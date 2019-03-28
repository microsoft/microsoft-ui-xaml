using System;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using Debug = System.Diagnostics.Debug;
using ConditionalAttribute = System.Diagnostics.ConditionalAttribute;

namespace MUXControlsAdhocApp.GridPages
{
    public class GridTrackInfo
    {
        // TODO: a line can have more than one name. For example, here the second line will have two names: row1-end and row2-start:
        // grid-template-rows: [row1-start] 25% [row1-end row2-start] 25% [row2-end];
        // TODO: Line more refers to the space between two tracks, not a cell
        public string LineName { get; set; }

        // TODO: track-size can be a length, a percentage, or a fraction of the free space in the grid (fr)
        // grid-template-columns: 40px 50px auto 50px 40px;
        // grid-template-rows: 25% 100px auto;
        // The fr unit allows you to set the size of a track as a fraction of the free space of the grid container. For example, this will set each item to one third the width of the grid container:
        // grid-template-columns: 1fr 1fr 1fr;
        public double Length { get; set; } = 0;
        public bool Auto { get; set; } = false;
        public double Fraction { get; set; } = 0.0;
        public double Percentage { get; set; } = 0.0;
    }

    public class GridLocation
    {
        // TODO: These being of type int won't suffice. They can be:
        //<line> - can be a number to refer to a numbered grid line, or a name to refer to a named grid line
        //span<number> - the item will span across the provided number of grid tracks
        //span<name> - the item will span across until it hits the next line with the provided name
        //auto - indicates auto-placement, an automatic span, or a default span of one
        //  grid-column-start: <number> | <name> | span<number> | span<name> | auto
        //  grid-column-end: <number> | <name> | span<number> | span<name> | auto
        //  grid-row-start: <number> | <name> | span<number> | span<name> | auto
        //  grid-row-end: <number> | <name> | span<number> | span<name> | auto
        public int Index { get; set; } = -1;

        public string LineName { get; set; }

        public int Span { get; set; } = 0;
}

    public class Grid : Panel
    {
        public static readonly DependencyProperty ColumnStartProperty =
            DependencyProperty.RegisterAttached(
              "ColumnStart",
              typeof(GridLocation),
              typeof(Grid),
              new PropertyMetadata(null, new PropertyChangedCallback(InvalidateMeasureOnChildPropertyChanged))
            );
        public static void SetColumnStart(UIElement element, GridLocation value)
        {
            element.SetValue(ColumnStartProperty, value);
        }
        public static GridLocation GetColumnStart(UIElement element)
        {
            return (GridLocation)element.GetValue(ColumnStartProperty);
        }

        public static readonly DependencyProperty ColumnEndProperty =
            DependencyProperty.RegisterAttached(
              "ColumnEnd",
              typeof(GridLocation),
              typeof(Grid),
              new PropertyMetadata(null, new PropertyChangedCallback(InvalidateMeasureOnChildPropertyChanged))
            );
        public static void SetColumnEnd(UIElement element, GridLocation value)
        {
            element.SetValue(ColumnEndProperty, value);
        }
        public static GridLocation GetColumnEnd(UIElement element)
        {
            return (GridLocation)element.GetValue(ColumnEndProperty);
        }

        public static readonly DependencyProperty RowStartProperty =
            DependencyProperty.RegisterAttached(
              "RowStart",
              typeof(GridLocation),
              typeof(Grid),
              new PropertyMetadata(null, new PropertyChangedCallback(InvalidateMeasureOnChildPropertyChanged))
            );
        public static void SetRowStart(UIElement element, GridLocation value)
        {
            element.SetValue(RowStartProperty, value);
        }
        public static GridLocation GetRowStart(UIElement element)
        {
            return (GridLocation)element.GetValue(RowStartProperty);
        }

        public static readonly DependencyProperty RowEndProperty =
            DependencyProperty.RegisterAttached(
              "RowEnd",
              typeof(GridLocation),
              typeof(Grid),
              new PropertyMetadata(null, new PropertyChangedCallback(InvalidateMeasureOnChildPropertyChanged))
            );
        public static void SetRowEnd(UIElement element, GridLocation value)
        {
            element.SetValue(RowEndProperty, value);
        }
        public static GridLocation GetRowEnd(UIElement element)
        {
            return (GridLocation)element.GetValue(RowEndProperty);
        }

        public List<GridTrackInfo> TemplateColumns
        {
            get
            {
                return _templateColumns;
            }
            set
            {
                if (_templateColumns != value)
                {
                    _templateColumns = value;
                    InvalidateMeasure();
                }
            }
        }
        private List<GridTrackInfo> _templateColumns = new List<GridTrackInfo>();

        public List<GridTrackInfo> TemplateRows
        {
            get
            {
                return _templateRows;
            }
            set
            {
                if (_templateRows != value)
                {
                    _templateRows = value;
                    InvalidateMeasure();
                }
            }
        }
        private List<GridTrackInfo> _templateRows = new List<GridTrackInfo>();

        private static void InvalidateMeasureOnChildPropertyChanged(DependencyObject source, DependencyPropertyChangedEventArgs args)
        {
            Grid parent = Windows.UI.Xaml.Media.VisualTreeHelper.GetParent(source) as Grid;
            if (parent != null)
            {
                parent.InvalidateMeasure();
            }
        }

        // Calculated info on one of the grid tracks
        protected class MeasureInfo
        {
            public double Size;

            public double Start;
        }
        private Dictionary<GridTrackInfo, MeasureInfo> _columns = new Dictionary<GridTrackInfo, MeasureInfo>();
        private Dictionary<GridTrackInfo, MeasureInfo> _rows = new Dictionary<GridTrackInfo, MeasureInfo>();

        // Tracks all the intermediate calculations of one direction (row or column) of the grid
        protected struct MeasureBlah
        {
            public MeasureBlah(List<GridTrackInfo> template, Dictionary<GridTrackInfo, MeasureInfo> calculated)
            {
                Template = template;
                Calculated = calculated;
                Available = 0.0;
                TotalFixed = 0.0;
                TotalFraction = 0.0;
                Remaining = 0.0;
            }

            public List<GridTrackInfo> Template { get; private set; }
            public Dictionary<GridTrackInfo, MeasureInfo> Calculated { get; private set; }

            public double Available;
            public double TotalFixed;
            public double TotalFraction;
            public double Remaining;
        }

        private static void ProcessFixedSizes(List<GridTrackInfo> template, Dictionary<GridTrackInfo, MeasureInfo> calculated, double available, out MeasureBlah measure)
        {
            measure = new MeasureBlah(template, calculated) { Available = available };

            for (int i = 0; i < template.Count; i++)
            {
                double fixedSize = template[i].Length;

                // Percentage is effectively a fixed size in that it needs to be applied before any
                // of the more relative sizes (fraction, auto, etc.)
                if (fixedSize == 0.0)
                {
                    fixedSize = (template[i].Percentage * available);
                }

                measure.TotalFixed += fixedSize;

                // Accumulate the fractional sizes now so we know how many pieces of pie to dish out
                measure.TotalFraction += template[i].Fraction;

                calculated[template[i]] = new MeasureInfo { Size = fixedSize };
            }

            measure.Remaining = measure.Available - measure.TotalFixed;
        }

        private void ProcessAutoSizes(ref MeasureBlah measureHorizontal, ref MeasureBlah measureVertical)
        {
            foreach (UIElement child in Children)
            {
                ChildGridLocations childLocation = GetChildGridLocations(child);

                // If none of the grid tracks are Auto then we can skip this item
                if (!childLocation.ColStart.Auto && !childLocation.RowStart.Auto)
                {
                    continue;
                }

                // Measure this child to see how much of the available space they would take.
                // Note that even if there are multiple Autos we don't attempt to preemptively split 
                // the space between them. They are all given a crack at being greedy.
                Size measureSize = new Size(measureHorizontal.Remaining, measureVertical.Remaining);
                child.Measure(measureSize);

                // Update that row/column with the dimensions
                Action<GridTrackInfo, MeasureBlah, double> updateAutoBasedOnMeasured = (GridTrackInfo track, MeasureBlah measure, double childDesired) =>
                {
                    if (track.Auto)
                    {
                        MeasureInfo info = measure.Calculated[track];
                        if (childDesired > info.Size)
                        {
                            info.Size = childDesired;
                        }
                    }
                };
                updateAutoBasedOnMeasured(childLocation.ColStart, measureHorizontal, child.DesiredSize.Width);
                updateAutoBasedOnMeasured(childLocation.RowStart, measureVertical, child.DesiredSize.Height);
            }

            // TODO: Do we do anything with Remaining? Or is that left to be handled in Arrange?
        }

        private static void ProcessFractionalSizes(ref MeasureBlah measure)
        {
            if (measure.TotalFraction <= 0.0)
            {
                return;
            }

            // What is the size of each fraction?
            double fractionSlice = measure.Remaining / measure.TotalFraction;

            for (int i = 0; i < measure.Template.Count; i++)
            {
                GridTrackInfo track = measure.Template[i];
                if (track.Fraction == 0.0)
                {
                    continue;
                }

                // We only apply the fraction if the item didn't also have a fixed size
                MeasureInfo info = measure.Calculated[track];
                if (info.Size != 0.0)
                {
                    continue;
                }

                double myFractionSlice = track.Fraction * fractionSlice;
                info.Size = myFractionSlice;
            }

            // Fractions consume all that's left
            measure.Remaining = 0.0;
        }


        private double ProcessOffsets(ref MeasureBlah measure)
        {
            double offset = 0.0;
            foreach (var entry in measure.Calculated)
            {
                entry.Value.Start = offset;
                offset += entry.Value.Size;
            }

            return offset;
        }

        private struct ChildGridLocations
        {
            public GridTrackInfo ColStart;
            public GridTrackInfo RowStart;
            public GridTrackInfo ColEnd;
            public GridTrackInfo RowEnd;
        }

        private ChildGridLocations GetChildGridLocations(UIElement child)
        {
            ChildGridLocations result;

            // Read preferences off the child
            GridLocation colStart = GetColumnStart(child);
            GridLocation rowStart = GetRowStart(child);
            GridLocation colEnd = GetColumnEnd(child);
            GridLocation rowEnd = GetRowEnd(child);

            // Map those to our grid lines
            result.ColStart = GetTrack(_templateColumns, colStart);
            result.RowStart = GetTrack(_templateRows, rowStart);
            result.ColEnd = GetTrack(_templateColumns, colEnd, result.ColStart);
            result.RowEnd = GetTrack(_templateRows, rowEnd, result.RowStart);

            return result;
        }

        private GridTrackInfo GetTrack(List<GridTrackInfo> list, GridLocation location, GridTrackInfo previous = null)
        {
            if (location == null)
            {
                return null;
            }

            // Exact track index
            int index = location.Index;
            if (index >= 0 && index < list.Count)
            {
                return list[index];
            }

            // Friendly track name
            if (!String.IsNullOrEmpty(location.LineName))
            {
                foreach (GridTrackInfo track in list)
                {
                    if (track.LineName == location.LineName)
                    {
                        return track;
                    }
                }
            }

            // Span relative to previous track
            if (previous != null)
            {
                int span = location.Span;
                if (span == 0)
                {
                    span = 1;
                }

                int previousIndex = list.IndexOf(previous);
                if (previousIndex >= 0)
                {
                    int spanIndex = previousIndex + span;
                    spanIndex = Math.Min(spanIndex, list.Count - 1);

                    return list[spanIndex];
                }
            }

            return null;
        }

        private MeasureInfo GetMeasureInfo(GridTrackInfo info, Dictionary<GridTrackInfo, MeasureInfo> calculated)
        {
            return calculated[info];
        }

        [Conditional("TRACE")]
        private void DumpTemplates()
        {
            Action<List<GridTrackInfo>> dumpTemplate = (List<GridTrackInfo> template) =>
            {
                Debug.Indent();
                for (int i = 0; i < template.Count; i++)
                {
                    GridTrackInfo track = template[i];
                    Debug.WriteLine($"{i} {{LineName={track.LineName}, Length={track.Length}, Percentage={track.Percentage}, Fraction={track.Fraction}, Auto={track.Auto}}}");
                }
                Debug.Unindent();
            };

            Debug.WriteLine("TemplateColumns");
            dumpTemplate(_templateColumns);

            Debug.WriteLine("TemplateRows");
            dumpTemplate(_templateRows);
        }

        [Conditional("TRACE")]
        private void DumpMeasureInfo(ref MeasureBlah measure, string info)
        {
            Debug.WriteLine(info);

            Debug.Indent();
            foreach (var entry in measure.Calculated)
            {
                int trackIndex = measure.Template.IndexOf(entry.Key);
                Debug.WriteLine($"{trackIndex} {{Size={entry.Value.Size}, Start={entry.Value.Start}}}");
                
            }
            Debug.Unindent();
        }

        [Conditional("TRACE")]
        private void DumpMeasureInfo(ref MeasureBlah horizontalMeasure, ref MeasureBlah verticalMeasure, string info)
        {
            DumpMeasureInfo(ref horizontalMeasure, "Horizontal " + info);
            DumpMeasureInfo(ref verticalMeasure, "Vertical " + info);
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            DumpTemplates();

            _columns.Clear();
            _rows.Clear();

            // First process any fixed sizes
            MeasureBlah horizontalMeasure;
            MeasureBlah verticalMeasure;
            ProcessFixedSizes(_templateColumns, _columns, availableSize.Width, out horizontalMeasure);
            ProcessFixedSizes(_templateRows, _rows, availableSize.Height, out verticalMeasure);
            DumpMeasureInfo(ref horizontalMeasure, ref verticalMeasure, "fixed sizes");

            // Next we need to know how large the auto sizes are
            ProcessAutoSizes(ref horizontalMeasure, ref verticalMeasure);
            DumpMeasureInfo(ref horizontalMeasure, ref verticalMeasure, "auto sizes");

            // Then we can figure out how large the fractional sizes should be
            ProcessFractionalSizes(ref horizontalMeasure);
            ProcessFractionalSizes(ref verticalMeasure);
            DumpMeasureInfo(ref horizontalMeasure, ref verticalMeasure, "fractional sizes");

            Size usedSize = new Size(0, 0);

            foreach (UIElement child in Children)
            {
                ChildGridLocations childLocation = GetChildGridLocations(child);

                MeasureInfo colMeasure = GetMeasureInfo(childLocation.ColStart, _columns);
                MeasureInfo rowMeasure = GetMeasureInfo(childLocation.RowStart, _rows);

                // TODO: Relate to availableSize
                Size measureSize = new Size(colMeasure.Size, rowMeasure.Size);

                // Give each child the maximum available space
                child.Measure(measureSize);
            }

            // Now that the sizes are known we can calculate the offsets for the grid tracks
            double width = ProcessOffsets(ref horizontalMeasure);
            double height = ProcessOffsets(ref verticalMeasure);
            DumpMeasureInfo(ref horizontalMeasure, ref verticalMeasure, "offsets");

            // If there's no entry for columns/rows use the minimal size, otherwise use the whole space.
            if (_templateColumns.Count > 0)
            {
                width = availableSize.Width;
            }
            if (_templateRows.Count > 0)
            {
                height = availableSize.Height;
            }

            return new Size(width, height);
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            foreach (UIElement child in Children)
            {
                ChildGridLocations childLocation = GetChildGridLocations(child);

                MeasureInfo colMeasure = GetMeasureInfo(childLocation.ColStart, _columns);
                MeasureInfo rowMeasure = GetMeasureInfo(childLocation.RowStart, _rows);

                double left = colMeasure.Start;
                double top = rowMeasure.Start;
                double right = left + colMeasure.Size;
                double bottom = top + rowMeasure.Size;

                if (childLocation.ColEnd!= null)
                {
                    MeasureInfo colEndMesure = GetMeasureInfo(childLocation.ColEnd, _columns);
                    right = colEndMesure.Start;
                }
                if (childLocation.RowEnd != null)
                {
                    MeasureInfo colEndMesure = GetMeasureInfo(childLocation.RowEnd, _rows);
                    right = colEndMesure.Start;
                }

                Rect arrangeRect = new Rect(left, top, (right - left), (bottom - top));
                child.Arrange(arrangeRect);
            }
            return finalSize;
        }
    }
}
