using Microsoft.UI.Xaml.Controls;
using System;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

#if !BUILD_WINDOWS
using Layout = Microsoft.UI.Xaml.Controls.Layout;
#endif

namespace DEPControlsTestApp.ItemsViewPrototype
{
#pragma warning disable CS8305 // Microsoft.UI.Xaml.Controls.LayoutPanel is for evaluation purposes only and is subject to change or removal in future updates.
    public class LayoutPanel : Microsoft.UI.Xaml.Controls.LayoutPanel
    {
        /*
        private Layout _layout;
        private LayoutPanelLayoutContext _layoutContext;

        internal object LayoutState { get; set; }

        public Layout Layout
        {
            get { return _layout; }
            set
            {
                SetValue(LayoutProperty, value);
                InvalidateMeasure();
            }
        }

        public static readonly DependencyProperty LayoutProperty =
            DependencyProperty.Register(
                nameof(Layout),
                typeof(Layout),
                typeof(LayoutPanel),
                new PropertyMetadata(null, OnLayoutChanged));

        private static void OnLayoutChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var me = d as LayoutPanel;

            if (me._layoutContext == null)
            {
                me._layoutContext = new LayoutPanelLayoutContext(me);
            }

            if (me._layout != null)
            {
                me._layout.UninitializeForContext(me._layoutContext);
                me._layout.MeasureInvalidated -= me.OnLayoutInvalidated;
            }

            me._layout = e.NewValue as Layout;
            if (me._layout != null)
            {
                me._layout.InitializeForContext(me._layoutContext);
                me._layout.MeasureInvalidated += me.OnLayoutInvalidated;
            }
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            Size desiredSize = default(Size);

            if (Layout != null)
            {
                var extent = Layout.Measure(_layoutContext, availableSize);

                desiredSize.Width = extent.Width;
                desiredSize.Height = extent.Height;
            }

            return desiredSize;
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            if (Layout != null)
            {
                Layout.Arrange(_layoutContext, finalSize);
            }

            return new Size(
                Math.Max(finalSize.Width, DesiredSize.Width),
                Math.Max(finalSize.Height, DesiredSize.Height));
        }

        private void OnLayoutInvalidated(Layout sender, object args)
        {
            this.InvalidateMeasure();
        }

        public Thickness BorderThickness
        {
            get { return (Thickness)GetValue(BorderThicknessProperty); }
            set { SetValue(BorderThicknessProperty, value); }
        }

        public static readonly DependencyProperty BorderThicknessProperty =
            DependencyProperty.Register(
                nameof(BorderThickness),
                typeof(Thickness),
                typeof(LayoutPanel),
                new PropertyMetadata(default(Thickness)));

        public Brush BorderBrush
        {
            get { return (Brush)GetValue(BorderBrushProperty); }
            set { SetValue(BorderBrushProperty, value); }
        }

        public static readonly DependencyProperty BorderBrushProperty =
            DependencyProperty.Register(
                nameof(BorderBrush),
                typeof(Brush),
                typeof(LayoutPanel),
                new PropertyMetadata(null));
    */
    }
#pragma warning restore CS8305 // Microsoft.UI.Xaml.Controls.LayoutPanel is for evaluation purposes only and is subject to change or removal in future updates.
}
