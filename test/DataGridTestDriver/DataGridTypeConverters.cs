using System;
using Windows.UI.Xaml.Data;
using Microsoft.Toolkit.Uwp.UI.Controls;
using System.Collections.Generic;
using Windows.UI.Xaml;

namespace DataGridTestDriver
{
    public class DataGridColumnConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value != null && targetType.Name == "String")
            {
                DataGridColumn column = value as DataGridColumn;
                if (column != null)
                {
                    string str = column.Header.ToString();
                    return str;
                }
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            DataGrid dataGrid = parameter as DataGrid;
            if (dataGrid != null)
            {
                foreach (DataGridColumn column in dataGrid.Columns)
                {
                    if (column.Header as string == value as string)
                    {
                        return column;
                    }
                }
            }

            return value;
        }
    }

    public class DataGridRowDetailsTemplateConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value == null)
                return "Null";

            Dictionary<string, DataTemplate> rowDetailsTemplates = parameter as Dictionary<string, DataTemplate>;
            
            if (rowDetailsTemplates != null)
            {
                foreach (string templateKey in rowDetailsTemplates.Keys)
                {
                    if (value == rowDetailsTemplates[templateKey])
                    {
                        return templateKey;
                    }
                }
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            string valueAsStr = value as string;

            if (valueAsStr == "Null")
                return null;

            Dictionary<string, DataTemplate> rowDetailsTemplates = parameter as Dictionary<string, DataTemplate>;

            foreach (string templateKey in rowDetailsTemplates.Keys)
            {
                if (valueAsStr == templateKey)
                {
                    return rowDetailsTemplates[templateKey];
                }
            }

            return value;
        }
    }
}
