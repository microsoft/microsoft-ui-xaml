Setter.ValueProperty
===

# Background

[`Setter`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.setter)
is a WinUI 3 class that associates a dependency property with a value inside a `Style`. The
`Setter.Value` property holds the value that will be applied, but until now there has been no
public `DependencyProperty` identifier exposed for it. This means setting values on a `Setter`
requires boxing the value to `Object` first:

```cpp
// WinUI 3 — current approach (boxing required)
auto widthSetter = Setter();
widthSetter.Value(box_value(300));
```

This spec exposes the `DependencyProperty` identifier for `Setter.Value` through a new static
property, `Setter.ValueProperty`. This unblocks the usage of the `Setter` class with
`XamlBindingHelper.SetPropertyFrom*` APIs, which require a `DependencyProperty` as their second
argument, allowing developers to set values without boxing:

```cpp
// WinUI 3 — new approach (no boxing)
XamlBindingHelper::SetPropertyFromInt32(
    widthSetter,
    Setter::ValueProperty(),
    300);
```

## Goals

* Expose the `DependencyProperty` identifier for `Setter.Value`.
* Follow the established pattern of other WinUI 3 classes that expose `DependencyProperty`
  identifiers for their properties.

## Non-goals

* Changing the behavior or storage of `Setter.Value` itself.
* Adding new overloads to `XamlBindingHelper` (covered in a separate spec).

# Conceptual pages (How To)

## Using Setter.ValueProperty with XamlBindingHelper

Previously there was no public way to obtain a `DependencyProperty` token for `Setter.Value`. With
`Setter.ValueProperty` exposed, this is now possible:

```cpp
auto setter = Setter();
// Set an Int32 value without boxing
XamlBindingHelper::SetPropertyFromInt32(setter, Setter::ValueProperty(), 300);
```

# API Pages

## Setter.ValueProperty property

Gets the `DependencyProperty` identifier for the
[`Setter.Value`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.setter.value)
property.

```idl
static Microsoft.UI.Xaml.DependencyProperty ValueProperty{ get; };
```

### Remarks

* `ValueProperty` is a read-only static property that returns a singleton `DependencyProperty`
  instance.

### Examples

* C#:
```csharp
var setter = new Setter();
// Use any existing SetPropertyFrom* overload with Setter.ValueProperty
XamlBindingHelper.SetPropertyFromDouble(setter, Setter.ValueProperty, 16.0);
```

* C++:
```cpp
auto setter = Setter();
// Use any existing SetPropertyFrom* overload with Setter::ValueProperty()
XamlBindingHelper::SetPropertyFromDouble(setter, Setter::ValueProperty(), 16.0);
```

# API Details

```idl
namespace Microsoft.UI.Xaml
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    runtimeclass Setter : Microsoft.UI.Xaml.SetterBase
    {
        // Existing members omitted for brevity

        [contract(Microsoft.UI.Xaml.WinUIContract, 10)]
        [static_name("Microsoft.UI.Xaml.ISetterStatics2")]
        {
            static Microsoft.UI.Xaml.DependencyProperty ValueProperty{ get; };
        }
    };
}
```
