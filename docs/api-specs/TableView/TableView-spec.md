# Background

TableView is a preview tabular control for WinUI 3. It presents an `ItemsSource` as rows and a developer-defined `Columns` collection as cells, with optional column headers, gridlines, density, alternating row backgrounds, leading-prefix frozen columns, and an empty-state template. It is display-only by default; **opt-in cell editing** is enabled by clearing `IsReadOnly`.

Rows are virtualized: TableView renders rows with `ItemsRepeater`, so large row counts are virtualized (only on-screen rows plus a small cache are realized).

This API is intentionally small: **opt-in cell editing** and **single row selection** on top of the existing read-only data presentation, with cell value automation added to the grid/table peers. Additional interactive features — multiple/extended selection, single-column sort, filtering, grouping, two-level hierarchy, and column resize/reorder — are planned separately. Marquee selection, multi-column sort, column virtualization, and row headers remain out of scope for v1.

TableView types are in the preview namespace:

```xaml
xmlns:tabular="using:Microsoft.UI.Xaml.Controls.Tabular"
```

# Conceptual pages (How To)

## How to use TableView

### Basic usage (with a TableViewTextColumn)

Set `TableView.ItemsSource` and add `TableViewTextColumn` instances to `Columns`. `TableViewTextColumn.Binding` is a CLR property, not a dependency property, and accepts a normal XAML `Binding`.

```xaml
<Page
    x:Class="Contoso.App.Views.PeoplePage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:tabular="using:Microsoft.UI.Xaml.Controls.Tabular">

    <Grid Padding="24">
        <tabular:TableView ItemsSource="{x:Bind People}">
            <tabular:TableView.Columns>
                <tabular:TableViewTextColumn
                    Header="Name"
                    Binding="{Binding Name}" />

                <tabular:TableViewTextColumn
                    Header="Role"
                    Binding="{Binding Role}" />

                <tabular:TableViewTextColumn
                    Header="Location"
                    Binding="{Binding Location}" />
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

### Template columns (custom cell content)

Use `TableViewTemplateColumn.CellTemplate` for custom cell layout or formatting.

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
                <tabular:TableViewTextColumn
                    Header="Order"
                    Binding="{Binding OrderNumber}" />

                <tabular:TableViewTextColumn
                    Header="Customer"
                    Binding="{Binding Customer}" />

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

`GridLinesVisibility` controls gridlines. `Density` controls row and cell spacing. Header height matches row height; built-in header and text-cell content is vertically centered.

```xaml
<tabular:TableView
    ItemsSource="{x:Bind People}"
    HeadersVisibility="Column"
    GridLinesVisibility="All"
    Density="Compact"
    RowBackground="{ThemeResource CardBackgroundFillColorDefaultBrush}"
    AlternatingRowBackground="{ThemeResource SubtleFillColorSecondaryBrush}">
    <tabular:TableView.Columns>
        <tabular:TableViewTextColumn Header="Name" Binding="{Binding Name}" />
        <tabular:TableViewTextColumn Header="Role" Binding="{Binding Role}" />
        <tabular:TableViewTextColumn Header="Location" Binding="{Binding Location}" />
    </tabular:TableView.Columns>
</tabular:TableView>
```

```csharp
using Microsoft.UI.Xaml.Controls.Tabular;

PeopleTable.HeadersVisibility = TableViewHeadersVisibility.Column;
PeopleTable.GridLinesVisibility = TableViewGridLinesVisibility.All;
PeopleTable.Density = TableViewDensity.Compact;
```

### Frozen (pinned) leading columns

Set `FrozenEdge="Leading"` on a contiguous prefix starting at column 0 to pin those columns to the leading edge. A later `Leading` column is ignored. `Trailing` is reserved.

```xaml
<tabular:TableView ItemsSource="{x:Bind Orders}">
    <tabular:TableView.Columns>
        <tabular:TableViewTextColumn
            Header="Order"
            Binding="{Binding OrderNumber}"
            FrozenEdge="Leading" />

        <tabular:TableViewTextColumn
            Header="Customer"
            Binding="{Binding Customer}" />

        <tabular:TableViewTextColumn
            Header="Status"
            Binding="{Binding Status}" />

        <tabular:TableViewTextColumn
            Header="Location"
            Binding="{Binding Location}" />
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
        <tabular:TableViewTextColumn Header="Order" Binding="{Binding OrderNumber}" />
        <tabular:TableViewTextColumn Header="Customer" Binding="{Binding Customer}" />
        <tabular:TableViewTextColumn Header="Status" Binding="{Binding Status}" />
    </tabular:TableView.Columns>
</tabular:TableView>
```

```csharp
Orders.Clear();
```

### Keyboard navigation & accessibility

TableView supports read-only row focus. Up, Down, Home, End, PageUp, and PageDown move focus through displayed rows.

Read-only UI Automation grid/table peers are provided for the table, rows, cells, and column headers.

```xaml
<tabular:TableView
    ItemsSource="{x:Bind People}"
    IsReadOnly="True"
    HeadersVisibility="Column"
    GridLinesVisibility="All">
    <tabular:TableView.Columns>
        <tabular:TableViewTextColumn Header="Name" Binding="{Binding Name}" />
        <tabular:TableViewTextColumn Header="Role" Binding="{Binding Role}" />
        <tabular:TableViewTextColumn Header="Location" Binding="{Binding Location}" />
    </tabular:TableView.Columns>
