// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using SelectionModelSampleApp.Common;

namespace SelectionModelSampleApp.Pages
{
    public sealed partial class FlatSelectionPage : Page, Common.IScenarioPage
    {
        public FlatSelectionPage()
        {
            this.InitializeComponent();

            m_items = SampleData.CreateFlat(20);

            // The model does not own the data - you point it at the same source the repeater uses.
            m_selectionModel = new SelectionModel();
            m_selectionModel.Source = m_items;
            m_selectionModel.SelectionChanged += OnSelectionChanged;

            Repeater.ItemsSource = m_items;
            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            m_selectionModel.ClearSelection();

            switch (scenario)
            {
                case "single":
                    // Single select: the second Select replaces the first.
                    m_selectionModel.SingleSelect = true;
                    SingleSelectSwitch.IsOn = true;
                    m_selectionModel.Select(3);
                    m_selectionModel.Select(7);
                    break;

                case "multi":
                default:
                    m_selectionModel.Select(3);
                    m_selectionModel.Select(4);
                    m_selectionModel.Select(5);
                    break;
            }

            RefreshVisuals();
        }

        private void OnSelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
        {
            m_eventCount++;
            // SelectionModelSelectionChangedEventArgs carries no data, so the handler has to read
            // the current state back off the model itself.
            EventLogText.Text = $"SelectionChanged #{m_eventCount}\nSelectedIndices = {SelectionState.Indices(sender)}";
            RefreshVisuals();
        }

        private void OnElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            args.Element.Tapped -= OnItemTapped;
            args.Element.Tapped += OnItemTapped;
        }

        private void OnItemTapped(object sender, TappedRoutedEventArgs e)
        {
            int index = Repeater.GetElementIndex(sender as UIElement);
            if (index < 0)
            {
                return;
            }

            bool? isSelected = m_selectionModel.IsSelected(index);
            if (isSelected == true)
            {
                m_selectionModel.Deselect(index);
            }
            else
            {
                m_selectionModel.Select(index);
            }
        }

        private int Index => (int)IndexBox.Value;

        private void OnSelect(object sender, RoutedEventArgs e) => m_selectionModel.Select(Index);

        private void OnDeselect(object sender, RoutedEventArgs e) => m_selectionModel.Deselect(Index);

        private void OnIsSelected(object sender, RoutedEventArgs e)
            => ResultText.Text = $"IsSelected({Index}) -> {SelectionState.Format(m_selectionModel.IsSelected(Index))}";

        private void OnSelectAll(object sender, RoutedEventArgs e) => m_selectionModel.SelectAll();

        private void OnSelectAllFlat(object sender, RoutedEventArgs e) => m_selectionModel.SelectAllFlat();

        private void OnClearSelection(object sender, RoutedEventArgs e) => m_selectionModel.ClearSelection();

        private void OnSetSelectedIndex(object sender, RoutedEventArgs e)
            => m_selectionModel.SelectedIndex = IndexPath.CreateFrom(5);

        private void OnClearSource(object sender, RoutedEventArgs e)
        {
            // Setting Source clears the selection and resets the anchor.
            m_selectionModel.Source = null;
            m_selectionModel.Source = m_items;
            ResultText.Text = "Source reassigned - selection and anchor were reset.";
        }

        private void OnSingleSelectToggled(object sender, RoutedEventArgs e)
        {
            m_selectionModel.SingleSelect = SingleSelectSwitch.IsOn;
            RefreshVisuals();
        }

        private void RefreshVisuals()
        {
            for (int i = 0; i < m_items.Count; i++)
            {
                m_items[i].IsSelected = m_selectionModel.IsSelected(i);
            }

            StateText.Text = SelectionState.Describe(m_selectionModel);
        }

        private readonly ObservableCollection<Item> m_items;
        private readonly SelectionModel m_selectionModel;
        private int m_eventCount;
    }
}
