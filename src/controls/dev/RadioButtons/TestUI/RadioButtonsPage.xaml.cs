﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows.Input;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Private.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI;

namespace MUXControlsTestApp
{
    public sealed partial class RadioButtonsPage : TestPage
    {
        ObservableCollection<string> m_stringItemCollection;
        ObservableCollection<RadioButton> m_radioButtonItemCollection;
        bool m_loaded = false;

        public RadioButtonsPage()
        {
            this.InitializeComponent();
            m_stringItemCollection = new ObservableCollection<string>();
            m_radioButtonItemCollection = new ObservableCollection<RadioButton>();
            this.Loaded += RadioButtonsPage_Loaded;
            this.SecondTestRadioButton.SelectedItem = this.TheRadioButton;
        }

        private void RadioButtonsPage_Loaded(object sender, RoutedEventArgs e)
        {
            m_loaded = true;
            RadioButtonsTestHooks.SetTestHooksEnabled(TestRadioButtons, true);
            RadioButtonsTestHooks.LayoutChanged += RadioButtonsTestHooks_LayoutChanged;

            SetMaxColumnsButton_Click(null, null);
            SetNumberOfItemsButton_Click(null, null);
            UpdateRadioButtonsSource();
            UpdateDisplayRadioButton();
        }

        private void RadioButtonsTestHooks_LayoutChanged(RadioButtons sender, object args)
        {
            LayoutNumberOfRowsTextBlock.Text = RadioButtonsTestHooks.GetRows(sender).ToString();
            LayoutNumberOfColumnsTextBlock.Text = RadioButtonsTestHooks.GetColumns(sender).ToString();
            LayoutNumberOfLargerColumnsTextBlock.Text = RadioButtonsTestHooks.GetLargerColumns(sender).ToString();
        }

        private void TestRadioButtons_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var index = TestRadioButtons.SelectedIndex;
            SelectedIndexTextBlock.Text = index.ToString();
            if (TestRadioButtons.SelectedItem != null)
            {
                SelectedItemTextBlock.Text = TestRadioButtons.SelectedItem.ToString();
            }
            else
            {
                SelectedItemTextBlock.Text = "null";
            }

            if (index > 0)
            {
                var radioButton = TestRadioButtons.ContainerFromIndex(index);
                if (radioButton != null)
                {
                    SelectedPositionInSetTextBlock.Text = radioButton.GetValue(AutomationProperties.PositionInSetProperty).ToString();
                    SelectedSizeOfSetTextBlock.Text = radioButton.GetValue(AutomationProperties.SizeOfSetProperty).ToString();
                }
            }
            else
            {
                SelectedPositionInSetTextBlock.Text = "-1";
                SelectedSizeOfSetTextBlock.Text = "-1";
            }
        }

        private void RootGotFocus(object sender, RoutedEventArgs e)
        {
            FocusedItemTextBlock.Text = e.OriginalSource.ToString();
        }

        private void TestRadioButtons_GotFocus(object sender, RoutedEventArgs e)
        {
            var stackPanel = VisualTreeHelper.GetChild(TestRadioButtons, 0);
            var repeater = (ItemsRepeater)VisualTreeHelper.GetChild(stackPanel, 1);
            FocusedIndexTextBlock.Text = repeater.GetElementIndex((UIElement)e.OriginalSource).ToString();
            RadioButtonsHasFocusCheckBox.IsChecked = true;
        }

        private void TestRadioButtons_LostFocus(object sender, RoutedEventArgs e)
        {
            FocusedIndexTextBlock.Text = "-1";
            RadioButtonsHasFocusCheckBox.IsChecked = false;
        }

        private void SetMaxColumnsButton_Click(object sender, RoutedEventArgs e)
        {
            if (UInt32.TryParse(MaxColumnsTextBlock.Text, out uint value))
            {
                TestRadioButtons.MaxColumns = (int)value;
                MaxColumnsTextBlock.BorderBrush = new SolidColorBrush(Colors.Black);
            }
            else
            {
                MaxColumnsTextBlock.BorderBrush = new SolidColorBrush(Colors.Red);
            }
        }

