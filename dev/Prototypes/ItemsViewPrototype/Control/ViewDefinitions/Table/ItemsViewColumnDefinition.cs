using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    [ContentProperty(Name = "CellTemplate")]
    public class ItemsViewColumnDefinition : ItemsViewColumnDefinitionBase
    {
        public string Heading { get; set; }

        public DataTemplate CellTemplate
        {
            get { return (DataTemplate)GetValue(CellTemplateProperty); }
            set { SetValue(CellTemplateProperty, value); }
        }

        public static readonly DependencyProperty CellTemplateProperty =
            DependencyProperty.Register("CellTemplate", typeof(DataTemplate), typeof(ItemsViewColumnDefinition), new PropertyMetadata(null));

        // TODO: Support GridLength

        protected override UIElement GetHeaderCore()
        {
            return new TextBlock() { Text = Heading };
        }

        protected override UIElement GetCellContentCore()
        {
            return CellTemplate.LoadContent() as UIElement;
        }
    }
}
