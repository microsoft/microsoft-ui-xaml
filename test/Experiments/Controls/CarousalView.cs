using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

namespace Experiments
{
    public class CarousalView : Control
    {
        public CarousalView()
        {
            this.DefaultStyleKey = typeof(CarousalView);
        }

        public object ItemsSource
        {
            get { return (object)GetValue(ItemsSourceProperty); }
            set { SetValue(ItemsSourceProperty, value); }
        }

        public static readonly DependencyProperty ItemsSourceProperty =
            DependencyProperty.Register("ItemsSource", typeof(object), typeof(CarousalView), new PropertyMetadata(0));


        public object ItemTemplate
        {
            get { return (object)GetValue(ItemTemplateProperty); }
            set { SetValue(ItemTemplateProperty, value); }
        }

        public static readonly DependencyProperty ItemTemplateProperty =
            DependencyProperty.Register("ItemTemplate", typeof(object), typeof(CarousalView), new PropertyMetadata(0));


        public double ItemWidth
        {
            get { return (double)GetValue(ItemWidthProperty); }
            set { SetValue(ItemWidthProperty, value); }
        }

        public static readonly DependencyProperty ItemWidthProperty =
            DependencyProperty.Register("ItemWidth", typeof(double), typeof(CarousalView), new PropertyMetadata(0));


        protected override void OnApplyTemplate()
        {
            m_previousButton = GetTemplateChild("PreviousButton") as ButtonBase;
            m_previousButton.Click += OnPreviousButtonClicked;
            m_nextButton = GetTemplateChild("NextButton") as ButtonBase;
            m_nextButton.Click += OnNextButtonClicked;
            m_ScrollViewer = GetTemplateChild("ScrollViewer") as ScrollViewer;
        }

        private void OnPreviousButtonClicked(object sender, RoutedEventArgs e)
        {
            m_ScrollViewer.ChangeView(m_ScrollViewer.HorizontalOffset - ItemWidth,
                null, null, false);
        }

        private void OnNextButtonClicked(object sender, RoutedEventArgs e)
        {
            m_ScrollViewer.ChangeView(m_ScrollViewer.HorizontalOffset + ItemWidth,
                 null, null, false);
        }

        private ButtonBase m_previousButton;
        private ButtonBase m_nextButton;
        private ScrollViewer m_ScrollViewer;
    }

    public class SnapPointForwardingGrid : Grid, IScrollSnapPointsInfo
    {
        public SnapPointForwardingGrid()
        {
        }

        public double HorizontalSnapPointSize
        {
            get { return (double)GetValue(HorizontalSnapPointSizeProperty); }
            set { SetValue(HorizontalSnapPointSizeProperty, value); }
        }

        public static readonly DependencyProperty HorizontalSnapPointSizeProperty =
            DependencyProperty.Register("HorizontalSnapPointSize", typeof(double), typeof(SnapPointForwardingGrid), new PropertyMetadata(0));

        public IReadOnlyList<float> GetIrregularSnapPoints(Orientation orientation, SnapPointsAlignment alignment)
        {
            return null;
        }

        public float GetRegularSnapPoints(Orientation orientation, SnapPointsAlignment alignment, out float offset)
        {
            if (alignment == SnapPointsAlignment.Center && 
                orientation == Orientation.Horizontal)
            {
                offset = (float)HorizontalSnapPointSize;
                return (float)HorizontalSnapPointSize;
            }

            offset = 0;
            return 0.0f;
        }

        public bool AreHorizontalSnapPointsRegular => true;

        public bool AreVerticalSnapPointsRegular => false;

        public event EventHandler<object> HorizontalSnapPointsChanged;
        public event EventHandler<object> VerticalSnapPointsChanged;
    }
}
