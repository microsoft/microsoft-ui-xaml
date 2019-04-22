using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace Flick
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class FlexDemo : Page
    {
        public FlexDemo()
        {
            this.InitializeComponent();
            this.Loaded += OnLoaded;
        }

        private async void OnLoaded(object sender, RoutedEventArgs e)
        {
            var items = await FlickApi.GetPhotos("architecture", 10);
            for(int i=0; i< items.Count; i++)
            {
                items[i].FlexBasis = 300;
                items[i].FlexGrow = i % 3 + 1;
                items[i].FlexShrink = 1;
            }
            repeater.ItemsSource = items;
        }

        private void Grid_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Escape)
            {
                Frame.GoBack();
            }
        }

        private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            repeater.InvalidateMeasure();
        }
    }
}
