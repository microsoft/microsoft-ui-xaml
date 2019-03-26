using System;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsAdhocApp.FlexboxPages
{
    public enum FlexboxDirection
    {
        Row,
        RowReverse,
        Column,
        ColumnReverse,
    }

    public enum FlexboxJustifyContent
    {
        Start,
        End,
        Center,
        SpaceBetween,
        SpaceAround,
        SpaceEvenly,
    }

    public enum FlexboxAlignItems
    {
        Start,
        End,
        Center,
        Stretch,
        Baseline,
    }

    public enum FlexboxAlignContent
    {
        Start,
        End,
        Center,
        Stretch,
        SpaceBetween,
        SpaceAround,
    }

    public enum FlexboxAlignSelf
    {
        Auto,
        Start,
        End,
        Center,
        Baseline,
        Stretch,
    }

    public class Flexbox : Panel
    {
        // Order (number)
        public static readonly DependencyProperty OrderProperty =
            DependencyProperty.RegisterAttached(
              "Order",
              typeof(int),
              typeof(Flexbox),
              new PropertyMetadata(0)
            );
        public static void SetOrder(UIElement element, int value)
        {
            element.SetValue(OrderProperty, value);
        }
        public static int GetOrder(UIElement element)
        {
            return (int)element.GetValue(OrderProperty);
        }

        // Grow (number, >= 0)
        public static readonly DependencyProperty GrowProperty =
            DependencyProperty.RegisterAttached(
              "Grow",
              typeof(double),
              typeof(Flexbox),
              new PropertyMetadata(0.0)
            );
        public static void SetGrow(UIElement element, double value)
        {
            element.SetValue(GrowProperty, value);
        }
        public static double GetGrow(UIElement element)
        {
            return (double)element.GetValue(GrowProperty);
        }

        // Shrink (number, >= 0)
        public static readonly DependencyProperty ShrinkProperty =
            DependencyProperty.RegisterAttached(
              "Shrink",
              typeof(double),
              typeof(Flexbox),
              new PropertyMetadata(0.0)
            );
        public static void SetShrink(UIElement element, double value)
        {
            element.SetValue(ShrinkProperty, value);
        }
        public static double GetShrink(UIElement element)
        {
            return (double)element.GetValue(ShrinkProperty);
        }

        // TODO: Child property: Basis (length | auto) - Determines meaning of Grow/Shrink. Does it use the "extra" space or the whole space?

        // AlignSelf (cross-axis alignment)
        public static readonly DependencyProperty AlignSelfProperty =
            DependencyProperty.RegisterAttached(
              "AlignSelf",
              typeof(FlexboxAlignSelf),
              typeof(Flexbox),
              new PropertyMetadata(FlexboxAlignSelf.Auto)
            );
        public static void SetAlignSelf(UIElement element, FlexboxAlignSelf value)
        {
            element.SetValue(AlignSelfProperty, value);
        }
        public static FlexboxAlignSelf GetAlignSelf(UIElement element)
        {
            return (FlexboxAlignSelf)element.GetValue(AlignSelfProperty);
        }

        private double MainAxis(Size value)
        {
            return (IsHorizontal ? value.Width : value.Height);
        }
        private double CrossAxis(Size value)
        {
            return (IsHorizontal ? value.Height : value.Width);
        }
        private Size CreateSize(double mainAxis, double crossAxis)
        {
            return IsHorizontal ?
                new Size(mainAxis, crossAxis) :
                new Size(crossAxis, mainAxis);
        }
        private Point CreatePoint(double mainAxis, double crossAxis)
        {
            return IsHorizontal ?
                new Point(mainAxis, crossAxis) :
                new Point(crossAxis, mainAxis);
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            _rows.Clear();

            uint itemsInRow = 0;

            double usedInCurrentMainAxis = 0;
            double usedInCurrentCrossAxis = 0;

            double usedMainAxis = 0;
            double usedCrossAxis = 0;

            Action completeRow = () =>
            {
                _rows.Add(new RowMeasureInfo {
                    MainAxis = usedInCurrentMainAxis,
                    CrossAxis = usedInCurrentCrossAxis,
                    Count = itemsInRow,
                });
                itemsInRow = 0;
                usedMainAxis = Math.Max(usedMainAxis, usedInCurrentMainAxis);
                usedInCurrentMainAxis = 0;
                usedCrossAxis += usedInCurrentCrossAxis;
                usedInCurrentCrossAxis = 0;
            };

            List<UIElement> sortedChildren = ChildrenSortedByOrder();
            foreach (UIElement child in sortedChildren)
            {
                // Give each child the maximum available space
                // TODO: What about flex-shrink? Should we try them with less?
                // TODO: This is where flex-basis would come into play
                child.Measure(availableSize);
                Size childDesiredSize = child.DesiredSize;

                if (usedInCurrentMainAxis + MainAxis(childDesiredSize) > MainAxis(availableSize))
                {
                    // Not enough space, time for a new row
                    if (Wrap)
                    {
                        completeRow();

                        // It's possible that even making a new row won't work. Sorry, you're not going to fit!
                        if (usedCrossAxis + CrossAxis(childDesiredSize) > CrossAxis(availableSize))
                        {
                            // TODO: What about flex-shrink?
                            break;
                        }
                    }
                    else
                    {
                        // Without the ability to wrap, we just flat out can't fit this item
                        // TODO: What about flex-shrink?
                        break;
                    }
                }

                // Contribute our space
                usedInCurrentMainAxis += MainAxis(childDesiredSize);
                usedInCurrentCrossAxis = Math.Max(usedInCurrentCrossAxis, CrossAxis(childDesiredSize));
                itemsInRow++;
            }

            // Incorporate any contribution from the pending row into our total calculation
            if (usedInCurrentMainAxis > 0)
            {
                completeRow();
            }

            Size returnSize = CreateSize(
                mainAxis: usedMainAxis,
                crossAxis: usedCrossAxis);

            return returnSize;
        }

        // Calculated information from Measure that we'll need during Arrange
        protected struct RowMeasureInfo
        {
            public double MainAxis;
            public double CrossAxis;
            public uint Count;
        }
        private List<RowMeasureInfo> _rows = new List<RowMeasureInfo>();

        protected override Size ArrangeOverride(Size finalSize)
        {
            int rowIndex = 0;
            double usedInCurrentMainAxis = 0;
            double crossOffsetForCurrentRow = 0;
            double usedInCurrentCrossAxis = 0;

            List<UIElement> sortedChildren = ChildrenSortedByOrder();
            foreach (UIElement child in sortedChildren)
            {
                RowMeasureInfo info = _rows[rowIndex];

                Size childDesiredSize = child.DesiredSize;
                if (usedInCurrentMainAxis + MainAxis(childDesiredSize) > MainAxis(finalSize))
                {
                    if (!Wrap)
                    {
                        usedInCurrentMainAxis = MainAxis(finalSize);
                        child.Arrange(new Rect(new Point(0, 0), new Size(0, 0)));
                        continue;
                    }

                    usedInCurrentMainAxis = 0;
                    crossOffsetForCurrentRow += (usedInCurrentCrossAxis > 0 ? usedInCurrentCrossAxis : info.CrossAxis);
                    rowIndex++;
                    info = _rows[rowIndex];
                }

                double mainOffset = usedInCurrentMainAxis;
                double excessMainAxis = (MainAxis(finalSize) - info.MainAxis);

                switch (JustifyContent)
                {
                    case FlexboxJustifyContent.Start:
                        break;
                    case FlexboxJustifyContent.Center:
                        mainOffset += (excessMainAxis * 0.5);
                        break;
                    case FlexboxJustifyContent.End:
                        mainOffset += excessMainAxis;
                        break;
                    case FlexboxJustifyContent.SpaceBetween:
                        if (info.Count > 1)
                        {
                            usedInCurrentMainAxis += (excessMainAxis / (info.Count - 1));
                        }
                        break;
                    case FlexboxJustifyContent.SpaceAround:
                        {
                            double spaceSlice = (excessMainAxis / info.Count);
                            if (usedInCurrentMainAxis == 0)
                            {
                                usedInCurrentMainAxis = mainOffset = (spaceSlice * 0.5);
                                mainOffset = usedInCurrentMainAxis;
                            }
                            usedInCurrentMainAxis += spaceSlice;
                            break;
                        }
                    case FlexboxJustifyContent.SpaceEvenly:
                        {
                            double spaceSlice = (excessMainAxis / (info.Count + 1));
                            mainOffset += spaceSlice;
                            usedInCurrentMainAxis += spaceSlice;
                        }
                        break;
                }

                double crossOffset = crossOffsetForCurrentRow;
                double excessCrossAxisInRow = (info.CrossAxis - CrossAxis(childDesiredSize));

                double totalCrossAxis = 0.0;
                _rows.ForEach(row => { totalCrossAxis += row.CrossAxis; });
                double excessCrossAxis = (CrossAxis(finalSize) - totalCrossAxis);

                switch (AlignContent)
                {
                    case FlexboxAlignContent.Start:
                        break;
                    case FlexboxAlignContent.Center:
                        crossOffset += (excessCrossAxis * 0.5);
                        break;
                    case FlexboxAlignContent.End:
                        crossOffset += excessCrossAxis;
                        break;
                    case FlexboxAlignContent.Stretch:
                        double extra = (excessCrossAxis / _rows.Count);
                        usedInCurrentCrossAxis = (info.CrossAxis + extra);
                        excessCrossAxisInRow = 0;
                        childDesiredSize = CreateSize(MainAxis(childDesiredSize), usedInCurrentCrossAxis);
                        break;
                    case FlexboxAlignContent.SpaceBetween:
                        if (_rows.Count > 1)
                        {
                            double spaceBetween = (excessCrossAxis / (_rows.Count - 1));
                            usedInCurrentCrossAxis = (info.CrossAxis + spaceBetween);
                        }
                        break;
                    case FlexboxAlignContent.SpaceAround:
                        {
                            double spaceAround = (excessCrossAxis / _rows.Count);
                            crossOffset += (spaceAround * 0.5);
                            usedInCurrentCrossAxis = (info.CrossAxis + spaceAround);
                        }
                        break;
                }

                FlexboxAlignItems alignItems;

                switch (GetAlignSelf(child))
                {
                    default:
                    case FlexboxAlignSelf.Auto:
                        alignItems = AlignItems;
                        break;
                    case FlexboxAlignSelf.Start:
                        alignItems = FlexboxAlignItems.Start;
                        break;
                    case FlexboxAlignSelf.End:
                        alignItems = FlexboxAlignItems.End;
                        break;
                    case FlexboxAlignSelf.Center:
                        alignItems = FlexboxAlignItems.Center;
                        break;
                    case FlexboxAlignSelf.Baseline:
                        alignItems = FlexboxAlignItems.Baseline;
                        break;
                    case FlexboxAlignSelf.Stretch:
                        alignItems = FlexboxAlignItems.Stretch;
                        break;

                }

                switch (alignItems)
                {
                    case FlexboxAlignItems.Start:
                        break;
                    case FlexboxAlignItems.Center:
                        crossOffset += (excessCrossAxisInRow * 0.5);
                        break;
                    case FlexboxAlignItems.End:
                        crossOffset += excessCrossAxisInRow;
                        break;
                    case FlexboxAlignItems.Stretch:
                        childDesiredSize = CreateSize(MainAxis(childDesiredSize), (usedInCurrentCrossAxis > 0 ? usedInCurrentCrossAxis : info.CrossAxis));
                        break;
                    case FlexboxAlignItems.Baseline:
                        // TODO: How do we support this?
                        crossOffset += (excessCrossAxisInRow * 0.5);
                        break;
                }

                child.Arrange(new Rect(CreatePoint(mainOffset, crossOffset), childDesiredSize));

                usedInCurrentMainAxis += MainAxis(childDesiredSize);
            }
            return finalSize;
        }

        private List<UIElement> ChildrenSortedByOrder()
        {
            List<UIElement> sorted = new List<UIElement>(Children);

            sorted.Sort((UIElement a, UIElement b) =>
            {
                // That's not actually what this means. It's not about order reversal, but about LTR versus RTL. I _think_
                return IsReversed ?
                    (GetOrder(b) - GetOrder(a)) :
                    (GetOrder(a) - GetOrder(b));
            });

            return sorted;
        }

        private bool IsHorizontal
        {
            get
            {
                return (Direction == FlexboxDirection.Row || Direction == FlexboxDirection.RowReverse);
            }
        }

        private bool IsReversed
        {
            get
            {
                return (Direction == FlexboxDirection.RowReverse || Direction == FlexboxDirection.ColumnReverse);
            }
        }

        // TODO: (nowrap | wrap | wrap-reverse (grows up instead of down))
        public bool Wrap
        {
            get
            {
                return _wrap;
            }
            set
            {
                if (_wrap != value)
                {
                    _wrap = value;
                    InvalidateMeasure();
                }
            }
        }
        private bool _wrap = true;

        public FlexboxDirection Direction
        {
            get
            {
                return _direction;
            }
            set
            {
                if (_direction != value)
                {
                    _direction = value;
                    InvalidateMeasure();
                }
            }
        }
        private FlexboxDirection _direction = FlexboxDirection.Row;

        // Main-axis alignment of the entire box
        public FlexboxJustifyContent JustifyContent
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
        private FlexboxJustifyContent _justifyContent = FlexboxJustifyContent.Start;

        // Cross-axis alignment of items
        public FlexboxAlignItems AlignItems
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
        private FlexboxAlignItems _alignItems = FlexboxAlignItems.Start;

        // Cross-axis alignment of lines
        public FlexboxAlignContent AlignContent
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
        private FlexboxAlignContent _alignContent = FlexboxAlignContent.Start;
    }
}
