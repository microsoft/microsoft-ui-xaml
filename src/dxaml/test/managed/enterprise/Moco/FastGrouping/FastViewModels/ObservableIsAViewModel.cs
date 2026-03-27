// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;

using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.FastGrouping
{
    public class ObservableIsAViewModel : GroupedDataModel
    {
        public ObservableIsAViewModel(int numberOfGroups, int numberOfItemsPerGroup, int emptyGroupsClusterSize = 0)
        {
            groupmap = new Dictionary<string, ObservableGroup>();
            Groups = new ObservableCollection<ObservableGroup>();

            for (int i = 0; i < numberOfGroups; i++)
            {
                var group = GroupedDataGenerator.CreateGroup("G:" + i, numberOfItemsPerGroup);
                Groups.Add(group);
                groupmap.Add(group.GroupName, group);
            }


            // add culstersize number of empty group for each group present in the datasource//
            int count = Groups.Count * (emptyGroupsClusterSize + 1);
            for (int i = 0; i < count; i++)
            {
                ObservableGroup currentGroup = Groups[i];

                for (int index = 0; index < emptyGroupsClusterSize; index++)
                {
                    // empty group //
                    ObservableGroup emptyGroupToAdd = new ObservableGroup();
                    emptyGroupToAdd.GroupName = currentGroup.GroupName + "E" + index;
                    groupmap.Add(emptyGroupToAdd.GroupName, emptyGroupToAdd);
                    Groups.Insert(i + index, emptyGroupToAdd);
                }
                i += emptyGroupsClusterSize;
            }
        }

        public override void ReplaceGroup(string groupName)
        {
            if (groupmap != null)
            {
                if (groupmap.ContainsKey(groupName))
                {
                    // remove group
                    var selectedGroup = groupmap[groupName];
                    if (Groups.Contains(selectedGroup))
                    {
                        //add new group
                        ObservableGroup g = new ObservableGroup();
                        g.GroupName = groupName;
                        g.Add(new ObservableItem() { ItemContent0 = g.GroupName, ItemContent1 = "Item " + g.Count });

                        int index = Groups.IndexOf(selectedGroup);
                        Groups[index] = g;

                        groupmap.Remove(groupName);
                        groupmap.Add(groupName, g);
                    }
                }
            }
        }

        public override void ResetGroup(string groupName)
        {
            if (groupmap != null)
            {
                if (groupmap.ContainsKey(groupName))
                {
                    var selectedGroup = groupmap[groupName];
                    if (Groups.Contains(selectedGroup))
                    {
                        List<object> newContents = new List<object>();
                        newContents.Add(new ObservableItem() { ItemContent0 = groupName, ItemContent1 = "Replaced Item 1" });
                        newContents.Add(new ObservableItem() { ItemContent0 = groupName, ItemContent1 = "Replaced Item 2" });
                        newContents.Add(new ObservableItem() { ItemContent0 = groupName, ItemContent1 = "Replaced Item 3" });
                        newContents.Add(new ObservableItem() { ItemContent0 = groupName, ItemContent1 = "Replaced Item 4" });
                        newContents.Add(new ObservableItem() { ItemContent0 = groupName, ItemContent1 = "Replaced Item 5" });

                        int index = Groups.IndexOf(selectedGroup);
                        Groups[index].ResetGroup(newContents);
                    }
                }
            }
        }


        public override void AddGroupItem(string groupName, object itemToAdd)
        {
            ObservableItem item = itemToAdd as ObservableItem;
            if (item == null)
            {
                throw new InvalidOperationException("Cannot add this item to the group");
            }

            if (groupmap != null)
            {
                if (groupmap.ContainsKey(groupName))
                {
                    var selectedGroup = groupmap[groupName];
                    selectedGroup.Add(item);
                }
                else
                {
                    var newGroup = new ObservableGroup() { GroupName = groupName };
                    newGroup.Add(item);
                    groupmap.Add(groupName, newGroup);
                    Groups.Add(newGroup);
                }
            }
        }

        public override void DeleteGroup(string groupName)
        {
            if (groupmap != null)
            {
                if (groupmap.ContainsKey(groupName))
                {
                    var selectedGroup = groupmap[groupName];
                    if (Groups.Contains(selectedGroup))
                    {
                        Groups.Remove(selectedGroup);
                    }
                    groupmap.Remove(groupName);
                }
            }
        }

        public override void RemoveSelectedGroupItems(IList<object> items)
        {
            List<ObservableItem> itemsToRemove = new List<ObservableItem>();
            foreach (ObservableItem item in items)
            {
                itemsToRemove.Add(item);
            }
            foreach (ObservableItem item in itemsToRemove)
            {
                var group = groupmap[item.ItemContent0];
                if (group.Contains(item))
                {
                    group.Remove(item);
                }
                else
                {
                    Debug.WriteLine("Group does not contain this item");
                }
            }
        }

        public override void ClearGroup(string groupName)
        {
            if (groupmap != null)
            {
                if (groupmap.ContainsKey(groupName))
                {
                    groupmap[groupName].Clear();
                }
            }
        }

        public override object GetGroupedDataSource()
        {
            return this.Groups;
        }

        public override void ReplaceFirstGroupItem(string groupName, object itemToReplaceWith)
        {
            if (groupmap != null)
            {
                if (groupmap.ContainsKey(groupName))
                {
                    ObservableItem item = itemToReplaceWith as ObservableItem;
                    groupmap[groupName][0] = item;
                }
            }
        }

        public override object GetGroup(string groupName)
        {
            if (groupmap != null)
            {
                if (groupmap.ContainsKey(groupName))
                {
                    return groupmap[groupName];
                }
            }

            return null;
        }

        public override void AddEmptyGroup(string groupName)
        {
            var newGroup = new ObservableGroup() { GroupName = groupName };
            groupmap.Add(groupName, newGroup);
            Groups.Add(newGroup);
        }

        #region private members

        private ObservableCollection<ObservableGroup> Groups { get; set; }
        private Dictionary<string, ObservableGroup> groupmap;

        #endregion
    }
}
