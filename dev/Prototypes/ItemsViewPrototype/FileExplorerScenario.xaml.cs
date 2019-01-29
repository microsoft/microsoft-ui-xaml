using DEPControlsTestApp.ItemsViewPrototype;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using Windows.Devices.Geolocation;
using Windows.Storage;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

using muxcp = Microsoft.UI.Xaml.Controls.Primitives;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace DEPControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class FileExplorerScenario : Page
    {
        public List<Item> allItems = new List<Item>();
        public ObservableDataSource<Item> Items = new ObservableDataSource<Item>();
        public ObservableDataSource<IGrouping<string, Item>> ItemsGroupedByParentMountain = new ObservableDataSource<IGrouping<string, Item>>();
        bool isGrouped = false;

        public FileExplorerScenario()
        {
            this.InitializeComponent();

            this.Loaded += FileExplorerScenario_Loaded;
            isGrouped = true;

            flatStackButton.Click += (sender, args) =>
            {
                itemsView.ItemsSource = null;
                itemsView.ViewDefinition = flatStackDefinition;
                itemsView.ItemsSource = Items;
                isGrouped = false;
            };

            flatFlowButton.Click += (sender, args) =>
            {
                itemsView.ItemsSource = null;
                itemsView.ViewDefinition = flatFlowDefinition;
                itemsView.ItemsSource = Items;
                isGrouped = false;
            };

            iconFlowButton.Click += (sender, args) =>
            {
                itemsView.ItemsSource = null;
                itemsView.ViewDefinition = iconFlowDefinition;
                itemsView.ItemsSource = Items;
                isGrouped = false;
            };

            flatTableButton.Click += (sender, args) =>
            {
                itemsView.ItemsSource = null;
                itemsView.ViewDefinition = flatTableDefinition;
                itemsView.ItemsSource = Items;
                isGrouped = false;
            };

            groupedTableButton.Click += (sender, args) =>
            {
                itemsView.ItemsSource = null;
                itemsView.ViewDefinition = groupedTableDefinition;
                itemsView.ItemsSource = ItemsGroupedByParentMountain;
                // itemsView.ChildrenRequested += ItemsView_ChildrenRequested;
                isGrouped = true;
            };

            itemsView.SortFunc = SortItems;
            itemsView.FilterFunc = FilterItems;
        }
        
        private void FileExplorerScenario_Loaded(object sender, RoutedEventArgs e)
        {
            LoadItems();
        }

        //private void ItemsView_ChildrenRequested(object sender, Microsoft.UI.Xaml.Controls.SelectionModelChildrenRequestedEventArgs e)
        //{
        //    var source = e.Source as IEnumerable<IGrouping<string, Item>>;
        //    if(source != null)
        //    {
        //        e.Children = e.Source;
        //    }
        //    else
        //    {
        //        e.Children = null;
        //    }
        //}

        private void OnColumnsChanged(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                var button = sender as ToggleButton;
                var columnName = button.Content.ToString();
                var table = itemsView.ViewDefinition as TableDefinition;
                if (table != null && table.ColumnDefinitions != null)
                {
                    if (button.IsChecked != null && button.IsChecked.Value)
                    {
                        var def = Resources[columnName] as ItemsViewColumnDefinitionBase;
                        Debug.Assert(def != null);
                        table.ColumnDefinitions.Add(def);
                    }
                    else
                    {
                        ItemsViewColumnDefinitionBase toRemove = null;
                        foreach (var columnDef in table.ColumnDefinitions)
                        {
                            var column = columnDef as ItemsViewColumnDefinitionBase;
                            if (column != null && column.ColumnName == columnName)
                            {
                                toRemove = columnDef;
                                break;
                            }
                        }

                        if (toRemove != null)
                        {
                            table.ColumnDefinitions.Remove(toRemove);
                        }
                    }
                }
            }
        }

        async void LoadItems()
        {
            Windows.Storage.StorageFile file = await StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///Assets/mtns.csv"));
            using (Stream stream = await file.OpenStreamForReadAsync())
            {
                using (StreamReader sr = new StreamReader(stream))
                {
                    while (!sr.EndOfStream)
                    {
                        string line = sr.ReadLine();
                        string[] values = line.Split(',');
                        string[] coordinates = values[4].Split();
                        double latitude = GetDecimalDegrees(coordinates[0]);
                        double longitude = GetDecimalDegrees(coordinates[1]);

                        allItems.Add(
                            new Item()
                            {
                                Rank = uint.Parse(values[0]),
                                Mountain = values[1],
                                Height_m = uint.Parse(values[2]),
                                Range = values[3],
                                Coordinates = values[4],
                                Prominence = uint.Parse(values[5]),
                                Parent_mountain = values[6],
                                First_ascent = uint.Parse(values[7]),
                                Ascents = values[8],
                                mapCenter = new Geopoint(new BasicGeoposition() { Latitude = latitude, Longitude = longitude })
                            });
                    }

                    var items = new ObservableCollection<Item>(allItems);
                    Items.Data = items;
                    var groups = from item in items
                                 group item by item.Parent_mountain into g
                                 select g;
                    ItemsGroupedByParentMountain.Data = new ObservableCollection<IGrouping<string, Item>>(groups);
                }
            }
        }

        private double GetDecimalDegrees(string valueInDMS)
        {
            double value = 0;
            valueInDMS = valueInDMS.Replace('d', '.');
            valueInDMS = valueInDMS.Replace('m', '.');
            valueInDMS = valueInDMS.Replace('s', '.');
            string[] valueInDegrees = valueInDMS.Split('.');
            value = Convert.ToDouble(valueInDegrees[0]) + (Convert.ToDouble(valueInDegrees[1])) / 60 + (Convert.ToDouble(valueInDegrees[2]) / 3600);
            return value;
        }

        private void TraceSelectionState(object sender, RoutedEventArgs e)
        {
            var selector = itemsView.Selector;
            var indices = selector.Model.SelectedIndices;
            var items = selector.Model.SelectedItems;
            //Debug.Assert(indices.Count == items.Count);
            //for (int i = 0; i < items.Count; i++)
            //{
            //    Debug.WriteLine(indices[i] + ":" + items[i]);
            //}

            foreach (var index in selector.Model.SelectedIndices)
            {
                Debug.WriteLine(index);
            }

            //foreach (var obj in selector.Model.SelectedItems)
            //{
            //    Debug.WriteLine(obj);
            //}
        }

        private void FilterItems(int index, string filterText)
        {
            switch (index)
            {
                case 0:
                    Items.Data = new ObservableCollection<Item>(from item in allItems
                                                                where item.Rank.ToString().Contains(filterText)
                                                                select item);
                    break;

                case 1:
                    Items.Data = new ObservableCollection<Item>(from item in allItems
                                                                where item.Mountain.Contains(filterText)
                                                                select item);
                    break;
                case 2:
                    Items.Data = new ObservableCollection<Item>(from item in allItems
                                                                where item.Height_m.ToString().Contains(filterText)
                                                                select item);
                    break;
                case 3:
                    Items.Data = new ObservableCollection<Item>(from item in allItems
                                                                where item.Range.Contains(filterText)
                                                                select item);
                    break;
                case 4:
                    Items.Data = new ObservableCollection<Item>(from item in allItems
                                                                where item.Coordinates.Contains(filterText)
                                                                select item);
                    break;
                case 5:
                    Items.Data = new ObservableCollection<Item>(from item in allItems
                                                                where item.Prominence.ToString().Contains(filterText)
                                                                select item);
                    break;
                case 6:
                    Items.Data = new ObservableCollection<Item>(from item in allItems
                                                                where item.Parent_mountain.Contains(filterText)
                                                                select item);
                    break;
                case 7:
                    Items.Data = new ObservableCollection<Item>(from item in allItems
                                                                where item.First_ascent.ToString().Contains(filterText)
                                                                select item);
                    break;
                case 8:
                    Items.Data = new ObservableCollection<Item>(from item in allItems
                                                                where item.Ascents.ToString().Contains(filterText)
                                                                select item);
                    break;
                default:
                    break;
            }
        }

        private void SortItems(int index, bool asc)
        {
            if (asc)
            {
                switch (index)
                {
                    case 0:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Rank ascending
                                                                    select item);
                        break;

                    case 1:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Mountain ascending
                                                                    select item);
                        break;
                    case 2:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Height_m ascending
                                                                    select item);
                        break;
                    case 3:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Range ascending
                                                                    select item);
                        break;
                    case 4:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Coordinates ascending
                                                                    select item);
                        break;
                    case 5:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Prominence ascending
                                                                    select item);
                        break;
                    case 6:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Parent_mountain ascending
                                                                    select item);
                        break;
                    case 7:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.First_ascent ascending
                                                                    select item);
                        break;
                    case 8:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Ascents ascending
                                                                    select item);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                switch (index)
                {
                    case 0:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Rank descending
                                                                    select item);
                        break;

                    case 1:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Mountain descending
                                                                    select item);
                        break;
                    case 2:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Height_m descending
                                                                    select item);
                        break;
                    case 3:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Range descending
                                                                    select item);
                        break;
                    case 4:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Coordinates descending
                                                                    select item);
                        break;
                    case 5:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Prominence descending
                                                                    select item);
                        break;
                    case 6:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Parent_mountain descending
                                                                    select item);
                        break;
                    case 7:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.First_ascent descending
                                                                    select item);
                        break;
                    case 8:
                        Items.Data = new ObservableCollection<Item>(from item in Items.Data
                                                                    orderby item.Ascents descending
                                                                    select item);
                        break;
                    default:
                        break;
                }
            }
        }

        private void IsGroup(object sender, IsGroupEventArgs args)
        {
            if (args.Item is IGrouping<string, Item>)
            {
                args.IsGroup = true;
            }
        }

        public muxcp.SelectionMode ConvertToSelectionMode(string value)
        {
            muxcp.SelectionMode mode = muxcp.SelectionMode.None;
            Enum.TryParse<muxcp.SelectionMode>(value, out mode);
            return mode;
        }

    }

    public class Item
    {
        public uint Rank { get; set; }
        public string Mountain { get; set; }
        public uint Height_m { get; set; }
        public string Range { get; set; }
        public string Coordinates { get; set; }
        public uint Prominence { get; set; }
        public string Parent_mountain { get; set; }
        public uint First_ascent { get; set; }
        public string Ascents { get; set; }
        public Uri ImageUri
        {
            get
            {
                return new Uri(string.Format("ms-appx:///Assets/mountain{0}.jpg", (Rank % 3) + 1));
            }
        }


        public Geopoint mapCenter { get; set; }
    }
}
