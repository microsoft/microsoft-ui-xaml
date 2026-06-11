
using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;
using Microsoft.UI.Xaml.Automation;

using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class TabViewSizingPage : Page
    {
        public TabViewSizingPage()
        {
            this.InitializeComponent();
        }

        int _newTabNumber = 1;

        private void SetSmallWidth_Click(object sender, object args)
        {
            LayoutRoot.Width = 500;
        }

        private void SetLargeWidth_Click(object sender, object args)
        {
            LayoutRoot.Width = 800;
        }

        private void GetWidthsButton_Click(object sender, object args)
        {
            WidthEqualText.Text = TabViewEqual.ActualWidth.ToString();
            WidthSizeToContentText.Text = TabViewSizeToContent.ActualWidth.ToString();
        }

        private void Tabview_AddTabButtonClick(TabView sender, object args)
        {
            TabViewItem item = new TabViewItem();
            item.IconSource = new Microsoft.UI.Xaml.Controls.SymbolIconSource { Symbol = Symbol.Accept };
            item.Header = "New Tab " + _newTabNumber++;
            item.Content = item.Header;

            sender.TabItems.Add(item);
        }

        private void Tabs_TabCloseRequested(TabView sender, TabViewTabCloseRequestedEventArgs e)
        {
            sender.TabItems.Remove(e.Tab);
        }
    }
}
