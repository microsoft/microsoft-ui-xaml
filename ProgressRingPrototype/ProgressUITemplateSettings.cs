using Windows.UI.Xaml;
using Windows.UI;

namespace ProgressRingPrototype
{
    public class ProgressUITemplateSettings : DependencyObject
    {
        public double ProgressPosition
        {
            get { return (double)GetValue(ProgressPositionProperty); }
            set { SetValue(ProgressPositionProperty, value); }
        }

        public static readonly DependencyProperty ProgressPositionProperty =
            DependencyProperty.Register("ProgressPosition", typeof(double), typeof(ProgressUITemplateSettings), new PropertyMetadata(0));






        public Color ForegroundColor
        {
            get { return (Color)GetValue(ForegroundColorProperty); }
            set { SetValue(ForegroundColorProperty, value); }
        }

        public static readonly DependencyProperty ForegroundColorProperty =
            DependencyProperty.Register("ForegroundColor", typeof(Color), typeof(ProgressUITemplateSettings), new PropertyMetadata(0));





        // TODO: Add a property for circle arc segment properties etc to support storyboards possibly in the future if we get asks.


    }
}
