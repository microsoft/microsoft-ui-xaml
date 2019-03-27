using System;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

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

        protected struct MeasureInfo
        {
            public double Size;
        }
        private Dictionary<GridTrackInfo, MeasureInfo> _columns = new Dictionary<GridTrackInfo, MeasureInfo>();
        private Dictionary<GridTrackInfo, MeasureInfo> _rows = new Dictionary<GridTrackInfo, MeasureInfo>();

        private static void PopulateFromTemplate(List<GridTrackInfo> template, Dictionary<GridTrackInfo, MeasureInfo> calculated, double available)
        {
            double totalFixed = 0.0;
            double totalFraction = 0.0;
            for (int i = 0; i < template.Count; i++)
            {
                double fixedSize = template[i].Length;
                if (fixedSize == 0.0)
                {
                    fixedSize = (template[i].Percentage * available);
                }
                totalFixed += fixedSize;
                totalFraction += template[i].Fraction;

                calculated[template[i]] = new MeasureInfo { Size = fixedSize };
            }

            // First subtract the fixed size
            double remaining = available - totalFixed;

            // What is the size of each fraction?
            double fractionSlice = 0.0;
            if (totalFraction > 0.0)
            {
                fractionSlice = remaining / totalFraction;
            }

            // Assign any fraction elements
            for (int i = 0; i < template.Count; i++)
            {
                if (calculated[template[i]].Size == 0.0)
                {
                    double fraction = template[i].Fraction;
                    double myFractionSlice = fraction * fractionSlice;
                    calculated[template[i]] = new MeasureInfo { Size = myFractionSlice };
                }
            }
        }

        private GridTrackInfo GetTrack(List<GridTrackInfo> list, GridLocation location)
        {
            int index = 0;
            if (location != null)
            {
                index = location.Index;
            }

            if (index >= list.Count)
            {
                // TODO: Handle
                return new GridTrackInfo();
            }

            return list[index];
        }

        private MeasureInfo GetMeasureInfo(GridTrackInfo info, Dictionary<GridTrackInfo, MeasureInfo> calculated)
        {
            return calculated[info];
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            _columns.Clear();
            _rows.Clear();

            PopulateFromTemplate(_templateColumns, _columns, availableSize.Width);
            PopulateFromTemplate(_templateRows, _rows, availableSize.Height);

            Size usedSize = new Size(0, 0);

            foreach (UIElement child in Children)
            {
                GridLocation colStart = GetColumnStart(child);
                GridLocation rowStart = GetRowStart(child);
                GridLocation colEnd = GetColumnEnd(child);
                GridLocation rowEnd = GetRowEnd(child);

                GridTrackInfo colStartTrack = GetTrack(_templateColumns, colStart);
                GridTrackInfo rowStartTrack = GetTrack(_templateRows, rowStart);
                GridTrackInfo colEndTrack = GetTrack(_templateColumns, colStart);
                GridTrackInfo rowEndTrack = GetTrack(_templateRows, rowStart);

                MeasureInfo colMeasure = GetMeasureInfo(colStartTrack, _columns);
                MeasureInfo rowMeasure = GetMeasureInfo(rowStartTrack, _rows);

                // TODO: Relate to availableSize
                Size measureSize = new Size(colMeasure.Size, rowMeasure.Size);

                // Give each child the maximum available space
                child.Measure(measureSize);
                Size childDesiredSize = child.DesiredSize;

                usedSize.Width = Math.Max(usedSize.Width, childDesiredSize.Width);
                usedSize.Height = Math.Max(usedSize.Height, childDesiredSize.Height);
            }

            return usedSize;
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            foreach (UIElement child in Children)
            {
                Size childDesiredSize = child.DesiredSize;
                child.Arrange(new Rect(new Point(0, 0), childDesiredSize));
            }
            return finalSize;
        }
    }
}
