using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// The Content Dialog item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace MUXControlsTestApp
{
    public sealed partial class DialogWithNavView : ContentDialog
    {
        private int paneClosingCount;
        private int dialogClosingCount;

        public DialogWithNavView()
        {
            this.InitializeComponent();
            navigationView.PaneClosing += NavigationView_PaneClosing;
            Closing += (sender, args) =>
            {
                DialogClosingCount.Text = (++dialogClosingCount).ToString();
                args.Cancel = KeepDialogOpen.IsChecked == true;
            };
        }

        private void NavigationView_PaneClosing(NavigationView sender, NavigationViewPaneClosingEventArgs args)
        {
            DialogPaneClosingCount.Text = (++paneClosingCount).ToString();
            args.Cancel = CancelDialogPaneClosing.IsChecked == true;
        }

        private void ResetDialogFocusCounts_Click(object sender, RoutedEventArgs e)
        {
            paneClosingCount = 0;
            dialogClosingCount = 0;
            DialogPaneClosingCount.Text = "0";
            DialogClosingCount.Text = "0";
        }

        private void ContentDialog_PrimaryButtonClick(ContentDialog sender, ContentDialogButtonClickEventArgs args)
        {
            Hide();
        }

        private void ContentDialog_SecondaryButtonClick(ContentDialog sender, ContentDialogButtonClickEventArgs args)
        {
            Hide();
        }
    }
}
