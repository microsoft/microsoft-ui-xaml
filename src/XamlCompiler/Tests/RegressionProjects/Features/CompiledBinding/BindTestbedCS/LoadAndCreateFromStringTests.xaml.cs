// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using BindTestbedModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace BindTestbed
{
    /// <summary>
    /// x:Load Tests
    /// </summary>
    [Windows.Foundation.Metadata.CreateFromString(MethodName = "BindTestbed.LoadAndCreateFromStringTests+Factory.MakeNewInstance")]
    internal sealed partial class LoadAndCreateFromStringTests : UserControl
    {
        internal class Factory
        {
            public static LoadAndCreateFromStringTests MakeNewInstance(string str)
            {
                return new LoadAndCreateFromStringTests();
            }
        }

        public DataModel Model { get; set; }
        public DOModel DOModel { get; set; }
        public LanguageSpecific LanguageModel { get; set; }

        public LoadAndCreateFromStringTests()
        {
            this.InitializeComponent();
            InitializeValues();
            DetectLeaksPage.TrackObject(this);
        }

        void InitializeValues()
        {
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
            this.InitializeValues();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            this.FindName("AnUnloadedTextBlock");
        }

        private void LoadInnerPanel_Click(object sender, RoutedEventArgs e)
        {
            this.FindName("InnerPanel");

        }

        private void UnloadInnerPanel_Click(object sender, RoutedEventArgs e)
        {
            UnloadObject(this.InnerPanel);
        }

        private void LoadOuterPanel_Click(object sender, RoutedEventArgs e)
        {
            this.FindName("OuterPanel");

        }

        private void UnloadOuterPanel_Click(object sender, RoutedEventArgs e)
        {
            UnloadObject(this.OuterPanel);
        }
    }
}