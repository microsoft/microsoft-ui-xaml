// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using BindTestbedModel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Foundation;
using System.Collections.ObjectModel;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    internal sealed partial class NullableTests : UserControl
    {
        public DataModel Model { get; set; }
        public DOModel DOModel { get; set; }

        public NullableTests()
        {
            this.InitializeComponent();
        }

        private void UpdateValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.UpdateValues();
            this.DOModel.UpdateValues();
        }

        private void ResetValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.InitializeValues();
            this.DOModel.UpdateValues();
        }
    }
}
