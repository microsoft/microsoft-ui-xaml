// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using BindTestbedModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    internal sealed partial class FunctionTests : UserControl
    {
        public DataModel Model { get; set; }
        public DOModel DOModel { get; set; }
        public LanguageSpecific LanguageModel { get; set; }

        public FunctionTests()
        {
            this.InitializeComponent();
            DetectLeaksPage.TrackObject(this);
        }

        public string FunctionOnRootNoArgs()
        {
            return "FunctionOnRootNoArgs";
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

        private void StopTrackingClick(object sender, RoutedEventArgs e)
        {
            Bindings.StopTracking();
        }

        private void ReInitializeBindingsClick(object sender, RoutedEventArgs e)
        {
            Bindings.Initialize();
        }
    }
}