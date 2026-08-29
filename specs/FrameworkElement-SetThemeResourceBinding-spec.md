FrameworkElement.SetThemeResourceBinding
===

# Background

XAML's [`{ThemeResource}`](https://learn.microsoft.com/windows/uwp/xaml-platform/themeresource-markup-extension)
markup extension creates a live binding from a dependency property to a keyed resource, and
updates that resource to match the effective theme whenever the app's theme or high-contrast setting changes.
It differs from [`{StaticResource}`](https://learn.microsoft.com/windows/uwp/xaml-platform/staticresource-markup-extension),
which resolves the resource once and never updates.

```xml
<Grid Background="{ThemeResource ApplicationPageBackgroundThemeBrush}" />
```

Today `{ThemeResource}` is available only in markup. There is no supported way to establish a
ThemeResource binding from code. Developers who build UI in code, or who need to
(re)wire a theme resource after load, currently have to either:

* re-implement theme tracking by hand by listening for `FrameworkElement.ActualThemeChanged` and
  re-querying `ResourceDictionary` on every change.
* implement a workaround with constructing equivalent markup and using `XamlReader.Load`

This spec adds an API that establishes the same live ThemeResource binding that `{ThemeResource}`
creates in markup, using the existing internal resolution and theme-tracking engine. It follows
the precedent of [`FrameworkElement.SetBinding`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.frameworkelement.setbinding),
which installs a live expression on the target property.

This API is being added to `FrameworkElement` because resolving a resource key requires an element to
define the ambient resource scope (the chain of `FrameworkElement.Resources` up to `Application.Resources`).
`FrameworkElement` is the lowest type in the hierarchy that carries a `Resources` dictionary, so it
is the natural anchor.

Note that this means the API does not apply to `Setter` objects, because they don't derive from
`FrameworkElement`.

# API Pages

## FrameworkElement.SetThemeResourceBinding(DependencyProperty, String) method

Establishes a live theme resource binding on the given dependency property, equivalent to setting
`{ThemeResource key}` on that property in markup.

```cs
public void SetThemeResourceBinding(DependencyProperty property, string resourceKey)
```

### Parameters

`property` [DependencyProperty](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.dependencyproperty)

The dependency property identifier of the property on which to establish the theme resource
binding. Attached dependency properties are supported; read-only dependency properties are not.

`resourceKey` [String](https://learn.microsoft.com/dotnet/api/system.string)

The key of the theme resource to resolve. The key is looked up in the ambient
`ResourceDictionary` scope, walking up from this element through each ancestor's `Resources`,
then the theme dictionaries, then `Application.Resources`.

### Exceptions

| Exception | Condition |
|---|---|
| [ArgumentException](https://learn.microsoft.com/dotnet/api/system.argumentexception) | `resourceKey` cannot be resolved in the element's current resource scope (matches markup, which fails the parse with `AG_E_PARSER_FAILED_RESOURCE_FIND`), or the resolved value is not assignable to `property`. |

### Remarks

The resource key is resolved immediately against the element's current position in the tree. The
bound value is automatically updated to match the effective theme or high-contrast setting when
it changes, and re-resolved if the element is later moved to a new location in the live tree.

Call this method after the element has been placed into the tree where the resource is available.
The key is resolved at call time by walking up from this element through the ambient `ResourceDictionary`
scope (each ancestor's `Resources`, then the theme dictionaries, then `Application.Resources`).

If the key cannot be resolved, or the resolved value is not assignable to `property`, this method
throws to match markup behavior, where an unresolvable `{ThemeResource}` fails the parse
(`AG_E_PARSER_FAILED_RESOURCE_FIND`).

The binding is installed at local value precedence, identical to setting the property directly
in code. As with any binding:

* Subsequently calling `SetValue(property, ...)` or setting the property replaces the
  binding with that local value (the theme binding is removed).
* Calling `ClearValue(property)` removes the binding and restores the property's default value.

Setting a new theme resource binding on a property that already has a theme resource binding replaces
the previous one. Do not mix theme resource bindings with other types of bindings (e.g. classic `Binding`
or `x:Bind`) on the same property.

> **Note:** This binding tracks theme and high-contrast changes, updating the value to match the new
> theme. It also re-resolves when the element is moved to a new location in the live tree, falling
> back to the value from the resource scope that was initially captured when the binding was installed
> if the key cannot be resolved in the new location. Merely adding a matching resource to an already-in-scope
> dictionary does not, trigger re-resolution. This matches the behavior of `{ThemeResource}` in markup.

# Examples

## Example: Setting a property to a ThemeResource in code

Set a theme-tracking background from code, equivalent to the markup at the top of this spec:

```cs
myGrid.SetThemeResourceBinding(
    Grid.BackgroundProperty,
    "ApplicationPageBackgroundThemeBrush");
```

## Example: Clearing a ThemeResource binding from a property

After calling `SetThemeResourceBinding`, a call to `ClearValue` will clear the binding:

```cs
textBlock.SetThemeResourceBinding(TextBlock.ForegroundProperty, "SystemControlForegroundBaseHighBrush");

// ...later, remove the ThemeResource and revert to the default:
textBlock.ClearValue(TextBlock.ForegroundProperty);
```

## Example: Overwriting a ThemeResource binding

A subsequent local set will replace the binding:

```cs
textBlock.SetThemeResourceBinding(TextBlock.ForegroundProperty, "SystemControlForegroundBaseHighBrush");

// ...later, replace the ThemeResource with another value:
textBlock.Foreground = new SolidColorBrush(Colors.Red);
```

## Example: Overwriting a ThemeResource binding with a new ThemeResource binding

A subsequent SetThemeResourceBinding will replace the existing binding:

```cs
textBlock.SetThemeResourceBinding(TextBlock.ForegroundProperty, "SystemControlForegroundBaseHighBrush");

// ...later, replace the ThemeResource with another ThemeResource:
textBlock.SetThemeResourceBinding(TextBlock.ForegroundProperty, "SystemControlForegroundBaseLowBrush");
```

# API Details

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml
{
    [webhosthidden]
    unsealed runtimeclass FrameworkElement : Microsoft.UI.Xaml.UIElement
    {
        // ...existing members...

        /// Establish a live {ThemeResource}-equivalent binding on 'property'. The resource key is
        /// resolved immediately against the element's current position in the tree.
        /// @param property The DependencyProperty on which to establish the binding. Cannot be a read-only property.
        /// @param resourceKey The key of the theme resource to resolve.
        /// @throw If 'resourceKey' cannot be resolved, or the resolved value is not assignable to 'property'.
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        [feature(Feature_ExperimentalApi)]
        void SetThemeResourceBinding(Microsoft.UI.Xaml.DependencyProperty property, String resourceKey);
    }
}
```

# Appendix

## Relationship to the existing markup path

This API connects to the existing ThemeResource mechanism:

* **Resolution** reuses the runtime resolver already used by Hot Reload / Live Visual Tree to apply
  theme bindings to arbitrary elements from code with no parser context. It reconstructs the
  ambient resource scope by walking the live element tree.
* **Binding install + theme tracking** reuses the same internal path as the parser, so a ThemeResource
  set from code is identical to a set from markup: same effective-value slot, same precedence, same
  re-resolution, and visible/editable in diagnostics tooling.

## Behavior summary

The following behaviors are inherited from the existing `{ThemeResource}` mechanism:

| Aspect | Behavior |
|---|---|
| Resolution timing | Eager, at call time, against the element's current tree location. |
| Re-resolution | Updated to the matching theme / high-contrast value on change; fully re-resolved on reparent into a new live scope. |
| Target property | Dependency properties only (including attached DPs); read-only DPs rejected. |
| Clearing / overriding | A later `SetValue` or `ClearValue` removes the binding; no dedicated clear API. |
| Missing key | Throws (matches markup, which fails the parse with `AG_E_PARSER_FAILED_RESOURCE_FIND`). |
| Type mismatch | Resolved value must be assignable to the property; no type-converter coercion. `{x:Null}`-keyed resource clears to null. |
| Precedence | Installed at local `BaseValueSource` (overrides Style; overridden by animation). |
| Threading | Must be called on the target's UI/dispatcher thread. |
| Object identity | The resolved object is **shared** across all bindings to the same key (no clone). Callers must not mutate it in place. |
