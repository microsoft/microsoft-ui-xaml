using System.Reflection;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    public sealed partial class GroupingDemo : NavPage
    {
        public GroupingDemo()
        {
            this.InitializeComponent();
            
            repeater.ItemsSource = Assembly.GetExecutingAssembly().GetTypes();
        }
    }
}
