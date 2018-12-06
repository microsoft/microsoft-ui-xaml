// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Windows.ApplicationModel.DataTransfer;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Media;
using System.Collections.ObjectModel;
using System.ComponentModel;

#if !BUILD_WINDOWS
using TreeViewNode = Microsoft.UI.Xaml.Controls.TreeViewNode;
using TreeView = Microsoft.UI.Xaml.Controls.TreeView;
#endif

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
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
                    NotifyPropertyChanged("TestTreeViewItemsSource");
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
            var root2 = new TreeViewItemSource() { Content = "Root.2" };
            var root = new TreeViewItemSource() { Content = "Root", Children = { root0, root1, root2 } };

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
