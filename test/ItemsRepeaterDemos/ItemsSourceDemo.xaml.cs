using System.Linq;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    public sealed partial class ItemsSourceDemo : Page
    {
        public ItemsSourceDemo()
        {
            this.InitializeComponent();

            #region collection of objects

            repeater.ItemsSource = Enumerable.Range(0, 100);

            #endregion

            #region collection of UIElements

            //ObservableCollection<Button> source = new ObservableCollection<Button>();
            //for (int i = 0; i < 100; i++)
            //{
            //    source.Add(new Button() { Content = i });
            //}
            //repeater.ItemsSource = source;

            #endregion

        }
    }
}
