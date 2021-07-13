ColorPicker (new `Orientation` property)
===

# Background

This API is an update to the existing ColorPicker control. The aim of this new API is, to make the ColorPicker more versatile to use and provide different layouting ways to maximize the space available.

# Conceptual pages (How To)

_Additional content to add to the existing page_

#### Specify the layout direction  

Using the `Orientation` property, you can specify whether the ColorPicker should take up more vertical space or more horizontal space.

```xaml
<muxc:ColorPicker IsAlphaEnabled="True" Orientation="Vertical"/>
```
![Vertical ColorPicker](./ColorPicker_VerticalMode.png)

```xaml
<muxc:ColorPicker IsAlphaEnabled="True" Orientation="Horizontal"/>
```
![Horizontal ColorPicker](./ColorPicker_HorizontalMode.png)

| Orientation | Meaning |
|-------------|---------|
| Horizontal  | The sliders will be put to the side of the color spectrum.|
| Vertical    | The sliders will be put underneath the color spectrum.|
# API Pages


## ColorPicker class

N/A


## ColorPicker.Orientation property

The orientation property specifies which way the way the parts of the ColorPicker will be laid out.

Using the orientation, you can specify whether the sliders should be laid out underneath the color area (vertical orientation) or to the side of it (horizontal orientation).

_Spec note: internal comment about this property that won't go into the public docs._

Introduction to one or more usages of the MyExample.PropertyOne property.

```c#
...
```

# API Details

```c# (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
  [webhosthidden]
  unsealed runtimeclass ColorPicker
  {
      /// Gets or sets the DisplayMode of the ColorPicker
      Windows.UI.Xaml.Controls.Orientation Orientation = Windows.UI.Xaml.Controls.Orientation.Vertical;
  }
}
```

# Appendix

<!-- TEMPLATE
  Anything else that you want to write down about implementation notes and for posterity,
  but that isn't necessary to understand the purpose and usage of the API.

  This or the Background section are a good place to describe alternative designs
  and why they were rejected.
-->