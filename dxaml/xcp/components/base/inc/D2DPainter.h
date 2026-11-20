// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ImmediatePainter.h>

struct ID3D11Device;
struct IDXGISwapChain1;

// Direct2D-specific surface of the immediate painter (D2D-HW and D2D-SW). The backend-neutral contract is
// IImmediatePainter in ImmediatePainter.h; this header adds what D2D tests and instrumentation need on top of it.
// Owned by the immediate-rendering-d2d-sw branch. See D2DPainter.cpp for the copy-accounting contract.
namespace RenderingBackend
{
    // Cumulative creation counts since the painter was created. A steady-state frame that repeats the previous frame
    // must not increase any of them (resources are cached and reused across frames).
    struct D2DResourceCounters
    {
        uint32_t solidBrushesCreated = 0;
        uint32_t textFormatsCreated = 0;
        uint32_t geometriesCreated = 0;   // damage masks, rounded-clip masks, and path geometries (all ID2D1Geometry)
        uint32_t layersCreated = 0;       // opacity / clip layers (ID2D1Layer or compatible targets)
        uint32_t targetBitmapsCreated = 0; // render target bitmaps (initial + Resize + device recovery)
        uint32_t pathGeometriesCreated = 0; // FillPath / StrokePath geometries (subset of geometriesCreated)
        uint32_t bitmapsCreated = 0;      // image bitmaps created by EnsureBitmap (not refreshed in place)
        uint32_t bitmapUploads = 0;       // EnsureBitmap pixel copies: creations plus in-place refreshes
        uint32_t scratchSurfacesCreated = 0; // ScrollPixels intermediate textures
        uint32_t gradientBrushesCreated = 0; // linear gradient brushes (each with its own stop collection)
        uint32_t effectsCreated = 0;      // Shadow and backdrop-blur effect objects
        uint32_t intermediateBitmapsCreated = 0; // shadow bitmaps and backdrop scratch surfaces
        uint32_t commandListsCreated = 0; // shadow shapes and backdrop masks
    };

    class ID2DImmediatePainter : public IImmediatePainter
    {
    public:
        // The D3D device the painter draws with, AddRef'd. Always the device passed to CreateD2DPainter: the painter
        // never creates its own D3D device, so callers own adapter selection (hardware versus WARP).
        virtual HRESULT GetD3DDevice(_Outptr_ ID3D11Device** device) const noexcept = 0;

        virtual D2DResourceCounters GetResourceCounters() const noexcept = 0;

        // Bytes the last EndFrame copied from the painter's target into the attached swap chain's back buffer (damage
        // rects clipped to the target, plus areas moved by ScrollPixels since the previous frame, times 4), in both
        // modes; 0 for offscreen frames. On WARP uploadBytesThisFrame is this plus EnsureBitmap copies; on a hardware
        // device it is a GPU-local copy and is not an upload.
        virtual uint64_t GetPresentCopyBytesThisFrame() const noexcept = 0;

        // SetPresentationSwapChain is on IImmediatePainter; for D2D the swap chain must be on GetD3DDevice's device.
    };

    // Same as CreateD2DPainter in ImmediatePainter.h, but returns the D2D-specific interface. mode must be
    // ImmediateD2DHardware or ImmediateD2DSoftware and device must be non-null (E_INVALIDARG otherwise).
    HRESULT CreateD2DPainter(
        Mode mode,
        _In_ ID3D11Device* device,
        const PainterOptions& options,
        _Out_ std::unique_ptr<ID2DImmediatePainter>& painter) noexcept;
}
