using Microsoft.Toolkit.Uwp.UI.Controls;
using System;
using System.Collections;
using System.Collections.Specialized;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;

namespace DataGridTestDriver
{
    public sealed partial class MainPage : Page
    {
        private DataGrid dg;
        private MockData mockData;

        public MainPage()
        {
            this.InitializeComponent();

            this.Loaded += MainPage_Loaded;

            tcDataGrid.SelectedTabChanged += TcDataGrid_SelectedTabChanged;
        }

        public void SetExceptionMessage(string exceptionMessage)
        {
            txtException.Text = exceptionMessage;
        }

        private void MainPage_Loaded(object sender, RoutedEventArgs e)
        {
            mockData = new MockData();

            DataGridPropertiesControl dataGridPropertiesControl = new DataGridPropertiesControl();
            dataGridPropertiesControl.MockData = mockData;
            tcDataGrid.AddTab("Properties", dataGridPropertiesControl);

            DataSourcesControl dataSourcesControl = new DataSourcesControl(mockData);
            tcDataGrid.AddTab("ItemsSource", dataSourcesControl);

            DataGridColumnsControl dataGridColumnsControl = new DataGridColumnsControl();
            dataGridColumnsControl.MockData = mockData;
            tcDataGrid.AddTab("Columns", dataGridColumnsControl);

            DataGridRowsControl dataGridRowsControl = new DataGridRowsControl();
            tcDataGrid.AddTab("Rows", dataGridRowsControl);

            DataGridMethodsControl dataGridMethodsControl = new DataGridMethodsControl();
            tcDataGrid.AddTab("Methods", dataGridMethodsControl);

            DataGridEventsControl dataGridEventsControl = new DataGridEventsControl();
            tcDataGrid.AddTab("Events", dataGridEventsControl);

            DataGridColumnPropertiesControl dataGridColumnPropertiesControl = new DataGridColumnPropertiesControl();
            tcDataGridColumn.AddTab("Properties", dataGridColumnPropertiesControl);
            dataGridColumnPropertiesControl.HeaderChanging += DataGridColumnPropertiesControl_HeaderChanging;

            DataGridColumnMethodsControl dataGridColumnMethodsControl = new DataGridColumnMethodsControl();
            tcDataGridColumn.AddTab("Methods", dataGridColumnMethodsControl);

            UpdateDataGrid();
            UpdateDataGridColumn();
        }

        private void TcDataGrid_SelectedTabChanged(object sender, EventArgs e)
        {
            if (tcDataGrid.Tabs[0] == tcDataGrid.SelectedTab)
            {
                (tcDataGrid.Tabs[0].Panel.Children[0] as DataGridPropertiesControl).OnActivated();
            }
        }

        private void DataGridColumnPropertiesControl_HeaderChanging(object sender, HeaderChangingEventArgs e)
        {
            DataGridColumnPropertiesControl dataGridColumnPropertiesControl = sender as DataGridColumnPropertiesControl;
            if (dataGridColumnPropertiesControl != null)
            {
                string oldHeader = cmbDataGridColumn.SelectedValue as string;
                string newHeader = e.NewHeader as string;
                cmbDataGridColumn.Items.Add(newHeader);
                cmbDataGridColumn.Items.Remove(oldHeader);
                cmbDataGridColumn.SelectedValue = newHeader;
            }
        }

        private void btnClearEvents_Click(object sender, RoutedEventArgs e)
        {
            lstEvents.Items.Clear();
        }

        private void cmbDataGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateDataGrid();
        }

