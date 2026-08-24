// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    internal sealed partial class MainPage : Page
    {
        public BindTestbedModel.DataModel Model { get; set; }
        public BindTestbedModel.DOModel DOModel { get; set; }
        public LanguageSpecific LanguageModel { get; set; }
        //// TODO: Convert BindTestbedModelCX to C++/WinRT
        //public BindTestbedCXModel.ModelCX ModelCX { get; set; }

        public MainPage()
        {
            this.DataContext = this;

            this.Model = App.Model;
            this.DOModel = App.DOModel;
            this.LanguageModel = App.LanguageModel;
            // TODO: Convert BindTestbedModelCX to C++/WinRT
            //this.ModelCX = App.ModelCX;

            this.InitializeComponent();
            InitializeValues();
            DetectLeaksPage.TrackObject(this);
        }

        void InitializeValues()
        {
            this.Model.InitializeValues();
            this.DOModel.UpdateValues();
            this.LanguageModel.InitializeValues();
            // TODO: Convert BindTestbedModelCX to C++/WinRT
            //this.ModelCX.InitializeValues();
        }

        private void DetectLeaks_Click(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(DetectLeaksPage));
        }
    }
}