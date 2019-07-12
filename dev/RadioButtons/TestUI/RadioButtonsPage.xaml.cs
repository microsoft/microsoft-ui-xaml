// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;
using System.Collections.ObjectModel;

#if !BUILD_WINDOWS
using RadioButtons = Microsoft.UI.Xaml.Controls.RadioButtons;
#endif

namespace MUXControlsTestApp
{
    [AddToTestInventory(Name = "RadioButtons", Icon = "RadioButton.png")]
    public sealed partial class RadioButtonsPage : TestPage
    {
        ObservableCollection<string> m_itemCollection;

        public RadioButtonsPage()
        {
            this.InitializeComponent();

            m_itemCollection = new ObservableCollection<string>();

            m_itemCollection.Add("Left");
            m_itemCollection.Add("Middle");
            m_itemCollection.Add("Right");

            ItemSourceRadioButtons.ItemsSource = m_itemCollection;
            ItemSourceRadioButtons.SelectedIndex = 0;

            RadioButtons.SelectedItem = "Orange";
        }

        private void RadioButtonsLoaded(object sender, RoutedEventArgs e)
        {
            ((ListViewItem)RadioButtons.ContainerFromIndex(2)).IsEnabled = false;
        }

        private void SelectItemBlue_Click(object sender, RoutedEventArgs e)
        {
            var item = RadioButtons.Items[4];
            RadioButtons.SelectedItem = "Blue";
        }

        private void SelectIndex5_Click(object sender, RoutedEventArgs e)
        {
            RadioButtons.SelectedIndex = 5;
        }

        private void OneColumn_Click(object sender, RoutedEventArgs e)
        {
            RadioButtons.MaximumColumns = 1;
        }

        private void RadioButtonSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            SelectedChangedTextBlock.Text = e.AddedItems[0].ToString();
            SelectedIndexTextBlock.Text = RadioButtons.SelectedIndex.ToString();
            if (RadioButtons.SelectedItem != null)
            {
                SelectedItemTextBlock.Text = RadioButtons.SelectedItem.ToString();
            }
        }
    }
}
