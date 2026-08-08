using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml;

// To learn more about WinUI and the WinUI project structure,
// see https://learn.microsoft.com/windows/apps/winui/winui3/

namespace XamlPGO.CommonControls1
{
    public class Item
    {
        public int Id { get; set; }
        public string Name { get; set; }
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class HomePage : Page
    {
        public ObservableCollection<Item> Items { get; } = new ObservableCollection<Item>();

        public HomePage()
        {
            this.InitializeComponent();
            Items.Add(new Item { Id = 1, Name = "Item 1" });
            Items.Add(new Item { Id = 2, Name = "Item 2" });
        }
    }
}
