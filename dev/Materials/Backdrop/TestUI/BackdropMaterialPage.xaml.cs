// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Automation;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml.Controls;
using System.Text;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "BackdropMaterial")]
    public sealed partial class BackdropMaterialPage : TestPage
    {
        public BackdropMaterialPage()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            SetupBackdrop(true);
        }

        private void SetupBackdrop(bool on)
        {
            var testFrame = this.Frame as TestFrame;
            var current = BackdropMaterial.GetApplyToRootOrPageBackground(testFrame);
            if (current != on)
            {
                if (on)
                {
                    testFrame.SetValue(BackdropMaterial.ApplyToRootOrPageBackgroundProperty, true);
                }
                else
                {
                    // Undo the material and set up a binding to put the background property back to its theme-aware value.
                    testFrame.SetValue(BackdropMaterial.ApplyToRootOrPageBackgroundProperty, false);
                    testFrame.SetBinding(Control.BackgroundProperty,
                        new Windows.UI.Xaml.Data.Binding { Source = testFrame.BackupThemeBackground, Path = new PropertyPath("Background") });
                }
            }
        }

        protected override void OnNavigatingFrom(NavigatingCancelEventArgs e)
        {
            base.OnNavigatingFrom(e);

            SetupBackdrop(false);
        }

        private void EnableBackdrop(object sender, RoutedEventArgs e)
        {
            SetupBackdrop(true);
        }
        private void DisableBackdrop(object sender, RoutedEventArgs e)
        {
            SetupBackdrop(false);
        }

        private void ReportBrushes(object sender, RoutedEventArgs e)
        {
            // Get the brush from the frame
            var testFrame = this.Frame as TestFrame;
            var backdropBrush = (object)Window.Current as Windows.UI.Composition.ICompositionSupportsSystemBackdrop;

            var frameBackground = testFrame.Background as SolidColorBrush; ;

            var sb = new StringBuilder();
            sb.AppendLine($"TestFrame.Background = {frameBackground?.Color.ToString() ?? "null"}");
            sb.AppendLine($"Window.Backdrop = {backdropBrush?.SystemBackdrop?.ToString() ?? "null"}");
            TestOutput.Text = sb.ToString();
        }
    }
}
