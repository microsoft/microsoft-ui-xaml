// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class ItemTemplateDemo : Page
    {
        public List<int> Data { get; set; }
        public List<MyData> Numbers = new List<MyData>();

        public ItemTemplateDemo()
        {
            Data = Enumerable.Range(0, 1000).ToList();
            
            for(int i=0;i<10;i++)
            {
                Numbers.Add(new MyData(i));
            }

            this.InitializeComponent();
        }

        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = (((int)args.DataContext) % 2 == 0) ? "even" : "odd";
        }

        private void ItemsRepeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            Popup popup = args.Element as Popup;

            if (popup != null)
            {
                popup.IsOpen = true;
            }
        }

        private void ChkSample_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (sender == chkSample0)
            {
                itemsRepeaterScrollHost0.Visibility = (bool)chkSample0.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample1)
            {
                itemsRepeaterScrollHost1.Visibility = (bool)chkSample1.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample2)
            {
                itemsRepeaterScrollHost2.Visibility = (bool)chkSample2.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample3)
            {
                itemsRepeaterScrollHost3.Visibility = (bool)chkSample3.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample4)
            {
                itemsRepeaterScrollHost4.Visibility = (bool)chkSample4.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample5)
            {
                itemsRepeater5.Visibility = (bool)chkSample5.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample6)
            {
                itemsRepeater6.Visibility = (bool)chkSample6.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample7)
            {
                itemsRepeater7.Visibility = (bool)chkSample7.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample8)
            {
                myItemsRepeater8.Visibility = (bool)chkSample8.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample9)
            {
                itemsRepeater9.Visibility = (bool)chkSample9.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample10)
            {
                itemsRepeater10.Visibility = (bool)chkSample10.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample11)
            {
                listView11.Visibility = (bool)chkSample11.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
            else if (sender == chkSample12)
            {
                itemsRepeater12.Visibility = (bool)chkSample12.IsChecked ? Visibility.Visible : Visibility.Collapsed;
            }
        }
    }

    public class MyData
    {
        public int number;

        public MyData(int number)
        {
            this.number = number;
        }
    }

    public partial class MySelector : DataTemplateSelector
    {
        public DataTemplate TemplateOdd { get; set; }

        public DataTemplate TemplateEven { get; set; }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            return (((int)item) % 2 == 0) ? TemplateEven : TemplateOdd;
        }
    }

    public partial class MyItemsRepeater : ItemsRepeater
    {
        public MyItemsRepeater()
        {
        }
    }
}
