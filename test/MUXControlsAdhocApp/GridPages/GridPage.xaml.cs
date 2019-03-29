using Windows.UI.Xaml.Controls;

namespace MUXControlsAdhocApp.GridPages
{
    public sealed partial class GridPage : Page
    {
        public GridPage()
        {
            this.InitializeComponent();

            _contentFrame.Navigate(typeof(GridPlaygroundPage));
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
                    _contentFrame.Navigate(typeof(GridPlaygroundPage));
                    break;

                case "TemplateColumnsAndRows":
                    _contentFrame.Navigate(typeof(GridTemplateColumnsAndRowsPage));
                    break;
            }
        }
    }
}
