using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

namespace MUXControlsTestApp
{
    public class BreadcrumbBarFolder
    {
        public string Name { get; set; }
        public SolidColorBrush Foreground { get; set; }
        public SolidColorBrush Background { get; set; }
    }

    [TopLevelTestPage(Name = "VisualProperties")]
    public sealed partial class VisualPropertiesPage : TestPage
    {
        TreeViewNode treeViewFolder;

        public VisualPropertiesPage()
        {
            this.InitializeComponent();

            Loaded += VisualPropertiesPage_Loaded;

            DefaultBreadCrumbBar.ItemsSource = new ObservableCollection<BreadcrumbBarFolder>{
                new BreadcrumbBarFolder { Name = "Home"},
                new BreadcrumbBarFolder { Name = "Folder1" },
                new BreadcrumbBarFolder { Name = "Folder2" },
                new BreadcrumbBarFolder { Name = "Folder3" },
            };

            InitializeSampleTreeView();
        }

        private void VisualPropertiesPage_Loaded(object sender, RoutedEventArgs e)
        {
            ForegroundPropertyComboBox.SelectedIndex = 0;
            BackgroundPropertyComboBox.SelectedIndex = 0;
        }

        private void ForegroundPropertyComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string colorName = e.AddedItems[0].ToString();
            SolidColorBrush color;

            switch (colorName)
            {
                case "Red":
                    color = new SolidColorBrush(Colors.Red);
                    break;
                case "Blue":
                    color = new SolidColorBrush(Colors.Blue);
                    break;
                default:
                    throw new Exception($"Invalid argument: {colorName}");
            }

            SetControlStackPanelForeground(color);
        }

        private void SetControlStackPanelForeground(SolidColorBrush color)
        {

            foreach (var child in ControlStackPanel.Children)
            {
                if (child is Control)
                {
                    (child as Control).Foreground = color;
                }
            } 
        }

        private void BackgroundPropertyComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string colorName = e.AddedItems[0].ToString();
            SolidColorBrush color;

            switch (colorName)
            {
                case "Green":
                    color = new SolidColorBrush(Colors.Green);
                    break;
                case "Purple":
                    color = new SolidColorBrush(Colors.Purple);
                    break;
                default:
                    throw new Exception($"Invalid argument: {colorName}");
            }

            SetControlStackPanelBackground(color);
        }

        private void SetControlStackPanelBackground(SolidColorBrush color)
        {

            foreach (var child in ControlStackPanel.Children)
            {
                if (child is Control)
                {
                    (child as Control).Background = color;
                }
            } 
        }

        private void InitializeSampleTreeView()
        {
            treeViewFolder = new TreeViewNode() { Content = "TreeViewFolder" };
            treeViewFolder.IsExpanded = true;
            treeViewFolder.Children.Add(new TreeViewNode() { Content = "TreeViewItem1" });

            DefaultTreeView.RootNodes.Add(treeViewFolder);
        }

        private async void ShowDialog_Click(object sender, RoutedEventArgs e)
        {
            await DefaultContentDialog.ShowAsync();
        }
    }
}
