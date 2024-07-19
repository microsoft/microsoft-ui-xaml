﻿using System;
using System.Collections.Generic;
using System.Text;
using System.Collections.ObjectModel;

namespace MUXControlsTestApp
{
    public class TreeViewViewModelItem
    {
        public string Name { get; set; }

        public ObservableCollection<TreeViewViewModelItem> Items { get; set; }
    }

    public class TreeViewViewModel
    {
        public ObservableCollection<TreeViewViewModelItem> Items { get; set; }

        public TreeViewViewModel()
        {
            Items = new ObservableCollection<TreeViewViewModelItem>();

            for (int i = 0; i < 5; i++)
            {
                TreeViewViewModelItem item = new()
                {
                    Name = $"Item {i + 1}",
                    Items = new ObservableCollection<TreeViewViewModelItem>()
                };

                for (int j = 0; j < 5; j++)
                {
                    item.Items.Add(new TreeViewViewModelItem()
                    {
                        Name = $"Item {i + 1}.{j + 1}"
                    });
                }

                Items.Add(item);
            }
        }
    }
}
