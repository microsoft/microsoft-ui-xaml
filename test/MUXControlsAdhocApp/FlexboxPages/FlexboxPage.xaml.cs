using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsAdhocApp.FlexboxPages
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class FlexboxPage : Page
    {
        public FlexboxPage()
        {
            this.InitializeComponent();

            _contentFrame.Navigate(typeof(FlexboxPlaygroundPage));
        }

        private void NavigationView_BackRequested(Microsoft.UI.Xaml.Controls.NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewBackRequestedEventArgs args)
        {
            Frame.GoBack();
        }

        private void NavigationView_ItemInvoked(Microsoft.UI.Xaml.Controls.NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs args)
        {
            string tag = args.InvokedItemContainer.Tag.ToString();
            switch (tag)
            {
                case "Home":
                    _contentFrame.Navigate(typeof(FlexboxPlaygroundPage));
                    break;

                case "Order":
                    _contentFrame.Navigate(typeof(FlexboxOrderPage));
                    break;

                case "Direction":
                    _contentFrame.Navigate(typeof(FlexboxDirectionPage));
                    break;

                case "Wrap":
                    _contentFrame.Navigate(typeof(FlexboxWrapPage));
                    break;

                case "JustifyContent":
                    _contentFrame.Navigate(typeof(FlexboxJustifyContentPage));
                    break;

                case "AlignItems":
                    _contentFrame.Navigate(typeof(FlexboxAlignItemsPage));
                    break;

                case "AlignContent":
                    _contentFrame.Navigate(typeof(FlexboxAlignContentPage));
                    break;
            }
            
        }
    }
}
