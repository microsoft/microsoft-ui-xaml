using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    public sealed partial class KeyIndexMappingDemo : Page
    {
        public KeyIndexMappingDemo()
        {
            this.InitializeComponent();
        }
    }

//    #region KeyIndexMapping

//    ObservableCollection<Button> source = new ObservableCollection<Button>();
//            for (int i = 0; i< 100; i++)
//            {
//                source.Add(new Button() { Content = i });
//            }
//repeater.ItemsSource = source;

//            #endregion

//    public class MyObservableCollection : ObservableCollection<int>, IKeyIndexMapping
//{
//    public string KeyFromIndex(int index)
//    {
//        return this[index].ToString();
//    }

//    public int IndexFromKey(string key)
//    {
//        return IndexOf(int.Parse(key));
//    }
//}
}
