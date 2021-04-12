using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Diagnostics;
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

namespace Experiments
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
            navigator.ItemInvoked += Navigator_ItemInvoked;
            navigator.IsBackButtonVisible = Microsoft.UI.Xaml.Controls.NavigationViewBackButtonVisible.Collapsed;
            navigator.IsSettingsVisible = false;
        }

        private void Navigator_ItemInvoked(Microsoft.UI.Xaml.Controls.NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs args)
        {
            var itemContent = args.InvokedItemContainer.Content.ToString();
            switch (itemContent)
            {
                case "Shimmer":
                    frame.Navigate(typeof(ShimmerPage));
                    break;

                case "Carousal":
                    frame.Navigate(typeof(CarousalPage));
                    break;

                case "CommandBar":
                    frame.Navigate(typeof(CommandBarPage));
                    break;

                case "SegmentedControl":
                    frame.Navigate(typeof(SegmentedControlPage));
                    break;

                default:
                    break;
            }

            headerText.Text = itemContent;
        }
    }
}
