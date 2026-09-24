// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using BindTestbedModel;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

namespace BindTestbed
{
    internal sealed partial class TwoWayTests : UserControl
    {
        public DataModel Model { get; set; }
        public DOModel DOModel { get; set; }
        public LanguageSpecific LanguageModel { get; set; }

        public TwoWayTests()
        {
            this.InitializeComponent();
            DetectLeaksPage.TrackObject(this);
            this.Loaded += TwoWayTests_Loaded;
        }

        private void TwoWayTests_Loaded(object sender, RoutedEventArgs e)
        {
            DetectLeaksPage.TrackBindingObject<TwoWayTests>(this.Bindings);
            this.Loaded -= TwoWayTests_Loaded;
        }

        private void UpdateValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.UpdateValues();
            this.DOModel.UpdateValues();
            this.LanguageModel.UpdateValues();
        }

        private void ResetValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.InitializeValues();
            this.DOModel.UpdateValues();
            this.LanguageModel.InitializeValues();
        }
    }
}