using MUXControlsTestApp.Utilities;
using Microsoft.Toolkit.Uwp.UI.Controls;
using System;
using System.Collections.Generic;
using System.Reflection;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;

namespace DataGridTestDriver
{
    public class DataGridEntityPropertiesControl : EntityPropertiesControl
    {
        private static DataGridColumnConverter s_dataGridColumnConverter = new DataGridColumnConverter();
        private static DataGridRowDetailsTemplateConverter s_rowDetailsTemplateConverter = new DataGridRowDetailsTemplateConverter();
        private ComboBox _comboBoxRowDetailsTemplate;

        public DataGridEntityPropertiesControl()
        {
            this.RowDetailsTemplates = new Dictionary<string, DataTemplate>();
        }

        public Dictionary<string, DataTemplate> RowDetailsTemplates
        {
            get;
            private set;
        }

        protected override void GenerateUI(PropertyInfo propertyInfo, out FrameworkElement propertyControl, out Type pretendType, out IValueConverter converter, out object converterParameter)
        {
            base.GenerateUI(propertyInfo, out propertyControl, out pretendType, out converter, out converterParameter);

            if (propertyInfo.PropertyType.FullName == "Microsoft.Toolkit.Uwp.UI.Controls.DataGridColumn")
            {
                pretendType = typeof(object);
                converter = s_dataGridColumnConverter;
                converterParameter = this.Entity;
                return;
            }

            if (propertyInfo.Name == "RowDetailsTemplate")
            {
                _comboBoxRowDetailsTemplate = new ComboBox();
                _comboBoxRowDetailsTemplate.VerticalAlignment = VerticalAlignment.Center;
                _comboBoxRowDetailsTemplate.HorizontalAlignment = HorizontalAlignment.Stretch;
                _comboBoxRowDetailsTemplate.Margin = new Thickness(1);
                _comboBoxRowDetailsTemplate.IsEnabled = propertyInfo.CanWrite && propertyInfo.SetMethod.IsPublic;
                PopulateRowDetailsTemplateComboBox();
                Binding binding = new Binding();
                binding.Source = this.Entity;
                binding.Path = new PropertyPath(propertyInfo.Name);
                binding.Mode = _comboBoxRowDetailsTemplate.IsEnabled ? BindingMode.TwoWay : BindingMode.OneWay;
                binding.Converter = s_rowDetailsTemplateConverter;
                binding.ConverterParameter = this.RowDetailsTemplates;
                _comboBoxRowDetailsTemplate.SetBinding(ComboBox.SelectedValueProperty, binding);
                propertyControl = _comboBoxRowDetailsTemplate;
            }
        }

        public void PopulateRowDetailsTemplateComboBox()
        {
            DataGrid dataGrid = this.Entity as DataGrid;

            if (dataGrid == null || _comboBoxRowDetailsTemplate == null)
                return;

            string oldSelectedItem = _comboBoxRowDetailsTemplate.SelectedItem == null ? string.Empty : _comboBoxRowDetailsTemplate.SelectedItem as string;
            int oldSelectedIndex = _comboBoxRowDetailsTemplate.SelectedIndex;
            int newSelectedIndex = 0;

            _comboBoxRowDetailsTemplate.Items.Clear();
            _comboBoxRowDetailsTemplate.Items.Add("Null");

            if (dataGrid.ItemsSource != null)
            {
                object firstItem = Utilities.Utilities.GetItemAtIndex(dataGrid.ItemsSource, 0);

                if (firstItem != null)
                {
                    foreach (string templateKey in this.RowDetailsTemplates.Keys)
                    {
                        if (templateKey.Contains("Person") && firstItem.GetType().Name.Contains("Person"))
                        {
                            _comboBoxRowDetailsTemplate.Items.Add(templateKey);
                            if (oldSelectedItem == templateKey)
                                newSelectedIndex = _comboBoxRowDetailsTemplate.Items.Count - 1;
                        }
                        else if (templateKey.Contains("Employee") && firstItem.GetType().Name.Contains("Employee"))
                        {
                            _comboBoxRowDetailsTemplate.Items.Add(templateKey);
                            if (oldSelectedItem == templateKey)
                                newSelectedIndex = _comboBoxRowDetailsTemplate.Items.Count - 1;
                        }
                    }
                }
            }

            _comboBoxRowDetailsTemplate.SelectedIndex = newSelectedIndex;
        }
    }
}
