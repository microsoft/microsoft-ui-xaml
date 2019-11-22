using System.Collections.ObjectModel;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    public sealed partial class MaintainViewportDemo : NavPage
    {
        ObservableCollection<int> Data = new ObservableCollection<int>();

        public MaintainViewportDemo()
        {
            this.InitializeComponent();
            for (int i = 0; i < 1000; i++)
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

        private void Button_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            int index;
            if(int.TryParse(indexTextBox.Text, out index))
            {
                Data.Insert(index, 12345);
            }
        }
    }
}
