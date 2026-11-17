# Background

TableView is a preview tabular control for WinUI 3. It presents an `ItemsSource` as rows and a developer-defined `Columns` collection as cells, with optional column headers, gridlines, density, alternating row backgrounds, leading-prefix frozen columns, and an empty-state template. Cells are display-only: TableView has no edit mode in this release (see [Deferred to V2 / future DataGrid](#deferred-to-v2--future-datagrid)).

Rows are virtualized: TableView renders rows with `ItemsRepeater`, so large row counts are virtualized (only on-screen rows plus a small cache are realized).

This API is intentionally small: **single row selection** and **single-level grouping** on top of the read-only data presentation. Additional interactive features — multiple/extended selection, two-level hierarchy, and column resize/reorder — are planned separately. Marquee selection, multi-column sort from the column headers, column virtualization, and row headers remain out of scope for v1 (`TableViewSource.ThenSort` provides multi-level sort from code).

The API proposed for stabilization, with its behavioural contracts, parity analysis and open questions, is reviewed in [TableView API review](../../design-notes/TabularControls/TableView-API-Review.md). Where this spec and the review differ, the review and the IDLs are current.

TableView types are in the preview namespace:

```xaml
xmlns:tabular="using:Microsoft.UI.Xaml.Controls.Tabular"
```

# Conceptual pages (How To)

## How to use TableView

### Basic usage (text cells with DisplayMemberBinding)

Set `TableView.ItemsSource` and add columns to `Columns`. For a plain text cell, set `TableViewColumn.DisplayMemberBinding` (the WPF `GridViewColumn.DisplayMemberBinding` pattern): each cell is a read-only `TextBlock` bound against its row's item, vertically centred, padded for the current `Density`, and trimmed with an ellipsis. The binding's `Path` is also the column's sort key unless `SortMemberPath` is set. The cell is display-only; V1 has no cell editing. For any other cell content, use a [template column](#template-columns-custom-cell-content).

```xaml
<Page
    x:Class="Contoso.App.Views.PeoplePage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:tabular="using:Microsoft.UI.Xaml.Controls.Tabular">

    <Grid Padding="24">
        <tabular:TableView ItemsSource="{x:Bind People}">
            <tabular:TableView.Columns>
                <tabular:TableViewColumn Header="Name" DisplayMemberBinding="{Binding Name}" />
                <tabular:TableViewColumn Header="Role" DisplayMemberBinding="{Binding Role}" />
                <tabular:TableViewColumn Header="Location" DisplayMemberBinding="{Binding Location}" />
            </tabular:TableView.Columns>
        </tabular:TableView>
    </Grid>
</Page>
```

```csharp
using Microsoft.UI.Xaml.Controls;
using System.Collections.ObjectModel;

namespace Contoso.App.Views;

public sealed partial class PeoplePage : Page
{
    public ObservableCollection<Person> People { get; } =
    [
        new("Asha Rao", "Designer", "Bengaluru"),
        new("Diego García", "Engineer", "Madrid"),
        new("Mina Tanaka", "Program Manager", "Tokyo"),
    ];

    public PeoplePage()
    {
        InitializeComponent();
    }
}

public sealed record Person(string Name, string Role, string Location);
```

`DisplayMemberBinding` takes a `{Binding}`, not `{x:Bind}`: it is handed to each cell rather than evaluated against the column. A `Converter` or `TargetNullValue` on the binding applies as usual.

### Template columns (custom cell content)

Use `TableViewTemplateColumn.CellTemplate` for custom cell layout or formatting. As with WPF's `GridViewColumn` (precedence `DisplayMemberBinding`, then `CellTemplate`), a template column that also has `DisplayMemberBinding` shows the `DisplayMemberBinding` text and ignores `CellTemplate`. The examples in this spec that put a `TextBlock` in a template style it with one shared `TableCellTextStyle`, which gives it the same look as a `DisplayMemberBinding` cell at the default density:

```xaml
<!-- Written once: centred, padded to match the header, and trimmed with an ellipsis. -->
<Style x:Key="TableCellTextStyle" TargetType="TextBlock">
    <Setter Property="VerticalAlignment" Value="Center" />
    <Setter Property="Padding" Value="8,4,8,4" />
    <Setter Property="TextTrimming" Value="CharacterEllipsis" />
</Style>
```

```xaml
<Page
    x:Class="Contoso.App.Views.OrdersPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:tabular="using:Microsoft.UI.Xaml.Controls.Tabular"
    xmlns:local="using:Contoso.App.Models">

    <Grid Padding="24">
        <tabular:TableView ItemsSource="{x:Bind Orders}">
            <tabular:TableView.Columns>
                <tabular:TableViewTemplateColumn
                    Header="Order"
                    SortMemberPath="OrderNumber">
                    <tabular:TableViewTemplateColumn.CellTemplate>
                        <DataTemplate>
                            <TextBlock Text="{Binding OrderNumber}" Style="{StaticResource TableCellTextStyle}" />
                        </DataTemplate>
                    </tabular:TableViewTemplateColumn.CellTemplate>
                </tabular:TableViewTemplateColumn>

                <tabular:TableViewTemplateColumn
                    Header="Customer"
                    SortMemberPath="Customer">
                    <tabular:TableViewTemplateColumn.CellTemplate>
                        <DataTemplate>
                            <TextBlock Text="{Binding Customer}" Style="{StaticResource TableCellTextStyle}" />
                        </DataTemplate>
                    </tabular:TableViewTemplateColumn.CellTemplate>
                </tabular:TableViewTemplateColumn>

                <tabular:TableViewTemplateColumn Header="Status">
                    <tabular:TableViewTemplateColumn.CellTemplate>
                        <DataTemplate x:DataType="local:Order">
                            <Border
                                Padding="8,2"
                                CornerRadius="10"
                                Background="{ThemeResource AccentFillColorDefaultBrush}"
                                HorizontalAlignment="Left">
                                <TextBlock
                                    Text="{x:Bind Status}"
                                    Foreground="{ThemeResource TextOnAccentFillColorPrimaryBrush}" />
                            </Border>
                        </DataTemplate>
                    </tabular:TableViewTemplateColumn.CellTemplate>
                </tabular:TableViewTemplateColumn>
            </tabular:TableView.Columns>
        </tabular:TableView>
    </Grid>
</Page>
```

```csharp
using Microsoft.UI.Xaml.Controls;
using System.Collections.ObjectModel;

namespace Contoso.App.Views;

public sealed partial class OrdersPage : Page
{
    public ObservableCollection<Order> Orders { get; } =
    [
        new("SO-1001", "Tailspin Toys", "Ready"),
        new("SO-1002", "Contoso", "In review"),
        new("SO-1003", "Fabrikam", "Blocked"),
    ];

    public OrdersPage()
    {
        InitializeComponent();
    }
}

public sealed record Order(string OrderNumber, string Customer, string Status);
```

### Headers, gridlines, and density

`HeadersVisibility` controls column-header visibility (`None` or `Column`). Row headers are out of scope, so there are no `Row`/`All` values.

`GridLinesVisibility` controls the body gridlines. The column-header lines (the separators between header cells and the rule under the header) are always drawn, whatever `GridLinesVisibility` is; both paint `TabularSurfaceHeaderSeparatorBrush`, so an app hides them by overriding that resource with `Transparent`. `Density` controls row and cell spacing. Header height matches row height and header content is vertically centered; a text-cell template should centre its `TextBlock` too, as `TableCellTextStyle` does. `Density` does not reach the padding inside a `CellTemplate`; a `DisplayMemberBinding` cell's padding follows it.

> **Migrating from earlier previews:** `HeaderGridLinesVisibility` was removed. `GridLinesVisibility` affects only the body lines; to hide the header separators and the rule under the header, override `TabularSurfaceHeaderSeparatorBrush` with `Transparent`.

```xaml
<tabular:TableView
    ItemsSource="{x:Bind People}"
    HeadersVisibility="Column"
    GridLinesVisibility="Horizontal"
    Density="Compact"
    RowBackground="{ThemeResource CardBackgroundFillColorDefaultBrush}"
    AlternatingRowBackground="{ThemeResource SubtleFillColorSecondaryBrush}">
    <tabular:TableView.Columns>
        <tabular:TableViewTemplateColumn Header="Name" SortMemberPath="Name">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Name}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
        <tabular:TableViewTemplateColumn Header="Role" SortMemberPath="Role">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Role}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
        <tabular:TableViewTemplateColumn Header="Location" SortMemberPath="Location">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Location}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
    </tabular:TableView.Columns>
</tabular:TableView>
```

```csharp
using Microsoft.UI.Xaml.Controls.Tabular;

PeopleTable.HeadersVisibility = TableViewHeadersVisibility.Column;
PeopleTable.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
PeopleTable.Density = TableViewDensity.Compact;
```

### Theme customization

In all themes, an application-set table or row `Foreground` supplies the normal-state value;
pointer-over, pressed, selected, and disabled states use the corresponding `TabularSurface*`
foreground brush. Cell content inherits that foreground unless its template supplies an explicit brush.

Override the named `TabularSurface*` brush keys to customize colors. To restyle column headers
beyond their brushes (typography, padding, the template), set `ColumnHeaderStyle`. Low-level Fluent color-token
overrides no longer flow through the removed default-style brush fallbacks.

Override `TabularSurfaceSelectionIndicatorDisabledBrush` and `TabularSurfaceRowForegroundDisabledBrush`
independently to customize the disabled-selected indicator and disabled row text.
By default, the disabled-selected indicator uses disabled-text colors rather than the accent color.

The default group header's item count inherits the header `Foreground` and is dimmed by
`TabularSurfaceGroupHeaderCountOpacity` (1.0 in High Contrast). A disabled header shows the count
at full opacity, since the disabled brush already dims it.

### Frozen (pinned) leading columns

Set `FrozenEdge="Leading"` on a contiguous prefix starting at column 0 to pin those columns to the leading edge. A later `Leading` column is ignored.

```xaml
<tabular:TableView ItemsSource="{x:Bind Orders}">
    <tabular:TableView.Columns>
        <tabular:TableViewTemplateColumn
            Header="Order"
            FrozenEdge="Leading"
            SortMemberPath="OrderNumber">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding OrderNumber}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>

        <tabular:TableViewTemplateColumn
            Header="Customer"
            SortMemberPath="Customer">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Customer}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>

        <tabular:TableViewTemplateColumn
            Header="Status"
            SortMemberPath="Status">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Status}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>

        <tabular:TableViewTemplateColumn
            Header="Location"
            SortMemberPath="Location">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Location}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
    </tabular:TableView.Columns>
</tabular:TableView>
```

```csharp
using Microsoft.UI.Xaml.Controls.Tabular;

orderNumberColumn.FrozenEdge = TableViewFrozenEdge.Leading;
```

### Empty-state template

Use `EmptyTemplate` when the table has no rows.

```xaml
<tabular:TableView ItemsSource="{x:Bind Orders}">
    <tabular:TableView.EmptyTemplate>
        <DataTemplate>
            <StackPanel
                Padding="32"
                HorizontalAlignment="Center"
                VerticalAlignment="Center"
                Spacing="8">
                <FontIcon Glyph="&#xE8A5;" FontSize="32" />
                <TextBlock
                    Text="No orders yet"
                    Style="{ThemeResource SubtitleTextBlockStyle}"
                    HorizontalAlignment="Center" />
                <TextBlock
                    Text="New orders will appear here."
                    HorizontalAlignment="Center" />
            </StackPanel>
        </DataTemplate>
    </tabular:TableView.EmptyTemplate>

    <tabular:TableView.Columns>
        <tabular:TableViewTemplateColumn Header="Order" SortMemberPath="OrderNumber">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding OrderNumber}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
        <tabular:TableViewTemplateColumn Header="Customer" SortMemberPath="Customer">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Customer}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
        <tabular:TableViewTemplateColumn Header="Status" SortMemberPath="Status">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Status}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
    </tabular:TableView.Columns>
</tabular:TableView>
```

```csharp
Orders.Clear();
```

### Keyboard navigation & accessibility

TableView supports cell focus on actionable cells. Up, Down, PageUp, and PageDown move to the same visible column in another displayed row (the nearest actionable cell, else the row); Left and Right move the cell cursor among the actionable cells of the focused row. Home and End move to the first or last actionable cell of the current row once a cell has focus, and Ctrl+Home / Ctrl+End move to the first or last row of the table. Enter and Space on a column header sort that column.

Read-only UI Automation grid/table peers are provided for the table, rows, cells, and column headers.

The keyboard focus model is described in the [functional spec](../../design-notes/TabularControls/TableView-functional-spec.md#keyboard-focus-and-accessibility-model).

```xaml
<tabular:TableView
    ItemsSource="{x:Bind People}"
    HeadersVisibility="Column"
    GridLinesVisibility="All">
    <tabular:TableView.Columns>
        <tabular:TableViewTemplateColumn Header="Name" SortMemberPath="Name">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Name}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
        <tabular:TableViewTemplateColumn Header="Role" SortMemberPath="Role">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Role}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
        <tabular:TableViewTemplateColumn Header="Location" SortMemberPath="Location">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Location}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
    </tabular:TableView.Columns>
</tabular:TableView>
```

C++/WinRT usage follows the same model: create a `Microsoft::UI::Xaml::Controls::Tabular::TableView`, populate its `Columns()` vector, and set `ItemsSource()` to a bindable collection.

### Selecting rows

Selection is **on by default** — `SelectionMode` defaults to `Single`. Set it to `None` for a display-only table. `SelectedItem`/`SelectedIndex` are read-only projections you can observe or bind *from*; drive selection with `Select`/`Deselect`/`DeselectAll`:

```xaml
<tabular:TableView
    x:Name="PeopleTable"
    ItemsSource="{x:Bind People}"
    SelectionMode="Single"
    SelectionChanged="OnSelectionChanged" />
```

```csharp
private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
{
    // One event carries the whole delta: replacing a selection reports both vectors.
    foreach (var removed in args.RemovedItems) { /* ... */ }
    foreach (var added in args.AddedItems) { /* ... */ }

    // The state has already settled, so these agree with the args.
    DetailsPane.Content = sender.SelectedItem;
}
```

Or drive it from code:

```csharp
PeopleTable.Select(0);                   // by index
bool isSelected = PeopleTable.IsSelected(0);
PeopleTable.DeselectAll();
```

Selection follows the **item**, not the index. Inserting a row above the selected one keeps the same item selected and simply shifts `SelectedIndex` (no `SelectionChanged`); removing the selected item clears the selection rather than quietly handing the app the row that took its place.

Selection is also independent of the current cell: the current cell is unaffected by what is selected, and vice versa.

### Grouping rows

Grouping is declared on the **data source**, not on the control. Wrap your collection in a `TableViewSource`, declare a `GroupBy`, and hand that to `ItemsSource`:

```csharp
var source = TableViewSource.From(People)
                            .GroupBy(item => ((Person)item).Department);

PeopleTable.ItemsSource = source;
```

```xaml
<tabular:TableView x:Name="PeopleTable">
    <tabular:TableView.GroupHeaderTemplate>
        <DataTemplate x:DataType="tabular:TableViewGroupInfo">
            <TextBlock>
                <Run Text="{x:Bind KeyText}" FontWeight="SemiBold" />
                <Run Text="{x:Bind ItemCountText}" />
            </TextBlock>
        </DataTemplate>
    </tabular:TableView.GroupHeaderTemplate>
</tabular:TableView>
```

`GroupHeaderTemplate` fills only the **content region** of the control-owned group-header band; the chevron and the themed background stay with the control, so a header template cannot accidentally remove the expander. Its data context is a `TableViewGroupInfo`, which exposes the raw `Key` and `ItemCount` as well as the culture-formatted `KeyText` and `ItemCountText`. Binding only `Key`/`ItemCount` costs nothing extra — the formatted strings are computed lazily.

The group key is any object, but its **string identity** is what buckets rows. `String`, `Int32`, `Int64`, `Guid`, `Boolean` and enum keys have a built-in identity and work with the single-argument overload. A key of any other (reference) type needs the two-argument overload, which names the identity explicitly:

```csharp
var source = TableViewSource.From(People)
                            .GroupBy(item => ((Person)item).Team,          // key object
                                     key => ((Team)key).Id);               // stable string identity
```

The identity selector receives the key object returned by the key selector, not the original source item.

If an identity cannot be produced — it is empty, it is not a string, the selector throws, or two genuinely different keys collapse onto the same identity without the identity overload — the call **throws** rather than quietly rendering an ungrouped table, so the mistake surfaces at development time.

This includes a key that is `null` or the **empty string**, even though `String` is a supported key type: `""` is not a usable bucket identity. Group on a property that can be blank or null and you must supply the fallback yourself:

```csharp
.GroupBy(item => ((Person)item).Department is string d && d.Length > 0 ? d : "(none)")
```

Grouping composes with filtering and sorting, and the order of the calls never matters. By default (`SortGroups(None)`) each group appears where its first item falls in the filtered, sorted sequence, as in WPF, so sorting the rows can also reorder the groups. To keep groups in key order regardless of the row sort, pin it with `SortGroups`:

```csharp
var source = TableViewSource.From(People)
                            .Filter(item => item is Person p && p.IsActive)
                            .GroupBy(item => ((Person)item).Department)
                            .SortGroups(SortDirection.Ascending)           // pins the GROUPS to key order
                            .Sort("Name", SortDirection.Ascending)         // orders rows WITHIN each group
                            .ThenSort("Id", SortDirection.Ascending);      // tie-breaker
```

`Sort` replaces every item sort level; `ThenSort` appends one. A column-header sort also replaces the item sort levels and keeps `SortGroups`: with `SortGroups(None)` it can reorder the groups, with `Ascending`/`Descending` it only reorders rows within each group. A filter always runs first, so a group whose last remaining row is filtered out disappears entirely. `ClearGroupBy()` removes grouping; a second `GroupBy` replaces the first rather than nesting, since v1 is single-level.

Every group can be expanded and collapsed by clicking or tapping the header band, or through its UI Automation `ExpandCollapse` pattern. From code, expand or collapse one group by key, or every group at once:

```csharp
PeopleTable.ExpandGroup("Sales");
PeopleTable.CollapseGroup("Sales");
PeopleTable.ExpandAllGroups();
PeopleTable.CollapseAllGroups();
```

`GroupExpanding` and `GroupCollapsed` are raised for every group whose state changes, whatever caused it.

Expansion is remembered as intent about a **group identity**, not as state on a group object, so collapsing a group and then re-sorting, re-filtering or regrouping keeps that group collapsed — and a group that appears later inherits whatever `ExpandAllGroups`/`CollapseAllGroups` last established, rather than an intent nobody expressed about it.

One thing to know when combining grouping with selection: group headers occupy the same index space as data rows, so under grouping `SelectedIndex` is **not** an index into `ItemsSource`. Use `IndexOf(item)` to get an item's index in that space. Headers themselves are never selectable — `Select(index)` on a header index is a no-op that keeps the current selection, `IsSelected` on it is `false`, and keyboard selection-follows-focus leaves the selection alone while focus is on a header. `StartBringItemIntoView` does accept a header index and brings the band into view.

### Editing cells

TableView has no edit mode in this release: no `IsReadOnly`, edit gestures, edit events, or `CellEditingTemplate`. Controls inside a `TableViewTemplateColumn.CellTemplate` are always live and two-way bound by the app. For rename-in-place and similar single-field edits, see [Rename-in-place guidance](#rename-in-place-guidance); full editing is [deferred to V2 / a future DataGrid](#deferred-to-v2--future-datagrid).

# API Pages

## TableView class

Represents a preview tabular control. Its cells are display-only; it has no edit mode.

`TableView` derives from `Microsoft.UI.Xaml.Controls.Control` and has `Columns` as its content property.

Template parts:

| Part | Type | Description |
|---|---|---|
| `PART_HeaderRow` | FrameworkElement | Header row root. |
| `PART_HeaderHost` | `TableViewCellsPanel` (typed `Panel`) | Host for column header elements. See [TableViewCellsPanel class](#tableviewcellspanel-class). |
| `PART_RowsRepeater` | `ItemsRepeater` | Repeater that displays rows. |
| `PART_EmptyStatePresenter` | `ContentControl` | Optional presenter for `EmptyTemplate`. |

Scrolling is structural rather than a named part. The header scroller and the body scroller are the nearest `ScrollViewer` ancestors of `PART_HeaderHost` / `PART_RowsRepeater` **inside the template**; their names are not looked up. The search stops at the `TableView`, so a template that omits them never latches onto a `ScrollViewer` in the app's own tree. The header scroller is horizontal-only and kept in sync with the body. The body scroller supplies the viewport for `*` column widths, the offset for frozen-column pinning, and the target of keyboard paging and scroll-into-view; without it, `*` columns keep their provisional width, frozen columns are not pinned, and the header is not scroll-synced. The default template supplies both, named `PART_HeaderScroller` and `PART_BodyScroller` for readability only.

### Example Usage

```xaml
<tabular:TableView
    x:Name="PeopleTable"
    ItemsSource="{x:Bind People}"
    HeadersVisibility="Column"
    GridLinesVisibility="All"
    Density="Standard">
    <tabular:TableView.Columns>
        <tabular:TableViewTemplateColumn Header="Name" SortMemberPath="Name">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Name}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
        <tabular:TableViewTemplateColumn Header="Role" SortMemberPath="Role">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Role}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
        <tabular:TableViewTemplateColumn Header="Location" SortMemberPath="Location">
            <tabular:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Location}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tabular:TableViewTemplateColumn.CellTemplate>
        </tabular:TableViewTemplateColumn>
    </tabular:TableView.Columns>
</tabular:TableView>
```

## TableView properties

| Property | Type | Default | Description |
|---|---|---|---|
| `ItemsSource` | `Object` | `null` | Source collection for table rows. |
| `Columns` | `IVector<TableViewColumn>` | Empty vector | Developer-defined column collection. This is the content property. |
| `HeadersVisibility` | `TableViewHeadersVisibility` | `Column` | Controls column-header visibility. |
| `GridLinesVisibility` | `TableViewGridLinesVisibility` | `All` | Controls the body's horizontal (row) and vertical (column) gridlines. |
| `Density` | `TableViewDensity` | `Standard` | Controls row/cell spacing. |
| `RowBackground` | `Brush` | `null` | Background brush for rows. |
| `AlternatingRowBackground` | `Brush` | `null` | Optional alternating row background for banding. |
| `EmptyTemplate` | `DataTemplate` | `null` | Template displayed when there are no rows. |
| `GroupHeaderTemplate` | `DataTemplate` | `null` | Content template for the group-header band shown when the source is grouped. Fills only the header's content region; the chevron and themed band remain control-owned. The data context is a `TableViewGroupInfo`. Ignored when the source is not grouped. |
| `ColumnHeaderStyle` | `Style` | `null` | `Style` applied to every `TableViewColumnHeader`. `TableViewColumn.HeaderStyle` overrides it for one column. Layered over the default header style, so it need not be `BasedOn` it. See [TableViewColumnHeader class](#tableviewcolumnheader-class). |
| `SelectionMode` | `TableViewSelectionMode` | `Single` | Gates row selection for the whole control. Selection is on by default, matching `ItemsView`, `ListView` and WPF's `DataGrid`; set `None` for a display-only table, which also clears any selection. |
| `SelectedItem` | `Object` | `null` | The selected data item. **Read-only** — drive selection through `Select`/`Deselect`/`DeselectAll`. |
| `SelectedIndex` | `Int32` | `-1` | The selected item's index in the displayed row projection; `-1` is "nothing selected". Under grouping, group headers occupy projection indexes but cannot be selected, so this value is not necessarily an index into `ItemsSource`. **Read-only.** |

`SelectedItem` and `SelectedIndex` are two read-only views of the same state and are always coherent. Both are dependency properties, so they can be bound *from* — `{x:Bind Table.SelectedItem.Name, Mode=OneWay}` — but not written. This matches `ItemsView`, whose `SelectedItem` and `CurrentItemIndex` are likewise get-only, and diverges from `Selector`/`ListView`, whose settable properties require a deferral-and-coercion layer to survive markup ordering and two-way bindings. Apps that need to drive selection from a view model do so through `Select(index)`.

Selection follows the **item**, not the index. Inserting a row above the selected one keeps the same item selected (its `SelectedIndex` shifts, with no `SelectionChanged`); removing the selected item clears the selection instead of selecting whatever slid into that slot. Replacing `ItemsSource` outright always clears the selection, even if the new source contains the same item.

A selection requested before it can be resolved is **not** queued: `Select(index)` called before `ItemsSource` is set, or while `SelectionMode` is `None`, is a no-op — matching `ItemsView`, whose `Select` is a straight pass-through to `SelectionModel`, and matching this control's own pointer and keyboard paths, which have always ignored a gesture while selection is off. Unlike markup, the caller controls ordering here, so a request that fired later when the mode or source changed would surprise more than it would help.

The one thing that *is* held is the live selection across an unload/reload cycle: unload drains the repeater's `ItemsSource` to release cache work on a detached subtree, and re-sourcing on load hands `SelectionModel` a new view, which clears it. The selected item is stashed and re-selected by identity on reload. `ItemsView` needs no equivalent because it never drains its repeater on unload.

An item that *is* resolvable but is not in `ItemsSource` has no index, so it clears the selection — matching `ListView`.

The current cell is the navigation position. It is set by keyboard navigation and by focusing a cell, and is independent of selection.

## TableView selection methods

All mutators resolve immediately when selection is on and a source exists; otherwise they are a no-op — `Select` never clears an existing selection to signal rejection. The only state carried across time is the live selection stashed over an unload/reload cycle, which `DeselectAll` and `SelectionMode = None` both discard.

Named to match `ItemsView`, which ships `Select` / `Deselect` / `IsSelected` over an item index. Identity-based `Select` overloads are deliberately absent, matching `ItemsView`; use `IndexOf(item)` to map an item to an index.

All index members (`Select`, `Deselect`, `IsSelected`, `SelectedIndex`, `IndexOf`, `StartBringItemIntoView`) use **display-projection indexes**: group headers take slots and rows under a collapsed group or parent are absent. Indexes shift whenever a group or row is expanded or collapsed and whenever the data is sorted, filtered, regrouped or changed, so call `IndexOf` immediately before `Select` or `StartBringItemIntoView` instead of caching an index. Data-index APIs (for example `GetItemFromDisplayIndex`, or item-based overloads) are deferred.

Group-header slots: `Select(headerIndex)` is a silent no-op that keeps the current selection (it neither selects the header nor clears); `IsSelected(headerIndex)` is always `false`, so `Deselect(headerIndex)` does nothing; `StartBringItemIntoView(headerIndex)` is valid and brings the group header band into view; `IndexOf` never returns a header slot (a `TableViewGroupInfo` or other header object returns `-1`). An out-of-range `Select` is also a no-op, while an out-of-range `StartBringItemIntoView` throws `E_INVALIDARG`.

| Method | Returns | Description |
|---|---|---|
| `Select(Int32 index)` | `void` | Selects the item at `index`. A negative index clears the selection. |
| `Deselect(Int32 index)` | `void` | Clears the selection only when `index` is the selected index, so a stale call cannot clobber a newer selection. |
| `IsSelected(Int32 index)` | `Boolean` | Whether `index` is the selected index. |
| `DeselectAll()` | `void` | Clears the selection. Works regardless of `SelectionMode`, so turning selection off can always be made to stick. |
| `IndexOf(Object item)` | `Int32` | The item's index in the displayed projection (the same space as `Select` and `SelectedIndex`, where group headers take slots). `-1` for null, or when the item is not displayed (absent, filtered out, or under a collapsed group or parent). Value-typed items (boxed numbers, strings) match by value, reference items by identity; duplicates, including equal boxed values, resolve to the first displayed match. Works before `Loaded`. |
| `StartBringItemIntoView(Int32 index, BringIntoViewOptions options)` | `void` | Asynchronously realizes the row at `index`, lays it out, and brings its real bounds into view, accounting for the sticky header and frozen columns. Same shape as `ItemsView`. `options` may be null; an out-of-range index throws `E_INVALIDARG`. Safe before `Loaded`. A pending request is cancelled when `ItemsSource` changes or the table unloads. |

## TableView selection events

| Event | Args | Description |
|---|---|---|
| `SelectionChanged` | `SelectionChangedEventArgs` | Raised after the selection has settled. Reading `SelectedItem`, `SelectedIndex`, or a row's `IsSelected` inside the handler observes the new state. Replacing a selection raises **one** event carrying both the removed and the added item. |

Selection is raised only for real changes: a re-select of the already-selected row, or an index shift caused by a collection reshape, does not raise `SelectionChanged`.

### Selection gestures

| Gesture | Behaviour |
|---|---|
| Pointer | Selects on **release**, for every pointer type — touch included, which reports no pressed button and so is admitted on device type rather than button state — matching `ListViewBaseItem` — which starts only timers and visuals on press and routes the actual selection through the tap interaction on release. Committing on press would select the row a touch pan started on, and would select on a press the user drags away from and cancels. The event is left **unhandled** so other handlers of the same press still run. |
| <kbd>Up</kbd>/<kbd>Down</kbd>/<kbd>PageUp</kbd>/<kbd>PageDown</kbd> | Selection follows the row reached by the keyboard cursor. This matches `ListView`'s default, where `SingleSelectionFollowsFocus` is `true`. When focus is not on a row or cell (the user clicked a header, or tabbed away and back), <kbd>Up</kbd>/<kbd>Down</kbd> resume from the **selected** row rather than restarting at the top. |
| <kbd>Left</kbd>/<kbd>Right</kbd> | Moves the cell cursor within the focused row without changing row selection. |
| <kbd>Home</kbd>/<kbd>End</kbd> | Moves the cell cursor to the first or last visible cell of the current row without changing row selection. |
| <kbd>Ctrl</kbd> + <kbd>Home</kbd>/<kbd>End</kbd> | Moves focus to the first or last cell in the table without changing selection. |
| <kbd>Space</kbd> | Selects the focused row without moving. Only when the row itself has focus — a <kbd>Space</kbd> inside a cell's interactive content belongs to that control. |
| <kbd>Ctrl</kbd> + click, <kbd>Ctrl</kbd> + <kbd>Space</kbd> | Toggles: selects an unselected row, and **deselects the selected one**. Matches `SingleSelector` and `ListViewBase` single-selection behaviour, and is the only gesture that can clear a selection — without it the app would have to call `DeselectAll`. |
| <kbd>Ctrl</kbd> + navigation key | Moves the focus cursor **without** changing the selection, matching `ListViewBase`. This is how a keyboard or screen-reader user reviews other rows and returns without disturbing the selection. |

### Known limitations

These are understood and deliberately not addressed by single selection:

- **The selection indicator scrolls with the row.** `PART_SelectionIndicator` lives inside the horizontally scrolling row content, so the accent strip scrolls off the leading edge. `TreeViewItem` and `ItemContainer` pin theirs to the container; doing the same here likely means reusing the frozen-column offset mechanism.
- **A `TableViewTemplateColumn` whose `CellTemplate` sets an explicit `Foreground` overrides the selected foreground.** `PART_CellForegroundPresenter` only reaches cells that *inherit* `Foreground`. A text cell whose `TextBlock` sets no `Foreground` inherits it correctly; template columns are free to set one, and in High Contrast that renders app-chosen text over `SystemColorHighlightColor`. Hovered, pressed and selected cells also turn off High Contrast text backplates (`HighContrastAdjustment="None"` on `PART_CellForegroundPresenter`, which cell content inherits), because the control paints its own high-contrast highlight pair; such app-coloured text therefore gets no automatic backplate either. Template columns should leave `Foreground` unset unless they take responsibility for the selected and High Contrast cases.
- **Reconciliation order is load-bearing.** Row chrome is restamped from `ItemsSourceView.CollectionChanged`, which is correct only because `SelectionModel` is handed the repeater's *shared* `ItemsSourceView` and is subscribed ahead of the control. Handing the model a raw source, or reordering those two calls in `ResolveSelectionAfterSourceChange`, silently reintroduces stale-index stamping — and an insert above the selection raises no event to correct it. This is deliberately different from `ItemsView`, which hands the model a raw source and repairs the resulting race afterwards with a dispatcher hop; the ordering here is structural instead.
- **Adding `Multiple`/`Extended` is not purely additive.** The enum values are appended and the event args already carry both vectors, so the shapes that are expensive to reverse are settled. But `SelectedItems` is deliberately **not** exposed in this release — it would be redundant with `SelectedItem` while at most one row can be selected, and `SelectionModel`'s view leaves `IndexOf` and `GetMany` unimplemented, so `Contains`, `ToList` and `ToArray` throw. It should be added with `Multiple`, where it becomes the only way to read the whole selection and those sharp edges are worth the capability. The gesture layer also routes through `SelectRowIndexFromInteraction(index, toggle)`, which carries no anchor or range state, and `ApplySelection` encodes single-selection semantics; multi-selection needs modifier state threaded through those entry points and an anchor model, closer to `ItemsView`'s `SelectorBase` strategy split.

## TableView grouping members

Grouping itself is declared on the data source (`TableViewSource.GroupBy`); the control contributes presentation, expand/collapse commands and expansion events.

| Member | Kind | Description |
|---|---|---|
| `GroupHeaderTemplate` | `DataTemplate` DP | Content template for the group-header band. The data context is a `TableViewGroupInfo`. |
| `ExpandGroup(Object groupKey)` | method | Expands the group whose key matches. The key is resolved to a group identity exactly as `GroupBy` does (the built-in identity, or the source's `TableViewGroupIdentitySelector`), so a null key and `""` never match each other. An unknown key, or a group already expanded, is a no-op and raises no event. |
| `CollapseGroup(Object groupKey)` | method | Collapses the matching group, on the same terms. |
| `ExpandAllGroups()` | method | Expands every group, including groups that do not exist yet — it moves the default rather than looping over the realized headers. No-op when the source is not grouped. |
| `CollapseAllGroups()` | method | Collapses every group, on the same terms. No-op when the source is not grouped. |
| `GroupExpanding` | event (`TableViewGroupExpandingEventArgs`: `GroupKey`) | Raised synchronously before a group expands. Not cancellable. Once raised, the direction is pinned: a handler that calls `ExpandGroup` on the same group is a no-op. |
| `GroupCollapsed` | event (`TableViewGroupCollapsedEventArgs`: `GroupKey`) | Raised after a group collapses. |

`GroupExpanding` and `GroupCollapsed` are raised for every group whose state changes, from any source: pointer, keyboard, UI Automation, `ExpandGroup`/`CollapseGroup`, and the bulk methods (once per group that changes). A request made during layout is posted whole to the next turn, so the events are never raised during measure or arrange or after the state has already changed. Handlers may change the source (for example to load the group's rows lazily); the control defers the source's refresh while `GroupExpanding` runs, so the change is coalesced into one reshape and the expansion is kept. An exception thrown by a group-event handler is contained and logged.

Expansion is stored as intent keyed by a group's **identity**, so it survives a reshape (sort, filter, or regroup) that re-creates every group object. Setting the bulk state also resets per-group exceptions, which is what makes "collapse all, then expand two, then re-sort" behave the way the user expects.

## TableViewSource grouping verbs

`TableViewSource` wraps an app collection and declares the shape the control should render. It accepts the same collection interfaces as `ItemsSourceView`; anything else throws `E_INVALIDARG`. The verbs change this source and return it, so they chain. The full member list (`Filter`, `Sort`, `ThenSort`, `SortGroups`, `ParentBy`, `Refresh`, `DeferRefresh`, `Source`, `IsLiveShaping`) and its contracts are in the [API review](../../design-notes/TabularControls/TableView-API-Review.md#21-data-and-shaping-tableviewsource).

| Member | Returns | Description |
|---|---|---|
| `TableViewSource.From(Object items)` | `TableViewSource` | Wraps a collection. The projection is populated by the time `From` returns. `Source` returns `items`. |
| `GroupBy(TableViewKeySelector keySelector)` | `TableViewSource` | Groups rows by the key the selector returns. The key's identity must be resolvable by the built-in value-type identity (`String`, `Int32`, `Int64`, `Guid`, `Boolean`, enum). A null `keySelector` throws `E_INVALIDARG`; use `ClearGroupBy()` to remove grouping. |
| `GroupBy(TableViewKeySelector keySelector, TableViewGroupIdentitySelector groupIdentitySelector)` | `TableViewSource` | Same, with the group's stable string identity supplied explicitly. Required for reference-typed keys. Supplying an identity selector is also the opt-in that makes two distinct key instances sharing one identity intentional rather than a collision. A null `groupIdentitySelector` selects the built-in identity. |
| `SortGroups(SortDirection direction)` | `TableViewSource` | `Ascending`/`Descending` orders groups by key, and item sorts then order rows only within groups. `None` (the default) orders groups by the position of each group's first item in the filtered, item-sorted sequence (source order when there is no item sort), matching WPF `ListCollectionView`. Survives `GroupBy` replacement, `ClearGroupBy`, `ClearSort`, `Sort` and column-header sorts. |
| `ClearGroupBy()` | `TableViewSource` | Removes grouping. |

Contract notes:

- **Single level.** A second `GroupBy` replaces the first; grouping axes do not nest or stack.
- **Fail fast.** An identity that is empty, non-string, produced by a throwing selector, or shared by two genuinely different keys without the identity overload throws `hresult_invalid_argument` at the point the projection is built. The projection does not silently fall back to ungrouped. A `null` key and an empty-string key are both rejected on these terms, so a group key derived from a nullable or possibly-blank property must be coalesced to a real label by the app.
- **Ordering.** Group order never depends on call order. `Sort`/`ThenSort` order rows within each group, whether declared before or after `GroupBy`; under `SortGroups(None)` they also determine group order through each group's first item. A filter applies before either, and a group whose last member is filtered out disappears.
- **Rows are identified by object identity.** There is no app-supplied row key, so selection and focus re-anchor across a reshape, but not across an item being re-created; the same object appearing in two rows fails fast.
- **UI-thread affine**, like any XAML items source. The wrapped collection must also raise its change notifications on that thread.

## TableViewGroupInfo class

The read-only projection a `GroupHeaderTemplate` binds against. It implements `INotifyPropertyChanged` and is updated **in place** while the header represents the same group. Recycling the header to another group creates a new projection, so a reference retained by an open context menu never changes group identity.

| Property | Type | Description |
|---|---|---|
| `Key` | `Object` | The `GroupBy` key for this group. |
| `ItemCount` | `Int32` | Number of members in the group. |
| `IsExpandable` | `Boolean` | `false` when the group cannot expand (for example, it has no members). |
| `IsExpanded` | `Boolean` | Current expansion state. |
| `KeyText` | `String` | Culture-formatted display text for `Key`, computed lazily. |
| `ItemCountText` | `String` | Culture-formatted display text for `ItemCount`, computed lazily. |

## TableViewGroupHeader class

The realized group-header container: a templated `ContentControl` whose `Content` is the `TableViewGroupInfo` and whose `ContentTemplate` is the app's `GroupHeaderTemplate`. Making it a templated control rather than a code-built visual tree means the chevron and themed band live in a control-owned `ControlTemplate`, so a header content template cannot drop them; replacing `Style`/`Template` outright still hands the whole visual to the app, which is why the parts are documented.

| Member | Kind | Description |
|---|---|---|
| `IsExpanded` | `Boolean` DP (read-only) | Expansion state of this header. Written by the control; change it with `TableView.ExpandGroup`/`CollapseGroup`. |
| `IsExpandable` | `Boolean` DP (read-only) | Whether this header can expand. |

The header forwards activation to its owning `TableView`, which raises `GroupExpanding`/`GroupCollapsed`; the header itself raises no events. Both DPs revert an external `SetValue` rather than throwing (WinUI has no `DependencyPropertyKey`), so observers can see a transient change notification; bind them `OneWay` only.

Template parts. Code looks up `PART_ContentPresenter` and `PART_GridLineBorder`, and degrades without either. The default visual states, however, target `PART_Root`, `PART_ContentPresenter` and `PART_ExpanderIcon` by name, so a template that keeps those state definitions must declare all three:

| Part | Type | Purpose |
|---|---|---|
| `PART_Root` | `Border` | The band. `CommonStates` sets its `Background` for `PointerOver` and `Pressed`. |
| `PART_ContentPresenter` | `ContentPresenter` | The app-customizable content region, showing `ContentTemplate` over the `TableViewGroupInfo`. `CommonStates` sets its `Foreground` and `HighContrastAdjustment`. |
| `PART_ExpanderGutter` | `Border` | Reserves the chevron column so header content stays aligned with the rows below. It keeps its width when the chevron hides. |
| `PART_ExpanderIcon` | `FontIcon` | Glyph driven by `ExpansionStates`, not by code; hidden by `NotExpandable`; recoloured by `CommonStates`. |
| `PART_GridLineBorder` | `Border` | Optional. Renders the border chrome; on the last row the control clears its bottom edge so it does not double the table's own border. |

Visual states: `CommonStates` (`Normal`/`PointerOver`/`Pressed`/`Disabled`), `ExpansionStates` (`Expanded`/`Collapsed`), `ExpandabilityStates` (`Expandable`/`NotExpandable`).

The **whole band** is the toggle target, matching `ListView` and `TreeView`, rather than a nested button inside it — one interactive element, one automation story.

## TableViewColumnHeader class

The realized column header: a templated `ContentControl`, one per column in `PART_HeaderHost`. The `TableView` creates and owns the headers, so the class is sealed and has no public constructor; apps restyle them through `TableView.ColumnHeaderStyle` or `TableViewColumn.HeaderStyle`, and fill the content region through the column's `Header`, `HeaderTemplate` or `HeaderTemplateSelector`. Its only member of its own is the read-only `Column`, the `TableViewColumn` the header represents.

```xml
<tabular:TableView.ColumnHeaderStyle>
    <Style TargetType="tabular:TableViewColumnHeader">
        <Setter Property="FontWeight" Value="Normal" />
        <Setter Property="Background" Value="{ThemeResource LayerFillColorDefaultBrush}" />
    </Style>
</tabular:TableView.ColumnHeaderStyle>
```

The default style sets `FontSize` and `FontWeight` from `{ThemeResource TableViewHeaderFontSize}` and `{ThemeResource TableViewHeaderFontWeight}` (14, `SemiBold`), a transparent `Background` so the whole header takes pointer input, and `IsTabStop`/`UseSystemFocusVisuals`. The header inherits `Foreground` from the `TableView`. `MinHeight` and `Padding` follow `Density` unless the applied `ColumnHeaderStyle`/`HeaderStyle` sets them. A keyboard-focused header takes the `TableView`'s `CornerRadius` so its focus visual is rounded, unless the applied style sets `CornerRadius`. A style's `ContentTemplate`, or a `ContentTemplateSelector` that returns a template, also renders a string `Header`; the column's `HeaderTemplate`/`HeaderTemplateSelector` wins over it. A `ColumnHeaderStyle` or `HeaderStyle` whose `TargetType` is not `TableViewColumnHeader` or one of its base types is ignored.

Template parts. All are optional; a missing part disables only its feature:

| Part | Type | Purpose |
|---|---|---|
| `PART_ContentPresenter` | `ContentPresenter` | Content region. On a sortable column the control adds the sort glyph's width to its right `Margin`, so text trims before the glyph. |
| `PART_GridLine` | `Border` | Vertical separator. Always shown, except the last column's where it would double the outer border; the default template paints it with `TabularSurfaceHeaderSeparatorBrush`. |
| `PART_SortIndicatorHost` | `Border` | Hosts the sort glyph on a sortable column. Under a High Contrast highlight state the glyph takes `PART_ContentPresenter`'s `Foreground`. |
| `PART_ResizeGripperHost` | `Border` | Hosts the resize gripper on a resizable column. Without it the column cannot be resized from the header, by pointer, keyboard or UI Automation. |

Visual states:

| Group | States | Notes |
|---|---|---|
| `CommonStates` | `Normal`, `PointerOver`, `Pressed`, `Disabled` | `PointerOver`/`Pressed` are entered only on a column the user can sort, and change only the fill (`TabularSurfaceHeaderBackgroundPointerOverBrush` / `...PressedBrush`). `Disabled` dims the text and sort glyph with `TabularSurfaceRowForegroundDisabledBrush`, as on rows. |
| `HighContrastStates` | `HighContrastNormal`, `HighContrastPointerOver`, `HighContrastPressed` | Entered alongside `PointerOver`/`Pressed` only under High Contrast: text and glyph switch to `TabularSurfaceHeaderForeground*Brush` and drop the text backplate. |
| `SortStates` | `Unsorted`, `SortAscending`, `SortDescending` | Show or hide the sort glyph; the glyph is hidden until the column is sorted. |

The header's automation peer is `TableViewColumnHeaderAutomationPeer` (`HeaderItem`, with `Invoke` to sort and `Transform` to resize). A `ContextFlyout` set through a header style takes precedence over `HeaderContextFlyout`.

## TableViewColumn class

Base class for TableView columns.

`TableViewColumn` derives from `Microsoft.UI.Xaml.DependencyObject` and has `Header` as its content property.

`Width` accepts `Pixel`/`Auto`/`*` (`GridLength`). `Pixel` is exact; `Auto` sizes to the widest realized cell and the current header; the cell width grows within a data set (it does not shrink as rows scroll out of view), while the header width follows the header as it is now (hiding the headers drops it, and a shorter header or turning off sorting narrows it); `*` takes a proportional share of the body viewport left after the fixed columns. `MinWidth`/`MaxWidth` clamp every mode. Auto widths reset when `ItemsSource` is assigned a new value or the whole `Columns` collection is replaced or reset, and for a single column when that column is added or replaced or its `Header`, `HeaderTemplate`, `HeaderTemplateSelector`, `CellTemplate` or `DisplayMemberBinding` changes; they do not reset on an in-place collection `Reset` or clear-and-refill, a `TableViewSource` filter or refilter, or a `Density` change; to re-fit, re-assign `ItemsSource` (or give the column a `Pixel` `Width`).

| Property | Type | Default | Description |
|---|---|---|---|
| `Header` | `Object` | `null` | Header content for the column. |
| `HeaderTemplate` | `DataTemplate` | `null` | Template used to display the header. |
| `HeaderTemplateSelector` | `DataTemplateSelector` | `null` | Template selector used to display the header; takes precedence over `HeaderTemplate` when both are set. |
| `HeaderStyle` | `Style` | `null` | `Style` for this column's `TableViewColumnHeader`; wins over `TableView.ColumnHeaderStyle`. |
| `HeaderToolTip` | `Object` | `null` | Opt-in tooltip for this column's header. The value is the tooltip's content — a string, or anything a `ToolTip` can host, but *not* a `ToolTip` itself: the control owns the `ToolTip` and its placement, so a `ToolTip` value is rejected and the header gets none. `null` or an empty string means no header tooltip. A `UIElement` is parented by that header's `ToolTip`, so each column needs its own instance. A dependency property, not a `Binding`: a header is not bound against a row, so there is nothing to defer. |
| `HeaderContextFlyout` | `Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase` | `null` | Context flyout for this column's header. Opens at the header cell; it does not receive a row data item. |
| `Width` | `GridLength` | Default column width | `Pixel` = exact; `Auto` = widest realized cell (grows within a data set) or the current header; `*` = a proportional share of the body viewport after fixed columns (see [Locked Star columns](#locked-star-columns) for a `*` column with `CanUserResize="False"`). |
| `MinWidth` | `Double` | `20` | Minimum column width. |
| `MaxWidth` | `Double` | Infinity | Maximum column width. |
| `ActualWidth` | `Double` | `120` | Read-only realized width. |
| `CanUserResize` | `Boolean` | `true` | Whether the user can resize this column (pointer, keyboard, UI Automation); also requires `TableView.CanUserResizeColumns`. |
| `CanUserSort` | `Boolean` | `true` | Whether the user can sort by this column's header. Gates user input only: `SortByColumn` and `ToggleSortDirection` ignore it, and turning it off does not clear an active sort. The table-level `CanUserSortColumns` and `CanUserResizeColumns` follow the same rule. |
| `SortDirection` | `SortDirection` | `None` | The header's sort arrow. **Display only**: the control sets it when it sorts; an app that sorts itself (for example after cancelling `Sorting`) can set it. Setting it never reshapes data, and a non-`None` value clears the arrow on other columns. If a column's `CustomSortComparer` throws during a header sort or `SortByColumn`, the sort is aborted before commit: the previous sort and arrow stay, and `Sorted` is not raised. A value set in XAML or before the column is added is applied when it joins `Columns`. |
| `SortMemberPath` | `String` | Empty string | Property path the column sorts by. When empty, the column sorts by `DisplayMemberBinding`'s `Path` (unless that binding names its own `Source`, `RelativeSource` or `ElementName`), and an app-derived column's `GetSortMemberPathCore()` override can supply it; otherwise the column sorts only through `CustomSortComparer`. |
| `SortCycle` | `TableViewSortCycle` | `AscendingDescending` | Order of directions a header click or `ToggleSortDirection` steps through; the `*None` values add a step back to the unsorted order. |
| `CustomSortComparer` | `ITableViewSortComparer` | `null` | Comparer for this column's sort; wins over `SortMemberPath`. Must be pure. |
| `Visibility` | `Visibility` | `Visible` | Column visibility. |
| `FrozenEdge` | `TableViewFrozenEdge` | `None` | Pins only when the column is in the contiguous leading prefix. Later `Leading` columns are ignored. |
| `DisplayMemberBinding` | `Binding` | `null` | Read-only text for each cell without a template, as WPF's `GridViewColumn.DisplayMemberBinding`. Each cell is a `TextBlock` whose `Text` takes this binding against its row's item; it is vertically centred, padded for `Density`, and trimmed with an ellipsis. Its `Path` is the sort key when `SortMemberPath` is empty. Takes precedence over a `TableViewTemplateColumn`'s `CellTemplate`, which is ignored while this is set (WPF `GridViewColumn` precedence: `DisplayMemberBinding`, then `CellTemplate`). Display-only: V1 has no cell editing. A CLR property, not a DP, so XAML hands the `Binding` object over rather than evaluating it against the column; changing it regenerates realized cells. |
| `CellToolTipBinding` | `Binding` | `null` | Opt-in per-cell tooltip. The binding is evaluated against each row's data item; its value becomes the cell's tooltip content — a string, or anything a `ToolTip` can host. `null` or an empty string means no tooltip for that cell. Use an `IValueConverter` for computed content. A CLR property, not a DP, so XAML hands the `Binding` object over rather than evaluating it against the column. |
| `CellContextFlyout` | `Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase` | `null` | Shared context flyout for this column's data cells. Opens at the requested cell; menu items inherit that cell's row data item unless the app supplies a different binding source. Does not apply to column or group headers. |

More sorting detail, including `GetSortMemberPathCore`, is in the [API review](../../design-notes/TabularControls/TableView-API-Review.md#23-sorting).

Methods:

| Method | Description |
|---|---|
| `GenerateElementCore(Object dataItem)` | Overridable method used by derived column types to create cell content. The base implementation returns the `DisplayMemberBinding` text cell, or `null` (an empty cell) when it is not set. |
| `NotifyCellContentChanged()` | Protected. Regenerates this column's realized cells and resets its `Auto` width; call it when a derived column's own property changes what it generates. Queued if called during layout. |

### Resizing columns

A column has a gripper while `TableView.CanUserResizeColumns` and the column's `CanUserResize` are both `true`, its `MinWidth` is less than its `MaxWidth`, and, for a `*` column, a resizable `*` column follows it to trade width with. A gesture that changes the width writes a `Pixel` `Width` and announces the new width, so an `Auto` or `*` column becomes `Pixel`, as in File Explorer; a double-click fit always does, even when the size is unchanged. The width is clamped to `MinWidth`/`MaxWidth` and to the resize bounds: when the table has `*` columns, a column can grow only by taking width from a resizable `*` column after it (up to the viewport less the other columns), and with no such column a `Pixel` or `Auto` column can only shrink.

| Gesture | Behaviour |
|---|---|
| Drag the gripper on a header's trailing edge | Resizes the column; <kbd>Esc</kbd>, a lost capture or unloading the table cancels the drag and restores the previous `Width`. |
| Double-click the gripper (mouse only) | Fits the column to its header and the widest **realized** row cell, measured unconstrained, within the same bounds as a drag, so a column that can only shrink is not widened to fit. Rows that are not realized are not measured. |
| <kbd>Alt</kbd> + <kbd>Left</kbd> / <kbd>Right</kbd> on a focused header | Narrows or widens the column by 8 DIPs (32 with <kbd>Shift</kbd>). |
| UI Automation `Transform.Resize` | Same path as a drag; see [Automation peers](#automation-peers-tableviewrowcellcolumnheader). |

#### Locked Star columns

While `CanUserResizeColumns` is true, a Star column with `CanUserResize="False"` keeps its **authored share**: the width its Star factor gives it in the authored layout (every column at its authored `Width`), so resizing other columns does not move it. It stops keeping the authored share when that no longer fits beside the fixed and resized columns plus the other Star columns' `MinWidth`, for example after a `MinWidth` / `MaxWidth` change or a narrower viewport. It is then divided together with the other Star columns by factor, so a `MaxWidth` clamp on one of them hands the excess to the rest. Because that share can be smaller than the authored share, crossing between the two states can move it by the difference.

Resizing a later column (by pointer, keyboard or UI Automation) never moves it: like an unlocked Star column before the resized one, it gets a **resize hold** at the width it rendered, during the resize and after it, so the resize gives width to or takes it from the columns after the resized one only, leaving empty space when they cannot grow. A resize hold overrides the authored share. It ends when something other than a user resize changes the column layout (a column's `MinWidth`, `MaxWidth`, authored `Width`, `Visibility` or `CanUserResize`, the `Columns` collection, `CanUserResizeColumns`, `ItemsSource` or the viewport width), or when an `Auto` column grows so that the held layout no longer fits. A canceled resize puts back the resize holds that were in place before it. Trimming a locked column to fit never makes it narrower than its `MinWidth`; when the minimums do not fit, the columns overflow the viewport.

### Cell tooltips

Text cells styled like `TableCellTextStyle` render with `CharacterEllipsis` and no wrapping, so a value wider than its column is
unreadable. `CellToolTipBinding` surfaces the full value:

```xml
<tabular:TableViewTemplateColumn Header="Notes"
                                 CellToolTipBinding="{Binding Notes}"
                                 SortMemberPath="Notes">
    <tabular:TableViewTemplateColumn.CellTemplate>
        <DataTemplate>
            <TextBlock Text="{Binding Notes}" Style="{StaticResource TableCellTextStyle}" />
        </DataTemplate>
    </tabular:TableViewTemplateColumn.CellTemplate>
</tabular:TableViewTemplateColumn>
```

Because the tooltip is an ordinary binding it tracks the row's `DataContext`: a recycled row
re-resolves its tooltips through the same inheritance that refreshes its cell text, and a source
`PropertyChanged` updates a live tooltip in place. There is no invalidation API, and none is needed.

The control owns the `ToolTip` and its placement (`PlacementMode.Mouse`), so the bound value is the
tooltip's *content*, not a `ToolTip`. A `UIElement` is parented by that cell's `ToolTip`, so a
converter must return a fresh element per evaluation. A tooltip the app sets inside the column's own
cell template is never touched; the control's tooltip covers the rest of the cell.

### Column header tooltips

`HeaderToolTip` is the header-side counterpart, and covers the whole header cell — content, padding
and sort affordance — except the resize gripper: while the pointer is on the gripper the tooltip is
suppressed, so the resize cursor is not reset under a resting pointer. It returns when the pointer
leaves the gripper or the gripper is withdrawn.

```xml
<tabular:TableViewTemplateColumn Header="Notes"
                                 HeaderToolTip="Free-form notes captured at intake"
                                 SortMemberPath="Notes">
    <tabular:TableViewTemplateColumn.CellTemplate>
        <DataTemplate>
            <TextBlock Text="{Binding Notes}" Style="{StaticResource TableCellTextStyle}" />
        </DataTemplate>
    </tabular:TableViewTemplateColumn.CellTemplate>
</tabular:TableViewTemplateColumn>
```

Header cells are rebuilt rather than recycled, so the value is read from the column when the header
is built and re-applied in place when it changes; there is no binding and no invalidation API.
Placement and ownership match the cell path, including leaving an app-set tooltip alone.

String content is reported as the header's UIA help text by
`TableViewColumnHeaderAutomationPeer`, joined with the column's sort state when it has one — the
header peer publishes the text itself rather than through
`AutomationProperties.HelpText`, and it reads the value from the column rather than the
tooltip visual. The provider is available once the header is realized. As with cells, non-string content is
mouse-only: pair it with `Header` text that carries the same information when it matters.

### Context menus

`CellContextFlyout` and `HeaderContextFlyout` are dependency properties on `TableViewColumn`.
Both default to `null` and accept a `FlyoutBase`, including `MenuFlyout`. They are read
for each request, so replacing or clearing one applies to the next request without
rebuilding the cells.

```xaml
<tabular:TableViewTemplateColumn Header="Name" SortMemberPath="Name">
    <tabular:TableViewTemplateColumn.CellTemplate>
        <DataTemplate>
            <TextBlock Text="{Binding Name}" Style="{StaticResource TableCellTextStyle}" />
        </DataTemplate>
    </tabular:TableViewTemplateColumn.CellTemplate>
    <tabular:TableViewTemplateColumn.CellContextFlyout>
        <MenuFlyout>
            <MenuFlyoutItem Text="{Binding Name}" />
        </MenuFlyout>
    </tabular:TableViewTemplateColumn.CellContextFlyout>
    <tabular:TableViewTemplateColumn.HeaderContextFlyout>
        <MenuFlyout>
            <MenuFlyoutItem Text="Column actions" />
        </MenuFlyout>
    </tabular:TableViewTemplateColumn.HeaderContextFlyout>
</tabular:TableViewTemplateColumn>
```

#### Resolution order

The first applicable flyout wins; cell and header properties do not replace a flyout
that application content already handles natively.

| Request target | Resolution order |
|---|---|
| Data cell | Application `ContextFlyout` inside the cell content; column `CellContextFlyout`; application `ContextFlyout` on the ancestor path above the cell through the row (including row-template elements and the row's own `ContextFlyout`); inherited table `ContextFlyout`. |
| Row outside a cell | Row/template menu reached by native routing or discovered on the ancestor path from the cells host through the template root and row; inherited table `ContextFlyout`. Column menus do not apply, and lookup never descends into arbitrary cells. |
| Column header | Application `ContextFlyout` inside the header content; column `HeaderContextFlyout`; inherited table `ContextFlyout`. Cell and row menus do not apply. |
| Group header | Its own application `ContextFlyout`, including one supplied by a group-header style; inherited table `ContextFlyout`. Column and data-row menus do not apply. |
| Empty table area | Inherited table `ContextFlyout`. |

When TableView searches cell/header content for a flyout, it searches visible elements
depth-first in visual order, choosing an outer element before its descendants. A nested
items host's own flyout is eligible, but its items are not searched; a nested TableView
is excluded entirely. Text controls' built-in text menus are not candidates for this
search. For a text control, only a `ContextFlyout` set locally on that element counts;
a style-set text-control flyout is not discovered by this search. These search rules
do not prevent a nested control from handling its own native context request.

With no candidate, the request remains unhandled and can bubble to the table or another
ancestor's `ContextFlyout`. `TableView.ContextFlyout` is inherited from `UIElement`;
there is no separate TableView-specific row-menu property. Use `ContextFlyout` on
`TableViewRow`, for example through a row style.

#### Data context and targeting

A column's `CellContextFlyout` instance is shared across its cells. Its menu items
inherit the requested cell's row data item through the flyout target, not the column
object or `SelectedItem`. Reopening that shared menu on another row supplies that row's
item. Row and row-template menus likewise inherit the target element's data context.
Application-set binding sources or local data contexts retain their normal XAML precedence.

`HeaderContextFlyout` targets the column-header cell, not the `TableViewColumn` object,
and does not automatically receive a row item or column object as its data context.
A flyout owned by header-template content instead uses that content element as its
target. A group-header menu inherits the header's `TableViewGroupInfo`; if the header is
recycled for another group while that menu remains open, its retained projection keeps
the original group's identity. The table-level fallback uses the table's data context
rather than the requested row's item.

Right-click and press-and-hold use the pointer location. Shift+F10, the Menu key, and
UIA `ShowContextMenu` resolve against the focused element or the automation peer's
target, without a pointer position. Column cell/header flyouts open at their cell/header
wrapper. Content-owned flyouts keep their owning content element as `Target`. When
TableView resolves a row-owned or table-fallback flyout for a cell request without a
pointer position, it keeps the owner as `Target` but positions the menu using the
requested cell's bounds, including a column-header cell. Pointer table fallback retains
native bubbling and pointer placement.
A row peer request targets the row rather than selecting an arbitrary cell.

For requests handled by TableView, the requested body cell becomes current and receives
cell-level focus; a row-only target receives row focus, and a header target receives
header focus. This focus move is intentional even when no flyout resolves: the request
is still interaction with that target. Normal focus-driven bring-into-view behavior
applies, including for partially visible cells; absence of a menu does not preserve
the previous focus or guarantee an unchanged scroll position. Selection is unchanged.
For an unhandled right-button press reaching a data row, TableView establishes the
target cell's focus and current-cell state before a native template-owned flyout can
open. The press remains unhandled and does not arm row selection, so native menu
ownership, targeting, and dismissal remain unchanged. This early focus path does not
run for a press originating inside a nested TableView, and it does not
override a child control that already handled the press. Focus already inside the target
cell is preserved.

This early path is specific to mouse/pen right-button presses. A template-owned flyout
handled natively for other input can still bypass TableView's context-request focus
move; this change does not redefine touch/hold, keyboard, or UIA behavior.

An unavailable pooled group-header automation peer rejects `ShowContextMenu` with
`ElementNotAvailable`, just as it rejects a row-coordinate query. It cannot open either
its retained menu or the table fallback.

## TableViewTemplateColumn class

A column that uses a consumer-provided `DataTemplate` for cell content.

`TableViewTemplateColumn` derives from `TableViewColumn`.

| Property | Type | Default | Description |
|---|---|---|---|
| `CellTemplate` | `DataTemplate` | `null` | Template used to display each cell. Ignored while `DisplayMemberBinding` is set, which takes precedence as in WPF's `GridViewColumn`. If both are null, the generated cell is empty. |

Controls in a `CellTemplate` are always live. Bind them to the item (`Mode=TwoWay` for interactive controls); TableView has no edit mode to buffer, commit or cancel their writes. See [Rename-in-place guidance](#rename-in-place-guidance) for an edit-on-demand pattern.

Example:

```xaml
<tabular:TableViewTemplateColumn Header="Status">
    <tabular:TableViewTemplateColumn.CellTemplate>
        <DataTemplate x:DataType="local:Order">
            <TextBlock Text="{x:Bind Status}" />
        </DataTemplate>
    </tabular:TableViewTemplateColumn.CellTemplate>
</tabular:TableViewTemplateColumn>
```

## TableViewRow class

Represents a generated row in a TableView.

`TableViewRow` derives from `Microsoft.UI.Xaml.Controls.Control`.

Template parts:

| Part | Type | Description |
|---|---|---|
| `PART_RootBorder` | `Border` | Row root border. Its `Background` is driven by `CommonStates`. |
| `PART_CellsHost` | `TableViewCellsPanel` (typed `Panel`) | Host for generated cell elements. See [TableViewCellsPanel class](#tableviewcellspanel-class). |
| `PART_SelectionIndicator` | `Shape` (default: `Rectangle`) | **Required by the default state definitions.** Leading-edge selection strip; the `Selected*` states animate `Opacity` to `1`, and `SelectedDisabled` also animates `Fill`. Custom templates using other indicator types must adapt the corresponding state targets. |

Visual states:

| Group | States |
|---|---|
| `CommonStates` | `Normal`, `PointerOver`, `Pressed`, `Disabled`, `Selected`, `SelectedPointerOver`, `SelectedPressed`, `SelectedDisabled` |

Selected states share the one group rather than living in a second, overlapping one: they animate the same `PART_RootBorder.Background`, so two groups would make the result depend on `GoToState` call order. Encoding both dimensions in the state name is the pattern `TreeViewItem` and `ItemContainer` use.

Properties:

| Property | Type | Default | Description |
|---|---|---|---|
| `IsSelected` | `Boolean` | `false` | Whether the row is the selected row. Read-only: selection is owned by the `TableView`, so a row cannot select itself — apps go through `Select(Int32)` / `Deselect(Int32)` / `DeselectAll()`. Rows are recycled, so this is always re-derived. |

## TableViewCellsPanel class

The layout panel that lines cells up under their columns. The default templates use it as `TableView`'s `PART_HeaderHost` and as each row's `PART_CellsHost`; it is public so that a re-templated `TableView` or `TableViewRow` can declare it in XAML.

`TableViewCellsPanel` derives from `Microsoft.UI.Xaml.Controls.Panel`.

| Member | Kind | Description |
|---|---|---|
| `TableViewCellsPanel()` | constructor | Creates the panel. Needed for XAML activation in a control template. |

It adds no properties, methods or events of its own; everything else is inherited from `Panel`.

Behavior:

- The **owning control populates `Children`**: one header cell (in `PART_HeaderHost`) or one cell wrapper (in `PART_CellsHost`) per column, in `Columns` order, each tagged with its `TableViewColumn`. Apps should not add, remove or reorder children; the control rebuilds them when the columns change.
- **Measure:** a cell in an `Auto` column is measured unconstrained, and its width is reported to the `TableView`, which takes the widest header or realized cell when it resolves the column's `ActualWidth`. A cell in a `Pixel` or `*` column is measured at the column's resolved width. A cell in a collapsed column is measured at zero width.
- **Arrange:** cells are placed left to right at each column's resolved `ActualWidth`, so the header band and every row stay aligned without a per-cell `Width` binding. Content wider than its column is clipped or ellipsized.
- **Frozen columns:** the leading frozen prefix is pinned against the body's horizontal offset through `UIElement.Translation`, `Canvas.ZIndex` and a clip on the cells.

Usage in a re-template:

```xaml
<!-- Inside a TableViewRow ControlTemplate. HorizontalAlignment="Left" matters: the panel reports
     the summed column width, and a Stretch alignment would centre it in a wider presenter. -->
<tabular:TableViewCellsPanel x:Name="PART_CellsHost" HorizontalAlignment="Left" />
```

`PART_HeaderHost` and `PART_CellsHost` are typed `Panel`, and any `Panel` is accepted, but only a `TableViewCellsPanel` reports measured widths and arranges at the column widths. With another panel, `Auto` columns ignore that host's content and its cells are not column-aligned.

Outside a `TableView` the panel has no columns to read. Untagged children are measured unconstrained and arranged at their desired width, side by side; this is a defensive fallback, not a supported layout.

## Enums

### TableViewFrozenEdge

| Value | Numeric value | Description |
|---|---:|---|
| `None` | 0 | Column is not frozen. |
| `Leading` | 1 | Column is pinned to the leading edge. |

Out-of-range values behave as `None`. Trailing-edge pinning, when implemented, will append a value.

### TableViewHeadersVisibility

Flags enum mirroring the `None`/`Column` slots of WPF `DataGridHeadersVisibility`. Row headers are out of scope, so `Row`/`All` (WPF values 2/3) are intentionally not defined.

| Value | Numeric value | Description |
|---|---:|---|
| `None` | 0 | No headers. |
| `Column` | 1 | Column-header strip. |

### TableViewGridLinesVisibility

Mirrors WPF `DataGridGridLinesVisibility` values exactly. Used by `GridLinesVisibility`.

| Value | Numeric value | Description |
|---|---:|---|
| `All` | 0 | Show horizontal and vertical gridlines. |
| `Horizontal` | 1 | Show horizontal gridlines. |
| `None` | 2 | Show no gridlines. |
| `Vertical` | 3 | Show vertical gridlines. |

### TableViewDensity

The zero value is the default, `Standard`. Persist and compare by name; the numeric values changed in this preview.

| Value | Numeric value | Description |
|---|---:|---|
| `Standard` | 0 | Standard spacing. **The default.** |
| `Compact` | 1 | Compact spacing. |
| `Comfortable` | 2 | Comfortable spacing. |

### TableViewSelectionMode

Only `None` and `Single` exist in this release: `Multiple`/`Extended` need an anchor, a range model and Ctrl/Shift gesture handling, none of which single selection implies. New members are appended, so these numeric values stay stable when they arrive.

Why a TableView-specific enum rather than reusing `ItemsViewSelectionMode` or `ListViewSelectionMode`: both already define `Multiple` and `Extended`, which this release cannot honor. Reusing either would make `SelectionMode = Multiple` compile, bind and round-trip through markup, and then fail or silently behave as `Single` at run time; an enum that only names the supported values makes the limit visible in IntelliSense and the type system instead. The names and numeric values (`None = 0`, `Single = 1`) match the first two `ItemsViewSelectionMode` members, so code and persisted values transfer. The cost is a third selection-mode enum in the namespace; if API review prefers reuse, the alternative is `ItemsViewSelectionMode` with `Multiple`/`Extended` rejected by `E_INVALIDARG` until they are implemented.

| Value | Numeric value | Description |
|---|---:|---|
| `None` | 0 | Selection is off; a display-only table. |
| `Single` | 1 | At most one row is selected. **The default.** |

### Cell tooltip accessibility

The control owns the `ToolTip`; the bound value is its content, not a `ToolTip` to attach. A `UIElement` is parented by that cell's `ToolTip`, so a converter returns a fresh element per evaluation.

- String tooltip text is published as the cell's `AutomationProperties.HelpText`, and retracted on recycle.
- `TableViewCellAutomationPeer` suppresses it at UIA query time when it equals the cell's own UIA text, so Narrator does not read it twice. Suppression is gated on the control's ownership record, so text the app set is never dropped, and it is resolved at query time because the cell's own binding may not have produced a value when the tooltip is applied.
- The popup is **pointer-only**. The UIA pairing serves keyboard and screen-reader users, which is why it is not optional.
- Placement is control-owned and fixed (`PlacementMode.Mouse`), matching `TabViewItem`. An app needing different placement uses a tooltip inside its own cell content template.
- Non-string content is **mouse-only** and has no accessible representation: no `HelpText` is published (it cannot be stringified), and the cell wrapper is internal so an app cannot set `HelpText` on it either. Keyboard and screen-reader users get nothing. `TabViewItem` and `NavigationViewItem` refuse non-string tooltip content outright for this reason; `TableView` allows it, so **use a converter that returns text whenever the value must be accessible**. Reaching parity needs a public cell element, which is post-v1.

## Selection event args

| Type | Members |
|---|---|
| `Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs` | `AddedItems`, `RemovedItems` (`IVector<Object>`) |

TableView reuses the **platform** `SelectionChangedEventArgs` rather than defining its own, so a handler can be shared with `ListView`/`ListBox`. Both vectors are snapshots built when the event is raised.

## Automation peers (TableView/Row/Cell/ColumnHeader)

TableView provides UI Automation peers for grid/table accessibility and row selection.

| Peer | Base type | Provider interfaces |
|---|---|---|
| `TableViewAutomationPeer` | `FrameworkElementAutomationPeer` | `ISelectionProvider`, `IGridProvider`, `ITableProvider`, `IItemContainerProvider` |
| `TableViewRowAutomationPeer` | `FrameworkElementAutomationPeer` | `ISelectionItemProvider`, `IExpandCollapseProvider` (tree rows) |
| `TableViewColumnHeaderAutomationPeer` (constructor takes the `TableViewColumnHeader` owner) | `FrameworkElementAutomationPeer` | `IInvokeProvider` (sort), `ITransformProvider` (resize; implementation-only, not in the IDL) |
| `TableViewGroupHeaderAutomationPeer` | `FrameworkElementAutomationPeer` | `IExpandCollapseProvider`, `IGridItemProvider` |
| `TableViewCellAutomationPeer` (constructor takes the cell `FrameworkElement` owner) | `FrameworkElementAutomationPeer` | `IGridItemProvider`, `ITableItemProvider`, `IValueProvider` |

These peers expose the table structure to assistive technologies. `TableViewCellAutomationPeer` implements `IValueProvider` read-only (`IsReadOnly` is always `true` and `SetValue` fails), and does not offer the Value pattern from `GetPattern` in this release; a cell's text is its `Name`. A selected row that is not realized (scrolled out of view) is named from the item's values at the visible columns' `SortMemberPath`s, joined like realized cell text; only when none yields text does it fall back to the item's own string form (for a class without `ToString`, its type name). Cell peers use the `Custom` automation control type with localized control type `cell`; UIA only permits that localized-control-type override on `Custom`, and the previous `DataItem`-inside-`DataItem` shape broke Narrator item counting.

Migration notes:

- Header `AutomationId` is no longer synthesized. `GetAutomationIdCore` returns empty unless the app sets `AutomationProperties.AutomationId`; the old pointer-derived value varied per process. Apps that need stable header IDs must set `AutomationProperties.AutomationId` on the header explicitly.
- Programmatic row focus now resolves to a cell. Apps with `GotFocus` / `LostFocus` handlers on `TableViewRow`, row focus-visual styling, or `FocusManager` assertions can observe a cell as the focused element instead of the row.
- Cell automation peers now report control type `Custom`, with localized control type `cell`.
- A cell's automation `Name` no longer repeats its column header while column headers are shown: the name is the cell value (for example "Sales"), the header comes from `ITableItemProvider.GetColumnHeaderItems`, and an empty cell is named "blank". Only while `HeadersVisibility` hides the column headers is the name "{header}, {value}" (for example "Department, Sales"), with `GetColumnHeaderItems` and `ITableProvider.GetColumnHeaders` then empty. A cell whose content is an interactive control (for example a `Button` or `ToggleSwitch`) is still named after its column header, since the control names itself. UI tests and automation scripts that find a cell by a name such as "Department, Sales" must match the value instead, or read the column through `GetColumnHeaderItems`.

Rows report `PositionInSet` / `SizeOfSet` and compose their name from their visible cells, so a virtualized row still announces "row *i* of *n*" and what it contains. When the source is grouped, both values are **relative to the containing group** and exclude the group-header bands, matching `ItemsControlAutomationPeer`. An app-set `AutomationProperties.PositionInSet` / `SizeOfSet` / `Level` always takes precedence over the computed value.

Row and group names honor explicit `AutomationProperties.Name` and `LabeledBy`. For template cells, naming reads the realized visible template rather than stringifying the data object. Layout-wrapper traversal is bounded; a named interactive control describes its own content without also concatenating its inner editors. Column-header set metadata comes from the actual header element.

`FindItemByProperty(Name)` returns only an exact public peer name. Data text can identify an unrealized candidate, but its name is checked again after preparation. Arbitrary template or application names cannot be inferred for never-realized items; clients can enumerate with a null property and read each returned peer's name.

A cell visual owns its peer through normal `OnCreateAutomationPeer` discovery; the row peer caches that same instance. Both tree enumeration and `IGridProvider.GetItem` connect it to its row before returning it. Child-parent queries and framework automation walks therefore cannot replace the visual's native peer association behind a retained provider. The implementation-only single-child Grid keeps the cell's border chrome and inherited data binding while allowing peer creation to be overridden (`Border` is sealed). Grid-only clients do not need a preliminary tree walk to establish the parent relationship.

Each realized column-header visual owns its `HeaderItem` peer. `ITableProvider.GetColumnHeaders`, `ITableItemProvider.GetColumnHeaderItems`, tree navigation, focus, and hit-testing use that same peer. Names and set metadata honor overrides on the header itself. The existing Grid layout and mouse handlers are unchanged; sortable headers expose Invoke and sort on Enter or Space on the actual keyboard target. Surviving header visuals retain identity, while a header rebuild creates new visuals and identities; a retained old header cannot invoke sorting on its replacement. No provider is fabricated before header realization. Cell names still include the semantic column name when the header strip is hidden. The public constructor takes the realized `TableViewColumnHeader` as its owner and derives the table and column from it.

Header `IsEnabled` reflects the owning table's effective state and disabled control ancestors between the header visual and the table (including template controls); Invoke rejects a disabled header without changing sort state. Cell header lookup resolves the realized visual independently of the table's peer type, so application-supplied table peers preserve canonical header identity. A cell with no reachable header (including a collapsed or removed column) returns an empty header array.

`TableViewCellAutomationPeer(FrameworkElement owner)` takes the realized cell element (typed `FrameworkElement` because the cell element type is internal, and therefore validated at run time) and derives the row, column and column index from it; an owner that is not a prepared TableView cell (with a row, column and column index) throws `E_INVALIDARG` (`ArgumentException` in .NET). Like every WinUI peer it is unsealed. The control creates the peers for its own cells; `FrameworkElementAutomationPeer.CreatePeerForElement` on a realized cell, or `IGridProvider.GetItem`, returns that peer.
A column header exposes the Transform pattern (required by UIA for a resizable `HeaderItem`) while `CanUserResizeColumns` and the column's `CanUserResize` are both true. The pattern's `CanResize` follows the same rule as the resize gripper's visibility, so it reports `false` for a pinned column (`MinWidth` equal to `MaxWidth`, or a Star column with no later resizable Star column to trade width with) and raises a property-changed event when that changes. `Resize(width, height)` takes `width` in physical pixels, the units of `BoundingRectangle`, and converts it to DIPs with the `XamlRoot.RasterizationScale`. It runs the same resize path as a pointer drag or Alt+Arrow, so it honors `MinWidth` / `MaxWidth` and the viewport bounds and announces the new width; `height` is ignored. `CanMove` and `CanRotate` are `false`, and `Move` / `Rotate` fail with `UIA_E_INVALIDOPERATION`.

`ScrollItem` is not implemented explicitly — `FrameworkElementAutomationPeer` already supplies a `ScrollItemAdapter` for every peer, so a client can bring a row or cell into view after reaching it through `IGridProvider.GetItem`.

`Selection`/`SelectionItem` are advertised only while `SelectionMode` allows selection — advertising them while it is `None` would tell an AT client the grid is selectable when every `Select()` would be refused. `CanSelectMultiple` is `false` and `IsSelectionRequired` is `false`. `GetSelection()` returns the selected row's own peer when that row is realized. Otherwise it returns a container-free data-item peer for the selected item, without realizing the row: a `DataItem` named from the item's values at the visible columns' `SortMemberPath`s, else from the item (a name the app gives the row, or the text of columns with no `SortMemberPath`, appears only once the row is realized), off screen with an empty bounding rectangle, offering `SelectionItem` and, until the row is realized, `VirtualizedItem`, whose `Realize()` creates the row and brings it into view. It has the same parent as realized rows. Once the row is realized, the data-item peer forwards its name, bounds, focus, enabled state, culture and patterns (including `ScrollItem`) to the row's peer (not its children: cells are reached through the row), but it remains a separate element with its own `RuntimeId`; `GetSelection()` then returns the row's peer, and returns the same data-item peer again if the row scrolls back out. While the same row stays selected, repeated calls return the same data-item peer: it follows the selected occurrence of an item that appears more than once, and a boxed value (which has no identity of its own) is tied to its row index and stops being available once that row is no longer selected. A data-item peer whose item has left the source fails with `UIA_E_ELEMENTNOTAVAILABLE` and no longer offers `SelectionItem`.

# API Details

```idl
// Generated from TabularShaping/SortDirection.idl, TableView/TableViewSource.idl, TableView/TableView.idl and
// dll-tabular/Microsoft.UI.Xaml.Controls.Tabular.idl (TabularControlsResources). Property-changed-callback
// attributes and the [MUX_INTERNAL] TableViewRowTemplateSelector are omitted. XamlControlsTabularXamlMetaDataProvider
// (IXamlMetadataProvider) is declared in the Microsoft.UI.Xaml.XamlTypeInfo namespace.
namespace Microsoft.UI.Xaml.Controls.Tabular
{
    [MUX_PREVIEW]
    [webhosthidden]
    enum SortDirection
    {
        None = 0,
        Ascending = 1,
        Descending = 2,
    };

    [MUX_PREVIEW]
    [webhosthidden]
    delegate Boolean TableViewPredicate(Object item);

    [MUX_PREVIEW]
    [webhosthidden]
    delegate Object TableViewKeySelector(Object item);

    [MUX_PREVIEW]
    [webhosthidden]
    delegate String TableViewGroupIdentitySelector(Object item);

    [MUX_PREVIEW]
    [webhosthidden]
    runtimeclass TableViewSource
    {
        Boolean IsLiveShaping { get; set; };
        Object Source { get; };

        TableViewSource Filter(TableViewPredicate predicate);
        [default_overload] [method_name("GroupBy")]
        TableViewSource GroupBy(TableViewKeySelector keySelector);
        [method_name("GroupByWithIdentity")]
        TableViewSource GroupBy(TableViewKeySelector keySelector, TableViewGroupIdentitySelector groupIdentitySelector);
        TableViewSource SortGroups(SortDirection direction);
        [method_name("SortByPath")]
        TableViewSource Sort(String sortMemberPath, SortDirection direction);
        [default_overload] [method_name("Sort")]
        TableViewSource Sort(TableViewKeySelector keySelector, SortDirection direction);
        [method_name("ThenSortByPath")]
        TableViewSource ThenSort(String sortMemberPath, SortDirection direction);
        [default_overload] [method_name("ThenSort")]
        TableViewSource ThenSort(TableViewKeySelector keySelector, SortDirection direction);
        TableViewSource ParentBy(TableViewKeySelector keySelector, TableViewKeySelector parentKeySelector);
        TableViewSource ClearFilter();
        TableViewSource ClearGroupBy();
        TableViewSource ClearSort();
        TableViewSource ClearParentBy();
        void Refresh();
        Windows.Foundation.IClosable DeferRefresh();

        static TableViewSource From(Object items);
    }

    [MUX_PREVIEW]
    [webhosthidden]
    enum TableViewFrozenEdge
    {
        None = 0,
        Leading = 1,
    };

    [MUX_PREVIEW]
    [webhosthidden]
    enum TableViewSelectionMode
    {
        None = 0,
        Single = 1,
    };

    [MUX_PREVIEW]
    [webhosthidden]
    [flags]
    enum TableViewHeadersVisibility
    {
        None = 0,
        Column = 1,
    };

    [MUX_PREVIEW]
    [webhosthidden]
    enum TableViewGridLinesVisibility
    {
        All = 0,
        Horizontal = 1,
        None = 2,
        Vertical = 3,
    };

    [MUX_PREVIEW]
    [webhosthidden]
    enum TableViewDensity
    {
        Standard = 0,
        Compact = 1,
        Comfortable = 2,
    };

    [MUX_PREVIEW]
    [webhosthidden]
    enum TableViewSortCycle
    {
        AscendingDescending = 0,
        AscendingDescendingNone = 1,
        DescendingAscending = 2,
        DescendingAscendingNone = 3,
    };

    [MUX_PREVIEW]
    [webhosthidden]
    interface ITableViewSortComparer
    {
        Int32 Compare(Object left, Object right);
    };

    [MUX_PREVIEW]
    [webhosthidden]
    [contentproperty("Header")]
    unsealed runtimeclass TableViewColumn : Microsoft.UI.Xaml.DependencyObject
    {
        TableViewColumn();

        Object Header { get; set; };

        Microsoft.UI.Xaml.DataTemplate HeaderTemplate { get; set; };

        Microsoft.UI.Xaml.Controls.DataTemplateSelector HeaderTemplateSelector { get; set; };

        Microsoft.UI.Xaml.Style HeaderStyle { get; set; };

        Object HeaderToolTip { get; set; };

        Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase HeaderContextFlyout { get; set; };

        [MUX_DEFAULT_VALUE("c_widthDefault")]
        Microsoft.UI.Xaml.GridLength Width { get; set; };

        [MUX_DEFAULT_VALUE("20.0")]
        Double MinWidth { get; set; };

        [MUX_DEFAULT_VALUE("std::numeric_limits<double>::infinity()")]
        Double MaxWidth { get; set; };

        [MUX_DEFAULT_VALUE("120.0")]
        Double ActualWidth { get; };

        [MUX_DEFAULT_VALUE("true")]
        Boolean CanUserResize { get; set; };

        [MUX_DEFAULT_VALUE("true")]
        Boolean CanUserSort { get; set; };

        [MUX_DEFAULT_VALUE("winrt::SortDirection::None")]
        SortDirection SortDirection { get; set; };

        String SortMemberPath { get; set; };

        [MUX_DEFAULT_VALUE("winrt::TableViewSortCycle::AscendingDescending")]
        TableViewSortCycle SortCycle { get; set; };

        ITableViewSortComparer CustomSortComparer { get; set; };

        [MUX_DEFAULT_VALUE("winrt::Visibility::Visible")]
        Microsoft.UI.Xaml.Visibility Visibility { get; set; };

        [MUX_DEFAULT_VALUE("winrt::TableViewFrozenEdge::None")]
        Microsoft.UI.Xaml.Controls.Tabular.TableViewFrozenEdge FrozenEdge { get; set; };

        Microsoft.UI.Xaml.Data.Binding DisplayMemberBinding;
        Microsoft.UI.Xaml.Data.Binding CellToolTipBinding;
        Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase CellContextFlyout { get; set; };

        overridable String GetSortMemberPathCore();
        overridable Microsoft.UI.Xaml.FrameworkElement GenerateElementCore(Object dataItem);
        protected void NotifyCellContentChanged();

        static Microsoft.UI.Xaml.DependencyProperty HeaderProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderTemplateProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderTemplateSelectorProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderStyleProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderToolTipProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderContextFlyoutProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty WidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty MinWidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty MaxWidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty ActualWidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CanUserResizeProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CanUserSortProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SortDirectionProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SortMemberPathProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SortCycleProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty VisibilityProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty FrozenEdgeProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CellContextFlyoutProperty { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    unsealed runtimeclass TableViewTemplateColumn : TableViewColumn
    {
        TableViewTemplateColumn();

        Microsoft.UI.Xaml.DataTemplate CellTemplate { get; set; };

        static Microsoft.UI.Xaml.DependencyProperty CellTemplateProperty { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    runtimeclass TableViewSortingEventArgs
    {
        Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn Column { get; };
        SortDirection Direction { get; };
        Boolean Cancel;
    }

    [MUX_PREVIEW]
    [webhosthidden]
    runtimeclass TableViewSortedEventArgs
    {
        Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn Column { get; };
        SortDirection Direction { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    runtimeclass TableViewGroupExpandingEventArgs
    {
        Object GroupKey { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    runtimeclass TableViewGroupCollapsedEventArgs
    {
        Object GroupKey { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    unsealed runtimeclass TableViewCellsPanel : Microsoft.UI.Xaml.Controls.Panel
    {
        TableViewCellsPanel();
    }

    [MUX_PREVIEW]
    [webhosthidden]
    runtimeclass TableViewColumnHeader : Microsoft.UI.Xaml.Controls.ContentControl
    {
        TableViewColumn Column { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    unsealed runtimeclass TableViewRow : Microsoft.UI.Xaml.Controls.Control
    {
        TableViewRow();

        [MUX_DEFAULT_VALUE("false")]
        Boolean IsSelected { get; };

        [MUX_DEFAULT_VALUE("0")]
        Int32 Level { get; };

        [MUX_DEFAULT_VALUE("false")]
        Boolean IsExpandable { get; };

        [MUX_DEFAULT_VALUE("false")]
        Boolean IsExpanded { get; };

        static Microsoft.UI.Xaml.DependencyProperty IsSelectedProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty LevelProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsExpandableProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsExpandedProperty { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    runtimeclass TableViewGroupInfo : Microsoft.UI.Xaml.Data.INotifyPropertyChanged
    {
        Object Key { get; };
        Int32 ItemCount { get; };
        Boolean IsExpandable { get; };
        Boolean IsExpanded { get; };
        String KeyText { get; };
        String ItemCountText { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    unsealed runtimeclass TableViewGroupHeader : Microsoft.UI.Xaml.Controls.ContentControl
    {
        TableViewGroupHeader();

        [MUX_DEFAULT_VALUE("false")]
        Boolean IsExpanded { get; };

        [MUX_DEFAULT_VALUE("false")]
        Boolean IsExpandable { get; };

        static Microsoft.UI.Xaml.DependencyProperty IsExpandedProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsExpandableProperty { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    [contentproperty("Columns")]
    unsealed runtimeclass TableView : Microsoft.UI.Xaml.Controls.Control
    {
        TableView();

        Object ItemsSource { get; set; };

        Windows.Foundation.Collections.IVector<TableViewColumn> Columns { get; };

        [MUX_DEFAULT_VALUE("winrt::TableViewHeadersVisibility::Column")]
        TableViewHeadersVisibility HeadersVisibility { get; set; };

        [MUX_DEFAULT_VALUE("winrt::TableViewGridLinesVisibility::All")]
        TableViewGridLinesVisibility GridLinesVisibility { get; set; };

        [MUX_DEFAULT_VALUE("true")]
        Boolean CanUserResizeColumns { get; set; };

        Microsoft.UI.Xaml.Media.Brush RowBackground { get; set; };

        Microsoft.UI.Xaml.Media.Brush AlternatingRowBackground { get; set; };

        Microsoft.UI.Xaml.DataTemplate EmptyTemplate { get; set; };

        Microsoft.UI.Xaml.DataTemplate GroupHeaderTemplate { get; set; };

        Microsoft.UI.Xaml.Style ColumnHeaderStyle { get; set; };

        [MUX_DEFAULT_VALUE("winrt::TableViewDensity::Standard")]
        TableViewDensity Density { get; set; };

        [MUX_DEFAULT_VALUE("winrt::TableViewSelectionMode::Single")]
        TableViewSelectionMode SelectionMode { get; set; };

        Object SelectedItem { get; };

        [MUX_DEFAULT_VALUE("-1")]
        Int32 SelectedIndex { get; };

        [MUX_DEFAULT_VALUE("true")]
        Boolean CanUserSortColumns { get; set; };

        void Select(Int32 index);
        void Deselect(Int32 index);
        Boolean IsSelected(Int32 index);
        void DeselectAll();
        Int32 IndexOf(Object item);
        void StartBringItemIntoView(Int32 index, Microsoft.UI.Xaml.BringIntoViewOptions options);
        void ExpandGroup(Object groupKey);
        void CollapseGroup(Object groupKey);
        void ExpandAllGroups();
        void CollapseAllGroups();
        void ExpandAllRows();
        void CollapseAllRows();
        Boolean SortByColumn(Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn column, SortDirection direction);
        Boolean ToggleSortDirection(Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn column);
        Boolean ClearSort();

        event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.Tabular.TableView, Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs> SelectionChanged;
        event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.Tabular.TableView, Microsoft.UI.Xaml.Controls.Tabular.TableViewSortingEventArgs> Sorting;
        event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.Tabular.TableView, Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs> Sorted;
        event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.Tabular.TableView, Microsoft.UI.Xaml.Controls.Tabular.TableViewGroupExpandingEventArgs> GroupExpanding;
        event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.Tabular.TableView, Microsoft.UI.Xaml.Controls.Tabular.TableViewGroupCollapsedEventArgs> GroupCollapsed;

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
        static Microsoft.UI.Xaml.DependencyProperty SelectionModeProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SelectedItemProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SelectedIndexProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CanUserSortColumnsProperty { get; };
    }

    [MUX_PREVIEW]
    [webhosthidden]
    unsealed runtimeclass TableViewAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.ISelectionProvider,
          Microsoft.UI.Xaml.Automation.Provider.IGridProvider,
          Microsoft.UI.Xaml.Automation.Provider.ITableProvider,
          Microsoft.UI.Xaml.Automation.Provider.IItemContainerProvider
    {
        TableViewAutomationPeer(Microsoft.UI.Xaml.Controls.Tabular.TableView owner);
    }

    [MUX_PREVIEW]
    [webhosthidden]
    unsealed runtimeclass TableViewCellAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.IGridItemProvider,
          Microsoft.UI.Xaml.Automation.Provider.ITableItemProvider,
          Microsoft.UI.Xaml.Automation.Provider.IValueProvider
    {
        TableViewCellAutomationPeer(Microsoft.UI.Xaml.FrameworkElement owner);
    }

    [MUX_PREVIEW]
    [webhosthidden]
    unsealed runtimeclass TableViewColumnHeaderAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider
    {
        TableViewColumnHeaderAutomationPeer(Microsoft.UI.Xaml.Controls.Tabular.TableViewColumnHeader owner);
    }

    [MUX_PREVIEW]
    [webhosthidden]
    unsealed runtimeclass TableViewRowAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.ISelectionItemProvider,
          Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider
    {
        TableViewRowAutomationPeer(Microsoft.UI.Xaml.Controls.Tabular.TableViewRow owner);
    }

    [MUX_PREVIEW]
    [webhosthidden]
    unsealed runtimeclass TableViewGroupHeaderAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
          Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider,
          Microsoft.UI.Xaml.Automation.Provider.IGridItemProvider
    {
        TableViewGroupHeaderAutomationPeer(Microsoft.UI.Xaml.Controls.Tabular.TableViewGroupHeader owner);
    }

    [webhosthidden]
    [default_interface]
    [MUX_PREVIEW]
    runtimeclass TabularControlsResources : Microsoft.UI.Xaml.ResourceDictionary
    {
        TabularControlsResources();
    }
}
```

# Appendix

## Deferred to V2 / future DataGrid

Cell editing is not part of TableView V1. TableView is read-only-first and is not a `DataGrid`
replacement; the named V1 scenarios (Task Manager, File Explorer details view, File properties)
need at most a one-field rename, which composes from template columns and app code (see
[Rename-in-place guidance](#rename-in-place-guidance)). A full edit-transaction model is the
`DataGrid` feature set and is deferred to TableView V2 or a future DataGrid.

Removed from V1, to be revisited there:

- `TableView.IsReadOnly` and `TableViewColumn.IsReadOnly` (and their dependency properties).
  Without an edit mode they have no meaning; V1 behaves as the previous default, `IsReadOnly = true`.
- `TableView.IsEditing` (read-only dependency property), `CommitEdit()` and `CancelEdit()`.
- Events `BeginningEdit`, `CellEditEnding` and `CellEditEnded`, with
  `TableViewBeginningEditEventArgs`, `TableViewCellEditEndingEventArgs`,
  `TableViewCellEditEndedEventArgs` and the `TableViewEditAction` enum.
- `TableViewColumn.CellEditingTemplate` and the overridable `TableViewColumn.CommitCellEditCore`.
- The built-in `TextBox` editor of `TableViewTextColumn`; the column itself is also deferred (see
  [TableViewTextColumn (deferred)](#tableviewtextcolumn-deferred)).
- Edit gestures (double-click/double-tap, `F2`, `Enter`, `Esc`, focus-loss commit), menu
  suppression while editing, the live-shaping hold during an edit, and the settable
  `IValueProvider` path on cells (the provider stays, read-only).

Design input carried forward for V2: a public `BeginEdit` (so a context menu or "New folder" can
start an edit), an async/deferrable commit (shell rename can prompt), per-row editors, row-scoped
transactions (WinUI has no `BindingGroup`; a cell commit writes to the item immediately), editing
UIA announcements, and the UX issues in GH #12354.

Re-adding editing is additive: new members on a new contract version, with `IsReadOnly`
defaulting to `true`, so V1 apps see no behavior change. Controls inside a `CellTemplate` are
already outside any TableView edit model.

## Rename-in-place guidance

Guidance only; no TableView API is involved. A single-field rename (File Explorer's Name column)
is a template swap owned by the app: the item carries `IsRenaming` and `PendingName`, the cell
template shows a `TextBlock` normally and an `x:Load`-ed `TextBox` while renaming, and the app
starts the rename from `F2`, a context-menu command or "New folder".

```xaml
<DataTemplate x:Key="FileNameTemplate" x:DataType="local:FileSystemEntry">
    <Grid ColumnSpacing="8">
        <Grid.ColumnDefinitions><ColumnDefinition Width="Auto"/><ColumnDefinition Width="*"/></Grid.ColumnDefinitions>
        <FontIcon Glyph="{x:Bind IconGlyph}" AutomationProperties.AccessibilityView="Raw"/>
        <TextBlock Grid.Column="1" Text="{x:Bind Name, Mode=OneWay}" TextTrimming="CharacterEllipsis"
                   VerticalAlignment="Center" Visibility="{x:Bind NameVisibility, Mode=OneWay}"/>
        <!-- Realized only while renaming; the state lives on the item, so it survives row recycling. -->
        <TextBox Grid.Column="1" x:Load="{x:Bind IsRenaming, Mode=OneWay}"
                 Text="{x:Bind PendingName, Mode=TwoWay, UpdateSourceTrigger=PropertyChanged}"
                 AutomationProperties.Name="Rename"
                 Loaded="OnRenameBoxLoaded" KeyDown="OnRenameBoxKeyDown" LostFocus="OnRenameBoxLostFocus"/>
    </Grid>
</DataTemplate>

<tabular:TableViewTemplateColumn Header="Name" Width="*"
                                 CellTemplate="{StaticResource FileNameTemplate}"
                                 SortMemberPath="NameSortKey"/>
```

```csharp
// TableView does not handle F2, so a handledEventsToo handler on the table sees it.
FileTable.AddHandler(UIElement.KeyDownEvent, new KeyEventHandler(OnTableKeyDown), handledEventsToo: true);

private IDisposable? _renameHold;

private void OnTableKeyDown(object sender, KeyRoutedEventArgs e)
{
    if (e.Key == VirtualKey.F2 && FileTable.SelectedItem is FileSystemEntry entry && !entry.IsRenaming)
    {
        BeginRename(entry);   // also called from a context-menu command and from "New folder"
        e.Handled = true;
    }
}

private void BeginRename(FileSystemEntry entry)
{
    _renameHold = Source.DeferRefresh();   // optional: keep the row in place while the user types
    entry.PendingName = entry.Name;
    entry.IsRenaming = true;               // x:Load realizes the TextBox
}

private void OnRenameBoxLoaded(object sender, RoutedEventArgs e)
{
    var box = (TextBox)sender;
    box.Focus(FocusState.Programmatic);
    box.Select(0, Path.GetFileNameWithoutExtension(box.Text).Length);
}

private async void OnRenameBoxKeyDown(object sender, KeyRoutedEventArgs e)
{
    var entry = (FileSystemEntry)((FrameworkElement)sender).DataContext;
    if (e.Key == VirtualKey.Escape) { e.Handled = true; EndRename(entry); }
    else if (e.Key == VirtualKey.Enter) { e.Handled = true; await CommitRenameAsync(entry); }
}

private async void OnRenameBoxLostFocus(object sender, RoutedEventArgs e) =>
    await CommitRenameAsync((FileSystemEntry)((FrameworkElement)sender).DataContext);

private async Task CommitRenameAsync(FileSystemEntry entry)
{
    if (!entry.IsRenaming) return;
    if (!IsValidName(entry.PendingName)) { ShowInvalidNameTip(); return; }   // the editor stays open
    try { await RenameAsync(entry, entry.PendingName); entry.Name = entry.PendingName; }
    catch (Exception ex) { ShowError(ex); return; }
    EndRename(entry);
}

private void EndRename(FileSystemEntry entry)
{
    entry.IsRenaming = false;
    _renameHold?.Dispose();                // the deferred re-sort runs once, here
    _renameHold = null;
    int index = FileTable.IndexOf(entry);
    if (index >= 0) { FileTable.Select(index); FileTable.StartBringItemIntoView(index, null); }
}
```

Notes:

- Mark `Esc` and `Enter` handled in the `TextBox` so TableView's own key handling leaves them alone.
- A row recycled during a rename re-realizes the `TextBox` from `IsRenaming`, so focus is lost and
  the `LostFocus` commit runs, matching Explorer, which commits on scroll-away.
- The commit can be asynchronous and can prompt, because the app, not the control, owns it.

## Display-only base

The display-only base delivered previously:

- Preview API surface for display-only TableView.
- Developer-defined columns through `TableView.Columns`.
- Text cells through `TableViewColumn.DisplayMemberBinding`, or a `TableViewTemplateColumn` with a bound `TextBlock`.
- Custom templated cells through `TableViewTemplateColumn`.
- Column-header strip rendering.
- Row virtualization through `ItemsRepeater`.
- Gridline visibility.
- Density settings.
- Row and alternating row background brushes.
- Empty-state template.
- Leading-prefix frozen columns.
- Read-only keyboard row-focus navigation.
- Read-only UI Automation grid/table peers.

## Deferred post-single-selection (still v1)

These ship additively in a later change — they are part of v1, not non-goals:

- Row selection: multiple/extended (Ctrl/Shift range selection). **Single selection ships here.**
- Column resize and reorder.

## TableViewTextColumn (deferred)

Cut from V1 and deferred to V2; it can return additively:

- `TableViewTextColumn` (`Binding`, with a built-in `TextBox` editor). For display-only text, V1 sets
  `TableViewColumn.DisplayMemberBinding` on a base column instead (see
  [Basic usage](#basic-usage-text-cells-with-displaymemberbinding)); its `Path` is the sort key unless
  `SortMemberPath` is set:

  ```xaml
  <!-- Was: <tabular:TableViewTextColumn Header="Type" Binding="{Binding TypeDisplay}" SortMemberPath="TypeSortKey" /> -->
  <tabular:TableViewColumn Header="Type" DisplayMemberBinding="{Binding TypeDisplay}" SortMemberPath="TypeSortKey" />
  ```

  A text cell that needs its own look is a `TableViewTemplateColumn` whose `CellTemplate` is a bound
  `TextBlock`; as in WPF's `GridViewColumn`, `DisplayMemberBinding` takes precedence over `CellTemplate`, so
  leave it unset on such a column.

  V1 has no cell editing either (see [Deferred to V2 / future DataGrid](#deferred-to-v2--future-datagrid)), so a
  `DisplayMemberBinding` cell is read-only. An editable text column is the V2 candidate.

## Non-goals (out of scope for v1)

- Replacing DataGrid.
- Marquee selection.
- Multi-column sort from the column headers (TableViewSource.ThenSort provides multi-level sort from code).
- Column virtualization.
- Row headers.
- Hierarchy deeper than two levels; row drag-drop.
- Spreadsheet-like interaction.
- Spreadsheet-scale column counts (100+) — `TableView` realizes a cell per visible column per row and targets ~5–50 columns.

Theme-XBF emission is suppressed in this release, and is tracked separately.
