using System;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Hosting;

namespace Experiments
{
    public sealed partial class ShimmerPage : Page
    {
        public ShimmerPage()
        {
            this.InitializeComponent();
            button.Click += Button_Click;
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            var button = sender as Button;
            if (button.Tag == null)
            {
                StartShimmer(sender as FrameworkElement);
            }
            else
            {
                StopShimmer(button);
            }
        }

        void StartShimmer(FrameworkElement target)
        {
            var compositor = Window.Current.Compositor;
            var gradientStop = compositor.CreateColorGradientStop(0.5f, Colors.Gray);
            var linear = compositor.CreateLinearGradientBrush();
            linear.ColorStops.Add(compositor.CreateColorGradientStop(0.0f, Colors.DarkGray));
            linear.ColorStops.Add(gradientStop);
            linear.StartPoint = new System.Numerics.Vector2(0, 0);
            linear.EndPoint = new System.Numerics.Vector2(1, 0);
            linear.MappingMode = CompositionMappingMode.Relative;

            var stopAnim = compositor.CreateScalarKeyFrameAnimation();
            stopAnim.Duration = TimeSpan.FromSeconds(2);
            stopAnim.InsertKeyFrame(0, 0.1f);
            stopAnim.InsertKeyFrame(1, 1.0f);
            stopAnim.IterationBehavior = AnimationIterationBehavior.Forever;
            gradientStop.StartAnimation("Offset", stopAnim);

            var visual = compositor.CreateSpriteVisual();
            visual.RelativeSizeAdjustment = new System.Numerics.Vector2(1, 1);
            visual.Brush = linear;

            target.Tag = "foo";
            ElementCompositionPreview.SetElementChildVisual(target, visual);
        }

        void StopShimmer(FrameworkElement target)
        {
            ElementCompositionPreview.SetElementChildVisual(target, null);
            target.Tag = null;
        }
    }

    /* Open Questions 
     * 
     * - Which two colors should be used in each theme.
     * - What should happen if system animations are turned off 
     * 
     */
}
