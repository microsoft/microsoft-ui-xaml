// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using SelectionModelSampleApp.Common;

namespace SelectionModelSampleApp.Pages
{
    public sealed partial class RangeSelectionPage : Page, Common.IScenarioPage
    {
        public RangeSelectionPage()
        {
            this.InitializeComponent();

            m_items = SampleData.CreateFlat(20);
            m_selectionModel = new SelectionModel();
            m_selectionModel.Source = m_items;
            m_selectionModel.SelectionChanged += (s, e) => RefreshVisuals();

            Repeater.ItemsSource = m_items;
            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            m_selectionModel.ClearSelection();

            // The anchor is the fixed end of the range - SelectRangeFromAnchor does not move it.
            m_selectionModel.SetAnchorIndex(4);
            m_selectionModel.SelectRangeFromAnchor(9);

            // Assigning Value before the NumberBox template is applied leaves the text blank, so
            // the values are pushed once the control is loaded.
            StartBox.Loaded += (s, e) => StartBox.Value = 4;
            EndBox.Loaded += (s, e) => EndBox.Value = 9;

            ResultText.Text =
                "SetAnchorIndex(4); SelectRangeFromAnchor(9)" + Environment.NewLine +
                $"AnchorIndex is still {SelectionState.Format(m_selectionModel.AnchorIndex)}";

            RefreshVisuals();
        }

        private int Start => (int)StartBox.Value;

        private int End => (int)EndBox.Value;

        private void OnSetAnchorIndex(object sender, RoutedEventArgs e)
        {
            m_selectionModel.SetAnchorIndex(Start);
            ResultText.Text = $"AnchorIndex is now {SelectionState.Format(m_selectionModel.AnchorIndex)}";
            RefreshVisuals();
        }

        private void OnSelectRangeFromAnchor(object sender, RoutedEventArgs e)
            => m_selectionModel.SelectRangeFromAnchor(End);

        private void OnDeselectRangeFromAnchor(object sender, RoutedEventArgs e)
            => m_selectionModel.DeselectRangeFromAnchor(End);

        private void OnSelectRangeFromAnchorTo(object sender, RoutedEventArgs e)
            => m_selectionModel.SelectRangeFromAnchorTo(IndexPath.CreateFrom(End));

        private void OnDeselectRangeFromAnchorTo(object sender, RoutedEventArgs e)
            => m_selectionModel.DeselectRangeFromAnchorTo(IndexPath.CreateFrom(End));

        private void OnSelectRange(object sender, RoutedEventArgs e)
            => m_selectionModel.SelectRange(IndexPath.CreateFrom(Start), IndexPath.CreateFrom(End));

        private void OnDeselectRange(object sender, RoutedEventArgs e)
            => m_selectionModel.DeselectRange(IndexPath.CreateFrom(Start), IndexPath.CreateFrom(End));

        private void OnClearAnchor(object sender, RoutedEventArgs e)
        {
            m_selectionModel.AnchorIndex = null;
            ResultText.Text = "AnchorIndex cleared - range methods now start from index 0.";
            RefreshVisuals();
        }

        private void OnClearSelection(object sender, RoutedEventArgs e) => m_selectionModel.ClearSelection();

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
    }
}
