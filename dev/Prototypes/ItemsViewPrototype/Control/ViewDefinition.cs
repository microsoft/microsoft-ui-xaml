using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class ViewDefinition : ViewDefinitionBase
    {
        private static Layout DefaultLayout = new StackLayout() { Orientation = Orientation.Vertical };

        public object ItemTemplate // Future: Switch type to be IElementFactory
        {
            get { return (object)GetValue(ItemTemplateProperty); }
            set { SetValue(ItemTemplateProperty, value); }
        }

        public static readonly DependencyProperty ItemTemplateProperty =
            DependencyProperty.Register(
                nameof(ItemTemplate),
                typeof(object),
                typeof(ViewDefinition),
                new PropertyMetadata(null));

        public VirtualizingLayout Layout
        {
            get { return (VirtualizingLayout)GetValue(LayoutProperty); }
            set { SetValue(LayoutProperty, value); }
        }

        public static readonly DependencyProperty LayoutProperty =
            DependencyProperty.Register(
                nameof(Layout),
                typeof(VirtualizingLayout),
                typeof(ViewDefinition),
                new PropertyMetadata(DefaultLayout));

        public UIElement Header
        {
            get { return (UIElement)GetValue(HeaderProperty); }
            set { SetValue(HeaderProperty, value); }
        }

        public static readonly DependencyProperty HeaderProperty =
            DependencyProperty.Register(
                nameof(Header),
                typeof(UIElement),
                typeof(ViewDefinition),
                new PropertyMetadata(null));


        protected override UIElement GetHeaderCore()
        {
            return Header;
        }

        protected override object GetElementFactoryCore()
        {
            return ItemTemplate;
        }

        protected override VirtualizingLayout GetLayoutCore()
        {
            return Layout;
        }
    }
}
