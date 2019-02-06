using Microsoft.Toolkit.Uwp.UI.Controls;
using Windows.UI.Xaml.Controls;

namespace DataGridTestDriver
{
    public sealed partial class DataGridEventsControl : UserControl
    {
        public DataGridEventsControl()
        {
            this.InitializeComponent();
        }

        public DataGrid DataGrid
        {
            get;
            set;
        }

        public bool UseDateTimeConverter
        {
            get
            {
                return (bool)chkUseDateTimeConverter.IsChecked;
            }
        }

        public bool SetRowGroupHeaderPropertyName
        {
            get
            {
                return (bool)chkSetRowGroupHeaderPropertyName.IsChecked;
            }
        }

        public bool SetRowGroupHeaderPropertyValue
        {
            get
            {
                return (bool)chkSetRowGroupHeaderPropertyValue.IsChecked;
            }
        }

        public bool ChangeSortDirection
        {
            get
            {
                return (bool)chkChangeSortDirection.IsChecked;
            }
        }

        public bool SetDataGridComboBoxColumnItemsSource
        {
            get
            {
                return (bool)chkSetDataGridComboBoxColumnItemsSource.IsChecked;
            }
        }
    }
}