</tabular:TableView>
```

```csharp
// Editing is opt-in. TableView is read-only by default; clear IsReadOnly to enable it.
PeopleTable.IsReadOnly = false;
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

Selection is also independent of editing. The current cell and an open editor are unaffected by what is selected, and vice versa — so a `CellEditEnding` handler can read `SelectedItem` without worrying about ordering.

### Editing cells

Editing is opt-in. Clear `IsReadOnly` on the control, and optionally set `IsReadOnly` on individual columns to keep them display-only.

```xaml
<tabular:TableView x:Name="PeopleTable"
                   ItemsSource="{x:Bind People}"
                   IsReadOnly="False"
                   CellEditEnding="OnCellEditEnding">
    <tabular:TableView.Columns>
        <tabular:TableViewTextColumn Header="Name" Binding="{Binding Name}" />
        <tabular:TableViewTextColumn Header="Id"   Binding="{Binding Id}" IsReadOnly="True" />
    </tabular:TableView.Columns>
</tabular:TableView>
```

Editing starts only from user input in this release. There is no programmatic way to open an editor. Gestures follow the standard grid model:

| Gesture | Behaviour |
|---|---|
| Double-click / double-tap a cell | Begins editing that cell |
| `F2` | Begins editing the current cell |
| `Enter` | Commits and closes the editor |
| `Esc` | Cancels and discards the editor |
| Moving focus off the cell | Commits |

The data item needs settable properties, and should raise `PropertyChanged` so the display cell refreshes once the edit closes.

**Validating, and rejecting a value:**

```csharp
private void OnCellEditEnding(TableView sender, TableViewCellEditEndingEventArgs args)
{
    if (args.EditAction == TableViewEditAction.Commit &&
        args.Item is Person p && string.IsNullOrWhiteSpace(p.Name))
    {
        args.Cancel = true;   // keeps the editor open
    }
}
```

`CellEditEnding` is raised synchronously. Set `Cancel` before the handler returns; the control reads it immediately after the handler completes.

A cancelled edit never reaches the data item. Editors use `UpdateSourceTrigger=Explicit`, so cancelling discards the editor and the display element re-reads the unchanged source.

# API Pages

## TableView class

Represents a preview tabular control. It is display-only by default; cell editing is opt-in via `IsReadOnly`.

`TableView` derives from `Microsoft.UI.Xaml.Controls.Control` and has `Columns` as its content property.

Template parts:

| Part | Type | Description |
|---|---|---|
| `PART_HeaderRow` | FrameworkElement | Header row root. |
| `PART_HeaderHost` | `Panel` | Host for column header elements. |
| `PART_RowsRepeater` | `ItemsRepeater` | Repeater that displays rows. |
| `PART_EmptyStatePresenter` | `ContentControl` | Optional presenter for `EmptyTemplate`. |

### Example Usage

```xaml
<tabular:TableView
    x:Name="PeopleTable"
    ItemsSource="{x:Bind People}"
    HeadersVisibility="Column"
    GridLinesVisibility="All"
    Density="Standard"
    IsReadOnly="True">
    <tabular:TableView.Columns>
        <tabular:TableViewTextColumn Header="Name" Binding="{Binding Name}" />
        <tabular:TableViewTextColumn Header="Role" Binding="{Binding Role}" />
        <tabular:TableViewTextColumn Header="Location" Binding="{Binding Location}" />
    </tabular:TableView.Columns>
</tabular:TableView>
```

## TableView properties

| Property | Type | Default | Description |
|---|---|---|---|
| `ItemsSource` | `Object` | `null` | Source collection for table rows. |
| `Columns` | `IVector<TableViewColumn>` | Empty vector | Developer-defined column collection. This is the content property. |
| `HeadersVisibility` | `TableViewHeadersVisibility` | `Column` | Controls column-header visibility. |
| `GridLinesVisibility` | `TableViewGridLinesVisibility` | `All` | Controls horizontal and vertical gridlines. |
| `Density` | `TableViewDensity` | `Standard` | Controls row/cell spacing. |
| `RowBackground` | `Brush` | `null` | Background brush for rows. |
| `AlternatingRowBackground` | `Brush` | `null` | Optional alternating row background for banding. |
| `EmptyTemplate` | `DataTemplate` | `null` | Template displayed when there are no rows. |
| `IsReadOnly` | `Boolean` | `true` | Gates editing for the whole control. Editing is opt-in: while `true`, user gestures do not open an editor regardless of per-column `IsReadOnly`. Setting it to `true` while a cell is open closes that edit. |
| `IsEditing` | `Boolean` | `false` | `true` while a cell editor is open, through the matching commit/cancel close. Read-only. |
| `SelectionMode` | `TableViewSelectionMode` | `Single` | Gates row selection for the whole control. Selection is on by default, matching `ItemsView`, `ListView` and WPF's `DataGrid`; set `None` for a display-only table, which also clears any selection. |
| `SelectedItem` | `Object` | `null` | The selected data item. **Read-only** — drive selection through `Select`/`Deselect`/`DeselectAll`. |
| `SelectedIndex` | `Int32` | `-1` | The selected item's index in the `ItemsSource` index space; `-1` is "nothing selected". **Read-only.** |

`SelectedItem` and `SelectedIndex` are two read-only views of the same state and are always coherent. Both are dependency properties, so they can be bound *from* — `{x:Bind Table.SelectedItem.Name, Mode=OneWay}` — but not written. This matches `ItemsView`, whose `SelectedItem` and `CurrentItemIndex` are likewise get-only, and diverges from `Selector`/`ListView`, whose settable properties require a deferral-and-coercion layer to survive markup ordering and two-way bindings. Apps that need to drive selection from a view model do so through `Select(index)`. Selection is independent of editing: opening or closing an edit never changes what is selected.

