// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Reflection;
using Windows.UI;
using Windows.UI.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

namespace MUXControlsTestApp.Utilities
{
    public partial class EntityPropertiesControl : UserControl
    {
        private object _entity = null;
        private string _filter = string.Empty;
        private int _level = 1;
        private int _maxLevel = 4;
        private static UniversalConverter s_universalConverter = new UniversalConverter();
        private static SolidColorBrushConverter s_solidColorBrushConverter = new SolidColorBrushConverter();
        private static EnumConverter s_enumConverter = new EnumConverter();

        public event EventHandler<EntityPropertyControlNeededEventArgs> EntityPropertyControlNeeded;
        public event EventHandler<EntityPropertyControlGeneratedEventArgs> EntityPropertyControlGenerated;
        public event EventHandler<EntityPropertyControlDiscardedEventArgs> EntityPropertyControlDiscarded;

        public EntityPropertiesControl()
        {
            this.InitializeComponent();
            this.ShowLevelSeparators = true;
            this.ShowPropertySeparators = true;

            Loaded += EntityPropertiesControl_Loaded;
        }

        public object Entity
        {
            get
            {
                return _entity;
            }

            set
            {
                if (_entity != value)
                {
                    GeneratingUI();
                    _entity = value;
                    GenerateUI();
                }
            }
        }

        public string Filter
        {
            get
            {
                return _filter;
            }

            set
            {
                if (_filter != value)
                {
                    GeneratingUI();
                    _filter = value;
                    GenerateUI();
                    if (txtFilter != null)
                        txtFilter.Text = _filter;
                }
            }
        }

        public int Level
        {
            get
            {
                return _level;
            }

            set
            {
                if (_level != value)
                {
                    if (value < 1 || value > _maxLevel)
                        throw new ArgumentOutOfRangeException();

                    GeneratingUI();
                    _level = value;
                    GenerateUI();
                    if (cmbLevel != null && _level <= cmbLevel.Items.Count)
                        cmbLevel.SelectedIndex = _level - 1;
                }
            }
        }

        public int MaxLevel
        {
            get
            {
                return _maxLevel;
            }
            
            set
            {
                if (_maxLevel != value)
                {
                    if (value < 1)
                        throw new ArgumentOutOfRangeException();

                    if (_level < value)
                    {
                        this.Level = value;
                    }

                    _maxLevel = value;

                    PopulateLevelsComboBox();
                }
            }
        }

        public bool ShowLevelSeparators
        {
            get;
            set;
        }

        public bool ShowPropertySeparators
        {
            get;
            set;
        }

        protected virtual void GenerateUI(PropertyInfo propertyInfo, out FrameworkElement propertyControl, out Type pretendType, out IValueConverter converter, out object converterParameter)
        {
            propertyControl = null;
            pretendType = null;
            converter = null;
            converterParameter = null;
        }

        private void GeneratingUI()
        {
            if (this.grdProperties != null && this.grdProperties.Children.Count > 0)
            {
                if (this.EntityPropertyControlDiscarded != null)
                {
                    int index = 0;
                    foreach (FrameworkElement child in this.grdProperties.Children)
                    {
                        if (Grid.GetColumnSpan(child) == 1 && Grid.GetColumn(child) == 0)
                        {
                            FrameworkElement nextChild = this.grdProperties.Children[index + 1] as FrameworkElement;
                            if (Grid.GetColumnSpan(nextChild) == 1 && Grid.GetColumn(nextChild) == 1)
                            {
                                EntityPropertyControlDiscardedEventArgs entityPropertyControlDiscardedEventArgs = 
                                    new EntityPropertyControlDiscardedEventArgs((child as TextBlock).Text.TrimEnd(':'), nextChild);
                                this.EntityPropertyControlDiscarded(this, entityPropertyControlDiscardedEventArgs);
                            }
                        }
                    }
                    index++;
                }

                this.grdProperties.Children.Clear();
                this.grdProperties.RowDefinitions.Clear();
            }
        }

