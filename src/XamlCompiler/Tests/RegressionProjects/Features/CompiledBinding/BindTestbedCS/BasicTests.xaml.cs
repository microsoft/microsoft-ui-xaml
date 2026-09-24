// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using BindTestbedModel;
using System;
using System.Diagnostics;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    [Windows.Foundation.Metadata.CreateFromString(MethodName = "BindTestbed.BasicTests.MakeNewInstance")]
    internal sealed partial class BasicTests : UserControl
    {
        public int MyInt { get; set; }
        public DataModel Model { get; set; }
        public DOModel DOModel { get; set; }

        public LanguageSpecific LanguageModel { get; set; }

        public String DPOnPage
        {
            get { return (String)GetValue(DPOnPageProperty); }
            set { SetValue(DPOnPageProperty, value); }
        }
        public static DependencyProperty DPOnPageProperty { get { return _DPOnPage; } }
        private const string DPOnPageName = "DPOnPage";
        private static readonly DependencyProperty _DPOnPage =
        DependencyProperty.Register(DPOnPageName, typeof(int), typeof(DataModel), new PropertyMetadata(0));

        public BindTestbedModel.IEmployee NullEmployee1;
        public String NullStringProperty;

        //Tests for private fields to the page
        private string PrivateStringField = "Private string Field";
        private int PrivateIntField = 2;

        public String NonDPOnPage { get; set; }

        private String PrivateProperty { get; set; }

        public BasicTests()
        {
            this.InitializeComponent();
            InitializeValues();
            this.Loaded += BasicTests_Loaded;
            DetectLeaksPage.TrackObject(this);
            NullEmployee1 = null;
            NullStringProperty = null;
        }

        public static BasicTests MakeNewInstance(string text)
        {
            return new BasicTests();
        }

        private void BasicTests_Loaded(object sender, RoutedEventArgs e)
        {
            // Demonstrates access to the Bindings property.
            IBasicTests_Bindings bindings = Bindings;
            Debug.Assert(bindings != null);
            DetectLeaksPage.TrackBindingObject<BasicTests>(bindings);
        }

        void InitializeValues()
        {

            this.PrivateIntField = 100;
            this.PrivateStringField = "Hello Dave.";
            this.DPOnPage = "DP on page";
            this.NonDPOnPage = "Non DP on Page";
            this.PrivateProperty = "PrivateProperty";
        }

        private void UpdateValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.UpdateValues();
            this.DOModel.UpdateValues();
            this.LanguageModel.UpdateValues();
            this.PrivateIntField = LanguageModel.IntField;
            this.PrivateStringField = LanguageModel.StringField;
            this.DPOnPage += "-";
        }

        private void ResetValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.InitializeValues();
            this.DOModel.UpdateValues();
            this.LanguageModel.InitializeValues();
            this.InitializeValues();
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