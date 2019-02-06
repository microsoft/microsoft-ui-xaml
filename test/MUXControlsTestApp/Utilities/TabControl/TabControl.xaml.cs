// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

namespace MUXControlsTestApp.Utilities
{
    public sealed partial class TabControl : UserControl
    { 
        public event EventHandler SelectedTabChanged;
        private Tab selectedTab = null;

        public TabControl()
        {
            this.InitializeComponent();
            this.Tabs = new ObservableCollection<Tab>();
        }

        public Tab SelectedTab
        {
            get
            {
                return selectedTab;
            }

            private set
            {
                if (value != selectedTab)
                {
                    selectedTab = value;
                    if (SelectedTabChanged != null)
                    {
                        SelectedTabChanged(this, EventArgs.Empty);
                    }
                }
            }
        }

        public ObservableCollection<Tab> Tabs
        {
            get;
            private set;
        }

        public Tab AddTab(string name)
        {
            return AddTab(name, null);
        }

        public Tab AddTab(string name, UIElement body)
        {
            foreach (Tab tab in this.Tabs)
            {
                if (tab.Name == name)
                    throw new ArgumentException();
            }

            Tab newTab = new Tab(name, body);
            this.spHeaders.Children.Add(newTab.Header);
            this.grdPanels.Children.Add(newTab.Panel);

            this.Tabs.Add(newTab);

            newTab.Header.Checked += Header_Checked;

            if (this.Tabs.Count == 1)
            {
                SelectTab(newTab);
            }

            return newTab;
        }

        public void RemoveTab(string name)
        {
            foreach (Tab tab in this.Tabs)
            {
                if (tab.Name == name)
                {
                    if (this.SelectedTab == tab)
                        UnselectTab();

                    tab.Header.Checked -= Header_Checked;
                    this.spHeaders.Children.Remove(tab.Header);
                    this.grdPanels.Children.Remove(tab.Panel);
                    this.Tabs.Remove(tab);

                    if (this.Tabs.Count > 0)
                        SelectTab(this.Tabs[0]);
                    return;
                }
            }

            throw new ArgumentException();
        }

        public void SelectTab(Tab tab)
        {
            if (tab == this.SelectedTab)
                return;

            foreach (Tab tabTmp in this.Tabs)
            {
                if (tabTmp == tab)
                {
                    UnselectTab();
                    tab.Header.IsChecked = true;
                    tab.Panel.Visibility = Visibility.Visible;
                    this.SelectedTab = tab;
                    return;
                }
            }

            throw new ArgumentException();
        }

        private void Header_Checked(object sender, RoutedEventArgs e)
        {
            foreach (Tab tab in this.Tabs)
            {
                if (sender == tab.Header)
                {
                    UnselectTab();
                    SelectTab(tab);
                }
            }
        }

        private void UnselectTab()
        {
            if (this.SelectedTab != null)
            {
                this.SelectedTab.Header.IsChecked = false;
                this.SelectedTab.Panel.Visibility = Visibility.Collapsed;
                this.SelectedTab = null;
            }
        }
    }

    public class Tab
    {
        internal Tab(string name, UIElement body)
        {
            this.Name = name;
            this.Header = new ToggleButton();
            this.Header.Content = name;
            this.Header.VerticalAlignment = VerticalAlignment.Center;
            this.Panel = new Grid();
            this.Panel.Visibility = Visibility.Collapsed;
            if (body != null)
            {
                this.Panel.Children.Add(body);
            }
        }

        public string Name
        {
            get;
            set;
        }

        public ToggleButton Header
        {
            get;
            set;
        }

        public Grid Panel
        {
            get;
            set;
        }
    }
}
