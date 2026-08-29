// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace Simple
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    [CLSCompliant(false)]
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            this.RunTestScenario(e.Parameter as string);
        }

        private void ClickHandler(object sender, RoutedEventArgs e) { }

        private void TappedHandler(object sender, TappedRoutedEventArgs e) { }

        #region RuntimeTestCode
        async void RunTestScenario(string navigationParameter)
        {
            // First allow the UI to complete rendering the page and 
            // the test process to open this process and retreives a handle.
            await this.ReleaseUIThreadForFewSeconds();

            // Execute the validation. Note that we may be running on a worker thread
            // here, which cannot access UI Elements, so use the dispatcher to execute the 
            // validation on the UI Thread.

            await this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {

                // We are called normally, no explicit background value to check.
                if (string.IsNullOrEmpty(navigationParameter))
                {
                    // Grid color should be white normally.
                    this.HardAssert(((SolidColorBrush)(this.MainGrid.Background)).Color.Equals(Color.FromArgb(255, 255, 255, 255)));
                }
                else if (navigationParameter == "IncrementalTest")
                {
                    // Grid color should be black now.
                    this.HardAssert(((SolidColorBrush)(this.MainGrid.Background)).Color.Equals(Color.FromArgb(255, 0, 0, 0)));
                }

                // Exit the app
                Application.Current.Exit();
            });

        }

        private async Task ReleaseUIThreadForFewSeconds()
        {
            await Task.Delay(TimeSpan.FromSeconds(5));
        }

        private void HardAssert(bool expression)
        {
            if (!expression)
            {
                throw new InvalidOperationException();
            }
        }

        #endregion
    }

    public class PropertyBag
    {
        public String StringProp { get; set; }
        public Int32 IntProp { get; set; }
        public Double DoubleProp { get; set; }
        public Boolean BooleanProp { get; set; }
    }
}