Selection follows the **item**, not the index. Inserting a row above the selected one keeps the same item selected (its `SelectedIndex` shifts, with no `SelectionChanged`); removing the selected item clears the selection instead of selecting whatever slid into that slot. Replacing `ItemsSource` outright always clears the selection, even if the new source contains the same item.

A selection requested before it can be resolved is **not** queued: `Select(index)` called before `ItemsSource` is set, or while `SelectionMode` is `None`, is a no-op — matching `ItemsView`, whose `Select` is a straight pass-through to `SelectionModel`, and matching this control's own pointer and keyboard paths, which have always ignored a gesture while selection is off. Unlike markup, the caller controls ordering here, so a request that fired later when the mode or source changed would surprise more than it would help.

The one thing that *is* held is the live selection across an unload/reload cycle: unload drains the repeater's `ItemsSource` to release cache work on a detached subtree, and re-sourcing on load hands `SelectionModel` a new view, which clears it. The selected item is stashed and re-selected by identity on reload. `ItemsView` needs no equivalent because it never drains its repeater on unload.

An item that *is* resolvable but is not in `ItemsSource` has no index, so it clears the selection — matching `ListView`.

The current cell is the navigation position and is deliberately independent of editability: a read-only column is still a valid current cell. Editing starts only from user gestures: double-click/double-tap a cell, or press `F2` on the current cell.

## TableView editing methods

| Method | Returns | Description |
|---|---|---|
| `CommitEdit()` | `Boolean` | Closes the open **cell** edit, writing the value back. Returns `false` if there is no open edit, a handler vetoed the close, validation rejected the value, or the column could not write the value. |
| `CancelEdit()` | `Boolean` | Closes the open **cell** edit, discarding the editor so the display cell re-reads the unchanged source. Returns `false` if there is no open edit or a handler vetoed the close. |

## TableView editing events

| Event | Args | Description |
|---|---|---|
| `BeginningEdit` | `TableViewBeginningEditEventArgs` | Raised before an edit opens. Set `Cancel` to `true` to prevent it. |
| `CellEditEnding` | `TableViewCellEditEndingEventArgs` | Raised before a cell edit closes. Vetoable via `Cancel`; the handler must decide before returning. |

`CellEditEnding` is raised synchronously. The control reads `Cancel` immediately after the handler returns, so validation or save decisions must be complete before returning from the handler.

## TableView selection methods

All mutators resolve immediately when selection is on and a source exists; otherwise they are a no-op — `Select` never clears an existing selection to signal rejection. The only state carried across time is the live selection stashed over an unload/reload cycle, which `DeselectAll` and `SelectionMode = None` both discard.

Named to match `ItemsView`, which ships `Select` / `Deselect` / `IsSelected` over an item index. Identity-based overloads are deliberately absent, matching `ItemsView`.

| Method | Returns | Description |
|---|---|---|
| `Select(Int32 index)` | `void` | Selects the item at `index`. A negative index clears the selection. |
| `Deselect(Int32 index)` | `void` | Clears the selection only when `index` is the selected index, so a stale call cannot clobber a newer selection. |
| `IsSelected(Int32 index)` | `Boolean` | Whether `index` is the selected index. |
| `DeselectAll()` | `void` | Clears the selection. Works regardless of `SelectionMode`, so turning selection off can always be made to stick. |

## TableView selection events

| Event | Args | Description |
|---|---|---|
| `SelectionChanged` | `SelectionChangedEventArgs` | Raised after the selection has settled. Reading `SelectedItem`, `SelectedIndex`, or a row's `IsSelected` inside the handler observes the new state. Replacing a selection raises **one** event carrying both the removed and the added item. |

Selection is raised only for real changes: a re-select of the already-selected row, or an index shift caused by a collection reshape, does not raise `SelectionChanged`.

### Selection gestures

| Gesture | Behaviour |
|---|---|
| Pointer | Selects on **release**, for every pointer type — touch included, which reports no pressed button and so is admitted on device type rather than button state — matching `ListViewBaseItem` — which starts only timers and visuals on press and routes the actual selection through the tap interaction on release. Committing on press would select the row a touch pan started on, and would select on a press the user drags away from and cancels. The event is left **unhandled** so a begin-edit gesture on the same press still runs. |
| <kbd>Up</kbd>/<kbd>Down</kbd>/<kbd>Home</kbd>/<kbd>End</kbd>/<kbd>PageUp</kbd>/<kbd>PageDown</kbd> | Selection follows the keyboard cursor. This matches `ListView`'s default, where `SingleSelectionFollowsFocus` is `true`. When focus is not on a row (the user clicked a header, or tabbed away and back), <kbd>Up</kbd>/<kbd>Down</kbd> resume from the **selected** row rather than restarting at the top. |
| <kbd>Space</kbd> | Selects the focused row without moving. Only when the row itself has focus — a <kbd>Space</kbd> inside a cell's interactive content belongs to that control. |
| <kbd>Ctrl</kbd> + click, <kbd>Ctrl</kbd> + <kbd>Space</kbd> | Toggles: selects an unselected row, and **deselects the selected one**. Matches `SingleSelector` and `ListViewBase` single-selection behaviour, and is the only gesture that can clear a selection — without it the app would have to call `DeselectAll`. |
| <kbd>Ctrl</kbd> + navigation key | Moves the focus cursor **without** changing the selection, matching `ListViewBase`. This is how a keyboard or screen-reader user reviews other rows and returns without disturbing the selection. |

