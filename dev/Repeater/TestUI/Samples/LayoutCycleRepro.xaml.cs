// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Windows.UI.Xaml.Controls;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class LayoutCycleRepro : Page
    {
        public LayoutCycleRepro()
        {
            this.InitializeComponent();
            goBackButton.Click += delegate { Frame.GoBack(); };

            this.List.ItemsSource = GetItems();
        }

        private List<LCItem> GetItems()
        {
            var items = new List<LCItem>();

            for (int i = 0; i < 100; i++)
            {
                var item = new LCItem() { Text = "Item" + i };

                if (i % 2 == 0)
                {
                    var subItems = new ObservableCollection<string>();
                    for (int j = 0; j < i / 3; j++)
                    {
                        subItems.Add("SubItem" + j);
                    }

                    item.Items = subItems;
                }
                else if (i % 3 == 0)
                {
                    var subItems = new ObservableCollection<string>();
                    for (int j = 0; j < i / 5; j++)
                    {
                        subItems.Add("SubItem" + j);
                    }

                    item.Items = subItems;
                }
                else if (i % 5 == 0)
                {
                    var subItems = new ObservableCollection<string>();
                    for (int j = 0; j < i / 7; j++)
                    {
                        subItems.Add("SubItem" + j);
                    }

                    item.Items = subItems;
                }

                items.Add(item);
            }

            return items;
        }
    }

    public class LCItem
    {
        public string Text { get; set; }

        public ObservableCollection<string> Items { get; set; }

        public bool HasItems => Items != null;
    }
}
