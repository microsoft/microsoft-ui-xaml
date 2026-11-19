// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <memory>
#include <RenderingBackend.h>

struct ID3D11Device;
struct IDXGISwapChain1;

// Shared painter contract for the immediate-rendering prototype backends (GDI+, D2D hardware, D2D WARP).
// See docs/design-notes/immediate-rendering-backends-proposal.md. The primitive set is deliberately limited to what the
// S1 (settings) and S2 (dashboard) bench scenes need: solid and rounded fills, rounded strokes, text, rectangular clips,
// 2D transforms, and group opacity. Painters draw to an offscreen premultiplied BGRA target; presentation is separate.
namespace RenderingBackend
{
    struct ColorF
    {
        float r = 0, g = 0, b = 0, a = 1; // straight (non-premultiplied) alpha
    };

    struct RectF
    {
        float left = 0, top = 0, right = 0, bottom = 0;
    };

    // Row-vector affine matrix, same layout as D2D1_MATRIX_3X2_F and Gdiplus::Matrix(m11, m12, m21, m22, dx, dy).
    struct Matrix3x2F
    {
        float m11 = 1, m12 = 0, m21 = 0, m22 = 1, dx = 0, dy = 0;
    };

    struct TextStyle
    {
        const wchar_t* fontFamily = L"Segoe UI";
        float fontSize = 14;      // DIPs
        uint32_t fontWeight = 400; // DWRITE_FONT_WEIGHT / CSS weight
    };

    struct PainterOptions
    {
        uint32_t width = 0;   // physical pixels
        uint32_t height = 0;  // physical pixels
        float dpiScale = 1.0f;
    };

    struct DeviceIdentity
    {
        Mode mode = Mode::Composition;
        uint64_t adapterLuid = 0; // (HighPart << 32) | LowPart; 0 for CPU-only painters
        bool isWarp = false;
        uint32_t featureLevel = 0; // D3D_FEATURE_LEVEL; 0 for CPU-only painters
    };

    struct FrameStats
    {
        uint64_t damagedPixels = 0;
        uint64_t paintedPixels = 0;
        uint32_t primitiveCount = 0;   // fills, strokes, and text runs issued this frame
        uint32_t damageRectCount = 0;
    };

    struct ResourceInventory
    {
        uint64_t surfaceBytes = 0;          // render targets / backbuffers owned by the painter
        uint64_t cacheBytes = 0;            // glyph, geometry, brush caches (best estimate)
        uint32_t cachedObjectCount = 0;
        uint64_t uploadBytesThisFrame = 0;  // CPU->GPU or CPU->surface copies attributable to the last frame
    };

    class IImmediatePainter
    {
    public:
        virtual ~IImmediatePainter() = default;

        virtual DeviceIdentity GetDeviceIdentity() const noexcept = 0;
        virtual HRESULT Resize(uint32_t width, uint32_t height, float dpiScale) noexcept = 0;

        // damageRects (physical pixels) == nullptr / damageRectCount == 0 means the whole target is damaged and is
        // reported as one damage rect. All drawing, including Clear, is clipped to the damage; pixels outside it keep
        // the previous frame's content. Clear does not count as a primitive. Drawing coordinates are DIPs.
        virtual HRESULT BeginFrame(_In_reads_opt_(damageRectCount) const RectF* damageRects, uint32_t damageRectCount) noexcept = 0;
        virtual void Clear(const ColorF& color) noexcept = 0;
        virtual void FillRectangle(const RectF& rect, const ColorF& color) noexcept = 0;
        virtual void FillRoundedRectangle(const RectF& rect, float radiusX, float radiusY, const ColorF& color) noexcept = 0;
        // The stroke is centered on rect's edge, matching D2D; Border layout insets rect by thickness/2.
        virtual void StrokeRoundedRectangle(const RectF& rect, float radiusX, float radiusY, float thickness, const ColorF& color) noexcept = 0;
        virtual void DrawText(_In_reads_(length) const wchar_t* text, uint32_t length, const TextStyle& style, const RectF& layoutRect, const ColorF& color) noexcept = 0;
        virtual void PushAxisAlignedClip(const RectF& clip) noexcept = 0;
        virtual void PopAxisAlignedClip() noexcept = 0;
        // Replaces the current transform (callers compose); identity is the default at BeginFrame.
        virtual void SetTransform(const Matrix3x2F& transform) noexcept = 0;
        // Group opacity: children are composited together, then blended once.
        virtual void PushOpacityLayer(float opacity) noexcept = 0;
        virtual void PopOpacityLayer() noexcept = 0;
        virtual HRESULT EndFrame(_Out_ FrameStats* stats) noexcept = 0;

