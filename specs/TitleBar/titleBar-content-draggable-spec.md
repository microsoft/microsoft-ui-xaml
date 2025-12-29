
# TitleBar Draggability API Specification

# Background
Custom title bar layouts often mix interactive elements—like buttons and search boxes—with non‑interactive visual elements. When these elements are arranged using containers such as Grid, StackPanel, or nested layouts, the system cannot reliably determine which parts of the TitleBar.Content should act as the draggable window surface. This can leave some areas unintentionally non‑draggable, resulting in inconsistent window movement.

To make dragging behavior predictable, developers need a simple way to exclude only the interactive elements from contributing to the drag surface, allowing the rest of the title bar to function as expected for window dragging. This proposal introduces an attached property to explicitly mark such elements, improving clarity and control in custom title bar designs.

This work is also part of the improvements tracked in **[#10421](https://github.com/microsoft/microsoft-ui-xaml/issues/10421)**.

A lightweight and explicit API is therefore needed to let developers clearly mark non draggable regions within any TitleBar layout while keeping default behaviour unchanged.

---

# Conceptual pages (How To)
_(Optional conceptual overview for docs.microsoft.com)_

Developers can use the `TitleBar.ExcludeFromDrag` attached property to indicate which UI elements in a custom title bar should not act as drag surfaces. Interactive controls are typically excluded from dragging, while all other non‑interactive areas of the title bar automatically function as draggable regions.

---

# API Pages

## TitleBar.ExcludeFromDrag attached property
The `ExcludeFromDrag` attached property allows you to mark elements within a custom title bar as draggable regions. This gives direct control over which areas of your layout can be used to drag the window.

```xml
<Button TitleBar.ExcludeFromDrag="True" />
```

### Example
```xml
<TitleBar Title="WinUI Gallery" Subtitle="Preview">    
    <TitleBar.Content>
        <StackPanel Orientation="Horizontal" Spacing="12">            
            <TextBlock
                Text="Quick Access"
                TitleBar.ExcludeFromDrag="True" />            
            <AutoSuggestBox
                Width="360"
                PlaceholderText="Search.."
                TitleBar.ExcludeFromDrag="True" />
        </StackPanel>
    </TitleBar.Content>
</TitleBar>
```

### Remarks
- Default TitleBar behaviour is unchanged.
- Use this property when building custom or nested title bar layouts.

---

# API Details
```c++
    [MUX_PROPERTY_CHANGED_CALLBACK_METHODNAME("OnExcludeFromDragPropertyChanged")]
    static Microsoft.UI.Xaml.DependencyProperty ExcludeFromDragProperty{ get; };
    static void SetExcludeFromDrag(Microsoft.UI.Xaml.DependencyObject element, Boolean value);
    static Boolean GetExcludeFromDrag(Microsoft.UI.Xaml.DependencyObject element);
```

---

# Appendix

## Pros
- Clear and explicit draggable‑region control.
- Works reliably with complex or nested layouts.
- No hidden logic or behavioural surprises.

## Cons
- Requires manual marking of draggable zones.
- Risk of accidental non‑draggable gaps.
