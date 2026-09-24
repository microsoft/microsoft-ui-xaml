// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using ConditionalControls;
using System.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;


namespace Conditionals
{
    public sealed partial class BindTests : UserControl, Microsoft.UI.Xaml.Data.INotifyPropertyChanged
    {
        public Model Model = new Model();

        public BindTests()
        {
            this.InitializeComponent();
        }

        public event Microsoft.UI.Xaml.Data.PropertyChangedEventHandler PropertyChanged;

        public string V2Property
        {
            get
            {
                return aButton.V2Property;
            }
            set
            {
                if (aButton.V2Property != value)
                {
                    aButton.V2Property = value;
                    NotifyPropertyChanged("V2Property");
                }
            }
        }

        public string V3Property
        {
            get
            {
                return aButton.V3Property;
            }
            set
            {
                if (aButton.V3Property != value)
                {
                    aButton.V3Property = value;
                    NotifyPropertyChanged("V3Property");
                }
            }
        }

        public void Click_V2(object sender, RoutedEventArgs e)
        {
            IVersionedProperties obj = sender as IVersionedProperties;
            clickV2results.Text = obj.V2Property;
        }

        public void Click_V3(object sender, RoutedEventArgs e)
        {
            IVersionedProperties obj = sender as IVersionedProperties;
            clickV3results.Text = obj.V3Property;
        }

        private void NotifyPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new Microsoft.UI.Xaml.Data.PropertyChangedEventArgs(propertyName));
        }
    }
}
