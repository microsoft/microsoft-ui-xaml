// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;

using ItemsRepeaterElementIndexChangedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementIndexChangedEventArgs;
using ItemsRepeaterElementPreparedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using SelectionModel = Microsoft.UI.Xaml.Controls.SelectionModel;

namespace ItemsRepeaterDemos
{
    public sealed partial class SelectionModelFlatDemo : NavPage
    {
        ObservableCollection<string> _data = new ObservableCollection<string>(Enumerable.Range(0, 1000).Select(x => x.ToString()));
        public SelectionModelFlatDemo()
        {
            this.InitializeComponent();
            repeater.ItemTemplate = elementFactory;
            repeater.ItemsSource = _data;
            selectionModel.Source = _data;
            repeater.ElementPrepared += Repeater_ElementPrepared;
            repeater.ElementIndexChanged += Repeater_ElementIndexChanged;
            selectionModel.PropertyChanged += SelectionModel_PropertyChanged;
            selectionModel.SingleSelect = true;
        }

        private void SelectionModel_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            var manager = sender as SelectionModel;
            if (e.PropertyName == "SelectedItem")
            {
                Debug.WriteLine("SelectedItem changed");
                Debug.WriteLine(manager.SelectedItem);
            }
            else if (e.PropertyName == "SelectedIndices")
            {
                Debug.WriteLine("SelectedIndices changed");
                for (int i = 0; i < manager.SelectedIndices.Count; i++)
                {
                    Debug.WriteLine(string.Format("{0}:{1}", manager.SelectedIndices[i], manager.SelectedItems[i].ToString()));
                }
            }
            else if (e.PropertyName == "SelectedItems")
            {
                Debug.WriteLine("SelectedItems changed");
                for (int i = 0; i < manager.SelectedIndices.Count; i++)
                {
                    Debug.WriteLine(string.Format("{0}:{1}", manager.SelectedIndices[i], manager.SelectedItems[i].ToString()));
                }
            }
            else if (e.PropertyName == "SelectedIndex")
            {
                Debug.WriteLine("SelectedIndex changed");
                Debug.WriteLine(manager.SelectedIndex);
            }

            var icpp = (ICustomPropertyProvider)selectionModel;
            var selectedItemProperty = icpp.GetCustomProperty("SelectedItem");
            var canRead = selectedItemProperty.CanRead;
            var selectedItem =  selectedItemProperty.GetValue(selectionModel);
        }

        private void Repeater_ElementIndexChanged(ItemsRepeater sender, ItemsRepeaterElementIndexChangedEventArgs args)
        {
            (args.Element as RepeaterItem).RepeatedIndex = args.NewIndex;
        }

        private void Repeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            (args.Element as RepeaterItem).RepeatedIndex = args.Index;
        }

        private void OnMultipleSelectionClicked(object sender, RoutedEventArgs e)
        {
            selectionModel.SingleSelect = multipleSelection.IsChecked.Value ? false : true;
        }
    }
}
