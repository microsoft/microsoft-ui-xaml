using Microsoft.Toolkit.Uwp.UI.Controls;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace DataGridTestDriver
{
    public sealed partial class DataGridRowsControl : UserControl
    {
        private DataGrid _dataGrid;

        public DataGridRowsControl()
        {
            this.InitializeComponent();
            this.DataGridRows = new List<DataGridRow>();
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
                    if (_dataGrid != null)
                    {
                        _dataGrid.LoadingRow -= DataGrid_LoadingRow;
                        _dataGrid.UnloadingRow -= DataGrid_UnloadingRow;
                        this.DataGridRows.Clear();
                    }

                    _dataGrid = value;

                    if (_dataGrid != null)
                    {
                        _dataGrid.LoadingRow += DataGrid_LoadingRow;
                        _dataGrid.UnloadingRow += DataGrid_UnloadingRow;
                    }
                }
            }
        }

        private List<DataGridRow> DataGridRows
        {
            get;
            set;
        }

        private void DataGrid_LoadingRow(object sender, DataGridRowEventArgs e)
        {
            this.DataGridRows.Add(e.Row);
        }

        private void DataGrid_UnloadingRow(object sender, DataGridRowEventArgs e)
        {
            this.DataGridRows.Remove(e.Row);
        }

        private void btnRefreshRows_Click(object sender, RoutedEventArgs e)
        {
            int selectedRow = int.MaxValue;
            int selectedIndex = -1;
            cmbRows.Items.Clear();
            foreach (DataGridRow dataGridRow in this.DataGridRows)
            {
                int addedRow = dataGridRow.GetIndex();
                cmbRows.Items.Add(addedRow);
                if (addedRow < selectedRow)
                {
                    selectedRow = addedRow;
                    selectedIndex = cmbRows.Items.Count - 1;
                }
            }
            if (selectedIndex >= 0)
                cmbRows.SelectedIndex = selectedIndex;
        }

        private void cmbRows_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbRows.SelectedIndex >= 0)
            {
                this.ecp.Entity = this.DataGridRows[cmbRows.SelectedIndex];
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
    }
}
