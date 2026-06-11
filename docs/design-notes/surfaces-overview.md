# Xaml's usage of surfaces

This document describes Xaml's usage of Composition and D3D surfaces. The contents will be obsolete (and updated) as Xaml moves to WinRT ICompositionDrawingSurfaceInterop.

## Table of Contents

- [Composition Surfaces](#composition-surfaces)
  - [Gutters](#gutters)
  - [MockDComp](#mockdcomp)
  - [Surface factories](#surface-factories)
- [Xaml Wrappers](#xaml-wrappers)
  - [HWRealization](#hwrealization)
  - [HWTexture/HWRgbTexture](#hwtexturehwrgbtexture)
  - [DCompSurface](#dcompsurface)
  - [SystemMemoryBits](#systemmemorybits)
- [Scenarios](#scenarios)
  - [Alpha Masks](#alpha-masks)
    - [GetAlphaMask](#getalphamask)
  - [Images](#images)
  - [\[Virtual\]SurfaceImageSource](#virtualsurfaceimagesource)
  - [LoadedImageSurface](#loadedimagesurface)
- [Room for improvement](#room-for-improvement)
  - [WinRT surface gaps](#winrt-surface-gaps)
- [Useful breakpoints](#useful-breakpoints)

Xaml renders content using WinRT SpriteVisuals. For anything more complicated than a (optionally rounded) rectangle filled/stroked with a solid color or a linear gradient, surfaces need to be involved. This means:
* All text and most shapes, which draw into an alpha mask surface
* All images, which load into a surface and is used in a brush to fill a visual
* [Virtual]SurfaceImageSource, which allows the app to draw using D3D into a surface that's shown in a Xaml ImageBrush
* LoadedImageSurface, a Composition interop feature that makes it easy to load content into a surface that can be fed into a Composition object graph

## Composition Surfaces

Composition surfaces are [atlased](https://en.wikipedia.org/wiki/Texture_atlas) behind the scenes. The atlas is a large D3D texture that is divided into smaller rects and handed out separately. Having a single large atlas surface rather than multiple separate surfaces makes D3D rendering much more efficient. D3D requires its device to be put into the correct state for each draw call, and part of that state is the surface used to fill the primitives. Having a single atlas surface means we can set it once on the device then issue many different draw calls for the primitives of the entire tree, rather than having to constantly switch state for each surface that we use.

The Composition surface API has two important methods - [BeginDraw](https://docs.microsoft.com/en-us/windows/win32/api/windows.ui.composition.interop/nf-windows-ui-composition-interop-icompositiondrawingsurfaceinterop-begindraw) and [EndDraw](https://docs.microsoft.com/en-us/windows/win32/api/windows.ui.composition.interop/nf-windows-ui-composition-interop-icompositiondrawingsurfaceinterop-enddraw).

BeginDraw is called before we update the surface. It takes a subrect of the surface that is getting updated. Note that "subrect" here is relative to the surface, and not relative to the atlas where the surface lives. Whenever Xaml draws an alpha mask, we draw the entire mask, so we can pass null in for the subrect. BeginDraw also takes an interface guid for the object that it returns. Xaml asks for an IDXGISurface that we later QI to an ID3D11Resource. BeginDraw returns the object for the surface along with an offset of where in that surface Xaml should start drawing. The returned surface is the entire atlas, with a clip applied to prevent Xaml from drawing outside the area where the surface lives.

After Xaml retrieves the surface via BeginDraw, we call `ID3D11DeviceContext::CopySubresourceRegion` to fill in the surface.

EndDraw is called after we're done updating the surface.

### Gutters

Gutters are an optimization associated with atlasing. When multiple subrects are put in the same atlas, we have to leave space between the various subrects so that they don't sample into each other around antialiased edges.

This space is filled with duplicated pixels from the outside edge of the texture so that antialiasing can produce the correct result. The responsibility of drawing into this space can go to either Xaml or Composition. Filling the gutters with Xaml is faster. It means that instead of drawing into a subrect of the correct size, we expand the subrect a bit to include the gutter regions as well and draw into all of it, filling in the gutters in the process. This is done with the private BeginDrawWithGutters API. Filling the gutters with Composition means that Composition makes additional draw calls to copy the edges of the surface after Xaml is done drawing. This incurs overhead from those extra draw calls. This is done with the normal BeginDraw API.

Xaml has an additional optimization where we leave the gutters empty instead of copying the outside edge of pixels to save additional copy time. We do this with text alpha masks, because text masks are typically completely transparent around the edges anyway. The parts that aren't transparent are usually antialiased glyph edges and are already partially transparent, so blending in more transparent pixels for antialiasing won't be that noticeable.

### Composition Surface Mock

The mock composition implementation can mock legacy IDCompositionSurfaces and output the pixels. After switching to WinRT surfaces, ICompositionDrawingSurfaceInterop is used instead.

### Surface factories

Composition surfaces are backed by a D2D or D3D surface, associated with a D2D or D3D device. Normally we just use Xaml's D2D device, but for [Virtual]SurfaceImageSource scenarios we want to use the device that the app has passed in for that [Virtual]SurfaceImageSource. The legacy Composition API that does this is a surface factory, which is created from the legacy IDCompositionDevice and a D2D/DXGI device. That surface factory can then create surfaces on the D2D/DXGI device. The WinRT Composition API that does this is ICompositionGraphicsDevice, which is created from the compositor and a D2D/DXGI device. The ICompositionGraphicsDevice can then create surfaces on the D2D/DXGI device.

The legacy IDCompositionSurfaceFactoryPartner3 interface also allows surfaces to be offered up. An offered surface can be reclaimed when the system is under memory pressure, which can prevent a UWP in the background from being terminated by the system to reclaim memory. Xaml offers surfaces on PLM suspend and reclaims them on PLM resume. A surface may fail to be reclaimed, in which case we need to discard it and re-create it. The offer/reclaim mechanism is not currently available via WinRT Composition APIs.

## Xaml Wrappers

Xaml has many layers of abstraction over Composition and D3D surfaces. Composition surfaces are the objects that get used in a tree of Composition visuals, while D3D textures are what the D2D rasterizer/image decoder work with. We update a scratch D3D texture as we render and decode, then copy it into a Composition surface at the end of the frame. Updating all the composition surfaces together avoids tearing issues where the surface is updated but the associated visuals aren't.

### HWRealization

The first link in the chain from a Xaml UIElement to an alpha mask is a HWRealization. A HWRealization stores both the alpha mask and a transform used to generate it. In order to render crisp text and shapes on large display scales, we render the mask itself at the larger scale to avoid having to stretch it up later. When we do a render walk, we'll match the current display scale against the scale stored on the mask, and regenerate the mask if the scales don't match. Text masks have an additional constraint in their subpixel positioning - the fractional pixel offsets are baked into the mask itself, and the mask is drawn at a whole number offset. If the fractional offset changes, then the mask is regenerated as well.

### HWTexture/HWRgbTexture

HWTexture/HWRgbTexture is an additional layer of abstraction over Composition surfaces. This layer largely exists because Xaml itself used to do the atlasing. Atlasing now happens inside the Composition layer.

This layer currently handles thread safety for off-thread image decode, which decodes images directly into a hardware surface off of the UI thread.

### DCompSurface

This is the wrapper closest to the Composition surface. Xaml batches its updates into scratch textures before copying them into a composition surface at the end of a frame. Xaml currently uses a legacy IDCompositionSurface, which it then wraps in a WinRT ICompositionSurface created with CreateCompositionSurfaceForDCompositionSurface.

DCompSurface has a couple of sets of APIs:
* `DCompSurface::Lock/Unlock` - these APIs hook up to the scratch D3D texture and are used by Xaml to write into the DCompSurface. The bits are stored in a scratch D3D texture wrapped in SystemMemoryBits. Both D2D output (for text and shape alpha mask rendering) and image decoding write bits into the DCompSurface via Lock/Unlock.
* `DCompSurface::BeginDraw/EndDraw` - these APIs hook up to the Composition surface. They're used for the [Virtual]SurfaceImageSource BeginDraw/EndDraw APIs. Xaml also goes through BeginDraw/EndDraw on the underlying Composition surface when copying into them.

### SystemMemoryBits

SystemMemoryBits is a scratch surface used by Xaml before we copy into the Composition surface. This is a fairly low-level object that calls D3D APIs that control whether the pixels are available to the CPU or GPU. `SystemMemoryBitsDriver::Create` creates the D3D texture. We write/decode into this scratch surface before copying it to the Composition surface.

The important D3D APIs here are [ID3D11DeviceContext::Map](https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-map) and [ID3D11DeviceContext::Unmap](https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-unmap). Map makes a D3D texture and makes the pixels unavailable to the GPU, putting it in a state where we can write into it. Unmap makes the texture available to the GPU again, and invalidates the system memory pointer to the underlying bits in the process, making it unaccessible for writing.

SystemMemoryBitsDriver automatically maps the texture when it's created. When it comes time to copy to a Composition surface, we'll call Unmap on the D3D texture before issuing a CopySubresourceRegion.

## Scenarios

### Alpha Masks

Alpha masks are used for text and shapes. They're an A8 surface (having only an alpha channel) to save on memory. Each pixel is a value in the range [0, 255] indicating how opaque it is. These surfaces are used in conjunction with a solid color brush to draw something like black text or a blue circle.

An alpha mask is hooked up to an ICompositionSurfaceBrush, which is connected to a brush graph that's then set as the Brush of some SpriteVisual.

For example, the brush for black text might look like:
```
ICompositionMaskBrush
- ICompositionMaskBrush.Mask: ICompositionSurfaceBrush
  - ICompositionSurfaceBrush.Surface: alpha mask surface
- ICompositionMaskBrush.Source: ICompositionColorBrush
  - ICompositionColorBrush.Color: black
```

The brush for a rounded corner border might look like:
```
ICompositionMaskBrush
- ICompositionMaskBrush.Mask: ICompositionNineGridBrush
  - ICompositionNineGridBrush.Source: ICompositionSurfaceBrush
    - ICompositionSurfaceBrush.Surface: alpha mask surface
- ICompositionMaskBrush.Source: ICompositionColorBrush
  - ICompositionColorBrush.Color: gray
```

ICompositionSurfaceBrush.Surface accepts an ICompositionSurface, so that's what Xaml has to provide. We currently take a legacy IDCompositionSurface and wrap it in an ICompositionSurface with CreateCompositionSurfaceForDCompositionSurface.

#### GetAlphaMask

Alpha masks are also exposed via the GetAlphaMask method. This is a deceivingly non-virtual method. Rather than have `UIElement::GetAlphaMask`, there are instead three different methods that are all named GetAlphaMask:
* [TextBlock::GetAlphaMask](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.controls.textblock.getalphamask?view=winui-3.0)
* [Shape::GetAlphaMask](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.shapes.shape.getalphamask?view=winui-3.0)
* [Image::GetAlphaMask](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.controls.image.getalphamask?view=winui-3.0)

These methods return a CompositionBrush that can be plugged into a Composition object graph (e.g. connected to an effect, a complex brush, or a visual). Under the covers this is also an ICompositionSurfaceBrush, so Xaml handles it the same way via an ICompositionSurface.

Alpha masks draw directly into the Composition surface, rather than into a scratch texture that's later copied into the Composition surface. `D2DTextDrawingContext::HWBuildGlyphRunSingleTexture` (for text) and `AlphaMask::Impl::RasterizeElement` (for shapes) call DCompSurface's BeginDraw/EndDraw.

### Images

Xaml decodes images off of the UI thread into Composition surfaces. Those same surfaces are used when rendering Image elements and ImageBrushes in the tree.

ImagingUtility's CopyToHardwareTiles decodes the encoded image into a temporary buffer, then updates the HWTexture wrapper with the decoded image pixels in the temporary buffer. This can be done off of the UI thread, and directly into the surfaces that back the Xaml BitmapSource object.

Images go through the scratch texture since off-thread decode can take multiple frames to complete. ImagingUtility's `CopyToHardwareTiles` calls DCompSurface's LockRect/UnlockRect. Note that we call unlock with update=true only after decoding the last tile, which means we don't queue the copy from the scratch texture to the Composition surface until the entire image is done decoding.

### [Virtual]SurfaceImageSource

Xaml's [SurfaceImageSource](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.media.imaging.surfaceimagesource?view=winui-3.0) exposes BeginDraw/EndDraw directly to apps to let them draw into a surface with D3D/D2D. We plumb the BeginDraw/EndDraw calls through Xaml's wrappers into the underlying Composition surface.

[VirtualSurfaceImageSource](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.media.imaging.virtualsurfaceimagesource?view=winui-3.0) does everything that SurfaceImageSource does, but it's a virtual surface. This means the app can create the VSIS at whatever size, unlimited by the max texture size of the system. The app must specify where in the virtual surface they're drawing. Xaml has a mechanism to tell the app which parts of the surface are visible. The app calls [IVirtualSurfaceImageSourceNative::RegisterForUpdatesNeeded](https://docs.microsoft.com/en-us/windows/win32/api/windows.ui.xaml.media.dxinterop/nf-windows-ui-xaml-media-dxinterop-ivirtualsurfaceimagesourcenative-registerforupdatesneeded?redirectedfrom=MSDN), passing in a callback interface. Xaml calls out to the interface's UpdatesNeeded method whenever the visible rect changes. In UpdatesNeeded, the app must call GetUpdateRectCount/GetUpdateRects to retrieve the rects of the VSIS that are now visible on screen. The app can then call BeginDraw on those rects to fill in content. See [VirtualSurfaceImageSource sample](https://docs.microsoft.com/en-us/previous-versions/windows/apps/hh825871(v=win.10)#virtualsurfaceimagesource) on MSDN for an example.

[Virtual]SurfaceImageSource requires the app to pass in a D2D or D3D device to use for drawing, via the [ISurfaceImageSourceNative::SetDevice](https://docs.microsoft.com/en-us/windows/win32/api/windows.ui.xaml.media.dxinterop/nf-windows-ui-xaml-media-dxinterop-isurfaceimagesourcenative-setdevice) or [ISurfaceImageSourceNativeWithD2D::SetDevice](https://docs.microsoft.com/en-us/windows/win32/api/windows.ui.xaml.media.dxinterop/nf-windows-ui-xaml-media-dxinterop-isurfaceimagesourcenativewithd2d-setdevice) APIs. Xaml then creates its Composition surfaces on the device that the app passes in. This is done via surface factories in legacy DComp surfaces, and will be done via ICompositionGraphicsDevice with WinRT surfaces.

VirtualSurfaceImageSource has an optimization that trims down the unnecessary rects in the backing atlas, reclaiming memory for parts of the image that aren't visible on screen. This is done via a legacy IDCompositionVirtualSurface API, and isn't available in WinRT.

### LoadedImageSurface

Xaml's [LoadedImageSurface](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.media.loadedimagesurface?view=winui-3.0) provides a way to interop with Composition APIs that expect an ICompositionSurface, like effects or surface brushes. The app can populate a LIS from a URI or a stream of an encoded image. Xaml handles image decoding on a background thread and raises LoadCompleted when the surface is ready.

LoadedImageSurface implements Composition's [ICompositionSurfaceFacade](https://docs.microsoft.com/en-us/uwp/api/windows.ui.composition.icompositionsurfacefacade?view=winrt-20348) interface, with a GetRealSurface method that returns the real underlying ICompositionSurface. GetRealSurface on a non-null facade isn't expected to return a null real surface. LoadedImageSurface will lazily create an empty WinRT Composition surface wrapper around a null legacy surface in GetRealSurface. This avoids having to touch the D2D or the D3D device and encountering an error.

When we later decode an image into the surface, we update the legacy DComp surface inside the empty wrapper that we've already created. That introduces a convenient behavior where the real surface inside the LoadedImageSurface never changes. Even across device loss recovery, we just have to swap out the legacy DComp surface inside the WinRT Composition surface wrapper. When we switch to WinRT Composition surfaces, we'll have to toss the existing surface and create a new one, which means the caller will have to call GetRealSurface again to get the new Composition surface.

## Room for improvement

* Xaml should work with a WinRT ICompositionSurface directly and move off of the legacy IDCompositionSurface. There's already a WinRT version of BeginDrawWithGutters called BeginDrawWithClear.
* Xaml has too many wrappers. HWTexture/HWRgbTexture were mainly meant for atlasing, which Xaml doesn't do anymore. They can be removed. The threading lock associated with off-thread decode can be moved down to DCompSurface (the Composition surface wrapper) or SystemMemoryBits (the scratch surface wrapper).
* What's the perf of CompositionDrawingSurface's BeginDraw/EndDraw? Do we still need the scratch texture at all for alpha mask rendering? Can we just have D2D draw directly into the CompositionSurface? Image decoding happens off-thread and can span multiple frames, so keep that decoding into a scratch texture and do a single copy at the end.
* Do we ever copy to more than 1 tile anymore? Virtual surfaces are handled by Composition now, instead of being tracked by Xaml. `CopyToHardwareTiles` has room for simplification if surfaceUpdateList only ever has one entry.

### WinRT surface gaps
* Need offer/reclaim in WinRT. Or do we bypass Composition and call the API on the D2D/DXGI device directly (need IDCompositionSurfaceFactoryPartner3 for offer/reclaim)
* Need IDCompositionVirtualSurface::Trim for VSIS in WinRT
* Text rendering hits D2DERR_WRONG_STATE with BeginDrawWithClear
* BeginDrawWithClear doesn't return the gutter size - do we assume 1?

## Useful breakpoints

Image decoding into the scratch texture
```
bu microsoft_ui_xaml!CopyToHardwareTiles
```

DCompSurface copying from the scratch texture into the Composition surface
```
bu microsoft_ui_xaml!DCompSurface::FlushUpdates
bu microsoft_ui_xaml!DCompSurface::CopySubresource
```

Text drawing into a Composition surface
```
bu microsoft_ui_xaml!D2DTextDrawingContext::HWBuildGlyphRunSingleTexture
```

Shape alpha mask rendering into a Composition surface
```
bu microsoft_ui_xaml!AlphaMask::Impl::RasterizeElement
```
