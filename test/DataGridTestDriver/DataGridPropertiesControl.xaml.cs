using MUXControlsTestApp.Utilities;
using Microsoft.Toolkit.Uwp.UI.Controls;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;

namespace DataGridTestDriver
{
    public sealed partial class DataGridPropertiesControl : UserControl
    {
        static private UniversalConverter _universalConverter = new UniversalConverter();
        private DataGrid _dataGrid;

        public DataGridPropertiesControl()
        {
            this.InitializeComponent();
            this.epc.EntityPropertyControlNeeded += Epc_EntityPropertyControlNeeded;
            this.epc.Level = 2;

            DataTemplate dataTemplate = Resources["PersonRowDetailsTemplate"] as DataTemplate;
            if (dataTemplate != null)
            {
                this.epc.RowDetailsTemplates.Add("PersonRowDetailsTemplate", dataTemplate);
            }

            dataTemplate = Resources["EmployeeRowDetailsTemplate"] as DataTemplate;
            if (dataTemplate != null)
            {
                this.epc.RowDetailsTemplates.Add("EmployeeRowDetailsTemplate", dataTemplate);
            }
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
                    epc.Entity = value;
                }
            }
        }

        public MockData MockData
        {
            get;
            set;
        }

        public void OnActivated()
        {
            if (this.epc != null)
                this.epc.PopulateRowDetailsTemplateComboBox();
        }

        private void Epc_EntityPropertyControlNeeded(object sender, EntityPropertyControlNeededEventArgs e)
        {
            switch (e.PropertyInfo.Name)
            {
                case "ItemsSource":
                    {
                        ComboBox comboBox = new ComboBox();
                        comboBox.VerticalAlignment = VerticalAlignment.Center;
                        comboBox.HorizontalAlignment = HorizontalAlignment.Stretch;
                        comboBox.Margin = new Thickness(1);
                        comboBox.Items.Add("Null");
                        comboBox.Items.Add("PersonCVS");
                        comboBox.Items.Add("EmployeeCVS");
                        comboBox.Items.Add("PersonGroupedByLastNameCVS");
                        comboBox.Items.Add("EmployeeGroupedByLastNameCVS");
                        comboBox.Items.Add("PersonList");
                        comboBox.Items.Add("EmployeeList");
                        comboBox.Items.Add("PersonBindableVectorView");
                        comboBox.Items.Add("EmployeeBindableVectorView");
                        comboBox.Items.Add("PersonObservableCollection");
                        comboBox.Items.Add("EmployeeObservableCollection");
                        comboBox.Items.Add("EmployeeProgressiveObservableCollection");
                        comboBox.Items.Add("Empty PersonObservableCollection");
                        comboBox.Items.Add("Empty EmployeeObservableCollection");
                        comboBox.SelectedIndex = 0;
                        comboBox.SelectionChanged += ItemsSourceComboBox_SelectionChanged;

                        e.PropertyControl = comboBox;
                        break;
                    }
                case "ColumnWidth":
                    {
                        StackPanel stackPanel = new StackPanel();
                        ComboBox comboBox = new ComboBox();
                        comboBox.VerticalAlignment = VerticalAlignment.Center;
                        comboBox.HorizontalAlignment = HorizontalAlignment.Stretch;
                        comboBox.Margin = new Thickness(1);
                        comboBox.Items.Add("Auto");
                        comboBox.Items.Add("Pixel");
                        comboBox.Items.Add("SizeToCells");
                        comboBox.Items.Add("SizeToHeader");
                        comboBox.Items.Add("Star");
                        comboBox.SelectedIndex = 0;
                        stackPanel.Children.Add(comboBox);

                        TextBox textBoxValue = new TextBox();
                        textBoxValue.VerticalAlignment = VerticalAlignment.Center;
                        textBoxValue.Margin = new Thickness(1);
                        Binding binding = new Binding();
                        binding.Source = _dataGrid;
                        binding.Path = new PropertyPath("ColumnWidth.Value");
                        binding.Mode = BindingMode.TwoWay;
                        binding.Converter = _universalConverter;
                        textBoxValue.SetBinding(TextBox.TextProperty, binding);
                        stackPanel.Children.Add(textBoxValue);

                        TextBox textBoxDesiredValue = new TextBox();
                        textBoxDesiredValue.VerticalAlignment = VerticalAlignment.Center;
                        textBoxDesiredValue.Margin = new Thickness(1);
                        binding = new Binding();
                        binding.Source = _dataGrid;
                        binding.Path = new PropertyPath("ColumnWidth.DesiredValue");
                        binding.Mode = BindingMode.TwoWay;
                        binding.Converter = _universalConverter;
                        textBoxDesiredValue.SetBinding(TextBox.TextProperty, binding);
                        stackPanel.Children.Add(textBoxDesiredValue);

                        TextBox textBoxDisplayValue = new TextBox();
                        textBoxDisplayValue.VerticalAlignment = VerticalAlignment.Center;
                        textBoxDisplayValue.Margin = new Thickness(1);
                        binding = new Binding();
                        binding.Source = _dataGrid;
                        binding.Path = new PropertyPath("ColumnWidth.DisplayValue");
                        binding.Mode = BindingMode.TwoWay;
                        binding.Converter = _universalConverter;
                        textBoxDisplayValue.SetBinding(TextBox.TextProperty, binding);
                        stackPanel.Children.Add(textBoxDisplayValue);

                        if (e.PropertyInfo.CanWrite && e.PropertyInfo.SetMethod.IsPublic)
                        {
                            Button btnSetColumnWidth = new Button();
                            btnSetColumnWidth.Content = "Set " + e.PropertyInfo.Name;
                            btnSetColumnWidth.HorizontalAlignment = HorizontalAlignment.Stretch;
                            btnSetColumnWidth.Margin = new Thickness(1);
                            btnSetColumnWidth.Tag = e.PropertyInfo;
                            btnSetColumnWidth.Click += BtnSetColumnWidth_Click;
                            stackPanel.Children.Add(btnSetColumnWidth);
                        }

                        e.PropertyControl = stackPanel;
                        break;
                    }
                case "Columns":
                    {
                        e.SkipUIAutoGeneration = true;
                        break;
                    }
            }
        }

        private void ItemsSourceComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.MockData != null)
            {
                ComboBox comboBox = sender as ComboBox;
                switch (comboBox.SelectedIndex)
                {
                    case 0:
                        _dataGrid.ItemsSource = null;
                        break;
                    case 1:
                        _dataGrid.ItemsSource = this.MockData.PersonCollectionViewSource.View;
                        break;
                    case 2:
                        _dataGrid.ItemsSource = this.MockData.EmployeeCollectionViewSource.View;
                        break;
                    case 3:
                        _dataGrid.ItemsSource = this.MockData.PersonGroupedByLastNameCollectionViewSource.View;
                        break;
                    case 4:
                        _dataGrid.ItemsSource = this.MockData.EmployeeGroupedByLastNameCollectionViewSource.View;
                        break;
                    case 5:
                        _dataGrid.ItemsSource = this.MockData.PersonList;
                        break;
                    case 6:
                        _dataGrid.ItemsSource = this.MockData.EmployeeList;
                        break;
                    case 7:
                        _dataGrid.ItemsSource = this.MockData.PersonBindableVectorView;
                        break;
                    case 8:
                        _dataGrid.ItemsSource = this.MockData.EmployeeBindableVectorView;
                        break;
                    case 9:
                        _dataGrid.ItemsSource = this.MockData.PersonObservableCollection;
                        break;
                    case 10:
                        _dataGrid.ItemsSource = this.MockData.EmployeeObservableCollection;
                        break;
                    case 11:
                        _dataGrid.ItemsSource = this.MockData.EmployeeProgressiveObservableCollection;
                        break;
                    case 12:
                        _dataGrid.ItemsSource = this.MockData.EmptyPersonObservableCollection;
                        break;
                    case 13:
                        _dataGrid.ItemsSource = this.MockData.EmptyEmployeeObservableCollection;
                        break;
                }

                this.epc.PopulateRowDetailsTemplateComboBox();
            }
        }

        private void BtnSetColumnWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Button btnSetColumnWidth = sender as Button;
                StackPanel stackPanel = btnSetColumnWidth.Parent as StackPanel;

                ComboBox comboBox = stackPanel.Children[0] as ComboBox;
                DataGridLengthUnitType unitType = DataGridLengthUnitType.Auto;
                switch (comboBox.SelectedIndex)
                {
                    case 1:
                        unitType = DataGridLengthUnitType.Pixel;
                        break;
                    case 2:
                        unitType = DataGridLengthUnitType.SizeToCells;
                        break;
                    case 3:
                        unitType = DataGridLengthUnitType.SizeToHeader;
                        break;
                    case 4:
                        unitType = DataGridLengthUnitType.Star;
                        break;
                }

                TextBox valueTextBox = stackPanel.Children[1] as TextBox;
                double value = Convert.ToDouble(valueTextBox.Text);

                TextBox desiredValueTextBox = stackPanel.Children[2] as TextBox;
                double desiredValue = Convert.ToDouble(desiredValueTextBox.Text);

                TextBox displayValueTextBox = stackPanel.Children[3] as TextBox;
                double displayValue = Convert.ToDouble(displayValueTextBox.Text);

                _dataGrid.ColumnWidth = new DataGridLength(value, unitType, desiredValue, displayValue);
            }
            catch (FormatException)
            {
            }
        }
    }
}