        private void GenerateUI()
        {
            if (_entity == null)
                return;

            Type typeEntity = _entity.GetType();
            int rowIndex = 0;

            for (int level = _level; level > 0; level--)
            {
                RowDefinition rowDefinition = null;
                TextBlock label = null;

                if (_level > 1 && this.ShowLevelSeparators)
                {
                    rowDefinition = new RowDefinition();
                    rowDefinition.Height = GridLength.Auto;
                    this.grdProperties.RowDefinitions.Add(rowDefinition);
                    label = new TextBlock();
                    label.VerticalAlignment = VerticalAlignment.Center;
                    label.Text = typeEntity.Name;
                    label.Foreground = new SolidColorBrush(Colors.LightGray);
                    label.FontWeight = new FontWeight { Weight = 800 };
                    label.Margin = new Thickness(2);
                    Grid.SetRow(label, rowIndex);
                    Grid.SetColumnSpan(label, 2);
                    this.grdProperties.Children.Add(label);
                    rowIndex++;

                    rowDefinition = new RowDefinition();
                    rowDefinition.Height = GridLength.Auto;
                    this.grdProperties.RowDefinitions.Add(rowDefinition);
                    Rectangle levelSeparator = new Rectangle();
                    levelSeparator.Height = 4;
                    levelSeparator.Fill = new SolidColorBrush(Colors.LightGray);
                    levelSeparator.Margin = new Thickness(0, 2, 0, 2);
                    Grid.SetRow(levelSeparator, rowIndex);
                    Grid.SetColumnSpan(levelSeparator, 2);
                    this.grdProperties.Children.Add(levelSeparator);
                    rowIndex++;
                }

#if ARM64 // CS7069: Reference to type 'BindingFlags' claims it is defined in 'System.Reflection', but it could not be found                    
                throw new NotImplementedException();
#else
                foreach (PropertyInfo propertyInfo in typeEntity.GetProperties(BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public))
                {
                    bool skipUIAutoGeneration = !string.IsNullOrWhiteSpace(this.Filter) && !propertyInfo.Name.ToLower().Contains(this.Filter.ToLower());
                    FrameworkElement propertyControl = null;

                    if (this.EntityPropertyControlNeeded != null)
                    {
                        EntityPropertyControlNeededEventArgs entityPropertyControlNeededEventArgs = new EntityPropertyControlNeededEventArgs(level, propertyInfo, skipUIAutoGeneration);
                        this.EntityPropertyControlNeeded(this, entityPropertyControlNeededEventArgs);
                        skipUIAutoGeneration = entityPropertyControlNeededEventArgs.SkipUIAutoGeneration;
                        propertyControl = entityPropertyControlNeededEventArgs.PropertyControl;
                    }

                    if (!skipUIAutoGeneration)
                    {
                        rowDefinition = new RowDefinition();
                        rowDefinition.Height = GridLength.Auto;
                        this.grdProperties.RowDefinitions.Add(rowDefinition);
                        label = new TextBlock();
                        label.VerticalAlignment = VerticalAlignment.Center;
                        label.Text = propertyInfo.Name + ":";
                        Grid.SetRow(label, rowIndex);
                        this.grdProperties.Children.Add(label);

                        if (propertyControl == null)
                        {
                            Type pretendType;
                            IValueConverter converter;
                            object converterParameter;

                            GenerateUI(propertyInfo, out propertyControl, out pretendType, out converter, out converterParameter);

                            if (propertyControl == null)
                            {
                                Type type = pretendType != null ? pretendType : propertyInfo.PropertyType;
                                Binding binding = new Binding();

                                if (converter != null)
                                {
                                    binding.Converter = converter;
                                }

                                if (converterParameter != null)
                                {
                                    binding.ConverterParameter = converterParameter;
                                }

                                try
                                {
                                    switch (type.FullName)
                                    {
                                        case "System.Boolean":
                                            {
                                                propertyControl = GenerateBooleanUI(propertyInfo, binding);
                                                break;
                                            }
                                        case "System.String":
                                        case "System.Int32":
                                        case "System.Single":
                                        case "System.Double":
                                        case "System.Object": // SelectedItem / Tag / Header
                                        case "System.Numerics.Vector2":
                                        case "System.Numerics.Vector3":
                                        case "Windows.Foundation.Point":
                                        case "Windows.Foundation.Size":
                                        case "Windows.UI.Text.FontWeight":
                                        case "Windows.UI.Xaml.Thickness":
                                        case "Windows.UI.Xaml.Media.FontFamily":
                                            {
                                                propertyControl = GenerateStringUI(propertyInfo, binding, type);
                                                break;
                                            }
                                        case "Windows.UI.Xaml.Media.Brush":
                                            {
                                                propertyControl = GenerateBrushUI(propertyInfo, binding);
                                                break;
                                            }
                                        case "Windows.UI.Xaml.Data.Binding":
                                            {
                                                propertyControl = GenerateBindingUI(propertyInfo, binding);
                                                break;
                                            }
                                        default:
                                            Type propertyType = propertyInfo.PropertyType;
                                            bool isNullableType = false;
                                            if (IsNullableType(propertyType))
                                            {
                                                isNullableType = true;
                                                propertyType = propertyType.GetGenericArguments()[0];
                                            }

                                            if (propertyType.GetTypeInfo().IsEnum)
                                            {
                                                // Examples: "Windows.UI.Text.FontStyle" / "Windows.UI.Xaml.Controls.ScrollBarVisibility"
                                                propertyControl = GenerateEnumUI(propertyInfo, binding, propertyType, isNullableType);
                                            }
                                            else
                                            {
                                                Debug.WriteLine("Unhandled type: " + propertyType.FullName);
                                            }
                                            break;
                                    }
                                }
                                catch (Exception ex)
                                {
                                    Debug.WriteLine("Exception caught: " + ex.ToString());
                                    Debug.WriteLine("type.FullName: " + type.FullName);
                                    Debug.WriteLine("propertyInfo: " + propertyInfo.ToString());
                                    if (!(ex is TargetInvocationException))
                                    {
                                        throw;
                                    }
                                }
                            }
                        }

                        if (propertyControl != null)
                        {
                            if (string.IsNullOrWhiteSpace(propertyControl.Name))
                            {
                                propertyControl.Name = "pc" + propertyInfo.Name;
                            }
                            if (string.IsNullOrWhiteSpace(AutomationProperties.GetName(propertyControl)))
                            {
                                AutomationProperties.SetName(propertyControl, propertyControl.Name);
                            }
                            Grid.SetRow(propertyControl, rowIndex);
                            Grid.SetColumn(propertyControl, 1);
                            this.grdProperties.Children.Add(propertyControl);

                            // Debug.WriteLine("Generated property: " + propertyInfo.ToString());

                            if (this.EntityPropertyControlGenerated != null)
                            {
                                EntityPropertyControlGeneratedEventArgs entityPropertyControlGeneratedEventArgs = new EntityPropertyControlGeneratedEventArgs(
                                    level, propertyInfo.Name, propertyInfo.PropertyType, propertyControl);
                                this.EntityPropertyControlGenerated(this, entityPropertyControlGeneratedEventArgs);
                            }
                        }

                        rowIndex++;

                        if (this.ShowPropertySeparators)
                        {
                            rowDefinition = new RowDefinition();
                            rowDefinition.Height = GridLength.Auto;
                            this.grdProperties.RowDefinitions.Add(rowDefinition);

                            Rectangle propertySeparator = new Rectangle();
                            propertySeparator.Height = 1;
                            propertySeparator.Fill = new SolidColorBrush(Colors.LightGray);
                            Grid.SetRow(propertySeparator, rowIndex);
                            Grid.SetColumnSpan(propertySeparator, 2);
                            this.grdProperties.Children.Add(propertySeparator);
                            rowIndex++;
                        }
                    }
                }
#endif

                typeEntity = typeEntity.GetTypeInfo().BaseType;
            }
        }