        private void SetNumberOfItemsButton_Click(object sender, RoutedEventArgs e)
        {
            if (UInt32.TryParse(NumberOfItemsTextBlock.Text, out uint value))
            {
                m_stringItemCollection.Clear();
                m_radioButtonItemCollection.Clear();
                for (int i = 0; i<value; i++)
                {
                    m_stringItemCollection.Add(i.ToString());
                    var radioButton = new RadioButton();
                    radioButton.Content = i.ToString() + "Radio Button";
                    radioButton.SetValue(AutomationProperties.NameProperty, "Radio Button " + i);
                    radioButton.Name = "Radio Button " + i;
                    m_radioButtonItemCollection.Add(radioButton);
                }

                if(SourceComboBox.SelectedItem == ItemsComboBoxItem)
                {
                    UpdateRadioButtonsSource();
                }
                NumberOfItemsTextBlock.BorderBrush = new SolidColorBrush(Colors.Black);
            }
            else
            {
                NumberOfItemsTextBlock.BorderBrush = new SolidColorBrush(Colors.Red);
            }
        }

        private void SourceComboBoxSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateRadioButtonsSource();
        }

        private void ItemTypeComboBoxSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateRadioButtonsSource();
        }

        private void SelectByIndexButton_Click(object sender, RoutedEventArgs e)
        {
            var index = getIndexToSelect();
            if (index != -2)
            {
                TestRadioButtons.SelectedIndex = index;
            }
        }

        private void SelectByItemButton_Click(object sender, RoutedEventArgs e)
        {
            var index = getIndexToSelect();
            if (index >= 0)
            {
                if (ItemTypeComboBox.SelectedItem == StringsComboBoxItem)
                {
                    if (SourceComboBox.SelectedItem == ItemsSourceComboBoxItem)
                    {
                        TestRadioButtons.SelectedItem = m_stringItemCollection[index];
                    }
                    else if (SourceComboBox.SelectedItem == ItemsComboBoxItem)
                    {
                        TestRadioButtons.SelectedItem = TestRadioButtons.Items[index];
                    }
                }
                else if (ItemTypeComboBox.SelectedItem == RadioButtonElementsComboBoxItem)
                {
                    TestRadioButtons.SelectedItem = m_radioButtonItemCollection[index];
                }
            }
            else
            {
                TestRadioButtons.SelectedItem = null;
            }

        }

        private int getIndexToSelect()
        {
            if (Int32.TryParse(IndexToSelectTextBlock.Text, out int value))
            {
                if (value >= m_radioButtonItemCollection.Count)
                {
                    IndexToSelectTextBlock.Foreground = new SolidColorBrush(Colors.DarkRed);
                    return -2;
                }
                if (value < -1)
                {
                    IndexToSelectTextBlock.Foreground = new SolidColorBrush(Colors.DarkRed);
                    return -2;
                }
                IndexToSelectTextBlock.Foreground = new SolidColorBrush(Colors.Black);
                return value;
            }
            IndexToSelectTextBlock.Foreground = new SolidColorBrush(Colors.DarkRed);
            return -2;
        }

        private void UpdateDisplayRadioButtonButton_Click(object sender, RoutedEventArgs e)
        {
            UpdateDisplayRadioButton();
        }
        private void InsertDisplayRadioButtonButton_Click(object sender, RoutedEventArgs e)
        {
            if(UpdateDisplayRadioButton())
            {
                var radioButton = new RadioButton();
                radioButton.Content = DisplayRadioButton.Content;
                radioButton.IsEnabled = !(bool)CustomDisabledCheckBox.IsChecked;
                radioButton.IsChecked = (bool)CustomCheckedCheckBox.IsChecked;
                m_radioButtonItemCollection.Insert(Int32.Parse(CustomIndexTextBox.Text), radioButton);

                if (SourceComboBox.SelectedItem == ItemsComboBoxItem)
                {
                    UpdateRadioButtonsSource();
                }
            }
            TestRadioButtons_SelectionChanged(null, null);
        }

        private void FocusSelectedItemButton_Clicked(object sender, RoutedEventArgs e)
        {
            var stackPanel = VisualTreeHelper.GetChild(TestRadioButtons, 0);
            var repeater = (ItemsRepeater)VisualTreeHelper.GetChild(stackPanel, 1);
            ((Control)repeater.TryGetElement(TestRadioButtons.SelectedIndex)).Focus(FocusState.Keyboard);
        }

        private void SetBorderWidthButton_Click(object sender, RoutedEventArgs e)
        {
            if(this.BorderWidthTextBox.Text == "inf")
            {
                this.TestRadioButtonsBorder.Width = float.NaN;
                this.BorderWidthTextBox.BorderBrush = new SolidColorBrush(Colors.Black);
            }
            else
            {
                if(float.TryParse(this.BorderWidthTextBox.Text, out float value))
                {
                    this.TestRadioButtonsBorder.Width = value;
                    this.BorderWidthTextBox.BorderBrush = new SolidColorBrush(Colors.Black);
                }
                else
                {
                    this.BorderWidthTextBox.BorderBrush = new SolidColorBrush(Colors.DarkRed);
                }
            }
        }

        private void Select5ThenChangeSourceButton_Clicked(object sender, RoutedEventArgs e)
        {
            TestRadioButtons.SelectedIndex = 5;
            ItemTypeComboBox.SelectedIndex = (ItemTypeComboBox.SelectedIndex + 1) % 2;
        }

        private void ClearRadioButtonsEventsButton_Click(object sender, RoutedEventArgs e)
        {
        }

        private void UpdateRadioButtonsSource()
        {
            if (m_loaded)
            {
                var source = SourceComboBox.SelectedItem;
                if (source == ItemsComboBoxItem)
                {
                    TestRadioButtons.Items.Clear();
                    TestRadioButtons.ItemsSource = null;
                    foreach (var item in GetItemsCollection())
                    {
                        TestRadioButtons.Items.Add(item);
                    }
                }
                else if (source == ItemsSourceComboBoxItem)
                {
                    TestRadioButtons.ItemsSource = GetItemsCollection();
                    TestRadioButtons.Items.Clear();
                }
            }
        }

        private IEnumerable GetItemsCollection()
        {
            if (ItemTypeComboBox.SelectedItem == StringsComboBoxItem)
            {
                InsertDisplayRadioButtonButton.IsEnabled = false;
                return m_stringItemCollection;
            }
            else
            {
                InsertDisplayRadioButtonButton.IsEnabled = true;
                return m_radioButtonItemCollection;
            }
        }

        private bool UpdateDisplayRadioButton()
        {
            if (UInt32.TryParse(CustomIndexTextBox.Text, out uint value))
            {
                if (value > m_radioButtonItemCollection.Count)
                {
                    DisplayRadioButtonErrorMessage.Text = "Index out of Range";
                    DisplayRadioButtonErrorMessage.Foreground = new SolidColorBrush(Colors.DarkRed);
                    return false;
                }
            }
            else
            {
                DisplayRadioButtonErrorMessage.Text = "Malformed Index";
                DisplayRadioButtonErrorMessage.Foreground = new SolidColorBrush(Colors.DarkRed);
                return false;
            }

            DisplayRadioButton.Content = CustomContentTextBox.Text;

            DisplayRadioButtonErrorMessage.Text = "Okay";
            DisplayRadioButtonErrorMessage.Foreground = new SolidColorBrush(Colors.Green);
            return true;
        }
    }
}
