
# Background
The Xaml [NavigationView](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.NavigationView) control enables navigation of app content. It has a header, a view for the main content, and a menu pane for navigation commands. There is additional discussion about how to use this control in the [Navigation view](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/navigationview) design document.

Currently, NavigationView's MenuItems list allows for displaying a flat list of items in the pane. 
It's common for apps to want to present users with a hierarchical navigation tree. 
This feature adds the capability to nest items within the pane by adding MenuItems/MenuItemsSource properties to NavigationViewItem, matching the existing properties on NavigationView. Some of the new APIs are copied from analogous existing APIs on the [TreeView](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TreeView) control.

# Hierarchy
To show a hierarchical list of nested navigation items in the pane, use either the `MenuItems` property or the `MenuItemsSource` property of **NavigationViewItem**.
Each NavigationViewItem can contain other NavigationViewItems and organizing elements like item headers and separators. 
To show a hierarchical list when using `MenuItemsSource`, set the `ItemTemplate` to be a NavigationViewItem, and bind its MenuItemsSource property to the next level of the hierarchy.

Although NavigationViewItem can contain any number of nested levels, we recommend keeping your app’s navigation hierarchy shallow. 
We believe two levels is ideal for usability and comprehension.

NavigationView shows hierarchy in all its pane display modes, including Top and LeftCompact.

![HierarchicalNavigationView modes](nav-view-screenshots.png)
# Examples

## Add a hierarchy of items in markup
Declare app navigation hierarchy in markup.

```Xaml
<muxc:NavigationView>
    <muxc:NavigationView.MenuItems>
        <muxc:NavigationViewItem Content="Home" Icon="Home" ToolTipService.ToolTip="Home"/>
        <muxc:NavigationViewItem Content="Collections" Icon="Keyboard" ToolTipService.ToolTip="Collections">
            <muxc:NavigationViewItem.MenuItems>
                <muxc:NavigationViewItem Content="Notes" Icon="Page" 
            ToolTipService.ToolTip="Notes"/>
                <muxc:NavigationViewItem Content="Mail" Icon="Mail" ToolTipService.ToolTip="Mail"/>
            </muxc:NavigationViewItem.MenuItems>
        </muxc:NavigationViewItem>
    </muxc:NavigationView.MenuItems>
</muxc:NavigationView>
```

## Add a hierarchy of items using data binding

Add a hierarchy of menu items to the NavigationView by 
* binding the MenuItemsSource property to the hierarchical data
* defining the item template to be a NavigationViewMenuItem, with its Content set to be the label of the menu item, and its MenuItemsSource property bound to the next level of the hierarchy

This example also demonstrates the Expanding and Collapsing events. These events are raised for a menu item with children.

```xaml
<DataTemplate x:Key="NavigationViewMenuItem" x:DataType="local:Category">
    <muxc:NavigationViewItem Content="{x:Bind Name}" MenuItemsSource="{x:Bind Children}"/>
</DataTemplate>

<muxc:NavigationView x:Name="navview" 
    MenuItemsSource="{x:Bind categories, Mode=OneWay}" 
    MenuItemTemplate="{StaticResource NavigationViewMenuItem}" 
    ItemInvoked="{x:Bind OnItemInvoked}" 
    Expanding="OnItemExpanding" 
    Collapsed="OnItemCollapsed" 
    PaneDisplayMode="Left">
    
    <StackPanel Margin="10,10,0,0">
        <TextBlock Margin="0,10,0,0" x:Name="ExpandingItemLabel" Text="Last Expanding: N/A"/>
        <TextBlock x:Name="CollapsedItemLabel" Text="Last Collapsed: N/A"/>
    </StackPanel>    
</muxc:NavigationView>
```

