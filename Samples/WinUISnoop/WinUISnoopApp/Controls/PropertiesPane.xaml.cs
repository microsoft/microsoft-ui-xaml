using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using WinUISnoopApp.SnoopData;

namespace WinUISnoopApp.Controls
{
    public sealed partial class PropertiesPane : UserControl
    {
        public static readonly DependencyProperty DisplayedPropertiesProperty =
            DependencyProperty.Register(
                nameof(DisplayedProperties),
                typeof(IEnumerable<ElementProperty>),
                typeof(PropertiesPane),
                new PropertyMetadata(null));

        public static readonly DependencyProperty SelectedPropertyProperty =
            DependencyProperty.Register(
                nameof(SelectedProperty),
                typeof(ElementProperty),
                typeof(PropertiesPane),
                new PropertyMetadata(null));

        public static readonly DependencyProperty PropertyValueTextProperty =
            DependencyProperty.Register(
                nameof(PropertyValueText),
                typeof(string),
                typeof(PropertiesPane),
                new PropertyMetadata(string.Empty));

        public static readonly DependencyProperty PropertyFilterTextProperty =
            DependencyProperty.Register(
                nameof(PropertyFilterText),
                typeof(string),
                typeof(PropertiesPane),
                new PropertyMetadata(string.Empty));

        public static readonly DependencyProperty SelectedPropertyNameProperty =
            DependencyProperty.Register(
                nameof(SelectedPropertyName),
                typeof(string),
                typeof(PropertiesPane),
                new PropertyMetadata(string.Empty));

        public event EventHandler PropertyValueSubmitted;

        public IEnumerable<ElementProperty> DisplayedProperties
        {
            get => (IEnumerable<ElementProperty>)GetValue(DisplayedPropertiesProperty);
            set => SetValue(DisplayedPropertiesProperty, value);
        }

        public ElementProperty SelectedProperty
        {
            get => (ElementProperty)GetValue(SelectedPropertyProperty);
            set => SetValue(SelectedPropertyProperty, value);
        }

        public string PropertyValueText
        {
            get => (string)GetValue(PropertyValueTextProperty);
            set => SetValue(PropertyValueTextProperty, value);
        }

        public string PropertyFilterText
        {
            get => (string)GetValue(PropertyFilterTextProperty);
            set => SetValue(PropertyFilterTextProperty, value);
        }

        public string SelectedPropertyName
        {
            get => (string)GetValue(SelectedPropertyNameProperty);
            set => SetValue(SelectedPropertyNameProperty, value);
        }

        public PropertiesPane()
        {
            this.InitializeComponent();
        }

        private void PropertyValueTextBox_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Enter)
            {
                e.Handled = true;
                PropertyValueSubmitted?.Invoke(this, EventArgs.Empty);
            }
        }
    }
}
