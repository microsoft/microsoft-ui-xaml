# ColorPicker Architecture

The ColorPicker directory contains a set of controls with the primary control being the ColorPicker itself. These controls, their relationships and function are outlined below and described in the following sections.


1. **ColorPicker** : The top-level control containing all other controls. This is control intended for user interaction and used by app developers.
2. **ColorPickerSlider** : A custom slider implementation used for the sliders in the ColorPicker control template itself. These sliders are unique in that they have a gradient background rendered to correspond with a defined HSV color channel. Sliders are 1-dimensional. 
3. **ColorSpectrum** : The main graphic displaying a field of possible colors the user can select from. The color spectrum is 2-dimensional.
4. **SpectrumBrush** : A special brush used to render the color spectrum background using the Composition APIs. This is only available on Windows version 1703 (RS2) and later.

The control hierarchy and namespaces are as follows:

```
 ─ ColorPicker             [Microsoft.UI.Xaml.Controls]
   ├── ColorPickerSlider   [Microsoft.UI.Xaml.Controls.Primitives]
   └── ColorSpectrum       [Microsoft.UI.Xaml.Controls.Primitives]
       └── SpectrumBrush   [Microsoft.UI.Xaml.Controls.Primitives]
```

Generalized helper methods are also located in two places:

 * ColorHelpers.[cpp/h] : Helpers specifically for the color picker and related controls. These are located in the ColorPicker directory itself.
 * ColorConversion.[cpp/h] : More generalized helpers working with RGB and HSV colors. The RGB and HSV structures are defined here as well as conversions between the two color representations and hex. These files are located in the Common directory separate from ColorPicker.

## ColorPicker


TODO


## ColorPickerSlider

These sliders have a background rendered based on the assigned HSV color channel. For example, the background could be a gradient of the selected color from minimum saturation to maximum saturation if the slider represents the Saturation channel. The slider takes care of all the rendering logic so the ColorPicker doesn't need to be aware of it and can simply use the slider itself.

Like the ColorSpectrum, this slider always operates in HSV color representation.

## ColorSpectrum

The color spectrum is the main two-dimensional graphic displaying a field of possible colors the user can select from. The spectrum always operates in HSV color representation. This is because the HSV color representation is specifically designed to describe human perception of color.

The spectrum itself is only 2-dimensional, yet a color can have up to 4 dimensions/channels (hue, saturation, value, alpha); therefore, the two channels that should be displayed must be specified. This is done using the `Components` property which has all possible combinations of the HSV color channels excluding alpha. Each of the components specified by setting `Components` will then take an axis on the spectrum and all values interpolated between them. The third, unspecified dimension and alpha will be considered maximum when rendering the spectrum.

The third channel, and alpha must be represented outside of the color spectrum itself. This is handled in the ColorPicker using two additional sliders.

The ColorSpectrum also maintains a cursor that visually identifies to the user the selected color. When the spectrum image is rendered (more on that below) an array of the color at every pixel in the spectrum image is stored (`m_hsvValues`). This array can become quite large, for example a 600px x 600px spectrum will build an array of 36000 unique HSV colors. When the pointer is pressed on the spectrum, the corresponding color at those X/Y pointer coordinates is looked-up in the array and set as the new selected color. The look-up is simple as the array indexes directly correspond to the pixel X/Y coordinates.

Note: Because the ColorSpectrum works in HSV, it is very important that the `HsvColor` property is used to get/set the selected color on the spectrum instead of `Color` (which uses RGB). This is necessary to avoid the accumulation of rounding errors when converting between HSV/RGB. This also means that the `HsvColor` must be used in code-behind instead of binding to `Color` in XAML. Remember the HSV color is the main color representation internally -- not RGB.

### Rendering the Spectrum

There are presently two different techniques used for generating/rendering the color spectrum depending on the version of Windows.

 1. On Windows version 1607 (RS1) and before, the spectrum is rendered as an array of individual bytes, converted to a WritableBitmap, set as the source of an ImageBrush and then finally set as the background/fill of a spectrum rectangle. 
 2. On Windows version 1703 (RS2) and later, the spectrum images are placed in a loaded image surface, which is then put into a SpectrumBrush. This technique uses the Composition APIs.

Rendering is done asynchronously for performance. However, it still can take 10s of milliseconds to re-render the spectrum. This time becomes negligible as the spectrum does not need to re-render itself as the selected color changes. A special trick is used as documented in the `ColorSpectrum.cpp` source:

> As the user perceives it, every time the third dimension not represented in the ColorSpectrum changes, the ColorSpectrum will visually change to accommodate that value.  For example, if the ColorSpectrum handles hue and luminosity, and the saturation externally goes from 1.0 to 0.5, then the ColorSpectrum will visually change to look more washed out to represent that third dimension's new value. Internally, however, we don't want to regenerate the ColorSpectrum bitmap every single time this happens, since that's very expensive. In order to make it so that we don't have to, we implement an optimization where, rather than having only one bitmap, we instead have multiple that we blend together using opacity to create the effect that we want. In the case where the third dimension is saturation or luminosity, we only need two: one bitmap at the minimum value of the third dimension, and one bitmap at the maximum.  Then we set the second's opacity at whatever the value of the third dimension is - e.g., a saturation of 0.5 implies an opacity of 50%. In the case where the third dimension is hue, we need six: one bitmap corresponding to red, yellow, green, cyan, blue, and purple. We'll then blend between whichever colors our hue exists between - e.g., an orange color would use red and yellow with an opacity of 50%. This optimization does incur slightly more startup time initially since we have to generate multiple bitmaps at once instead of only one, but the running time savings after that are *huge* when we can just set an opacity instead of generating a brand new bitmap.

## SpectrumBrush

TODO

## Ideas for Future Implementations

RGB and HSV channels are identified separately in most places. For example, HSV channels are represented by the `ColorPickerHsvChannel` enum. Since HSV is used as the primary representation internally there is no corresponding enum for RGB channels. In the control template, however, there are separate controls depending on the active color representation RGB/HSV. This means there is a duplication of a large number of controls.

These could all be unified, for example, by referring to both hue and red as channel 1, etc. This greatly simplifies the effort required in the control template itself and removes duplicate controls. Instead, the same input controls can be used and only the active color representation set. Internal representation could add an enum for Channel1, Channel2, Channel3 and Alpha.

The 'ThirdDimension' slider really should not be named 'dimension'. 'Channel' is the standardized term so 'ThirdChannelSlider' would be a better choice. 'Components' is also the term used in control properties.

NumberBox controls can be used for all numerical value input instead of .