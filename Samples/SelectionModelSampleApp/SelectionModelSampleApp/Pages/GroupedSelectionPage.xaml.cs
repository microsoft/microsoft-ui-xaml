// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using SelectionModelSampleApp.Common;

namespace SelectionModelSampleApp.Pages
{
    public sealed partial class GroupedSelectionPage : Page, Common.IScenarioPage
    {
        public GroupedSelectionPage()
        {
            this.InitializeComponent();

            m_groups = SampleData.CreateGrouped(3, 4);

            m_selectionModel = new SelectionModel();
            // Group derives from ObservableCollection, so SelectionModel resolves the children
            // automatically without a ChildrenRequested handler.
            m_selectionModel.Source = m_groups;
            m_selectionModel.SelectionChanged += (s, e) => RefreshVisuals();

            GroupsControl.ItemsSource = m_groups;
            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            m_selectionModel.ClearSelection();

            switch (scenario)
            {
                case "full":
                    // Selecting every leaf of group 1 makes the group itself report true.
                    m_selectionModel.SelectRange(IndexPath.CreateFrom(1, 0), IndexPath.CreateFrom(1, 3));
                    break;

                case "partial":
                default:
                    // One leaf of group 1 - the group reports null (partially selected).
                    m_selectionModel.Select(1, 2);
                    break;
            }

            RefreshVisuals();
        }

        private int GroupIndex => (int)GroupBox.Value;
        private int ItemIndex => (int)ItemBox.Value;

        private IndexPath ItemPath => IndexPath.CreateFrom(GroupIndex, ItemIndex);

        private void OnSelect(object sender, RoutedEventArgs e) => m_selectionModel.Select(GroupIndex, ItemIndex);

        private void OnDeselect(object sender, RoutedEventArgs e) => m_selectionModel.Deselect(GroupIndex, ItemIndex);

        private void OnIsSelected(object sender, RoutedEventArgs e)
            => ResultText.Text = $"IsSelected({GroupIndex}, {ItemIndex}) -> {SelectionState.Format(m_selectionModel.IsSelected(GroupIndex, ItemIndex))}";

        private void OnSelectAt(object sender, RoutedEventArgs e) => m_selectionModel.SelectAt(ItemPath);

        private void OnDeselectAt(object sender, RoutedEventArgs e) => m_selectionModel.DeselectAt(ItemPath);

        private void OnIsSelectedAtGroup(object sender, RoutedEventArgs e)
        {
            var groupPath = IndexPath.CreateFrom(GroupIndex);
            ResultText.Text = $"IsSelectedAt({groupPath}) -> {SelectionState.Format(m_selectionModel.IsSelectedAt(groupPath))}";
        }

        private void OnSelectWholeGroup(object sender, RoutedEventArgs e)
        {
            int lastItem = m_groups[GroupIndex].Count - 1;
            m_selectionModel.SelectRange(
                IndexPath.CreateFrom(GroupIndex, 0),
                IndexPath.CreateFrom(GroupIndex, lastItem));
        }

        private void OnSelectAll(object sender, RoutedEventArgs e) => m_selectionModel.SelectAll();

        private void OnSelectAllFlat(object sender, RoutedEventArgs e)
        {
            // SelectAllFlat assumes a flat source, so on grouped data it selects the group nodes
            // themselves rather than the leaves.
            m_selectionModel.SelectAllFlat();
            ResultText.Text = "SelectAllFlat() treats the source as flat - it selects the groups, not the leaves.";
        }

        private void OnClearSelection(object sender, RoutedEventArgs e) => m_selectionModel.ClearSelection();

        private void RefreshVisuals()
        {
            for (int g = 0; g < m_groups.Count; g++)
            {
                var group = m_groups[g];
                group.IsSelected = m_selectionModel.IsSelectedAt(IndexPath.CreateFrom(g));

                for (int i = 0; i < group.Count; i++)
                {
                    group[i].IsSelected = m_selectionModel.IsSelected(g, i);
                }
            }

            StateText.Text = SelectionState.Describe(m_selectionModel);
        }

        private readonly ObservableCollection<Group> m_groups;
        private readonly SelectionModel m_selectionModel;
    }
}
