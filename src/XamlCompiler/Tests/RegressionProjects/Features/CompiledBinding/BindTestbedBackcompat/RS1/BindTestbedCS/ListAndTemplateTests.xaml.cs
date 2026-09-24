// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    internal sealed partial class ListAndTemplateTests : UserControl
    {
        public BindTestbedModel.DataModel Model { get; set; }
        public BindTestbedModel.DOModel DOModel { get; set; }
        public BindTestbedCXModel.ModelCX ModelCX { get; set; }

        public LanguageSpecific LanguageModel { get; set; }

        public Object SomeButtonContent
        {
            set
            {
                this.someButton.Content = value;
            }
        }

        public ListAndTemplateTests()
        {
            this.InitializeComponent();
            DetectLeaksPage.TrackObject(this);
        }

        private void UpdateValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.UpdateValues();
            this.DOModel.UpdateValues();
            this.LanguageModel.UpdateValues();
            this.ModelCX.UpdateValues();
        }

        private void ResetValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.InitializeValues();
            this.DOModel.UpdateValues();
            this.LanguageModel.InitializeValues();
            this.ModelCX.InitializeValues();
        }
    }
}