// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

namespace MUXControlsTestApp.Utilities
{
    /// <summary>
    /// Test panel that lays out Rectangles in columns and rows.
    /// </summary>
    public class TilePanel : Panel
    {
        public TilePanel()
        {
        }

        public Color TileColor
        {
            get { return (Color)GetValue(TileColorProperty); }
            set { SetValue(TileColorProperty, value); }
        }

        public static readonly DependencyProperty TileColorProperty =
            DependencyProperty.Register(
                "TileColor",
                typeof(Color),
                typeof(TilePanel),
                new PropertyMetadata(Colors.DarkGray, new PropertyChangedCallback(OnPropertyChanged)));

        public int TileCount
        {
            get { return (int)GetValue(TileCountProperty); }
            set { SetValue(TileCountProperty, value); }
        }

        public static readonly DependencyProperty TileCountProperty =
            DependencyProperty.Register(
                "TileCount",
                typeof(int),
                typeof(TilePanel),
                new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));

        public double TileHeight
        {
            get { return (double)GetValue(TileHeightProperty); }
            set { SetValue(TileHeightProperty, value); }
        }

        public static readonly DependencyProperty TileHeightProperty =
            DependencyProperty.Register(
                "TileHeight",
                typeof(double),
                typeof(TilePanel),
                new PropertyMetadata(80.0, new PropertyChangedCallback(OnPropertyChanged)));

        public double TileWidth
        {
            get { return (double)GetValue(TileWidthProperty); }
            set { SetValue(TileWidthProperty, value); }
        }

        public static readonly DependencyProperty TileWidthProperty =
            DependencyProperty.Register(
                "TileWidth",
                typeof(double),
                typeof(TilePanel),
                new PropertyMetadata(110.0, new PropertyChangedCallback(OnPropertyChanged)));

        protected override Size MeasureOverride(Size availableSize)
        {
            Size result = new Size();

            if (TileCount > 0)
            {
                int columnCount, rowCount;

                GetColumnAndRowCounts(availableSize, out columnCount, out rowCount);

                result.Width = columnCount * TileWidth;
                result.Height = rowCount * TileHeight;
            }

            return result;
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            if (TileCount > 0)
            {
                int columnCount, rowCount;

                GetColumnAndRowCounts(finalSize, out columnCount, out rowCount);

                double horizontalOffset = 0.0;
                double verticalOffset = 0.0;

                if (columnCount * TileWidth < finalSize.Width)
                {
                    switch (HorizontalAlignment)
                    {
                        case HorizontalAlignment.Center:
                        case HorizontalAlignment.Stretch:
                            horizontalOffset = (finalSize.Width - columnCount * TileWidth) / 2.0;
                            break;
                        case HorizontalAlignment.Right:
                            horizontalOffset = finalSize.Width - columnCount * TileWidth;
                            break;
                    }
                }

                if (rowCount * TileHeight < finalSize.Height)
                {
                    switch (VerticalAlignment)
                    {
                        case VerticalAlignment.Center:
                        case VerticalAlignment.Stretch:
                            verticalOffset = (finalSize.Height - rowCount * TileHeight) / 2.0;
                            break;
                        case VerticalAlignment.Bottom:
                            verticalOffset = finalSize.Height - rowCount * TileHeight;
                            break;
                    }
                }

                for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
                {
                    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
                    {
                        if (columnIndex * rowCount + rowIndex < TileCount)
                        {
                            Rect rectangleRect = new Rect(
                                horizontalOffset + columnIndex * TileWidth,
                                verticalOffset + rowIndex * TileHeight,
                                TileWidth,
                                TileHeight);
                            Children[columnIndex * rowCount + rowIndex].Arrange(rectangleRect);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            return base.ArrangeOverride(finalSize);
        }

        private static void OnPropertyChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            TilePanel tilePanel = obj as TilePanel;

            if (args.Property == TilePanel.TileCountProperty)
            {
                int oldTileCount = (int)args.OldValue;
                int newTileCount = (int)args.NewValue;

                if (newTileCount < 0)
                {
                    throw new ArgumentException("TileCount must be positive.");
                }

                if (newTileCount > oldTileCount)
                {
                    for (int tileIndex = oldTileCount; tileIndex < newTileCount; tileIndex++)
                    {
                        Rectangle rectangle = new Rectangle();
                        rectangle.Width = tilePanel.TileWidth;
                        rectangle.Height = tilePanel.TileHeight;
                        tilePanel.SetRectangleColor(rectangle);
                        tilePanel.Children.Add(rectangle);
                    }
                }
                else if (newTileCount < oldTileCount)
                {
                    for (int tileIndex = oldTileCount-1; tileIndex >= newTileCount; tileIndex--)
                    {
                        tilePanel.Children.RemoveAt(tileIndex);
                    }
                }
            }
            else if (args.Property == TilePanel.TileColorProperty)
            {
                foreach (Rectangle rectangle in tilePanel.Children)
                {
                    tilePanel.SetRectangleColor(rectangle);
                }
            }
            else
            {
                tilePanel.InvalidateMeasure();
            }
        }

        private void SetRectangleColor(Rectangle rectangle)
        {
            LinearGradientBrush lgb = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };
            GradientStop gs = new GradientStop() { Color = Colors.LightGray, Offset = 0.0 };
            lgb.GradientStops.Add(gs);
            gs = new GradientStop() { Color = TileColor, Offset = 1.0 };
            lgb.GradientStops.Add(gs);

            rectangle.Fill = lgb;
        }

        private int GetColumnCount(double totalWidth)
        {
            double columnCountD = totalWidth / TileWidth;
            int columnCountI = (int)columnCountD;
            if (columnCountD - columnCountI > 0.5)
            {
                columnCountI++;
            }
            return columnCountI;
        }

        private int GetRowCount(double totalHeight)
        {
            double rowCountD = totalHeight / TileHeight;
            int rowCountI = (int)rowCountD;
            if (rowCountD - rowCountI > 0.5)
            {
                rowCountI++;
            }
            return rowCountI;
        }

        private void GetColumnAndRowCounts(Size size, out int columnCount, out int rowCount)
        {
            columnCount = 0;
            rowCount = 0;

            if (TileCount > 0)
            {
                if (double.IsPositiveInfinity(size.Width) &&
                    double.IsPositiveInfinity(size.Height))
                {
                    columnCount = GetColumnCount(Math.Sqrt(TileCount) * TileWidth);
                    rowCount = (int)Math.Ceiling((double)TileCount / columnCount);
                }
                else if (double.IsPositiveInfinity(size.Height))
                {
                    columnCount = GetColumnCount(size.Width);
                    rowCount = (int)Math.Ceiling((double)TileCount / columnCount);
                }
                else
                {
                    rowCount = GetRowCount(size.Height);
                    columnCount = (int)Math.Ceiling((double)TileCount / rowCount);
                }
            }
        }
    }
}
