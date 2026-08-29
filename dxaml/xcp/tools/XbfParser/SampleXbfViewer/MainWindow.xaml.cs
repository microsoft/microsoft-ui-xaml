// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Microsoft.Win32;
using Microsoft.Xaml.WidgetSpinner;
using System.Windows;
using System.Windows.Input;

namespace SampleXbfViewer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static DependencyProperty XbfFileViewModelProperty =
            DependencyProperty.Register("XbfFileViewModel", typeof(XbfFileViewModel), typeof(MainWindow), null);

        public static DependencyProperty XamlObjectViewModelProperty =
            DependencyProperty.Register("XamlObjectViewModel", typeof(XamlObjectViewModel), typeof(MainWindow), null);

        public XbfFileViewModel XbfFileViewModel
        {
            get => (XbfFileViewModel)GetValue(XbfFileViewModelProperty);
            set => SetValue(XbfFileViewModelProperty, value);
        }

        public XamlObjectViewModel XamlObjectViewModel
        {
            get => (XamlObjectViewModel)GetValue(XamlObjectViewModelProperty);
            set => SetValue(XamlObjectViewModelProperty, value);
        }

        public MainWindow()
        {
            DataContext = this;

            InitializeComponent();
        }

        private void BrowseButton_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog
            {
                DefaultExt = ".xbf",
                Filter = "XBF Files (*.xbf)|*.xbf"
            };

            var result = dialog.ShowDialog();
            if (result.HasValue && result.Value)
            {
                FilePathTextBox.Text = dialog.FileName;
            }

            OpenXbfFile(FilePathTextBox.Text);
        }

        private void FilePathTextBox_OnKeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                OpenXbfFile(FilePathTextBox.Text);
            }
        }

        private void OpenFileButton_Click(object sender, RoutedEventArgs e)
        {
            OpenXbfFile(FilePathTextBox.Text);
        }

        private void OpenXbfFile(string path)
        {
            if (!System.IO.File.Exists(path))
            {
                return;
            }

            var widgetSpinner = new WidgetSpinner();
            var xbfFile = widgetSpinner.DeserializeXbfFile(path);
            XbfFileViewModel = new XbfFileViewModel(xbfFile);
            XamlObjectViewModel = new XamlObjectViewModel(widgetSpinner.CreateObjectGraph(xbfFile));
        }
    }
}
