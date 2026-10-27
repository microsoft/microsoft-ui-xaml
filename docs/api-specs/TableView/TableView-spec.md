TableView control
===

# Background

`TableView` displays a collection as rows and a set of developer-defined columns as cells, with column headers, sorting, single-row selection, opt-in cell editing, and filtering, grouping and hierarchy declared through `TableViewSource`.

WinUI 3 has no in-box tabular control. Apps that need a table today either compose one from `ListView` or `ItemsRepeater` (and re-implement headers, column sizing, alignment, keyboard navigation and UI Automation themselves), or take a dependency on the Windows Community Toolkit `DataGrid`.

`TableView` targets read-mostly tables with a moderate number of columns (roughly 5–50):

- File and property lists (name, type, size, date modified).
- Task-manager-style metric tables, sorted by the largest value.
- Admin and settings grids with occasional in-place edits.

`TableView` is not a replacement for WPF's `DataGrid` and is not a spreadsheet. Compared with `ListView` and `ItemsView`, it adds a column model (headers, widths, frozen columns, per-column sort and edit policy) and grid/table UI Automation. Rows are virtualized, so large row counts stay responsive.

The control is in preview. Its types are in the `Microsoft.UI.Xaml.Controls.Tabular` namespace, and an app must merge `TabularControlsResources` into its application resources (see [Display data in a table](#display-data-in-a-table)).

**In this release**

- Text and template columns with Pixel, Auto and Star widths, min/max clamps, and leading frozen columns.
- Column resize by pointer, keyboard and UI Automation.
- Single-column sort by header click or API, with a per-column direction cycle and custom comparers.
- Single row selection (`None` or `Single`).
- Opt-in, cell-scoped editing with a built-in text editor or a custom editing template.
- Filtering, single-level grouping and parent/child hierarchy through `TableViewSource`.
- Empty-state template, density presets, grid lines, row banding, tooltips and context menus.

For features deliberately left out of this release, see [Not in this release](#not-in-this-release).

# Conceptual pages (How To)

## Display data in a table

Merge `TabularControlsResources` once, in App.xaml. Without it, `TableView` fails to activate.

```xaml
<Application.Resources>
    <ResourceDictionary>
        <ResourceDictionary.MergedDictionaries>
            <XamlControlsResources xmlns="using:Microsoft.UI.Xaml.Controls" />
            <TabularControlsResources xmlns="using:Microsoft.UI.Xaml.Controls.Tabular" />
        </ResourceDictionary.MergedDictionaries>
    </ResourceDictionary>
</Application.Resources>
```

Set `ItemsSource` and add columns. `Columns` is the content property, so columns can be declared directly inside the element. The examples in this spec use `xmlns:tabular="using:Microsoft.UI.Xaml.Controls.Tabular"`.

```xaml
<tabular:TableView x:Name="OrdersTable"
                   ItemsSource="{x:Bind Orders}"
                   AutomationProperties.Name="Orders"
                   Density="Compact"
                   GridLinesVisibility="Horizontal">

    <tabular:TableView.EmptyTemplate>
        <DataTemplate>
            <TextBlock Margin="24" Text="No orders match the current view." />
        </DataTemplate>
    </tabular:TableView.EmptyTemplate>

    <!-- Leading frozen prefix: stays in place while the body scrolls horizontally. -->
    <tabular:TableViewTextColumn Header="Order" Binding="{Binding OrderNumber}"
                                 Width="100" FrozenEdge="Leading" />
    <tabular:TableViewTextColumn Header="Customer" Binding="{Binding Customer}"
                                 Width="*" MinWidth="160" />
    <tabular:TableViewTextColumn Header="Region" Binding="{Binding Region}" Width="Auto" />

    <tabular:TableViewTemplateColumn Header="Status" Width="120" SortMemberPath="Status">
        <tabular:TableViewTemplateColumn.CellTemplate>
            <DataTemplate>
                <Border Padding="8,2" CornerRadius="10" HorizontalAlignment="Left"
                        Background="{ThemeResource AccentFillColorDefaultBrush}">
                    <TextBlock Text="{Binding Status}"
                               Foreground="{ThemeResource TextOnAccentFillColorPrimaryBrush}" />
                </Border>
            </DataTemplate>
        </tabular:TableViewTemplateColumn.CellTemplate>
    </tabular:TableViewTemplateColumn>
</tabular:TableView>
```

```csharp
public sealed partial class OrdersPage : Page
{
    public ObservableCollection<Order> Orders { get; } =
    [
        new() { OrderNumber = "SO-1001", Customer = "Tailspin Toys", Region = "West", Status = "Ready", Total = 1250m },
        new() { OrderNumber = "SO-1002", Customer = "Contoso", Region = "North", Status = "In review", Total = 980m },
        new() { OrderNumber = "SO-1003", Customer = "Fabrikam", Region = "East", Status = "Blocked", Total = 4300m },
    ];

    public OrdersPage() => InitializeComponent();
}

// The model used by every example in this spec. Editable cells need settable properties,
// and PropertyChanged keeps displayed cells (and live shaping) up to date.
public sealed partial class Order : INotifyPropertyChanged
{
    private string _orderNumber = "", _customer = "", _region = "", _status = "", _notes = "";
    private decimal _total;
    private DateTime _orderDate = DateTime.Today;
    private bool _isActive = true, _isShipped;

    public string OrderNumber { get => _orderNumber; set => Set(ref _orderNumber, value); }
    public string Customer { get => _customer; set => Set(ref _customer, value); }
    public string Region { get => _region; set => Set(ref _region, value); }
    public string Status { get => _status; set => Set(ref _status, value); }
    public decimal Total { get => _total; set => Set(ref _total, value); }
    public DateTime OrderDate { get => _orderDate; set => Set(ref _orderDate, value); }
    public string Notes { get => _notes; set => Set(ref _notes, value); }
    public bool IsActive { get => _isActive; set => Set(ref _isActive, value); }
    public bool IsShipped { get => _isShipped; set => Set(ref _isShipped, value); }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Set<T>(ref T field, T value, [CallerMemberName] string? name = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value)) return;
        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }
}
```

- **Columns.** `TableViewTextColumn.Binding` is a CLR property that takes a normal `{Binding}`; it is not a dependency property. `TableViewTemplateColumn.CellTemplate` supplies any visual. Cell content must bind to its inherited `DataContext` (the row item); the control reuses cell elements for different items, so content that stores the item or sets its own `DataContext` can show the wrong item.
- **Width.** `Width` is a `GridLength` (default 120 pixels). Pixel is exact. `Auto` sizes to the header and the widest *realized* cell, and only grows within a data set; it re-measures when `ItemsSource` or `Columns` is replaced, or the column's header or `CellTemplate` changes. Star (`*`) shares the viewport width left after the other columns. `MinWidth` (default 20) and `MaxWidth` (default infinity) clamp every mode, and `ActualWidth` reports the result.
- **Frozen columns.** Only a contiguous prefix of `Leading` columns starting at column 0 is pinned; `Leading` on a later column is ignored. `Trailing` is reserved and has no effect. Frozen columns mirror in right-to-left layouts.
- **Showing and hiding columns.** Set a column's `Visibility` to `Collapsed` to hide it and keep its state. To reorder columns, move them within `Columns`. A column belongs to one table; adding a column that another `TableView` owns is ignored.
- **Empty state.** `EmptyTemplate` is shown when there are no rows to display: `ItemsSource` is null or empty, or a filter removed every row. When it is null (the default), nothing is shown.
- **Appearance.** `HeadersVisibility` (`Column` or `None`) shows or hides the header band. `GridLinesVisibility` (default `All`) controls body lines only; header separators are always drawn. `Density` sets the minimum row height and cell padding: `Compact` (30, `8,2`), `Standard` (default; 40, `8,4`) or `Comfortable` (48, `8,8`).

## Sort and select rows

Sorting and selection are on by default. This example opens a metric column largest-first, sorts it when the page loads, and shows the selected order in a details pane.

```xaml
<tabular:TableView x:Name="OrdersTable"
                   ItemsSource="{x:Bind Orders}"
                   Loaded="OnTableLoaded"
                   Sorting="OnSorting"
                   Sorted="OnSorted"
                   SelectionChanged="OnSelectionChanged">
    <tabular:TableViewTextColumn Header="Customer" Binding="{Binding Customer}" Width="*" />
    <!-- First click sorts largest-first; the third click returns to source order. -->
    <tabular:TableViewTextColumn x:Name="TotalColumn" Header="Total" Binding="{Binding Total}"
                                 SortCycle="DescendingAscendingNone" />
    <tabular:TableViewTextColumn Header="Notes" Binding="{Binding Notes}" CanSort="False" />
</tabular:TableView>
```

```csharp
private void OnTableLoaded(object sender, RoutedEventArgs e)
{
    // Returns false if the sort could not be applied.
    OrdersTable.SortByColumn(TotalColumn, SortDirection.Descending);

    if (Orders.Count > 0)
    {
        OrdersTable.Select(0);
    }
}

private void OnSorting(TableView sender, TableViewSortingEventArgs args)
{
    // args.Column is null when ClearSort is clearing every column.
    // Set args.Cancel = true to keep the current order and sort the data yourself.
}

private void OnSorted(TableView sender, TableViewSortedEventArgs args)
{
    StatusText.Text = args.Column is null
        ? "Unsorted"
        : $"Sorted by {args.Column.Header} ({args.Direction})";
}

private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
{
    DetailsPane.Content = sender.SelectedItem as Order;   // null when nothing is selected
}
```

**Sorting**

- Sorting is single-column: sorting one column clears any other. The active direction is reported by the read-only `TableViewColumn.SortDirection`.
- A header click (or Enter/Space on a focused header) acts like `ToggleSortDirection`: it steps through the column's `SortCycle`. The `*None` cycles add a third, unsorted step.
- The sort key is the column's `CustomSortComparer` if set, otherwise `SortMemberPath`. A text column with no `SortMemberPath` uses its `Binding` path. A template column with neither is not sortable. Several columns can share a `SortMemberPath`; each sorts independently.
- `CanUserSortColumns = false` (on the table) turns off the header UI only; the sort methods still work. `CanSort = false` (on a column) blocks both, and setting it on the sorted column clears that sort.
- `SortByColumn` and `ToggleSortDirection` return `false` and change nothing when the column cannot sort, does not belong to this table, the source belongs to another table, the call would change nothing, `Sorting` was canceled, an open edit would not close, or the source contains null items or the same object twice. `SortByColumn(column, SortDirection.None)` clears a sorted column even when its `CanSort` is `false`. `ClearSort` returns `true` only if something was cleared.
- `Sorting` is raised before the change. Setting `Cancel` leaves the sort state, sort glyph and `Sorted` untouched, so the app owns the ordering (it can call `TableViewSource.Sort` with a property path to light the glyph). `Sorted` is raised after the change; a null `Column` means the sort was cleared.

**Selection**

- `SelectionMode` defaults to `Single`. Set it to `None` for a display-only table; doing so clears the selection.
- `SelectedItem` and `SelectedIndex` are read-only dependency properties: bind *from* them, and change selection with `Select`, `Deselect` and `DeselectAll`, as with `ItemsView`.
- `Select(index)` does nothing when the index is past the end, is a group header, there is no source, or `SelectionMode` is `None`. `Select` with a negative index clears the selection. `Deselect(index)` clears only if `index` is the selected index.
- Selection follows the item, not the index. Inserting rows above the selected row shifts `SelectedIndex` without raising `SelectionChanged`. Removing the selected item, or collapsing the group or parent row that contains it, clears the selection. When `ItemsSource` is replaced, the selected object stays selected if the new source contains it; otherwise the selection clears.
- `SelectedIndex` is an index into the displayed rows. Under grouping, group headers occupy indexes but cannot be selected, so it is not an index into `ItemsSource`.
- `SelectionChanged` uses the platform `SelectionChangedEventArgs`; replacing a selection raises one event with both the removed and the added item.
- Clicking another row while a cell edit is open moves the selection immediately, even if the edit stays open because its commit is vetoed.
- Gestures: pointer release selects (right-click never does); keyboard navigation selects the focused row; Ctrl+navigation moves focus without selecting; Space selects; Ctrl+click and Ctrl+Space toggle. See [Accessibility and keyboard](#accessibility-and-keyboard).

## Filter, group, and show hierarchy

Filtering, grouping, hierarchy and data-side sorting are declared on a `TableViewSource` that wraps your collection. Each verb returns the same source, so calls chain.

```csharp
// Active orders, grouped by region (regions A-Z), rows newest-first within each region.
var source = TableViewSource.From(Orders)
    .Filter(item => ((Order)item).IsActive)
    .Sort("Region", SortDirection.Ascending)          // before GroupBy: orders the groups
    .GroupBy(item => ((Order)item).Region is { Length: > 0 } region ? region : "(none)")
    .Sort("OrderDate", SortDirection.Descending);     // after GroupBy: orders rows in each group

source.IsLiveShaping = true;   // re-filter, regroup and re-sort when an Order raises PropertyChanged
OrdersTable.ItemsSource = source;
OrdersTable.CollapseAllGroups();

// Later: show everything again.
source.ClearGroupBy().ClearFilter().ClearSort();
```

```xaml
<tabular:TableView.GroupHeaderTemplate>
    <DataTemplate>
        <!-- The DataContext is a TableViewGroupInfo that is updated in place, so use {Binding}. -->
        <TextBlock>
            <Run Text="{Binding KeyText}" FontWeight="SemiBold" />
            <Run Text="{Binding ItemCountText}" />
        </TextBlock>
    </DataTemplate>
</tabular:TableView.GroupHeaderTemplate>
```

For a parent/child tree, give each item a key and a parent key:

```csharp
// Contoso org chart: Id is the key, ManagerId the parent key (null for the CEO).
OrgTable.ItemsSource = TableViewSource.From(Employees).ParentBy(
    item => ((Employee)item).Id,
    item => ((Employee)item).ManagerId);

OrgTable.ExpandAllRows();   // rows start collapsed
```

**Source rules**

- `From` accepts the same collections as `ItemsSourceView`: `IVector<Object>` (including `IObservableVector<Object>`), `IBindableVector`, `IIterable<Object>` or `IBindableIterable`. The projection is ready when `From` returns.
- The filter always applies first. After that, declaration order matters: a `Sort` declared before `GroupBy` orders the groups, and one declared after it orders rows within each group. A later `Filter` or `GroupBy` replaces the earlier one; remove them with `ClearFilter`, `ClearGroupBy` and `ClearSort` rather than passing a pass-everything delegate.
- `Sort` adds sort keys in order: the first is primary, and later keys break ties. Sorting an existing key again re-sorts it in place, and `SortDirection.None` removes it. Prefer the property-path overload; the key-selector overload is for computed keys and is identified by delegate instance.
- `TableViewSource` throws `E_INVALIDARG` for invalid input (null items or selectors, unsupported collection, empty sort path). `TableView`'s own methods instead return `false` or do nothing.
- A `TableViewSource` can be the `ItemsSource` of only one `TableView` at a time. A second table renders empty, without an exception, until the first one releases it.
- Sorting from the control and from `TableViewSource.Sort` replace each other: the last one applied wins. A path sort lights the matching column's glyph; a key-selector sort clears every glyph.
- Rows are identified by object identity, so selection and focus survive a reshape. The source and its change notifications must stay on the UI thread.
- `IsLiveShaping` (default `false`) reshapes when an item's sort, group, filter or parent value changes. Reshapes are batched to one per UI-thread turn, re-read every item, and wait while a cell is being edited.

**Grouping**

- Grouping is single-level. Expand and collapse groups by clicking the header band, with Left/Right, through UI Automation, or all at once with `ExpandAllGroups` and `CollapseAllGroups` (no-ops when the source is not grouped). There is no per-group expand API.
- Expansion is remembered per group identity, so it survives re-sorting, re-filtering and regrouping. A group whose last row is filtered out disappears.
- Grouping fails fast: building the projection throws `E_INVALIDARG` rather than showing an ungrouped table. Keys have a built-in identity for String, integer, floating-point, Boolean, Char, Guid, DateTime, TimeSpan and enum values; any other key type needs the `GroupBy(keySelector, groupIdentitySelector)` overload, whose selector receives the key and returns its identity string.
- Without an identity selector, a null or empty-string key, or two different keys with the same identity, throws, so coalesce blank values to a label such as `"(none)"`. With an identity selector, any key (including null or empty) is accepted as long as the selector returns a non-empty string; a selector that throws or returns an empty string fails.
- `GroupHeaderTemplate` fills only the content area of the header band; the chevron and band stay control-owned.

**Hierarchy**

- `ParentBy` has no depth limit. A null or empty parent key makes a root; an unknown parent key makes an orphan root. A null or duplicate key, a self-parent or a cycle throws `E_INVALIDARG` and keeps the previous projection.
- Value-typed keys compare by value and type (an `Int32` never matches an `Int64`). Other keys, including app-defined enums and structs, compare by reference.
- Sorting applies within each set of siblings, a filter keeps matching rows plus their ancestors (shown expanded), and `GroupBy` groups the roots.
- `ExpandAllRows` and `CollapseAllRows` set every node's state and do nothing for a flat source; they never affect group headers. Users expand individual rows with the chevron, Left/Right or UI Automation; there is no per-row expand API.
- `TableViewRow.Level` (1 for roots), `IsExpandable` and `IsExpanded` are read-only and drive the row's hierarchy visual states.
- `ParentBy` and `GroupBy` are code-only; they cannot be declared in XAML.

## Edit cells

Editing is off by default. Set `IsReadOnly="False"` on the table, and opt individual columns out with their own `IsReadOnly`.

```xaml
<tabular:TableView x:Name="OrdersTable"
                   ItemsSource="{x:Bind Orders}"
                   IsReadOnly="False"
                   BeginningEdit="OnBeginningEdit"
                   CellEditEnding="OnCellEditEnding">

    <!-- Built-in TextBox editor. -->
    <tabular:TableViewTextColumn Header="Customer" Binding="{Binding Customer}" Width="*" />

    <tabular:TableViewTextColumn Header="Order" Binding="{Binding OrderNumber}" IsReadOnly="True" />

    <!-- Custom editor: classic {Binding}, TwoWay, UpdateSourceTrigger=Explicit. -->
    <tabular:TableViewTemplateColumn Header="Status" SortMemberPath="Status">
        <tabular:TableViewTemplateColumn.CellTemplate>
            <DataTemplate>
                <TextBlock Text="{Binding Status}" VerticalAlignment="Center" />
            </DataTemplate>
        </tabular:TableViewTemplateColumn.CellTemplate>
        <tabular:TableViewTemplateColumn.CellEditingTemplate>
            <DataTemplate>
                <ComboBox AutomationProperties.Name="Status"
                          SelectedItem="{Binding Status, Mode=TwoWay, UpdateSourceTrigger=Explicit}">
                    <x:String>Ready</x:String>
                    <x:String>In review</x:String>
                    <x:String>Blocked</x:String>
                </ComboBox>
            </DataTemplate>
        </tabular:TableViewTemplateColumn.CellEditingTemplate>
    </tabular:TableViewTemplateColumn>
</tabular:TableView>
```

```csharp
private void OnBeginningEdit(TableView sender, TableViewBeginningEditEventArgs args)
{
    // Shipped orders are locked. Decide before returning: there is no deferral.
    if (args.Item is Order { IsShipped: true })
    {
        args.Cancel = true;
    }
}

private void OnCellEditEnding(TableView sender, TableViewCellEditEndingEventArgs args)
{
    if (args.EditAction == TableViewEditAction.Commit && !CanSaveNow)
    {
        args.Cancel = true;   // keeps the editor open
    }
}

// Close an open edit from code (for example, from a Save button).
bool closed = OrdersTable.CommitEdit();   // false if no edit is open, or it was vetoed or invalid
```

| Gesture | Result |
|---|---|
| Double-click or double-tap a cell | Starts editing that cell |
| F2 | Starts editing the current cell |
| Enter | Commits and closes the editor |
| Esc | Cancels and closes the editor |
| Moving focus out of the cell | Commits |

- A cell is editable when the table and the column are both not read-only and the column has an editor: `TableViewTextColumn` provides a `TextBox`, and any column can supply `CellEditingTemplate`. A template column without one cannot be edited.
- Editing starts only from user input; there is no `BeginEdit` method. `IsEditing` reports whether a cell editor is open.
- The editor writes to the item only on commit, so Esc leaves the item unchanged. The built-in editor uses a TwoWay, `UpdateSourceTrigger=Explicit` copy of the column's `Binding`. A `CellEditingTemplate` must use classic `{Binding}` (not `{x:Bind}`) with `Mode=TwoWay, UpdateSourceTrigger=Explicit` to get the same behavior.
- Validation uses `INotifyDataErrorInfo`. A value that fails validation blocks a commit and keeps the editor open; Esc or `CancelEdit` still closes it.
- `BeginningEdit` and `CellEditEnding` are synchronous; `Cancel` is read when the handler returns. `CellEditEnding` is raised for every close, with `EditAction` reporting `Commit` or `Cancel`. `CommitEdit` returns `false`, leaving the edit open, when `CellEditEnding` vetoes it or validation fails. `CancelEdit` returns `false`, leaving the edit open, only when `CellEditEnding` vetoes it. Both return `false` when no edit is open.
- If the edited item is removed, filtered out, or `ItemsSource` is replaced, the edit closes with `EditAction = Cancel` and nothing is written. If the item is only moved or hidden (sorted, regrouped, or under a collapsed group or parent), a valid pending value is committed.
- Editing is cell-scoped: each commit writes to the item immediately. There is no row transaction.

## Customize headers, menus, and tooltips

```xaml
<tabular:TableView ItemsSource="{x:Bind Orders}"
                   AlternatingRowBackground="{ThemeResource SubtleFillColorSecondaryBrush}">
    <tabular:TableView.ColumnHeaderStyle>
        <Style TargetType="tabular:TableViewColumnHeader">
            <Setter Property="FontWeight" Value="Normal" />
        </Style>
    </tabular:TableView.ColumnHeaderStyle>

    <tabular:TableViewTextColumn Header="Customer" Binding="{Binding Customer}" Width="*">
        <tabular:TableViewTextColumn.CellContextFlyout>
            <MenuFlyout>
                <!-- Items get the row's item as their DataContext. -->
                <MenuFlyoutItem Text="Copy order number" Click="OnCopyOrderNumber" />
            </MenuFlyout>
        </tabular:TableViewTextColumn.CellContextFlyout>
    </tabular:TableViewTextColumn>

    <tabular:TableViewTextColumn Header="Notes" Binding="{Binding Notes}" Width="200"
                                 CellToolTipBinding="{Binding Notes}"
                                 HeaderToolTip="Notes captured when the order was placed"
                                 CanResize="False" />
</tabular:TableView>
```

- **Header content.** `Header` is the header content. `HeaderTemplateSelector`, when set, takes precedence over `HeaderTemplate`.
- **Header styles.** `TableView.ColumnHeaderStyle` applies to every header. A column's `HeaderStyle` replaces it for that column (the two are not merged). Either style is layered over the default header style, so it doesn't need `BasedOn`. Its `TargetType` must be `TableViewColumnHeader` or one of its base types; a style with any other or no `TargetType` is ignored.
- **Tooltips.** `CellToolTipBinding` is evaluated against each row's item; `HeaderToolTip` is set directly. In both, the value is the tooltip *content* (null or empty means none); the control owns the `ToolTip`, so a `ToolTip` value is rejected. String tooltips are also exposed to UI Automation; non-string content is visual-only, so use text when it matters.
- **Context menus.** The first available flyout opens; the target receives focus and selection does not change. No menu opens while a cell is being edited, or on a collapsed column.

| Target | Precedence (first wins) |
|---|---|
| Cell | A `ContextFlyout` set inside the cell content → `TableViewColumn.CellContextFlyout` → the row's `ContextFlyout` → `TableView.ContextFlyout` |
| Column header | A `ContextFlyout` from the header style or `HeaderTemplate` → `TableViewColumn.HeaderContextFlyout` → `TableView.ContextFlyout` |
| Group header | The group header style's `ContextFlyout` → `TableView.ContextFlyout` |

- **Resizing.** A column can be resized when `TableView.CanUserResizeColumns` and its `CanResize` are both `true` (the defaults): drag the header gripper, or press Alt+Left/Right on a focused header (add Shift for a larger step). Esc during a drag restores the column's original `Width`. Widths stay within `MinWidth`/`MaxWidth`; a Star column with no later Star column to trade width with cannot be resized, and a Pixel or Auto column with no column to give width to can only shrink.
- **Theming.** `RowBackground` and `AlternatingRowBackground` are null by default (no banding). Colors come from `TabularSurface*` theme brushes, which apps can override; for example, set `TabularSurfaceHeaderSeparatorBrush` to `Transparent` to hide the header separators.

# API Pages

_Spec note: all types are in `Microsoft.UI.Xaml.Controls.Tabular` and are preview (experimental) API._

## TableView class

Represents a control that displays data in rows and customizable columns.

```xaml
<tabular:TableView ItemsSource="{x:Bind Orders}">
    <tabular:TableViewTextColumn Header="Customer" Binding="{Binding Customer}" />
</tabular:TableView>
```

### Template parts

A missing required part disables the related feature.

| Part | Type | Description |
|---|---|---|
| `PART_HeaderRow` | `FrameworkElement` | Header band; collapsed when headers are hidden. |
| `PART_HeaderHost` | `Panel` (`TableViewCellsPanel`) | Hosts one header per column. |
| `PART_RowsRepeater` | `ItemsRepeater` | Hosts the rows. Don't template-bind its `ItemsSource`. |
| `PART_EmptyStatePresenter` | `ContentControl` or `ContentPresenter` | Optional. Hosts `EmptyTemplate`. |

The header and body scrollers are the nearest `ScrollViewer` ancestors of `PART_HeaderHost` and `PART_RowsRepeater` inside the template; their names aren't looked up. Without a body scroller, Star widths, frozen columns and header scroll sync don't work.

### Other TableView members

| Member | Description |
|---|---|
| `ItemsSource` | The rows. Any collection, or a `TableViewSource` to filter, group or show a hierarchy. |
| `Columns` | The columns (content property). Starts empty. |
| `HeadersVisibility` | Default `Column`. `None` hides the header band. |
| `GridLinesVisibility` | Default `All`. Body grid lines only. |
| `Density` | Default `Standard`. Row height and cell padding preset. |
| `RowBackground` | Default null. Background brush for rows. |
| `AlternatingRowBackground` | Default null. Background brush for alternate rows (banding). |
| `EmptyTemplate` | Default null. Shown when there are no rows to display. |
| `GroupHeaderTemplate` | Content of each group header; its `DataContext` is a `TableViewGroupInfo`. |
| `ColumnHeaderStyle` | `Style` for every `TableViewColumnHeader`. |
| `CanUserResizeColumns` | Default `true`. Turns column resize on or off for the whole table. |
| `CanUserSortColumns` | Default `true`. Turns sort-by-header-click on or off; the sort methods work either way. |
| `SortByColumn` | Sorts by one column in the given direction, or clears it with `None`. Returns `false` when nothing changed. See [Sort and select rows](#sort-and-select-rows). |
| `ToggleSortDirection` | Moves a column to the next direction in its `SortCycle`. Returns `false` when nothing changed. |
| `ClearSort` | Clears the sort. Returns `true` if a sort was cleared. |
| `Sorting` | Raised before a sort change. Cancelable. |
| `Sorted` | Raised after a sort change. |
| `SelectionMode` | Default `Single`. |
| `SelectedItem` | Read-only. The selected item, or null. |
| `SelectedIndex` | Read-only. The selected item's index in the displayed rows, or -1. |
| `Select` | Selects the row at an index. |
| `Deselect` | Clears the selection if the index is selected. |
| `IsSelected` | Whether the row at an index is selected. |
| `DeselectAll` | Clears the selection. |
| `SelectionChanged` | Raised after the selection changes. |
| `IsReadOnly` | Default `true`. Set to `false` to allow editing. |
| `IsEditing` | Read-only (not a dependency property). `true` while a cell editor is open, including during `BeginningEdit`. |
| `CommitEdit` | Commits and closes the open cell edit. Returns `false` if no edit is open or it stays open. |
| `CancelEdit` | Cancels and closes the open cell edit. Returns `false` if no edit is open or it stays open. |
| `BeginningEdit` | Raised before a cell edit opens. Synchronous, cancelable. |
| `CellEditEnding` | Raised before a cell edit closes. Synchronous, cancelable. |
| `ExpandAllGroups` | Expands every group. No-op when not grouped. |
| `CollapseAllGroups` | Collapses every group. No-op when not grouped. |
| `ExpandAllRows` | Expands every hierarchy row. No-op without `ParentBy`. |
| `CollapseAllRows` | Collapses every hierarchy row. No-op without `ParentBy`. |

## TableViewColumn class

The base class for `TableView` columns. Derive from it, or use `TableViewTextColumn` or `TableViewTemplateColumn`.

### TableViewColumn.GenerateElementCore method

Override in a derived column to create a cell's display element. The returned element must bind to its inherited `DataContext` (the row item), and must not set its own `DataContext` or keep a reference to `dataItem`: the same element can later display a different item.

_Spec note: the editing hooks for derived columns (`GenerateEditingElement`, `PrepareCellForEdit`, `CommitCellEdit`, `CancelCellEdit`) are internal in this release; `CellEditingTemplate` is the supported way to customize an editor._

### Other TableViewColumn members

| Member | Description |
|---|---|
| `Header` | Header content (content property). |
| `HeaderTemplate` | Template for the header content. |
| `HeaderTemplateSelector` | Template selector for the header content; takes precedence over `HeaderTemplate`. |
| `HeaderStyle` | Style for this column's header; replaces `TableView.ColumnHeaderStyle`. |
| `HeaderToolTip` | Header tooltip content. |
| `Width` | Default 120 pixels. Pixel, `Auto` or Star. See [Display data in a table](#display-data-in-a-table). |
| `MinWidth` | Default 20. |
| `MaxWidth` | Default infinity. |
| `ActualWidth` | Read-only. The resolved width in pixels. |
| `CanResize` | Default `true`. |
| `FrozenEdge` | Default `None`. |
| `Visibility` | Default `Visible`. |
| `CanSort` | Default `true`. |
| `SortCycle` | Default `AscendingDescending`. The directions repeated header clicks step through. |
| `SortMemberPath` | Default empty. Property path of the sort key. |
| `CustomSortComparer` | Default null. Custom ordering; takes precedence over `SortMemberPath`. Not a dependency property. |
| `SortDirection` | Read-only. The column's current sort direction, set by the table. |
| `GetSortMemberPathCore` | Overridable. Returns the effective sort path; `TableViewTextColumn` falls back to its `Binding` path. |
| `GenerateElement` | Creates a cell's display element by calling `GenerateElementCore`. |
| `IsReadOnly` | Default `false`. A read-only column can still be the current cell. |
| `CellEditingTemplate` | Editor for the column's cells. |
| `CellToolTipBinding` | A `Binding` evaluated against each row item to produce the cell tooltip. Not a dependency property. |
| `CellContextFlyout` | Context menu for the column's cells. |
| `HeaderContextFlyout` | Context menu for the column header. |

## TableViewTextColumn class

A column that displays text from a binding, and edits it with a `TextBox`.

### Other TableViewTextColumn members

| Member | Description |
|---|---|
| `Binding` | The display binding (a CLR property, not a dependency property). The editor uses a TwoWay, `UpdateSourceTrigger=Explicit` copy of it, and it supplies the default sort path. Setting `CellEditingTemplate` replaces the built-in editor. |

## TableViewTemplateColumn class

A column whose cells are created from a `DataTemplate`. Set `SortMemberPath` or `CustomSortComparer` to make it sortable, and `CellEditingTemplate` to make it editable.

### Other TableViewTemplateColumn members

| Member | Description |
|---|---|
| `CellTemplate` | Display template. A null template gives an empty cell. |

## TableViewSource class

Wraps a collection and declares how a `TableView` filters, sorts, groups or nests its rows. Sealed. See [Filter, group, and show hierarchy](#filter-group-and-show-hierarchy). The delegates are `TableViewPredicate` (returns whether an item is shown), `TableViewKeySelector` (returns an item's key) and `TableViewIdentitySelector` (returns a group key's identity string).

_Spec note: `TableViewIdentitySelector` is declared with a parameter named `item`, but it receives the group key returned by the `keySelector`, not a source item._

### Other TableViewSource members

| Member | Description |
|---|---|
| `From` | Static. Wraps a collection. |
| `Filter` | Shows only items for which the `TableViewPredicate` returns `true`. |
| `ClearFilter` | Removes the filter. |
| `Sort(String, SortDirection)` | Adds or updates a sort key by property path. ABI name `SortByPath`. |
| `Sort(TableViewKeySelector, SortDirection)` | Adds or updates a sort key computed by a delegate. |
| `ClearSort` | Removes every sort key. |
| `GroupBy(TableViewKeySelector)` | Groups by a key that has a built-in identity. |
| `GroupBy(TableViewKeySelector, TableViewIdentitySelector)` | Groups with an app-supplied identity string. ABI name `GroupByWithIdentity`. |
| `ClearGroupBy` | Removes grouping. |
| `ParentBy` | Shows a parent/child tree from a key and a parent key. |
| `ClearParentBy` | Shows the items flat again and resets expansion. |
| `IsLiveShaping` | Default `false`. Reshapes when an item raises `PropertyChanged`. |

## ITableViewSortComparer interface

Orders two row items for a column's `CustomSortComparer`. `Compare` returns a negative number, zero or a positive number. It's an interface rather than a delegate so a comparer can be declared as a XAML resource. `Compare` must be a pure function of its arguments; calling into or modifying the `TableView` or its source from `Compare` is not supported.

```csharp
public sealed partial class OrderNumberComparer : ITableViewSortComparer
{
    // "SO-9" sorts before "SO-10".
    public int Compare(object left, object right) =>
        int.Parse(((Order)left).OrderNumber[3..]).CompareTo(int.Parse(((Order)right).OrderNumber[3..]));
}
```

## TableViewColumnHeader class

The header for one column. `TableView` creates headers; there is no public constructor. Restyle them with `TableView.ColumnHeaderStyle` or `TableViewColumn.HeaderStyle`.

### Template parts and visual states

| Part (all optional) | Type | Description |
|---|---|---|
| `PART_ContentPresenter` | `ContentPresenter` | Header content. |
| `PART_GridLine` | `Border` | Vertical separator. |
| `PART_SortIndicatorHost` | `Border` | Hosts the sort glyph. |
| `PART_ResizeGripperHost` | `Border` | Hosts the resize gripper. Without it, the column can't be resized. |

Visual states: `CommonStates` (`Normal`, `PointerOver`, `Pressed`, `Disabled`); `HighContrastStates` (`HighContrastNormal`, `HighContrastPointerOver`, `HighContrastPressed`); `SortStates` (`Unsorted`, `SortAscending`, `SortDescending`).

### Other TableViewColumnHeader members

| Member | Description |
|---|---|
| `Column` | The `TableViewColumn` this header represents. |

## TableViewRow class

The container for one data row. `TableView` creates rows and can reuse a row for a different item.

### Template parts and visual states

| Part | Type | Description |
|---|---|---|
| `PART_RootBorder` | `Border` | Required. Row background, driven by `CommonStates`. |
| `PART_GridLineBorder` | `Border` | Required. Grid line chrome. |
| `PART_CellsHost` | `Panel` (`TableViewCellsPanel`) | Required. One cell per column. |
| `PART_CellForegroundPresenter` | `ContentPresenter` | Required. Hosts `PART_CellsHost`; carries the state-driven `Foreground`. |
| `PART_SelectionIndicator` | `Shape` | Selection strip; `Selected*` states animate its `Opacity`. |
| `PART_RowExpanderGutter` | `FrameworkElement` | Optional. Hierarchy indent and chevron toggle target. |
| `PART_RowExpanderIcon` | `FontIcon` | Optional. Hierarchy chevron. |

Visual states: `CommonStates` (`Normal`, `PointerOver`, `Pressed`, `Disabled`, `Selected`, `SelectedPointerOver`, `SelectedPressed`, `SelectedDisabled`); `RowHierarchyStates` (`RowHierarchical`, `RowNotHierarchical`); `RowExpandabilityStates` (`RowExpandable`, `RowNotExpandable`); `RowExpansionStates` (`RowExpanded`, `RowCollapsed`). The hierarchy groups are entered on every row, including in flat tables.

### Other TableViewRow members

| Member | Description |
|---|---|
| `IsSelected` | Read-only. Whether this is the selected row. |
| `Level` | Read-only. Depth in a hierarchy, 1 for roots; 0 when the source has no hierarchy. |
| `IsExpandable` | Read-only. Whether the row has children. |
| `IsExpanded` | Read-only. Whether the row's children are shown. |

## TableViewGroupHeader class

The band shown above each group. Its `Content` and `DataContext` are the group's `TableViewGroupInfo`, and `TableView.GroupHeaderTemplate` sets its `ContentTemplate`. The whole band toggles the group.

### Template parts and visual states

| Part | Type | Description |
|---|---|---|
| `PART_Root` | `Border` | The band. |
| `PART_ContentPresenter` | `ContentPresenter` | The group header content. |
| `PART_ExpanderGutter` | `Border` | Keeps content aligned with the rows below. |
| `PART_ExpanderIcon` | `FontIcon` | The chevron. |
| `PART_GridLineBorder` | `Border` | Optional. Grid line chrome. |

Visual states: `CommonStates` (`Normal`, `PointerOver`, `Pressed`, `Disabled`); `ExpansionStates` (`Expanded`, `Collapsed`); `ExpandabilityStates` (`Expandable`, `NotExpandable`). The default states target `PART_Root`, `PART_ContentPresenter` and `PART_ExpanderIcon`.

### Other TableViewGroupHeader members

| Member | Description |
|---|---|
| `IsExpanded` | Set by `TableView`; treat as read-only. Setting it does not expand or collapse the group. |
| `IsExpandable` | Set by `TableView`; treat as read-only. When `false`, the header can't be toggled. |
| `ToggleRequested` | Raised when the band is activated. |

## TableViewGroupInfo class

Describes one group for a `GroupHeaderTemplate`. It's updated in place and raises `PropertyChanged`, so bind to it with `{Binding}`.

### Other TableViewGroupInfo members

| Member | Description |
|---|---|
| `Key` | The group key. |
| `KeyText` | The key as culture-formatted text. |
| `ItemCount` | The number of rows in the group. |
| `ItemCountText` | `ItemCount` as formatted text. |
| `Level` | Always 0 in this release. |
| `IsExpandable` | Whether the group can expand. |
| `IsExpanded` | Whether the group is expanded. |

## TableViewCellsPanel class

The panel that lays cells out under their columns. Use it as `PART_HeaderHost` and `PART_CellsHost` when you re-template `TableView` or `TableViewRow`. `TableView` fills its `Children`; don't add children yourself. Another `Panel` is accepted, but then `Auto` widths and column alignment don't work.

```xaml
<tabular:TableViewCellsPanel x:Name="PART_CellsHost" HorizontalAlignment="Left" />
```

## TabularControlsResources class

The resource dictionary that contains the default styles and theme resources for the `Tabular` controls. Merge it into your application resources (see [Display data in a table](#display-data-in-a-table)).

## Event args

`SelectionChanged` uses the platform `Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs`.

### TableViewSortingEventArgs class

Provides data for `TableView.Sorting`: `Column` (null when every column is being cleared), `Direction` (about to be applied) and `Cancel`.

### TableViewSortedEventArgs class

Provides data for `TableView.Sorted`: `Column` (null when the sort was cleared) and `Direction` (now applied).

### TableViewBeginningEditEventArgs class

Provides data for `TableView.BeginningEdit`: the `Item` and `Column` of the cell, and `Cancel`.

### TableViewCellEditEndingEventArgs class

Provides data for `TableView.CellEditEnding`: the `Item` and `Column` of the cell, the `EditAction` (`Commit` or `Cancel`), and `Cancel`.

### TableViewGroupHeaderToggleRequestedEventArgs class

Provides data for `TableViewGroupHeader.ToggleRequested`: `GroupKey`, the key of the activated group.

## Enums

### SortDirection enum

The sort direction of a column or sort key: `None`, `Ascending`, `Descending`.

### TableViewSortCycle enum

The directions a column steps through on repeated header clicks: `AscendingDescending`, `AscendingDescendingNone`, `DescendingAscending`, `DescendingAscendingNone`. The first direction is used for an unsorted column; the `*None` values add an unsorted step.

### TableViewSelectionMode enum

Whether rows can be selected: `None` or `Single`.

### TableViewEditAction enum

How a cell edit closes: `Commit` or `Cancel`.

### TableViewFrozenEdge enum

Which edge a column is pinned to: `None`, `Leading`, or `Trailing` (reserved; no effect).

### TableViewHeadersVisibility enum

Which headers are shown: `None` or `Column`. A flags enum with the same values as WPF `DataGridHeadersVisibility`.

### TableViewGridLinesVisibility enum

Which body grid lines are drawn: `All`, `Horizontal`, `None` or `Vertical`, with the same names and values as WPF `DataGridGridLinesVisibility`.

### TableViewDensity enum

The row height and padding preset: `Compact`, `Standard` or `Comfortable`.

## Automation peers

### TableViewAutomationPeer class

Exposes a `TableView` to UI Automation with the Grid, Table, Selection and ItemContainer patterns. Selection is reported only while `SelectionMode` isn't `None`; `CanSelectMultiple` is `false`.

### TableViewRowAutomationPeer class

Exposes a `TableViewRow` with the SelectionItem pattern, plus ExpandCollapse for rows of a hierarchy.

### TableViewCellAutomationPeer class

Exposes a cell with the GridItem and TableItem patterns, plus the Value pattern for editable cells of a `TableViewTextColumn` with no `CellEditingTemplate`. The constructor throws `E_INVALIDARG` for null arguments, a negative `columnIndex`, or a column outside the row's table. `SetValue` on a read-only cell fails.

### TableViewColumnHeaderAutomationPeer class

Exposes a column header with the Invoke pattern, which sorts the column. At run time it also exposes the Transform pattern to resize the column (`Resize` takes physical pixels; it can't move or rotate), but `ITransformProvider` isn't declared in the IDL. The constructor throws `E_INVALIDARG` unless `column` belongs to `owner`.

### TableViewGroupHeaderAutomationPeer class

Exposes a `TableViewGroupHeader` with the ExpandCollapse and GridItem patterns. A group that can't expand reports `LeafNode`.

# API Details

_Spec note: `[MUX_PREVIEW]` expands to `contract(Microsoft.UI.Xaml.XamlContract, 12), feature(Feature_Experimental)`._

```c#
namespace Microsoft.UI.Xaml.Controls.Tabular
{
    [MUX_PREVIEW, webhosthidden] enum SortDirection { None = 0, Ascending = 1, Descending = 2 };
    [MUX_PREVIEW, webhosthidden] enum TableViewFrozenEdge { None = 0, Leading = 1, Trailing = 2 };
    [MUX_PREVIEW, webhosthidden] enum TableViewSelectionMode { None = 0, Single = 1 };
    [MUX_PREVIEW, webhosthidden, flags] enum TableViewHeadersVisibility { None = 0, Column = 1 };
    [MUX_PREVIEW, webhosthidden] enum TableViewGridLinesVisibility { All = 0, Horizontal = 1, None = 2, Vertical = 3 };
    [MUX_PREVIEW, webhosthidden] enum TableViewDensity { Compact = 0, Standard = 1, Comfortable = 2 };
    [MUX_PREVIEW, webhosthidden] enum TableViewEditAction { Commit = 0, Cancel = 1 };

    [MUX_PREVIEW, webhosthidden]
    enum TableViewSortCycle
    {
        AscendingDescending = 0,
        AscendingDescendingNone = 1,
        DescendingAscending = 2,
        DescendingAscendingNone = 3,
    };

    [MUX_PREVIEW, webhosthidden]
    interface ITableViewSortComparer
    {
        Int32 Compare(Object left, Object right);
    };

    [MUX_PREVIEW, webhosthidden] delegate Boolean TableViewPredicate(Object item);
    [MUX_PREVIEW, webhosthidden] delegate Object TableViewKeySelector(Object item);
    [MUX_PREVIEW, webhosthidden] delegate String TableViewIdentitySelector(Object item);

    [MUX_PREVIEW, webhosthidden]
    runtimeclass TableViewSource
    {
        static TableViewSource From(Object items);
        TableViewSource Filter(TableViewPredicate predicate);
        [default_overload] [method_name("GroupBy")]
        TableViewSource GroupBy(TableViewKeySelector keySelector);
        [method_name("GroupByWithIdentity")]
        TableViewSource GroupBy(TableViewKeySelector keySelector, TableViewIdentitySelector groupIdentitySelector);
        [method_name("SortByPath")]
        TableViewSource Sort(String sortMemberPath, SortDirection direction);
        [default_overload] [method_name("Sort")]
        TableViewSource Sort(TableViewKeySelector keySelector, SortDirection direction);
        TableViewSource ClearFilter();
        TableViewSource ClearGroupBy();
        TableViewSource ClearSort();
        TableViewSource ParentBy(TableViewKeySelector keySelector, TableViewKeySelector parentKeySelector);
        TableViewSource ClearParentBy();
        Boolean IsLiveShaping { get; set; };
    };

    [MUX_PREVIEW, webhosthidden, contentproperty("Header")]
    unsealed runtimeclass TableViewColumn : Microsoft.UI.Xaml.DependencyObject
    {
        TableViewColumn();

        Object Header { get; set; };
        Microsoft.UI.Xaml.DataTemplate HeaderTemplate { get; set; };
        Microsoft.UI.Xaml.Controls.DataTemplateSelector HeaderTemplateSelector { get; set; };
        Object HeaderToolTip { get; set; };
        Microsoft.UI.Xaml.Style HeaderStyle { get; set; };
        Microsoft.UI.Xaml.GridLength Width { get; set; };
        Double MinWidth { get; set; };
        Double MaxWidth { get; set; };
        Boolean CanResize { get; set; };
        Double ActualWidth { get; };
        TableViewFrozenEdge FrozenEdge { get; set; };
        Microsoft.UI.Xaml.Visibility Visibility { get; set; };
        Boolean CanSort { get; set; };
        TableViewSortCycle SortCycle { get; set; };
        String SortMemberPath { get; set; };
        ITableViewSortComparer CustomSortComparer { get; set; };
        SortDirection SortDirection { get; };
        overridable String GetSortMemberPathCore();
        Microsoft.UI.Xaml.FrameworkElement GenerateElement(Object dataItem);
        overridable Microsoft.UI.Xaml.FrameworkElement GenerateElementCore(Object dataItem);
        Boolean IsReadOnly { get; set; };
        Microsoft.UI.Xaml.DataTemplate CellEditingTemplate { get; set; };
        Microsoft.UI.Xaml.Data.Binding CellToolTipBinding;
        Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase CellContextFlyout { get; set; };
        Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase HeaderContextFlyout { get; set; };

        static Microsoft.UI.Xaml.DependencyProperty HeaderProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderTemplateProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderTemplateSelectorProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderToolTipProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderStyleProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty WidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty MinWidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty MaxWidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CanResizeProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty ActualWidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty FrozenEdgeProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty VisibilityProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsReadOnlyProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CellEditingTemplateProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CellContextFlyoutProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderContextFlyoutProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CanSortProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SortCycleProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SortMemberPathProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SortDirectionProperty { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewTextColumn : TableViewColumn
    {
        TableViewTextColumn();

        Microsoft.UI.Xaml.Data.Binding Binding;
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewTemplateColumn : TableViewColumn
    {
        TableViewTemplateColumn();

        Microsoft.UI.Xaml.DataTemplate CellTemplate { get; set; };

        static Microsoft.UI.Xaml.DependencyProperty CellTemplateProperty { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewCellsPanel : Microsoft.UI.Xaml.Controls.Panel
    {
        TableViewCellsPanel();
    };

    [MUX_PREVIEW, webhosthidden]
    runtimeclass TableViewGroupInfo : Microsoft.UI.Xaml.Data.INotifyPropertyChanged
    {
        Object Key { get; };
        Int32 ItemCount { get; };
        Int32 Level { get; };
        Boolean IsExpandable { get; };
        Boolean IsExpanded { get; };
        String KeyText { get; };
        String ItemCountText { get; };
    };

    [MUX_PREVIEW, webhosthidden, default_interface]
    runtimeclass TableViewGroupHeaderToggleRequestedEventArgs
    {
        Object GroupKey { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewGroupHeader : Microsoft.UI.Xaml.Controls.ContentControl
    {
        TableViewGroupHeader();

        Boolean IsExpanded { get; set; };
        Boolean IsExpandable { get; set; };
        event Windows.Foundation.TypedEventHandler<TableViewGroupHeader, TableViewGroupHeaderToggleRequestedEventArgs> ToggleRequested;

        static Microsoft.UI.Xaml.DependencyProperty IsExpandedProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsExpandableProperty { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    runtimeclass TableViewColumnHeader : Microsoft.UI.Xaml.Controls.ContentControl
    {
        TableViewColumn Column { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewRow : Microsoft.UI.Xaml.Controls.Control
    {
        TableViewRow();

        Boolean IsSelected { get; };
        Int32 Level { get; };
        Boolean IsExpandable { get; };
        Boolean IsExpanded { get; };

        static Microsoft.UI.Xaml.DependencyProperty IsSelectedProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty LevelProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsExpandableProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsExpandedProperty { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    runtimeclass TableViewSortingEventArgs
    {
        TableViewColumn Column { get; };
        SortDirection Direction { get; };
        Boolean Cancel;
    };

    [MUX_PREVIEW, webhosthidden]
    runtimeclass TableViewSortedEventArgs
    {
        TableViewColumn Column { get; };
        SortDirection Direction { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    runtimeclass TableViewBeginningEditEventArgs
    {
        Object Item { get; };
        TableViewColumn Column { get; };
        Boolean Cancel;
    };

    [MUX_PREVIEW, webhosthidden]
    runtimeclass TableViewCellEditEndingEventArgs
    {
        Object Item { get; };
        TableViewColumn Column { get; };
        TableViewEditAction EditAction { get; };
        Boolean Cancel;
    };

    [MUX_PREVIEW, webhosthidden, contentproperty("Columns")]
    unsealed runtimeclass TableView : Microsoft.UI.Xaml.Controls.Control
    {
        TableView();

        Object ItemsSource { get; set; };
        Windows.Foundation.Collections.IVector<TableViewColumn> Columns { get; };
        TableViewHeadersVisibility HeadersVisibility { get; set; };
        TableViewGridLinesVisibility GridLinesVisibility { get; set; };
        TableViewDensity Density { get; set; };
        Microsoft.UI.Xaml.Media.Brush RowBackground { get; set; };
        Microsoft.UI.Xaml.Media.Brush AlternatingRowBackground { get; set; };
        Microsoft.UI.Xaml.DataTemplate EmptyTemplate { get; set; };
        Microsoft.UI.Xaml.DataTemplate GroupHeaderTemplate { get; set; };
        Microsoft.UI.Xaml.Style ColumnHeaderStyle { get; set; };
        Boolean CanUserResizeColumns { get; set; };
        // Editing
        Boolean IsReadOnly { get; set; };
        Boolean IsEditing { get; };
        Boolean CommitEdit();
        Boolean CancelEdit();
        event Windows.Foundation.TypedEventHandler<TableView, TableViewBeginningEditEventArgs> BeginningEdit;
        event Windows.Foundation.TypedEventHandler<TableView, TableViewCellEditEndingEventArgs> CellEditEnding;
        // Selection
        TableViewSelectionMode SelectionMode { get; set; };
        Object SelectedItem { get; };
        Int32 SelectedIndex { get; };
        void Select(Int32 index);
        void Deselect(Int32 index);
        Boolean IsSelected(Int32 index);
        void DeselectAll();
        event Windows.Foundation.TypedEventHandler<TableView, Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs> SelectionChanged;
        // Grouping and hierarchy
        void ExpandAllGroups();
        void CollapseAllGroups();
        void ExpandAllRows();
        void CollapseAllRows();
        // Sorting
        Boolean CanUserSortColumns { get; set; };
        Boolean SortByColumn(TableViewColumn column, SortDirection direction);
        Boolean ToggleSortDirection(TableViewColumn column);
        Boolean ClearSort();
        event Windows.Foundation.TypedEventHandler<TableView, TableViewSortingEventArgs> Sorting;
        event Windows.Foundation.TypedEventHandler<TableView, TableViewSortedEventArgs> Sorted;

        static Microsoft.UI.Xaml.DependencyProperty ItemsSourceProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty ColumnsProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeadersVisibilityProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty GridLinesVisibilityProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CanUserResizeColumnsProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty RowBackgroundProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty AlternatingRowBackgroundProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty EmptyTemplateProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty GroupHeaderTemplateProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty ColumnHeaderStyleProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty DensityProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsReadOnlyProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SelectionModeProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SelectedItemProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SelectedIndexProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CanUserSortColumnsProperty { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.ISelectionProvider,
          Microsoft.UI.Xaml.Automation.Provider.IGridProvider,
          Microsoft.UI.Xaml.Automation.Provider.ITableProvider,
          Microsoft.UI.Xaml.Automation.Provider.IItemContainerProvider
    {
        TableViewAutomationPeer(TableView owner);
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewRowAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.ISelectionItemProvider,
          Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider
    {
        TableViewRowAutomationPeer(TableViewRow owner);
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewGroupHeaderAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider,
          Microsoft.UI.Xaml.Automation.Provider.IGridItemProvider
    {
        TableViewGroupHeaderAutomationPeer(TableViewGroupHeader owner);
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewColumnHeaderAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider
    {
        TableViewColumnHeaderAutomationPeer(TableView owner, TableViewColumn column);
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewCellAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.IGridItemProvider,
          Microsoft.UI.Xaml.Automation.Provider.ITableItemProvider,
          Microsoft.UI.Xaml.Automation.Provider.IValueProvider
    {
        TableViewCellAutomationPeer(Microsoft.UI.Xaml.FrameworkElement cell, TableViewRow row, TableViewColumn column, Int32 columnIndex);
    };

    [MUX_PREVIEW, webhosthidden, default_interface]
    runtimeclass TabularControlsResources : Microsoft.UI.Xaml.ResourceDictionary
    {
        TabularControlsResources();
    };
}
```

# Appendix

## Design decisions

- **A dedicated `TableViewSelectionMode` with only `None` and `Single`.** Reusing `ItemsViewSelectionMode` would let `Multiple` and `Extended` compile and bind, then fail or act as `Single` at run time. The names and values match the first two `ItemsViewSelectionMode` members, and new members will be appended. Alternative: reuse `ItemsViewSelectionMode` and reject unsupported values with `E_INVALIDARG` (open question 2).
- **Read-only `SelectedItem` and `SelectedIndex`.** Matches `ItemsView`, and avoids the deferral and coercion that settable `Selector` properties need to survive markup ordering and two-way bindings. Alternative: settable, like `ListView`.
- **Reuse the platform `SelectionChangedEventArgs`.** Handlers can be shared with `ListView`, and the args already carry added and removed vectors for a future multiple-selection mode.
- **Shaping is declared on `TableViewSource`, not on the control.** Filter, group and hierarchy describe the data, compose in a defined order, and go beyond what WinUI's `CollectionViewSource` offers (it can't filter or sort). The control's own sort is a convenience over the same projection; the two sort paths replace each other (last one wins) rather than stacking. Alternative: shaping properties on `TableView`.
- **Grouping fails fast.** An unresolvable group identity throws instead of silently showing an ungrouped table, a bug an app could otherwise ship without noticing.
- **Synchronous, cancelable edit events without a deferral.** Keeps the edit state machine simple and re-entrancy-safe. A deferral can be added later without breaking callers.
- **Cell-scoped editing, no `BindingGroup`.** WinUI has no `BindingGroup`, so each commit writes to the item immediately and Esc reverts only the current cell. Unlike WPF's `DataGrid`, there is no row-level rollback.
- **`ITableViewSortComparer` is an interface, not a delegate,** so a comparer can be a XAML resource assigned in markup.
- **`SortDirection` is declared in `Microsoft.UI.Xaml.Controls.Tabular`.** A type declared into `Microsoft.UI.Xaml.Data`, which the Windows App SDK also owns, is invisible to C# apps that reference both WinMDs.
- **`TableViewHeadersVisibility` and `TableViewGridLinesVisibility` mirror WPF** names and values, so existing code and persisted values carry over. Row headers are out of scope, so `Row` and `All` aren't defined.

## Accessibility and keyboard

`TableView` has two tab stops, the header band and the body, which share one column position. The control itself isn't a tab stop. All arrow keys mirror in right-to-left layouts.

| Where | Keys | Result |
|---|---|---|
| Header band | Left, Right | Move between headers. |
| | Down | Move into the body. |
| | Enter, Space | Sort the focused column. |
| | Alt+Left/Right, Alt+Shift+Left/Right | Resize the focused column (small or large step). |
| Body | Up, Down, PageUp, PageDown | Move between rows; selection follows. |
| | Right, Left | Move from a row into its cells and back; move between cells. |
| | Home, End; Ctrl+Home, Ctrl+End | First or last cell in the row; first or last cell in the table. |
| | Ctrl+navigation key | Move focus without changing the selection. |
| | Space; Ctrl+Space | Select the focused row; toggle its selection. |
| | Enter on a cell with a control; Esc | Move focus into the control; return to the cell. |
| | F2, Enter, Esc | Start, commit and cancel an edit. |
| Group header or tree row | Left, Right | Collapse, expand. |

- **Cell names.** A cell is named by its value ("blank" when empty), and its column header comes from `ITableItemProvider.GetColumnHeaderItems`. When column headers are hidden, the name is "{header}, {value}". A cell that holds an interactive control is named after its column header, because the control names itself.
- **Rows** report `PositionInSet` and `SizeOfSet`; in a grouped table these are relative to the group. App-set `AutomationProperties` values win.
- **Header help text.** A string `HeaderToolTip` is reported as the header's UIA HelpText, together with the column's sort state.
- **Tooltips.** Tooltips open on pointer hover and on keyboard focus. String cell tooltips are also exposed as the cell's HelpText. Non-string tooltip content is visual-only and has no accessible equivalent.
- **High Contrast.** A template column that sets an explicit `Foreground` overrides the selected and High Contrast foreground, so leave it unset.
- Gamepad and XY focus navigation aren't supported in this release.

## Not in this release

- Multiple or extended selection, and `SelectedItems`.
- Column reorder by drag or by API (move columns within `Columns` instead).
- Trailing frozen columns (`TableViewFrozenEdge.Trailing` is reserved).
- Row editing: row commit and cancel, `RowEditEnding`, programmatic `BeginEdit`, and a public current-cell API.
- Public editing hooks for derived columns.
- `RowStyle` and `CellStyle`.
- Multi-column sort from the UI.
- Multi-level grouping, group aggregates, and a group header tooltip API.
- Per-group and per-row programmatic expand and collapse.
- Column virtualization (targets roughly 5–50 columns), row headers, clipboard, and incremental loading.
- Declaring `GroupBy` and `ParentBy` in XAML.
- Gamepad and XY focus navigation.

## Open questions

1. **Multiple and Extended selection.** Proposed: this release supports `None` and `Single` only; `Multiple` and `Extended` are appended to `TableViewSelectionMode` later, with `SelectedItems` and a separate `MultiSelectStates` visual state group. Question: must they ship in v1?
2. **Selection mode enum.** Proposed: a dedicated `TableViewSelectionMode`. Question: reuse `ItemsViewSelectionMode` instead and reject unsupported values with `E_INVALIDARG`?
3. **Pointer selection while a cell edit is open.** Proposed: clicking another row moves the selection immediately, even if the edit then stays open because its commit was vetoed. Question: block or defer the selection change instead?
4. **`Select` with a negative index.** Proposed: `Select(index < 0)` clears the selection (current behavior). Question: confirm, or make it a no-op like other out-of-range indexes?
5. **Column reorder.** Proposed: no reorder gesture or API; apps move columns within `Columns`. Question: is a drag gesture or `MoveColumn` API required for v1?
6. **Header resize automation.** Proposed: `TableViewColumnHeaderAutomationPeer` exposes the Transform pattern at run time without declaring `ITransformProvider` in the IDL. Question: declare it in the IDL?
7. **Row brushes.** Proposed: ship `RowBackground` and `AlternatingRowBackground`. Question: deprecate them when `RowStyle` arrives?
8. **`TableViewFrozenEdge.Trailing`.** Proposed: reserved, with no effect. Question: remove it until it's implemented?
9. **Error model.** Proposed: `TableViewSource` throws `E_INVALIDARG` for invalid input, while `TableView` methods return `false` or do nothing. Question: confirm the asymmetry, or align the two?
10. **Group header state properties.** Proposed: `TableViewGroupHeader.IsExpanded` and `IsExpandable` are read/write dependency properties that `TableView` sets. Question: make them read-only?
11. **`TableViewIdentitySelector` parameter name.** Proposed: the parameter is named `item`, although it receives the group key. Question: rename it to `key`?
