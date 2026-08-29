// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using System.Collections.ObjectModel;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class NonVirtualStackLayoutSamplePage : Page
    {
        private ObservableCollection<int> _numbers = new ObservableCollection<int>();
        private ObservableCollection<UIElement> _uiElements = new ObservableCollection<UIElement>();
        private SolidColorBrush _solidColorBrushRed = new SolidColorBrush(Colors.Red);
        private SolidColorBrush _solidColorBrushGreen = new SolidColorBrush(Colors.Green);

        public NonVirtualStackLayoutSamplePage()
        {
            this.InitializeComponent();

            for (int i = 0; i < 10; i++)
            {
                _numbers.Add(i);
                _uiElements.Add(new TextBlock() { Text = i.ToString(), Foreground = _solidColorBrushRed });
            }

            itemsRepeater1.ItemsSource = _numbers;
            itemsRepeater2.ItemsSource = _uiElements;
        }

        private void BtnClear1_Click(object sender, RoutedEventArgs e)
        {
            _numbers.Clear();
        }

        private void BtnReplace1_Click(object sender, RoutedEventArgs e)
        {
            int childIndex = int.Parse(txtChildIndex1.Text);
            _numbers[childIndex] = _numbers.Count;
        }

        private void BtnInsert1_Click(object sender, RoutedEventArgs e)
        {
            int childIndex = int.Parse(txtChildIndex1.Text);
            _numbers.Insert(childIndex, _numbers.Count);
        }

        private void BtnRemoveAt1_Click(object sender, RoutedEventArgs e)
        {
            int childIndex = int.Parse(txtChildIndex1.Text);
            _numbers.RemoveAt(childIndex);
        }

        private void BtnClear2_Click(object sender, RoutedEventArgs e)
        {
            _uiElements.Clear();
        }

        private void BtnReplace2_Click(object sender, RoutedEventArgs e)
        {
            int childIndex = int.Parse(txtChildIndex2.Text);
            _uiElements[childIndex] = new TextBlock() { Text = _uiElements.Count.ToString(), Foreground = _solidColorBrushGreen };
        }

        private void BtnInsert2_Click(object sender, RoutedEventArgs e)
        {
            int childIndex = int.Parse(txtChildIndex2.Text);
            _uiElements.Insert(childIndex, new TextBlock() { Text = _uiElements.Count.ToString(), Foreground = _solidColorBrushGreen });
        }

        private void BtnRemoveAt2_Click(object sender, RoutedEventArgs e)
        {
            int childIndex = int.Parse(txtChildIndex2.Text);
            _uiElements.RemoveAt(childIndex);
        }
    }
}
