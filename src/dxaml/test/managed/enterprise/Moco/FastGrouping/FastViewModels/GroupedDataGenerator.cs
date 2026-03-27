// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.FastGrouping
{
    internal class GroupedDataGenerator
    {
        public static ObservableGroup CreateGroup(string groupName, int itemsCount)
        {
            var group = new ObservableGroup() { GroupName = groupName };
            for (int i = 0; i < itemsCount; i++)
            {
                group.Add(new ObservableItem() { ItemContent0 = groupName, ItemContent1 = "I:" + i });
            }

            return group;
        }
    }
}
