// Copyright (c) Microsoft Corporation.
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
using XamlCompilerEtlReader;
using XamlCompilerEtlReader.Structure;

namespace XamlCompilerEtlViewer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            Application.Current.Exit += Current_Exit;
            InitializeComponent();
        }

        void Current_Exit(object sender, ExitEventArgs e)
        {
            Application.Current.Exit -= Current_Exit;
            _perfDataContext.Dispose();
        }

        private void ClearButton_Click(object sender, RoutedEventArgs e)
        {
            _perfDataContext.ClearEvents();
        }
    }
}