```c#
public class Category
{
    public String Name { get; set; }
    public String Icon { get; set; }
    public ObservableCollection<Category> Children { get; set; }
}
    
public sealed partial class HierarchicalNavigationViewDataBinding : Page
{
    public HierarchicalNavigationViewDataBinding()
    {
        this.InitializeComponent();
    }  
    
    public ObservableCollection<Category> Categories = new ObservableCollection<Category>()
    {
        new Category(){
            Name = "Menu Item 1",
            Icon = "Icon",
            Children = new ObservableCollection<Category>() {
               new Category(){
                    Name = "Menu Item 2",
                    Icon = "Icon",
                    Children = new ObservableCollection<Category>() {
                        new Category() { 
                            Name  = "Menu Item 2", 
                            Icon = "Icon",
                            Children = new ObservableCollection<Category>() {
                                new Category() { Name  = "Menu Item 3", Icon = "Icon" },
                                new Category() { Name  = "Menu Item 4", Icon = "Icon" }
                            }
                        }
                    }
                }
            }
        },
        new Category(){
            Name = "Menu Item 5",
            Icon = "Icon",
            Children = new ObservableCollection<Category>() {
                new Category(){
                    Name = "Menu Item 6",
                    Icon = "Icon",
                    Children = new ObservableCollection<Category>() {
                        new Category() { Name  = "Menu Item 7", Icon = "Icon" },
                        new Category() { Name  = "Menu Item 8", Icon = "Icon" }
                    }
                }
            }
        },
        new Category(){ Name = "Menu Item 9", Icon = "Icon" }
    };

    private void OnItemInvoked(object sender, NavigationViewItemInvokedEventArgs e)
    {
        var clickedItem = e.InvokedItem;
        var clickedItemContainer = e.InvokedItemContainer;
    }

    private void OnItemExpanding(object sender, NavigationViewItemExpandingEventArgs e)
    {
        var nvib = e.ExpandingItemContainer;
        var name = "Last Expanding: " + nvib.Content.ToString();
        ExpandingItemLabel.Text = name;
    }

    private void OnItemCollapsed(object sender, NavigationViewItemCollapsedEventArgs e)
    {
        var nvib = e.CollapsedItemContainer;
        var name = "Last Collapsed: " + nvib.Content;
        CollapsedItemLabel.Text = name;
    }
}

```

