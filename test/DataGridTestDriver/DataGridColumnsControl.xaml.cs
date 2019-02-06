using Microsoft.Toolkit.Uwp.UI.Controls;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Markup;

namespace DataGridTestDriver
{
    public sealed partial class DataGridColumnsControl : UserControl
    {
        public DataGridColumnsControl()
        {
            this.InitializeComponent();
        }

        public DataGrid DataGrid
        {
            get;
            set;
        }

        public MockData MockData
        {
            get;
            set;
        }

        private void btnAdd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                DataGridColumn dataGridColumn = GetNewColumn();

                if (dataGridColumn != null)
                {
                    dataGridColumn.Header = "AddedColumn";
                }

                this.DataGrid.Columns.Add(dataGridColumn);
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnInsert_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                int index = Convert.ToInt32(txtIndex.Text);
                DataGridColumn dataGridColumn = GetNewColumn();

                if (dataGridColumn != null)
                {
                    dataGridColumn.Header = "InsertedColumn";
                }

                this.DataGrid.Columns.Insert(index, dataGridColumn);
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnRemoveAt_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                int index = Convert.ToInt32(txtIndex.Text);

                this.DataGrid.Columns.RemoveAt(index);
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnClear_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                this.DataGrid.Columns.Clear();
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnClearException_Click(object sender, RoutedEventArgs e)
        {
            txtException.Text = string.Empty;
        }

        private void cmbType_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbType != null)
            {
                if (cmbCellTemplate != null)
                {
                    cmbCellTemplate.IsEnabled = cmbType.SelectedIndex == 3;
                }

                if (cmbCellEditingTemplate != null)
                {
                    cmbCellEditingTemplate.IsEnabled = cmbType.SelectedIndex == 3;
                }

                if (txtPropertyChoices != null)
                {
                    txtPropertyChoices.IsEnabled = cmbType.SelectedIndex == 2;
                }
            }
        }

        private DataGridColumn GetNewColumn()
        {
            DataGridColumn dataGridColumn = null;

            switch (cmbType.SelectedIndex)
            {
                case 0:
                    dataGridColumn = new DataGridCheckBoxColumn();
                    break;
                case 1:
                    dataGridColumn = new DataGridTextColumn();
                    break;
                case 2:
                    dataGridColumn = new DataGridComboBoxColumn();
                    break;
                case 3:
                    dataGridColumn = new DataGridTemplateColumn();
                    LoadTemplates(dataGridColumn as DataGridTemplateColumn);
                    break;
            }

            if (dataGridColumn != null)
            {
                DataGridBoundColumn dataGridBoundColumn = dataGridColumn as DataGridBoundColumn;
                if (dataGridBoundColumn != null && !string.IsNullOrEmpty(txtPropertyName.Text))
                {
                    Binding binding = new Binding();
                    binding.Path = new PropertyPath(txtPropertyName.Text);
                    dataGridBoundColumn.Binding = binding;
                }

                DataGridComboBoxColumn dataGridComboBoxColumn = dataGridColumn as DataGridComboBoxColumn;
                if (dataGridComboBoxColumn != null)
                {
                    if (string.IsNullOrEmpty(txtPropertyChoices.Text))
                    {
                        if (txtPropertyName.Text == "ZipCode")
                        {
                            dataGridComboBoxColumn.ItemsSource = this.MockData.ZipCodes;
                        }
                    }
                    else
                    {
                        dataGridComboBoxColumn.DisplayMemberPath = txtPropertyChoices.Text;
                        dataGridComboBoxColumn.ItemsSource = this.MockData.Addresses;
                    }
                }
            }

            return dataGridColumn;
        }

        private void LoadTemplates(DataGridTemplateColumn dataGridTemplateColumn)
        {
            string dataTemplateStr = string.Empty;
            DataTemplate dataTemplate = null;

            switch (cmbCellTemplate.SelectedIndex)
            {
                case 0:
                    dataTemplateStr =
                       string.Format(
                           @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' 
                                 xmlns:local='using:DataGridTestDriver'>
                                 <CheckBox IsChecked='{{Binding {0}, Mode=TwoWay}}'/>
                             </DataTemplate>", txtPropertyName.Text);
                    break;
                case 1:
                    dataTemplateStr =
                       string.Format(
                           @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' 
                                 xmlns:local='using:DataGridTestDriver'>
                                 <TextBlock VerticalAlignment='Center' Text='{{Binding {0}, Mode=OneWay}}'/>
                             </DataTemplate>", txtPropertyName.Text);
                    break;
                case 2:
                    dataTemplateStr =
                       string.Format(
                           @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' 
                                 xmlns:local='using:DataGridTestDriver'>
                                 <CalendarDatePicker HorizontalAlignment='Stretch' IsEnabled='False' Date='{{Binding {0}, Mode=TwoWay}}'/>
                             </DataTemplate>", txtPropertyName.Text);
                    break;
            }

            if (!string.IsNullOrEmpty(dataTemplateStr))
            {
                dataTemplate = XamlReader.Load(dataTemplateStr) as DataTemplate;
                dataGridTemplateColumn.CellTemplate = dataTemplate;
                dataTemplateStr = string.Empty;
            }

            switch (cmbCellEditingTemplate.SelectedIndex)
            {
                case 0:
                    dataTemplateStr =
                       string.Format(
                           @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' 
                                 xmlns:local='using:DataGridTestDriver'>
                                 <CheckBox IsChecked='{{Binding {0}, Mode=TwoWay}}'/>
                             </DataTemplate>", txtPropertyName.Text);
                    break;
                case 1:
                    dataTemplateStr =
                       string.Format(
                           @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' 
                                 xmlns:local='using:DataGridTestDriver'>
                                 <TextBox Text='{{Binding {0}, Mode=TwoWay}}'/>
                             </DataTemplate>", txtPropertyName.Text);
                    break;
                case 2:
                    dataTemplateStr =
                       string.Format(
                           @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' 
                                 xmlns:local='using:DataGridTestDriver'>
                                 <CalendarDatePicker HorizontalAlignment='Stretch' Date='{{Binding {0}, Mode=TwoWay}}'/>
                             </DataTemplate>", txtPropertyName.Text);
                    break;
            }

            if (!string.IsNullOrEmpty(dataTemplateStr))
            {
                dataTemplate = XamlReader.Load(dataTemplateStr) as DataTemplate;
                dataGridTemplateColumn.CellEditingTemplate = dataTemplate;
            }
        }
    }
}
