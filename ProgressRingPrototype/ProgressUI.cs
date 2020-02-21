using System;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;

namespace ProgressRingPrototype
{
    public sealed class ProgressUI : RangeBase
    {
        public ProgressUI()
        {
            this.DefaultStyleKey = typeof(ProgressUI);
        }

        public ProgressUITemplateSettings TemplateSettings { get; } = new ProgressUITemplateSettings();

        protected override void OnValueChanged(double oldValue, double newValue)
        {
            base.OnValueChanged(oldValue, newValue);

            var value = Value / (Maximum - Minimum);
            Debug.WriteLine("Setting Value: " + value);
            TemplateSettings.ProgressPosition = value;

            // TODO : If we want to go down the path of supporting storyboards, 
            // we can set templateSetings.arc* properties etc here.
        }

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();
            RegisterAndUpdateColors();
        }

        private void RegisterAndUpdateColors()
        {
            if (Foreground is SolidColorBrush)
            {
                var foreground = (Foreground as SolidColorBrush);
                RegisterPropertyChangedCallback(ForegroundProperty,
                    new DependencyPropertyChangedCallback(OnForegroundChanged));

                foreground.RegisterPropertyChangedCallback(SolidColorBrush.ColorProperty,
                    new DependencyPropertyChangedCallback(OnForegroundChanged));
            }

            if (Background is SolidColorBrush)
            {
                var background = (Background as SolidColorBrush);
                TemplateSettings.BackgroundColor = background.Color;
                RegisterPropertyChangedCallback(BackgroundProperty,
                   new DependencyPropertyChangedCallback(OnBackgroundChanged));

                background.RegisterPropertyChangedCallback(SolidColorBrush.ColorProperty,
                    new DependencyPropertyChangedCallback(OnBackgroundChanged));
            }
        }

        private void OnForegroundChanged(DependencyObject sender, DependencyProperty dp)
        {
            if (Foreground is SolidColorBrush)
            {
                TemplateSettings.ForegroundColor = (Foreground as SolidColorBrush).Color;
            }
        }

        private void OnBackgroundChanged(DependencyObject sender, DependencyProperty dp)
        {
            if (Background is SolidColorBrush)
            {
                TemplateSettings.BackgroundColor = (Background as SolidColorBrush).Color;
            }
        }

        public bool IsIndeterminate
        {
            get { return (bool)GetValue(IsIndeterminateProperty); }
            set { SetValue(IsIndeterminateProperty, value); }
        }

        public static readonly DependencyProperty IsIndeterminateProperty =
            DependencyProperty.Register("IsIndeterminate", typeof(bool), typeof(ProgressUI), new PropertyMetadata(false, new PropertyChangedCallback(OnIsIndeterminateChanged)));

        private static void OnIsIndeterminateChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            VisualStateManager.GoToState((ProgressUI)d, (bool)e.NewValue ? "Indeterminate" : "Normal", true);
        }


        public float StrokeWidth
        {
            get { return (float)GetValue(StrokeWidthProperty); }
            set { SetValue(StrokeWidthProperty, value); }
        }

        public static readonly DependencyProperty StrokeWidthProperty =
            DependencyProperty.Register("StrokeWidth", typeof(float), typeof(ProgressUI), new PropertyMetadata((float)1.0));
    }
}
