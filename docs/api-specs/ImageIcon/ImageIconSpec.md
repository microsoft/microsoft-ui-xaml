# Image Icon

## Background

Today, WinUI (Xaml) has a way to add icons in different formats using the elements that inherit from 
[IconElement](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.IconElement).
These are: BitmapIcon, FontIcon, SymbolIcon, PathIcon, and IconSourceElement.

BitmapIcon supports a set of bitmap types such as png and jpeg
(the same set supported by 
[BitmapImage](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Media.Imaging.BitmapImage)).
The general [Image](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.Image)
element on the other hand supports these and more types, plus supports loading types from
a stream rather than just URI.

The new API in this spec is an `ImageIcon` type, which like the other icons is an IconElement
(so can be used where IconElement is required, such as 
[NavigationViewItem.Icon](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.NavigationViewItem.Icon)).
ImageIcon supports all of the image formats that ImageSource supports. 

ImageIcon is a superset of the existing BitmapIcon minus the
[BitmapIcon.ShowAsMonochrome](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.BitmapIcon.ShowAsMonochrome)
property, as it is not a requirement from design to have all icons be monochrome.
Since ShowAsMonochrome will not be part of ImageIcon, the 
[IconElement.Foreground](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.IconElement.Foreground)
inherited from IconElement will be ignored by this control.
It will be up to the application to update the icon to match the user's theme. 

Along with the various IconElement types, which are elements, there are IconSource types.
For example BitmapIcon and
[BitmapIconSource](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.BitmapIconSource).
Similarly in this spec, along with `ImageIcon` is an `ImageIconSource`.

In summary:

```
IconElement
    BitmapIcon
    ImageIcon **new

IconSource
    BitmapIconSource
    ImageIconSource **new

FrameworkElement
    Image
```

# API Pages

## ImageIcon class

Represents an icon that uses an image as its content.

ImageIcon can be used just like an
[Image](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.Image)
element, except it derives from
[IconElement](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.IconElement)
and can therefore be used anywhere an IconElement is required, such as
[NavigationViewItem.Icon](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.NavigationViewItem.Icon).

> Note: the ImageIcon.Foreground property is currently ignored.

## Is this the right control? 

Use an ImageIcon control when you want to display an image as your icon. Do not use an ImageIcon if you want to display an icon from a font or if you want the image to be displayed as monochrome. 

## Examples

### Adding an ImageIcon to an AppBarButton
Below is an example of how you can use ImageIcon in an App Bar Button Item.

> Spec note: This example will also be added to the App Bar Button Item page with the rest of the icon controls. 

XAML
```xml
<AppBarButton Label="ImageIcon" Click="AppBarButton_Click">
    <AppBarButton.Icon>
        <ImageIcon Source="Wheel.svg"/>
    </AppBarButton.Icon>
</AppBarButton>
```

Similar to ImageIcon, ImageIconSource can also be used in an AppBarButton. Below is an example of how to use the ImageIconSource control. 

XAML
```xml
<AppBarButton >
    <AppBarButton.Icon>
        <IconSourceElement>
            <ImageIconSource ImageSource="ms-appx:///Assets/globe.png"/>
        </IconSourceElement>
    </AppBarButton.Icon>
</AppBarButton>
```

### Adding an ImageIcon from a strem

This example shows how to load an ImageIcon from a stream for either a bitmap stream or an svg stream. You can then add your icon to the markup like in the previous example. 

C#
```cs
// using a stream to a bitmap (png, jpg)
var source = new BitmapSource();
await source.SetSourceAsync(bitmapStream);
var icon = new ImageIcon() { Source = source };

// using a stream to an svg
var source = new SvgImageSource();
await source.SetSourceAsync(svgStream);
var icon = new ImageIcon() { Source = source };
```

If you choose to use the ImageIconSource, here is how you can load an image from a stream. 

C#
```cs
var source = new SvgImageSource();
await source.SetSourceAsync(svgStream);
new ImageIconSource() { Source = svgSource };
```

## ImageIcon member notes

| Name	| Type | Description | Default |
|:--- | :--- | :--- | :--- |
|Source | ImageSource | Gets and sets the icon source file that will be used in the application. | Null |

## ImageIconSource class

_Same as above but with IconSource as the base type_

# API Details

```cs
namespace Microsoft.UI.Xaml.Controls
{
    [webhosthidden]
    unsealed runtimeclass ImageIcon : Windows.UI.Xaml.Controls.IconElement
    {
        ImageIcon();

        Windows.UI.Xaml.Media.ImageSource Source{ get; set; };

        static Windows.UI.Xaml.DependencyProperty SourceProperty{ get; };
    }

    [webhosthidden]
    unsealed runtimeclass ImageIconSource : IconSource
    {
        ImageIconSource();

        Windows.UI.Xaml.Media.ImageSource ImageSource{ get; set; };

        static Windows.UI.Xaml.DependencyProperty ImageSourceProperty{ get; };
    }
}
```

# Appendix

## Scope

|Capability	| Priority |
|:--- | :--- |
| Icon can be used anywhere a BitmapIcon is accepted today | Must|
| PNG, JPEG, and SVG images are types the control will accept | Must|
| Icon must respond to user theming changes (Light, Dark, and High Contrast) | Could |
| ICO icons will be accepted by the control | Could |
| SVG image support will be improved | Won't |


## Accessibility

Since ImageIcon does not respond to the foreground property inherited from IconElement,
it is up to the application to handle the icon color changes when the user’s theme is changed.  

## Open Questions

- Should ICOs be supported in the future? 
- Should the ShowAsMonochrome property be added in the future? 
This is somehing that was added because of a design requirement back when BitmapIcon was first designed but it is no longer a hard design requirement. 

