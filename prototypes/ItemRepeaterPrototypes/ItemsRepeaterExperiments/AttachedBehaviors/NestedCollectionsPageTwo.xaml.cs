using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;


// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace ItemsRepeaterExperiments.AttachedBehaviors
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class NestedCollectionsPageTwo : Page
    {
        public NestedCollectionsPageTwo()
        {
            this.InitializeComponent();
        }

        private List<int> Teams = Enumerable.Range(0, 50).ToList();

        private ObservableCollection<object> TreeData = Data.CreateNested(2, 2, 3);

        private void OnPointerMoved(object sender, PointerRoutedEventArgs e)
        {
            var ip = SelectionBehavior.GetIndexPath(sender as DependencyObject);
            indexPath.Text = ip.ToString();
            isSelected.Text = "IsSelected: " + treeSelectionModel.IsSelectedAt(ip).ToString();
            e.Handled = true;
        }
    }

    public class TreeTemplateSelector : DataTemplateSelector
    {
        public DataTemplate GroupTemplate { get; set; }

        public DataTemplate ItemTemplate { get; set; }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            return item is ObservableCollection<object> ? GroupTemplate : ItemTemplate;
        }
    }

    public class Data
    {
        static int _next = 0;

        public static ObservableCollection<object> CreateNested(int levels = 3, int groupsAtLevel = 5, int countAtLeaf = 10)
        {
            var data = new ObservableCollection<object>();
            if (levels != 0)
            {
                for (int i = 0; i < groupsAtLevel; i++)
                {
                    data.Add(CreateNested(levels - 1, groupsAtLevel, countAtLeaf));
                }
            }
            else
            {
                for (int i = 0; i < countAtLeaf; i++)
                {
                    data.Add(_next++);
                }
            }

            return data;
        }
    }
}
