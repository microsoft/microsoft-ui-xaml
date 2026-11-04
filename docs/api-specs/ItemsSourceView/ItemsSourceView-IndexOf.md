# Background
<!-- Use this section to provide background context for the new API(s) 
in this spec. -->
The Xaml [ItemsRepeater](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.ItemsRepeater) control displays a list of objects using a specified template (similar to a [ListView](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ListView)). You provide the list of objects with the [ItemsRepeater.ItemsSource](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.ItemsRepeater.ItemsSource) property. The list need only be an IIterable but can be an IVector for better perf. ItemsRepeater exposes a read-only [ItemsRepeater.ItemsSourceView](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.ItemsRepeater.ItemsSourceView) property that provides a view on this list (an [ItemsSourceView](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.ItemsSourceView)), which has helpers like Count and GetAt over what might be just an IIterable.

Currently the ItemsSourceView class does not expose an IndexOf function. However during development, this method was used multiple times internally on derivatives of the class. To make the usage of this existing function easier on subclasses, this spec adds an IndexOf function to the ItemsSourceView.

# Description

The IndexOf function returns the index of the given element in the items source the ItemsSourceView is used for.
Note that if an element is not contained in the items source, the function will return -1.
> Spec note: returning -1 matches the behavior of for example IndexOf on [UIElementCollection](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.UIElementCollection) and List<T>.
# Examples
C# example

```c#
var items = new List<string>() { "One", "Two" };
var itemsSourceView = new ItemsSourceView(items);
var secondItem = itemsSourceView.GetAt(1);
Assert.AreEqual(1, itemsSourceView.IndexOf(secondItem));
```


# Remarks

If an element is not present in the collection, the method returns -1.

# API Notes
ItemsSourceView.IndexOf(IInspectable item): Returns index in the items source. If an item is not in the items source, it will return -1. Null is an acceptable value for IndexOf and calling IndexOf with null will return the index of null in the ItemsSource or -1 if the ItemsSource does not contain null.

If the ItemsSourceView's underlying collection supports a compatible `IndexOf`, it will be used and that value returned. Otherwise the first matching value from its enumeration will be returned. Note in this case that for strings and value types, -1 will always be returned, because items will be compared by boxed references rather than the values.

# API Details
<!-- The exact API, in MIDL3 format (https://docs.microsoft.com/en-us/uwp/midl-3/) -->

```MIDL

unsealed runtimeclass ItemsSourceView: Windows.UI.Xaml.Interop.INotifyCollectionChanged
{
     // ...
    Int32 IndexOf(IInspectable item);

}

```

# Appendix
