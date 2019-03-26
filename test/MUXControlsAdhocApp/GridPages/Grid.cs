using System;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsAdhocApp.GridPages
{
    public class TrackInfo
    {
        // TODO: a line can have more than one name. For example, here the second line will have two names: row1-end and row2-start:
        // grid-template-rows: [row1-start] 25% [row1-end row2-start] 25% [row2-end];
        public string LineName;

        // TODO: track-size can be a length, a percentage, or a fraction of the free space in the grid (fr)
        // grid-template-columns: 40px 50px auto 50px 40px;
        // grid-template-rows: 25% 100px auto;
        // The fr unit allows you to set the size of a track as a fraction of the free space of the grid container. For example, this will set each item to one third the width of the grid container:
        // grid-template-columns: 1fr 1fr 1fr;
        public double Length { get; set; } = 0;
    }

    public class Grid : Panel
    {
        public List<TrackInfo> TemplateColumns
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
        private List<TrackInfo> _templateColumns = new List<TrackInfo>();

        public List<TrackInfo> TemplateRows
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
        private List<TrackInfo> _templateRows = new List<TrackInfo>();

        private TrackInfo GetTrack(List<TrackInfo> list, int index)
        {
            if (index >= list.Count)
            {
                // TODO: Handle
                return new TrackInfo();
            }

            return list[index];
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            Size usedSize = new Size(0, 0);

            foreach (UIElement child in Children)
            {
                TrackInfo columnStart = GetTrack(_templateColumns, 0);
                TrackInfo rowStart = GetTrack(_templateRows, 0);

                // TODO: Relate to availableSize
                Size measureSize = new Size(columnStart.Length, rowStart.Length);

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
