RadialGradientBrush
===

# Background

Xaml currently has a [LinearGradientBrush](http://msdn.microsoft.com/library/Microsoft.UI.Xaml.Media.LinearGradientBrush),
which draws a gradient along a line. New here is a `RadialGradientBrush` which draws a gradient along the radius of a circle.

This is similar to the WPF [RadialGradientBrush](https://docs.microsoft.com/dotnet/api/System.Windows.Media.RadialGradientBrush),
and is implemented using Composition's
[CompositionRadialGradientBrush](https://docs.microsoft.com/uwp/api/Windows.UI.Composition.CompositionRadialGradientBrush).

## Approach to using a CompositionBrush

The Xaml `RadialGradientBrush` will inherit from
[XamlCompositionBrushBase](https://docs.microsoft.com/uwp/api/windows.ui.xaml.media.xamlcompositionbrushbase)
and use a [CompositionRadialGradientBrush](https://docs.microsoft.com/uwp/api/windows.ui.composition.compositionradialgradientbrush),
which is supported on Windows 10 1903 (v10.0.18362+).
This seems like a useful model for new brushes going forward for both WinUI 2 and 3, and we aren't too concerned about
duplicating some properties from other candidate base classes
[GradientBrush](https://docs.microsoft.com/uwp/api/windows.ui.xaml.media.gradientbrush).

The RadialGradientBrush API will more closely mirror the behavior of the Composition API than the WPF API.
For example, the gradient origin defaults to center and provides a `GradientOriginOffset` property
to specify an offset rather than providing a `GradientOrigin` property.

Notable inconsistencies with the composition API include:
1. Usage of [Point](https://docs.microsoft.com/uwp/api/windows.foundation.point)
rather than [Vector2](https://docs.microsoft.com/uwp/api/windows.foundation.numerics.vector2)/[float2](https://docs.microsoft.com/windows/win32/numerics_h/float2-structure)
2. Usage of [GradientStop](https://docs.microsoft.com/uwp/api/windows.ui.xaml.media.gradientstop)
rather than [CompositionColorGradientStop](https://docs.microsoft.com/uwp/api/windows.ui.composition.compositioncolorgradientstop)

The main reason for the above is to enable Xaml markup parsing for brushes defined in markup.

### Animation support

Ideally, `RadialGradientBrush` would support animation of `GradientStops` and other properties
using composition animations by implementing
[IAnimationObject](https://docs.microsoft.com/uwp/api/windows.ui.composition.ianimationobject).
However, the `IAnimationObject` interface and support for animation of non-composition types is only supported on
Windows 10 1809 (v10.0.17763) and newer. This could be revisited with WinUI 3.

### Downlevel fallback behavior

The options for fallback behavior on downlevel pre-1903 OS versions are:

1. Draw a gradient using another renderer, e.g. Direct2D
   * This would require loading additional expensive dependencies which WinUI doesn't currently rely on.
2. Draw a custom gradient surface, e.g. using a CompositionSurfaceBrush or WriteableBitmap
   * This would require largely reimplementing the complex CompositionRadialGradientBrush.
3. **Rely on FallbackColor**
   * Since [CompositionRadialGradientBrush](https://docs.microsoft.com/uwp/api/windows.ui.composition.compositionradialgradientbrush)
should be available downlevel with WinUI 3, #1 (FallbackColor) seems like a reasonable approach in the meantime.

# API Pages

## RadialGradientBrush class

Paints an area with a radial gradient.
A center point defines the origin of the gradient, and an ellipse defines the outer bounds of the gradient.

The following example creates a radial gradient with six gradient stops and uses it to paint a Rectangle.

### Examples

![A rectangle filled with a radial gradient.](images/ColorRadialGradientBrush.png)

XAML
```XAML
<Page
  xmlns:media="using:Microsoft.UI.Xaml.Media">

  <Rectangle Width="200" Height="200">
      <Rectangle.Fill>
          <media:RadialGradientBrush>
              <GradientStop Color="Blue" Offset="0.0" />
              <GradientStop Color="Yellow" Offset="0.2" />
              <GradientStop Color="LimeGreen" Offset="0.4" />
              <GradientStop Color="LightBlue" Offset="0.6" />
              <GradientStop Color="Blue" Offset="0.8" />
              <GradientStop Color="LightGray" Offset="1" />
          </media:RadialGradientBrush>
      </Rectangle.Fill>
  </Rectangle>

</Page>
```

This example creates a radial gradient with that uses Absolute mapping mode with
custom values for `EllipseCenter`, `EllipseRadius` and `GradientOriginOffset`:

![A rectangle filled with an offset radial gradient.](images/OffsetRadialGradientBrush.png)

```XAML
<Page
  xmlns:media="using:Microsoft.UI.Xaml.Media">

  <Rectangle Width="200" Height="200">
      <Rectangle.Fill>
          <media:RadialGradientBrush
            MappingMode="Absolute"
            EllipseCenter="50,50"
            EllipseRadius="100,100"
            GradientOriginOffset="50,0"
            >
              <GradientStop Color="Yellow" Offset="0.0" />
              <GradientStop Color="Blue" Offset="1" />
          </media:RadialGradientBrush>
      </Rectangle.Fill>
  </Rectangle>

</Page>
```

### Remarks

#### Gradient layout

The gradient is drawn within an ellipse that is defined by the `EllipseCenter` and `EllipseRadius` properties.
Colors for the gradient start at the center of the ellipse and end at the radius.

The colors for the radial gradient are defined by color stops added to the `GradientStops` collection property.
Each gradient stop specifies a color and an offset along the gradient.

The gradient origin defaults to center and can be offset using the `GradientOriginOffset` property.

`MappingMode` defines whether `EllipseCenter`, `EllipseRadius` and `GradientOriginOffset` represent relative or absolute coordinates.

When `MappingMode` is set to `RelativeTBoundingBox`, the X and Y values of the three properties are treated asrelative to the brush's rendered bounds,
where `(0,0)` represents the top left and `(1,1)` represents the bottom right of the brush's rendered bounds for the
`EllipseCenter` and `EllipseRadius` properties and `(0,0)` represents the center for the `GradientOriginOffset` property.

When `MappingMode` is set to `Absolute`, the X and Y values of the three properties are treated as absolute coordinates within the brush's rendered bounds.

#### InterpolationSpace

The underlying [CompositionRadialGradientBrush](https://docs.microsoft.com/uwp/api/windows.ui.composition.compositionradialgradientbrush)
currently only supports the following
[CompositionColorSpace](https://docs.microsoft.com/uwp/api/windows.ui.composition.compositioncolorspace) values:

* `Auto`
* `Rgb`
* `RgbLinear`

Applying any other interpolation color space will have no effect.

#### Windows 10 Version Support

Gradient rendering is supported on Windows 10 version 1903 (v10.0.18362.0) and higher.
On earlier OS versions the brush will render a solid color specified by the `FallbackColor` property.

#### See Also

* [LinearGradientBrush](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Media.LinearGradientBrush)

* [CompositionRadialGradientBrush](https://docs.microsoft.com/uwp/api/windows.ui.composition.compositionradialgradientbrush)

## Other RadialGracientBrush members

| | |
| - | - |
| EllipseCenter | The center of the ellipse that contains the gradient. The default is `(0.5, 0.5)`. |
| EllipseRadius | The radius of the ellipse that contains the gradient. The default is `(0.5, 0.5)`. |
| GradientOriginOffset | The gradient origin's offset from the center of the element. |
| GradientStops | A collection of [GradientStop](https://docs.microsoft.com/uwp/api/windows.ui.xaml.media.gradientstop) objects that define the gradient. |
| InterpolationSpace | The color space used to interpolate the gradient's colors. The default is `Auto`. For supported values, see [CompositionRadialGradientBrush.InterpolationSpace](https://docs.microsoft.com/uwp/api/windows.ui.composition.compositiongradientbrush.interpolationspace#Windows_UI_Composition_CompositionGradientBrush_InterpolationSpace) |
| MappingMode | Defines whether `EllipseCenter`, `EllipseRadius` and `GradientOriginOffset` represent relative coordinates in the range 0 to 1 or absolute coordinates. The default is `RelativeToBoundingBox`. |
| SpreadMethod | Gets or sets the type of spread method that specifies how to draw a gradient that starts or ends inside the bounds of the object to be painted. The default is `Pad`. |

# API Details

```C++
[WUXC_VERSION_PREVIEW]
[webhosthidden]
[contentproperty("GradientStops")]
unsealed runtimeclass RadialGradientBrush : Windows.UI.Xaml.Media.XamlCompositionBrushBase
{
    RadialGradientBrush();

    [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
    [MUX_DEFAULT_VALUE("winrt::Point(0.5,0.5)")]
    Windows.Foundation.Point EllipseCenter { get; set; };

    [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
    [MUX_DEFAULT_VALUE("winrt::Point(0.5,0.5)")]
    Windows.Foundation.Point EllipseRadius { get; set; };

    [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
    Windows.Foundation.Point GradientOriginOffset { get; set; };

    [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
    [MUX_DEFAULT_VALUE("winrt::BrushMappingMode::RelativeToBoundingBox")]
    Windows.UI.Xaml.Media.BrushMappingMode MappingMode { get; set; };

    [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
    [MUX_DEFAULT_VALUE("winrt::Windows::UI::Composition::CompositionColorSpace::Auto")]
    Windows.UI.Composition.CompositionColorSpace InterpolationSpace { get; set; };

    [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
    [MUX_DEFAULT_VALUE("winrt::GradientSpreadMethod::Pad")]
    Windows.UI.Xaml.Media.GradientSpreadMethod SpreadMethod { get; set; };

    Windows.Foundation.Collections.IObservableVector<Windows.UI.Xaml.Media.GradientStop> GradientStops { get; };

    static Windows.UI.Xaml.DependencyProperty EllipseCenterProperty { get; };
    static Windows.UI.Xaml.DependencyProperty EllipseRadiusProperty { get; };
    static Windows.UI.Xaml.DependencyProperty GradientOriginOffsetProperty { get; };
    static Windows.UI.Xaml.DependencyProperty InterpolationSpaceProperty { get; };
    static Windows.UI.Xaml.DependencyProperty MappingModeProperty { get; };
    static Windows.UI.Xaml.DependencyProperty SpreadMethodProperty { get; };
}
```

