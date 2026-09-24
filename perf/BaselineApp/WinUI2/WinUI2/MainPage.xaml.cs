using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.Storage;
using Windows.Storage.Pickers;
using System;
using System.Collections.Generic;
using Windows.UI.ViewManagement;
using Windows.System;
using Windows.UI.Core;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Controls;
using System.Linq;
using System.Threading.Tasks;
using Windows.UI.WindowManagement;
using Windows.UI.Xaml.Input;
using WinRT.Interop;
using System.Diagnostics;

namespace WinUI2
{
    public class FolderItem
    {
        public string Name { get; set; }
        public string Path { get; set; }
        public ObservableCollection<FolderItem> SubItems { get; set; }
    }

    public class TabContent
    {
        public string? Header { get; set; }
        public string? Content { get; set; }
        public string? FilePath { get; set; }
    }

    public sealed partial class MainPage : Page
    {
        private Stopwatch stopwatch;
        private StorageFile? currentFile;
        private static readonly string[] lineSeperator = new[] { "\r\n", "\r", "\n" };
        private static readonly char[] wordSeperator = new[] { ' ', '\r', '\n' };
        private StorageFolder? currentFolder;

        public MainPage()
        {
            InitializeComponent();
            UpdateTitleBar("Untitled");
            AddNewTab("Unttiled", "", null);
        }

        private async void OpenFile_Click(object sender, RoutedEventArgs e)
        {
            // Create a new instance of the FileOpenPicker
            var picker = new FileOpenPicker();

            // Set the view mode to show a list of files
            picker.ViewMode = PickerViewMode.List;

            // Set the suggested start location to the documents library
            picker.SuggestedStartLocation = PickerLocationId.DocumentsLibrary;

            // Add filter for .txt file. But also keep all files as option.
            picker.FileTypeFilter.Add(".txt");
            picker.FileTypeFilter.Add("*");

            StorageFile file = await picker.PickSingleFileAsync();
            if (file != null)
            {
                string text = await FileIO.ReadTextAsync(file);
                AddNewTab(file.Name, text, file.Path);
                

            }
        }

        private async void SaveFile_Click(object sender, RoutedEventArgs e)
        {
            var tab = FileTabView.SelectedItem as TabViewItem;
            if (tab != null && tab.Tag is StorageFile file)
            {
                if (tab.Content is Grid grid && grid.Children[1] is TextBox textBox)
                {
                    await FileIO.WriteTextAsync(file, textBox.Text);
                }
            }
            else
            {
                SaveAsFile_Click(sender, e);
            }
        }

        private async void SaveAsFile_Click(object sender, RoutedEventArgs e)
        {
            var picker = new FileSavePicker();
            picker.CommitButtonText = "Save";
            picker.SuggestedStartLocation = PickerLocationId.DocumentsLibrary;
            picker.FileTypeChoices.Add("Plain Text", new List<string>() { ".txt" });

            StorageFile file = await picker.PickSaveFileAsync();
            if (file != null)
            {
                var tab = FileTabView.SelectedItem as TabViewItem;
                if (tab != null)
                {
                    if (tab.Content is Grid grid && grid.Children[1] is TextBox textBox)
                    {
                        await FileIO.WriteTextAsync(file, textBox.Text);
                        tab.Header = file.Name;
                        tab.Tag = file;
                    }
                }
            }
        }

        private void FileTabView_TabCloseRequested(TabView sender, TabViewTabCloseRequestedEventArgs args)
        {
            // Remove the tab from the TabView
            sender.TabItems.Remove(args.Tab);
        }


        private void CloseFile_Click(object sender, RoutedEventArgs e)
        {
            var selectedTab = FileTabView.SelectedItem as TabContent;
            if (selectedTab != null)
            {
                FileTabView.TabItems.Remove(selectedTab);
            }
        }

        private void UpdateTitleBar(string fileName)
        {
            var appView = ApplicationView.GetForCurrentView();
            appView.Title = fileName;
        }

        private void EditorTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            UpdateStatusBar();
        }

        private void EditorTextBox_SelectionChanged(object sender, RoutedEventArgs e)
        {
            UpdateCursorPosition();
        }

        private void UpdateStatusBar()
        {
            var tab = FileTabView.SelectedItem as TabContent;
            if (tab == null)
                return;
            var text = tab.Content;
            if (!string.IsNullOrEmpty(text))
            {
                var lines = text.Split(lineSeperator, StringSplitOptions.None).Length;
                var words = text.Split(wordSeperator, StringSplitOptions.RemoveEmptyEntries).Length;

                LineCountTextBlock.Text = $"Lines: {lines}";
                WordCountTextBlock.Text = $"Words: {words}";
                UpdateCursorPosition();
            }
        }

        private TextBox? GetActiveTabTextBox()
        {
            var tab = FileTabView.ContainerFromIndex(FileTabView.SelectedIndex) as TabViewItem;
            if (tab == null)
                return null;

            return (tab.Content as Grid)?.Children[1] as TextBox;
        }
        private void UpdateCursorPosition()
        {
            var textBox = GetActiveTabTextBox();
            if (textBox == null)
                return;
            var text = textBox.Text;

            if (!string.IsNullOrEmpty(text))
            {
                var selectionStart = textBox.SelectionStart;
                var line = text.Substring(0, selectionStart).Split(lineSeperator, StringSplitOptions.None).Length;
                var lastNewLineIndex = selectionStart > 0 ? text.LastIndexOfAny(wordSeperator, selectionStart - 1) : 0;
                var charPosition = (lastNewLineIndex == -1) ? selectionStart : selectionStart - lastNewLineIndex - 1;
                CursorPositionTextBlock.Text = $"Ln: {line}, Ch: {charPosition}";
            }
        }

