# Trigger and MultiTrigger (experimental)

> This API is **experimental** (gated behind `Feature_ExperimentalApi`). It is a proof of
> concept. Several behaviors below are deliberately limited and are listed under
> "Open questions".

# Background

`VisualState` groups describe mutually exclusive modes. They are a poor fit for styling that
depends on several independent conditions, such as "selected, and the next row is also
selected". WPF solves this with `Trigger` and `MultiTrigger`: rules that apply a set of
setters while their conditions hold, and that combine freely.

This spec adds the WPF-style conditional setters as a proof of concept. It does not replace
visual states: use visual states for named modes and transitions, and triggers for composable
conditional styling.

Not included: `DataTrigger`, `MultiDataTrigger`, entry/exit actions, `Style.Triggers`, and
`ControlTemplate.Triggers`.

# Conceptual pages (How To)

A trigger lives in the existing `FrameworkElement.Triggers` collection of an element. Place it
on the root element of a control template to style that control.

- A `Trigger` has one condition: a property name and the value it must equal.
- A `MultiTrigger` has several `Condition`s and applies its setters only while **all** hold.
- The setters are ordinary `Setter` objects that name a template part with `Target`, exactly as
  in `VisualState.Setters`: `Target="PartName.Property"`.
- Inside a control template the conditions are tested against the **templated control** (for
  example the `ListViewItem`). On an element that is not part of a template, they are tested
  against that element.
- `Property` is a property name: `IsSelected`, or an attached property qualified by its owner,
  such as `RowContext.PreviousRelation`. `Value` is converted to the property's type, so
  `True`, `SameGroup` and `100` work for a Boolean, an enum and a number.

Evaluation rules:

- Triggers are active while their element is in the live tree and are re-evaluated when any
  property they test changes.
- Several triggers can apply at once. For each target property the **last declared active
  setter wins**. When it deactivates, the previous active setter (or the original value)
  shows through again. A `MultiTrigger` has no extra priority.
- When the element leaves the tree, all values applied by its triggers are removed.

# Examples

## A selection that joins neighbors in a ListView item template

```xml
<ControlTemplate TargetType="ListViewItem">
  <Grid>
    <Grid.Triggers>
      <Trigger Property="IsSelected" Value="True">
        <Setter Target="Selection.Visibility" Value="Visible" />
      </Trigger>

      <MultiTrigger>
        <MultiTrigger.Conditions>
          <Condition Property="IsSelected" Value="True" />
          <Condition Property="RowContext.PreviousRelation" Value="SameGroup" />
          <Condition Property="RowContext.IsPreviousSelected" Value="True" />
        </MultiTrigger.Conditions>
        <Setter Target="Selection.CornerRadius" Value="0,0,8,8" />
      </MultiTrigger>
    </Grid.Triggers>

    <Border x:Name="Selection" Visibility="Collapsed" CornerRadius="8" />
    <ContentPresenter />
  </Grid>
</ControlTemplate>
```

# API Pages

| Type | Members |
|---|---|
| `Trigger : TriggerBase` | `Property` (string), `Value` (object), `Setters` (content property) |
| `MultiTrigger : TriggerBase` | `Conditions`, `Setters` (content property) |
| `Condition` | `Property` (string), `Value` (object) |
| `ConditionCollection` | A collection of `Condition` |

# API Details

```idl
namespace Microsoft.UI.Xaml
{
    [feature(Feature_ExperimentalApi)]
    runtimeclass Condition : DependencyObject
    {
        Condition();
        String Property;
        Object Value;
    }

    [feature(Feature_ExperimentalApi)]
    runtimeclass ConditionCollection : IVector<Condition> { ConditionCollection(); }

    [feature(Feature_ExperimentalApi)]
    [contentproperty("Setters")]
    runtimeclass Trigger : TriggerBase
    {
        Trigger();
        String Property;
        Object Value;
        SetterBaseCollection Setters{ get; };
    }

    [feature(Feature_ExperimentalApi)]
    [contentproperty("Setters")]
    runtimeclass MultiTrigger : TriggerBase
    {
        MultiTrigger();
        ConditionCollection Conditions{ get; };
        SetterBaseCollection Setters{ get; };
    }
}
```

# Open questions

- **Precedence.** Trigger setters are applied the same way as `VisualState` setters, so they
  override local values and `Style`, and can conflict with a `VisualState` or animation that
  sets the same property. A single precedence contract still has to be defined.
- **Placement.** The proposal puts triggers in `ControlTemplate.Triggers`. This proof of concept
  uses `FrameworkElement.Triggers` on the template root to avoid parser and XAML compiler
  changes. `ControlTemplate.Triggers` and `Style.Triggers` remain to be done.
- **Property types.** `Property` is a string resolved at run time, not a `DependencyProperty`
  checked against the template's `TargetType`. Conditions on string-valued properties are not
  compared yet.
- **No OR.** All conditions of a `MultiTrigger` must hold. Expressing "same group or other
  group" takes one trigger per combination.
- **Lifetime.** Conditions are tested only while the element is in the live tree.
- Whether `StateTrigger` and `AdaptiveTrigger` should share the condition evaluation.
