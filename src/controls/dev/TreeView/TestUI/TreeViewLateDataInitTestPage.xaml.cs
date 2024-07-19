﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using Microsoft.UI.Xaml;
using System.Collections.ObjectModel;
using System.ComponentModel;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    /// Test page for late initialization data binding. ItemsSource is null until we click the button to set it.
    public sealed partial class TreeViewLateDataInitTest : TestPage, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private ObservableCollection<TreeViewItemSource> m_testTreeViewItemsSource;
        public ObservableCollection<TreeViewItemSource> TestTreeViewItemsSource
        {
            get { return m_testTreeViewItemsSource; }
            set
            {
                if (m_testTreeViewItemsSource != value)
                {
                    m_testTreeViewItemsSource = value;
                    NotifyPropertyChanged(nameof(TestTreeViewItemsSource));
                }
            }
        }

        public TreeViewLateDataInitTest()
        {
            this.InitializeComponent();
        }

        private void InitializeItemsSource_Click(object sender, RoutedEventArgs e)
        {
            TestTreeViewItemsSource = PrepareItemsSource();
        }

        private ObservableCollection<TreeViewItemSource> PrepareItemsSource()
        {
            var root0 = new TreeViewItemSource() { Content = "Root.0" };
            var root1 = new TreeViewItemSource() { Content = "Root.1" };
            var root = new TreeViewItemSource() { Content = "Root", Children = { root0, root1 } };

            return new ObservableCollection<TreeViewItemSource> { root };
        }

        private void NotifyPropertyChanged(String propertyName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}
