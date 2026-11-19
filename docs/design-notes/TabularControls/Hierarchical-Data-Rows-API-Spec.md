# TableView hierarchical rows — API spec

## Summary

`TableView` can show a flat collection as an expandable tree. The app keeps its data flat, one
item per row, and declares how the rows relate with two property paths on `TableView`:

- `ItemKeyPath`, the property that holds each item's unique key, and
- `ParentKeyPath`, the property that holds the key of the item's parent, or `null` for a root.

The items need no children collection, level, or expanded flag. Because the relation lives on the
table rather than in the data, the same collection can be shown as different trees, for example
by manager in one table and by mentor in another.

> Earlier previews declared the relation with `TableViewSource.ParentBy(keySelector, parentKeySelector)`.
> `TableViewSource` is removed; data shaping is `CollectionViewSource`, and the relation moved to
> `TableView`. Hierarchy on `CollectionViewSource` itself is future work.

## API

```idl
namespace Microsoft.UI.Xaml.Controls.Tabular
{
    runtimeclass TableView
    {
        // Property paths of each item's key and of its parent's key. Both set: the rows nest.
        String ItemKeyPath { get; set; };
        String ParentKeyPath { get; set; };

        // Expand or collapse every row of a hierarchical source. No-ops otherwise.
        void ExpandAllRows();
        void CollapseAllRows();
    }

    runtimeclass TableViewRow
    {
        Int32   Level        { get; };  // 1-based depth (1 = root)
        Boolean IsExpandable { get; };  // the row has children
        Boolean IsExpanded   { get; };
    }
}
```

| Member | Behavior |
| --- | --- |
| `ItemKeyPath` / `ParentKeyPath` | Both non-empty: builds the tree over `ItemsSource`. Changing either replaces the relation and resets expansion. Either empty: the rows are flat. The relation is re-applied when `ItemsSource` changes. |
| `ExpandAllRows()` / `CollapseAllRows()` | Opens or closes every node. Individual rows toggle through the chevron, the keyboard, or the UIA ExpandCollapse pattern. |
| `TableViewRow.Level` / `IsExpandable` / `IsExpanded` | Read-only row state, used for templating and automation. |

### Rules

- **Keys.**
  - Each path is evaluated like a `{Binding}` path on the item (the item type must be bindable).
  - Each key must be unique and must not be null or empty.
  - Strings, numbers, `Boolean`, `Char16`, `Guid`, `DateTime`, `TimeSpan`, `Point`, `Size`, `Rect` and WinRT enums are compared by value, with the type included (an `Int32` never matches an `Int64`, so give both properties the same type). Any other key is compared by reference.
- **Roots.** An item is a root if its parent key is null or empty, or if no item has that key (an orphan).
- **Invalid data.** A null or empty key, a duplicate key, a node that is its own parent, and a cycle all throw `E_INVALIDARG`, naming the key where there is one. The previous rows stay on screen.
  - Setting a path that makes the data invalid throws from the setter and keeps the previous relation. When the relation is applied to a new `ItemsSource` and the data is invalid, the rows are shown flat and the error is logged.
  - Invalid data that arrives later through a collection change leaves the relation declared; every later reshape validates again until the data is fixed or a path is cleared.
  - A throw raised while handling a collection change may be swallowed by the collection (C++/WinRT observable vectors do this). The previous rows still stay on screen.
- **Initial state.** Rows start collapsed.
- **Expansion memory.** Expansion is remembered per key. It survives sort, filter and collection changes.
- **Composition with a `CollectionViewSource` view** (`ItemsSource = cvs.View`):
  - The view's `SortDescriptions` order the rows within each set of siblings (siblings keep the view's order). A header click sorts the view.
  - The view's `Filter` runs before the rows are nested: a match whose parent is filtered out is shown as a root. (`TableViewSource` kept the ancestors of a match; that is not available in V1.)
  - With `GroupDescriptions`, each root is placed in its group and keeps its subtree.
- **Change tracking.**
  - Adding, removing or replacing items in the collection updates the tree.
  - Changing a property of an existing item (its key or parent key) is not observed. Call `Refresh()` on the view, or replace the item, to move a row to a new parent. Live shaping is not available in V1.

## Example

### Input

One flat `ObservableCollection<Employee>`:

| Id | Name | ManagerId |
| --- | --- | --- |
| 1 | Ada | `null` |
| 2 | Ben | 1 |
| 3 | Cy | 1 |
| 4 | Dan | 2 |
| 5 | Eve | 99 *(no such Id)* |

### XAML

```xml
<Page xmlns:tv="using:Microsoft.UI.Xaml.Controls.Tabular" ...>
    <tv:TableView x:Name="Org" SelectionMode="Single" ItemKeyPath="Id" ParentKeyPath="ManagerId">
        <tv:TableViewTemplateColumn Header="Name" SortMemberPath="Name">
            <tv:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Name}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tv:TableViewTemplateColumn.CellTemplate>
        </tv:TableViewTemplateColumn>
        <tv:TableViewTemplateColumn Header="Id" SortMemberPath="Id">
            <tv:TableViewTemplateColumn.CellTemplate>
                <DataTemplate>
                    <TextBlock Text="{Binding Id}" Style="{StaticResource TableCellTextStyle}" />
                </DataTemplate>
            </tv:TableViewTemplateColumn.CellTemplate>
        </tv:TableViewTemplateColumn>
    </tv:TableView>
</Page>
```

### C#

```csharp
public sealed class Employee
{
    public int  Id        { get; init; }
    public string Name    { get; init; } = "";
    public int? ManagerId { get; init; }
}

var people = new ObservableCollection<Employee>
{
    new() { Id = 1, Name = "Ada" },
    new() { Id = 2, Name = "Ben", ManagerId = 1 },
    new() { Id = 3, Name = "Cy",  ManagerId = 1 },
    new() { Id = 4, Name = "Dan", ManagerId = 2 },
    new() { Id = 5, Name = "Eve", ManagerId = 99 },
};

Org.ItemsSource = people; // ItemKeyPath="Id", ParentKeyPath="ManagerId" (null => root) are set in XAML
Org.ExpandAllRows(); // optional; rows start collapsed
```

### Output

Initial (collapsed): only the roots show. Eve is a root because her manager `99` does not exist.

```
Name        Id
> Ada       1
  Eve       5
```

After `ExpandAllRows()`:

```
Name            Id     Level
v Ada           1      1
  v Ben         2      2
      Dan       4      3
    Cy          3      2
  Eve           5      1
```

`>` marks a collapsed row and `v` an expanded one. Leaves have no chevron, and each level is
indented by the `TableViewRowIndentSize` theme resource.

Showing the same `people` as a different tree needs only a second table. For example, if
`Employee` also had a `MentorId`:

```xml
<tv:TableView x:Name="MentorTable" ItemsSource="{x:Bind People}" ItemKeyPath="Id" ParentKeyPath="MentorId" />
```
