using System;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

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

    public enum GridJustifyItems
    {
        Start,
        End,
        Center,
        Stretch,
    }

    public enum GridAlignItems
    {
        Start,
        End,
        Center,
        Stretch,
    }

    public enum GridJustifyContent
    {
        Start,
        End,
        Center,
        Stretch,
        SpaceAround,
        SpaceBetween,
        SpaceEvenly,
    }

    public enum GridAlignContent
    {
        Start,
        End,
        Center,
        Stretch,
        SpaceAround,
        SpaceBetween,
        SpaceEvenly,
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

        public double ColumnGap
        {
            get
            {
                return _columnGap;
            }
            set
            {
                if (_columnGap != value)
                {
                    _columnGap = value;
                    InvalidateMeasure();
                }
            }
        }
        private double _columnGap = 0.0;

        public double RowGap
        {
            get
            {
                return _rowGap;
            }
            set
            {
                if (_rowGap != value)
                {
                    _rowGap = value;
                    InvalidateMeasure();
                }
            }
        }
        private double _rowGap = 0.0;

        public GridJustifyItems JustifyItems
        {
            get
            {
                return _justifyItems;
            }
            set
            {
                if (_justifyItems != value)
                {
                    _justifyItems = value;
                    InvalidateMeasure();
                }
            }
        }
        private GridJustifyItems _justifyItems = GridJustifyItems.Stretch;

        public GridAlignItems AlignItems
        {
            get
            {
                return _alignItems;
            }
            set
            {
                if (_alignItems != value)
                {
                    _alignItems = value;
                    InvalidateMeasure();
                }
            }
        }
        private GridAlignItems _alignItems = GridAlignItems.Stretch;

        public GridJustifyContent JustifyContent
        {
            get
            {
                return _justifyContent;
            }
            set
            {
                if (_justifyContent != value)
                {
                    _justifyContent = value;
                    InvalidateMeasure();
                }
            }
        }
        private GridJustifyContent _justifyContent = GridJustifyContent.Start;

        public GridAlignContent AlignContent
        {
            get
            {
                return _alignContent;
            }
            set
            {
                if (_alignContent != value)
                {
                    _alignContent = value;
                    InvalidateMeasure();
                }
            }
        }
        private GridAlignContent _alignContent = GridAlignContent.Start;

        private static void InvalidateMeasureOnChildPropertyChanged(DependencyObject source, DependencyPropertyChangedEventArgs args)
        {
            Grid parent = VisualTreeHelper.GetParent(source) as Grid;
            if (parent != null)
            {
                parent.InvalidateMeasure();
            }
        }

        private static GridTrackInfo _lastInTrack = new GridTrackInfo();

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
                // Add all the items from the markup defined template, plus one more grid line for the end
                Template = new List<GridTrackInfo>(template);
                Template.Add(_lastInTrack);

                Calculated = calculated;
                Available = 0.0;
                Remaining = 0.0;
                TotalFixed = 0.0;
                TotalFraction = 0.0;
                TotalAutos = 0;
                Gap = 0.0;
            }

            public List<GridTrackInfo> Template { get; private set; }
            public Dictionary<GridTrackInfo, MeasureInfo> Calculated { get; private set; }

            public double Available;
            public double Remaining;
            public double TotalFixed;
            public double TotalFraction;
            public uint TotalAutos;
            public double Gap;
        }

        private static MeasureBlah InitializeMeasure(List<GridTrackInfo> template, Dictionary<GridTrackInfo, MeasureInfo> calculated, double gap, double available)
        {
            int numberOfGaps = (template.Count - 1);
            if ((gap > 0.0) && (numberOfGaps > 0))
            {
                available -= (gap * numberOfGaps);
            }

            MeasureBlah measure = new MeasureBlah(template, calculated);
            measure.Available = available;
            measure.Gap = gap;

            return measure;
        }

        private static void ProcessFixedSizes(ref MeasureBlah measure)
        {
            for (int i = 0; i < measure.Template.Count; i++)
            {
                GridTrackInfo track = measure.Template[i];

                double fixedSize = track.Length;

                // Percentage is effectively a fixed size in that it needs to be applied before any
                // of the more relative sizes (fraction, auto, etc.)
                if (fixedSize == 0.0)
                {
                    fixedSize = (track.Percentage * measure.Available);
                }

                measure.TotalFixed += fixedSize;

                // Accumulate the fractional sizes now so we know how many pieces of pie to dish out
                measure.TotalFraction += track.Fraction;

                if (track.Auto)
                {
                    measure.TotalAutos++;
                }

                measure.Calculated[track] = new MeasureInfo { Size = fixedSize };
            }

            measure.Remaining = Math.Max(measure.Available - measure.TotalFixed, 0.0);
        }

        private void ProcessAutoSizes(ref MeasureBlah measureHorizontal, ref MeasureBlah measureVertical)
        {
            foreach (UIElement child in Children)
            {
                ChildGridLocations childLocation = GetChildGridLocations(child, ref measureHorizontal, ref measureVertical);

                List<GridTrackInfo> autoHorizontal = new List<GridTrackInfo>();
                List<GridTrackInfo> autoVertical = new List<GridTrackInfo>();

                Func<GridTrackInfo, GridTrackInfo, MeasureBlah, List<GridTrackInfo>, bool> getAutoTracks = (GridTrackInfo start, GridTrackInfo end, MeasureBlah measure, List<GridTrackInfo> autoTracks) =>
                {
                    if ((start == null) || (end == null))
                    {
                        return false;
                    }

                    int startIndex = measure.Template.IndexOf(start);
                    int endIndex = measure.Template.IndexOf(end);

                    for (int i = startIndex; i <= endIndex; i++)
                    {
                        GridTrackInfo track = measure.Template[i];
                        if (track.Auto)
                        {
                            autoTracks.Add(track);
                        }
                    }
                    return (autoTracks.Count > 0);
                };

                bool affectsHorizontalAuto = getAutoTracks(childLocation.ColStart, childLocation.ColEnd, measureHorizontal, autoHorizontal);
                bool affectsVerticalAuto = getAutoTracks(childLocation.RowStart, childLocation.RowEnd, measureVertical, autoVertical);

                // If none of the grid tracks are Auto then we can skip this item
                if (!affectsHorizontalAuto && !affectsVerticalAuto)
                {
                    continue;
                }

                // Measure this child to see how much of the available space they would take.
                // Note that even if there are multiple Autos we don't attempt to preemptively split 
                // the space between them. They are all given a crack at being greedy.
                Size measureSize = new Size(measureHorizontal.Remaining, measureVertical.Remaining);
                child.Measure(measureSize);
                DumpInfo($"Child inside Auto range measured to {child.DesiredSize}");

                // Update that row/column with the dimensions
                UpdateAutoBasedOnMeasured(autoHorizontal, ref measureHorizontal, child.DesiredSize.Width);
                UpdateAutoBasedOnMeasured(autoVertical, ref measureVertical, child.DesiredSize.Height);
            }
        }

        // NOTE: Can't do as an anonymous inline Action above because we need to declare the struct MeasureBlah as ref (and Actions don't support ref parameters)
        private static void UpdateAutoBasedOnMeasured(List<GridTrackInfo> tracks, ref MeasureBlah measure, double childDesired)
        {
            if (tracks.Count == 0)
            {
                return;
            }

            double autoSlice = (childDesired / tracks.Count);

            for (int i = 0; i < tracks.Count; i++)
            {
                GridTrackInfo track = tracks[i];
                MeasureInfo info = measure.Calculated[track];
                double moreSize = (autoSlice - info.Size);
                if (moreSize > 0)
                {
                    DumpInfo($"Increasing Auto size track {i} by {moreSize}");
                    info.Size += moreSize;
                    measure.Remaining = Math.Max(measure.Remaining - moreSize, 0.0);
                }
            }
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

        // Allow any Auto tracks to take the remaining space
        private static void ProcessAutoRemainingSize(ref MeasureBlah measure)
        {
            if (measure.TotalAutos <= 0)
            {
                return;
            }

            // If there were fractional elements they would have already taken up all the space
            if (measure.Remaining <= 0.0)
            {
                return;
            }

            // How much extra space do we give up to each Auto element?
            double autoSlice = measure.Remaining / (double)measure.TotalAutos;

            for (int i = 0; i < measure.Template.Count; i++)
            {
                GridTrackInfo track = measure.Template[i];
                if (!track.Auto)
                {
                    continue;
                }

                MeasureInfo info = measure.Calculated[track];
                info.Size += autoSlice;
            }

            // We've consumed all that's left
            measure.Remaining = 0.0;
        }


        private double ProcessOffsets(ref MeasureBlah measure)
        {
            double offset = 0.0;
            foreach (var entry in measure.Calculated)
            {
                entry.Value.Start = offset;
                offset += entry.Value.Size;
                offset += measure.Gap;
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

        private ChildGridLocations GetChildGridLocations(UIElement child, ref MeasureBlah measureHorizontal, ref MeasureBlah measureVertical)
        {
            return GetChildGridLocations(child, measureHorizontal.Template, measureVertical.Template);
        }

        private ChildGridLocations GetChildGridLocations(UIElement child, List<GridTrackInfo> horizontal, List<GridTrackInfo> vertical)
        {
            ChildGridLocations result;

            // Read preferences off the child
            GridLocation colStart = GetColumnStart(child);
            GridLocation rowStart = GetRowStart(child);
            GridLocation colEnd = GetColumnEnd(child);
            GridLocation rowEnd = GetRowEnd(child);

            // Map those to our grid lines
            result.ColStart = GetTrack(horizontal, colStart);
            result.RowStart = GetTrack(vertical, rowStart);
            result.ColEnd = GetTrack(horizontal, colEnd, result.ColStart);
            result.RowEnd = GetTrack(vertical, rowEnd, result.RowStart);

            return result;
        }

        private GridTrackInfo GetTrack(List<GridTrackInfo> list, GridLocation location, GridTrackInfo previous = null)
        {
            if (location != null)
            {
                // Exact track index
                int index = location.Index;
                if (index >= 0)
                {
                    index = Math.Min(index, list.Count - 1);
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
            }

            // Span relative to previous track
            if (previous != null)
            {
                // By default go 1 beyond the previous one
                int span = 1;
                if ((location != null) && (location.Span > 0))
                {
                    span = location.Span;
                }

                int previousIndex = list.IndexOf(previous);
                if (previousIndex >= 0)
                {
                    int spanIndex = Math.Min(previousIndex + span, list.Count - 1);

                    return list[spanIndex];
                }
            }

            return null;
        }

        private MeasureInfo GetMeasureInfo(GridTrackInfo info, Dictionary<GridTrackInfo, MeasureInfo> calculated)
        {
            if (info != null)
            {
                MeasureInfo result;
                if (!calculated.TryGetValue(info, out result))
                {
                    return new MeasureInfo();
                }
                return result;
            }
            else
            {
                // TODO: This is where auto rows/columns would kick in
                return new MeasureInfo();
            }

        }

#region Tracing
        [Conditional("GRID_TRACE")]
        private static void DumpConditional(bool condition, string write, ref string separator)
        {
            if (condition)
            {
                Debug.Write(separator + write);
                separator = ", ";
            }
        }

        [Conditional("GRID_TRACE")]
        private void DumpTemplates()
        {
            Action<List<GridTrackInfo>> dumpTemplate = (List<GridTrackInfo> template) =>
            {
                for (int i = 0; i < template.Count; i++)
                {
                    GridTrackInfo track = template[i];
                    Debug.Write($"{i} {{");
                    string separator = String.Empty;
                    DumpConditional(!String.IsNullOrEmpty(track.LineName), $"LineName='{track.LineName}'", ref separator);
                    DumpConditional(track.Length != 0.0, $"Length={track.Length}", ref separator);
                    DumpConditional(track.Percentage != 0.0, $"Percentage={track.Percentage}", ref separator);
                    DumpConditional(track.Fraction != 0.0, $"Fraction={track.Fraction}", ref separator);
                    DumpConditional(track.Auto, $"Auto={track.Auto}", ref separator);
                    Debug.WriteLine($"}}");
                }
            };

            DumpBegin("TemplateColumns");
            dumpTemplate(_templateColumns);
            DumpEnd();

            DumpBegin("TemplateRows");
            dumpTemplate(_templateRows);
            DumpEnd();
        }

        [Conditional("GRID_TRACE")]
        private void DumpChildren(ref MeasureBlah measureHorizontal, ref MeasureBlah measureVertical)
        {
            DumpBegin("Children");
            foreach (UIElement child in Children)
            {
                Debug.WriteLine(child.GetType().Name + " {");
                Debug.Indent();
                ChildGridLocations locations = GetChildGridLocations(child, ref measureHorizontal, ref measureVertical);

                Action<GridLocation, string> dumpLocation = (GridLocation location, string info) =>
                {
                    Debug.Write(info);
                    if (location == null)
                    {
                        return;
                    }

                    string separator = String.Empty;
                    DumpConditional(location.Index >= 0, $"Index={location.Index}", ref separator);
                    DumpConditional(!String.IsNullOrEmpty(location.LineName), $"LineName='{location.LineName}'", ref separator);
                    DumpConditional(location.Span > 0, $"Span={location.Span}", ref separator);
                };

                dumpLocation(GetColumnStart(child), "Column {");
                dumpLocation(GetColumnEnd(child), "} to {");
                Debug.WriteLine("}");
                dumpLocation(GetRowStart(child), "Row {");
                dumpLocation(GetRowEnd(child), "} to {");
                Debug.WriteLine("}");
                Debug.Unindent();
                Debug.WriteLine("}");
            }
            DumpEnd();
        }

        [Conditional("GRID_TRACE")]
        private static void DumpMeasureInfo(ref MeasureBlah measure, string info, bool includeOffset = false)
        {
            DumpBegin(info);
            foreach (var entry in measure.Calculated)
            {
                int trackIndex = measure.Template.IndexOf(entry.Key);
                if (includeOffset)
                {
                    Debug.WriteLine($"{trackIndex} {{Size={entry.Value.Size}, Start={entry.Value.Start}}}");
                }
                else
                {
                    Debug.WriteLine($"{trackIndex} {{Size={entry.Value.Size}}}");
                }
                
            }
            DumpEnd();
        }

        [Conditional("GRID_TRACE")]
        private static void DumpMeasureInfo(ref MeasureBlah measureHorizontal, ref MeasureBlah measureVertical, string info, bool includeOffset = false)
        {
            DumpBegin(info);
            DumpMeasureInfo(ref measureHorizontal, "Columns", includeOffset);
            Debug.WriteLine($"Remaining={measureHorizontal.Remaining}");
            DumpMeasureInfo(ref measureVertical, "Rows", includeOffset);
            Debug.WriteLine($"Remaining={measureVertical.Remaining}");
            DumpEnd();
        }

        [Conditional("GRID_TRACE")]
        private static void DumpBegin(Size size, string info)
        {
            Debug.WriteLine($"{info}({size.Width}, {size.Height}) {{");
            Debug.Indent();
        }

        [Conditional("GRID_TRACE")]
        private static void DumpBegin(string info)
        {
            Debug.WriteLine($"{info} {{");
            Debug.Indent();
        }

        [Conditional("GRID_TRACE")]
        private static void DumpEnd()
        {
            Debug.Unindent();
            Debug.WriteLine("}");
        }

        [Conditional("GRID_TRACE")]
        private static void DumpInfo(string info)
        {
            Debug.WriteLine(info);
        }
#endregion

        protected override Size MeasureOverride(Size availableSize)
        {
            DumpBegin(availableSize, "Measure");
            DumpTemplates();
            DumpInfo($"ColumnGap={_columnGap}, RowGap={_rowGap}");

            _columns.Clear();
            _rows.Clear();

            MeasureBlah measureHorizontal = InitializeMeasure(_templateColumns, _columns, _columnGap, availableSize.Width);
            MeasureBlah measureVertical = InitializeMeasure(_templateRows, _rows, _rowGap, availableSize.Height);
            DumpChildren(ref measureHorizontal, ref measureVertical);

            // First process any fixed sizes
            ProcessFixedSizes(ref measureHorizontal);
            ProcessFixedSizes(ref measureVertical);
            DumpMeasureInfo(ref measureHorizontal, ref measureVertical, "Fixed");

            // Next we need to know how large the auto sizes are
            ProcessAutoSizes(ref measureHorizontal, ref measureVertical);
            DumpMeasureInfo(ref measureHorizontal, ref measureVertical, "Auto");

            // Then we can figure out how large the fractional sizes should be
            ProcessFractionalSizes(ref measureHorizontal);
            ProcessFractionalSizes(ref measureVertical);
            DumpMeasureInfo(ref measureHorizontal, ref measureVertical, "Fractional");

            // And then the auto elements can claim any remaining sizes
            ProcessAutoRemainingSize(ref measureHorizontal);
            ProcessAutoRemainingSize(ref measureVertical);
            DumpMeasureInfo(ref measureHorizontal, ref measureVertical, "Auto remainder");

            Size usedSize = new Size(0, 0);

            foreach (UIElement child in Children)
            {
                ChildGridLocations childLocation = GetChildGridLocations(child, ref measureHorizontal, ref measureVertical);

                MeasureInfo colMeasure = GetMeasureInfo(childLocation.ColStart, _columns);
                MeasureInfo rowMeasure = GetMeasureInfo(childLocation.RowStart, _rows);

                // TODO: Relate to availableSize
                Size measureSize = new Size(colMeasure.Size, rowMeasure.Size);

                // Give each child the maximum available space
                child.Measure(measureSize);
            }

            // Now that the sizes are known we can calculate the offsets for the grid tracks
            double width = ProcessOffsets(ref measureHorizontal);
            double height = ProcessOffsets(ref measureVertical);
            DumpMeasureInfo(ref measureHorizontal, ref measureVertical, "Calculate offsets", includeOffset: true);

            // If there's no entry for columns/rows use the minimal size, otherwise use the whole space.
            if (_templateColumns.Count > 0)
            {
                width = availableSize.Width;
            }
            if (_templateRows.Count > 0)
            {
                height = availableSize.Height;
            }

            DumpEnd();
            return new Size(width, height);
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            DumpBegin(finalSize, "Arrange");
            foreach (UIElement child in Children)
            {
                // TODO: Avoid recreating these lists
                var templateColumns = new List<GridTrackInfo>(_templateColumns);
                templateColumns.Add(_lastInTrack);
                var templateRows = new List<GridTrackInfo>(_templateRows);
                templateRows.Add(_lastInTrack);
                ChildGridLocations childLocation = GetChildGridLocations(child, templateColumns, templateRows);

                // TODO: This should be driven by auto rows/columns, but until that's implemented, avoid a crash by hiding the item
                if ((childLocation.ColStart == null) || (childLocation.RowStart == null))
                {
                    child.Arrange(new Rect(0, 0, 0, 0));
                    continue;
                }

                MeasureInfo colMeasure = GetMeasureInfo(childLocation.ColStart, _columns);
                MeasureInfo rowMeasure = GetMeasureInfo(childLocation.RowStart, _rows);
                MeasureInfo colEndMesure = GetMeasureInfo(childLocation.ColEnd, _columns);
                MeasureInfo rowEndMesure = GetMeasureInfo(childLocation.RowEnd, _rows);

                double left = colMeasure.Start;
                double top = rowMeasure.Start;
                double right = colEndMesure.Start;
                double bottom = rowEndMesure.Start;

                // The left edge of a grid line includes the gap amount. As long as we have any
                // coordinates at all we should be trimming that off.
                if (right > 0.0)
                {
                    right -= _columnGap;
                }
                if (bottom > 0.0)
                {
                    bottom -= _rowGap;
                }

                // They might have specified grid references that were inverted. If so, collapse
                // them and essentially zero out the child's arrange size.
                if (right < left)
                {
                    right = left;
                }
                if (bottom < top)
                {
                    bottom = top;
                }

                double width = (right - left);
                double height = (bottom - top);

                double desiredWidth = Math.Min(child.DesiredSize.Width, width);
                double unusedWidth = (width - desiredWidth);
                switch (_justifyItems)
                {
                    case GridJustifyItems.Start:
                        width = desiredWidth;
                        break;
                    case GridJustifyItems.End:
                        left += unusedWidth;
                        width = desiredWidth;
                        break;
                    case GridJustifyItems.Center:
                        left += unusedWidth * 0.5;
                        width = desiredWidth;
                        break;
                    case GridJustifyItems.Stretch:
                        break;
                }

                double desiredHeight = Math.Min(child.DesiredSize.Height, height);
                double unusedHeight = (height - desiredHeight);
                switch (_alignItems)
                {
                    case GridAlignItems.Start:
                        height = desiredHeight;
                        break;
                    case GridAlignItems.End:
                        top += unusedHeight;
                        height = desiredHeight;
                        break;
                    case GridAlignItems.Center:
                        top += unusedHeight * 0.5;
                        height = desiredHeight;
                        break;
                    case GridAlignItems.Stretch:
                        break;
                }

                DumpBegin(child.GetType().Name);
                DumpInfo("leftTrack=" + _templateColumns.IndexOf(childLocation.ColStart));
                DumpInfo("topTrack=" + _templateRows.IndexOf(childLocation.RowStart));
                DumpInfo("rightTrack=" + _templateColumns.IndexOf(childLocation.ColEnd));
                DumpInfo("bottomTrack=" + _templateRows.IndexOf(childLocation.RowEnd));
                DumpInfo($"left={left}, top={top}, right={right}, bottom={bottom}");
                DumpEnd();

                Rect arrangeRect = new Rect(left, top, width, height);
                child.Arrange(arrangeRect);
            }

            DumpEnd();
            return finalSize;
        }
    }
}
