// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.ApplicationModel.DataTransfer;

namespace WinUICsDesktopSampleApp.WindowPlacement
{
    public sealed partial class PlacementPage : UserControl
    {
        private readonly ObservableCollection<Window> m_windows = new ObservableCollection<Window>();
        private int m_nextNumber = 1;

        public PlacementPage()
        {
            InitializeComponent();
            LogList.ItemsSource = Log.Items;
            WindowsCombo.ItemsSource = m_windows;
            WindowsCombo.DisplayMemberPath = "Title";
            StatusText.Text = $"Packaged app: {Windows.ApplicationModel.Package.Current.Id.FamilyName}\n" +
                $"OS: {Environment.OSVersion.Version}  PID: {Environment.ProcessId}";
        }

        private void OnOpen(object sender, RoutedEventArgs e)
        {
            Run("Open window", () =>
            {
                if (SetSizeCheck.IsChecked == true &&
                    (double.IsNaN(WidthBox.Value) || double.IsNaN(HeightBox.Value)))
                {
                    throw new ArgumentException("Enter a fallback width and height.");
                }

                var window = new TestWindow(m_nextNumber++, PlacementIdBox.Text);
                OpenTestWindow(window, () =>
                {
                    window.PlacementId = PlacementIdBox.Text;
                    if (SetSizeCheck.IsChecked == true)
                    {
                        window.Width = WidthBox.Value;
                        window.Height = HeightBox.Value;
                    }
                    var presenter = InitialPresenterCombo.SelectedIndex switch
                    {
                        1 => AppWindowPresenterKind.FullScreen,
                        2 => AppWindowPresenterKind.CompactOverlay,
                        _ => AppWindowPresenterKind.Overlapped,
                    };
                    window.AppWindow.SetPresenter(presenter);
                });
            });
        }

        private void OnOpenMarkup(object sender, RoutedEventArgs e) =>
            Run("Open markup window", () => OpenTestWindow(new MarkupWindow(), () => { }));

        private void OpenTestWindow(Window window, Action configure)
        {
            window.Closed += (sender, args) =>
            {
                Log.Write($"{window.Title}: Closed");
                m_windows.Remove(window);
            };
            m_windows.Add(window);
            WindowsCombo.SelectedItem = window;
            try
            {
                configure();
                Log.Write($"{window.Title}: Activate(), PlacementId=\"{window.PlacementId}\"");
                window.Activate();
                Log.Write($"{window.Title}: {PlacementDiagnostics.Describe(window).Replace("\n", "; ")}");
            }
            catch
            {
                window.Close();
                throw;
            }
        }

        private void OnActivateSelected(object sender, RoutedEventArgs e) =>
            Run("Activate selected", () => (WindowsCombo.SelectedItem as Window)?.Activate());

        private void OnCloseSelected(object sender, RoutedEventArgs e) =>
            Run("Close selected", () => (WindowsCombo.SelectedItem as Window)?.Close());

        public void CloseTestWindows()
        {
            foreach (Window window in m_windows.ToList())
            {
                window.Close();
            }
        }

        private void OnCloseAll(object sender, RoutedEventArgs e) => Run("Close all", CloseTestWindows);

        private void OnReadStore(object sender, RoutedEventArgs e) =>
            Run("Read saved records", () => StoreText.Text = PlacementDiagnostics.ReadSavedRecords());

        private async void OnDeleteSavedRecords(object sender, RoutedEventArgs e)
        {
            var dialog = new ContentDialog
            {
                XamlRoot = this.XamlRoot,
                Title = "Delete saved records?",
                Content = "Delete all saved window placements for this sample? " +
                    "Open test windows will save new records when they close.",
                PrimaryButtonText = "Delete",
                CloseButtonText = "Cancel",
                DefaultButton = ContentDialogButton.Close,
            };

            if (await dialog.ShowAsync() == ContentDialogResult.Primary)
            {
                Run("Delete saved records", () =>
                {
                    PlacementDiagnostics.DeleteSavedRecords();
                    StoreText.Text = PlacementDiagnostics.ReadSavedRecords();
                    Log.Write("Deleted all saved placement records.");
                });
            }
        }

        private void Run(string action, Action callback)
        {
            ErrorText.Text = "";
            try
            {
                callback();
            }
            catch (Exception ex)
            {
                ErrorText.Text = $"{action}: 0x{ex.HResult:X8} {ex.Message}";
                Log.Error(action, ex);
            }
        }

        private void OnClearLog(object sender, RoutedEventArgs e) => Log.Items.Clear();

        private void OnCopyLog(object sender, RoutedEventArgs e)
        {
            var package = new DataPackage();
            package.SetText(string.Join(Environment.NewLine, Log.Items.Reverse()));
            Clipboard.SetContent(package);
        }
    }
}
