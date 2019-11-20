using System.Collections.ObjectModel;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    public sealed partial class AnimationDemo : Page
    {
        ObservableCollection<int> Data = new ObservableCollection<int>();

        public AnimationDemo()
        {
            this.InitializeComponent();
            for(int i=0; i<10; i++)
            {
                Data.Add(i);
            }

            repeater.ItemsSource = Data;
        }

        private void InsertBeforeClick(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            var value = (int)(sender as Button).Tag;
            Data.Insert(Data.IndexOf(value), 1000 + value);
        }

        private void RemoveClick(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            var value = (int)(sender as Button).Tag;
            Data.RemoveAt(Data.IndexOf(value));
        }
    }
}
