// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using BindTestbedModel;
using Windows.UI.Popups;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;


// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    internal sealed partial class EventTests : UserControl
    {
        public DataModel Model { get; set; }
        public DOModel DOModel { get; set; }
        public LanguageSpecific LanguageModel { get; set; }
        public RoutedEventHandler clickDelegate;

        public EventTests()
        {
            this.InitializeComponent();
            clickDelegate = this.Click_RegularArgs;
            DetectLeaksPage.TrackObject(this);
            this.Loaded += EventTests_Loaded;
        }

        private void EventTests_Loaded(object sender, RoutedEventArgs e)
        {
            DetectLeaksPage.TrackBindingObject<EventTests>(this.Bindings);
            this.Loaded -= EventTests_Loaded;
        }

        private void Click_RegularArgs(object sender, RoutedEventArgs e)
        {
            MessageDialog dlg = new MessageDialog("Clicked - RegularArgs");
            var t = dlg.ShowAsync();
        }

        private void Click_NoArgs()
        {
            MessageDialog dlg = new MessageDialog("Clicked - No args");
            var t = dlg.ShowAsync();
        }

        private void Click_BaseArgs(object sender, object e)
        {
            MessageDialog dlg = new MessageDialog("Clicked - Base args");
            var t = dlg.ShowAsync();
        }

        private void Click_Overloaded()
        {
            MessageDialog dlg = new MessageDialog("Clicked - Overloaded");
            var t = dlg.ShowAsync();
        }
    }
}