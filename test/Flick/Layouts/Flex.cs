using Microsoft.UI.Xaml.Controls;
using System;
using Windows.Foundation;
using Windows.UI.Xaml;

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
            DependencyProperty.RegisterAttached("FlexShrink", typeof(int), typeof(Flex), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));


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

        protected override Size MeasureOverride(NonVirtualizingLayoutContext context, Size availableSize)
        {
            double mainPosition = 0.0;
            double crossPosition = 0.0;
            double currentLineSize = 0;

            var children = context.Children;
            bool isReverse = FlexDirection == FlexDirection.RowReverse || FlexDirection == FlexDirection.ColumnReverse;
            for (int i = 0; i < children.Count; i++)
            {
                var child =  isReverse? children[children.Count -1 - i] : children[i];
                child.Measure(availableSize);
                
                if (FlexWrap == FlexWrap.Wrap)
                {
                    if (Main(availableSize) - (mainPosition + Main(child.DesiredSize)) < 0)
                    {
                        // wrap since the current item will not fit.
                        mainPosition = 0.0;
                        crossPosition += currentLineSize;
                        currentLineSize = 0;
                    }
                }

                // Let's position child at mainPosition, crossPosition 

                // Now calculate position for next child.
                mainPosition += Main(child.DesiredSize);
                currentLineSize = Math.Max(currentLineSize, Cross(child.DesiredSize));
            }

            return Size(mainPosition, crossPosition + currentLineSize);
        }

        protected override Size ArrangeOverride(NonVirtualizingLayoutContext context, Size finalSize)
        {
            double mainPosition = 0.0;
            double crossPosition = 0.0;
            double currentLineSize = 0;

            var children = context.Children;
            bool isReverse = FlexDirection == FlexDirection.RowReverse || FlexDirection == FlexDirection.ColumnReverse;
            for (int i=0; i< children.Count; i++)
            {
                var child = isReverse ? children[children.Count - 1 - i] : children[i];
                var desired = child.DesiredSize;

                if (FlexWrap == FlexWrap.Wrap)
                {
                    if (Main(finalSize) - (mainPosition + Main(child.DesiredSize)) < 0)
                    {
                        // wrap since the current item will not fit.
                        mainPosition = 0.0;
                        crossPosition += currentLineSize;
                        currentLineSize = 0;
                    }
                }

                child.Arrange(Rect(mainPosition, crossPosition, Main(desired), Cross(desired)));

                // Now calculate position for next child.
                mainPosition += Main(desired);
                currentLineSize = Math.Max(currentLineSize, Cross(child.DesiredSize));
            }

            return finalSize;
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            (d as Flex).InvalidateMeasure();
        }

        #region Axis Helpers

        private double Main(Point point)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                case FlexDirection.RowReverse:
                    value = point.X;
                    break;
                case FlexDirection.Column:
                case FlexDirection.ColumnReverse:
                    value = point.Y;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

        private double Cross(Point point)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                case FlexDirection.RowReverse:
                    value = point.Y;
                    break;
                case FlexDirection.Column:
                case FlexDirection.ColumnReverse:
                    value = point.X;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

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

        private double MainStart(Rect rect)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                    value = rect.X;
                    break;
                case FlexDirection.RowReverse:
                    value = rect.Right;
                    break;
                case FlexDirection.Column:
                    value = rect.Y;
                    break;
                case FlexDirection.ColumnReverse:
                    value = rect.Bottom;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

        private double MainEnd(Rect rect)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                    value = rect.Right;
                    break;
                case FlexDirection.RowReverse:
                    value = rect.X;
                    break;
                case FlexDirection.Column:
                    value = rect.Bottom;
                    break;
                case FlexDirection.ColumnReverse:
                    value = rect.Y;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

        private double CrossStart(Rect rect)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                    value = rect.Y;
                    break;
                case FlexDirection.RowReverse:
                    value = rect.Right;
                    break;
                case FlexDirection.Column:
                    value = rect.X;
                    break;
                case FlexDirection.ColumnReverse:
                    value = rect.Bottom;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

        private double CrossEnd(Rect rect)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                    value = rect.Right;
                    break;
                case FlexDirection.RowReverse:
                    value = rect.X;
                    break;
                case FlexDirection.Column:
                    value = rect.Bottom;
                    break;
                case FlexDirection.ColumnReverse:
                    value = rect.Y;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

        private double MainSize(Rect rect)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                case FlexDirection.RowReverse:
                    value = rect.Width;
                    break;
                case FlexDirection.Column:
                case FlexDirection.ColumnReverse:
                    value = rect.Height;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

        private double CrossSize(Rect rect)
        {
            double value = 0.0;
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                case FlexDirection.RowReverse:
                    value = rect.Height;
                    break;
                case FlexDirection.Column:
                case FlexDirection.ColumnReverse:
                    value = rect.Width;
                    break;
                default:
                    throw new NotImplementedException();
            }

            return value;
        }

        private Point Point(double main, double cross)
        {
            switch (FlexDirection)
            {
                case FlexDirection.Row:
                case FlexDirection.RowReverse:
                    return new Point(main, cross);
                case FlexDirection.Column:
                case FlexDirection.ColumnReverse:
                    return new Point(cross, main);
                default:
                    throw new NotImplementedException();
            }
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
        SpaceBetween,
        SpaceAround,
        SpaceEvenly
    }

    #endregion
}
