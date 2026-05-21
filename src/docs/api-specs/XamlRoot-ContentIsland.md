<!--
    This md file used template: api-review-process.md
-->

XamlRoot.ContentIsland
===

# Background

ContentIsland APIs are the preferred way to host UI from
UI framework B within UI framework A.  In all supported Xaml scenarios, the Xaml framework uses a ContentIsland to connect
Xaml content to the rest of the scene graph.  It's how Xaml exposes all its input, output, and accessibility.

As we enable more hosting scenarios, the ContentIsland is a crucial object.  We're continuing to add to its API surface.

We could decide to just expose bits of functionality from the ContentIsland through Xaml as we go, but that would require
significant churn on the Xaml API surface as we enable new features. It would also leave us with an awkward API shape, where
there's a lot of duplication between the ContentIsland and Xaml APIs.

Instead, the new **XamlRoot.ContentIsland** API exposes the entire ContentIsland to the app author.  This allows us to add APIs
and enable new scenarios without churning the Xaml API.  At first, not all of the ContentIsland API will be useful to the
Xaml developer.  It's like how DesktopWindowXamlSource exposes an HWND: it is a powerful API that enables many important scenarios,
but there are some things the app author should not do with that HWND.  We just don't support all the things you could do with
it.  Similarly, the ContentIsland object from Xaml is immediately useful in some ways, but there are some applications of
the ContentIsland that aren't yet fully supported.

# API Pages

_(Each of the following L2 sections correspond to a page that will be on docs.microsoft.com)_

## XamlRoot.ContentIsland Property

**XamlRoot.ContentIsland** gives you access to the underlying **ContentIsland** object that is connecting the
**XamlRoot** content to the rest of the WinAppSDK Scene Graph.  For example, if you're using a XamlIsland object to host
Xaml content within another UI framework, you'll use XamlIsland's ContentIsland to connect your XamlIsland to its host.
Xaml always depends on ContentIsland for coordinating its input, output, layout, and accessibility, so whenever you have
live Xaml content, **XamlRoot.ContentIsland** will be valid.

**XamlRoot.ContentIsland** will be null if it's associated with a XamlIsland that's been closed.  

You shouldn't modify the state of the returned **ContentIsland** or its underlying objects.  For example, you shouldn't
call Close or Dispose on these objects.

The **XamlRoot.ContentIslandEnvironment** property returns the same object as **XamlRoot.ContentIsland.Environment**, and the
**XamlRoot.CoordinateConverter** property returns the same object as **XamlRoot.ContentIsland.CoordinateConverter**.

Here's one example of how you could use the **ContentIsland** object:  If you need to find the screen coordinates of a
given Xaml element, you can get a **CoordinateConverter** object from the **ContentIsland** and call **ConvertLocalToScreen**
(see example code).

```c#
// This transform will convert a point in the coordinate space of "myButton" to the
// coorindate space of the Xaml tree.
GeneralTransform transform = myButton.TransformToVisual(null);

// This is the postiion of myButton in the Xaml tree.
Point localCoordinates = transform.TransformPoint(new Windows.Foundation.Point(0, 0));

// The ContentIsland is the object that connects the Xaml visuals to the rest of the scene.
ContentIsland contentIsland = myButton.XamlRoot.ContentIsland;

// Get the screen coordinates.  This could be useful when talking to UIAutomation
// or other APIs.
Windows.Graphics.PointInt32 screenCoordinates = 
    contentIsland.CoordinateConverter.ConvertLocalToScreen(localCoordinates);

Debug.WriteLine($"screenCoordinates: {screenCoordinates.X}, {screenCoordinates.Y}");
```

# API Details

```c# (but really MIDL3)
    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    runtimeclass XamlRoot
    {
        // ===== NEW =====

        [contract(Microsoft.UI.Xaml.WinUIContract, 8)]
        {
            Microsoft.UI.Content.ContentIsland ContentIsland{ get; };
        }

        // ===== Already existing in WinAppSDK 1.6 =====

        Microsoft.UI.Xaml.UIElement Content{ get; };
        Windows.Foundation.Size Size{ get; };
        Double RasterizationScale{ get; };
        Boolean IsHostVisible{ get; };
        event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.XamlRoot,Microsoft.UI.Xaml.XamlRootChangedEventArgs> Changed;
    
        [contract(Microsoft.UI.Xaml.WinUIContract, 5)]
        {
            Microsoft.UI.Content.ContentIslandEnvironment ContentIslandEnvironment{ get; };
        }
    
        [contract(Microsoft.UI.Xaml.WinUIContract, 7)]
        {
            Microsoft.UI.Content.ContentCoordinateConverter CoordinateConverter{ get; };
        }
    };
```