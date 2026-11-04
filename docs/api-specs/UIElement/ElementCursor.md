UIElement.ProtectedCursor
===

# Background

_This spec adds a ProtectedCursor property to the Xaml [UIElement](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.UIElement) class_

Today in a WinUI app you can set the cursor by setting
[CoreWindow.PointerCursor](https://docs.microsoft.com/uwp/api/Windows.UI.Core.CoreWindow.PointerCursor). 

There are two basic scenarios for specifying the cursor for an element: the app author setting the cursor and the control (element) author.
An example of a control author setting it is
[HyperlinkButton](http://msdn.microsoft.com/library/Microsoft.UI.Xaml.Controls.HyperlinkButton),
which automatically sets the correct cursor, and doesn't require the app to specify it.
The API in this spec is just for that control author scenario, but with the expectation that we'll add the app case in the future.

WPF exposes this capability to control authors using the
[UIElement.OnQueryCursor](https://docs.microsoft.com/dotnet/api/System.Windows.UIElement.OnQueryCursor)
virtual method. You optionally override this and specify the cursor you want to use. UIElement then calls that virtual, quite frequently.
Rather than that virtual method approach, the solution being added to WinUI is a protected property.
That way the subclass need only set it during state changes (such as the
[UIElement.PointerEntered](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.UIElement.PointerEntered)
event).

In the future we expect to also have a _public_ Cursor property that the app developer can use.
So two cursor properties, one public and one protected. If both are set, the public property will override the protected property.

This isn't a pattern we have today.
The proposal here is to name them *Cursor* (future public property) and *ProtectedCursor* (protected property in this spec).

A note about WPF compatibility ... WPF has the
[Cursor property on FrameworkElement](https://docs.microsoft.com/dotnet/api/System.Windows.FrameworkElement.Cursor)
rather than UIElement. We put this new ProtectedCursor property on UIElement (and will put the future Cursor property there)
because cursors are "core", which is the intent of the UIElement/FrameworkElement split. And this is a source compatible difference.

# API Pages

## UIElement.ProtectedCursor

Gets or sets the cursor that displays when the  pointer is over this element. Defaults to null, indicating no change to the cursor.

(Type: [CoreCursor](https://docs.microsoft.com/uwp/api/Windows.UI.Core.CoreCursor)))

If a parent and descendant element both have this property set, and the pointer is over the descendent,
the descendant's value is used and the parent's value is ignored.

A pointer is 'over' an element if it hit-tests to the element or a child element.
A related example of this is the
[ButtonBase.IsPointerOver](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.Primitives.ButtonBase.IsPointerOver)
property.

An exception to this is if the pointer has been captured using the
[UIElement.CapturePointer](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.UIElement.CapturePointer)
method, which sends pointer input to the capturing element regardless of where it's located.
If pointer input is captured to an element or its tree, the pointer is "over" that element.

Note that even if a child of the element marks a pointer event Handled
([PointerEventArgs.Handled](https://docs.microsoft.com/uwp/api/Windows.UI.Core.PointerEventArgs.Handled)),
the ProtectedCursor property will still be used.

The following shows a custom Button control that has a Help cursor.

```cs
public class HelpButton : Button
{
    public HelpButton()
    {
        this.ProtectedCursor = new CoreCursor(CoreCursorType.Help, 0);
    }
}
```

# API Details

```cs
[webhosthidden]
unsealed runtimeclass UIElement : Microsoft.UI.Xaml.DependencyObject
{
  // ...
  Windows.UI.Core.CoreCursor ProtectedCursor;
}
```