        private void cmbDataGridColumn_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateDataGridColumn();
        }

        private void UpdateDataGrid()
        {
            if (cmbDataGridColumn != null)
            {
                cmbDataGridColumn.SelectedIndex = 0;
                while (cmbDataGridColumn.Items.Count > 1)
                    cmbDataGridColumn.Items.RemoveAt(1);
            }

            if (dg != null)
            {
                if (Utilities.Utilities.IsRS4OrHigher)
                {
                    dg.BringIntoViewRequested -= UIE_BringIntoViewRequested;
                }

                dg.AutoGeneratingColumn -= Dg_AutoGeneratingColumn;
                dg.BeginningEdit -= Dg_BeginningEdit;
                dg.CellEditEnded -= Dg_CellEditEnded;
                dg.CellEditEnding -= Dg_CellEditEnding;
                dg.ColumnDisplayIndexChanged -= Dg_ColumnDisplayIndexChanged;
                dg.ColumnHeaderDragCompleted -= Dg_ColumnHeaderDragCompleted;
                dg.ColumnHeaderDragDelta -= Dg_ColumnHeaderDragDelta;
                dg.ColumnHeaderDragStarted -= Dg_ColumnHeaderDragStarted;
                dg.ColumnReordered -= Dg_ColumnReordered;
                dg.ColumnReordering -= Dg_ColumnReordering;
                dg.CopyingRowClipboardContent -= Dg_CopyingRowClipboardContent;
                dg.CurrentCellChanged -= Dg_CurrentCellChanged;
                dg.LoadingRow -= Dg_LoadingRow;
                dg.LoadingRowDetails -= Dg_LoadingRowDetails;
                dg.LoadingRowGroup -= Dg_LoadingRowGroup;
                dg.PreparingCellForEdit -= Dg_PreparingCellForEdit;
                dg.RowDetailsVisibilityChanged -= Dg_RowDetailsVisibilityChanged;
                dg.RowEditEnded -= Dg_RowEditEnded;
                dg.RowEditEnding -= Dg_RowEditEnding;
                dg.SelectionChanged -= Dg_SelectionChanged;
                dg.Sorting -= Dg_Sorting;
                dg.UnloadingRow -= Dg_UnloadingRow;
                dg.UnloadingRowDetails -= Dg_UnloadingRowDetails;
                dg.UnloadingRowGroup -= Dg_UnloadingRowGroup;

                dg.Columns.CollectionChanged -= Dg_Columns_CollectionChanged;
            }

            if (cmbDataGrid.SelectedIndex == 0)
            {
                dg2.Visibility = Visibility.Collapsed;
                dg1.Visibility = Visibility.Visible;
                dg = dg1;
            }
            else
            {
                dg1.Visibility = Visibility.Collapsed;
                dg2.Visibility = Visibility.Visible;
                dg = dg2;
            }

            if (Utilities.Utilities.IsRS4OrHigher)
            {
                dg.BringIntoViewRequested += UIE_BringIntoViewRequested;
            }

            dg.AutoGeneratingColumn += Dg_AutoGeneratingColumn;
            dg.BeginningEdit += Dg_BeginningEdit;
            dg.CellEditEnded += Dg_CellEditEnded;
            dg.CellEditEnding += Dg_CellEditEnding;
            dg.ColumnDisplayIndexChanged += Dg_ColumnDisplayIndexChanged;
            dg.ColumnHeaderDragCompleted += Dg_ColumnHeaderDragCompleted;
            dg.ColumnHeaderDragDelta += Dg_ColumnHeaderDragDelta;
            dg.ColumnHeaderDragStarted += Dg_ColumnHeaderDragStarted;
            dg.ColumnReordered += Dg_ColumnReordered;
            dg.ColumnReordering += Dg_ColumnReordering;
            dg.CopyingRowClipboardContent += Dg_CopyingRowClipboardContent;
            dg.CurrentCellChanged += Dg_CurrentCellChanged;
            dg.LoadingRow += Dg_LoadingRow;
            dg.LoadingRowDetails += Dg_LoadingRowDetails;
            dg.LoadingRowGroup += Dg_LoadingRowGroup;
            dg.PreparingCellForEdit += Dg_PreparingCellForEdit;
            dg.RowDetailsVisibilityChanged += Dg_RowDetailsVisibilityChanged;
            dg.RowEditEnded += Dg_RowEditEnded;
            dg.RowEditEnding += Dg_RowEditEnding;
            dg.SelectionChanged += Dg_SelectionChanged;
            dg.Sorting += Dg_Sorting;
            dg.UnloadingRow += Dg_UnloadingRow;
            dg.UnloadingRowDetails += Dg_UnloadingRowDetails;
            dg.UnloadingRowGroup += Dg_UnloadingRowGroup;

            dg.Columns.CollectionChanged += Dg_Columns_CollectionChanged;

            if (cmbDataGridColumn != null)
            {
                foreach (DataGridColumn dataGridColumn in dg.Columns)
                {
                    if (dataGridColumn.Header != null)
                        cmbDataGridColumn.Items.Add(dataGridColumn.Header.ToString());
                }
            }

            if (tcDataGrid != null && tcDataGrid.Tabs.Count == 6)
            {
                (tcDataGrid.Tabs[0].Panel.Children[0] as DataGridPropertiesControl).DataGrid = dg;
                (tcDataGrid.Tabs[1].Panel.Children[0] as DataSourcesControl).DataGrid = dg;
                (tcDataGrid.Tabs[2].Panel.Children[0] as DataGridColumnsControl).DataGrid = dg;
                (tcDataGrid.Tabs[3].Panel.Children[0] as DataGridRowsControl).DataGrid = dg;
                (tcDataGrid.Tabs[4].Panel.Children[0] as DataGridMethodsControl).DataGrid = dg;
                (tcDataGrid.Tabs[5].Panel.Children[0] as DataGridEventsControl).DataGrid = dg;
            }
        }

        private void Dg_UnloadingRowDetails(object sender, DataGridRowDetailsEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_UnloadingRowDetails " + DataGridRowDetailsEventArgsToString(e));
            }
        }

        private void Dg_UnloadingRowGroup(object sender, DataGridRowGroupHeaderEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_UnloadingRowGroup " + DataGridRowGroupHeaderEventArgsToString(e));
            }
        }

        private void Dg_UnloadingRow(object sender, DataGridRowEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_UnloadingRow " + DataGridRowEventArgsToString(e));
            }
        }

        private void Dg_Sorting(object sender, DataGridColumnEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_Sorting " + DataGridColumnEventArgsToString(e));
            }

            DataGridEventsControl dataGridEventsControl = (tcDataGrid.Tabs[5].Panel.Children[0] as DataGridEventsControl);

            if (dataGridEventsControl.ChangeSortDirection)
            {
                switch (e.Column.SortDirection)
                {
                    case null:
                        e.Column.SortDirection = DataGridSortDirection.Ascending;
                        break;
                    case DataGridSortDirection.Ascending:
                        e.Column.SortDirection = DataGridSortDirection.Descending;
                        break;
                    case DataGridSortDirection.Descending:
                        e.Column.SortDirection = null;
                        break;
                }
            }
        }

        private void Dg_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_SelectionChanged");
            }
        }

        private void Dg_RowEditEnding(object sender, DataGridRowEditEndingEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_RowEditEnding " + DataGridRowToString(e.Row) + ", EditAction: " +  e.EditAction);
            }
        }

        private void Dg_RowEditEnded(object sender, DataGridRowEditEndedEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_RowEditEnded " + DataGridRowToString(e.Row) + ", EditAction: " + e.EditAction);
            }
        }

        private void Dg_RowDetailsVisibilityChanged(object sender, DataGridRowDetailsEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_RowDetailsVisibilityChanged " + DataGridRowDetailsEventArgsToString(e));
            }
        }

        private void Dg_PreparingCellForEdit(object sender, DataGridPreparingCellForEditEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_PreparingCellForEdit " + DataGridRowToString(e.Row) + ", " + DataGridColumnToString(e.Column) + ", EditingElement: " + FEToString(e.EditingElement));
            }

            DataGridEventsControl dataGridEventsControl = (tcDataGrid.Tabs[5].Panel.Children[0] as DataGridEventsControl);

            if (dataGridEventsControl.SetDataGridComboBoxColumnItemsSource)
            {
                DataGridComboBoxColumn dataGridComboBoxColumn = e.Column as DataGridComboBoxColumn;

                if (dataGridComboBoxColumn != null && dataGridComboBoxColumn.Binding != null && 
                    dataGridComboBoxColumn.Binding.Path != null && dataGridComboBoxColumn.Binding.Path.Path.Contains("Cit"))
                {
                    object dataItem = Utilities.Utilities.GetItemAtIndex((sender as DataGrid).ItemsSource, e.Row.GetIndex());
                    Person person = dataItem as Person;
                    if (person != null)
                    {
                        dataGridComboBoxColumn.ItemsSource = person.CitiesOfResidence;
                    }
                }
            }
        }

        private void Dg_LoadingRowDetails(object sender, DataGridRowDetailsEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_LoadingRowDetails " + DataGridRowDetailsEventArgsToString(e));
            }
        }

        private void Dg_LoadingRowGroup(object sender, DataGridRowGroupHeaderEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_LoadingRowGroup " + DataGridRowGroupHeaderEventArgsToString(e));
            }

            DataGridEventsControl dataGridEventsControl = (tcDataGrid.Tabs[5].Panel.Children[0] as DataGridEventsControl);

            if (dataGridEventsControl.SetRowGroupHeaderPropertyName)
            {
                e.RowGroupHeader.PropertyName = "LastName";
            }

            if (dataGridEventsControl.SetRowGroupHeaderPropertyValue)
            {
                ICollectionViewGroup group = e.RowGroupHeader.CollectionViewGroup;
                Person person = group.GroupItems[0] as Person;
                e.RowGroupHeader.PropertyValue = "The " + person.LastName + "s";
            }
        }

        private void Dg_LoadingRow(object sender, DataGridRowEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_LoadingRow " + DataGridRowEventArgsToString(e));
            }
        }

        private void Dg_CurrentCellChanged(object sender, EventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_CurrentCellChanged");
            }
        }

        private void Dg_CopyingRowClipboardContent(object sender, DataGridRowClipboardEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_CopyingRowClipboardContent " + DataGridRowClipboardEventArgsToString(e));
            }
        }

        private void Dg_ColumnReordering(object sender, DataGridColumnReorderingEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_ColumnReordering " + DataGridColumnToString(e.Column));
            }
        }

        private void Dg_ColumnReordered(object sender, DataGridColumnEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_ColumnReordered " + DataGridColumnEventArgsToString(e));
            }
        }

        private void Dg_ColumnHeaderDragStarted(object sender, DragStartedEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_ColumnHeaderDragStarted HorizontalOffset: " + e.HorizontalOffset + ", VerticalOffset: " + e.VerticalOffset);
            }
        }

        private void Dg_ColumnHeaderDragDelta(object sender, DragDeltaEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_ColumnHeaderDragDelta HorizontalChange: " + e.HorizontalChange + ", VerticalChange: " + e.VerticalChange);
            }
        }

        private void Dg_ColumnHeaderDragCompleted(object sender, DragCompletedEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_ColumnHeaderDragCompleted Canceled: " + e.Canceled + ", HorizontalChange: " + e.HorizontalChange + ", VerticalChange: " + e.VerticalChange);
            }
        }

        private void Dg_ColumnDisplayIndexChanged(object sender, DataGridColumnEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_ColumnDisplayIndexChanged " + DataGridColumnEventArgsToString(e));
            }
        }

        private void Dg_CellEditEnding(object sender, DataGridCellEditEndingEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_CellEditEnding " + DataGridRowToString(e.Row) + ", " + DataGridColumnToString(e.Column) + ", EditingElement: " + FEToString(e.EditingElement) + ", EditAction: " + e.EditAction);
            }
        }

        private void Dg_CellEditEnded(object sender, DataGridCellEditEndedEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_CellEditEnded " + DataGridRowToString(e.Row) + ", " + DataGridColumnToString(e.Column) + ", EditAction: " + e.EditAction);
            }
        }

        private void Dg_BeginningEdit(object sender, DataGridBeginningEditEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_BeginningEdit " + DataGridRowToString(e.Row) + ", " + DataGridColumnToString(e.Column));
            }
        }

        private void Dg_AutoGeneratingColumn(object sender, DataGridAutoGeneratingColumnEventArgs e)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_AutoGeneratingColumn " + DataGridColumnToString(e.Column) + ", PropertyName: " + e.PropertyName + ", PropertyType: " + e.PropertyType.Name);
            }

            if ((tcDataGrid.Tabs[5].Panel.Children[0] as DataGridEventsControl).UseDateTimeConverter && e.PropertyType.Name == "DateTime")
            {
                (e.Column as DataGridBoundColumn).Binding.Converter = new MUXControlsTestApp.Utilities.UniversalConverter();
            }
        }

        private void Dg_Columns_CollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Reset)
            {
                while (cmbDataGridColumn.Items.Count > 1)
                {
                    cmbDataGridColumn.Items.RemoveAt(1);
                }
            }
            else
            {
                if (e.OldItems != null)
                {
                    foreach (DataGridColumn dataGridColumn in e.OldItems)
                    {
                        if (dataGridColumn.Header != null)
                            cmbDataGridColumn.Items.Remove(dataGridColumn.Header.ToString());
                    }
                }

                if (e.NewItems != null)
                {
                    foreach (DataGridColumn dataGridColumn in e.NewItems)
                    {
                        if (dataGridColumn.Header != null)
                            cmbDataGridColumn.Items.Add(dataGridColumn.Header.ToString());
                    }
                }
            }

            (tcDataGrid.Tabs[4].Panel.Children[0] as DataGridMethodsControl).RefreshColumns();

            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "Dg_Columns_CollectionChanged Action: " + e.Action.ToString() + ", OldStartingIndex: " + e.OldStartingIndex + ", NewStartingIndex: " + e.NewStartingIndex);
            }
        }

        private void UIE_BringIntoViewRequested(UIElement sender, BringIntoViewRequestedEventArgs args)
        {
            if ((bool)chkRecordEvents.IsChecked)
            {
                lstEvents.Items.Insert(0, "UIE_BringIntoViewRequested");
            }
        }

        private string FEToString(FrameworkElement frameworkElement)
        {
            string feStr = "null";
            if (frameworkElement != null)
            {
                feStr = frameworkElement.Name + " (" + frameworkElement.GetType().Name + ")";
            }
            return feStr;
        }

        private string DataGridRowDetailsEventArgsToString(DataGridRowDetailsEventArgs e)
        {
            return "Row: " + DataGridRowToString(e.Row) + ", DetailsElement: " + FEToString(e.DetailsElement);
        }

        private string DataGridRowGroupHeaderEventArgsToString(DataGridRowGroupHeaderEventArgs e)
        {
            return "PropertyName: " + e.RowGroupHeader.PropertyName + ", PropertyValue: " + e.RowGroupHeader.PropertyValue + ", SublevelIndent: " + e.RowGroupHeader.SublevelIndent;
        }

        private string DataGridRowEventArgsToString(DataGridRowEventArgs e)
        {
            return "Row: " + DataGridRowToString(e.Row);
        }

        private string DataGridRowToString(DataGridRow dataGridRow)
        {
            return "Index: " + dataGridRow.GetIndex();
        }

        private string DataGridColumnEventArgsToString(DataGridColumnEventArgs e)
        {
            return "Column: " + DataGridColumnToString(e.Column);
        }

        private string DataGridColumnToString(DataGridColumn dataGridColumn)
        {
            return "DisplayIndex: " + dataGridColumn.DisplayIndex;
        }

        private string DataGridRowClipboardEventArgsToString(DataGridRowClipboardEventArgs dataGridRowClipboardEventArgs)
        {
            string str = "IsColumnHeadersRow: " + dataGridRowClipboardEventArgs.IsColumnHeadersRow;
            foreach (DataGridClipboardCellContent ccc in dataGridRowClipboardEventArgs.ClipboardRowContent)
            {
                str += " " + ccc.Content.ToString();
            }
            return str;
        }

        private void UpdateDataGridColumn()
        {
            if (cmbDataGridColumn.SelectedIndex == 0)
            {
                if (tcDataGrid != null)
                {
                    tcDataGrid.Visibility = Visibility.Visible;

                    if (tcDataGrid.Tabs.Count == 6)
                    {
                        (tcDataGrid.Tabs[4].Panel.Children[0] as DataGridMethodsControl).RefreshColumns();
                    }
                }

                if (tcDataGridColumn != null)
                { 
                    tcDataGridColumn.Visibility = Visibility.Collapsed;
                }
            }
            else if (tcDataGrid != null && tcDataGridColumn != null && tcDataGridColumn.Tabs.Count == 2)
            {
                DataGridColumnPropertiesControl dataGridColumnPropertiesControl = tcDataGridColumn.Tabs[0].Panel.Children[0] as DataGridColumnPropertiesControl;
                dataGridColumnPropertiesControl.DataGrid = dg;

                DataGridColumnMethodsControl dataGridColumnMethodsControl = tcDataGridColumn.Tabs[1].Panel.Children[0] as DataGridColumnMethodsControl;
                dataGridColumnMethodsControl.DataGrid = dg;

                foreach (DataGridColumn dataGridColumn in dg.Columns)
                {
                    if (dataGridColumn.Header != null && dataGridColumn.Header.ToString() == (string)cmbDataGridColumn.SelectedValue)
                    {
                        dataGridColumnPropertiesControl.DataGridColumn = dataGridColumn;
                        dataGridColumnMethodsControl.DataGridColumn = dataGridColumn;
                        break;
                    }
                }

                tcDataGrid.Visibility = Visibility.Collapsed;
                tcDataGridColumn.Visibility = Visibility.Visible;
            }
        }

        private void btnClearException_Click(object sender, RoutedEventArgs e)
        {
            txtException.Text = string.Empty;
        }
    }
}
