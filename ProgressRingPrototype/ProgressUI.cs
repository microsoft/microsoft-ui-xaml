using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls.Primitives;

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
    }
}
