using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI.Xaml;

// Notes:
// (1) Item's Horizontal and Vertical Alignment conflict 
// with align-content stretch and align-items stretch
// For these two to work, we need to have the xaml alignments be set to stretch.
//
// (2) Is there equivalent of align-items baseline in xaml ?
//
// (3) order is not impl - don't think it is valuable, we
// can just change the order of the children, needs a sort of children :/
//
// (4) FlexWrap-WrapReverse not impl - not sure if it is valuable. Need one more pass?
//
// todo for perf - possibly cache dependency properties since they dont change often.
//
namespace Flick
{
    public class Flex : NonVirtualizingLayout
    {

        #region Dependency Properties

        public FlexDirection FlexDirection
        {
            get { return (FlexDirection)GetValue(FlexDirectionProperty); }
            set { SetValue(FlexDirectionProperty, value); }
        }

        public static readonly DependencyProperty FlexDirectionProperty =
            DependencyProperty.Register("FlexDirection", typeof(FlexDirection), typeof(Flex), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));

        public FlexWrap FlexWrap
        {
            get { return (FlexWrap)GetValue(FlexWrapProperty); }
            set { SetValue(FlexWrapProperty, value); }
        }

        public static readonly DependencyProperty FlexWrapProperty =
            DependencyProperty.Register("FlexWrap", typeof(FlexWrap), typeof(Flex), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));


        public JustifyContent JustifyContent
        {
            get { return (JustifyContent)GetValue(JustifyContentProperty); }
            set { SetValue(JustifyContentProperty, value); }
        }

        public static readonly DependencyProperty JustifyContentProperty =
            DependencyProperty.Register("JustifyContent", typeof(JustifyContent), typeof(Flex), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));

        public AlignItems AlignItems
        {
            get { return (AlignItems)GetValue(AlignItemsProperty); }
            set { SetValue(AlignItemsProperty, value); }
        }

        public static readonly DependencyProperty AlignItemsProperty =
            DependencyProperty.Register("AlignItems", typeof(AlignItems), typeof(Flex), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));

        public AlignContent AlignContent
        {
            get { return (AlignContent)GetValue(AlignContentProperty); }
            set { SetValue(AlignContentProperty, value); }
        }

        public static readonly DependencyProperty AlignContentProperty =
            DependencyProperty.Register("AlignContent", typeof(AlignContent), typeof(Flex), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));

        public static AlignItems GetAlignSelf(DependencyObject obj)
        {
            return (AlignItems)obj.GetValue(AlignSelfProperty);
        }

        public static void SetAlignSelf(DependencyObject obj, AlignItems value)
        {
            obj.SetValue(AlignSelfProperty, value);
        }

        public static readonly DependencyProperty AlignSelfProperty =
            DependencyProperty.RegisterAttached("AlignSelf", typeof(AlignItems), typeof(Flex), new PropertyMetadata(0));


        public static int GetFlexGrow(DependencyObject obj)
        {
            return (int)obj.GetValue(FlexGrowProperty);
        }

        public static void SetFlexGrow(DependencyObject obj, int value)
        {
            obj.SetValue(FlexGrowProperty, value);
        }

        public static readonly DependencyProperty FlexGrowProperty =
            DependencyProperty.RegisterAttached("FlexGrow", typeof(int), typeof(Flex), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));


        public static int GetFlexShrink(DependencyObject obj)
        {
            return (int)obj.GetValue(FlexShrinkProperty);
        }

        public static void SetFlexShrink(DependencyObject obj, int value)
        {
            obj.SetValue(FlexShrinkProperty, value);
        }

        public static readonly DependencyProperty FlexShrinkProperty =
            DependencyProperty.RegisterAttached("FlexShrink", typeof(int), typeof(Flex), new PropertyMetadata(1, new PropertyChangedCallback(OnPropertyChanged)));


        public static int GetFlexBasis(DependencyObject obj)
        {
            return (int)obj.GetValue(FlexBasisProperty);
        }

        public static void SetFlexBasis(DependencyObject obj, int value)
        {
            obj.SetValue(FlexBasisProperty, value);
        }

        public static readonly DependencyProperty FlexBasisProperty =
            DependencyProperty.RegisterAttached("FlexBasis", typeof(int), typeof(Flex), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));

        #endregion

        protected override void InitializeForContextCore(NonVirtualizingLayoutContext context)
        {
            context.LayoutState = new FlexState();
        }

        protected override void UninitializeForContextCore(NonVirtualizingLayoutContext context)
        {
            context.LayoutState = null;
        }

        private FlexState State(NonVirtualizingLayoutContext context)
        {
            return context.LayoutState as FlexState;
        }

        // TODO: Separate concern of order/iteration away from here to simplify
        protected override Size MeasureOverride(NonVirtualizingLayoutContext context, Size availableSize)
        {
            var state = State(context);
            double mainPosition = 0.0;
            double crossPosition = 0.0;
            var children = context.Children;
            bool isReverse = FlexDirection == FlexDirection.RowReverse || FlexDirection == FlexDirection.ColumnReverse;
            state.Lines.Clear();
            var currentLine = new LineInfo();
            currentLine.StartIndex = isReverse ? children.Count - 1 : 0;
            for (int i = 0; i < children.Count; i++)
            {
                var childIndex = isReverse ? children.Count - 1 - i : i;
                var child = children[childIndex];
                var basis = GetFlexBasis(child);
                var measureSize = basis != 0 ? Size(Math.Min(basis, Main(availableSize)), Cross(availableSize)) : availableSize;
                child.Measure(measureSize);

                if (FlexWrap == FlexWrap.Wrap)
                {
                    if (Main(availableSize) - (mainPosition + Main(child.DesiredSize)) < 0)
                    {
                        // wrap since the current item will not fit.
                        currentLine.MainSize = mainPosition;
                        GrowLineIfNeeded(ref currentLine, children, availableSize, isReverse);
                        ShrinkLineIfNeeded(ref currentLine, children, availableSize, isReverse);
                        state.Lines.Add(currentLine);
                        mainPosition = 0.0;
                        crossPosition += currentLine.CrossSize;
                        currentLine = new LineInfo();
                        currentLine.StartIndex = childIndex;
                        currentLine.CrossPosition = crossPosition;
                    }
                }

                // Let's position child at mainPosition, crossPosition 
                currentLine.SumGrow += GetFlexGrow(child);
                currentLine.SumShrink += GetFlexShrink(child);
                currentLine.CountInLine++;

                // Now calculate position for next child.
                mainPosition += Main(child.DesiredSize);
                currentLine.CrossSize = Math.Max(currentLine.CrossSize, Cross(child.DesiredSize));
            }

            if (currentLine.CountInLine > 0)
            {
                currentLine.MainSize = mainPosition;
                GrowLineIfNeeded(ref currentLine, children, availableSize, isReverse);
                ShrinkLineIfNeeded(ref currentLine, children, availableSize, isReverse);
                state.Lines.Add(currentLine);
            }

            state.LastExtent = Size(mainPosition, crossPosition + currentLine.CrossSize);
            return state.LastExtent;
        }

        protected override Size ArrangeOverride(NonVirtualizingLayoutContext context, Size finalSize)
        {
            var state = State(context);
            bool isReverse = FlexDirection == FlexDirection.RowReverse || FlexDirection == FlexDirection.ColumnReverse;
            int step = isReverse ? -1 : 1;
            int childIndex = isReverse ? context.Children.Count - 1 : 0;
            var layoutItemAlignment = AlignItems;
            double extraCrossSpaceInExtent = Cross(finalSize) - Cross(state.LastExtent);
            for (int lineIndex = 0; lineIndex < state.Lines.Count; lineIndex++)
            {
                var currentLine = state.Lines[lineIndex];
                var mainPosition = 0.0;

                double crossContentOffset = GetContentAlignedCrossOffset(extraCrossSpaceInExtent, lineIndex, state);
                double extraMainSpaceInLine = Main(finalSize) - currentLine.MainSize;

                for (int indexInLine = 0; indexInLine < currentLine.CountInLine; indexInLine++)
                {
                    var currentChild = context.Children[childIndex];
                    var childAlign = GetAlignSelf(currentChild);
                    var itemAlignment = childAlign == AlignItems.Auto ? layoutItemAlignment : childAlign;
                    double itemMainSize = Main(currentChild.DesiredSize);
                    double itemCrossSize = Cross(currentChild.DesiredSize);

                    double mainOffset = mainPosition;
                    if (extraMainSpaceInLine > 0)
                    {
                        mainOffset += GetContentJustifiedMainOffset(currentLine.CountInLine, extraMainSpaceInLine, indexInLine);
                    }

                    double crossOffset = currentLine.CrossPosition + crossContentOffset + GetItemsAlignedCrossOffset(currentLine, itemCrossSize, itemAlignment);

                    if (AlignContent == AlignContent.Stretch)
                    {
                        itemCrossSize = currentLine.CrossSize + extraCrossSpaceInExtent / state.Lines.Count;
                    }
                    else if (itemAlignment == AlignItems.Stretch)
                    {
                        itemCrossSize = currentLine.CrossSize;
                    }

                    currentChild.Arrange(Rect(mainOffset, crossOffset, itemMainSize, itemCrossSize));
                    mainPosition += Main(currentChild.DesiredSize);

                    childIndex += step;
                }
            }

            return finalSize;
        }

        private void GrowLineIfNeeded(ref LineInfo currentLine, IReadOnlyList<UIElement> children, Size availableSize, bool isReverse)
        {
            var mainAvailableSize = Main(availableSize);
            if (currentLine.SumGrow > 0 && mainAvailableSize > currentLine.MainSize)
            {
                var extraMainSpaceInLine = mainAvailableSize - currentLine.MainSize;
                // Line eats up all the space.
                currentLine.MainSize = Main(availableSize);
                currentLine.CrossSize = 0.0;
                for (int i = 0; i < currentLine.CountInLine; i++)
                {
                    // grow the item
                    var childIndex = isReverse ? currentLine.StartIndex - i : currentLine.StartIndex + i;
                    var currentChild = children[childIndex];
                    var currentChildFlexGrow = GetFlexGrow(currentChild);
                    // no need to re-measure if we are not going to grow this item.
                    if (currentChildFlexGrow > 0)
                    {
                        var growBy = (extraMainSpaceInLine / currentLine.SumGrow) * currentChildFlexGrow;
                        var basis = GetFlexBasis(currentChild);
                        var measureSize = basis != 0 ? Size(Math.Min(basis + growBy, Main(availableSize)), Cross(availableSize)) : availableSize;
                        currentChild.Measure(measureSize);
                    }
                    currentLine.CrossSize = Math.Max(currentLine.CrossSize, Cross(currentChild.DesiredSize));
                }
            }
        }

        private void ShrinkLineIfNeeded(ref LineInfo currentLine, IReadOnlyList<UIElement> children, Size availableSize, bool isReverse)
        {
            var mainAvailableSize = Main(availableSize);
            if (currentLine.SumShrink > 0 && mainAvailableSize < currentLine.MainSize)
            {
                var deficitMainSpaceInLine = currentLine.MainSize - mainAvailableSize;
                // Line eats up all the space.
                currentLine.MainSize = Main(availableSize);
                currentLine.CrossSize = 0.0;
                for (int i = 0; i < currentLine.CountInLine; i++)
                {
                    // grow the item
                    var childIndex = isReverse ? currentLine.StartIndex - i : currentLine.StartIndex + i;
                    var currentChild = children[childIndex];
                    var currentChildFlexShrink = GetFlexShrink(currentChild);
                    // no need to re-measure if we are not going to grow this item.
                    if (currentChildFlexShrink > 0)
                    {
                        var shrinkBy = (deficitMainSpaceInLine / currentLine.SumShrink) * currentChildFlexShrink;
                        var basis = GetFlexBasis(currentChild);
                        var measureSize = basis != 0 ? Size(Math.Min(basis - shrinkBy, Main(availableSize)), Cross(availableSize)) : availableSize;
                        currentChild.Measure(measureSize);
                    }
                    currentLine.CrossSize = Math.Max(currentLine.CrossSize, Cross(currentChild.DesiredSize));
                }
            }
        }

        private double GetContentAlignedCrossOffset(double extraCrossSpaceInExtent, int lineIndex, FlexState state)
        {
            double crossContentOffset = 0;
            if (extraCrossSpaceInExtent > 0)
            {
                switch (AlignContent)
                {
                    case AlignContent.FlexStart:
                        break;
                    case AlignContent.FlexEnd:
                        crossContentOffset = extraCrossSpaceInExtent;
                        break;
                    case AlignContent.Center:
                        crossContentOffset = extraCrossSpaceInExtent / 2;
                        break;
                    case AlignContent.Stretch:
                        crossContentOffset = lineIndex * extraCrossSpaceInExtent / (state.Lines.Count);
                        break;
                    case AlignContent.SpaceBetween:
                        if (state.Lines.Count > 1)
                        {
                            crossContentOffset = lineIndex * (extraCrossSpaceInExtent / (state.Lines.Count - 1));
                        }
                        break;
                    case AlignContent.SpaceAround:
                        crossContentOffset = (lineIndex * 2 + 1) * extraCrossSpaceInExtent / (state.Lines.Count * 2);
                        break;
                }
            }

            return crossContentOffset;
        }

        private double GetItemsAlignedCrossOffset(LineInfo line, double itemCrossSize, AlignItems alignment)
        {
            double crossOffset = 0.0;
            double extraCrossSpace = line.CrossSize - itemCrossSize;
            switch (alignment)
            {
                case AlignItems.Auto:
                case AlignItems.FlexStart:
                    break;
                case AlignItems.FlexEnd:
                    crossOffset = extraCrossSpace;
                    break;
                case AlignItems.Center:
                    crossOffset = extraCrossSpace / 2;
                    break;
                case AlignItems.Stretch:
                    break;
                default:
                    throw new NotImplementedException();
            }

            return crossOffset;
        }

        private double GetContentJustifiedMainOffset(int countInLine, double extraMainSpace, int itemIndex)
        {
            double mainOffset = 0;
            switch (JustifyContent)
            {
                case JustifyContent.FlexStart:
                    break;
                case JustifyContent.FlexEnd:
                    mainOffset = extraMainSpace;
                    break;
                case JustifyContent.Center:
                    mainOffset = extraMainSpace / 2;
                    break;
                case JustifyContent.SpaceBetween:
                    mainOffset = countInLine > 1 ?
                        itemIndex * (extraMainSpace / (countInLine - 1)) :
                        0;
                    break;
                case JustifyContent.SpaceAround:
                    mainOffset = extraMainSpace / (countInLine * 2) + (itemIndex) * (extraMainSpace / countInLine);
                    break;
                case JustifyContent.SpaceEvenly:
                    mainOffset = (itemIndex + 1) * (extraMainSpace / (countInLine + 1));
                    break;

                default:
                    throw new NotImplementedException();
            }

            return mainOffset;
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var layout = (d as Flex);
            if (layout != null)
            {
                layout.InvalidateMeasure();
            }
            else if (d is UIElement)
            {
                (d as UIElement).InvalidateMeasure();
            }
        }

        #region Axis Helpers

        private double Main(Size size)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                case FlexDirection.RowReverse:
                    value = size.Width;
                    break;
                case FlexDirection.Column:
                case FlexDirection.ColumnReverse:
                    value = size.Height;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

        private double Cross(Size size)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                case FlexDirection.RowReverse:
                    value = size.Height;
                    break;
                case FlexDirection.Column:
                case FlexDirection.ColumnReverse:
                    value = size.Width;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

        private Size Size(double main, double cross)
        {
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                case FlexDirection.RowReverse:
                    return new Size(main, cross);
                case FlexDirection.Column:
                case FlexDirection.ColumnReverse:
                    return new Size(cross, main);
                default:
                    throw new NotImplementedException();
            }
        }

        private Rect Rect(double main, double cross, double mainSize, double crossSize)
        {
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                case FlexDirection.RowReverse:
                    return new Rect(main, cross, mainSize, crossSize);
                case FlexDirection.Column:
                case FlexDirection.ColumnReverse:
                    return new Rect(cross, main, crossSize, mainSize);
                default:
                    throw new NotImplementedException();
            }
        }

        #endregion

        class LineInfo
        {
            public double CrossPosition { get; set; }
            public int StartIndex { get; set; }
            public int CountInLine { get; set; }
            public double MainSize { get; set; }
            public double CrossSize { get; set; }
            public int SumGrow { get; set; }
            public int SumShrink { get; set; }

            public override string ToString()
            {
                return $"Offset:{CrossPosition} Count:{CountInLine} Main:{MainSize} Cross:{CrossSize}";
            }
        };

        class FlexState
        {
            public List<LineInfo> Lines { get; set; } = new List<LineInfo>();
            public Size LastExtent { get; set; }
        }

    }

    #region Enums
    public enum FlexDirection
    {
        Row,
        RowReverse,
        Column,
        ColumnReverse
    };

    public enum FlexWrap
    {
        NoWrap,
        Wrap,
        // WrapReverse
    };

    public enum JustifyContent
    {
        FlexStart,
        FlexEnd,
        Center,
        SpaceBetween,
        SpaceAround,
        SpaceEvenly
    };

    public enum AlignItems
    {
        Auto,
        FlexStart,
        FlexEnd,
        Center,
        Stretch,
        // Baseline
    };

    public enum AlignContent
    {
        FlexStart,
        FlexEnd,
        Center,
        Stretch,
        SpaceBetween,
        SpaceAround
    }

    #endregion
}
