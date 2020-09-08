# ColorPicker Architecture

This document is primarily intended for developers and contributors to WinUI. For additional information see:

* [Color Picker Design Docs](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/color-picker) Useful for high-level information related to use of this control.
* [Color Picker Class Docs](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.colorpicker) Useful as an API reference for developers implementing this control in an application.

The ColorPicker directory contains a set of controls with the primary control being the `ColorPicker` itself. These controls, their relationships, and function are outlined below and then described in the following sections.

1. `ColorPicker` : The top-level control containing all other controls. This is the control intended for user interaction and used by application developers.
2. `ColorPickerSlider` : A custom slider implementation used for the sliders in the `ColorPicker` control template itself. These sliders are unique in that they have a gradient background rendered to correspond with a defined HSV color channel. Sliders are 1-dimensional. This control is not useful apart from the `ColorPicker`.
3. `ColorSpectrum` : The main graphic displaying a field of possible colors the user can select from. The color spectrum is 2-dimensional and can be viewed in a Box or Ring shape.
4. `SpectrumBrush` : A special brush used to render the color spectrum background using composition APIs. This is only available on Windows version 1703 (RS2) and later.

The control hierarchy and namespaces are as follows:

```
 ─ ColorPicker             [Microsoft.UI.Xaml.Controls]
   ├── ColorPickerSlider   [Microsoft.UI.Xaml.Controls.Primitives]
   └── ColorSpectrum       [Microsoft.UI.Xaml.Controls.Primitives]
       └── SpectrumBrush   [Microsoft.UI.Xaml.Controls.Primitives]
```

Generalized helper methods are also located in two places:

 * ColorHelpers.[cpp/h] : Helpers specifically for the color picker and related controls. These are located in the `dev/ColorPicker` directory itself.
 * ColorConversion.[cpp/h] : More generalized helpers for working with RGB and HSV colors. The RGB and HSV structures are defined here as well as conversions between the two color representations and hex. These files are located in the `dev/Common` directory separate from ColorPicker.

## ColorPicker