Selection is suppressed while a cell edit is open, because row navigation is: an open editor owns its keys.

### Known limitations

These are understood and deliberately not addressed by single selection:

- **Pointer selection is not blocked during an open edit.** Keyboard navigation is suppressed while editing, but clicking another row moves `SelectedItem` and the highlight immediately. If the resulting commit is then vetoed by validation, the editor stays open on the previous row while the selection has already moved. Whether selection should be blocked, deferred, or allowed to diverge from the edit target is an open decision.
- **The selection indicator scrolls with the row.** `PART_SelectionIndicator` lives inside the horizontally scrolling row content, so the accent strip scrolls off the leading edge. `TreeViewItem` and `ItemContainer` pin theirs to the container; doing the same here likely means reusing the frozen-column offset mechanism.
- **A `TableViewTemplateColumn` whose `CellTemplate` sets an explicit `Foreground` overrides the selected foreground.** `PART_CellForegroundPresenter` only reaches cells that *inherit* `Foreground`. `TableViewTextColumn` correctly sets none; template columns are free to, and in High Contrast that renders app-chosen text over `SystemColorHighlightColor`. Template columns should leave `Foreground` unset unless they take responsibility for the selected and High Contrast cases.
- **The in-file brush fallbacks cannot vary the selection indicator by theme.** `CommonStyles/TabularSurfaces_themeresources.xaml` is the canonical source and maps the indicator to `SystemColorHighlightColor` in High Contrast. The last-resort fallbacks in `TableView.xaml` are a flat dictionary, so the indicator stays `SystemAccentColor` there — a host that does not merge the shared dictionary gets an accent-coloured indicator in High Contrast. The row fills and foregrounds are unaffected: they use `{ThemeResource}` colours that do resolve per theme.
- **Reconciliation order is load-bearing.** Row chrome is restamped from `ItemsSourceView.CollectionChanged`, which is correct only because `SelectionModel` is handed the repeater's *shared* `ItemsSourceView` and is subscribed ahead of the control. Handing the model a raw source, or reordering those two calls in `ResolveSelectionAfterSourceChange`, silently reintroduces stale-index stamping — and an insert above the selection raises no event to correct it. This is deliberately different from `ItemsView`, which hands the model a raw source and repairs the resulting race afterwards with a dispatcher hop; the ordering here is structural instead.
- **Adding `Multiple`/`Extended` is not purely additive.** The enum values are appended and the event args already carry both vectors, so the shapes that are expensive to reverse are settled. But `SelectedItems` is deliberately **not** exposed in this release — it would be redundant with `SelectedItem` while at most one row can be selected, and `SelectionModel`'s view leaves `IndexOf` and `GetMany` unimplemented, so `Contains`, `ToList` and `ToArray` throw. It should be added with `Multiple`, where it becomes the only way to read the whole selection and those sharp edges are worth the capability. The gesture layer also routes through `SelectRowIndexFromInteraction(index, toggle)`, which carries no anchor or range state, and `ApplySelection` encodes single-selection semantics; multi-selection needs modifier state threaded through those entry points and an anchor model, closer to `ItemsView`'s `SelectorBase` strategy split.

## TableViewColumn class

Base class for TableView columns.

`TableViewColumn` derives from `Microsoft.UI.Xaml.DependencyObject` and has `Header` as its content property.

`Width` accepts `Pixel`/`Auto`/`*` (`GridLength`) for forward-compatibility; in v1 only explicit pixel widths are realized — `Auto` and `*` fall back to the default column width. True Auto/Star sizing is deferred.

| Property | Type | Default | Description |
|---|---|---|---|
| `Header` | `Object` | `null` | Header content for the column. |
| `HeaderTemplate` | `DataTemplate` | `null` | Template used to display the header. |
| `HeaderTemplateSelector` | `DataTemplateSelector` | `null` | Template selector used to display the header; takes precedence over `HeaderTemplate` when both are set. |
| `Width` | `GridLength` | Default column width | Accepts `Pixel`/`Auto`/`*` (`GridLength`) for forward-compatibility; in v1 only explicit pixel widths are realized — `Auto` and `*` fall back to the default column width. True Auto/Star sizing is deferred. |
| `MinWidth` | `Double` | `20` | Minimum column width. |
| `MaxWidth` | `Double` | Infinity | Maximum column width. |
| `ActualWidth` | `Double` | `120` | Read-only realized width. |
| `FrozenEdge` | `TableViewFrozenEdge` | `None` | Pins only when the column is in the contiguous leading prefix. Later `Leading` columns are ignored; `Trailing` is reserved. |
| `Visibility` | `Visibility` | `Visible` | Column visibility. |

Methods:

| Method | Description |
|---|---|
| `GenerateElement(Object dataItem)` | Generates the cell element for a data item. |
| `GenerateElementCore(Object dataItem)` | Overridable method used by derived column types to create cell content. |
| `IsReadOnly` (`Boolean`, default `false`) | Per-column opt-out. A read-only column is still a valid current cell for keyboard navigation, but cannot be edited. |
| `CellEditingTemplate` (`DataTemplate`, default `null`) | Editing visual for any column type. A column with neither a `CellEditingTemplate` nor a built-in editor is not editable. |
| `CellToolTipBinding` (`Binding`, default `null`) | Opt-in per-cell tooltip. The binding is evaluated against each row's data item; its value becomes the cell's tooltip content — a string, or anything a `ToolTip` can host. `null` or an empty string means no tooltip for that cell. Use an `IValueConverter` for computed content. A CLR property, not a DP, so XAML hands the `Binding` object over rather than evaluating it against the column (same shape as `TableViewTextColumn.Binding`). |

