// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.IO;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using SelectionModelSampleApp.Pages;

namespace SelectionModelSampleApp
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();
            this.AppWindow.Resize(new Windows.Graphics.SizeInt32(1920, 1200));
            NavView.Loaded += (s, e) => NavView.SelectedItem = FindNavItem(StartupTag) ?? NavView.MenuItems[0];
        }

        /// <summary>
        /// Optional "&lt;PageTag&gt;:&lt;scenario&gt;" command line argument used to capture
        /// documentation screenshots in a reproducible state.
        /// </summary>
        public static string StartupTag { get; set; }

        public static string StartupScenario { get; set; }

        /// <summary>
        /// When set (via an "out=&lt;path&gt;" command line argument) the window renders itself to
        /// this PNG path once the scenario has been applied, then exits.
        /// </summary>
        public static string CapturePath { get; set; }

        private object FindNavItem(string tag)
        {
            if (string.IsNullOrEmpty(tag))
            {
                return null;
            }

            foreach (var menuItem in NavView.MenuItems)
            {
                if (menuItem is NavigationViewItem item && (item.Tag as string) == tag)
                {
                    return item;
                }
            }

            return null;
        }

        private void OnNavigationSelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
        {
            if (args.SelectedItem is NavigationViewItem item && item.Tag is string tag)
            {
                Type pageType = tag switch
                {
                    "Flat" => typeof(FlatSelectionPage),
                    "Grouped" => typeof(GroupedSelectionPage),
                    "Range" => typeof(RangeSelectionPage),
                    "IndexPath" => typeof(IndexPathPage),
                    "Events" => typeof(EventsPage),
                    "Binding" => typeof(BindingPage),
                    _ => typeof(FlatSelectionPage),
                };

                ContentFrame.Navigate(pageType);

                if (tag == StartupTag &&
                    !string.IsNullOrEmpty(StartupScenario) &&
                    ContentFrame.Content is Common.IScenarioPage scenarioPage)
                {
                    scenarioPage.ApplyScenario(StartupScenario);

                    if (!string.IsNullOrEmpty(CapturePath))
                    {
                        _ = CaptureAndExitAsync();
                    }
                }
            }
        }

        /// <summary>
        /// Renders the window content to a PNG and exits. Used to produce the API spec screenshots
        /// deterministically, without depending on an interactive desktop.
        /// </summary>
        private async System.Threading.Tasks.Task CaptureAndExitAsync()
        {
            // Give layout, the ItemsRepeater and the NavigationView time to settle.
            await System.Threading.Tasks.Task.Delay(2500);

            // Only the page content is captured: the NavigationView pane uses an acrylic backdrop,
            // which RenderTargetBitmap cannot reproduce. An explicit opaque background keeps the
            // PNG readable on both light and dark documentation themes.
            ContentFrame.Background =
                (Microsoft.UI.Xaml.Media.Brush)Application.Current.Resources["ApplicationPageBackgroundThemeBrush"];
            await System.Threading.Tasks.Task.Delay(500);

            // Move focus out of the page so no text selection or focus visual appears in the PNG.
            NavView.Focus(FocusState.Programmatic);
            await System.Threading.Tasks.Task.Delay(500);

            var bitmap = new Microsoft.UI.Xaml.Media.Imaging.RenderTargetBitmap();
            await bitmap.RenderAsync(ContentFrame);
            var pixels = await bitmap.GetPixelsAsync();

            using (var fileStream = System.IO.File.Create(CapturePath))
            {
                var stream = fileStream.AsRandomAccessStream();
                var encoder = await Windows.Graphics.Imaging.BitmapEncoder.CreateAsync(
                    Windows.Graphics.Imaging.BitmapEncoder.PngEncoderId, stream);

                encoder.SetPixelData(
                    Windows.Graphics.Imaging.BitmapPixelFormat.Bgra8,
                    Windows.Graphics.Imaging.BitmapAlphaMode.Premultiplied,
                    (uint)bitmap.PixelWidth,
                    (uint)bitmap.PixelHeight,
                    96,
                    96,
                    pixels.ToArray());

                await encoder.FlushAsync();
                await stream.FlushAsync();
            }

            Application.Current.Exit();
        }
    }
}
