# Can Xaml host a ContentIsland?

Apps can use the XamlIsland API to host Xaml within another framework.

But can the Xaml framework host another framework using ContentIslands?

In WinAppSDK 1.7 we added the XamlRoot.ContentIsland API (in exp builds it was XamlRoot.TryGetContentIsland()).
This allows an app to use a ChildSiteLink to host a ContentIsland inside a XamlRoot (which represents any kind
of Xaml content).

The code would look something like this:

``` cs
    // Insert the ContentIsland "guestContentIsland" into the Xaml tree as a child of the "placementElement"
    // object.  The placementElement is the Xaml UIElement that will serve as a placeholder for the guest
    // ContentIsland.  However the placementElement is moved and resized in the Xaml scene, the guest 
    // ContentIsland will follow and have the same size.
    void InsertContentIslandIntoXamlScene(FrameworkElement placementElement, ContentIsland guestContentIsland)
    {
        // The placementVisual is a Visual that's a representation of the placementElement in the scene graph.
        // It will have the same position and size as the placementElement.
        var placementVisual = (ContainerVisual)ElementCompositionPreview.GetElementVisual(placementElement);

        // This ChildContentLink will manage the connection for us between the:
        //  * Parent ContentIsland (which we get from XamlRoot.TryGetContentIsland())
        //  * Placement Visual (which is the same size and location as placementElement)
        //  * Child ContentIsland
        // We save a ref to the ChildContentLink so we can manage it later if we want.
        _childSiteLink = ChildSiteLink.Create(
            placementElement.XamlRoot.TryGetContentIsland(),
            placementVisual);

        // We need to keep the size of the ChildContentLink in sync with the size of the placementElement
        // for UIA to work correctly.
        var sizeChangedEventHandler = new SizeChangedEventHandler((s,e) =>
        {
            _childSiteLink.ActualSize = new System.Numerics.Vector2(
                (float)placementElement.ActualWidth,
                (float)placementElement.ActualHeight);
        });
        placementElement.SizeChanged += sizeChangedEventHandler;
        sizeChangedEventHandler.Invoke(null, null);

        // We also need to keep the offset of the ChildContentLink within the parent ContentIsland in sync
        // with that of the placementElement for UIA to work correctly.
        var layoutUpdatedEventHandler = new EventHandler<object>((s, e) =>
        {
            var transform = placementElement.TransformToVisual(null);
            var point = transform.TransformPoint(new Windows.Foundation.Point(0, 0));
            _childSiteLink.LocalToParentTransformMatrix = System.Numerics.Matrix4x4.CreateTranslation(
                (float)(point.X),
                (float)(point.Y),
                0);
        });
        placementElement.LayoutUpdated += layoutUpdatedEventHandler;
        layoutUpdatedEventHandler.Invoke(null, null);

        // When the element is unloaded from the Xaml tree, let's clean up after ourselves.
        placementElement.Unloaded += (s, e) =>
        {
            placementElement.SizeChanged -= sizeChangedEventHandler;
            placementElement.LayoutUpdated -= layoutUpdatedEventHandler;
            _childSiteLink.Dispose();
        };

        // After we call Connect, the guest ContentIsland will be displayed in the Xaml scene.
        _childSiteLink.Connect(guestContentIsland);
    }
```

You can see this code in a "xaml-hosting-xaml" sample where we host a XamlIsland inside a WinUI Desktop app using
ChildSiteLink.

BUT there are some things we haven't figured out yet:
* **UIA**.  How can we hook up the UIA tree correctly between Xaml and the hosted content?  Xaml doesn't
appear to have a ready-made way to do this.  We do something like this for WebView2 support, but that uses
some custom code.  
* **Keyboard focus and Navigation**.  Again, Xaml doesn't really have an extensibility point for content
inside of it taking keyboard focus and negotiating when focus should come back to the outer Xaml content.