[Control Properties](https://docs.microsoft.com/en-us/uwp/api/microsoft.ui.xaml.controls.colorpicker#properties), [Control Events](https://docs.microsoft.com/en-us/uwp/api/microsoft.ui.xaml.controls.colorpicker#events)

Most of the detailed functionality specific to the `ColorPicker` is implemented in the primitive controls outlined below. See the links above and at the top of this document for full details on the usage of the `ColorPicker` itself.

The `ColorPicker` functions by monitoring for changing in properties or user selection and then updating the selected color and graphical controls (preview rectangles, sliders, input text boxes, etc.) accordingly. It also ensures the various properties and input is validated before computing the updated selected color.

Internally, the `ColorPicker` always represents the selected color in HSV instead of RGB. For cases where precision is important this creates some problems as the color is always converted to RGB before being available from external APIs. This is in contrast to the `ColorPickerSlider` and `ColorSpectrum` which pass the color information directly in HSV to avoid loss of precision. 

### Template Parts

The color picker looks for and uses the following parts in the control template. These controls usually have two things done internally when the template is applied: (1) connect events to watch for value changes and update the color picker accordingly (2) set automation properties such as name.

* `ColorSpectrum` : [ColorSpectrum] The 2-dimensional field of colors that the user can select from.
* `ColorPreviewRectangleGrid` : [Grid] A container holding both the current preview color and previous color controls. In the template this is used to quickly show/hide the preview rectangles all at once. In code this is necessary to get the correct actual width/height used for rendering the preview rectangle backgrounds (which, when alpha is enabled, may be checkered).
* `ColorPreviewRectangle` : [Rectangle] A rectangle with the fill set to the currently selected color. This must be above the checkered background rectangle (which isn't a template part) so opacity can be used to blend with the checkered background drawn with the `ColorPreviewRectangleCheckeredBackgroundImageBrush`.
* `PreviousColorRectangle` : [Rectangle] A rectangle with the fill set to the previously selected color.
* `ColorPreviewRectangleCheckeredBackgroundImageBrush` : [ImageBrush] The image brush fill of the checkered background rectangle that must be below the `ColorPreviewRectangle`. The checkered background rectangle itself is not a template part but should still be in a control template -- it just isn't used in code-behind.

* `ThirdDimensionSlider` : [ColorPickerSlider] The 1-dimensional slider representing the third channel of the selected color not shown in the `ColorSpectrum`. This will always be an HSV channel. 
* `ThirdDimensionSliderGradientBrush` : [LinearGradientBrush] The image brush fill of the background rectangle that must be below the `ThirdDimensionSlider`. It visually represents a gradient of values the user can choose from. The background gradient rectangle itself is not a template part but should still be in the control template -- it just isn't used in code-behind. The background image brush is not managed by the slider but instead by the `ColorPicker`.
* `AlphaSlider` : [ColorPickerSlider] The 1-dimensional slider representing the alpha channel of the selected color not shown in the `ColorSpectrum`.
* `AlphaSliderGradientBrush` : [LinearGradientBrush] The image brush fill of the `AlphaSliderBackgroundRectangle` background rectangle that must be below the `AlphaSlider`. It visually represents a gradient of alpha values the user can choose from. The background image brush is not managed by the slider but instead by the `ColorPicker`.
* `AlphaSliderBackgroundRectangle` : [Rectangle] The background rectangle whose fill is used to represent the gradient of values the user can choose from in the alpha slider. This is a template part -- unlike the third dimension -- because the checkered background must be re-rendered if its size changes.
* `AlphaSliderCheckeredBackgroundImageBrush` : [ImageBrush] The image brush fill of the checkered background rectangle that must be below the `AlphaSlider`. The checkered background rectangle itself is not a template part but should still be in a control template -- it just isn't used in code-behind.

* `MoreButton` : [ButtonBase] A button used to expand the color picker showing additional input controls.
* `ColorRepresentationComboBox` : [ComboBox] The ComboBox used by the user to select between the RGB and HSV color representation. 
* `RGBComboBoxItem` : [ComboBoxItem] The item specifying 'RGB' color representation within the `ColorRepresentationComboBox`.
* `HSVComboBoxItem` : [ComboBoxItem] The item specifying 'HSV' color representation within the `ColorRepresentationComboBox`.

* `RedTextBox` : [TextBox] The red channel input text box the user can modify.
* `GreenTextBox` : [TextBox] The green channel input text box the user can modify.
* `BlueTextBox` : [TextBox] The blue channel input text box the user can modify.
* `HueTextBox` : [TextBox] The hue channel input text box the user can modify.
* `SaturationTextBox` : [TextBox] The saturation channel input text box the user can modify.
* `ValueTextBox` : [TextBox] The value channel input text box the user can modify.
* `AlphaTextBox` : [TextBox] The alpha channel input text box the user can modify (shared for both RGB/HSV color representation). This value is a percentage from 0%..100% and the '%' symbol will be automatically added and cannot be removed.
* `HexTextBox` : [TextBox] The hexadecimal color input text box the user can modify. This is always in RGB color representation in either #RGB or #ARGB format. The '#' symbol will be automatically added and cannot be removed.

* `RedLabel` : [TextBlock] A localized label to identify the red channel input text box. 
* `GreenLabel` : [TextBlock] A localized label to identify the green channel input text box.
* `BlueLabel` : [TextBlock] A localized label to identify the blue channel input text box.
* `HueLabel` : [TextBlock] A localized label to identify the hue channel input text box.
* `SaturationLabel` : [TextBlock] A localized label to identify the saturation channel input text box.
* `ValueLabel` : [TextBlock] A localized label to identify the value channel input text box.
* `AlphaLabel` : [TextBlock] A localized label to identify the alpha channel input text box (shared for both RGB/HSV).

## ColorPickerSlider

A custom slider implementation used for the sliders in the `ColorPicker` control template. These sliders are unique in that they have a gradient background rendered to correspond with a defined HSV color channel (important notes on background below). Sliders provide a one-dimensional method of modifying a color channel. They are used for the third color channel not on the `ColorSpectrum` or the alpha channel.

Visually to the user, these sliders have a background rendered based on the assigned HSV color channel or alpha. For example, the background could be a gradient of the selected color from minimum saturation to maximum saturation if the slider represents the saturation channel. However, the slider itself does not draw this background gradient. The gradient is managed by the parent `ColorPicker` and shown underneath the slider as the fill of a background rectangle. For the alpha channel slider there is a further checkered background that is rendered separately and then blended using opacity. `ColorPickerSlider` and `ColorPicker` are intrinsically linked together and `ColorPickerSlider` is not useful as a stand-alone control.

Like the ColorSpectrum, this slider always operates in HSV color representation.

### Template Parts

The `ColorPickerSlider` has no template parts differing from the standard `Slider`. 

## ColorSpectrum

[Control Properties](https://docs.microsoft.com/en-us/uwp/api/microsoft.ui.xaml.controls.primitives.colorspectrum#properties), [Control Events](https://docs.microsoft.com/en-us/uwp/api/microsoft.ui.xaml.controls.primitives.colorspectrum#events)

The color spectrum is the main 2-dimensional graphic displaying a field of possible colors the user can select from. The spectrum always operates in HSV color representation. This is because the HSV color representation is specifically designed to describe human perception of color.

The spectrum itself is only 2-dimensional, yet a color can have up to 4 dimensions/channels (hue, saturation, value, alpha); therefore, the two channels that should be displayed must be specified. This is done using the `Components` property which has all possible combinations of the HSV color channels excluding alpha (Within a `ColorPicker`, the spectrum's properties including `Components` are set with TemplateBinding in the style). Each of the components specified by setting `Components` will then take an axis on the spectrum and all values interpolated between them. The third channel, and alpha, must be represented outside of the color spectrum itself. This is handled in the `ColorPicker` using two additional sliders. Note that the third dimension and alpha will be considered maximum when rendering the spectrum.

The `ColorSpectrum` also maintains a cursor that visually identifies to the user the selected color. When the spectrum image is rendered (more on that below) an array of the color at every pixel in the spectrum image is stored (`m_hsvValues`). This array can become quite large, for example a 600px x 600px spectrum will build an array of 36000 unique HSV colors. When the pointer is pressed on the spectrum, the corresponding color at those X/Y pointer coordinates is looked-up in the array and set as the new selected color. The look-up is simple as the array indexes directly correspond to the pixel X/Y coordinates. The selected color is communicated from the `ColorSpectrum` to the `ColorPicker` using the spectrum's `ColorChanged` event and the `HsvColor` property.

Note: Because the `ColorSpectrum` works in HSV, it is very important that the `HsvColor` property is used to get/set the selected color on the spectrum instead of `Color` (which uses RGB). This is necessary to avoid the accumulation of rounding errors when converting between HSV/RGB. This also means that the `HsvColor` must be used in code-behind instead of binding to `Color` in XAML. Remember the HSV color is the main color representation internally -- not RGB.

### Template Parts

The color spectrum looks for and uses the following parts in the control template. 

* `LayoutRoot` : [Grid] The root element of the control. Changes to the size of the root element will trigger re-calculation of the spectrum.
* `SizingGrid` : [Grid] The grid representing the actual width/height that the spectrum will be drawn to.
* `SpectrumRectangle` : [Rectangle] The background rectangle is used to represent the color spectrum gradient. This is blended with the overlay rectangle using opacity (see notes below on rendering). This rectangle is only visible in Box shape.
* `SpectrumEllipse` : [Ellipse] The background ellipse use to represent the color spectrum gradient. This is blended with the overlay ellipse using opacity (see notes below on rendering). This ellipse is only visible in Ring shape.
* `SpectrumOverlayRectangle` : [Rectangle] The overlay rectangle use to represent the color spectrum gradient. This is blended with the background rectangle using opacity (see notes below on rendering). This rectangle is only visible in Box shape.
* `SpectrumOverlayEllipse` : [Ellipse] The overlay ellipse use to represent the color spectrum gradient. This is blended with the background ellipse using opacity (see notes below on rendering). This ellipse is only visible in Ring shape.
* `InputTarget` : [FrameworkElement] The control used to track pointer interaction by the user and get X/Y coordinates to change the selected color in the spectrum.
* `SelectionEllipsePanel` : [Panel] The panel representing the selected color indicator shown on the spectrum. 
* `ColorNameToolTip` : [ToolTip] The tool-tip used to present the selected color's displayed name to the user.

### Rendering the Spectrum

There are two different techniques used for generating/rendering the color spectrum depending on the version of Windows.

 1. On Windows version 1607 (RS1) and before, the spectrum is rendered as an array of individual bytes, converted to a WritableBitmap, set as the source of an ImageBrush and then finally set as the background/fill of a spectrum rectangle. 
 2. On Windows version 1703 (RS2) and later, the spectrum images are placed in a loaded image surface, which is then put into a SpectrumBrush. This technique uses the Composition APIs.

Rendering is done asynchronously for performance. However, it still can take 10s of milliseconds to re-render the spectrum. This time becomes negligible as the spectrum does not need to re-render itself as the selected color changes. A special trick is used as documented in the `ColorSpectrum.cpp` source:

> As the user perceives it, every time the third dimension not represented in the ColorSpectrum changes, the ColorSpectrum will visually change to accommodate that value.  For example, if the ColorSpectrum handles hue and luminosity, and the saturation externally goes from 1.0 to 0.5, then the ColorSpectrum will visually change to look more washed out to represent that third dimension's new value. Internally, however, we don't want to regenerate the ColorSpectrum bitmap every single time this happens, since that's very expensive. In order to make it so that we don't have to, we implement an optimization where, rather than having only one bitmap, we instead have multiple that we blend together using opacity to create the effect that we want. In the case where the third dimension is saturation or luminosity, we only need two: one bitmap at the minimum value of the third dimension, and one bitmap at the maximum.  Then we set the second's opacity at whatever the value of the third dimension is - e.g., a saturation of 0.5 implies an opacity of 50%. In the case where the third dimension is hue, we need six: one bitmap corresponding to red, yellow, green, cyan, blue, and purple. We'll then blend between whichever colors our hue exists between - e.g., an orange color would use red and yellow with an opacity of 50%. This optimization does incur slightly more startup time initially since we have to generate multiple bitmaps at once instead of only one, but the running time savings after that are *huge* when we can just set an opacity instead of generating a brand new bitmap.

## SpectrumBrush

Provides a way of drawing the `ColorSpectrum` background using the composition APIs in Windows. Internally, this uses two `CompositionSurfaceBrush` and both a minimum and maximum surface composited together. This brush is how the spectrum is rendered to the display on Windows version 1703 (RS2) and later.  

## Ideas for Future Implementations / Lessons Learned

* RGB and HSV channels are identified separately in most places. For example, HSV channels are represented by the `ColorPickerHsvChannel` enum. Since HSV is used as the primary representation internally there is no corresponding enum for RGB channels. In the control template, however, there are separate controls depending on the active color representation RGB/HSV. This means there is a duplication of a large number of controls. These could all be unified, for example, by referring to both hue and red as channel 1, etc. This greatly simplifies the effort required in the control template itself and removes duplicate controls. Instead, the same input controls can be used and only the active color representation set. Internal representation could add an enum for Channel1, Channel2, Channel3 and Alpha.

* The 'ThirdDimension' slider really should not be named 'dimension'. 'Channel' is the standardized term so 'ThirdChannelSlider' would be a better choice. 'Components' is also the term used in control properties.

* HSV color representation should be externally exposed by the color picker for those applications that require higher levels of precision. A new HsvColor struct should be added to the existing RGB struct already in WinUI.

* The `ColorPickerSlider` could render its own background and be made a completely independent control. This would simplify the `ColorPicker` template and code-behind considerably.

* Composting of the preview/slider backgrounds with a checkered background underneath can be done all at once in code-behind when the background image is calculated. This would remove several template parts and greatly simplify the overall template. The affect on performance should be minimal and not detectable.
