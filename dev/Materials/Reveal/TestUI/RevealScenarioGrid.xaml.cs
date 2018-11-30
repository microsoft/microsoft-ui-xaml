// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

namespace MUXControlsTestApp
{
    public sealed partial class RevealScenarioGrid : TestPage
    {
        private ObservableCollection<RevealGridItem> gridSource;
        private ObservableCollection<RevealBackgroundItem> backgroundTypeSource;
        private ObservableCollection<RevealGridTypeItem> gridItemTypeSource;
        private ObservableCollection<SelectionModeItem> selectionModeSource;

        public RevealScenarioGrid()
        {
            this.InitializeComponent();

            gridSource = RevealGridItem.GetItems();
            backgroundTypeSource = RevealBackgroundItem.GetItems();
            gridItemTypeSource = RevealGridTypeItem.GetItems();
            selectionModeSource = SelectionModeItem.GetItems();
        }

        private void BackgroundComboBoxChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox box = (ComboBox)sender;
            RevealBackgroundItem item = (RevealBackgroundItem)box.SelectedItem;
            VisualStateManager.GoToState(this, item.Name, false);
        }

        private void ComboBox_SelectionChanged_1(object sender, SelectionChangedEventArgs e)
        {
            ComboBox box = (ComboBox)sender;
            RevealGridTypeItem item = (RevealGridTypeItem)box.SelectedItem;
            VisualStateManager.GoToState(this, item.Name, false);
        }

        private void ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox box = (ComboBox)sender;
            SelectionModeItem item = (SelectionModeItem)box.SelectedItem;
            RevealGrid.SelectionMode = item.Mode;
        }
    }

    public class RevealGridItem
    {
        private static ObservableCollection<RevealGridItem> items;

        public string Header;
        public string SubHeader = "This is a sub-header";
        public BitmapImage Image;

        public static ObservableCollection<RevealGridItem> GetItems()
        {
            if(items == null)
            {
                items = new ObservableCollection<RevealGridItem>();
                items.Add(new RevealGridItem { Header = "Smile", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage12.jpg")) });
                items.Add(new RevealGridItem { Header = "Business", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage23.jpg")) });
                items.Add(new RevealGridItem { Header = "Kid", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage20.jpg")) });
                items.Add(new RevealGridItem { Header = "Metal", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage22.jpg")) });
                items.Add(new RevealGridItem { Header = "Super Metal", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage7.jpg")) });
                items.Add(new RevealGridItem { Header = "Smile", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage12.jpg")) });
                items.Add(new RevealGridItem { Header = "Business", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage23.jpg")) });
                items.Add(new RevealGridItem { Header = "Kid", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage20.jpg")) });
                items.Add(new RevealGridItem { Header = "Metal", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage22.jpg")) });
                items.Add(new RevealGridItem { Header = "Super Metal", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage7.jpg")) });
                items.Add(new RevealGridItem { Header = "Smile", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage12.jpg")) });
                items.Add(new RevealGridItem { Header = "Business", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage23.jpg")) });
                items.Add(new RevealGridItem { Header = "Kid", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage20.jpg")) });
                items.Add(new RevealGridItem { Header = "Metal", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage22.jpg")) });
                items.Add(new RevealGridItem { Header = "Super Metal", Image = new BitmapImage(new Uri("https://raw.githubusercontent.com/Microsoft/Windows-universal-samples/master/SharedContent/media/Samples/LandscapeImage7.jpg")) });
            }

            return items;
        }
    }

    public class RevealBackgroundItem
    {
        private static ObservableCollection<RevealBackgroundItem> items;

        public string Name;
        public RevealTestBackgrounds Value;

        public static ObservableCollection<RevealBackgroundItem> GetItems()
        {
            if(items == null)
            {
                items = new ObservableCollection<RevealBackgroundItem>();
                foreach (RevealTestBackgrounds theme in Enum.GetValues(typeof(RevealTestBackgrounds)))
                {
                    items.Add(new RevealBackgroundItem { Name = Enum.GetName(typeof(RevealTestBackgrounds), theme), Value = theme });
                }
            }

            return items;
        }
    }

    public class RevealGridTypeItem
    {
        private static ObservableCollection<RevealGridTypeItem> items;

        public string Name;
        public RevealGridItemTemplates Template;

        public static ObservableCollection<RevealGridTypeItem> GetItems()
        {
            if(items == null)
            {
                items = new ObservableCollection<RevealGridTypeItem>();
                foreach (RevealGridItemTemplates template in Enum.GetValues(typeof(RevealGridItemTemplates)))
                {
                    items.Add(new RevealGridTypeItem { Name = Enum.GetName(typeof(RevealGridItemTemplates), template), Template = template });
                }
            }

            return items;
        }
    }

    public class SelectionModeItem
    {
        private static ObservableCollection<SelectionModeItem> items;

        public string Name;
        public ListViewSelectionMode Mode;

        public static ObservableCollection<SelectionModeItem> GetItems()
        {
            if (items == null)
            {
                items = new ObservableCollection<SelectionModeItem>();
                foreach (ListViewSelectionMode mode in Enum.GetValues(typeof(ListViewSelectionMode)))
                {
                    items.Add(new SelectionModeItem { Name = Enum.GetName(typeof(ListViewSelectionMode), mode), Mode = mode });
                }
            }

            return items;
        }
    }

    public enum RevealTestBackgrounds
    {
        Default = 0,
        Acrylic = 1,
        Accent = 2
    }

    public enum RevealGridItemTemplates
    {
        ImageWithText = 0,
        Image = 1,
        BaseLowItems = 2
    }
}
