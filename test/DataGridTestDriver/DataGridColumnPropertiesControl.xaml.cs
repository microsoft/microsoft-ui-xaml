using MUXControlsTestApp.Utilities;
using Microsoft.Toolkit.Uwp.UI.Controls;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;

namespace DataGridTestDriver
{
    public sealed partial class DataGridColumnPropertiesControl : UserControl
    {
        static private UniversalConverter _universalConverter = new UniversalConverter();
        private DataGridColumn _dataGridColumn;

        public event EventHandler<HeaderChangingEventArgs> HeaderChanging;

        public DataGridColumnPropertiesControl()
        {
            this.InitializeComponent();
            this.epc.EntityPropertyControlNeeded += Epc_EntityPropertyControlNeeded;
            this.epc.EntityPropertyControlGenerated += Epc_EntityPropertyControlGenerated;
            this.epc.Level = 3;
        }

        public DataGrid DataGrid
        {
            get;
            set;
        }

        public DataGridColumn DataGridColumn
        {
            get
            {
                return _dataGridColumn;
            }

            set
            {
                if (_dataGridColumn != value)
                {
                    _dataGridColumn = value;
                    epc.Entity = value;
                }
            }
        }

        private void Epc_EntityPropertyControlNeeded(object sender, EntityPropertyControlNeededEventArgs e)
        {
            switch (e.PropertyInfo.Name)
            {
                case "Width":
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
                        comboBox.SelectedIndex = (int)_dataGridColumn.Width.UnitType;
                        stackPanel.Children.Add(comboBox);

                        TextBox textBoxValue = new TextBox();
                        textBoxValue.VerticalAlignment = VerticalAlignment.Center;
                        textBoxValue.Margin = new Thickness(1);
                        Binding binding = new Binding();
                        binding.Source = _dataGridColumn;
                        binding.Path = new PropertyPath("Width.Value");
                        binding.Mode = BindingMode.OneWay;
                        binding.Converter = _universalConverter;
                        textBoxValue.SetBinding(TextBox.TextProperty, binding);
                        stackPanel.Children.Add(textBoxValue);

                        TextBox textBoxDesiredValue = new TextBox();
                        textBoxDesiredValue.VerticalAlignment = VerticalAlignment.Center;
                        textBoxDesiredValue.Margin = new Thickness(1);
                        binding = new Binding();
                        binding.Source = _dataGridColumn;
                        binding.Path = new PropertyPath("Width.DesiredValue");
                        binding.Mode = BindingMode.OneWay;
                        binding.Converter = _universalConverter;
                        textBoxDesiredValue.SetBinding(TextBox.TextProperty, binding);
                        stackPanel.Children.Add(textBoxDesiredValue);

                        TextBox textBoxDisplayValue = new TextBox();
                        textBoxDisplayValue.VerticalAlignment = VerticalAlignment.Center;
                        textBoxDisplayValue.Margin = new Thickness(1);
                        binding = new Binding();
                        binding.Source = _dataGridColumn;
                        binding.Path = new PropertyPath("Width.DisplayValue");
                        binding.Mode = BindingMode.OneWay;
                        binding.Converter = _universalConverter;
                        textBoxDisplayValue.SetBinding(TextBox.TextProperty, binding);
                        stackPanel.Children.Add(textBoxDisplayValue);

                        if (e.PropertyInfo.CanWrite && e.PropertyInfo.SetMethod.IsPublic)
                        {
                            Button btnSetWidth = new Button();
                            btnSetWidth.Content = "Set " + e.PropertyInfo.Name;
                            btnSetWidth.HorizontalAlignment = HorizontalAlignment.Stretch;
                            btnSetWidth.Margin = new Thickness(1);
                            btnSetWidth.Tag = e.PropertyInfo;
                            btnSetWidth.Click += BtnSetWidth_Click;
                            stackPanel.Children.Add(btnSetWidth);
                        }

                        e.PropertyControl = stackPanel;
                        break;
                    }
            }
        }

        private void Epc_EntityPropertyControlGenerated(object sender, EntityPropertyControlGeneratedEventArgs e)
        {
            if (e.PropertyName == "Header")
            {
                TextBox textBox = e.PropertyControl as TextBox;
                if (textBox != null)
                {
                    textBox.TextChanged += HeaderTextBox_TextChanged;
                }
            }
        }

        private void HeaderTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            this.HeaderChanging?.Invoke(this, new HeaderChangingEventArgs((sender as TextBox).Text));
        }

        private void BtnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Button btnSetWidth = sender as Button;
                StackPanel stackPanel = btnSetWidth.Parent as StackPanel;

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

                _dataGridColumn.Width = new DataGridLength(value, unitType, desiredValue, displayValue);
            }
            catch (FormatException)
            {
            }
        }
    }

    public class HeaderChangingEventArgs : EventArgs
    {
        internal HeaderChangingEventArgs(object newHeader)
        {
            this.NewHeader = newHeader;
        }

        public object NewHeader
        {
            get;
            private set;
        }
    }
}