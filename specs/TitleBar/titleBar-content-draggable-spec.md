TitleBar Drag Region API Specification
===

# Background

Custom title bar layouts often combine **interactive controls** and **non‑interactive visual elements** using containers such as `Grid`, `StackPanel`, or deeper nested structures. While this flexibility enables rich and branded title bar designs, it also introduces ambiguity when determining **which parts of the title bar should behave as draggable regions** for moving the window.

Under the **current default behavior**, the framework treats the entire `TitleBar.Content` area as the primary drag surface and then subtracts (or *"punches holes"* from) regions that should not initiate window dragging. This approach works reasonably well for dense, predictable layouts. However, it **fails in scenarios with empty gaps, uneven spacing, nested templates, or dynamically generated UI**, where the system cannot reliably infer developer intent. These situations can lead to **unexpected non‑draggable gaps**, creating inconsistent or unintuitive window‑drag behavior for users.

This problem has been raised and discussed by developers in the WinUI community, for example in: **[#10421](https://github.com/microsoft/microsoft-ui-xaml/issues/10421)**.

This specification evaluates multiple approaches to defining draggable and non‑draggable regions within `TitleBar.Content`, and proposes a solution that balances **predictable defaults** with **developer‑provided intent**.

---

# Conceptual pages (How To)

#### Problem example: gaps become non-draggable

```xml
<TitleBar Title="Main Titlte" Subtitle="subtitle" x:Name="titleBar">
    <TitleBar.Content>
        <Grid>
            <Grid.ColumnDefinitions>
                <ColumnDefinition Width="150" />
                <ColumnDefinition Width="200" />
                <ColumnDefinition Width="50" />
            </Grid.ColumnDefinitions>
            <Border Grid.Column="0" Background="LightBlue" BorderBrush="Black" BorderThickness="1">
                <AutoSuggestBox PlaceholderText="Search"/>
            </Border>
            <Border Grid.Column="1" />
            <Border Grid.Column="2" Background="LightCoral" BorderBrush="Black" BorderThickness="1">
                <TextBlock Text="Help" VerticalAlignment="Center" HorizontalAlignment="Center" />
            </Border>
        </Grid>
    </TitleBar.Content>
</TitleBar>
```

#### Output:
![Non draggable gaps in TitleBar Content](./images/titlebar-drag-issue.png)

In this simple layout:
- Column 0 contains **Sample Search Box**
- Column 2 contains **Help**
- Column 1 is **empty visual space** that may become a non-draggable gap

Even in simple cases, it is non-trivial for the framework to automatically classify such gaps as draggable or non-draggable. More complex layouts—nested controls, templated UI, and dynamic content—make automatic detection even harder.

## Approaches Overview

In this approach the framework recursively traverses the visual tree and **excludes interactive controls from drag by default**. Developers can then **override** per element using `TitleBar.IsDragRegion`:
- Set `IsDragRegion="True"` to **include** an element in the drag region even if it is an interactive control (e.g., ribbon areas that should drag).
- Set `IsDragRegion="False"` to **exclude** non‑control surfaces or containers or any UI Element from drag.
- If the property is **omitted**, the framework uses the new default behavior.

**Advantages**
- **Low developer effort** (good defaults).
- **High flexibility** (simple overrides where needed).
- **Consistent, accessible behavior** aligned with product expectations.

**XAML sample**
```xml
<TitleBar Title="Main Titlte" Subtitle="subtitle" x:Name="titleBar">
    <TitleBar.Content>
        <Grid>
            <Grid.ColumnDefinitions>
                <ColumnDefinition Width="150" />
                <ColumnDefinition Width="200" />
                <ColumnDefinition Width="50" />
            </Grid.ColumnDefinitions>
            <Border Grid.Column="0" Background="LightBlue" BorderBrush="Black" BorderThickness="1">
                <AutoSuggestBox PlaceholderText="Search"/>
            </Border>
            <Border Grid.Column="1" />
            <Border Grid.Column="2" Background="LightCoral" BorderBrush="Black" BorderThickness="1">
                <TextBlock Text="Help" VerticalAlignment="Center" HorizontalAlignment="Center" />
            </Border>
        </Grid>
    </TitleBar.Content>
</TitleBar>
```
#### Output:
![Non draggable gaps in TitleBar Content](./images/titlebar-drag-issue-fixed.png)

#### Notes on behavior
- The framework still uses `ReadLocalValue(IsDragRegionProperty)` to determine whether the developer **explicitly set** a value. If **omitted**, the framework applies the new default drag‑region rules.
---

## Additional How‑To Topics

### Styling and Containers
You can apply `IsDragRegion` to containers to include/exclude large UI areas (e.g., toolbars). Use it sparingly to avoid accidentally disabling drag for entire subtrees; prefer marking the minimum necessary element.

```xml
<StackPanel Orientation="Horizontal" TitleBar.IsDragRegion="False">
  <Button Content="Back"/>
  <Button Content="Forward"/>
  <Button Content="Refresh"/>
</StackPanel>
```

### Nested Layouts
Use explicit `IsDragRegion` overrides on the specific nested element(s) that deviate from the default mode.

```xml
<Grid>
  <StackPanel Orientation="Horizontal">
    <TextBlock Text="Title"/>
    <Grid>
      <Button Content="Settings" TitleBar.IsDragRegion="False"/> <!-- Exclude -->
    </Grid>
  </StackPanel>
</Grid>
```

### Using in XAML, C#, and C++/WinRT

<table>
  <tr>
    <th>Language</th>
    <th>Code Sample</th>
    <th>Notes</th>
  </tr>
  <tr>
    <td><b>XAML</b></td>
    <td>
<pre lang="xml">&lt;TitleBar&gt;
  &lt;TitleBar.Content&gt;
    &lt;TextBlock Text="My App" /&gt;
    &lt;AutoSuggestBox TitleBar.IsDragRegion="True" /&gt;
  &lt;/TitleBar.Content&gt;
&lt;/TitleBar&gt;</pre>
    </td>
    <td>Enable enhanced defaults and opt a control into drag.</td>
  </tr>
  <tr>
    <td><b>C#</b></td>
    <td>
<pre lang="csharp">// Behavior on TitleBar
TitleBar tb = this.AppWindow.TitleBar();

// Per-element override
var search = new AutoSuggestBox();
TitleBar.SetIsDragRegion(search, true);</pre>
    </td>
    <td>Set the boolean and override an element in code-behind.</td>
  </tr>
  <tr>
    <td><b>C++/WinRT</b></td>
    <td>
<pre lang="cpp">using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

TitleBar tb = AppWindow().TitleBar();

AutoSuggestBox search{};
TitleBar::SetIsDragRegion(search, true);</pre>
    </td>
    <td>Equivalent usage in C++/WinRT.</td>
  </tr>
</table>

---

# API Pages

## TitleBar.IsDragRegion attached property
Marks an element as **included** in the window drag region (`True`) or **excluded** (`False`), overriding the framework default for the current behavior.

```xml
<Button Content="Refresh" TitleBar.IsDragRegion="False"/>
```

### Example Usage
```xml
<TitleBar>
  <TitleBar.Content>
    <AutoSuggestBox TitleBar.IsDragRegion="True"/>
    <TextBlock Text="Title"/>
  </TitleBar.Content>
</TitleBar>
```

---

# API Details

```c#
namespace Microsoft.UI.Xaml.Controls
{
    // Attached property used for per-element overrides
    static DependencyProperty IsDragRegionProperty { get; };
    static void SetIsDragRegion(DependencyObject element, Boolean value);
    static Boolean GetIsDragRegion(DependencyObject element);
}
```

---

## Appendix

### Keyboard Behaviour
This API affects **pointer hit‑testing** for window drag only; it does not change keyboard interaction. Ensure that interactive controls in `TitleBar.Content` remain fully focusable and operable. Keep a logical tab order in the title bar.

### Automation Behaviour
`IsDragRegion` **do not alter** UIA patterns or names of elements. Interactability for screen readers remains unchanged. Developers should verify that drag affordances are communicated visually and do not conflict with UIA expectations.

### Backward Compatibility
- The updated drag‑region model introduces a new global default: interactive controls are not draggable, and non‑interactive visuals are draggable, unless explicitly overridden.