        // Copies the current target as premultiplied BGRA8. For tests and fidelity capture, not the present path.
        virtual HRESULT ReadPixels(_Out_writes_bytes_(bufferSize) uint8_t* buffer, uint32_t stride, uint32_t bufferSize) noexcept = 0;
        virtual ResourceInventory GetResourceInventory() const noexcept = 0;

        // Presentation (decision: one composition swap chain per root for every immediate mode). Attaches, or with
        // nullptr detaches, the root's swap chain (DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED, created
        // with CreateSwapChainForComposition by the render loop). With a swap chain attached, EndFrame copies only the
        // frame's damage rects from the painter's target into the back buffer and calls Present1 with those dirty
        // rects; ResourceInventory::uploadBytesThisFrame reports that copy. Without one, frames are offscreen only and
        // report 0. D2D painters require the swap chain to be on their own device (E_INVALIDARG otherwise); the GDI+
        //         painter accepts a swap chain on any D3D11 device and copies with UpdateSubresource. Resize also resizes the attached
                // swap chain's buffers (ResizeBuffers); the next frame after attach or Resize is treated as fully damaged.
        virtual HRESULT SetPresentationSwapChain(_In_opt_ IDXGISwapChain1* swapChain) noexcept = 0;
    };

    // Dispatches to the per-backend factories below. Composition is not an immediate painter (E_INVALIDARG).
    HRESULT CreateImmediatePainter(Mode mode, const PainterOptions& options, _Out_ std::unique_ptr<IImmediatePainter>& painter) noexcept;

    // Per-backend factories, each in its own source file so backends evolve independently.
    HRESULT CreateGdiPlusPainter(const PainterOptions& options, _Out_ std::unique_ptr<IImmediatePainter>& painter) noexcept;          // GdiPlusPainter.cpp
    HRESULT CreateD2DPainter(Mode mode, _In_ ID3D11Device* device, const PainterOptions& options, _Out_ std::unique_ptr<IImmediatePainter>& painter) noexcept; // D2DPainter.cpp

    // D3D device for the D2D modes (D2DDeviceFactory.cpp). ImmediateD2DSoftware => WARP. ImmediateD2DHardware => a
    // hardware adapter; must fail with DXGI_ERROR_NOT_FOUND rather than silently returning WARP or the Basic Render
    // Driver when no hardware adapter exists.
    HRESULT CreateD3DDeviceForMode(Mode mode, _Outptr_ ID3D11Device** device, _Out_ DeviceIdentity* identity) noexcept;

    // Brackets one root's paint with PaintBegin/PaintEnd and emits ResourceInventory, so every backend reports the same
    // events. paint is invoked between BeginFrame and EndFrame; returns the first failure.
    template <typename PaintFn>
    HRESULT PaintTracedFrame(
        IImmediatePainter& painter,
        uint32_t frameNumber,
        uint64_t rootId,
        _In_reads_opt_(damageRectCount) const RectF* damageRects,
        uint32_t damageRectCount,
        PaintFn&& paint,
        _Out_opt_ FrameStats* statsOut = nullptr) noexcept
    {
        TracePaintBegin(frameNumber, rootId);
        FrameStats stats{};
        HRESULT hr = painter.BeginFrame(damageRects, damageRectCount);
        if (SUCCEEDED(hr))
        {
            paint(painter);
            hr = painter.EndFrame(&stats);
        }
        TracePaintEnd(frameNumber, rootId, stats.damagedPixels, stats.paintedPixels, stats.primitiveCount, stats.damageRectCount);
        const auto inventory = painter.GetResourceInventory();
        TraceResourceInventory(painter.GetDeviceIdentity().mode, inventory.surfaceBytes, inventory.cacheBytes,
            inventory.cachedObjectCount, inventory.uploadBytesThisFrame);
        if (statsOut)
        {
            *statsOut = stats;
        }
        return hr;
    }
}
