# Lightweight Bindings

This document describes the lightweight binding infrastructure added to optimize simple property bindings.

## Overview

The standard XAML `Binding` and `BindingExpression` infrastructure is powerful but heavyweight. It involves:
- `PropertyPath` parsing and object creation
- `Binding` object with many optional properties (Mode, Converter, FallbackValue, etc.)
- `BindingOperations` infrastructure
- Complex property path listener chains for multi-level paths
- Support for DataContext inheritance, RelativeSource, etc.

For simple internal bindings where the source object and property are known at compile time, this overhead is unnecessary.

## DirectSourceBindingExpression

`DirectSourceBindingExpression` is a lightweight alternative for simple single-property bindings with an explicit source object. It's designed for internal framework use where:
- The source is an explicit `DependencyObject` (not DataContext-based)
- The path is a single property (no multi-level paths like `Foo.Bar`)
- Only OneWay binding mode is needed
- Optionally, a value converter can be applied

### Benefits

| Aspect | Full Binding | DirectSourceBindingExpression |
|--------|--------------|-------------------------------|
| Objects created | 3+ (PropertyPath, Binding, BindingExpression) | 1 |
| Path parsing | Yes (runtime string parsing) | No (compile-time KnownPropertyIndex) |
| DataContext support | Yes | No |
| RelativeSource support | Yes | No |
| Multi-level paths | Yes | No |
| TwoWay binding | Yes | No |
| Value converter | Yes | Yes |

### Usage

Use `DXamlCore::SetDirectBinding` instead of `DXamlCore::SetBinding`:

```cpp
// Old approach - creates PropertyPath, Binding, BindingExpression
IFC_RETURN(DXamlCore::SetBinding(
    ctl::as_iinspectable(uiCommand),
    wrl_wrappers::HStringReference(L"Label").Get(),
    target,
    KnownPropertyIndex::AppBarButton_Label));

// New approach - creates only DirectSourceBindingExpression
auto sourceDO = ctl::query_interface_cast<DependencyObject>(uiCommand);
if (sourceDO)
{
    IFC_RETURN(DXamlCore::SetDirectBinding(
        sourceDO.Get(),
        KnownPropertyIndex::XamlUICommand_Label,
        target,
        KnownPropertyIndex::AppBarButton_Label));
}
```

With a converter:

```cpp
ctl::ComPtr<MyConverter> converter;
IFC_RETURN(ctl::make(&converter));
IFC_RETURN(DXamlCore::SetDirectBinding(
    sourceDO.Get(),
    KnownPropertyIndex::XamlUICommand_IconSource,
    target,
    KnownPropertyIndex::AppBarButton_Icon,
    converter.Get()));
```

### Implementation Details

`DirectSourceBindingExpression` derives from `BindingExpressionBase` and:

1. **Holds a weak reference to the source** - Prevents reference cycles
2. **Registers as `IDPChangedEventHandler`** - Listens for property changes on the source
3. **Directly reads the source property** - No property path traversal needed
4. **Optionally applies a converter** - Supports `IValueConverter` for type transformation
5. **Refreshes on source property change** - Calls `RefreshExpression` when the watched property changes

### Files

- `dxaml/xcp/dxaml/lib/DirectSourceBindingExpression.h` - Header
- `dxaml/xcp/dxaml/lib/DirectSourceBindingExpression.cpp` - Implementation

## When to Use Each

| Scenario | Recommended Approach |
|----------|---------------------|
| App developer binding in XAML | Standard `{Binding}` or `{x:Bind}` |
| Framework internal binding, explicit source, single property | `DirectSourceBindingExpression` |
| Complex paths, DataContext, TwoWay, etc. | Standard `Binding`/`BindingExpression` |

## Example: CommandingHelpers

The `CommandingHelpers` class was updated to use `DirectSourceBindingExpression` for binding `XamlUICommand` properties to control properties:

- `BindToLabelPropertyIfUnset` - Binds `XamlUICommand.Label` to target's label property
- `BindToIconPropertyIfUnset` - Binds `XamlUICommand.IconSource` with converter
- `BindToAccessKeyIfUnset` - Binds `XamlUICommand.AccessKey`
- `BindToDescriptionPropertiesIfUnset` - Binds `XamlUICommand.Description`
- `BindToKeyboardAcceleratorsIfUnset` - Binds keyboard accelerators with converter

These bindings are set up when a control's `Command` property is set to an `XamlUICommand`, enabling the command to provide default Label, Icon, AccessKey, etc. values.
