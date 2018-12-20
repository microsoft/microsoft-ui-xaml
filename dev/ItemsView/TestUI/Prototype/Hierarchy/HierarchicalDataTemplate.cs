using Windows.UI.Xaml;
using Windows.UI.Xaml.Markup;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    [ContentProperty(Name = "Template")]
    public class HierarchicalDataTemplate: DataTemplate
    {
        public object ItemsSource
        {
            get { return (object)GetValue(ItemsSourceProperty); }
            set { SetValue(ItemsSourceProperty, value); }
        }

        public static readonly DependencyProperty ItemsSourceProperty =
            DependencyProperty.Register("ItemsSource", typeof(object), typeof(HierarchicalDataTemplate), new PropertyMetadata(0));

        public DataTemplate Template
        {
            get { return (DataTemplate)GetValue(MyPropertyProperty); }
            set { SetValue(MyPropertyProperty, value); }
        }

        public static readonly DependencyProperty MyPropertyProperty =
            DependencyProperty.Register("Template", typeof(DataTemplate), typeof(HierarchicalDataTemplate), new PropertyMetadata(0));



        public HierarchicalDataTemplate()
        {

        }
    }
}
