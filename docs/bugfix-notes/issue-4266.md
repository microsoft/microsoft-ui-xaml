# Issue #4266: Binding to bool shows Windows.Foundation.IReference`1<Boolean>

## The bug

A `{Binding}` from a `bool` to a string property (like `TextBlock.Text` or `Run.Text`) shows `Windows.Foundation.IReference`1<Boolean>` instead of `True` or `False`.

In the report, a `Run` and a CheckBox are both bound to a `bool` dependency property on a custom control. The CheckBox binding is TwoWay. The text is right at first. It goes wrong after the user clicks the CheckBox.

## Root cause

When the target is a string, `DefaultValueConverter::CreateConverter` picks a converter by source type. Numbers, `Uri` and `Guid` get their own converters. Everything else, `Boolean` included, gets `StringValueConverter` (dxaml\xcp\dxaml\lib\DefaultValueConverter.cpp:46-63).

`StringValueConverter::Convert` (DefaultValueConverter.cpp:728) only knows two ways to make a string:

1. If the object has `ICustomPropertyProvider`, call `GetStringRepresentation` (:741-745).
2. Otherwise, return `GetRuntimeClassName` (:771).

So the result depends on which object holds the `bool`:

| Box | Has ICPP? | Shows |
|-----|-----------|-------|
| The first value in the report | Yes (inferred, not checked: it shows `True`, and the only other branch prints the class name) | `True` |
| A box made by WinUI, e.g. `ToggleButton.IsChecked` after a click | No | `Windows.Foundation.IReference`1<Boolean>` |

WinUI boxes bools with `PropertyValue::CreateFromBoolean` (dxaml\xcp\components\valueboxer\lib\CValueBoxer.cpp:2428-2432). That returns a cached `Windows.Foundation.PropertyValue` box (dxaml\xcp\components\valueboxer\lib\Value.cpp:384-390, dxaml\xcp\dxaml\lib\StaticStore.cpp:66-72 and :127-138). That OS box has no ICPP.

In the report, the CheckBox click writes the WinUI box back to the dependency property through the TwoWay binding. The dependency property stores that box as-is. The `Run` then reads it, and falls into the class-name branch.

## The fix

In `StringValueConverter::Convert`, when there is no ICPP, check whether the source is an `IPropertyValue` whose type is `PropertyType_Boolean`. If so, return `True` or `False` (DefaultValueConverter.cpp:748-772). Everything else still returns the class name.

`True`/`False` is the same text .NET's `Boolean.ToString()` gives, so a C# app now sees the same text no matter which object holds the `bool`.

## Why this fixes it

- The box in the bug is an OS `PropertyValue` boolean box (StaticStore.cpp:66-72). It implements `IPropertyValue` with type `PropertyType_Boolean`, so it now takes the new branch.
- The new test hits exactly this box two ways: `CheckBox.IsChecked` (ToggleButton's getter re-boxes through `CValueBoxer`) and `PropertyValue::CreateBoolean` as a `DataContext`.
- Without the fix, the test fails at the first check: `Values (True, Windows.Foundation.IReference`1<Boolean>)`. That is the exact text from the issue.

## Why it's safe

- **Narrow trigger.** The new code only runs when the source has no ICPP (:742) AND is an `IPropertyValue` (:754) AND its type is Boolean (:758). Before, that case always returned the class name, which no one wants to see in UI.
- **Other types don't change.** Every other source still reaches `GetRuntimeClassName` (:771). The test checks that a `Point` box still shows `Windows.Foundation.IReference`1<Windows.Foundation.Point>`.
- **ICPP boxes don't change.** The ICPP branch (:741-745) runs first, as before. So C# apps that already show `True` see no change.
- **Who sees the change.** `StringValueConverter` is created only at DefaultValueConverter.cpp:62, for string targets. But `DynamicValueConverter` reaches it from more than `{Binding}`:
  - `XamlBindingHelper.ConvertValue(typeof(String), <bool box>)` (dxaml\xcp\dxaml\lib\XamlBindingHelperFactory_Partial.cpp:47-52) now returns `True`/`False` instead of the class name. This is a public API change, and it is intended: it is the same conversion. The test checks it. x:Bind's generated code only passes strings into `ConvertValue`, so x:Bind output does not change.
  - Any non-ICPP `IPropertyValue` that reports `PropertyType_Boolean` changes, including one an app implements itself. That is the point of the fix.
- **`ConvertValueToString` is not touched.** That helper prints a bool as `1`/`0` (:952-960). It is used by automation (dxaml\xcp\dxaml\lib\FrameworkElement_partial.cpp:506-508) and diagnostics. Reusing it here would have shown `1`/`0` in bindings, and changing it would change those other callers.
- **Only the forward direction changes.** For a TwoWay binding, `DynamicValueConverter::ConvertBack` asks for the same (Boolean, String) converter (dxaml\xcp\dxaml\lib\DynamicValueConverter.cpp:124), and `StringValueConverter::ConvertBack` still returns E_FAIL (dxaml\xcp\dxaml\lib\DefaultValueConverter.h:474-481). So string-to-bool write-back fails the same way it did before.
- **Errors.** `get_Type` and `GetBoolean` failures propagate with `IFC`, the same as `ConvertHelper` does with `get_Type` (:116). A box that says it is Boolean and then fails `GetBoolean` would be broken anyway.

## Test

`BindingIntegrationTests::CanBindBooleanBoxToString` (dxaml\test\native\external\framework\dataBinding\tests\BindingIntegrationTests.cpp:375-415):

| Case | Expect |
|------|--------|
| `TextBox.Text` OneWay-bound to `CheckBox.IsChecked`, set `true` | `True` |
| Same, set `false` | `False` |
| `TextBlock.Text="{Binding}"`, `DataContext = PropertyValue::CreateBoolean(true)` | `True` |
| Same with `false` | `False` |
| Same with `PropertyValue::CreatePoint(1, 2)` | Class name, unchanged |
| `XamlBindingHelper::ConvertValue(String, PropertyValue::CreateBoolean(true))` | `True` |

Results (VM, amd64chk, `-HostingMode WPF`):

- With the fix: `*BindingIntegrationTests*` 80/80 pass.
- Negative check (MUX built without the product change): the new test fails with `Values (True, Windows.Foundation.IReference`1<Boolean>)`.

## Out of scope

- **Enums.** A comment on the issue says a `List<VirtualKey>` shows `IReference`1<Windows.System.VirtualKey>` per item. That is the ItemsControl default item text, not a `{Binding}` to a string, and turning an enum value into its name needs metadata lookups. Not changed here.
- **x:Bind.** One comment hits a compile error (WMC1121) with `x:Bind` to `IReference<bool>`. That is the XAML compiler, not the runtime converter. Another comment says `IsEnabled="{x:Bind ViewModel.IsEnabled}"` shows the class name after an update. `IsEnabled` is a `bool` target with no text, so that comment is missing details, and there is no repro. Not changed here.
- **Other non-ICPP boxes** (`UInt8`, `DateTime`, `TimeSpan`, `Point`, ...). `UInt8` is not in `IsNumericType`, so it also lands here. They still show the class name. There is no single obvious text for them, and the bug is about `bool`.
