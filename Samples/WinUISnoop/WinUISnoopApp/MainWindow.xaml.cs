using System;
using System.Collections.Generic;
using System.Linq;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System.Collections.ObjectModel;
using WinUISnoopApp.SnoopData;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.IO;
using System.Text;
using Windows.ApplicationModel.DataTransfer;
using WinRT.Interop;

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the public WinUI documentation.

namespace WinUISnoopApp
{
    public sealed partial class MainWindow : Window, System.ComponentModel.INotifyPropertyChanged
    {
        private AppWindow m_appWindow;
        public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;
        private void NotifyPropertyChanged([CallerMemberName] String propertyName = null)
        {
            PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
        }

        private ObservableCollection<TreeItem> DataSource = new ObservableCollection<TreeItem>();

        // PropertiesList holds the full, unfiltered property list for the currently selected element.
        // DisplayedProperties is the filtered subset actually bound to the ListView. We keep both so the
        // filter can be applied/reapplied cheaply without re-fetching from the tap.
        private ObservableCollection<ElementProperty> PropertiesList = new ObservableCollection<ElementProperty>();
        public ObservableCollection<ElementProperty> DisplayedProperties { get; } = new ObservableCollection<ElementProperty>();

        private string m_propertyNameFilter = string.Empty;
        public string PropertyNameFilter
        {
            get { return m_propertyNameFilter; }
            set
            {
                string normalizedValue = value ?? string.Empty;
                if (m_propertyNameFilter != normalizedValue)
                {
                    m_propertyNameFilter = normalizedValue;
                    NotifyPropertyChanged();
                    RefreshDisplayedProperties();
                }
            }
        }

