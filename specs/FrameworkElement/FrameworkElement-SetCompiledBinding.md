FrameworkElement.SetCompiledBinding
===

# Background

WinUI supports binding a dependency property to an object's data through
[`{Binding}`][data-binding] and [`{x:Bind}`][binding-comparison].
In markup, `{x:Bind}` generates code at build time to read the source value. This avoids the
runtime property-path lookup used by `{Binding}` and allows the binding logic to be checked by the
compiler.

Developers who build UI in code can call [`FrameworkElement.SetBinding`][set-binding] with a
`Binding` object. However, there has been no equivalent API that accepts app-compiled code for
reading the source. Developers instead have to describe the source property with a string:

```csharp
textBlock.SetBinding(
    TextBlock.TextProperty,
    new Binding { Path = new PropertyPath(nameof(ItemViewModel.DisplayName)) });
```

Building `Binding` objects from code also has problems with IL trimming. Classic `Binding`s use
reflection and metadata to get values from the source object, and the required metadata may be
removed by the trimmer. There is the `GeneratedBindableCustomProperty` attribute as the supported
mitigation.

This feature adds `FrameworkElement.SetCompiledBinding`. The app supplies a
`CompiledBindingGetter` delegate that receives the element's effective `DataContext` and returns
the value for the target dependency property:

```csharp
textBlock.SetCompiledBinding(
    TextBlock.TextProperty,
    source => ((ItemViewModel)source).DisplayName);
```

The lambda is compiled by the app's normal language toolchain. WinUI invokes it initially and
again when the source raises `INotifyPropertyChanged`. This provides a convenient, compiler-checked
binding for code-authored UI without constructing a property path or invoking the XAML compiler.
It's also safe under IL trimming without any additional attributes (provided the lambda itself
doesn't use reflection).

This API is intentionally smaller than either `{Binding}` or `{x:Bind}`. It is a OneWay binding
from the element's `DataContext`, represented by one getter delegate. It does not provide binding
modes, converters, fallback values, source selection, or generated subscriptions to individual
properties.

> This API is **experimental** (gated behind `Feature_ExperimentalApi`) while the design is
> finalized.

# Conceptual pages (How To)

## Creating a compiled binding in code

Call `SetCompiledBinding` on a `FrameworkElement`, passing the target dependency property and a
getter. The getter receives the element's effective `DataContext`, including a `DataContext`
inherited from an ancestor.

```csharp
class ItemViewModel : INotifyPropertyChanged
{
    private string _displayName;

    public string DisplayName
    {
        get => _displayName;
        set
        {
            if (_displayName != value)
            {
                _displayName = value;
                PropertyChanged?.Invoke(
                    this,
                    new PropertyChangedEventArgs(nameof(DisplayName)));
            }
        }
    }

    public event PropertyChangedEventHandler PropertyChanged;
}

var viewModel = new ItemViewModel { DisplayName = "Initial value" };
var textBlock = new TextBlock { DataContext = viewModel };

textBlock.SetCompiledBinding(
    TextBlock.TextProperty,
    source => ((ItemViewModel)source).DisplayName);
```

The binding evaluates immediately. If the current `DataContext` implements
`INotifyPropertyChanged`, the binding reevaluates the getter for every `PropertyChanged`
notification. Because the getter is opaque to WinUI, WinUI cannot determine which source
properties it reads and does not filter notifications by property name.

If the effective `DataContext` changes, the binding stops listening to the old object, starts
listening to the new object, and reevaluates. This includes changes to an inherited `DataContext`.

If the effective `DataContext` is `null`, WinUI does not invoke the getter. The target property
uses its default value until a non-null `DataContext` becomes available.

The getter must only read from its source and return a value. It must not change the target
element's `DataContext` during evaluation. Doing so causes the evaluation to fail with an invalid
operation error.

# Examples

## Example: Update a target through INotifyPropertyChanged

The getter can contain ordinary compiled code rather than a property-path string:

```csharp

class ItemViewModel : INotifyPropertyChanged
{
    // omitted: FirstName and LastName properties that raise PropertyChanged
}

var viewModel = new ItemViewModel
{
    FirstName = "First",
    LastName = "Last",
};

var textBlock = new TextBlock { DataContext = viewModel };
textBlock.SetCompiledBinding(
    TextBlock.TextProperty,
    source =>
    {
        var item = (ItemViewModel)source;
        return $"{item.FirstName} {item.LastName}";
    });

viewModel.FirstName = "Updated";
// TextBlock.Text is now "Updated Last" after ItemViewModel raises PropertyChanged.
```

The getter runs for every notification, so either `FirstName` or `LastName` can cause the
computed value to update.

## Example: Use a compiled binding in a code-authored DataTemplate

A code-authored `DataTemplate` creates its elements before the hosting control assigns their
`DataContext`. You can install the binding while creating the element; it displays the dependency
property's default value until the control supplies the data item.

```csharp

class ItemViewModel : INotifyPropertyChanged
{
    // omitted: DisplayName property that raises PropertyChanged

    // If this type doesn't implement INotifyPropertyChanged or doesn't raise PropertyChanged,
    // the binding will only evaluate when the DataContext changes.
}

var items = new[]
{
    new ItemViewModel { DisplayName = "Item 1" },
    new ItemViewModel { DisplayName = "Item 2" },
};

var template = new DataTemplate(() =>
{
    var textBlock = new TextBlock();
    textBlock.SetCompiledBinding(
        TextBlock.TextProperty,
        source => ((ItemViewModel)source).DisplayName);
    return textBlock;
});

var repeater = new ItemsRepeater
{
    ItemsSource = items,
    ItemTemplate = template,
};
```

When an element is recycled for a different item, its `DataContext` changes and the compiled
binding follows the new item.

## Example: Replace or clear a compiled binding

Like other local expressions, a compiled binding is replaced by a subsequent local value:

```csharp
textBlock.SetCompiledBinding(
    TextBlock.TextProperty,
    source => ((ItemViewModel)source).DisplayName);

// Replace and remove the compiled binding.
textBlock.Text = "Fixed value";
```

Call `ClearValue` to remove the binding and restore the next value determined by dependency
property precedence:

```csharp
textBlock.ClearValue(TextBlock.TextProperty);
```

# API Pages

## FrameworkElement.SetCompiledBinding(DependencyProperty, CompiledBindingGetter) method

Establishes a OneWay binding that invokes an app-supplied getter against the element's effective
`DataContext`.

```csharp
public void SetCompiledBinding(
    DependencyProperty dp,
    CompiledBindingGetter getter);
```

### Parameters

`dp` [DependencyProperty][dependency-property]

The dependency property identifier of the target property.