### Cell tooltips

Text cells render with `CharacterEllipsis` and no wrapping, so a value wider than its column is
unreadable. `CellToolTipBinding` surfaces the full value:

```xml
<tabular:TableViewTextColumn Header="Notes"
                             Binding="{Binding Notes}"
                             CellToolTipBinding="{Binding Notes}" />
```

Because the tooltip is an ordinary binding it tracks the row's `DataContext`: a recycled row
re-resolves its tooltips through the same inheritance that refreshes its cell text, and a source
`PropertyChanged` updates a live tooltip in place. There is no invalidation API, and none is needed.

The control owns the `ToolTip` and its placement (`PlacementMode.Mouse`), so the bound value is the
tooltip's *content*, not a `ToolTip`. A `UIElement` is parented by that cell's `ToolTip`, so a
converter must return a fresh element per evaluation. A tooltip the app sets inside the column's own
cell template is never touched; the control's tooltip covers the rest of the cell.

## TableViewTextColumn class

A column that generates a data-bound `TextBlock` for each cell, and a `TextBox` when the cell is edited.

`TableViewTextColumn` derives from `TableViewColumn`.

| Member | Type | Description |
|---|---|---|
| `Binding` | `Microsoft.UI.Xaml.Data.Binding` | CLR property used to bind generated cell text. This is not a dependency property. |

The editing `TextBox` reuses the column's `Binding` — its `Path`, `Converter`, `ConverterParameter`, `ConverterLanguage`, `TargetNullValue`, `FallbackValue` and source selector are all carried across — but forces `Mode=TwoWay` (a one-way display binding could never write back) and `UpdateSourceTrigger=Explicit`. `Explicit` is what lets the control decide *when* the value lands on the item, so a cancel can discard the editor without mutating the source and a validation failure can hold the edit open; with `PropertyChanged`, every keystroke would already have mutated the item.

The property the column edits is derived from `Binding.Path`, and only for bindings against the row data item: when the binding names an explicit `Source`, `ElementName` or `RelativeSource`, the path is relative to *that* object, so the column reports nothing rather than let the control snapshot and validate the wrong one. A column is never asked to author this path separately — it already knows what it is bound to, and the public "which field is this column about?" concept is the base column's `SortMemberPath`.

Setting `CellEditingTemplate` on a text column replaces the generated `TextBox` entirely, so an app can supply a different editor without subclassing. The built-in text editor selects all text when editing starts, so the first keystroke replaces the value.

Example:

```xaml
<tabular:TableViewTextColumn
    Header="Name"
    Binding="{Binding Name}" />
```

## TableViewTemplateColumn class

A column that uses a consumer-provided `DataTemplate` for cell content.

`TableViewTemplateColumn` derives from `TableViewColumn`.

| Property | Type | Default | Description |
|---|---|---|---|
| `CellTemplate` | `DataTemplate` | `null` | Template used to display each cell. If null, the generated cell is empty. |

`CellEditingTemplate` is inherited from `TableViewColumn` — it is not specific to template columns. A template column with no `CellEditingTemplate` is not editable: there is no single value to infer an editor from, and falling back to `CellTemplate` would open an "editor" that silently discards every change.

The editable value inside a `CellEditingTemplate` should use classic `{Binding}`, not `{x:Bind}`. The base commit discovers editor bindings with `GetBindingExpression`, and compiled `{x:Bind}` bindings are not discoverable that way.

Example:

```xaml
<tabular:TableViewTemplateColumn Header="Status">
    <tabular:TableViewTemplateColumn.CellTemplate>
        <DataTemplate x:DataType="local:Order">
            <TextBlock Text="{x:Bind Status}" />
        </DataTemplate>
    </tabular:TableViewTemplateColumn.CellTemplate>
    <tabular:TableViewTemplateColumn.CellEditingTemplate>
        <DataTemplate x:DataType="local:Order">
            <ComboBox ItemsSource="{x:Bind StatusChoices}"
                      SelectedItem="{Binding Status, Mode=TwoWay}" />
        </DataTemplate>
    </tabular:TableViewTemplateColumn.CellEditingTemplate>
</tabular:TableViewTemplateColumn>
```

## TableViewRow class

Represents a generated row in a TableView.

`TableViewRow` derives from `Microsoft.UI.Xaml.Controls.Control`.

Template parts:

| Part | Type | Description |
|---|---|---|
| `PART_RootBorder` | `Border` | Row root border. Its `Background` is driven by `CommonStates`. |
| `PART_CellsHost` | `Panel` | Host for generated cell elements. |
| `PART_SelectionIndicator` | `UIElement` | **Required.** Leading-edge accent strip; `Opacity` is animated `0 → 1` by the `Selected*` states, which target it by name — a re-template that omits it fails when a row is first selected, not at parse time. |

Visual states:

| Group | States |
|---|---|
| `CommonStates` | `Normal`, `PointerOver`, `Pressed`, `Disabled`, `Selected`, `SelectedPointerOver`, `SelectedPressed`, `SelectedDisabled` |

