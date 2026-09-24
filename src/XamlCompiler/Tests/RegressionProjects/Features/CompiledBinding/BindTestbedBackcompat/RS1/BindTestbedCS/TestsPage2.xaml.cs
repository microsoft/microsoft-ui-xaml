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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    internal sealed partial class TestsPage2 : UserControl
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
        private static readonly DependencyProperty _DPOnPage = DependencyProperty.Register(DPOnPageName, typeof(int), typeof(DataModel), new PropertyMetadata(0));

        public BindTestbedModel.IEmployee NullEmployee;
        public String NullStringProperty;

        public string ImageUriString = "http://static-hp-wus.s-msn.com/sc/homepage/i/65/e8a77758e8644573ba5d41ada16e8c.jpg";

        //Tests for private fields to the page
        private string PrivateStringField = "Private string Field";
        private int PrivateIntField = 2;


        public IEmployee Person
        {
            get { return (IEmployee)GetValue(PersonProperty); }
            set { SetValue(PersonProperty, value); }
        }

        // Using a DependencyProperty as the backing store for Person.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty PersonProperty =
            DependencyProperty.Register("Person", typeof(IEmployee), typeof(TestsPage2), new PropertyMetadata(null));

        public ObservableCollection<String> a;

        public IManager NiceManager { get { return Model.Employees[0].DirectManager; } }

        public TestsPage2()
        {
            a = new ObservableCollection<string>();
            this.InitializeComponent();
            InitializeValues();
            a.Add("A");
            a.Add("A");
            a.Add("A");
            this.NullEmployee = null;
            this.NullStringProperty = null;
            DetectLeaksPage.TrackObject(this);
            this.Loaded += TestsPage2_Loaded;
        }

        private void TestsPage2_Loaded(object sender, RoutedEventArgs e)
        {
            DetectLeaksPage.TrackBindingObject<TestsPage2>(this.Bindings);
            this.Loaded -= TestsPage2_Loaded;
        }

        void InitializeValues()
        {
            this.PrivateIntField = 100;
            this.PrivateStringField = "Hello Dave.";
            this.DPOnPage = "DP on page";
        }

        private void UpdateValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.UpdateValues();
            this.DOModel.UpdateValues();
            this.LanguageModel.UpdateValues();
            this.PrivateIntField = LanguageModel.IntField;
            this.PrivateStringField = LanguageModel.StringField;
            this.DPOnPage += "-";
            if (Model.IntPropWithINPC % 5 == 0)
            {
                Grid.SetColumn(BisqueRectangle, 1);
            }
        }

        private void ResetValuesClick(object sender, RoutedEventArgs e)
        {
            this.Model.InitializeValues();
            this.DOModel.UpdateValues();
            this.LanguageModel.InitializeValues();
            this.InitializeValues();
        }

        private void UndeferElementClick(object sender, RoutedEventArgs e)
        {
            this.FindName("deferedTextBlock");
        }
    }
}