// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using SelectionModelSampleApp.Common;

namespace SelectionModelSampleApp.Pages
{
    public sealed partial class BindingPage : Page, Common.IScenarioPage
    {
        public BindingPage()
        {
            this.InitializeComponent();

            m_items = SampleData.CreateFlat(12);

            SharedSelectionModel.Source = m_items;
            SharedSelectionModel.SingleSelect = true;
            SharedSelectionModel.SelectionChanged += (s, e) => RefreshVisuals();
            SharedSelectionModel.PropertyChanged += OnModelPropertyChanged;

            Repeater.ItemsSource = m_items;
            RefreshVisuals();
        }

        public void ApplyScenario(string scenario)
        {
            // SelectedItem is surfaced through ICustomPropertyProvider + INotifyPropertyChanged,
            // so the {Binding} in the page updates without SelectedItem being a dependency property.
            SharedSelectionModel.Select(3);
            RefreshVisuals();
        }

        private void OnModelPropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs args)
        {
            m_log.Insert(0, $"PropertyChanged: {args.PropertyName}");
            if (m_log.Count > 10)
            {
                m_log.RemoveAt(m_log.Count - 1);
            }

            LogText.Text = string.Join(Environment.NewLine, m_log);
        }

        private void OnElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            args.Element.Tapped -= OnItemTapped;
            args.Element.Tapped += OnItemTapped;
        }

        private void OnItemTapped(object sender, TappedRoutedEventArgs e)
        {
            int index = Repeater.GetElementIndex(sender as UIElement);
            if (index >= 0)
            {
                SharedSelectionModel.Select(index);
            }
        }

        private void OnSingleSelectToggled(object sender, RoutedEventArgs e)
        {
            if (m_items == null)
            {
                // Toggled fires while the XAML is still being parsed, before the page is ready.
                return;
            }

            SharedSelectionModel.SingleSelect = SingleSelectSwitch.IsOn;
            RefreshVisuals();
        }

        private void RefreshVisuals()
        {
            if (m_items == null)
            {
                // Toggled can fire while the XAML is still being parsed.
                return;
            }

            for (int i = 0; i < m_items.Count; i++)
            {
                m_items[i].IsSelected = SharedSelectionModel.IsSelected(i);
            }

            StateText.Text = SelectionState.Describe(SharedSelectionModel);
        }

        private readonly ObservableCollection<Item> m_items;
        private readonly List<string> m_log = new List<string>();
    }
}
