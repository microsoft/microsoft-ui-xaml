DataTemplate from code
===

# Background

A [`DataTemplate`](https://learn.microsoft.com/windows/winui/api/microsoft.ui.xaml.datatemplate)
describes the visual structure that a control stamps out for each of its data items. Today a
`DataTemplate` is authored declaratively in XAML markup:

```xml
<DataTemplate>
    <TextBlock Text="{Binding}" />
</DataTemplate>
```

At runtime a control calls `DataTemplate.LoadContent()` (or realizes elements through the
`IElementFactory` that a `DataTemplate` implements) to produce a fresh copy of that markup for
each item. The XAML in a `DataTemplate` is parsed once and then replayed on demand.

There has been no supported way to build a `DataTemplate` entirely from code. Developers who
generate UI dynamically, for example in a C# app that builds item visuals conditionally, or in a
component library that ships templates without XAML resources, had to either author a XAML string
and parse it at runtime (`XamlReader.Load`), or bypass `DataTemplate` altogether and implement
their own `IElementFactory`. Both approaches are awkward: string XAML loses compile-time checking,
and a custom `IElementFactory` doesn't interoperate with the many APIs typed as `DataTemplate`
(`ItemsControl.ItemTemplate`, `ContentControl.ContentTemplate`, `ContentPresenter.ContentTemplate`,
and so on). This API provides a way to build a `DataTemplate` from code for use with properties
that only work with a `DataTemplate` and not an `IElementFactory`.

This feature adds a `DataTemplate` constructor that takes a callback. Each time the template needs
to realize its content, it invokes the callback to build and return a `UIElement` subtree. The
resulting `DataTemplate` behaves like any other `DataTemplate`: you can assign it to any property
typed as `DataTemplate`, it participates in `ItemsRepeater` element recycling, and its realized
elements receive a `DataContext` and raise `DataContextChanged` exactly as markup-authored
templates do.

> This API is **experimental** (gated behind `Feature_ExperimentalApi`) while the design is
> finalized.

# Conceptual pages (How To)

## Creating a DataTemplate in code

Pass a callback to the `DataTemplate` constructor. The callback takes no arguments and returns the
root `UIElement` of a freshly built subtree. The control that owns the template invokes the
callback whenever it needs a new realized element. Note that this callback is called sychronously
on WinUI's UI thread, and nothing explicitly prevents reentrancy if the callback itself causes a
call back to the DataTemplate.

```csharp
var template = new DataTemplate(() =>
{
    // Equivalent to
    //
    // <DataTemplate>
    //     <TextBlock Text="{Binding Path=DisplayName}" />
    // </DataTemplate>

    var textBlock = new TextBlock();
    textBlock.SetBinding(
        TextBlock.TextProperty,
        new Binding { Path = new PropertyPath(nameof(ItemViewModel.DisplayName)) });
    return textBlock;
});

myItemsRepeater.ItemTemplate = template;
```

Key points:

* **The callback runs once per realized element, not once per item.** Controls call it on a cache
  miss. When a control recycles elements (as `ItemsRepeater` does), a pooled element is reused and
  the callback is *not* invoked again. Don't rely on the callback firing a fixed number of times.
* **Return a fresh tree each time.** Build a new element subtree on every call; don't cache and
  return the same instance, because the control may realize several items concurrently.
* **Data binding still works.** The realized root element and its descendants receive their
  `DataContext` from the hosting control after the callback returns. Use `{Binding}` / `SetBinding`
  against the `DataContext`, and listen to `DataContextChanged` to react to the bound item.
* **Returning `null` is allowed.** A callback that returns `null` behaves like an empty
  `<DataTemplate/>`, and no content is produced.

# Examples

## Example: Use a code-authored template with ItemsRepeater

Use a callback to create items for an `ItemsRepeater`. Each realized `TextBlock` is bound to its
corresponding string. As the user scrolls, `ItemsRepeater` recycles `TextBlock` instances and
updates their `DataContext`. The callback runs only when the repeater needs to grow its element pool.


```csharp
sealed class ItemViewModel
{
    public ItemViewModel(string colorName, Color color)
    {
        ColorName = colorName;
        Color = color;
    }

    public string ColorName { get; }
    public Color Color { get; }
}

var items = new[]
{
    new ItemViewModel("Red", Color.FromArgb(0xFF, 0xFF, 0x00, 0x00)),
    new ItemViewModel("Green", Color.FromArgb(0xFF, 0x00, 0x80, 0x00)),
    new ItemViewModel("Blue", Color.FromArgb(0xFF, 0x00, 0x00, 0xFF)),
};

var template = new DataTemplate(() =>
{
    // Equivalent to
    //
    // <DataTemplate>
    //     <TextBlock Text="{Binding Path=ColorName}">
    //         <TextBlock.Foreground>
    //             <SolidColorBrush Color="{Binding Path=Color}" />
    //         </TextBlock.Foreground>
    //     </TextBlock>
    // </DataTemplate>

    var textBlock = new TextBlock();
    textBlock.SetBinding(
        TextBlock.TextProperty,
        new Binding { Path = new PropertyPath(nameof(ItemViewModel.ColorName)) });

    var foreground = new SolidColorBrush();
    foreground.SetBinding(
        SolidColorBrush.ColorProperty,
        new Binding { Path = new PropertyPath(nameof(ItemViewModel.Color)) });
    textBlock.Foreground = foreground;

    return textBlock;
});

var repeater = new ItemsRepeater
{
    ItemsSource = items,
    ItemTemplate = template,
};
```

## Example: React to the bound item with DataContextChanged

Update the element subtree when a data object is bound.

Note: This simulates the code generated by `x:Bind` - manually write values to a property in response to a change.
One difference is that here we're listening for `DataContextChanged`, whereas `x:Bind` doesn't care about the
`DataContext` and instead listens for `INotifyPropertyChanged` on the source object.

```csharp
var template = new DataTemplate(() =>
{
    // Similar to
    //
    // <DataTemplate>
    //     <TextBlock Text="{x:Bind Path=DisplayName}" />
    // </DataTemplate>

    var textBlock = new TextBlock();
    textBlock.DataContextChanged += (sender, args) =>
    {
        // args.NewValue is the item this element is now displaying.
        textBlock.Text = (args.NewValue as ItemViewModel)?.DisplayName;
    };
    return textBlock;
});
```

## Example: Load content directly

Any `DataTemplate`, including a code-authored one, can be inflated directly with `LoadContent()`.
Each call invokes the callback and returns a new tree:

```csharp
var template = new DataTemplate(() => new Button { Content = "Click me" });

var first = (Button)template.LoadContent();
var second = (Button)template.LoadContent();
// first and second are distinct Button instances.
```

# API Pages

## DataTemplate(DataTemplateElementFactory) constructor

Initializes a new instance of the `DataTemplate` class whose content is produced by a callback
instead of by parsing XAML markup.

```csharp
public DataTemplate(DataTemplateElementFactory elementFactory)
```

The `elementFactory` callback is invoked each time the template realizes a new element (for example
from `LoadContent()`, or when a control such as `ItemsRepeater` has a cache miss). The callback
returns the root `UIElement` of the subtree to use, or `null` to produce no content.

A `DataTemplate` created this way is a fully functional `DataTemplate`: assign it to any property
typed as `DataTemplate`, inflate it with `LoadContent()`, or use it through the `IElementFactory`
interface. Realized elements receive their `DataContext` from the hosting control and participate in
element recycling.

Remarks:

* The callback is not guaranteed to run once per data item. Controls that recycle elements reuse
  pooled instances and only call the callback on a cache miss. Don't depend on the invocation count.
* Return a new subtree on each call. Returning a shared instance leads to a single element being
  parented in multiple places.
* `elementFactory` must not be `null`; passing `null` throws `ArgumentNullException`.

## DataTemplateElementFactory delegate

Represents the method that builds the element subtree for a code-authored `DataTemplate`.

```csharp
public delegate UIElement DataTemplateElementFactory();
```

The delegate takes no parameters and returns the root `UIElement` of a newly constructed subtree,
or `null` to produce no content. You supply an instance of this delegate to the
`DataTemplate(DataTemplateElementFactory)` constructor.

# API Details

```csharp (but really MIDL3)
namespace Microsoft.UI.Xaml
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [feature(Feature_ExperimentalApi)]
    [webhosthidden]
    /// Represents the method that builds the element subtree for a code-authored DataTemplate.
    /// Build and return a fresh subtree on every call. Do not cache and return a shared instance.
    /// @return The root UIElement of a newly constructed subtree, or null to produce no content.
    delegate Microsoft.UI.Xaml.UIElement DataTemplateElementFactory();

    [webhosthidden]
    [contract(Microsoft.UI.Xaml.WinUIContract, 2)]
    unsealed runtimeclass DataTemplate : Microsoft.UI.Xaml.FrameworkTemplate
    {
        DataTemplate();

        /// Initialize a new instance of the DataTemplate class whose content is produced by
        /// the specified callback instead of by parsing XAML markup. The callback is invoked
        /// each time the template realizes a new element, not once per data item.
        ///
        /// @param elementFactory A callback that builds and returns the root UIElement of a fresh
        ///                       subtree, or null to produce no content. Must not be null.
        /// @throw If @p elementFactory is null.
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        [feature(Feature_ExperimentalApi)]
        DataTemplate(Microsoft.UI.Xaml.DataTemplateElementFactory elementFactory);

        Microsoft.UI.Xaml.DependencyObject LoadContent();

        // ... existing members ...
    }
}
```
