using System.Reflection;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    public sealed partial class GroupingDemo : Page
    {
        public GroupingDemo()
        {
            this.InitializeComponent();
            
            repeater.ItemsSource = Assembly.GetExecutingAssembly().GetTypes();
        }
    }
}
