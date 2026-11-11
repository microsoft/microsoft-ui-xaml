# ListView RowContext (experimental)

> This API is **experimental** (gated behind `Feature_ExperimentalApi`). It is a proof of
> concept and may change or be removed.

# Background

Rich list styles often need to draw differently depending on a row's neighbors. For example,
a selected row joins to a selected neighbor with square corners, and a separator is drawn only
between two rows that are both selected or both unselected.

Today the template of a `ListViewItem` has no way to learn about its neighbors. Apps must track
selection and order themselves and push the result into every container, which is fragile with
virtualization, grouping, and collection changes.

`RowContext` makes `ListView` report this information itself. It supplies **facts only**. What
to draw is left to the item template, a style, or a visual state.

This spec does **not** include a conditional-setter mechanism such as `Trigger`/`MultiTrigger`.
Templates can react to the values with existing mechanisms (bindings, state triggers, or
property-changed callbacks).

# Conceptual pages (How To)

`ListView` maintains four read-only attached properties on each realized `ListViewItem`:

| Property | Meaning |
|---|---|
| `PreviousRelation` | Relation of the previous displayed row: `None`, `SameGroup`, or `OtherGroup` |
| `NextRelation` | Relation of the next displayed row |
| `IsPreviousSelected` | The previous displayed row is selected |
| `IsNextSelected` | The next displayed row is selected |

Rules:

- Neighbors are the **displayed** rows in order, whether or not their containers are realized.
- If there is no neighbor, the relation is `None` and the matching `Is...Selected` is `false`.
- In a list that is not grouped, every neighbor is `SameGroup`.
- Group headers are not neighbors.
- Values are correct when the container is prepared and are updated when selection, the
  collection, or the groups change.

Limits of this proof of concept:

- Only `ListViewItem` in a vertical `ItemsStackPanel`. Other containers and panels keep the defaults.
- Values are set one property at a time, so a consumer can briefly see a mix of old and new values
  during an update.

# Examples

## Square the corners between selected rows

```csharp
void UpdateCorners(ListViewItem item)
{
    bool joinAbove = item.IsSelected && RowContext.GetIsPreviousSelected(item);
    bool joinBelow = item.IsSelected && RowContext.GetIsNextSelected(item);
    selectionBackground.CornerRadius = new CornerRadius(
        joinAbove ? 0 : 8, joinAbove ? 0 : 8, joinBelow ? 0 : 8, joinBelow ? 0 : 8);
}
```

# API Pages

## RowContext class

Static class that owns the read-only attached properties above.

## NeighborRelation enum

| Value | Meaning |
|---|---|
| `None` | There is no neighboring row |
| `SameGroup` | The neighbor is in the same group (or the list is not grouped) |
| `OtherGroup` | The neighbor is in a different group |

# API Details

```idl
namespace Microsoft.UI.Xaml.Controls
{
    [feature(Feature_ExperimentalApi)]
    enum NeighborRelation
    {
        None = 0,
        SameGroup = 1,
        OtherGroup = 2,
    };

    [feature(Feature_ExperimentalApi)]
    static runtimeclass RowContext
    {
        static Microsoft.UI.Xaml.DependencyProperty PreviousRelationProperty{ get; };
        static NeighborRelation GetPreviousRelation(ListViewItem element);

        static Microsoft.UI.Xaml.DependencyProperty NextRelationProperty{ get; };
        static NeighborRelation GetNextRelation(ListViewItem element);

        static Microsoft.UI.Xaml.DependencyProperty IsPreviousSelectedProperty{ get; };
        static Boolean GetIsPreviousSelected(ListViewItem element);

        static Microsoft.UI.Xaml.DependencyProperty IsNextSelectedProperty{ get; };
        static Boolean GetIsNextSelected(ListViewItem element);
    }
}
```

# Open questions

- Whether `GridViewItem` and other layouts should also report context.
- Whether the four values should update together to avoid transient inconsistent combinations.
- How group headers should take part, for example to show a separator above the first group.
- Whether a conditional-setter mechanism should later consume these values.
