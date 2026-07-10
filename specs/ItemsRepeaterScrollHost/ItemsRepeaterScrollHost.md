# Background
[The Windows UI Library](https://docs.microsoft.com/en-us/uwp/toolkits/winui/) (**WinUI**, aka "MUX")
ships a set of Xaml controls in Nuget, rather than as part of Windows. Apps can use the
controls in this package and target down to RS1.

WinUI includes the Xaml 
[ItemsRepeater](https://docs.microsoft.com/en-us/uwp/api/microsoft.ui.xaml.controls.itemsrepeater)
control, which shows a collection of items, like a more primitive ListView control. Typically 
lists are large and need to be scrolled, so it's typically put into a
[ScrollViewer](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ScrollViewer) control:

```xaml
<ScrollViewer>
    <ItemsRepeater ... />
</ScrollViewer>
```

When a scrolled list changes or is resized, the item which the user is currently looking at shouldn't move
or disappear. This item is called the anchor element, so ItemsRepeater and ScrollViewer coordinate
to preserve the location of the anchor element on the screen (that is, in the view port).

This coordination between ItemsRepeater and ScrollViewer is enabled by ScrollViewer implementing
[IScrollAnchorProvider](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.IScrollAnchorProvider).
But ScrollViewer ships in WUX, and didn't implement that interface until RS5, and
so anchoring doesn't work for an ItemsRepeater on an app running on RS4 or below.

The solution for this is the new `ItemsRepeaterScrollHost` control in this spec.
This wraps the ScrollViewer, and does the coordination with the ItemsRepeater.

```xaml
<ItemsRepeaterScrollHost>
    <ScrollViewer>
        <ItemsRepeater ... />
    </ScrollViewer>
</ItemsRepeaterScrollHost> 
```

# Description
<!-- Use this section to provide a brief description of the feature.
For an example, see the introduction to the PasswordBox control 
(http://docs.microsoft.com/windows/uwp/design/controls-and-patterns/password-box). -->

Use the ItemsRepeaterScrollHost when you're using an 
[ItemsRepeater](https://docs.microsoft.com/en-us/uwp/api/microsoft.ui.xaml.controls.itemsrepeater) 
in a [ScrollViewer](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ScrollViewer),
and your app will run on versions of Windows prior to Windows 10 1809. If your app will only run
on versions of Windows 1809 or higher, there is no need to use this control.

# Examples
This example shows a scrollable list of people.

```xaml
<Page
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:controls="using:Microsoft.UI.Xaml.Controls">

    <controls:ItemsRepeaterScrollHost>
        <ScrollViewer>
            <controls:ItemsRepeater ItemsSource='{x:Bind PeopleCollection}' />
        </ScrollViewer>
    </controls:ItemsRepeaterScrollHost> 
    
</Page>
```

# Remarks
The ItemsRepeaterScrollHost must be the direct parent of the ScrollViewer.
The ItemsRepeater need not be the direct child of the ScrollViewer.

If you use an ItemsRepeaterScrollHost on a version of Windows at least as recent
as 1809, it will have no effect.

# API Notes
The ItemsRepeaterScrollHost properties have the
same functionality and behavior as the like named properties
on the RS5 ScrollViewer:

[HorizontalAnchorRatio](https://docs.microsoft.com/en-us/uwp/api/Windows.UI.Xaml.Controls.ScrollViewer.HorizontalAnchorRatio)

[VerticalAnchorRatio](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ScrollViewer.VerticalAnchorRatio)

[CurrentAnchor](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ScrollViewer.CurrentAnchor)

# API Details
```C#
[webhosthidden]
[contentproperty("ScrollViewer")]
runtimeclass ItemsRepeaterScrollHost : Windows.UI.Xaml.FrameworkElement
{
    ItemsRepeaterScrollHost();
    
    Windows.UI.Xaml.Controls.ScrollViewer ScrollViewer;
    
    Windows.UI.Xaml.UIElement CurrentAnchor { get; }    
    
    Double HorizontalAnchorRatio;
    Double VerticalAnchorRatio;
}
```
