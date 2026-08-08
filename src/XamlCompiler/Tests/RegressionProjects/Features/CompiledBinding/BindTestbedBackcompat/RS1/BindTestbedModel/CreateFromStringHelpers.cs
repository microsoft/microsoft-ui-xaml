// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;

namespace BindTestbedModel
{
    public sealed class Circle : FrameworkElement, INotifyPropertyChanged
    {
        private static readonly DependencyProperty TestPropertyProperty = DependencyProperty.Register(
          "Diameter",
          typeof(Diameter),
          typeof(Circle),
          new PropertyMetadata(null)
        );

        private static readonly DependencyProperty TestStringPropertyProperty = DependencyProperty.Register(
          "TestStringProperty",
          typeof(string),
          typeof(Circle),
          new PropertyMetadata(null)
        );

        public Diameter Diameter
        {
            get { return (Diameter)GetValue(TestPropertyProperty); }
            set
            {
                SetValue(TestPropertyProperty, value);
                this.OnPropertyChanged("Diameter");
            }
        }

        public string TestStringProperty
        {
            get { return (string)GetValue(TestStringPropertyProperty); }
            set
            {
                SetValue(TestStringPropertyProperty, value);
                this.OnPropertyChanged("TestStringProperty");
            }
        }
        public event PropertyChangedEventHandler PropertyChanged;

        public void OnPropertyChanged(string propertyName)
        {
            // Raise the PropertyChanged event, passing the name of the property whose value has changed.
            this.PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        
    }

    [Windows.Foundation.Metadata.CreateFromString(MethodName = "BindTestbedModel.Diameter.MakeNewDiameter")]
    public sealed class Diameter : FrameworkElement
    {
        public static object MakeNewDiameter(string args)
        {
            if (args == null || args.Length == 0)
            {
                return null;
            }
            Diameter ret = new Diameter();
            int res = Int32.Parse(args);
            ret.Width = res;
            return ret;
        }
    }
}
