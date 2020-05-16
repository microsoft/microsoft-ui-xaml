using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace MUXControlsTestApp.Samples
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class NestedTreeSample : Page
    {
        public NestedTreeSample()
        {
            this.InitializeComponent();
        }
        private List<int> Teams = Enumerable.Range(0, 50).ToList();

        private ObservableCollection<object> TreeData = Data.CreateNested(10, 3, 3);
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
