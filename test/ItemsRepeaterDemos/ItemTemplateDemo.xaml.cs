using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    public sealed partial class ItemTemplateDemo : Page
    {
        public ItemTemplateDemo()
        {
            this.InitializeComponent();

            repeater.ItemsSource = Enumerable.Range(0, 10);
        }
    }

    public class MySelector: DataTemplateSelector
    {
        public DataTemplate Template1 { get; set; }

        public DataTemplate Template2 { get; set; }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            return ((int)item) % 2 == 0 ? Template1 : Template2;
        }
    }
}





































//public class MyElementFactory : IElementFactoryShim
//{
//    public UIElement GetElement(Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs args)
//    {
//        return new Button() { Content = args.Data };

//    }

//    public void RecycleElement(Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs args)
//    {

//    }
//}

//public class MyElementFactory: IElementFactoryShim
//{
//    List<Button> pool = new List<Button>();

//    public UIElement GetElement(Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs args)
//    {
//        Button element = null;
//        if(pool.Count > 0)
//        {
//            element = pool[0];
//            pool.RemoveAt(0);
//        }
//        else
//        {
//            element = new Button();
//        }

//        element.Content = args.Data;
//        element.Width = ((int)args.Data) % 2 == 0 ? 100 : 150;
//        return element;
//    }

//    public void RecycleElement(Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs args)
//    {
//        pool.Add(args.Element as Button);
//    }
//}