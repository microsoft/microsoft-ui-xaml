// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using System.Collections.ObjectModel;
using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;
namespace MUXControlsTestApp
{
    public class TabDataItemTemp : DependencyObject
    {
        public String Header { get; set; }
        public SymbolIconSource IconSource { get; set; }
        public String Content { get; set; }
    }
    [TopLevelTestPage(Name = "ComboBox", Icon = "ComboBox.png")]
    public sealed partial class ComboBoxPage : TestPage
    {
        SymbolIconSource _iconSource;
        int myIndex = 5;
        public ComboBoxPage()
        {
            this.InitializeComponent();
            //Tabs.Loaded += Tabs_Loaded;
            //Tabs.SelectionChanged += Tabs_SelectionChanged;
            _iconSource = new SymbolIconSource();
            _iconSource.Symbol = Symbol.Placeholder;


            ObservableCollection<TabDataItemTemp> itemSource = new ObservableCollection<TabDataItemTemp>();
            for (int i = 0; i < 5; i++)
            {
                var item = new TabDataItemTemp();
                item.IconSource = _iconSource;
                item.Header = "Item " + i;
                item.Content = "This is tab " + i + ".";
                itemSource.Add(item);
            }
            DataBindingTabView.TabItemsSource = itemSource;
            //TabViewListView lv = FindVisualChildByName(Tabs, "TabListView") as TabViewListView;
            //lv.ContainerContentChanging += Lv_ContainerContentChanging;
        }

        private void Lv_ContainerContentChanging(ListViewBase sender, ContainerContentChangingEventArgs args)
        {
            var m = 5;
            m += 1;
        }

        private void Tabs_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
        // throw new NotImplementedException();
        }

        private void Tabs_Loaded(object sender, RoutedEventArgs e)
        {
            //var myList = Tabs.TabItems;
           // throw new NotImplementedException();
        }

        private void Tabs_AddTabButtonClick(TabView sender, object e)
        {
            var newTab = new TabViewItem();
            newTab.IconSource = new Microsoft.UI.Xaml.Controls.SymbolIconSource() { Symbol = Symbol.Document };
            newTab.Header = "New Document";
           // newTab.Content = newTab.Header;

            sender.TabItems.Add(newTab);
        }

        // Remove the requested tab from the TabView
        private void Tabs_TabCloseRequested(TabView sender, TabViewTabCloseRequestedEventArgs args)
        {
            sender.TabItems.Remove(args.Tab);
        }

        private void ClearListView(object sender, RoutedEventArgs args)
        {
            /*var index = Tabs.SelectedIndex;

            Tabs.TabItems.Clear();
            var newIndex = Tabs.SelectedIndex;
            var k = 1;
            var newTab1 = new TabViewItem();
            newTab1.Header = "tab 1";
            var newTab2 = new TabViewItem();
            newTab2.Header = "tab 2";
            Tabs.TabItems.Add(newTab1);
            var index3 = Tabs.SelectedIndex;
            Tabs.TabItems.Add(newTab2);
            var index4 = Tabs.SelectedIndex;
            var whatever = 2;*/
            /*var indexBefore = MyListCheck.SelectedIndex;
            MyListCheck.Items.Clear();

            var index = MyListCheck.SelectedIndex;
            var k = false;*/
        }
    }
}
