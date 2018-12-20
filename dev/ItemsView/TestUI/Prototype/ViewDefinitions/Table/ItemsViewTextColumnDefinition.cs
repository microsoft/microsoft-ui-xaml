using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    public class ItemsViewTextColumnDefinition : ItemsViewColumnDefinitionBase
    {
        public string Heading { get; set; }

        public string DisplayMemberPath { get; set; }

        protected override UIElement GetHeaderCore()
        {
            return new TextBlock()
            {
                Text = string.IsNullOrWhiteSpace(Heading) ? DisplayMemberPath : Heading
            };
        }

        protected override UIElement GetCellContentCore()
        {
            if (_template == null)
            {
                // TODO: Is there a way we can pass a binding onto this type instaed of directly giving a path? BindingBase in WPF?
                var templateStr = string.Format("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{{Binding {0}}}'/></DataTemplate>", DisplayMemberPath);
                _template = XamlReader.Load(templateStr) as DataTemplate;
            }

            return _template.LoadContent() as UIElement;
        }

        private DataTemplate _template;
    }
}