## Selection
By default, any item can contain children, be invoked, or be selected.
When providing users with a hierarchical tree of navigation options, you may choose to make parent items non-selectable, for example when your app doesn't have a destination page associated with that parent item.
To prevent an item from showing the selection indicator when invoked, set its [SelectsOnInvoked](https://docs.microsoft.com/en-us/uwp/api/microsoft.ui.xaml.controls.navigationviewitem.selectsoninvoked?view=winui-2.3) property to False.

```xaml
<DataTemplate x:Key="NavigationViewMenuItem" x:DataType="local:Category">
    <muxc:NavigationViewItem Content="{x:Bind Name}" 
        MenuItemsSource="{x:Bind Children}"
        SelectsOnInvoked="{x:Bind IsLeaf}" />
</DataTemplate>

<muxc:NavigationView x:Name="navview" 
    MenuItemsSource="{x:Bind categories, Mode=OneWay}" 
    MenuItemTemplate="{StaticResource NavigationViewMenuItem}">
   
</muxc:NavigationView>
```

```c#
public class Category
{
    public String Name { get; set; }
    public String Icon { get; set; }
    public ObservableCollection<Category> Children { get; set; }
    public bool IsLeaf { get; set; }
}
    
public sealed partial class HierarchicalNavigationViewDataBinding : Page
{
    public HierarchicalNavigationViewDataBinding()
    {
        this.InitializeComponent();
    }      
    
    public ObservableCollection<Category> Categories = new ObservableCollection<Category>()
    {
        new Category(){
            Name = "Menu Item 1",
            Icon = "Icon",
            Children = new ObservableCollection<Category>() {
                new Category(){
                    Name = "Menu Item 2",
                    Icon = "Icon",
                    Children = new ObservableCollection<Category>() {
                        new Category() { 
                            Name  = "Menu Item 2", 
                            Icon = "Icon",
                            Children = new ObservableCollection<Category>() {
                                new Category() { Name  = "Menu Item 3", Icon = "Icon", IsLeaf = true },
                                new Category() { Name  = "Menu Item 4", Icon = "Icon", IsLeaf = true }
                            }
                        }
                    }
                }
            }
        },
        new Category(){
            Name = "Menu Item 5",
            Icon = "Icon",
            Children = new ObservableCollection<Category>() {
                new Category(){
                    Name = "Menu Item 6",
                    Icon = "Icon",
                    Children = new ObservableCollection<Category>() {
                        new Category() { Name  = "Menu Item 7", Icon = "Icon", IsLeaf = true },
                        new Category() { Name  = "Menu Item 8", Icon = "Icon", IsLeaf = true }
                    }
                }
            }
        },
        new Category(){ Name = "Menu Item 9", Icon = "Icon", IsLeaf = true }
    };
}
```

Selected items will draw their selection indicators along their left edge when in left mode or their bottom edge when in top mode. 
The selected item may not always remain visible.
For example, the selected item may be a child node inside a non-expanded subtree.
In this situation, the first visible ancestor of the selected item will show as selected, and the selection indicator will move as users expand the subtree. 
The entire navigation view will show no more than one selection indicator.

In both Top and Left modes, clicking the arrows on NavigationViewItems will expand or collapse the subtree. Clicking or tapping 
_elsewhere_ on the NavigationViewItem will trigger the `ItemInvoked` event, and it will also collapse or expand the subtree.

## Keyboarding
Users can move focus around the navigation view using their [keyboard](https://docs.microsoft.com/en-us/windows/uwp/design/input/keyboard-interactions). 
The arrow keys expose "inner navigation" within the pane and follow the interactions provided in [tree view](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/tree-view). The key actions change when navigating through the NavigationView vs. its flyout menu, which is displayed in Top and Left-compact modes of HierarchicalNavigationView.

### Up Arrow
When in Left Nav or Flyout, moves focus to the item directly above the item currently in focus
When in Top Nav, does nothing

### Down Arrow
When in Left Nav or Flyout, moves focus the item directly below the item currently in focus; note that the items do not need to be visually adjecent, focus will move from the last item in the pane's list to the settings item.
When in Top Nav, does nothing

### Right Arrow
When in Top Nav, moves focus to the item directly to the right of the item currently in focus
When in Left Nav or Flyout, does nothing

#### Left Arrow
When in Top Nav, moves focus to the item directly to the left the item currently in focus
When in Left Nav or Flyout, does nothing

### Space/Enter Key
Invokes/Selects item
When in Left Expanded mode and item has children, expands/collapses item and does not change focus
When in Left Compact/Top mode and item has children, expands children into a flyout and places focus on first item in flyout
When in flyout and on leaf node, invokes/selects item and closes flyout

### Esc Key
When in flyout, closes the flyout

# Remarks
<!-- Explanation and guidance that doesn't fit into the Examples
section.  For example, see the Remarks for the MediaPlayerElement 
(https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.MediaPlayerElement#remarks). -->


# API Notes
<!-- Give a one or two line description of each API (type
and member), or at least the ones that aren't obvious
from their name.  These descriptions are what show up
in IntelliSense. -->
| API | Description | Notes |
| - | - | - | 
| MenuItems property | Gets the collection of menu items displayed as children of the NavigationViewItem. | |
| MenuItemsSource property | Gets or sets an object source used to generate the children of the NavigationViewItem | |
| IsChildSelected property | A descendant item is selected | |
| IsExpanded property | Gets or sets a value that indicates whether a tree node is expanded. Ignored if there are no menu items. | Analogous to [TreeViewItem.IsExpanded](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TreeViewItem.IsExpanded) |
| HasUnrealizedChildren property | Gets or sets a value that indicates whether the current item has child items that haven't been shown. | Analogous to [TreeViewItem.HasUnrealizedChildren](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TreeViewItem.HasUnrealizedChildren) |
| Expand method |  Expands the specified node in the tree. | Analogous to [TreeView.Expand](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TreeView.Expand) |
| Collapse method |  Collapses the specified node in the tree. | Analogous to [TreeView.Collapse](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TreeView.Collapse) |
| Expanding event | Occurs when a node in the tree starts to expand. | Analogous to [TreeView.Expanding](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TreeView.Expanding). In order to fill in nodes as they're expanding, set the `HasUnrealizedChildren` property to true, and then add the children during this `Expanding` event. See the TreeView example [fill a node when it's expanding](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/tree-view#fill-a-node-when-its-expanding).|
| Collapsed event | Occurs when a node in the tree is collapsed. |  Analogous to [TreeView.Collapsed](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TreeView.Collapsed) |
| NavigationViewItemExpandingEventArgs | Provides event data for NavigationView.Expanding event.  | |
| NavigationViewItemCollapsedEventArgs | Provides event data for NavigationView.Collapsed event.  | |

# API Details
```c++
[WUXC_VERSION_MUXONLY]
[webhosthidden]
runtimeclass NavigationViewItemExpandingEventArgs
{
    NavigationViewItemBase ExpandingItemContainer { get; };
    Object ExpandingItem{ get; };
}

[WUXC_VERSION_MUXONLY]
[webhosthidden]
runtimeclass NavigationViewItemCollapsedEventArgs
{
    NavigationViewItemBase CollapsedItemContainer { get; };
    Object CollapsedItem{ get; };
}

[WUXC_VERSION_RS3]
[webhosthidden]
[WUXC_INTERFACE_NAME("INavigationView", f209ce15-391a-42ca-9fc6-f79da65aca32)]
[WUXC_STATIC_NAME("INavigationViewStatics", 363a86c7-72da-4420-b871-15d9d0d45756)]
[WUXC_CONSTRUCTOR_NAME("INavigationViewFactory", e50687c1-b7c2-4975-ad7a-5f4fe6a514c9)]
[MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
[MUX_PROPERTY_CHANGED_CALLBACK_METHODNAME("OnPropertyChanged")]
unsealed runtimeclass NavigationView : Windows.UI.Xaml.Controls.ContentControl
{
...
    [WUXC_VERSION_PREVIEW]
    {
        event Windows.Foundation.TypedEventHandler<NavigationView, NavigationViewExpandingEventArgs> Expanding;
        event Windows.Foundation.TypedEventHandler<NavigationView, NavigationViewCollapsedEventArgs> Collapsed;

        void Expand(NavigationViewItem item);
        void Collapse(NavigationViewItem item);
    }
}

[WUXC_VERSION_RS3]
[webhosthidden]
[WUXC_INTERFACE_NAME("INavigationViewItemBase", edf04eb1-37d1-471f-8570-3829ee5b2bc6)]
[WUXC_CONSTRUCTOR_NAME("INavigationViewItemBaseFactory", eb014cef-7890-4ebb-8245-02e8510f321d)]
[default_interface]
unsealed runtimeclass NavigationViewItemBase : Windows.UI.Xaml.Controls.ContentControl
{
    [WUXC_VERSION_PREVIEW]
    {
        Boolean IsSelected{ get; set; };
        static Windows.UI.Xaml.DependencyProperty IsSelectedProperty { get; };
    }
}

[WUXC_VERSION_RS3]
[webhosthidden]
[WUXC_INTERFACE_NAME("INavigationViewItem", 8614be0f-b7b6-4851-960a-f5e3f69f624a)]
[WUXC_STATIC_NAME("INavigationViewItemStatics", 803c0081-fda5-4b90-aace-3f2306dbe5c4)]
[WUXC_CONSTRUCTOR_NAME("INavigationViewItemFactory", 973bdb4a-7e08-4f76-923c-f12bd685e86e)]
unsealed runtimeclass NavigationViewItem : NavigationViewItemBase
{
...
    [WUXC_VERSION_PREVIEW]
    {
        [MUX_DEFAULT_VALUE("false")]
        Boolean IsExpanded{ get; set; };
        [MUX_DEFAULT_VALUE("false")]
        Boolean HasUnrealizedChildren{ get; set; };
        [MUX_DEFAULT_VALUE("false")]
        Boolean IsChildSelected{ get; };

        [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
        Windows.Foundation.Collections.IVector<Object> MenuItems{ get; };
        [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
        Object MenuItemsSource{ get; set; };

        static Windows.UI.Xaml.DependencyProperty IsExpandedProperty{ get; };
        static Windows.UI.Xaml.DependencyProperty HasUnrealizedChildrenProperty{ get; };
        static Windows.UI.Xaml.DependencyProperty IsChildSelectedProperty{ get; };
        static Windows.UI.Xaml.DependencyProperty MenuItemsProperty{ get; };
        static Windows.UI.Xaml.DependencyProperty MenuItemsSourceProperty{ get; };
    }
}

```

# Appendix
<!-- Anything else that you want to write down for posterity, but 
that isn't necessary to understand the purpose and usage of the API.
For example, implementation details. -->
