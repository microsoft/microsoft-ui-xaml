// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;

namespace MUXControlsTestApp.Samples
{
    public class StoreMockData
    {
        public static ObservableCollection<Category> Create(int numCategories, int numItemsPerCategory)
        {
            var data = new ObservableCollection<Category>();
            for (int i = 0; i < numCategories; i++)
            {
                var category = CreateCategory(numItemsPerCategory, "Category " + i.ToString());
                data.Add(category);
            }

            return data;
        }

        public static Category CreateCategory(int numItems, string name)
        {
            var category = new Category() { Name = name };
            for (int i = 0; i < numItems; i++)
            {
                category.Add(new Item() { Name = "Item " + i.ToString() });
            }

            return category;
        }
    }

    public class Category : ObservableCollection<Item>
    {
        public string Name { get; set; }
    }

    public class Item
    {
        public string Name { get; set; }
    }
}