        private TreeItem m_selectedElement;
        public TreeItem SelectedElement
        {
            get { return m_selectedElement; }

            set
            {
                if (m_selectedElement != value)
                {
                    m_selectedElement = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private ElementProperty m_selectedProperty;
        public ElementProperty SelectedProperty
        {
            get { return m_selectedProperty; }

            set
            {
                if (m_selectedProperty != value)
                {
                    m_selectedProperty = value;
                    NotifyPropertyChanged();
                    SelectedPropertyName = value?.Name ?? string.Empty;
                    PropertyValueText = value?.Value ?? string.Empty;
                }
            }

        }

        private string m_selectedPropertyName = string.Empty;
        public string SelectedPropertyName
        {
            get { return m_selectedPropertyName; }
            set
            {
                string normalizedValue = value ?? string.Empty;
                if (m_selectedPropertyName != normalizedValue)
                {
                    m_selectedPropertyName = normalizedValue;
                    NotifyPropertyChanged();
                }
            }
        }

        private string m_propertyValueText = string.Empty;
        public string PropertyValueText
        {
            get { return m_propertyValueText; }
            set
            {
                string normalizedValue = value ?? string.Empty;
                if (m_propertyValueText != normalizedValue)
                {
                    m_propertyValueText = normalizedValue;
                    NotifyPropertyChanged();
                }
            }
        }

        private string m_themeToggleTooltip = "Switch to dark theme";
        public string ThemeToggleTooltip
        {
            get { return m_themeToggleTooltip; }
            private set
            {
                if (m_themeToggleTooltip != value)
                {
                    m_themeToggleTooltip = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public MainWindow()
        {
            this.InitializeComponent();
            this.Closed += MainWindow_Closed;

            this.Title = "WinUI Snoop";
            ConfigureWindowChrome();
            RootLayout.ActualThemeChanged += RootLayout_ActualThemeChanged;
            UpdateThemeToggleLabel();
        }

        private void MainWindow_Closed(object sender, WindowEventArgs args)
        {
            tapHandler?.Stop();
            tapHandler = null;
        }

        private void ConfigureWindowChrome()
        {
            m_hwnd = WindowNative.GetWindowHandle(this);
            WindowId windowId = Win32Interop.GetWindowIdFromWindow(m_hwnd);
            m_appWindow = AppWindow.GetFromWindowId(windowId);

            this.SystemBackdrop = new MicaBackdrop();

            if (AppWindowTitleBar.IsCustomizationSupported() && m_appWindow != null)
            {
                this.ExtendsContentIntoTitleBar = true;
                this.SetTitleBar(TitleBarDragRegion);

                m_appWindow.TitleBar.PreferredHeightOption = TitleBarHeightOption.Standard;
                m_appWindow.TitleBar.ButtonBackgroundColor = Colors.Transparent;
                m_appWindow.TitleBar.ButtonInactiveBackgroundColor = Colors.Transparent;
                UpdateTitleBarInsets();
            }
        }

        private void UpdateTitleBarInsets()
        {
            if (m_appWindow == null)
            {
                return;
            }

            LeftInsetColumn.Width = new GridLength(m_appWindow.TitleBar.LeftInset);
            RightInsetColumn.Width = new GridLength(m_appWindow.TitleBar.RightInset);
        }

        private void RootLayout_ActualThemeChanged(FrameworkElement sender, object args)
        {
            UpdateThemeToggleLabel();
        }

        private void UpdateThemeToggleLabel()
        {
            bool isDarkTheme = RootLayout.ActualTheme == ElementTheme.Dark;
            ThemeToggleTooltip = isDarkTheme ? "Switch to light theme" : "Switch to dark theme";
        }

        private void ThemeToggleButton_Click(object sender, RoutedEventArgs e)
        {
            RootLayout.RequestedTheme = RootLayout.ActualTheme == ElementTheme.Dark ? ElementTheme.Light : ElementTheme.Dark;
            UpdateThemeToggleLabel();
        }

        private void PickPressed(object sender, PointerRoutedEventArgs args)
        {
            if (!((UIElement)sender).CapturePointer(args.Pointer))
                return;

            if (m_hwnd == IntPtr.Zero)
            {
                m_hwnd = HwndHelpers.FindMainWindowHandle();
                System.Diagnostics.Debug.Assert(m_hwnd != IntPtr.Zero);
            }
            HwndHelpers.ShowWindow(m_hwnd, HwndHelpers.SW_HIDE);
            // TODO: Set Cross cursor on the Pick element (will need a custom subclass to really do this):
            // ((UIElement)sender).ProtectedCursor = new CoreCursor(CoreCursorType.Cross, 0);
        }

        private void PickCaptureLost(object sender, PointerRoutedEventArgs args)
        {
            HwndHelpers.ShowWindow(m_hwnd, HwndHelpers.SW_SHOWNORMAL);
        }

        Process m_lastPickedProcess;
        private void PickReleased(object sender, PointerRoutedEventArgs args)
        {
            if (m_lastPickedProcess != null)
            {
                ConnectOnBackgroundThread(m_lastPickedProcess);
            }
        }

        private ContentDialog m_bitnessIssueDialog = new ContentDialog();

        private void PickMoved(object sender, PointerRoutedEventArgs args)
        {
            if (!args.Pointer.IsInContact)
                return;

            Process targetProcess = HwndHelpers.GetProcessAtCursorPos();
            if (targetProcess != null)
            {
                bool containsWinUI = false;
                bool bitnessIssue = false;
                try
                {
                    foreach (var module in targetProcess.Modules)
                    {
                        if ((module as ProcessModule).ModuleName.Equals("Microsoft.UI.Xaml.dll", StringComparison.InvariantCultureIgnoreCase))
                        {
                            containsWinUI = true;
                            break;
                        }
                    }
                }
                catch (System.ComponentModel.Win32Exception e)
                {
                    // Might be a 64-bit process, which we can't inspect if Snoop is 32-bit.
                    bitnessIssue = targetProcess.Is64Bit() && IntPtr.Size == 4;
                    System.Diagnostics.Debug.Assert(bitnessIssue, e.Message);
                }
                m_bitnessIssueDialog.Hide();
                if (bitnessIssue)
                {
                    tbConnectedStatus.Text = "Target: " + targetProcess.ProcessName + " (" + targetProcess.Id + ")" + " (unable to connect)";
                    m_bitnessIssueDialog.Title = "Unsupported connection";
                    m_bitnessIssueDialog.Content = "Target process is 64-bit while WinUI Snoop is 32-bit. Cross-bitness snooping is not currently supported.";
                    m_bitnessIssueDialog.CloseButtonText = "OK";
                    m_bitnessIssueDialog.XamlRoot = this.Content.XamlRoot;
                    _ = m_bitnessIssueDialog.ShowAsync();
                }
                else
                {
                    tbConnectedStatus.Text = "Target: " + targetProcess.ProcessName + " (" + targetProcess.Id + ")" + (containsWinUI ? "" : " (no WinUI)");
                }
                System.Diagnostics.Debug.WriteLine(tbConnectedStatus.Text);
                m_lastPickedProcess = containsWinUI ? targetProcess : null;
            }
            else
            {
                tbConnectedStatus.Text = "Target not found";
                System.Diagnostics.Debug.WriteLine(tbConnectedStatus.Text);
                m_lastPickedProcess = null;
            }
        }

        private void PropertiesPane_PropertyValueSubmitted(object sender, EventArgs e)
        {
            var contentDialog = new ContentDialog()
            {
                Title = "Not yet implemented",
                Content = "The ability to set property values is not yet implemented.",
                CloseButtonText = "OK"
            };
            contentDialog.XamlRoot = this.Content.XamlRoot;
            _ = contentDialog.ShowAsync();
        }

        TreeItem FindItem(ObservableCollection<TreeItem> collection, string element)
        {
            foreach (var item in collection)
            {
                if (item.Element == element)
                    return item;

                var childItem = FindItem(item.Children, element);
                if (childItem != null)
                    return childItem;
            }

            return null;
        }

        TreeItem FindItem(string element)
        {
            return FindItem(this.DataSource, element);
        }

        public void HandleConnected()
        {
            tbConnectedStatus.Text = "Connected to " + tapHandler.ConnectProcess.ProcessName + " (" + tapHandler.ConnectProcess.Id + ")";
        }

        public void HandleVisualTreeChange(TAPVisualTreeChange visualTreeChange)
        {
            if (visualTreeChange.MutationType == "Add")
            {
                string element = visualTreeChange.Element;
                string shortType = visualTreeChange.ElementType;
                int dotIndex = shortType.LastIndexOf('.');
                if (dotIndex >= 0)
                {
                    shortType = shortType.Substring(dotIndex + 1);
                }
                string name = shortType + " (" + element + ")";
                TreeItem newItem = new TreeItem(element, name, visualTreeChange.ElementType);

                if (visualTreeChange.IsParentNull())
                {
                    // Add directly
                    this.DataSource.Add(newItem);
                }
                else
                {
                    // Search for the parent.
                    TreeItem parent = FindItem(visualTreeChange.Parent);
                    if (parent != null)
                    {
                        parent.Children.Add(newItem);
                    }
                }
            }
            else
            {
                System.Diagnostics.Debug.Assert(visualTreeChange.MutationType == "Remove");
                // TODO: Implement this case to remove the relevant element from the tree.
                //       Consider whether we want the tree to always be live, or if we want
                //       refreshing to be user-driven (which can be helpful when trying to
                //       examine the state of the app at one point even when it may still be
                //       changing).
            }
        }

        private void UpdatePropertiesList()
        {
            int propertiesListIndex = 0;
            for (int i = 0; i < SelectedElement.Properties.Count; i++)
            {
                var prop = SelectedElement.Properties[i];
                if (propertiesListIndex >= PropertiesList.Count)
                {
                    // At the end of PropertiesList, so add the new property.
                    PropertiesList.Add(prop);
                    propertiesListIndex++;
                }
                else
                {
                    var currentProperty = PropertiesList[propertiesListIndex];
                    int compare = currentProperty.Name.CompareTo(prop.Name);
                    if (compare == 0)
                    {
                        // Already have a property with that name. Update its data.
                        currentProperty.CopyFrom(prop);
                        propertiesListIndex++;
                    }
                    else if (compare < 0)
                    {
                        // New list doesn't have this property. Remove it.
                        PropertiesList.RemoveAt(propertiesListIndex);
                        i--; // decrement to re-run this iteration
                    }
                    else
                    {
                        // New property for the list. Insert it.
                        PropertiesList.Insert(propertiesListIndex, prop);
                        propertiesListIndex++;
                    }
                }
            }
            while (propertiesListIndex < PropertiesList.Count)
            {
                // This property at the end is not in the element's list. Remove it.
                PropertiesList.RemoveAt(propertiesListIndex);
            }

            RefreshDisplayedProperties();
            if (SelectedProperty != null)
            {
                PropertyValueText = SelectedProperty.Value ?? string.Empty;
            }

#if DEBUG
            if (SelectedElement.Properties.Count != PropertiesList.Count)
            {
                for (int i = 0; i < SelectedElement.Properties.Count; i++)
                    System.Diagnostics.Debug.WriteLine("SelectedElement.Properties[" + i + "].Name=" + SelectedElement.Properties[i].Name);
                for (int i = 0; i < PropertiesList.Count; i++)
                    System.Diagnostics.Debug.WriteLine("PropertiesList[" + i + "].Name=" + PropertiesList[i].Name);
            }
            System.Diagnostics.Debug.Assert(SelectedElement.Properties.Count == PropertiesList.Count);
#endif
        }

        // Rebuild DisplayedProperties from PropertiesList, applying the current name filter.
        // Cheap to call: at most a few hundred items.
        private void RefreshDisplayedProperties()
        {
            DisplayedProperties.Clear();
            string filter = m_propertyNameFilter;
            bool hasFilter = !string.IsNullOrEmpty(filter);
            foreach (var prop in PropertiesList)
            {
                if (!hasFilter || (prop.Name != null && prop.Name.IndexOf(filter, StringComparison.OrdinalIgnoreCase) >= 0))
                {
                    DisplayedProperties.Add(prop);
                }
            }
        }

        // Pending subtree export completion. Maps root element handle -> TCS.
        private Dictionary<string, TaskCompletionSource<bool>> m_pendingSubTreeRequests
            = new Dictionary<string, TaskCompletionSource<bool>>();

        public void HandleElementProperties(TAPElementProperties elementProperties)
        {
            // Find the tree item this response belongs to
            TreeItem target = FindItem(elementProperties.Element);
            if (target == null)
                target = SelectedElement;

            if (target != null)
            {
                target.Properties.Clear();
                foreach (var prop in elementProperties.Properties.OrderBy(p => p.Name))
                {
                    target.Properties.Add(new ElementProperty(prop.Name) { Value = prop.Value, Source = prop.Source });
                }
            }

            // Update the UI properties list if this is the currently selected element
            if (target != null && ReferenceEquals(target, SelectedElement))
            {
                UpdatePropertiesList();
            }
        }

        public void HandleSubTreePropertiesDone(string rootElement)
        {
            if (m_pendingSubTreeRequests.TryGetValue(rootElement, out var tcs))
            {
                m_pendingSubTreeRequests.Remove(rootElement);
                tcs.TrySetResult(true);
            }
        }

        void TreeSelectionChanged(object sender, TreeViewSelectionChangedEventArgs args)
        {
            // Prefer args.AddedItems over treeView.SelectedItem: in WinUI's TreeView the
            // SelectedItem property can lag behind the SelectionChanged event on the very
            // first selection, which previously caused the properties pane to stay empty
            // until the user clicked a *second* node.
            TreeItem selectedItem = null;
            if (args.AddedItems != null && args.AddedItems.Count > 0)
            {
                selectedItem = args.AddedItems[0] as TreeItem;
            }
            if (selectedItem == null)
            {
                selectedItem = this.treeView.SelectedItem as TreeItem;
            }

            if (tapHandler != null && selectedItem != null)
            {
                this.SelectedElement = selectedItem;
                this.SelectedProperty = null;

                // Repopulate the bound list immediately from whatever we have cached for this
                // element (typically empty on the first click). The tap response will refresh
                // it again as soon as it arrives, but doing this now guarantees the ListView
                // reflects the newly-selected element without waiting for the round trip.
                PropertiesList.Clear();
                foreach (var p in selectedItem.Properties)
                {
                    PropertiesList.Add(p);
                }
                RefreshDisplayedProperties();

                tapHandler.WriteToTap("GET-PROPERTIES: " + selectedItem.Element);
            }
        }

        // ----- Hover-to-highlight in target app -----
        // The TreeView event we hook is PointerMoved (continuous) rather than
        // per-item PointerEntered/Exited: those would only ever fire on the deepest
        // hovered TreeViewItem and we'd lose the highlight when the pointer moves
        // from a child back into the parent's exposed area. Hit-testing on every
        // move and only acting on transitions is simpler and tracks correctly.
        private TreeItem m_hoverHighlightedItem;

        private void TreeViewPointerMoved(object sender, PointerRoutedEventArgs e)
        {
            if (tapHandler == null) return;

            TreeItem hovered = null;
            try
            {
                var screenPt = e.GetCurrentPoint(null).Position;
                var elements = Microsoft.UI.Xaml.Media.VisualTreeHelper.FindElementsInHostCoordinates(screenPt, treeView);
                foreach (var el in elements)
                {
                    if (el is FrameworkElement fe && fe.DataContext is TreeItem ti)
                    {
                        hovered = ti;
                        break; // deepest match first
                    }
                }
            }
            catch
            {
                // Hit-testing can throw if visual tree is mid-update; ignore.
            }

            if (!ReferenceEquals(hovered, m_hoverHighlightedItem))
            {
                m_hoverHighlightedItem = hovered;
                if (hovered != null)
                {
                    tapHandler.WriteToTap("HIGHLIGHT: " + hovered.Element);
                }
                else
                {
                    tapHandler.WriteToTap("CLEAR-HIGHLIGHT");
                }
            }
        }

        private void TreeViewPointerExited(object sender, PointerRoutedEventArgs e)
        {
            if (m_hoverHighlightedItem != null)
            {
                m_hoverHighlightedItem = null;
                tapHandler?.WriteToTap("CLEAR-HIGHLIGHT");
            }
        }

        TapHandler tapHandler;

        private void ConnectOnBackgroundThread(Process connectProcess)
        {
            this.DataSource.Clear();
            this.PropertiesList.Clear();
            this.DisplayedProperties.Clear();
            this.SelectedElement = null;
            this.SelectedProperty = null;

            tbConnectedStatus.Text = "Connecting to " + connectProcess.ProcessName + " (" + connectProcess.Id + ")";

            tapHandler = new TapHandler(this, connectProcess);
            tapHandler.Start();
        }

        IntPtr m_hwnd = IntPtr.Zero;

        // ----- Context menu handlers -----

        private void CopyInstanceId_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuFlyoutItem item && item.Tag is TreeItem treeItem)
            {
                var dataPackage = new DataPackage();
                dataPackage.SetText(treeItem.Element);
                Clipboard.SetContent(dataPackage);
            }
        }

        private async void ExpandSubTree_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuFlyoutItem item && item.Tag is TreeItem treeItem)
            {
                // Expand level-by-level, yielding between levels so the UI
                // can realize child containers before we try to expand them.
                var currentLevel = new List<TreeItem> { treeItem };
                while (currentLevel.Count > 0)
                {
                    var nextLevel = new List<TreeItem>();
                    foreach (var node in currentLevel)
                    {
                        node.IsExpanded = true;
                        if (node.Children != null)
                        {
                            nextLevel.AddRange(node.Children);
                        }
                    }
                    await Task.Delay(1);
                    currentLevel = nextLevel;
                }
            }
        }

        private void CollapseSubTree_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuFlyoutItem item && item.Tag is TreeItem treeItem)
            {
                CollapseSubTree(treeItem);
            }
        }

