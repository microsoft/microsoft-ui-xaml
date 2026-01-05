TitleBar ExcludeFromDrag
===

# Background

Custom title bar layouts often combine interactive controls with non-interactive visual elements using containers like `Grid`, `StackPanel`, or nested structures. These layouts can unintentionally create gaps or regions whose drag behavior the framework cannot reliably infer. As a result, some title bar areas may unexpectedly become non-draggable, leading to inconsistent window movement.

To ensure predictable dragging behavior, developers need a way to explicitly mark UI elements that should **not** participate in window dragging—typically interactive controls. All remaining areas then function automatically as draggable regions. This proposal introduces the `TitleBar.ExcludeFromDrag` attached property to provide explicit, predictable control.

This work is also related to **[#10421](https://github.com/microsoft/microsoft-ui-xaml/issues/10421)**.

---

# Conceptual pages (How To)

## How to use TitleBar.ExcludeFromDrag

Custom layouts inside `TitleBar.Content` can introduce empty regions or areas where the framework cannot determine whether dragging should occur.

### Standard Usage (Default behaviour)

When you customize `TitleBar.Content`, apply `TitleBar.ExcludeFromDrag="True"` to interactive controls (or any element that should not initiate dragging). All unmarked areas—including visual gaps—automatically behave as draggable regions.

#### Problem example: gaps become non-draggable

```xml
<TitleBar>
    <TitleBar.Content>
        <Grid ColumnDefinitions="50, 200, 50">
            <TextBlock Text="Title" />
            <TextBlock Text="Help" Grid.Column="2" />
        </Grid>
    </TitleBar.Content>
</TitleBar>
```

In this simple layout:
- Column 0 contains **Title**
- Column 2 contains **Help**
- Column 1 is **empty visual space** that may become a non-draggable gap

Even in simple cases, it is non-trivial for the framework to automatically classify such gaps as draggable or non-draggable. More complex layouts—nested controls, templated UI, and dynamic content—make automatic detection even harder.

#### Recommended solution: exclude interactive controls

```xml
<TitleBar>
    <TitleBar.Content>
        <StackPanel Orientation="Horizontal" Spacing="12">
            <TextBlock Text="Dashboard" />

            <!-- Interactive elements excluded from drag -->
            <AutoSuggestBox Width="250"
                            PlaceholderText="Search..."
                            TitleBar.ExcludeFromDrag="True" />

            <Button Content="Help"
                    TitleBar.ExcludeFromDrag="True" />
        </StackPanel>
    </TitleBar.Content>
</TitleBar>
```

### Advanced Usage

#### Applying to container boundaries

You can apply `TitleBar.ExcludeFromDrag` to containers as well (for example, to prevent dragging from a toolbar strip that contains multiple interactive elements). In this case, the entire container area is treated as non-draggable.

```xml
<TitleBar>
    <TitleBar.Content>
        <Grid>
            <StackPanel Orientation="Horizontal"
                        TitleBar.ExcludeFromDrag="True">
                <Button Content="Back" />
                <Button Content="Forward" />
                <Button Content="Refresh" />
            </StackPanel>
            <TextBlock Text="My App" HorizontalAlignment="Center" />
        </Grid>
    </TitleBar.Content>
</TitleBar>
```

#### Nested layouts

`ExcludeFromDrag` does **not** automatically inherit to descendants. If you need fine-grained behavior in nested layouts, mark the specific elements that should not initiate dragging.

```xml
<TitleBar>
    <TitleBar.Content>
        <Grid>
            <StackPanel Orientation="Horizontal">
                <TextBlock Text="Title" />
                <Grid>
                    <Button Content="Settings" TitleBar.ExcludeFromDrag="True" />
                </Grid>
            </StackPanel>
        </Grid>
    </TitleBar.Content>
</TitleBar>
```

### Using TitleBar.ExcludeFromDrag in XAML, C#, and C++

As any API can be used from XAML, C#, or C++/WinRT, the following table shows equivalent ways to apply the property.

<table>
  <tr>
    <th>Language</th>
    <th>Code Sample</th>
    <th>Notes</th>
  </tr>
  <tr>
    <td><b>XAML</b></td>
    <td>
<pre lang="xml">&lt;Button Content="Refresh"
        TitleBar.ExcludeFromDrag="True" /&gt;</pre>
    </td>
    <td>Recommended for declarative UI.</td>
  </tr>
  <tr>
    <td><b>C#</b></td>
    <td>
<pre lang="csharp">var button = new Button { Content = "Refresh" };
TitleBar.SetExcludeFromDrag(button, true);</pre>
    </td>
    <td>Uses the generated attached-property accessors.</td>
  </tr>
  <tr>
    <td><b>C++/WinRT</b></td>
    <td>
<pre lang="cpp">#include &lt;winrt/Microsoft.UI.Xaml.Controls.h&gt;
using namespace Microsoft::UI::Xaml::Controls;

Button button;
button.Content(box_value(L"Refresh"));
TitleBar::SetExcludeFromDrag(button, true);</pre>
    </td>
    <td>Uses static accessor methods on <code>TitleBar</code>.</td>
  </tr>
</table>

---

# API Pages

## TitleBar.ExcludeFromDrag attached property

The `ExcludeFromDrag` attached property allows developers to mark UI elements inside a custom title bar that should not contribute to window dragging. This ensures predictable drag behavior in custom and nested layouts.

```xml
<Button Content="Refresh"
        TitleBar.ExcludeFromDrag="True" />
```

### Example Usage

```xml
<TitleBar Title="SampleApp" Subtitle="Preview">
    <TitleBar.Content>
        <Grid ColumnDefinitions="50, *, Auto">
            <TextBlock Text="SampleApp" />
            <AutoSuggestBox Grid.Column="1"
                            PlaceholderText="Search..."
                            TitleBar.ExcludeFromDrag="True" />
            <Button Grid.Column="2" Content="Settings"
                    TitleBar.ExcludeFromDrag="True" />
        </Grid>
    </TitleBar.Content>
</TitleBar>
```

### Remarks

- Default behavior remains unchanged for non-custom layouts.
- Use this property only when customizing `TitleBar.Content`.
- Interactive controls should typically be excluded from dragging.

## TitleBar.SetExcludeFromDrag method

Sets the `ExcludeFromDrag` attached property value on the specified element.

```csharp
TitleBar.SetExcludeFromDrag(element, true);
```

## TitleBar.GetExcludeFromDrag method

Gets the `ExcludeFromDrag` attached property value from the specified element.

```csharp
bool excluded = TitleBar.GetExcludeFromDrag(element);
```

---

# API Details

```c++
static Microsoft.UI.Xaml.DependencyProperty ExcludeFromDragProperty{ get; };
static void SetExcludeFromDrag(Microsoft.UI.Xaml.DependencyObject element, Boolean value);
static Boolean GetExcludeFromDrag(Microsoft.UI.Xaml.DependencyObject element);
```

---

## Appendix

### Keyboard Behaviour

This API does not introduce new keyboard interactions by itself; it influences pointer-driven window dragging. However, custom title bar content must still follow common keyboard accessibility expectations:

- Interactive controls in `TitleBar.Content` should remain focusable and keyboard-operable.
- Do not rely on dragging gestures as the only means of window movement for keyboard users.
- Ensure tab order and directional navigation (where applicable) remain logical in the title bar area.

### Other Behaviour

- A future improvement may rename this property or adjust behavior based on API review feedback.
- Alternatives such as automatic region detection were considered but rejected due to unpredictability in complex UI layouts.
- Consider applying `ExcludeFromDrag` conservatively: prefer marking only interactive elements and leaving the rest draggable.
