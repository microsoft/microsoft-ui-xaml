using System;
using Windows.UI.Xaml.Controls;

namespace ManagedUWP
{
    public sealed partial class MainPage : UserControl
    {
        public MainPage()
        {
            try
            {
                this.InitializeComponent();
            }
            catch
            {
                this.InitializeComponent();
            }
            contentFrame.Navigate(typeof(WelcomePage), null);
        }

        private void NavView_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
        {
            if (args.IsSettingsInvoked)
            {
                contentFrame.Navigate(typeof(SettingsPage));
                return;
            }

            Type pageType = null;
            NavigationViewItem navViewItem = args.InvokedItemContainer as NavigationViewItem;
            if (navViewItem != null)
            {

                switch (navViewItem.Tag)
                {
                    case "WelcomePage":
                        pageType = typeof(WelcomePage);
                        break;
                    case "WinUIPage":
                        pageType = typeof(WinUIPage);
                        break;
                }
                if (pageType != null)
                {
                    contentFrame.Navigate(pageType);
                }
            }

        }

        private void NavView_BackRequested(NavigationView sender, NavigationViewBackRequestedEventArgs args)
        {
            if (contentFrame.CanGoBack)
                contentFrame.GoBack();
        }
    }
}
