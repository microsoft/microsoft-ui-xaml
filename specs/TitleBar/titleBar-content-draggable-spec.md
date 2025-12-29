
# TitleBar Draggability API Specification

# Background
Custom title bar layouts often combine interactive controls with non-interactive visual elements using containers like `Grid`, `StackPanel`, or nested structures. These layouts can unintentionally create gaps or non-interactive regions whose drag behavior the framework cannot reliably infer. As a result, some title bar areas may unexpectedly become non-draggable, leading to inconsistent window movement.

To ensure predictable dragging behavior, developers need a way to explicitly mark UI elements that should **not** participate in window dragging—typically interactive controls. All remaining areas then function automatically as draggable regions. This proposal introduces the `TitleBar.ExcludeFromDrag` attached property to provide this explicit, predictable control.

This work is also related to **[#10421](https://github.com/microsoft/microsoft-ui-xaml/issues/10421)**.

---

# Conceptual pages (How To)

Custom layouts inside `TitleBar.Content` can introduce empty regions or areas where the framework cannot determine whether dragging should occur. For example:

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
- Column 1 is **empty visual space** that becomes a non-draggable gap

Even in simple cases, it is non-trivial for the framework to automatically classify such gaps as draggable or non-draggable. More complex layouts—nested controls, templated UI, dynamic content—make automatic detection even harder.

To provide clarity and control, developers can apply `TitleBar.ExcludeFromDrag` to interactive controls or any element that should not initiate dragging. All unmarked areas, including visual gaps, automatically behave as draggable regions.

### Example
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

### Key Guidance
- Mark interactive controls with `ExcludeFromDrag="True"`.
- All other regions—including layout gaps—become draggable by default.
- This minimizes annotation and ensures consistent title bar drag behavior.

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
- Nested layouts do not inherit exclusion automatically.

---

# API Details
```c++
    static Microsoft.UI.Xaml.DependencyProperty ExcludeFromDragProperty{ get; };
    static void SetExcludeFromDrag(Microsoft.UI.Xaml.DependencyObject element, Boolean value);
    static Boolean GetExcludeFromDrag(Microsoft.UI.Xaml.DependencyObject element);
```

---

# Appendix
- A future improvement may rename this property or adjust behavior based on API review feedback.
- Alternatives such as automatic region detection were considered but rejected due to unpredictability in complex UI layouts.

