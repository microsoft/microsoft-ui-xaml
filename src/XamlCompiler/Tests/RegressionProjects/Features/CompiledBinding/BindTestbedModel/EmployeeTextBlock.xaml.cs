// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace BindTestbedModel
{
    public sealed partial class EmployeeTextBlock : UserControl
    {
        private static readonly DependencyProperty employee = DependencyProperty.Register(
            "Employee", typeof(IEmployee), typeof(EmployeeTextBlock), new PropertyMetadata(0));
        public static DependencyProperty EmployeeProperty { get { return employee; } }

        public IEmployee Employee
        {
            get { return GetValue(EmployeeProperty) as IEmployee; }
            set { SetValue(EmployeeProperty, value); }
        }

        public EmployeeTextBlock()
        {
            this.InitializeComponent();
        }
    }
}
