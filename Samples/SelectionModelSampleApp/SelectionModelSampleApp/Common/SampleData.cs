// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace SelectionModelSampleApp.Common
{
    /// <summary>
    /// A leaf data item. Exposes IsSelected / IsPartiallySelected purely so the sample can
    /// visualize what SelectionModel reports; SelectionModel itself stores no state on the item.
    /// </summary>
    public class Item : INotifyPropertyChanged
    {
        public Item(string label)
        {
            Label = label;
        }

        public string Label { get; }

        public bool? IsSelected
        {
            get => m_isSelected;
            set
            {
                if (m_isSelected != value)
                {
                    m_isSelected = value;
                    RaisePropertyChanged();
                }
            }
        }

        public override string ToString() => Label;

        public event PropertyChangedEventHandler PropertyChanged;

        private void RaisePropertyChanged([CallerMemberName] string name = null)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));

        private bool? m_isSelected = false;
    }

    /// <summary>
    /// A group of items. Deriving from ObservableCollection means SelectionModel can resolve the
    /// children automatically, without a ChildrenRequested handler.
    /// </summary>
    public class Group : ObservableCollection<Item>, INotifyPropertyChanged
    {
        public Group(string name)
        {
            Name = name;
        }

        public string Name { get; }

        public bool? IsSelected
        {
            get => m_isSelected;
            set
            {
                if (m_isSelected != value)
                {
                    m_isSelected = value;
                    OnPropertyChanged(new PropertyChangedEventArgs(nameof(IsSelected)));
                }
            }
        }

        public override string ToString() => Name;

        private bool? m_isSelected = false;
    }

    public static class SampleData
    {
        public static ObservableCollection<Item> CreateFlat(int count)
        {
            var items = new ObservableCollection<Item>();
            for (int i = 0; i < count; i++)
            {
                items.Add(new Item($"Item {i}"));
            }

            return items;
        }

        public static ObservableCollection<Group> CreateGrouped(int groupCount, int itemsPerGroup)
        {
            var groups = new ObservableCollection<Group>();
            for (int g = 0; g < groupCount; g++)
            {
                var group = new Group($"Group {g}");
                for (int i = 0; i < itemsPerGroup; i++)
                {
                    group.Add(new Item($"{g}.{i}"));
                }

                groups.Add(group);
            }

            return groups;
        }
    }
}
