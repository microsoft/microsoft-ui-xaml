# TableView hierarchical rows — API spec

## Summary

`TableView` can show a flat collection as an expandable tree. The app keeps its data flat, one
item per row, and declares how the rows relate with two selectors on `TableViewSource`:

- a **key selector** that returns each item's unique key, and
- a **parent key selector** that returns the key of the item's parent, or `null` for a root.

The items need no children collection, level, or expanded flag. Because the relation lives on the
source rather than in the data, the same collection can be shown as different trees, for example
by manager in one table and by mentor in another.

## API

```idl
namespace Microsoft.UI.Xaml.Controls.Tabular
{
    delegate Object TableViewKeySelector(Object item);

    runtimeclass TableViewSource
    {
        // Declares a self-referencing parent/child relation over the flat source.
        TableViewSource ParentBy(TableViewKeySelector keySelector,
                                   TableViewKeySelector parentKeySelector);

        // Removes the relation and shows the source flat again.
        TableViewSource ClearParentBy();
    }

    runtimeclass TableView
    {
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
| `ParentBy(key, parentKey)` | Builds the tree. Calling it again replaces the relation and resets expansion. |
| `ClearParentBy()` | Removes the tree and shows the source flat. Resets expansion. |
| `ExpandAllRows()` / `CollapseAllRows()` | Opens or closes every node. Individual rows toggle through the chevron, the keyboard, or the UIA ExpandCollapse pattern. |
| `TableViewRow.Level` / `IsExpandable` / `IsExpanded` | Read-only row state, used for templating and automation. |

### Rules

- **Keys.**
  - Each key must be unique and must not be null or empty.
  - Strings, numbers, `Boolean`, `Char16`, `Guid`, `DateTime`, `TimeSpan`, `Point`, `Size`, `Rect` and WinRT enums are compared by value, with the type included (an `Int32` never matches an `Int64`). Any other key is compared by reference.
  - Convert a C# enum key to its integer value first.
- **Roots.** An item is a root if its parent key is null or empty, or if no item has that key (an orphan).
- **Invalid data.** A null or empty key, a duplicate key, a node that is its own parent, and a cycle all throw `E_INVALIDARG`, naming the key where there is one. The previous rows stay on screen.
  - After a throw the relation stays declared. Every later reshape or collection change validates again and throws again, until the data is fixed or `ClearParentBy()` is called.
  - A throw raised while handling a collection change may be swallowed by the collection (C++/WinRT observable vectors do this). The previous rows still stay on screen.
- **Initial state.** Rows start collapsed.
- **Expansion memory.** Expansion is remembered per key. It survives sort, filter and collection changes.
- **Composition with other verbs:**
  - `Sort` orders the rows within each set of siblings.
  - `Filter` keeps each match plus its ancestors, and shows those ancestors expanded while the filter is active.
  - `GroupBy` buckets the roots, and each root keeps its subtree.
- **Change tracking.**
  - Adding, removing or replacing items in the collection updates the tree.
  - Changing a property of an existing item (its key or parent key) is not noticed. To move a row to a new parent, replace the item.

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
    <tv:TableView x:Name="Org" SelectionMode="Single">
        <tv:TableViewTextColumn Header="Name" Binding="{Binding Name}" />
        <tv:TableViewTextColumn Header="Id"   Binding="{Binding Id}" />
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

var source = TableViewSource.From(people).ParentBy(
    new TableViewKeySelector(item => ((Employee)item).Id),
    new TableViewKeySelector(item => ((Employee)item).ManagerId!)); // null => root

Org.ItemsSource = source;
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

Showing the same `people` as a different tree needs only a second source. For example, if
`Employee` also had a `MentorId`:

```csharp
MentorTable.ItemsSource = TableViewSource.From(people).ParentBy(
    new TableViewKeySelector(item => ((Employee)item).Id),
    new TableViewKeySelector(item => ((Employee)item).MentorId!));
```
