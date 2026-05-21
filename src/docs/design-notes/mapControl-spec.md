# Map Control API Spec 

## Background
WinUI3 does not currently include Map Control support, as WinUI2 does, creating a gap between
these two frameworks. This control was previously a native control. 

The developer community as well as some internal teams have been requesting a Map Control
in WinUI3 for some time, as this has been preventing them from moving to the newest framework, 
forcing them to stay with previous versions. Existing WinUI3 customers can not add maps functionality 
to their apps.

The decision was made to drop support for all different maps previously supported
and focus on Azure Maps. Since Azure Maps will be the long-term supported map
moving forward, we decided to utilize it for our WinUI3 control by having it inside a webView.

## Compatibility
As this control is already being used by developers in WinUI2, we plan to make this API
similar to the existing version. Additionally, since this control will be using Azure Maps,
we want to ensure seamless communication between native and web, having the same structure
of elements in the map. This will reduce potential issues and development costs.

Nonetheless, this will still include changes in the license and authentication to use the control. 
As previously mentioned, WinUI2 currently uses Bing Maps keys, and this new control will be 
requiring Azure Maps keys. Furthermore, 3D, Streetside, and offline modes are not supported 
in Azure Maps and consequently will not be supported in WinUI 3 version.

|  | WinUI2 | WinUI 3 |
| ---------- |----------|----------|
| Map Source | Bing Maps    | Azure Maps   |
| Namespace | Windows.UI.Xaml.\*    | Microsoft.UI.Xaml.\*    |

# Sample scenarios

### **Scenario 1: Showing a map and setting initial location**
XAML
```xaml
<MapControl x:Name="map1" />
```

C#
```csharp
map1.MapServiceToken = "<AzureMapsToken>";

BasicGeoposition position = new BasicGeoposition { Latitude = 43.076298, Longitude = -89.399864 };
Geopoint point = new Geopoint(position);

map1.Center = point;
```

### **Scenario 2: Showing a pin on the map**

C#

```csharp
public void AddIcon()
{
    var myLandmarks = new List<MapElement>();

    BasicGeoposition position = new BasicGeoposition { Latitude = -30.034647, Longitude = -51.217659 };
    Geopoint point = new Geopoint(position);

    var icon = new MapIcon
    {
        Location = point,
    };

    MyLandmarks.Add(icon);

    var LandmarksLayer = new MapElementsLayer
    {
        MapElements = MyLandmarks
    };

    map1.Layers.Add(LandmarksLayer);

    map1.Center = point;
    map1.ZoomLevel = 14;
}
```
![Scenario 2: Showing a pin on the map](images\mapcontrol-scenario2.png)