Selected states share the one group rather than living in a second, overlapping one: they animate the same `PART_RootBorder.Background`, so two groups would make the result depend on `GoToState` call order. Encoding both dimensions in the state name is the pattern `TreeViewItem` and `ItemContainer` use.

Properties:

| Property | Type | Default | Description |
|---|---|---|---|
| `IsSelected` | `Boolean` | `false` | Whether the row is the selected row. Read-only: selection is owned by the `TableView`, so a row cannot select itself — apps go through `Select(Int32)` / `Deselect(Int32)` / `DeselectAll()`. Rows are recycled, so this is always re-derived. |

## Enums

### TableViewFrozenEdge

| Value | Numeric value | Description |
|---|---:|---|
| `None` | 0 | Column is not frozen. |
| `Leading` | 1 | Column is pinned to the leading edge. Implemented in v1. |
| `Trailing` | 2 | Reserved for future trailing-edge pinning. |

### TableViewHeadersVisibility

Flags enum mirroring the `None`/`Column` slots of WPF `DataGridHeadersVisibility`. Row headers are out of scope, so `Row`/`All` (WPF values 2/3) are intentionally not defined.

| Value | Numeric value | Description |
|---|---:|---|
| `None` | 0 | No headers. |
| `Column` | 1 | Column-header strip. |

### TableViewGridLinesVisibility

Mirrors WPF `DataGridGridLinesVisibility` values exactly.

| Value | Numeric value | Description |
|---|---:|---|
| `All` | 0 | Show horizontal and vertical gridlines. |
| `Horizontal` | 1 | Show horizontal gridlines. |
| `None` | 2 | Show no gridlines. |
| `Vertical` | 3 | Show vertical gridlines. |

### TableViewDensity

| Value | Numeric value | Description |
|---|---:|---|
| `Compact` | 0 | Compact spacing. |
| `Standard` | 1 | Standard spacing. |
| `Comfortable` | 2 | Comfortable spacing. |

### TableViewSelectionMode

Only `None` and `Single` exist in this release: `Multiple`/`Extended` need an anchor, a range model and Ctrl/Shift gesture handling, none of which single selection implies. New members are appended, so these numeric values stay stable when they arrive.

| Value | Numeric value | Description |
|---|---:|---|
| `None` | 0 | Selection is off; a display-only table. |
| `Single` | 1 | At most one row is selected. **The default.** |

### TableViewEditAction

How an edit is being closed.

| Value | Numeric value | Description |
|---|---:|---|
| `Commit` | 0 | The edited value is being written to the source. |
| `Cancel` | 1 | The edit is being cancelled and the editor is discarded. |

## Editing event args

| Type | Members |
|---|---|
| `TableViewBeginningEditEventArgs` | `Item`, `Column` (read-only); `Cancel` (settable) |
| `TableViewCellEditEndingEventArgs` | `Item`, `Column`, `EditAction` (read-only); `Cancel` (settable) |

### Cell tooltip accessibility

The control owns the `ToolTip`; the bound value is its content, not a `ToolTip` to attach. A `UIElement` is parented by that cell's `ToolTip`, so a converter returns a fresh element per evaluation.

- String tooltip text is published as the cell's `AutomationProperties.HelpText`, and retracted on recycle and when a cell edit begins.
- `TableViewCellAutomationPeer` suppresses it at UIA query time when it equals the cell's own UIA text, so Narrator does not read it twice. Suppression is gated on the control's ownership record, so text the app set is never dropped, and it is resolved at query time because the cell's own binding may not have produced a value when the tooltip is applied.
- The popup is **pointer-only**: cell focus in `TableView` is row-level, so there is no cell element for the framework's keyboard-tooltip path to fire on. The UIA pairing is what serves keyboard and screen-reader users, which is why it is not optional.
- Placement is control-owned and fixed (`PlacementMode.Mouse`), matching `TabViewItem`. An app needing different placement uses a tooltip inside its own cell content template.
- Non-string content is pointer-only: it cannot be stringified, and the cell wrapper the tooltip attaches to is internal, so an app cannot set `HelpText` on it. Use a converter that returns text when the value must be accessible.

## Selection event args

| Type | Members |
|---|---|
| `Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs` | `AddedItems`, `RemovedItems` (`IVector<Object>`) |

TableView reuses the **platform** `SelectionChangedEventArgs` rather than defining its own, so a handler can be shared with `ListView`/`ListBox`. Both vectors are snapshots built when the event is raised.

## Automation peers (TableView/Row/Cell/ColumnHeader)

TableView provides UI Automation peers for grid/table accessibility, cell value access, and row selection.

| Peer | Base type | Provider interfaces |
|---|---|---|
| `TableViewAutomationPeer` | `FrameworkElementAutomationPeer` | `ISelectionProvider`, `IGridProvider`, `ITableProvider`, `IItemContainerProvider` |
| `TableViewRowAutomationPeer` | `FrameworkElementAutomationPeer` | `ISelectionItemProvider` |
| `TableViewColumnHeaderAutomationPeer` | `FrameworkElementAutomationPeer` | (none) |
| `TableViewCellAutomationPeer` | `FrameworkElementAutomationPeer` | `IGridItemProvider`, `ITableItemProvider`, `IValueProvider` |

These peers expose the table structure to assistive technologies. Cell peers also expose their value.