        private static void CollapseSubTree(TreeItem item)
        {
            item.IsExpanded = false;
            if (item.Children != null)
            {
                foreach (var child in item.Children)
                {
                    CollapseSubTree(child);
                }
            }
        }

        private void ExportElementXaml_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuFlyoutItem item && item.Tag is TreeItem treeItem)
            {
                XamlExporter.Export(treeItem, includeDefaultProperties: false);
            }
        }

        private void ExportElementXamlAll_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuFlyoutItem item && item.Tag is TreeItem treeItem)
            {
                XamlExporter.Export(treeItem, includeDefaultProperties: true);
            }
        }

        private void ExportSubTreeXaml_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuFlyoutItem item && item.Tag is TreeItem treeItem)
            {
                _ = FetchAndExportSubTreeAsync(treeItem, includeDefaultProperties: false);
            }
        }

        private void ExportSubTreeXamlAll_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuFlyoutItem item && item.Tag is TreeItem treeItem)
            {
                _ = FetchAndExportSubTreeAsync(treeItem, includeDefaultProperties: true);
            }
        }

        private async Task FetchAndExportSubTreeAsync(TreeItem root, bool includeDefaultProperties)
        {
            if (tapHandler != null)
            {
                // Send a single command; the tap walks the entire subtree, sends
                // one ElementProperties message per node, then a SubTreePropertiesDone.
                var tcs = new TaskCompletionSource<bool>();
                m_pendingSubTreeRequests[root.Element] = tcs;
                tapHandler.WriteToTap("GET-SUBTREE-PROPERTIES: " + root.Element);

                // Wait for completion or timeout
                await Task.WhenAny(tcs.Task, Task.Delay(TimeSpan.FromSeconds(60)));
                m_pendingSubTreeRequests.Remove(root.Element);
            }

            XamlExporter.ExportSubTree(root, includeDefaultProperties);
        }
    }
}