# API Pages
The [BasicGeoposition](https://learn.microsoft.com/en-us/uwp/api/windows.devices.geolocation.basicgeoposition?view=winrt-22621) 
and [Geopoint](https://learn.microsoft.com/en-us/uwp/api/windows.devices.geolocation.geopoint?view=winrt-22621) 
classes come from the Windows.Devices.Geolocation namespace.


### **MapControl class**
Represents a symbolic map of the Earth.
```csharp
public class MapControl: Control
```
### MapControl.MapServiceToken property
Gets or sets the authentication key required for using the MapControl and online mapping services. 
See [Azure Maps Docs](https://learn.microsoft.com/en-us/azure/azure-maps/how-to-manage-account-keys) on steps to obtain 
a shared key for authentication. Invalid token will prevent authentication with Azure Maps service, map control will 
raise a map service error and only show a blue background.
```csharp
public String MapServiceToken{ get;set; };
```
### MapControl.Center property
Gets or sets the center of the map
```csharp
public Windows.Devices.Geolocation.Geopoint Center{ get;set; };
```
### MapControl.Layers property
Gets or sets the collection of MapLayer objects that are children of the MapControl. Each layer contains `MapElements` 
rendered on top of the map. First layer in the vector is bottommost layer rendered in the map.
```csharp
public Windows.Foundation.Collections.IVector<MapLayer> Layers{ get;set; };
```
### MapControl.ZoomLevel property
Gets or sets the zoom level of the map, which is a value between 0 and 24. This is calculated using a 
[Spherical Mercator](https://learn.microsoft.com/en-us/azure/azure-maps/zoom-levels-and-tile-grid?tabs=csharp) projection 
coordinate system.
```csharp
public double ZoomLevel{ get;set; };
```
### MapControl.InteractiveControlsVisible property
Gets or sets a value indicating if the map is rendering the default interactive controls from azure maps as an overlay. 
Controls included are zoom, rotation, pitch and style.
```csharp
public bool InteractiveControlsVisible{ get;set; };
```
### MapControl.MapElementClick event
Occurs when the user taps or clicks a MapElement on the MapControl
```csharp
public event TypedEventHandler<MapControl, MapElementClickEventArgs> MapElementClick;
```
### MapControl.MapServiceErrorOccurred event
Occurs when communicating with azure maps web service fails.
This event can be raised at any time, not synchronously with the API call made to trigger it.
```csharp
public event TypedEventHandler<MapControl, MapControlMapServiceErrorOccurredEventArgs> MapServiceErrorOccurred;
```

### **MapElement class**
Represents an element displayed on a MapControl

_Spec note:_  
_This MapElement class serves as a base-class, and the only sub-class is MapIcon. So in a sense this baseclass is 
redundant. However, we are copying this pattern from WUX.Maps. Classes like MapPolygon will be added in the future and 
will also be inheriting from MapElement._
```csharp
public class MapElement : Microsoft.UI.Xaml.DependencyObject
```

### **MapIcon class**
Displays an icon on a MapControl. The icon is a pushpin.
```csharp
public class MapIcon : Microsoft.UI.Xaml.Controls.MapElement
```
### MapIcon.Location property
Gets or sets the geographic location of the MapIcon on the MapControl
```csharp
public Windows.Devices.Geolocation.Geopoint Location{ get;set; };
```

### **MapLayer class**
Represents a collection of map data to which you can bind data and manipulate independently of other map types of map data

_Spec note:_  
_This MapLayer class serves as a base-class, and the only sub-class is MapElementsLayer. So in a sense this baseclass is 
redundant. However, we are copying this pattern from WUX.Maps._

```csharp
public class MapLayer : Microsoft.UI.Xaml.DependencyObject
```

### **MapElementsLayer class**
Represents a collection of map elements to which you can bind data and manipulate independently of other map elements
```csharp
public class MapElementsLayer : Microsoft.UI.Xaml.Controls.MapLayer
```
### MapElementsLayer.MapElements property
Gets the collection of MapElement objects that are children of the MapElementsLayer
```csharp
public Windows.Foundation.Collections.IVector<MapElement> MapElements{ get;set; };
```
### MapElementsLayer.MapElementClick event
Occurs when the user taps or clicks a MapElement that has been add to the MapElementsLayer.
This is raised following the MapControl.MapElementClick event.
```csharp
event Windows.Foundation.TypedEventHandler<MapElementsLayer, MapElementClickEventArgs> MapElementClick;
```

### **MapElementClickEventArgs class**
Provides data for the MapElementClick event
```csharp
public class MapElementClickEventArgs
```
### MapElementClickEventArgs.Location property	
Gets the geographic location that corresponds to where the MapElementsLayer received user input
```csharp
public Windows.Devices.Geolocation.Geopoint Location{ get; };
```
### MapElementClickEventArgs.Element property
Gets the map element that corresponds to where the MapElementsLayer received user input
```csharp
public MapElement Element{ get; };
```

### **MapControlMapServiceErrorOccurredEventArgs class**
Provides data for the MapControlWebRuntimeErrorOccurred event
```csharp
public class MapControlMapServiceErrorOccurredEventArgs
```
### MapControlMapServiceErrorOccurredEventArgs.DiagnosticMessage property	
Gets error string returned from the web exception. This is for diagnostic purposes only, 
format of the string is not reliable and is subject to change.
```csharp
public String DiagnosticMessage{ get; };
```


# API Details
```csharp
namespace Microsoft.UI.Xaml.Controls
{
    unsealed runtimeclass MapControl : Microsoft.UI.Xaml.Controls.Control
    {
        MapControl();

        String MapServiceToken{ get;set; };
        Windows.Devices.Geolocation.Geopoint Center{ get;set; };
        Windows.Foundation.Collections.IVector<MapLayer> Layers{ get;set; };
        Double ZoomLevel{ get;set; };
        Boolean ShowControls{ get;set; };

        static Microsoft.UI.Xaml.DependencyProperty LayersProperty{ get; };
        static Microsoft.UI.Xaml.DependencyProperty MapServiceTokenProperty{ get; };
        static Microsoft.UI.Xaml.DependencyProperty CenterProperty{ get; };
        static Microsoft.UI.Xaml.DependencyProperty ZoomLevelProperty{ get; };
        static Microsoft.UI.Xaml.DependencyProperty ShowControlsProperty{ get; };

        event Windows.Foundation.TypedEventHandler<MapControl, MapElementClickEventArgs> MapElementClick;
        event Windows.Foundation.TypedEventHandler<MapControl, MapControlMapServiceErrorOccurredEventArgs> MapServiceErrorOccurred;
    };

    unsealed runtimeclass MapElement : Microsoft.UI.Xaml.DependencyObject
    {
    };

    unsealed runtimeclass MapIcon : Microsoft.UI.Xaml.Controls.MapElement
    {
        MapIcon();
        Windows.Devices.Geolocation.Geopoint Location{ get;set; };

        static Microsoft.UI.Xaml.DependencyProperty LocationProperty{ get; };
    };

    unsealed runtimeclass MapLayer : Microsoft.UI.Xaml.DependencyObject
    {
    };

    unsealed runtimeclass MapElementsLayer : Microsoft.UI.Xaml.Controls.MapLayer
    {
        MapElementsLayer();

        Windows.Foundation.Collections.IVector<MapElement> MapElements{ get;set; };
        static Microsoft.UI.Xaml.DependencyProperty MapElementsProperty{ get; };

        event Windows.Foundation.TypedEventHandler<MapElementsLayer, MapElementClickEventArgs> MapElementClick;
    };

    runtimeclass MapElementClickEventArgs
    {
        Windows.Devices.Geolocation.Geopoint Location{ get; };
        MapElement Element{ get; };
    };
}
```