`Selection`/`SelectionItem` are advertised only while `SelectionMode` allows selection — advertising them while it is `None` would tell an AT client the grid is selectable when every `Select()` would be refused. `CanSelectMultiple` is `false` and `IsSelectionRequired` is `false`. `GetSelection()` returns the selected row's provider when that row is realized; a selected row scrolled out of the realization window is reached through `IItemContainerProvider.FindItemByProperty`, which realizes it.

# API Details

```idl
namespace Microsoft.UI.Xaml.Controls.Tabular
{
    [MUX_PREVIEW, webhosthidden]
    enum TableViewFrozenEdge
    {
        None = 0,
        Leading = 1,
        Trailing = 2,
    };

    [MUX_PREVIEW, webhosthidden, flags]
    enum TableViewHeadersVisibility
    {
        None = 0,
        Column = 1,
    };

    [MUX_PREVIEW, webhosthidden]
    enum TableViewGridLinesVisibility
    {
        All = 0,
        Horizontal = 1,
        None = 2,
        Vertical = 3,
    };

    [MUX_PREVIEW, webhosthidden]
    enum TableViewDensity
    {
        Compact = 0,
        Standard = 1,
        Comfortable = 2,
    };

    [MUX_PREVIEW, webhosthidden]
    enum TableViewEditAction
    {
        Commit = 0,
        Cancel = 1,
    };

    [MUX_PREVIEW, webhosthidden]
    enum TableViewSelectionMode
    {
        None = 0,
        Single = 1,
    };

    [MUX_PREVIEW, webhosthidden]
    runtimeclass TableViewBeginningEditEventArgs
    {
        Object Item { get; };
        Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn Column { get; };
        Boolean Cancel;
    };

    [MUX_PREVIEW, webhosthidden]
    runtimeclass TableViewCellEditEndingEventArgs
    {
        Object Item { get; };
        Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn Column { get; };
        Microsoft.UI.Xaml.Controls.Tabular.TableViewEditAction EditAction { get; };
        Boolean Cancel;
    };

    [MUX_PREVIEW, webhosthidden, contentproperty("Header")]
    unsealed runtimeclass TableViewColumn : Microsoft.UI.Xaml.DependencyObject
    {
        TableViewColumn();

        Object Header;
        Microsoft.UI.Xaml.DataTemplate HeaderTemplate;
        Microsoft.UI.Xaml.Controls.DataTemplateSelector HeaderTemplateSelector;
        Microsoft.UI.Xaml.GridLength Width;
        Double MinWidth;
        Double MaxWidth;
        Double ActualWidth { get; };
        Microsoft.UI.Xaml.Controls.Tabular.TableViewFrozenEdge FrozenEdge;
        Microsoft.UI.Xaml.Visibility Visibility;

        Microsoft.UI.Xaml.FrameworkElement GenerateElement(Object dataItem);
        overridable Microsoft.UI.Xaml.FrameworkElement GenerateElementCore(Object dataItem);

        Boolean IsReadOnly;
        Microsoft.UI.Xaml.DataTemplate CellEditingTemplate;
        Microsoft.UI.Xaml.Data.Binding CellToolTipBinding;

        static Microsoft.UI.Xaml.DependencyProperty HeaderProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderTemplateProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeaderTemplateSelectorProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty WidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty MinWidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty MaxWidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty ActualWidthProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty FrozenEdgeProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty VisibilityProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsReadOnlyProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty CellEditingTemplateProperty { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewTextColumn : Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn
    {
        TableViewTextColumn();

        Microsoft.UI.Xaml.Data.Binding Binding;
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewTemplateColumn : Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn
    {
        TableViewTemplateColumn();

        Microsoft.UI.Xaml.DataTemplate CellTemplate;

        static Microsoft.UI.Xaml.DependencyProperty CellTemplateProperty { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewRow : Microsoft.UI.Xaml.Controls.Control
    {
        TableViewRow();

        Boolean IsSelected { get; };

        static Microsoft.UI.Xaml.DependencyProperty IsSelectedProperty { get; };
    };

    [MUX_PREVIEW, webhosthidden, contentproperty("Columns")]
    unsealed runtimeclass TableView : Microsoft.UI.Xaml.Controls.Control
    {
        TableView();

        Object ItemsSource;
        Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn> Columns { get; };
        Microsoft.UI.Xaml.Controls.Tabular.TableViewHeadersVisibility HeadersVisibility;
        Microsoft.UI.Xaml.Controls.Tabular.TableViewGridLinesVisibility GridLinesVisibility;
        Microsoft.UI.Xaml.Media.Brush RowBackground;
        Microsoft.UI.Xaml.Media.Brush AlternatingRowBackground;
        Microsoft.UI.Xaml.DataTemplate EmptyTemplate;
        Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity Density;
        Boolean IsReadOnly;

        // ----- Editing -----

        Boolean IsEditing { get; };

        Boolean CommitEdit();
        Boolean CancelEdit();

        event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.Tabular.TableView, Microsoft.UI.Xaml.Controls.Tabular.TableViewBeginningEditEventArgs> BeginningEdit;
        event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.Tabular.TableView, Microsoft.UI.Xaml.Controls.Tabular.TableViewCellEditEndingEventArgs> CellEditEnding;

        // ----- Selection -----

        Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode SelectionMode;
        Object SelectedItem { get; };
        Int32 SelectedIndex { get; };

        void Select(Int32 index);
        void Deselect(Int32 index);
        Boolean IsSelected(Int32 index);
        void DeselectAll();

        event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.Tabular.TableView, Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs> SelectionChanged;

        static Microsoft.UI.Xaml.DependencyProperty ItemsSourceProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty ColumnsProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty HeadersVisibilityProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty GridLinesVisibilityProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty RowBackgroundProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty AlternatingRowBackgroundProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty EmptyTemplateProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty DensityProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty IsReadOnlyProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SelectionModeProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SelectedItemProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty SelectedIndexProperty { get; };
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewAutomationPeer :
        Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
        Microsoft.UI.Xaml.Automation.Provider.ISelectionProvider,
        Microsoft.UI.Xaml.Automation.Provider.IGridProvider,
        Microsoft.UI.Xaml.Automation.Provider.ITableProvider,
        Microsoft.UI.Xaml.Automation.Provider.IItemContainerProvider
    {
        TableViewAutomationPeer(Microsoft.UI.Xaml.Controls.Tabular.TableView owner);
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewRowAutomationPeer :
        Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
        Microsoft.UI.Xaml.Automation.Provider.ISelectionItemProvider
    {
        TableViewRowAutomationPeer(Microsoft.UI.Xaml.Controls.Tabular.TableViewRow owner);
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewColumnHeaderAutomationPeer :
        Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        TableViewColumnHeaderAutomationPeer(
            Microsoft.UI.Xaml.Controls.Tabular.TableView owner,
            Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn column);
    };

    [MUX_PREVIEW, webhosthidden]
    unsealed runtimeclass TableViewCellAutomationPeer :
        Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer,
        Microsoft.UI.Xaml.Automation.Provider.IGridItemProvider,
        Microsoft.UI.Xaml.Automation.Provider.ITableItemProvider,
        Microsoft.UI.Xaml.Automation.Provider.IValueProvider
    {
        TableViewCellAutomationPeer(
            Microsoft.UI.Xaml.FrameworkElement cell,
            Microsoft.UI.Xaml.Controls.Tabular.TableViewRow row,
            Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn column,
            Int32 columnIndex);
    };
}
```

