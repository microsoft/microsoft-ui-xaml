using MUXControlsTestApp.ItemsViewPrototype;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Xml.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class ItemsViewTestUIPage : Page
    {
        public ItemsViewTestUIPage()
        {
            this.InitializeComponent();

            basic.Click += (object sender, RoutedEventArgs e) =>
            {
                Frame.Navigate(typeof(ItemsViewBasicPage));
            };

            fileExplorer.Click += (object sender, RoutedEventArgs e) =>
            {
                Frame.Navigate(typeof(FileExplorerScenario));
            };
        }
    }
}
