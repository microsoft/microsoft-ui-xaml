using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    public class ViewDefinition: ViewDefinitionBase
    {
        public ViewGenerator ViewGenerator
        {
            get { return (ViewGenerator)GetValue(ViewGeneratorProperty); }
            set { SetValue(ViewGeneratorProperty, value); }
        }

        public static readonly DependencyProperty ViewGeneratorProperty =
            DependencyProperty.Register("ViewGenerator", typeof(ViewGenerator), typeof(ViewDefinitionBase), new PropertyMetadata(0));

        public VirtualizingLayoutBase Layout
        {
            get { return (VirtualizingLayoutBase)GetValue(LayoutProperty); }
            set { SetValue(LayoutProperty, value); }
        }

        public static readonly DependencyProperty LayoutProperty =
            DependencyProperty.Register("Layout", typeof(VirtualizingLayoutBase), typeof(ViewDefinitionBase), new PropertyMetadata(0));

        public UIElement Header
        {
            get { return (UIElement)GetValue(HeaderProperty); }
            set { SetValue(HeaderProperty, value); }
        }

        public static readonly DependencyProperty HeaderProperty =
            DependencyProperty.Register("Header", typeof(UIElement), typeof(VirtualizingLayoutBase), new PropertyMetadata(null));


        protected override UIElement GetHeaderCore()
        {
            return Header;
        }

        protected override ViewGenerator GetViewGeneratorCore()
        {
            return ViewGenerator;
        }

        protected override VirtualizingLayoutBase GetLayoutCore()
        {
            return Layout;
        }
    }
}