# Appendix

## Cell editing scope

This release adds **opt-in cell editing** and **single row selection** on top of the display-only TableView. The remaining interactive v1 features (multiple/extended selection, sorting, grouping, cell keyboard navigation, column resize/reorder) are planned separately and land additively.

Added here:

- Control-level opt-in through `IsReadOnly`, plus per-column `IsReadOnly`.
- Editors supplied by built-in columns, or by `CellEditingTemplate` on the base `TableViewColumn`; `TableViewTextColumn` produces a `TextBox` when no template is supplied.
- Edit lifecycle API: `CommitEdit`, `CancelEdit`, and `IsEditing`. Editing starts from user gestures only.
- Vetoable events `BeginningEdit` and `CellEditEnding`. `CellEditEnding` is synchronous, so handlers must set `Cancel` before returning.
- Rollback for a cancelled cell edit by using `UpdateSourceTrigger=Explicit`; the source is unchanged until commit, so cancelling discards the editor and re-displays the original value.
- Validation via `INotifyDataErrorInfo`, scoped to the property the column edits. The edited property is derived from the column's `Binding` rather than authored separately, so it stays one concept with the base column's `SortMemberPath`.
- Gestures: double-click/double-tap, `F2`, `Enter`, `Esc`, and commit on focus loss. The pointer gesture is handled by `TableViewRow`, which owns its cells and can therefore resolve which cell a press landed on; the control keeps the edit state machine.

### Editing is cell-scoped in this release

`CommitEdit()` and `CancelEdit()` close the open **cell** edit. There is no row-scoped commit or
cancel, and no `RowEditEnding`, because a row transaction cannot yet be closed on its own - the edit
state machine models "a cell editor is open" and nothing else. Both arrive with row editing rather
than shipping as API that silently does nothing.

### When the edited value reaches the data item

**A commit at cell scope writes to the data item immediately.** This differs from WPF's DataGrid and
is worth stating explicitly, because the difference is invisible until an app tries to cancel a row.

WPF keeps pending values in the row's `BindingGroup`, and the values reach the item when
`BindingGroup.CommitEdit()` runs at row scope. Cancelling the row therefore reverts every cell
edited in it, with no cooperation from the data item.

WinUI has no `BindingGroup`. TableView instead binds the editor with
`UpdateSourceTrigger=Explicit` and writes on commit, which keeps `Esc` restorable for the cell being
edited. The consequences:

- Cancelling a cell reverts that cell because the pending value never reached the item.
- There is no row-level transaction in this release.
- An app observing `PropertyChanged` sees one notification per cell commit, not one per row.

Deliberately out of scope for cell editing: multi-cell row transactions and editing a11y
announcements (blocked on localized resource strings).

The display-only base delivered previously:

- Preview API surface for display-only TableView.
- Developer-defined columns through `TableView.Columns`.
- Text cells through `TableViewTextColumn`.
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
- Single-column sort and filtering.
- Grouping and two-level hierarchy.
- Column resize and reorder.

## Non-goals (out of scope for v1)

- Replacing DataGrid.
- Marquee selection.
- Multi-column sort.
- Column virtualization.
- Row headers.
- Hierarchy deeper than two levels; row drag-drop.
- Spreadsheet-like interaction.
- Spreadsheet-scale column counts (100+) — `TableView` realizes a cell per visible column per row and targets ~5–50 columns.

Theme-XBF emission is suppressed in this release, and is tracked separately.
