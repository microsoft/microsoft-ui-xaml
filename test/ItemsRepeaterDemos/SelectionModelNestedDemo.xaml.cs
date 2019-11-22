using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    public sealed partial class SelectionModelNestedDemo : NavPage
    {
        static object _data = Data.CreateNested(3, 2, 4);
        public SelectionModelNestedDemo()
        {
            this.InitializeComponent();
            rootRepeater.ItemsSource = _data;
            selectionModel.Source = _data;
            selectionModel.PropertyChanged += SelectionModel_PropertyChanged;
            selectionModel.SingleSelect = true;
        }

        private void SelectionModel_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "SelectedItem")
            {
                var selected = (sender as SelectionModel).SelectedItem;
            }
        }

        private void ElementFactory_SelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = args.DataContext is IEnumerable ? "group" : "item";
        }

        private void OnBackClicked(object sender, RoutedEventArgs e)
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            Frame.GoBack();
        }

        private void OnMultipleSelectionClicked(object sender, RoutedEventArgs e)
        {
            selectionModel.SingleSelect = multipleSelection.IsChecked.Value ? false : true;
        }

        private void Repeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            if (args.Element is TreeViewItem)
            {
                (args.Element as TreeViewItem).RepeatedIndex = args.Index;
            }
        }

        private void OnElementIndexChanged(ItemsRepeater sender, ItemsRepeaterElementIndexChangedEventArgs args)
        {
            if (args.Element is TreeViewItem)
            {
                (args.Element as TreeViewItem).RepeatedIndex = args.NewIndex;
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            var indices = selectionModel.SelectedIndices;
            var items = selectionModel.SelectedItems;
            Debug.Assert(indices.Count == items.Count);
            for (int i = 0; i < items.Count; i++)
            {
                Debug.WriteLine(indices[i] + ":" + items[i]);
            }

            foreach (var index in selectionModel.SelectedIndices)
            {
                Debug.WriteLine(index);
            }

            foreach (var obj in selectionModel.SelectedItems)
            {
                Debug.WriteLine(obj);
            }
        }
    }
}