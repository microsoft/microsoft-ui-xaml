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

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    /// 

    public class Category
    {
        public String Name { get; set; }
        public String Icon { get; set; }
        public ObservableCollection<Category> Children { get; set; }

        public Category(String name, String icon, ObservableCollection<Category> children)
        {
            this.Name = name;
            this.Icon = icon;
            this.Children = children;
        }
    }

    public sealed partial class HierarchicalNavigationViewDataBinding : Page
    {

        ObservableCollection<Category> categories = new ObservableCollection<Category>();

        public HierarchicalNavigationViewDataBinding()
        {
            this.InitializeComponent();

            var categories3 = new ObservableCollection<Category>();
            categories3.Add(new Category("Menu Item C", "Icon", null));
            categories3.Add(new Category("Menu Item D", "Icon", null));

            var categories2 = new ObservableCollection<Category>();
            categories2.Add(new Category("Menu Item B", "Icon", categories3));

            categories.Add(new Category("Menu Item A", "Icon", categories2));
            categories.Add(new Category("Menu Item E", "Icon", null));
            categories.Add(new Category("Menu Item F", "Icon", null));

        }
    }
}