        private void Page_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (Window.Current.CoreWindow.GetKeyState(VirtualKey.Control).HasFlag(CoreVirtualKeyStates.Down))
            {
                switch (e.Key)
                {
                    case VirtualKey.S:
                        SaveFile_Click(sender, e);
                        break;
                    case VirtualKey.O:
                        OpenFile_Click(sender, e);
                        break;
                }
            }
        }

        private void FileTabView_AddTabButtonClick(TabView sender, object args)
        {
            AddNewTab("Untitled", string.Empty, null);
        }

        private void SelectAll_Click(object sender, RoutedEventArgs e)
        {
            var textBox = GetActiveTabTextBox();
            if (textBox == null)
                return;
            textBox.Focus(FocusState.Programmatic);
            textBox.SelectAll();
        }

        private async void GoTo_Click(object sender, RoutedEventArgs e)
        {
            var inputTextBox = new TextBox
            {
                AcceptsReturn = false,
                PlaceholderText = "Enter line number"
            };

            var dialog = new ContentDialog
            {
                Title = "Go To Line",
                Content = inputTextBox,
                PrimaryButtonText = "Done",
                CloseButtonText = "Cancel",
                XamlRoot = this.Content.XamlRoot // Set the XamlRoot property
            };

            if (await dialog.ShowAsync() == ContentDialogResult.Primary)
            {
                if (int.TryParse(inputTextBox.Text, out int lineNumber))
                {
                    var textBox = GetActiveTabTextBox();
                    if (textBox == null)
                        return;
                    var lines = textBox.Text.Split(lineSeperator, StringSplitOptions.None);
                    if (lineNumber > 0 && lineNumber <= lines.Length)
                    {
                        int charIndex = 0;
                        for (int i = 0; i < lineNumber - 1; i++)
                        {
                            charIndex += lines[i].Length + 1; // +1 for the newline character
                        }
                        textBox.Focus(FocusState.Programmatic);
                        textBox.SelectionStart = charIndex;
                    }
                }
            }
        }

        private void AddNewTab(string header, string content, string? filePath)
        {
            stopwatch = Stopwatch.StartNew();
            var newTab = new TabContent
            {
                Header = header,
                Content = content,
                FilePath = filePath
            };
            FileTabView.TabItems.Add(newTab);
            FileTabView.SelectedItem = newTab;
            stopwatch.Stop();
            UpdateStatusBar();
            TabAddTimeTextBlock.Text = $"Tab Add Time: {stopwatch.ElapsedMilliseconds} ms";
        }

        private async void OpenFolderButton_Click(object sender, RoutedEventArgs e)
        {
            stopwatch.Restart();
            var picker = new FolderPicker();
            picker.SuggestedStartLocation = PickerLocationId.Desktop;
            picker.FileTypeFilter.Add("*");

            currentFolder = await picker.PickSingleFolderAsync();
            if (currentFolder != null)
            {
                var folderItems = await LoadFolderContentsAsync(currentFolder);
                FolderTreeView.ItemsSource = folderItems;
                stopwatch.Stop();
                TreeViewLoadTimeTextBlock.Text = $"TreeView Load Time: {stopwatch.ElapsedMilliseconds} ms";
            }
        }

        private async Task<ObservableCollection<FolderItem>> LoadFolderContentsAsync(StorageFolder folder, int currentDepth = 0, int maxDepth = 3)
        {
            var items = new ObservableCollection<FolderItem>();

            // Stop processing if the current depth exceeds the maximum depth
            if (currentDepth >= maxDepth)
            {
                return items;
            }

            // Get all subfolders and files
            var subFolders = await folder.GetFoldersAsync();
            var files = await folder.GetFilesAsync();

            // Filter files to include only .txt, .cpp, and .h
            var filteredFiles = files.Where(file =>
                file.FileType.Equals(".txt", StringComparison.OrdinalIgnoreCase) ||
                file.FileType.Equals(".cpp", StringComparison.OrdinalIgnoreCase) ||
                file.FileType.Equals(".h", StringComparison.OrdinalIgnoreCase)).ToList();

            // Add filtered files to the items collection
            foreach (var file in filteredFiles)
            {
                items.Add(new FolderItem
                {
                    Name = file.Name,
                    Path = file.Path,
                    SubItems = null
                });
            }

            // Recursively process subfolders
            foreach (var subFolder in subFolders)
            {
                var subFolderItems = await LoadFolderContentsAsync(subFolder, currentDepth + 1, maxDepth);

                // Only add the folder if it contains valid files or subfolders with valid files
                if (subFolderItems.Count > 0)
                {
                    items.Add(new FolderItem
                    {
                        Name = subFolder.Name,
                        Path = subFolder.Path,
                        SubItems = subFolderItems
                    });
                }
            }

            return items;
        }

        private async void FolderTreeView_DoubleTapped(object sender, DoubleTappedRoutedEventArgs e)
        {
            if (FolderTreeView.SelectedNodes.Count <= 0)
                return;
            // Get the clicked item
            if (FolderTreeView.SelectedNodes[0].Content is FolderItem selectedItem && selectedItem.SubItems == null)
            {
                // Assume it's a file (SubItems == null indicates it's not a folder)                
                var file = await StorageFile.GetFileFromPathAsync(selectedItem.Path);
                if (file != null)
                {
                    // Read the file content
                    string content = await FileIO.ReadTextAsync(file);

                    // Open the file in a new tab
                    AddNewTab(file.Name, content, selectedItem.Path);
                }
            }
        }
    }
}