        private FrameworkElement GenerateBooleanUI(PropertyInfo propertyInfo, Binding binding)
        {
            CheckBox checkBox = new CheckBox();
            checkBox.VerticalAlignment = VerticalAlignment.Center;
            checkBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            checkBox.Margin = new Thickness(1);
            checkBox.IsEnabled = propertyInfo.CanWrite && propertyInfo.SetMethod.IsPublic;
            binding.Source = _entity;
            binding.Path = new PropertyPath(propertyInfo.Name);
            binding.Mode = checkBox.IsEnabled ? BindingMode.TwoWay : BindingMode.OneWay;
            checkBox.SetBinding(CheckBox.IsCheckedProperty, binding);
            return checkBox;
        }

        private FrameworkElement GenerateStringUI(PropertyInfo propertyInfo, Binding binding, Type type)
        {
            TextBox textBox = new TextBox();
            textBox.VerticalAlignment = VerticalAlignment.Center;
            textBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            textBox.Margin = new Thickness(1);
            textBox.IsEnabled = propertyInfo.CanWrite && propertyInfo.SetMethod.IsPublic;
            if (type.FullName == "System.Double" || type.FullName == "System.Single")
            {
                textBox.MinWidth = 170.0;
            }
            binding.Source = _entity;
            binding.Path = new PropertyPath(propertyInfo.Name);
            binding.Mode = textBox.IsEnabled ? BindingMode.TwoWay : BindingMode.OneWay;
            if (binding.Converter == null)
            {
                binding.Converter = s_universalConverter;
            }
            textBox.SetBinding(TextBox.TextProperty, binding);
            return textBox;
        }