`getter` [CompiledBindingGetter](#compiledbindinggetter-delegate)

The app-supplied function that receives the effective `DataContext` and returns the target value.

### Exceptions

| Exception | Condition |
|---|---|
| [ArgumentNullException][argument-null] | `dp` or `getter` is `null`. |
| [ArgumentException][argument] | The value returned by `getter` is not valid for `dp`. |
| [InvalidOperationException][invalid-operation] | `getter` changes this element's `DataContext` while the getter is being evaluated. |

Exceptions raised by app code in `getter` propagate to the caller that caused the binding to
evaluate.

### Remarks

The binding source is this element's effective `DataContext`. A locally set `DataContext` takes
precedence over an inherited `DataContext`. When the effective `DataContext` changes, the getter
is reevaluated against the new source.

When the source implements `INotifyPropertyChanged`, the getter is reevaluated for every
`PropertyChanged` notification, regardless of the reported property name. When the source does not
implement `INotifyPropertyChanged`, the getter is evaluated when a non-null effective `DataContext`
is available. Changes within the source are not observed, but changes in the effective `DataContext`
reevaluate the binding. The getter runs only when the new DataContext is non-null, otherwise the
binding returns the default value of the property.

When the effective `DataContext` is `null`, the getter is not invoked and the target property's
default value is used. The getter is invoked when a non-null `DataContext` later becomes effective.

The binding is installed at local value precedence. Setting another binding or local value on
`dp` replaces the compiled binding. Calling `ClearValue(dp)` removes it. Because this is an opaque
binding expression rather than a classic `BindingExpression`,
`FrameworkElement.GetBindingExpression(dp)` returns `null`.

This API creates a OneWay binding. There is no write-back path to the source.

## CompiledBindingGetter delegate

Represents the method that reads a value from a programmatic compiled binding's source.

```csharp
public delegate object CompiledBindingGetter(object source);
```

The `source` parameter is the target element's non-null effective `DataContext`. Cast it to the
expected app type and return the value to assign to the target dependency property.

WinUI keeps the delegate alive for the lifetime of the binding. The delegate may capture app
state, but it should not capture the target element because doing so can create a reference cycle.

The getter must not change the target element's `DataContext` while it is running.

# API Details

```csharp (but really MIDL3)
namespace Microsoft.UI.Xaml.Data
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [feature(Feature_ExperimentalApi)]
    /// Represents the method that reads a value from a programmatic compiled binding's source.
    /// @param source The target element's effective DataContext.
    /// @return The value to assign to the target dependency property.
    delegate Object CompiledBindingGetter(Object source);
}

namespace Microsoft.UI.Xaml
{
    [webhosthidden]
    unsealed runtimeclass FrameworkElement : Microsoft.UI.Xaml.UIElement
    {
        // ...existing members...

        /// Establishes a OneWay binding that invokes an app-supplied getter against this
        /// element's effective DataContext.
        /// @param dp The dependency property on which to establish the binding.
        /// @param getter The function that reads the target value from the effective DataContext.
        /// @throw If dp or getter is null, the getter returns a value that is invalid for dp,
        ///        or the getter changes this element's DataContext during evaluation.
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        [feature(Feature_ExperimentalApi)]
        void SetCompiledBinding(
            Microsoft.UI.Xaml.DependencyProperty dp,
            Microsoft.UI.Xaml.Data.CompiledBindingGetter getter);
    }
}
```

# Appendix

## Relationship to Binding and x:Bind

| Aspect | `SetBinding` / `{Binding}` | `SetCompiledBinding` | `{x:Bind}` |
|---|---|---|---|
| Source expression | Runtime `PropertyPath` | App-supplied delegate | XAML-compiler-generated code |
| Default source | `DataContext` | Effective `DataContext` | Markup page or data-template item |
| Mode | OneTime, OneWay, or TwoWay | OneWay | OneTime, OneWay, or TwoWay |
| Change tracking | Selected path notifications | Every source INPC notification | Generated subscriptions |
| Converter support | Yes | Express conversion in the getter | Functions or converters |
| Available from code | Yes | Yes | No; authored in markup |
| `GetBindingExpression` | Returns a `BindingExpression` | Returns `null` | Returns `null` |

`SetCompiledBinding` is called "compiled" because the app supplies executable code that its
language compiler type-checks and compiles. The API does not parse an `x:Bind` expression and does
not invoke the XAML compiler.

## Behavior summary

| Aspect | Behavior |
|---|---|
| Source | The target element's effective local or inherited `DataContext`. |
| Initial evaluation | Immediate when `DataContext` is non-null; otherwise the target uses its default. |
| Source replacement | Follows effective `DataContext` changes and stops listening to the old source. |
| Source notification | Reevaluates for every `INotifyPropertyChanged.PropertyChanged` event. |
| Source without INPC | Evaluates initially and on `DataContext` changes only. |
| Binding mode | OneWay. |
| Precedence | Installed as a local expression. |
| Clearing or overriding | A later local value or binding replaces it; `ClearValue` removes it. |
| Getter failure | The error propagates to the operation that caused evaluation. |
| Getter side effects | Changing the target's `DataContext` during evaluation is invalid. |
| Threading | Must be called on the target element's UI thread. |

[argument]: https://learn.microsoft.com/dotnet/api/system.argumentexception
[argument-null]: https://learn.microsoft.com/dotnet/api/system.argumentnullexception
[binding-comparison]: https://learn.microsoft.com/windows/apps/develop/data-binding/data-binding-in-depth#xbind-and-binding-feature-comparison
[data-binding]: https://learn.microsoft.com/windows/apps/develop/data-binding/data-binding-in-depth
[dependency-property]: https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.dependencyproperty
[invalid-operation]: https://learn.microsoft.com/dotnet/api/system.invalidoperationexception
[set-binding]: https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.frameworkelement.setbinding
