using System;
using System.Collections.Generic;
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
using mux = Microsoft.UI.Xaml.Controls;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    /// 
    public sealed partial class HierarchicalNavigationViewMarkup : Page
    {
        int test = 0;

        public HierarchicalNavigationViewMarkup()
        {
            this.InitializeComponent();
        }

        private void ClickedItem(object sender, mux.NavigationViewItemInvokedEventArgs e)
        {
            var clickedItem = e.InvokedItem;
            var clickedItemContainer = e.InvokedItemContainer;
            //clickedItemContainer.Content = "I was clicked: " + test;
            test++;
        }

        private void PrintSelectedItem(object sender, RoutedEventArgs e)
        {
            var selectedItem = navview.SelectedItem;
            if (selectedItem != null)
            {
                var label = (String)((mux.NavigationViewItem)selectedItem).Content;
                SelectedItemLabel.Text = label;
            }
        }

        private void CollapseSelectedItem(object sender, RoutedEventArgs e)
        {
            var selectedItem = navview.SelectedItem;
            if(selectedItem != null)
            {
                var container = (mux.NavigationViewItem)navview.ContainerFromMenuItem(selectedItem);
                container.IsExpanded = false;
            }
        }

        private void RemoveSecondMenuItem(object sender, RoutedEventArgs e)
        {
            //var item = (mux.NavigationViewItem)navview.MenuItems[0];
            //var menuitemscount = item.MenuItems.Count;
            //var item2 = (mux.NavigationViewItem)item.MenuItems[0];
            //var menuitemscount1 = item2.MenuItems.Count;
            //item2.MenuItems.RemoveAt(0);
            //var menuitemscount2 = item2.MenuItems.Count;
            //var menuitemscount3 = item2.MenuItems.Count;
            var count = navview.MenuItems.Count;
            navview.MenuItems.RemoveAt(2);
            var count1 = navview.MenuItems.Count;
            var count2 = navview.MenuItems.Count;
        }
    }
}
