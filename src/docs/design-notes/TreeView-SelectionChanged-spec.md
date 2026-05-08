TreeView.SelectionChanged API spec
===

# Background

The Xaml
[TreeView](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.TreeView)
control allows the display of a collection a data objects in a tree configuration:

```xml
<TreeView
    ItemsSource="{x:Bind Folders}"
    SelectionMode="Multiple" />
```

It currently allows developers to specify whether and how nodes can be selected and to retrieve
the set of selected nodes, but it does not allow developers to attach an event handler to the
moment the selection changes.  This spec aims to fix that.

# Conceptual pages (How To)

The `TreeView.SelectionChanged` event allows developers to respond to the occurrence that the user
or code-behind changes the set of selected nodes in the `TreeView` control.

XAML
```xml
<Page
    x:Class="FolderTreeViewApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">

    <Grid>
        <Grid.RowDefinitions>
            <RowDefinition />
            <RowDefinition Height="Auto" />
        </Grid.RowDefinitions>

    <TreeView
        ItemsSource="{x:Bind Folders}"
        SelectionMode="Multiple"
        SelectionChanged="OnTreeViewSelectionChanged" />

    <StackPanel Orientation="Horizontal">
        <TextBlock Text="{x:Bind SelectedFolders.Count}" />
        <TextBlock Text=" folder(s) selected." />
    </StackPanel>
</Page>
```

C#
```cs
namespace FolderTreeViewApp
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
            this.PopulateFolders();
            this.SelectedFolders = new ObservableCollection<Folder>();
        }

        public ObservableCollection<Folder> Folders
        {
            get;
            private set;
        }

        public ObservableCollection<Folder> SelectedFolders
        {
            get;
            private set;
        }

        public void OnTreeViewSelectionChanged(TreeView sender, TreeViewSelectionChangedEventArgs args)
        {
            foreach (object item in args.RemovedItems)
            {
                this.SelectedFolders.Remove((Folder)item);
            }

            foreach (object item in args.AddedItems)
            {
                this.SelectedFolders.Add((Folder)item);
            }
        }

        private void PopulateFolders()
        {
            // Populates the this.Folders collection 
        }
    }
    
    public class Folder
    {
        public string DiskPath
        {
            get;
            set;
        }
    }
```





# API pages

## TreeViewSelectionChangedEventArgs class

Represents the event args associated with the `TreeView.SelectionChanged` event.
Contains a list of the data objects added to and removed from the list of selected items.

Namespace: Microsoft.UI.Xaml.Controls

C#
```cs
public class TreeViewSelectionChangedEventArgs
```

## TreeView.SelectionChanged event

The `TreeView` control raises this event in response to selection changing,
which corresponds to the
[TreeView.SelectedItem](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.TreeView.SelectedItem),
[TreeView.SelectedItems](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.TreeView.SelectedItems),
and
[TreeView.SelectedNodes](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.TreeView.SelectedNodes)
properties.


C#
```cs
public event TypedEventHandler<TreeView, TreeViewSelectionChangedEventArgs> SelectionChanged;
```




# API Details

```cs
namespace Microsoft.UI.Xaml.Controls
{
    runtimeclass TreeViewSelectionChangedEventArgs
    {
        Windows.Foundation.Collections.IVector<Object> AddedItems{ get; };
        Windows.Foundation.Collections.IVector<Object> RemovedItems{ get; };
    }
}
```

```cs
namespace Microsoft.UI.Xaml.Controls
{
    unsealed runtimeclass TreeView : Microsoft.UI.Xaml.Controls.Control
    {
        // existing APIs snipped

        event Windows.Foundation.TypedEventHandler<TreeView, TreeViewSelectionChangedEventArgs> SelectionChanged;
    }
}
```