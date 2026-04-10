XamlBindingHelper
===

# Background

[`XamlBindingHelper`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.markup.xamlbindinghelper)
is a utility class in WinUI 3 that provides static helper methods for setting dependency property
values without boxing them to `IInspectable`. It already has overloads for primitive types such as
`Int32`, `Double`, `Boolean`, `String`, and several `Windows.Foundation` structs (`Point`, `Rect`,
`Size`, `TimeSpan`).

In WinUI 2, developers could use the
[`XamlDirect`](https://learn.microsoft.com/uwp/api/windows.ui.xaml.core.direct.xamldirect) API to
set struct-typed property values directly without boxing. For example `SetThicknessProperty`,
`SetCornerRadiusProperty`, and `SetColorProperty`. `XamlDirect` was intentionally not carried
forward to WinUI 3 because it is a narrow, low-level API with a limited number of consumers and
significant maintenance costs.

However, there is a real performance gap: developers who need to configure `Setter.Value` with
struct types - `Thickness`, `CornerRadius`, and `Windows.UI.Color` � currently have no option
other than boxing the value to `Object` first:

```cpp
// WinUI 3 � current approach (boxing required)
auto widthSetter = Setter();
widthSetter.Value(box_value(300));
```

This spec extends `XamlBindingHelper` with three new struct-typed overloads to close that gap:

- `SetPropertyFromThickness`
- `SetPropertyFromCornerRadius`
- `SetPropertyFromColor`

With these additions, developers can set struct-typed values on a `Setter` without any boxing:

```cpp
// WinUI 3 � new approach (no boxing)
XamlBindingHelper::SetPropertyFromThickness(
    widthSetter,
    Setter::ValueProperty(),
    thickness);
```

## Goals

* Provide boxing-free helpers for the three most commonly needed struct types when configuring `Setter.Value`.
* Follow the established `SetPropertyFrom*` naming pattern already present on `XamlBindingHelper`.

## Non-goals

* Re-introducing `XamlDirect` or any part of its API surface into WinUI 3.
* Adding `SetPropertyFrom*` overloads for every possible WinUI or Windows struct type.

# Conceptual pages (How To)

## Setting Setter.Value without boxing

Before these APIs, setting a struct-typed value on `Setter.Value` required boxing the value to
`Object` first. For example, setting a `Thickness` on a `Setter` looked like this:

```cpp
auto setter = Setter();
setter.Value(box_value(Thickness{ 2, 4, 2, 4 }));
```

With the new APIs, the struct can be passed directly and no boxing is needed in application code:

```cpp
auto setter = Setter();
Thickness thickness{ 2, 4, 2, 4 };
XamlBindingHelper::SetPropertyFromThickness(setter, Setter::ValueProperty(), thickness);
```

# API Pages

_(Each level-two section below maps to a docs.microsoft.com API page.)_

## XamlBindingHelper.SetPropertyFromThickness method

Sets a `Microsoft.UI.Xaml.Thickness` value on the specified dependency property of an object,
without requiring the caller to box the struct to `IInspectable`.

```idl
static void SetPropertyFromThickness(
    Object dependencyObject,
    Microsoft.UI.Xaml.DependencyProperty propertyToSet,
    Microsoft.UI.Xaml.Thickness value);
```

### Parameters

| Parameter | Type | Description |
|---|---|---|
| `dependencyObject` | `object` | The target object that owns `propertyToSet`. |
| `propertyToSet` | `DependencyProperty` | The dependency property to assign the value to. |
| `value` | `Microsoft.UI.Xaml.Thickness` | The `Thickness` value to set. |

### Examples

* C#:
```csharp
var thicknessSetter = new Setter();
var thickness = new Thickness();
thickness.Left = 2;
thickness.Top = 4;
thickness.Right = 2;
thickness.Bottom = 4;
XamlBindingHelper.SetPropertyFromThickness(thicknessSetter, Setter.ValueProperty, thickness);
DemoBorder.SetValue(Border.BorderThicknessProperty, thicknessSetter.Value);
```

* C++:
```cpp
auto thicknessSetter = Setter();
Thickness thickness{};
thickness.Left = 2;
thickness.Top = 4;
thickness.Right = 2;
thickness.Bottom = 4;
XamlBindingHelper::SetPropertyFromThickness(thicknessSetter, Setter::ValueProperty(), thickness);
DemoBorder().SetValue(Border::BorderThicknessProperty(), thicknessSetter.Value());
```

### Remarks

* If `dependencyObject` or `propertyToSet` is `null`, an exception is thrown by the WinRT layer from the underlying `E_POINTER` failure.

Sets a `Microsoft.UI.Xaml.CornerRadius` value on the specified dependency property of an object,
without requiring the caller to box the struct to `IInspectable`.

```idl
static void SetPropertyFromCornerRadius(
    Object dependencyObject,
    Microsoft.UI.Xaml.DependencyProperty propertyToSet,
    Microsoft.UI.Xaml.CornerRadius value);
```

### Parameters

| Parameter | Type | Description |
|---|---|---|
| `dependencyObject` | `object` | The target object that owns `propertyToSet`. |
| `propertyToSet` | `DependencyProperty` | The dependency property to assign the value to. |
| `value` | `Microsoft.UI.Xaml.CornerRadius` | The `CornerRadius` value to set. |

### Examples

* C#:
```csharp
var cornerRadiusSetter = new Setter();
var cornerRadius = new CornerRadius();
cornerRadius.TopLeft = 5;
cornerRadius.TopRight = 10;
cornerRadius.BottomRight = 15;
cornerRadius.BottomLeft = 20;
XamlBindingHelper.SetPropertyFromCornerRadius(cornerRadiusSetter, Setter.ValueProperty, cornerRadius);
DemoBorder.SetValue(Border.CornerRadiusProperty, cornerRadiusSetter.Value);
```

* C++:
```cpp
auto cornerRadiusSetter = Setter();
CornerRadius cornerRadius{};
cornerRadius.TopLeft = 5;
cornerRadius.TopRight = 10;
cornerRadius.BottomRight = 15;
cornerRadius.BottomLeft = 20;
XamlBindingHelper::SetPropertyFromCornerRadius(cornerRadiusSetter, Setter::ValueProperty(), cornerRadius);
DemoBorder().SetValue(Border::CornerRadiusProperty(), cornerRadiusSetter.Value());
```

### Remarks

* If `dependencyObject` or `propertyToSet` is `null`, an exception is thrown by the WinRT layer from the underlying `E_POINTER` failure.

## XamlBindingHelper.SetPropertyFromColor method

Sets a `Windows.UI.Color` value on the specified dependency property of an object, without
requiring the caller to box the struct to `IInspectable`.

```idl
static void SetPropertyFromColor(
    Object dependencyObject,
    Microsoft.UI.Xaml.DependencyProperty propertyToSet,
    Windows.UI.Color value);
```

### Parameters

| Parameter | Type | Description |
|---|---|---|
| `dependencyObject` | `object` | The target object that owns `propertyToSet`. |
| `propertyToSet` | `DependencyProperty` | The dependency property to assign the value to. |
| `value` | `Windows.UI.Color` | The `Color` value to set. |

### Examples

Setting `Setter.Value` to a color:

* C#:
```csharp
var colorSetter = new Setter();
XamlBindingHelper.SetPropertyFromColor(colorSetter, Setter.ValueProperty, Colors.BlueViolet);
DemoBorder.SetValue(
    Border.BorderBrushProperty,
    new SolidColorBrush((Windows.UI.Color)colorSetter.Value));
```

Setting `SolidColorBrush.ColorProperty` directly on a brush:

* C#:
```csharp
var brush = new SolidColorBrush();
XamlBindingHelper.SetPropertyFromColor(brush, SolidColorBrush.ColorProperty, Colors.BlueViolet);
DemoBorder.SetValue(Border.BorderBrushProperty, brush);
```

* C++:
```cpp
auto brush = SolidColorBrush();
XamlBindingHelper::SetPropertyFromColor(
    brush,
    SolidColorBrush::ColorProperty(),
    Colors::BlueViolet());
DemoBorder().SetValue(Border::BorderBrushProperty(), brush);
```

### Remarks

* If `dependencyObject` or `propertyToSet` is `null`, an exception is thrown by the WinRT layer from the underlying `E_POINTER` failure.

# API Details

```idl
namespace Microsoft.UI.Xaml.Markup
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    [default_interface]
    runtimeclass XamlBindingHelper
    {
        // Existing members omitted for brevity

        [contract(Microsoft.UI.Xaml.WinUIContract, 10)]
        {
            static void SetPropertyFromThickness(
                Object dependencyObject,
                Microsoft.UI.Xaml.DependencyProperty propertyToSet,
                Microsoft.UI.Xaml.Thickness value);

            static void SetPropertyFromCornerRadius(
                Object dependencyObject,
                Microsoft.UI.Xaml.DependencyProperty propertyToSet,
                Microsoft.UI.Xaml.CornerRadius value);

            static void SetPropertyFromColor(
                Object dependencyObject,
                Microsoft.UI.Xaml.DependencyProperty propertyToSet,
                Windows.UI.Color value);
        }
    };
}
```