        private FrameworkElement GenerateBrushUI(PropertyInfo propertyInfo, Binding binding)
        {
            ComboBox comboBox = new ComboBox();
            comboBox.VerticalAlignment = VerticalAlignment.Center;
            comboBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            comboBox.Margin = new Thickness(1);
            comboBox.Items.Add("Null");

#if ARM64 // CS7069: Reference to type 'BindingFlags' claims it is defined in 'System.Reflection', but it could not be found                    
            throw new NotImplementedException();
#else
            foreach (PropertyInfo colorPropertyInfo in typeof(Colors).GetProperties(BindingFlags.DeclaredOnly | BindingFlags.Static | BindingFlags.Public))
            {
                comboBox.Items.Add(colorPropertyInfo.Name);
            }
#endif
            SolidColorBrush currentBrush = propertyInfo.GetValue(_entity) as SolidColorBrush;
            if (currentBrush != null)
            {
                Color currentColor = currentBrush.Color;
                bool isKnownColor = false;

#if ARM64 // CS7069: Reference to type 'BindingFlags' claims it is defined in 'System.Reflection', but it could not be found                    
                throw new NotImplementedException();
#else
                foreach (PropertyInfo colorPropertyInfo in typeof(Colors).GetProperties(BindingFlags.DeclaredOnly | BindingFlags.Static | BindingFlags.Public))
                {
                    if (colorPropertyInfo.GetValue(null).ToString() == currentColor.ToString())
                    {
                        isKnownColor = true;
                        break;
                    }
                }
#endif

                if (!isKnownColor)
                {
                    comboBox.Items.Add(currentColor.ToString());
                }
            }
            comboBox.IsEnabled = propertyInfo.CanWrite && propertyInfo.SetMethod.IsPublic;
            binding.Source = _entity;
            binding.Path = new PropertyPath(propertyInfo.Name);
            binding.Mode = comboBox.IsEnabled ? BindingMode.TwoWay : BindingMode.OneWay;
            if (binding.Converter == null)
            {
                binding.Converter = s_solidColorBrushConverter;
            }
            comboBox.SetBinding(ComboBox.SelectedValueProperty, binding);
            return comboBox;
        }

