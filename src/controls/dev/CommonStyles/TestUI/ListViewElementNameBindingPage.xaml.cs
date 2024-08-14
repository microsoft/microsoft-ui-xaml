// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;

namespace MUXControlsTestApp
{
    public sealed partial class ListViewElementNameBindingPage : TestPage
    {
        public List<MUXControlsTestApp.Samples.MyData> Numbers = new List<MUXControlsTestApp.Samples.MyData>();

        public ListViewElementNameBindingPage()
        {
            for (int i = 0; i < 3; i++)
            {
                Numbers.Add(new MUXControlsTestApp.Samples.MyData(i));
            }

            this.InitializeComponent();
        }

        private void ChkSample_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (sender == chkSample0)
            {
                listView0.Visibility = (bool)chkSample0.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample1)
            {
                listView1.Visibility = (bool)chkSample1.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample2)
            {
                listView2.Visibility = (bool)chkSample2.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample3)
            {
                listView3.Visibility = (bool)chkSample3.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample4)
            {
                listView4.Visibility = (bool)chkSample4.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
        }

        private void ListView_ItemClick(object sender, ItemClickEventArgs e)
        {
            Popup popup = ((sender as ItemsControl).ItemsPanelRoot.Children[(((MUXControlsTestApp.Samples.MyData)e.ClickedItem).number)] as ListViewItem).ContentTemplateRoot as Popup;

            if (popup != null)
            {
                popup.IsOpen = true;
            }
        }
    }
}
