using Microsoft.Toolkit.Uwp.UI.Controls;
using Microsoft.Toolkit.Uwp.UI.Controls.Primitives;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace DataGridTestDriver
{
    public sealed partial class DataGridMethodsControl : UserControl
    {
        private DataGrid _dataGrid;

        public DataGridMethodsControl()
        {
            this.InitializeComponent();
        }

        public DataGrid DataGrid
        {
            get
            {
                return _dataGrid;
            }
            set
            {
                if (_dataGrid != value)
                {
                    _dataGrid = value;
                    OnDataGridChanged();
                }
            }
        }

        public void RefreshColumns()
        {
            cmbColumn1.Items.Clear();
            cmbColumn2.Items.Clear();

            if (this.DataGrid == null)
            {
                btnScrollIntoView.IsEnabled = false;
            }
            else
            {
                cmbColumn1.Items.Add("Null");

                foreach (DataGridColumn column in this.DataGrid.Columns)
                {
                    cmbColumn1.Items.Add(column.Header);
                    cmbColumn2.Items.Add(column.Header);
                }

                cmbColumn1.SelectedIndex = 0;
                btnScrollIntoView.IsEnabled = true;

                if (cmbColumn2.Items.Count > 0)
                {
                    btnGetColumnContainingElement.IsEnabled = true;
                    cmbColumn2.SelectedIndex = 0;
                }
                else
                {
                    btnGetColumnContainingElement.IsEnabled = false;
                }
            }
        }

        private void btnBeginEdit_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                chkBeginEditResult.IsChecked = null;
                bool result = this.DataGrid.BeginEdit(e);
                chkBeginEditResult.IsChecked = result;
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnCancelEdit_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                chkCancelEditResult.IsChecked = null;
                bool result = this.DataGrid.CancelEdit(cmbCancelEditingUnit.SelectedIndex == 0 ? DataGridEditingUnit.Cell : DataGridEditingUnit.Row);
                chkCancelEditResult.IsChecked = result;
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnCommitEdit_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                chkCommitEditResult.IsChecked = null;
                bool result = this.DataGrid.CommitEdit(cmbCommitEditingUnit.SelectedIndex == 0 ? DataGridEditingUnit.Cell : DataGridEditingUnit.Row, (bool)chkExitEditingMode.IsChecked);
                chkCommitEditResult.IsChecked = result;
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnScrollIntoView_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                DataGridColumn column = null;

                if (cmbColumn1.SelectedIndex != 0)
                {
                    string header = cmbColumn1.SelectedItem as string;

                    foreach (DataGridColumn columnTmp in this.DataGrid.Columns)
                    {
                        if (columnTmp.Header as string == header)
                        {
                            column = columnTmp;
                            break;
                        }
                    }
                }

                int itemIndex = Convert.ToInt32(txtItemIndex.Text);
                object item = Utilities.Utilities.GetItemAtIndex(this.DataGrid.ItemsSource, itemIndex);

                this.DataGrid.ScrollIntoView(item, column);
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnGetColumnContainingElement_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtGetColumnContainingElementResult.Text = string.Empty;
                FrameworkElement frameworkElement = null;
                DataGridColumn column = null;
                string header = cmbColumn2.SelectedItem as string;

                foreach (DataGridColumn columnTmp in this.DataGrid.Columns)
                {
                    if (columnTmp.Header as string == header)
                    {
                        column = columnTmp;
                        break;
                    }
                }

                if (cmbFrameworkElement.SelectedIndex == 0)
                {
                    DataGridColumnHeadersPresenter columnHeadersPresenter = Utilities.Utilities.GetChildOfType<DataGridColumnHeadersPresenter>(this.DataGrid);
                    if (columnHeadersPresenter != null)
                    {
                        int childCount = VisualTreeHelper.GetChildrenCount(columnHeadersPresenter);
                        for (int i = 0; i < childCount; i++)
                        {
                            DependencyObject current = VisualTreeHelper.GetChild(columnHeadersPresenter, i);
                            DataGridColumnHeader columnHeader = current as DataGridColumnHeader;
                            if (columnHeader != null && columnHeader.Content == column.Header)
                            {
                                frameworkElement = columnHeader;
                            }
                        }
                    }
                }
                else
                {
                    frameworkElement = column.GetCellContent(Utilities.Utilities.GetItemAtIndex(this.DataGrid.ItemsSource, 0));
                }

                column = DataGridColumn.GetColumnContainingElement(frameworkElement);
                if (column == null)
                {
                    txtGetColumnContainingElementResult.Text = "Null";
                }
                else
                {
                    txtGetColumnContainingElementResult.Text = column.Header.ToString();
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

        private void OnDataGridChanged()
        {
            if (this.DataGrid == null)
            {
                btnBeginEdit.IsEnabled = false;
                btnCancelEdit.IsEnabled = false;
                btnCommitEdit.IsEnabled = false;
            }
            else
            {
                btnBeginEdit.IsEnabled = true;
                btnCancelEdit.IsEnabled = true;
                btnCommitEdit.IsEnabled = true;
            }

            RefreshColumns();
        }
    }
}
