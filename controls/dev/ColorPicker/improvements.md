# ColorPicker Improvements

This document lists ideas for future implementations and lessons learned that may not be adopted due to breaking changes. These improvements should be considered for future implementations of this or similar controls.

* RGB and HSV channels are identified separately in most places. For example, HSV channels are represented by the `ColorPickerHsvChannel` enum. Since HSV is used as the primary representation internally there is no corresponding enum for RGB channels. In the control template, however, there are separate controls depending on the active color representation RGB/HSV. This means there is a duplication of a large number of controls. These could all be unified, for example, by referring to both hue and red as channel 1, etc. This greatly simplifies the effort required in the control template itself and removes duplicate controls. Instead, the same input controls can be used and only the active color representation set. Internal representation could add an enum for Channel1, Channel2, Channel3 and Alpha.

* The 'ThirdDimension' slider really should not be named 'dimension'. 'Channel' is the standardized term so 'ThirdChannelSlider' would be a better choice. 'Components' is also the term used in control properties.

* The `ColorPickerSlider` could render its own background and be made a completely independent control. This would simplify the `ColorPicker` template and code-behind considerably.

* Composting of the preview/slider backgrounds with a checkered background underneath can be done all at once in code-behind when the background image is calculated. This would remove several template parts and greatly simplify the overall template. The affect on performance should be minimal and not detectable.