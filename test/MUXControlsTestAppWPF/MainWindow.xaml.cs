// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MUXControlsTestAppWPF
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            Loaded += MainWindow_Loaded;
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            Windows.UI.Xaml.Application.Current.Resources = new Microsoft.UI.Xaml.Controls.XamlControlsResources();
            var frame = new MUXControlsTestApp.TestFrame(typeof(MUXControlsTestAppForIslands.MainPage));
            windowXamlHost.Child = frame;
            frame.Navigate(typeof(MUXControlsTestAppForIslands.MainPage));
        }
    }
}