        private FrameworkElement GenerateBindingUI(PropertyInfo propertyInfo, Binding binding)
        {
            StackPanel stackPanel = new StackPanel();
            ComboBox comboBoxMode = new ComboBox();
            comboBoxMode.HorizontalAlignment = HorizontalAlignment.Stretch;
            comboBoxMode.Margin = new Thickness(1);
            comboBoxMode.Items.Add("OneTime");
            comboBoxMode.Items.Add("OneWay");
            comboBoxMode.Items.Add("TwoWay");
            comboBoxMode.SelectedIndex = 0;
            stackPanel.Children.Add(comboBoxMode);

            TextBox textBoxPath = new TextBox();
            textBoxPath.HorizontalAlignment = HorizontalAlignment.Stretch;
            textBoxPath.Margin = new Thickness(1);
            binding.Source = _entity;
            binding.Path = new PropertyPath("Binding.Path");
            binding.Mode = BindingMode.OneWay;
            textBoxPath.SetBinding(TextBox.TextProperty, binding);
            stackPanel.Children.Add(textBoxPath);

            if (propertyInfo.CanWrite && propertyInfo.SetMethod.IsPublic)
            {
                Button btnSetBinding = new Button();
                btnSetBinding.Content = "Set " + propertyInfo.Name;
                btnSetBinding.HorizontalAlignment = HorizontalAlignment.Stretch;
                btnSetBinding.Margin = new Thickness(1);
                btnSetBinding.Tag = propertyInfo;
                btnSetBinding.Click += BtnSetBinding_Click;
                stackPanel.Children.Add(btnSetBinding);
            }
            return stackPanel;
        }

        private FrameworkElement GenerateEnumUI(PropertyInfo propertyInfo, Binding binding, Type type, bool isNullableType)
        {
            ComboBox comboBox = new ComboBox();
            comboBox.VerticalAlignment = VerticalAlignment.Center;
            comboBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            comboBox.Margin = new Thickness(1);
            if (isNullableType)
            {
                comboBox.Items.Add("Null");
            }
#if ARM64 // CS7069: Reference to type 'BindingFlags' claims it is defined in 'System.Reflection', but it could not be found                    
            throw new NotImplementedException();
#else
            foreach (FieldInfo fieldInfo in type.GetFields(BindingFlags.Public | BindingFlags.Static))
            {
                comboBox.Items.Add(fieldInfo.Name);
            }
#endif
            comboBox.IsEnabled = propertyInfo.CanWrite && propertyInfo.SetMethod.IsPublic;
            binding.Source = _entity;
            binding.Path = new PropertyPath(propertyInfo.Name);
            binding.Mode = comboBox.IsEnabled ? BindingMode.TwoWay : BindingMode.OneWay;
            if (binding.Converter == null)
            {
                binding.Converter = s_enumConverter;
            }
            if (binding.ConverterParameter == null)
            {
                binding.ConverterParameter = type;
            }
            comboBox.SetBinding(ComboBox.SelectedValueProperty, binding);
            return comboBox;
        }

        private void BtnSetBinding_Click(object sender, RoutedEventArgs e)
        {
            Button btnSetBinding = sender as Button;
            PropertyInfo propertyInfo = btnSetBinding.Tag as PropertyInfo;
            StackPanel stackPanel = btnSetBinding.Parent as StackPanel;
            ComboBox comboBoxMode = stackPanel.Children[0] as ComboBox;
            TextBox textBoxPath = stackPanel.Children[1] as TextBox;
            Binding binding = new Binding();
            binding.Path = new PropertyPath(textBoxPath.Text);

            switch (comboBoxMode.SelectedIndex)
            {
                case 0:
                    binding.Mode = BindingMode.OneTime;
                    break;
                case 1:
                    binding.Mode = BindingMode.OneWay;
                    break;
                case 2:
                    binding.Mode = BindingMode.TwoWay;
                    break;
            }

            propertyInfo.SetValue(_entity, binding);
        }

        private void cmbLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            this.Level = cmbLevel.SelectedIndex + 1;
        }

        private void btnApplyFilter_Click(object sender, RoutedEventArgs e)
        {
            this.Filter = txtFilter.Text;
        }

        private void EntityPropertiesControl_Loaded(object sender, RoutedEventArgs e)
        {
            PopulateLevelsComboBox();
            if (cmbLevel != null && _level <= cmbLevel.Items.Count)
                cmbLevel.SelectedIndex = _level - 1;
        }

        private void PopulateLevelsComboBox()
        {
            if (cmbLevel != null)
            {
                cmbLevel.Items.Clear();
                for (int level = 1; level <= _maxLevel; level++)
                {
                    cmbLevel.Items.Add(level);
                }
            }
        }

        private static bool IsNullableType(Type type)
        {
            return type != null && type.GetTypeInfo().IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>);
        }
    }
}
