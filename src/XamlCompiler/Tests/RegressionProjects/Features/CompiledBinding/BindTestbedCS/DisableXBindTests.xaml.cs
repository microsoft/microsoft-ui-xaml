// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using BindTestbedModel;
using Windows.UI.Popups;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    internal sealed partial class DisableXBindTests : UserControl
    {
        public DataModel Model { get; set; }
        public DOModel DOModel { get; set; }
        public LanguageSpecific LanguageModel { get; set; }
        public DisableXBindTests()
        {
            this.InitializeComponent();
            DetectLeaksPage.TrackObject(this);
            this.Loaded += DisableXBindTests_Loaded;
        }

        private void DisableXBindTests_Loaded(object sender, RoutedEventArgs e)
        {
            IXamlBindScopeDiagnostics bindingsAsDiag = (IXamlBindScopeDiagnostics)this.Bindings;
            for (int lineNumber = 0; lineNumber < 40; lineNumber++)
            {
                for (int columnNumber = 0; columnNumber < 100; columnNumber++)
                {
                    bindingsAsDiag.Disable(lineNumber, columnNumber);
                }
            }
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
    }
}
