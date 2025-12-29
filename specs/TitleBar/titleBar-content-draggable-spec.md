
# Background and Motivation

The `TitleBar` component currently aligns content using **Center** alignment. Switching to **Stretch** improves visual layout but introduces a major usability issue: any empty space created when explicit widths are applied becomes **non‑draggable**, breaking expected window behaviours such as dragging and double‑click to maximize.

This effort is also part of the broader improvements tracked in **[#10421](https://github.com/microsoft/microsoft-ui-xaml/issues/10421)**.

Custom or deeply nested content layouts further make it difficult for developers to manage drag regions without explicit control. Interactive controls like `Button`, `TextBox`, and `ComboBox` must stay non‑draggable for consistent UX, yet no built‑in mechanism exists today to define draggable zones within `TitleBar.Content`.

To resolve this, a lightweight and explicit API is needed to give developers clear control over draggable regions while keeping default behaviour unchanged.

---

# API Proposal

## Overview
Introduce a new attached property:

```
TitleBar.IsDraggable="True"
```

This property lets developers explicitly mark UI elements inside `TitleBar.Content` as draggable. Default behaviour remains unchanged unless developers opt in.

## Behaviour Summary
- Default TitleBar behaviour is unchanged.
- Developers annotate draggable elements when using custom or Stretch‑aligned layouts.
- Interactive controls remain non‑draggable unless explicitly marked.
- Nested UI elements do not inherit draggability automatically.

## API Surface

```csharp
public static class TitleBar
{
    public static readonly DependencyProperty IsDraggableProperty =
        DependencyProperty.RegisterAttached(
            "IsDraggable",
            typeof(bool),
            typeof(TitleBar),
            new PropertyMetadata(false));
}
```

## Sample Usage

```xml
<TitleBar
    Title="WinUI Gallery"
    Subtitle="Preview">

    <TitleBar.Content>
        <StackPanel Orientation="Horizontal" Spacing="12">

            <TextBlock
                Text="Quick Access"
                TitleBar.IsDraggable="True" />

            <AutoSuggestBox
                Width="360"
                PlaceholderText="Search.."
                TitleBar.IsDraggable="True" />

        </StackPanel>
    </TitleBar.Content>
</TitleBar>
```

## Pros
- Clear and explicit draggable‑region control.
- Works reliably with complex or nested layouts.
- No hidden logic or behavioural surprises.

## Cons
- Requires manual marking of draggable zones.
- Risk of accidental non‑draggable gaps.

---
