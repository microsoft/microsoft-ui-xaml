using Microsoft.Toolkit.Uwp.UI.Controls;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace DataGridTestDriver
{
    public sealed partial class DataGridColumnMethodsControl : UserControl
    {
        public DataGridColumnMethodsControl()
        {
            this.InitializeComponent();
        }

        public DataGrid DataGrid
        {
            get;
            set;
        }

        public DataGridColumn DataGridColumn
        {
            get;
            set;
        }

        private void btnGetCellContent_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtGetCellContentResult.Text = string.Empty;
                int itemIndex = Convert.ToInt32(txtItemIndex.Text);
                object item = Utilities.Utilities.GetItemAtIndex(this.DataGrid.ItemsSource, itemIndex);
                FrameworkElement frameworkElement = this.DataGridColumn.GetCellContent(item);
                if (frameworkElement == null)
                {
                    txtGetCellContentResult.Text = "Null";
                }
                else
                {
                    txtGetCellContentResult.Text = frameworkElement.ToString();
                    if (frameworkElement is TextBlock)
                    {
                        txtGetCellContentResult.Text += " " + (frameworkElement as TextBlock).Text;
                    }
                    else if (frameworkElement is TextBox)
                    {
                        txtGetCellContentResult.Text += " " + (frameworkElement as TextBox).Text;
                    }
                    else if(frameworkElement is ComboBox)
                    {
                        txtGetCellContentResult.Text += " " + (frameworkElement as ComboBox).SelectedItem;
                    }
                    else if (frameworkElement is CheckBox)
                    {
                        txtGetCellContentResult.Text += " " + (frameworkElement as CheckBox).IsChecked;
                    }
                }
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
    }
}
