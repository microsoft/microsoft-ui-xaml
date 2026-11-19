// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>
#include <D2DPainter.h>
#include <D2DDeviceFactory.h>
#include <GpuTimestampProfiler.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <cmath>
#include <cstring>
#include <cwchar>
#include <new>
#include <unordered_map>

// Direct2D immediate painter shared by D2D-HW and D2D-SW; the device decides hardware versus WARP.
// Owned by the immediate-rendering-d2d-sw branch.
//
// Device ownership
//   The painter draws with the ID3D11Device it is given (CreateD3DDeviceForMode picks WARP or a hardware adapter) and
//   never creates its own. GetDeviceIdentity reports that device's adapter LUID, WARP flag, and feature level. Drawing
//   goes through ID2D1Factory1 -> ID2D1Device (from the device's IDXGIDevice) -> ID2D1DeviceContext, targeting an
//   ID2D1Bitmap1 over a B8G8R8A8 premultiplied D3D11 texture at physical size; DIPs map to pixels with SetDpi.
//
// Damage
//   BeginFrame clips everything to the damage: one rect is an aliased axis-aligned clip, several rects are a layerless
//   PushLayer whose geometric mask is a (cached) geometry group of the rects. Clear replaces pixels: under the
//   single-rect clip directly, and for several rects by temporarily popping the mask and clearing each rect under its
//   own axis-aligned clip. (Clear issued inside user clips or layers on a multi-rect frame falls back to a COPY-blend
//   fill of the damage, which still composites through the mask, so translucent clear colors blend there.) More than
//   MaxDamageRects rects are merged into their bounding box; that box is repainted from the retained scene.
//
// Copy accounting: ResourceInventory::uploadBytesThisFrame (test plan metric W5)
//   Reset at BeginFrame; describes the frame completed by the last EndFrame.
//   D2D-SW: WARP rasterizes on the CPU straight into the target bitmap, which lives in system memory, so drawing
//     itself uploads nothing. Making the frame visible needs a copy of the damaged pixels from that target into the
//     presentation surface (the attached swap chain's back buffer), which DWM then consumes. The painter counts that
//     copy: for a presented frame, uploadBytesThisFrame == sum over damage rects (clipped to the target) of
//     width * height * 4 == FrameStats::damagedPixels * 4 for non-overlapping damage. A frame painted with no swap
//     chain attached (offscreen-only: tests, fidelity capture) reports 0. ReadPixels is a diagnostic readback and is
//     never counted. Copies DWM makes on its own side are outside the process and are measured with the WPR Gpu
//     profile instead.
//   D2D-HW: uploadBytesThisFrame counts CPU-to-GPU uploads the painter issues itself: EnsureBitmap pixel copies
//     (width * height * 4 per creation or refresh). Other primitives (fills, rounded rects, strokes, paths, text,
//     glyph runs) issue none the painter can see: geometry is tessellated into D2D's internal vertex buffers and glyphs
//     into D2D's glyph atlas, neither of which is attributable from outside D2D; use the WPR Gpu profile (DMA packets)
//     for those. The target-to-back-buffer copy is a same-adapter, GPU-local copy;
//     it is not an upload, and is exposed separately as ID2DImmediatePainter::GetPresentCopyBytesThisFrame (which on
//     WARP is included in uploadBytesThisFrame).
//   Both: EnsureBitmap copies count in the frame they happen in, or the next frame when made between frames (on WARP the
//     copy is CPU to system memory, on hardware CPU to GPU). ScrollPixels moves pixels inside the target with two
//     device copies through a scratch texture (GPU-local on hardware, a memcpy inside WARP; neither is counted as an
//     upload itself); the moved destination rect is then copied to the back buffer and added to Present1's dirty rects
//     on the next presented frame, so it counts in GetPresentCopyBytesThisFrame (and on WARP in uploadBytesThisFrame)
//     although FrameStats::damagedPixels does not include it.
//
// GPU time (test plan W3)
//   On a hardware device (identity.isWarp == false) every frame, including the back-buffer copy and Present, is
//   bracketed with GpuTimestampProfiler, which emits GpuPaintTime when the result is read back at a later BeginFrame.
//   On WARP the "GPU" is CPU rasterization on WARP's own threads, already covered by CPU sampling, so it is skipped.
//
// Device removal
//   When the device is removed or reset, BeginFrame, EndFrame, Resize, and ReadPixels return the failure
//   (DXGI_ERROR_DEVICE_REMOVED or the device's removed reason) instead of crashing, and the painter emits DeviceLost
//   once. Drawing calls between a failed BeginFrame and EndFrame are no-ops. Recovery (new device, recreate resources)
//   is the caller's decision; the painter stays failed.
//
// Resources (W6)
//   One solid-color brush (SetColor per primitive); text formats cached by family/size/weight; rounded rects drawn with
//   direct calls (no geometry objects); opacity layers and rounded clips are layerless (D2D owns and pools the layer
//   surfaces, so layersCreated stays 0). Rounded clips use an ID2D1RoundedRectangleGeometry mask cached by rect and
//   radii (LRU, MaxClipGeometries); paths are ID2D1PathGeometry objects cached by PathData::cacheKey (LRU,
//   MaxPathGeometries; cacheKey 0 builds one per call); strokes use D2D's default stroke style (flat caps, miter joins).
//   Glyph runs create nothing. Images are ID2D1Bitmap1 objects (96 DPI, premultiplied BGRA) keyed by BitmapSource::key,
//   refreshed in place with CopyFromMemory when only the generation changes; their bytes count in cacheBytes.
//   Creations and destructions are emitted per frame as ResourceChurn; a steady-state frame creates nothing.
namespace RenderingBackend
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        constexpr uint32_t MaxStackDepth = 64;
        constexpr uint32_t MaxDamageRects = 32;
        constexpr uint32_t MaxScrollRects = 8;
        constexpr uint32_t MaxTextFormats = 32;
        constexpr uint32_t MaxClipGeometries = 32;
        constexpr uint32_t MaxPathGeometries = 128;
        constexpr size_t MaxFamilyLength = 64;

        // Best-effort sizes for objects whose memory D2D/DWrite do not report.
        constexpr uint64_t BrushBytesEstimate = 64;
        constexpr uint64_t TextFormatBytesEstimate = 4096;
        constexpr uint64_t GeometryBytesEstimate = 256;

        struct PixelRect
        {
            uint32_t left = 0, top = 0, right = 0, bottom = 0;

            bool IsEmpty() const noexcept { return right <= left || bottom <= top; }
            uint64_t Area() const noexcept
            {
                return IsEmpty() ? 0 : static_cast<uint64_t>(right - left) * (bottom - top);
            }
            bool operator==(const PixelRect& other) const noexcept
            {
                return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
            }
        };

        uint32_t ClampToExtent(float value, uint32_t extent) noexcept
        {
            if (!(value > 0))
            {
                return 0; // also NaN
            }
            if (value >= static_cast<float>(extent))
            {
                return extent;
            }
            return static_cast<uint32_t>(value);
        }

        PixelRect ToPixelRect(const RectF& rect, uint32_t width, uint32_t height) noexcept
        {
            PixelRect result;
            result.left = ClampToExtent(std::floor(rect.left), width);
            result.top = ClampToExtent(std::floor(rect.top), height);
            result.right = ClampToExtent(std::ceil(rect.right), width);
            result.bottom = ClampToExtent(std::ceil(rect.bottom), height);
            return result;
        }

        D2D1_RECT_F ToD2D(const RectF& rect) noexcept
        {
            return D2D1::RectF(rect.left, rect.top, rect.right, rect.bottom);
        }

        D2D1_COLOR_F ToD2D(const ColorF& color) noexcept
        {
            return D2D1::ColorF(color.r, color.g, color.b, color.a);
        }

        D2D1_MATRIX_3X2_F ToD2D(const Matrix3x2F& m) noexcept
        {
            return D2D1::Matrix3x2F(m.m11, m.m12, m.m21, m.m22, m.dx, m.dy);
        }

        bool IsDeviceLoss(HRESULT hr) noexcept
        {
            return hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_HUNG ||
                hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR || hr == D2DERR_RECREATE_TARGET;
        }

        enum class StackEntry : uint8_t
        {
            Clip,
            Layer,
            RoundedClip, // a layer with a geometric mask
        };

        struct ClipGeometryEntry
        {
            RectF rect;
            float radiusX = 0, radiusY = 0;
            uint64_t lastUse = 0;
            ComPtr<ID2D1RoundedRectangleGeometry> geometry;
        };

        struct PathGeometryEntry
        {
            uint64_t key = 0;
            bool nonZeroFill = false;
            uint64_t lastUse = 0;
            RectF bounds;
            ComPtr<ID2D1PathGeometry> geometry;
        };

        struct BitmapEntry
        {
            uint64_t generation = 0;
            uint32_t width = 0, height = 0;
            ComPtr<ID2D1Bitmap1> bitmap;
        };

        struct TextFormatEntry
        {
            wchar_t family[MaxFamilyLength]{};
            float size = 0;
            uint32_t weight = 0;
            ComPtr<IDWriteTextFormat> format;
        };

        class D2DPainter final : public ID2DImmediatePainter
        {
        public:
            ~D2DPainter() override
            {
                if (m_frameActive)
                {
                    UnwindStack();
                    PopDamageClip();
                    m_frameActive = false;
                    m_context->EndDraw();
                    if (m_profileGpu)
                    {
                        m_profiler.EndPaint(m_d3dContext.Get());
                    }
                }
            }

            HRESULT Initialize(Mode mode, ID3D11Device* device, const PainterOptions& options) noexcept
            {
                m_device = device;
                m_device->GetImmediateContext(&m_d3dContext);

                ComPtr<IDXGIDevice> dxgiDevice;
                ComPtr<IDXGIAdapter> adapter;
                ComPtr<IDXGIAdapter1> adapter1;
                DXGI_ADAPTER_DESC1 desc{};
                IFC_RETURN(m_device.As(&dxgiDevice));
                IFC_RETURN(dxgiDevice->GetAdapter(&adapter));
                IFC_RETURN(adapter.As(&adapter1));
                IFC_RETURN(adapter1->GetDesc1(&desc));
                m_identity.mode = mode;
                m_identity.adapterLuid = HardwareAdapter::PackLuid(desc.AdapterLuid);
                // WARP, and the WARP-backed Basic Render Driver, regardless of which mode the caller asked for.
                m_identity.isWarp = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0 ||
                    (desc.VendorId == HardwareAdapter::MicrosoftVendorId && desc.DeviceId == HardwareAdapter::BasicRenderDriverDeviceId);
                m_identity.featureLevel = static_cast<uint32_t>(m_device->GetFeatureLevel());

                D2D1_FACTORY_OPTIONS factoryOptions{};
                IFC_RETURN(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &factoryOptions,
                    reinterpret_cast<void**>(m_factory.ReleaseAndGetAddressOf())));
                IFC_RETURN(m_factory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice));
                IFC_RETURN(m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_context));
                IFC_RETURN(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(m_dwriteFactory.ReleaseAndGetAddressOf())));

                IFC_RETURN(m_context->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &m_brush));
                ++m_counters.solidBrushesCreated;
                ++m_churnCreated;

                m_width = options.width;
                m_height = options.height;
                m_dpiScale = options.dpiScale;
                m_context->SetDpi(96.0f * m_dpiScale, 96.0f * m_dpiScale);
                IFC_RETURN(CreateTarget());

                m_profileGpu = !m_identity.isWarp && SUCCEEDED(m_profiler.Initialize(m_device.Get()));
                m_forceFullDamage = true;
                return S_OK;
            }

            DeviceIdentity GetDeviceIdentity() const noexcept override
            {
                return m_identity;
            }

            HRESULT GetD3DDevice(_Outptr_ ID3D11Device** device) const noexcept override
            {
                if (device == nullptr)
                {
                    return E_POINTER;
                }
                *device = m_device.Get();
                (*device)->AddRef();
                return S_OK;
            }

            D2DResourceCounters GetResourceCounters() const noexcept override
            {
                return m_counters;
            }

            uint64_t GetPresentCopyBytesThisFrame() const noexcept override
            {
                return m_presentCopyBytes;
            }

            HRESULT Resize(uint32_t width, uint32_t height, float dpiScale) noexcept override
            {
                if (width == 0 || height == 0 || !(dpiScale > 0))
                {
                    return E_INVALIDARG;
                }
                if (m_frameActive)
                {
                    return E_ILLEGAL_METHOD_CALL;
                }
                IFC_RETURN(CheckDevice());

                ReleaseTarget();
                ResetDamageGeometry(); // in DIPs, so it depends on the DPI
                m_pendingScrollCount = 0; // the next frame is fully damaged
                m_width = width;
                m_height = height;
                m_dpiScale = dpiScale;
                m_context->SetDpi(96.0f * m_dpiScale, 96.0f * m_dpiScale);
                m_forceFullDamage = true;

                if (m_swapChain)
                {
                    HRESULT hr = ResizeSwapChain();
                    if (FAILED(hr))
                    {
                        IFC_RETURN(HandleFailure(hr));
                    }
                }
                const HRESULT hr = CreateTarget();
                if (FAILED(hr))
                {
                    IFC_RETURN(HandleFailure(hr));
                }
                return S_OK;
            }

            HRESULT BeginFrame(_In_reads_opt_(damageRectCount) const RectF* damageRects, uint32_t damageRectCount) noexcept override
            {
                if (m_frameActive)
                {
                    return E_ILLEGAL_METHOD_CALL;
                }
                ++m_frameNumber;
                m_stats = {};
                m_uploadBytes = 0;
                m_presentCopyBytes = 0;
                IFC_RETURN(CheckDevice());
                if (!m_targetBitmap)
                {
                    return E_UNEXPECTED;
                }

                ComputeDamage(damageRects, damageRectCount);
                if (m_damageCount > 1)
                {
                    const HRESULT hr = EnsureDamageGeometry();
                    if (FAILED(hr))
                    {
                        IFC_RETURN(HandleFailure(hr));
                    }
                }

                if (m_profileGpu)
                {
                    m_profiler.Collect(m_d3dContext.Get(), nullptr, 0);
                    m_profiler.BeginPaint(m_d3dContext.Get(), m_frameNumber);
                }
                m_context->BeginDraw();
                m_context->SetTransform(D2D1::Matrix3x2F::Identity());
                m_transform = {};
                m_context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                m_context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
                m_context->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
                PushDamageClip();
                m_depth = 0;
                m_overflowDepth = 0;
                // Bitmap copies made between frames, and areas moved by ScrollPixels, belong to this frame.
                m_uploadBytes = m_pendingUploadBytes;
                m_pendingUploadBytes = 0;
                m_frameScrollCount = m_pendingScrollCount;
                for (uint32_t i = 0; i < m_frameScrollCount; ++i)
                {
                    m_frameScrollRects[i] = m_pendingScrollRects[i];
                }
                m_pendingScrollCount = 0;
                m_frameActive = true;
                return S_OK;
            }

            void Clear(const ColorF& color) noexcept override
            {
                if (!m_frameActive)
                {
                    return;
                }
                m_stats.paintedPixels += m_stats.damagedPixels;
                const D2D1_COLOR_F clearColor = ToD2D(color);
                if (!m_damageUsesLayer)
                {
                    m_context->Clear(clearColor);
                    return;
                }

                D2D1_MATRIX_3X2_F saved;
                m_context->GetTransform(&saved);
                m_context->SetTransform(D2D1::Matrix3x2F::Identity());
                if (m_depth == 0 && m_overflowDepth == 0)
                {
                    // Clear blends through a layer's mask, so leave the mask and clear each rect under its own clip.
                    m_context->PopLayer();
                    for (uint32_t i = 0; i < m_damageCount; ++i)
                    {
                        m_context->PushAxisAlignedClip(ToDips(m_damage[i]), D2D1_ANTIALIAS_MODE_ALIASED);
                        m_context->Clear(clearColor);
                        m_context->PopAxisAlignedClip();
                    }
                    PushDamageClip();
                }
                else
                {
                    m_brush->SetColor(clearColor);
                    m_context->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
                    m_context->FillRectangle(ToDips(m_damageBounds), m_brush.Get());
                    m_context->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
                }
                m_context->SetTransform(saved);
            }

            void FillRectangle(const RectF& rect, const ColorF& color) noexcept override
            {
                if (!m_frameActive)
                {
                    return;
                }
                m_brush->SetColor(ToD2D(color));
                m_context->FillRectangle(ToD2D(rect), m_brush.Get());
                CountPrimitive(rect);
            }

            void FillRoundedRectangle(const RectF& rect, float radiusX, float radiusY, const ColorF& color) noexcept override
            {
                if (!m_frameActive)
                {
                    return;
                }
                m_brush->SetColor(ToD2D(color));
                if (radiusX > 0 && radiusY > 0)
                {
                    m_context->FillRoundedRectangle(D2D1::RoundedRect(ToD2D(rect), radiusX, radiusY), m_brush.Get());
                }
                else
                {
                    m_context->FillRectangle(ToD2D(rect), m_brush.Get());
                }
                CountPrimitive(rect);
            }

            void StrokeRoundedRectangle(const RectF& rect, float radiusX, float radiusY, float thickness, const ColorF& color) noexcept override
            {
                if (!m_frameActive || !(thickness > 0))
                {
                    return;
                }
                m_brush->SetColor(ToD2D(color));
                if (radiusX > 0 && radiusY > 0)
                {
                    m_context->DrawRoundedRectangle(D2D1::RoundedRect(ToD2D(rect), radiusX, radiusY), m_brush.Get(), thickness);
                }
                else
                {
                    m_context->DrawRectangle(ToD2D(rect), m_brush.Get(), thickness);
                }
                const float half = thickness / 2;
                CountPrimitive({ rect.left - half, rect.top - half, rect.right + half, rect.bottom + half });
            }

            void DrawText(_In_reads_(length) const wchar_t* text, uint32_t length, const TextStyle& style, const RectF& layoutRect, const ColorF& color) noexcept override
            {
                if (!m_frameActive || text == nullptr || length == 0)
                {
                    return;
                }
                ComPtr<IDWriteTextFormat> format = GetTextFormat(style);
                if (!format)
                {
                    return;
                }
                m_brush->SetColor(ToD2D(color));
                m_context->DrawText(text, length, format.Get(), ToD2D(layoutRect), m_brush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
                CountPrimitive(layoutRect);
            }

            void PushAxisAlignedClip(const RectF& clip) noexcept override
            {
                if (!m_frameActive)
                {
                    return;
                }
                if (m_depth >= MaxStackDepth)
                {
                    ++m_overflowDepth;
                    return;
                }
                m_context->PushAxisAlignedClip(ToD2D(clip), D2D1_ANTIALIAS_MODE_ALIASED);
                m_stack[m_depth++] = StackEntry::Clip;
            }

            void PopAxisAlignedClip() noexcept override
            {
                PopEntry(StackEntry::Clip);
            }

            void SetTransform(const Matrix3x2F& transform) noexcept override
            {
                if (!m_frameActive)
                {
                    return;
                }
                m_transform = transform;
                m_context->SetTransform(ToD2D(transform));
            }

            void PushOpacityLayer(float opacity) noexcept override
            {
                if (!m_frameActive)
                {
                    return;
                }
                if (m_depth >= MaxStackDepth)
                {
                    ++m_overflowDepth;
                    return;
                }
                const float clamped = opacity >= 0 ? (opacity <= 1 ? opacity : 1.0f) : 0.0f;
                // Layerless: D2D allocates and pools the intermediate surface, so no ID2D1Layer is created.
                const D2D1_LAYER_PARAMETERS1 parameters = D2D1::LayerParameters1(D2D1::InfiniteRect(), nullptr,
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, D2D1::IdentityMatrix(), clamped, nullptr, D2D1_LAYER_OPTIONS1_NONE);
                m_context->PushLayer(&parameters, nullptr);
                m_stack[m_depth++] = StackEntry::Layer;
            }

            void PopOpacityLayer() noexcept override
            {
                PopEntry(StackEntry::Layer);
            }

            void PushRoundedRectangleClip(const RectF& clip, float radiusX, float radiusY) noexcept override
            {
                if (!m_frameActive)
                {
                    return;
                }
                if (m_depth >= MaxStackDepth)
                {
                    ++m_overflowDepth;
                    return;
                }
                // Layerless, like opacity layers. The mask and content bounds are in the current world space. Without a
                // mask (out of memory) the content bounds still clip to the rect.
                ID2D1Geometry* mask = GetClipGeometry(clip, radiusX, radiusY);
                const D2D1_LAYER_PARAMETERS1 parameters = D2D1::LayerParameters1(ToD2D(clip), mask,
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, D2D1::IdentityMatrix(), 1.0f, nullptr, D2D1_LAYER_OPTIONS1_NONE);
                m_context->PushLayer(&parameters, nullptr);
                m_stack[m_depth++] = StackEntry::RoundedClip;
            }

            void PopRoundedRectangleClip() noexcept override
            {
                PopEntry(StackEntry::RoundedClip);
            }

            void FillPath(const PathData& path, const ColorF& color) noexcept override
            {
                if (!m_frameActive)
                {
                    return;
                }
                RectF bounds;
                ComPtr<ID2D1PathGeometry> geometry = GetPathGeometry(path, &bounds);
                if (!geometry)
                {
                    return;
                }
                m_brush->SetColor(ToD2D(color));
                m_context->FillGeometry(geometry.Get(), m_brush.Get());
                CountPrimitive(bounds);
            }

            void StrokePath(const PathData& path, float thickness, const ColorF& color) noexcept override
            {
                if (!m_frameActive || !(thickness > 0))
                {
                    return;
                }
                RectF bounds;
                ComPtr<ID2D1PathGeometry> geometry = GetPathGeometry(path, &bounds);
                if (!geometry)
                {
                    return;
                }
                m_brush->SetColor(ToD2D(color));
                // A null stroke style is D2D's default: solid, flat caps, miter joins (miter limit 10).
                m_context->DrawGeometry(geometry.Get(), m_brush.Get(), thickness, nullptr);
                const float half = thickness / 2;
                CountPrimitive({ bounds.left - half, bounds.top - half, bounds.right + half, bounds.bottom + half });
            }

            void DrawGlyphRun(const GlyphRunDesc& run, const ColorF& color) noexcept override
            {
                static_assert(sizeof(GlyphOffset) == sizeof(DWRITE_GLYPH_OFFSET), "GlyphOffset must match DWRITE_GLYPH_OFFSET");
                if (!m_frameActive || run.fontFace == nullptr || !(run.fontEmSize > 0) || run.glyphCount == 0 ||
                    run.glyphIndices == nullptr || run.glyphAdvances == nullptr)
                {
                    return;
                }
                DWRITE_GLYPH_RUN glyphRun{};
                glyphRun.fontFace = run.fontFace;
                glyphRun.fontEmSize = run.fontEmSize;
                glyphRun.glyphCount = run.glyphCount;
                glyphRun.glyphIndices = run.glyphIndices;
                glyphRun.glyphAdvances = run.glyphAdvances;
                glyphRun.glyphOffsets = reinterpret_cast<const DWRITE_GLYPH_OFFSET*>(run.glyphOffsets);
                glyphRun.isSideways = FALSE;
                glyphRun.bidiLevel = run.bidiLevel;
                m_brush->SetColor(ToD2D(color));
                // Grayscale text antialiasing is set at BeginFrame (ClearType does not suit premultiplied targets).
                m_context->DrawGlyphRun(D2D1::Point2F(run.baselineOrigin.x, run.baselineOrigin.y), &glyphRun, m_brush.Get(),
                    DWRITE_MEASURING_MODE_NATURAL);

                float advance = 0;
                for (uint32_t i = 0; i < run.glyphCount; ++i)
                {
                    advance += run.glyphAdvances[i];
                }
                DWRITE_FONT_METRICS metrics{};
                run.fontFace->GetMetrics(&metrics);
                const float unitsPerEm = metrics.designUnitsPerEm != 0 ? metrics.designUnitsPerEm : 1.0f;
                const float ascent = run.fontEmSize * metrics.ascent / unitsPerEm;
                const float descent = run.fontEmSize * metrics.descent / unitsPerEm;
                const float x = run.baselineOrigin.x;
                const bool rightToLeft = (run.bidiLevel & 1) != 0;
                CountPrimitive({ rightToLeft ? x - advance : x, run.baselineOrigin.y - ascent,
                    rightToLeft ? x : x + advance, run.baselineOrigin.y + descent });
            }

            HRESULT EnsureBitmap(const BitmapSource& source) noexcept override
            {
                if (source.pixels == nullptr || source.width == 0 || source.height == 0 ||
                    source.stride < static_cast<uint64_t>(source.width) * 4)
                {
                    return E_INVALIDARG;
                }
                IFC_RETURN(CheckDevice());

                const auto found = m_bitmaps.find(source.key);
                if (found != m_bitmaps.end() && found->second.generation == source.generation)
                {
                    return S_OK;
                }

                HRESULT hr = S_OK;
                const uint64_t bytes = static_cast<uint64_t>(source.width) * source.height * 4;
                if (found != m_bitmaps.end() && found->second.width == source.width && found->second.height == source.height)
                {
                    hr = found->second.bitmap->CopyFromMemory(nullptr, source.pixels, source.stride);
                    if (FAILED(hr))
                    {
                        IFC_RETURN(HandleFailure(hr));
                    }
                    found->second.generation = source.generation;
                }
                else
                {
                    ComPtr<ID2D1Bitmap1> bitmap;
                    // 96 DPI: DrawBitmap source rects are in bitmap DIPs, which then equal bitmap pixels.
                    const D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
                        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f, 96.0f);
                    hr = m_context->CreateBitmap(D2D1::SizeU(source.width, source.height), source.pixels, source.stride,
                        &properties, &bitmap);
                    if (FAILED(hr))
                    {
                        IFC_RETURN(HandleFailure(hr));
                    }
                    ++m_counters.bitmapsCreated;
                    ++m_churnCreated;

                    if (found != m_bitmaps.end())
                    {
                        m_bitmapBytes -= static_cast<uint64_t>(found->second.width) * found->second.height * 4;
                        ++m_churnDestroyed;
                        found->second = BitmapEntry{ source.generation, source.width, source.height, bitmap };
                    }
                    else
                    {
                        // Built without exceptions: allocation failure fails fast, like the other std containers here.
                        m_bitmaps.emplace(source.key, BitmapEntry{ source.generation, source.width, source.height, bitmap });
                    }
                    m_bitmapBytes += bytes;
                }

                ++m_counters.bitmapUploads;
                if (m_frameActive)
                {
                    m_uploadBytes += bytes;
                }
                else
                {
                    m_pendingUploadBytes += bytes;
                }
                return S_OK;
            }

            void DrawBitmap(uint64_t key, const RectF& destination, _In_opt_ const RectF* sourceRect, float opacity) noexcept override
            {
                if (!m_frameActive)
                {
                    return;
                }
                const auto found = m_bitmaps.find(key);
                if (found == m_bitmaps.end())
                {
                    return;
                }
                const float clamped = opacity >= 0 ? (opacity <= 1 ? opacity : 1.0f) : 0.0f;
                D2D1_RECT_F source{};
                if (sourceRect != nullptr)
                {
                    source = ToD2D(*sourceRect);
                }
                const D2D1_RECT_F destinationRect = ToD2D(destination);
                m_context->DrawBitmap(found->second.bitmap.Get(), &destinationRect, clamped, D2D1_INTERPOLATION_MODE_LINEAR,
                    sourceRect != nullptr ? &source : nullptr, nullptr);
                CountPrimitive(destination);
            }

            void ReleaseBitmap(uint64_t key) noexcept override
            {
                const auto found = m_bitmaps.find(key);
                if (found == m_bitmaps.end())
                {
                    return;
                }
                m_bitmapBytes -= static_cast<uint64_t>(found->second.width) * found->second.height * 4;
                ++m_churnDestroyed;
                m_bitmaps.erase(found);
            }

            HRESULT ScrollPixels(const RectF& rect, int32_t dx, int32_t dy) noexcept override
            {
                if (m_frameActive)
                {
                    return E_ILLEGAL_METHOD_CALL;
                }
                IFC_RETURN(CheckDevice());
                if (!m_targetTexture)
                {
                    return E_UNEXPECTED;
                }
                const PixelRect area = ToPixelRect(rect, m_width, m_height);
                if (area.IsEmpty() || (dx == 0 && dy == 0))
                {
                    return S_OK;
                }

                // Destination: area intersected with area moved by (dx, dy); everything else scrolls out of rect.
                const int64_t left = (std::max)(static_cast<int64_t>(area.left), static_cast<int64_t>(area.left) + dx);
                const int64_t top = (std::max)(static_cast<int64_t>(area.top), static_cast<int64_t>(area.top) + dy);
                const int64_t right = (std::min)(static_cast<int64_t>(area.right), static_cast<int64_t>(area.right) + dx);
                const int64_t bottom = (std::min)(static_cast<int64_t>(area.bottom), static_cast<int64_t>(area.bottom) + dy);
                if (right <= left || bottom <= top)
                {
                    return S_OK;
                }
                const PixelRect destination{ static_cast<uint32_t>(left), static_cast<uint32_t>(top),
                    static_cast<uint32_t>(right), static_cast<uint32_t>(bottom) };
                const D3D11_BOX sourceBox{ static_cast<UINT>(left - dx), static_cast<UINT>(top - dy), 0,
                    static_cast<UINT>(right - dx), static_cast<UINT>(bottom - dy), 1 };

                if (!m_scrollScratch)
                {
                    D3D11_TEXTURE2D_DESC desc{};
                    m_targetTexture->GetDesc(&desc);
                    desc.BindFlags = 0;
                    desc.MiscFlags = 0;
                    const HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &m_scrollScratch);
                    if (FAILED(hr))
                    {
                        IFC_RETURN(HandleFailure(hr));
                    }
                    ++m_counters.scratchSurfacesCreated;
                    ++m_churnCreated;
                }

                // One copy cannot overlap its source, so go through the scratch texture at the same coordinates.
                // Both copies stay on the device: GPU-local on hardware, a memcpy inside WARP.
                m_d3dContext->CopySubresourceRegion(m_scrollScratch.Get(), 0, sourceBox.left, sourceBox.top, 0,
                    m_targetTexture.Get(), 0, &sourceBox);
                m_d3dContext->CopySubresourceRegion(m_targetTexture.Get(), 0, destination.left, destination.top, 0,
                    m_scrollScratch.Get(), 0, &sourceBox);

                AddPendingScrollRect(destination);
                return S_OK;
            }

            HRESULT EndFrame(_Out_ FrameStats* stats) noexcept override
            {
                if (stats != nullptr)
                {
                    *stats = {};
                }
                if (!m_frameActive)
                {
                    return m_lost ? m_lostHr : E_ILLEGAL_METHOD_CALL;
                }

                UnwindStack();
                PopDamageClip();
                m_frameActive = false;
                HRESULT hr = m_context->EndDraw();
                if (SUCCEEDED(hr) && m_swapChain)
                {
                    hr = PresentDamage();
                }
                if (m_profileGpu)
                {
                    m_profiler.EndPaint(m_d3dContext.Get());
                }
                if (SUCCEEDED(hr))
                {
                    m_forceFullDamage = false;
                }
                EmitChurn();
                if (stats != nullptr)
                {
                    *stats = m_stats;
                }
                if (FAILED(hr))
                {
                    IFC_RETURN(HandleFailure(hr));
                }
                return S_OK;
            }

            HRESULT ReadPixels(_Out_writes_bytes_(bufferSize) uint8_t* buffer, uint32_t stride, uint32_t bufferSize) noexcept override
            {
                if (buffer == nullptr)
                {
                    return E_POINTER;
                }
                const uint64_t rowBytes = static_cast<uint64_t>(m_width) * 4;
                if (stride < rowBytes || bufferSize < static_cast<uint64_t>(stride) * (m_height - 1) + rowBytes)
                {
                    return E_INVALIDARG;
                }
                if (m_frameActive)
                {
                    return E_ILLEGAL_METHOD_CALL;
                }
                IFC_RETURN(CheckDevice());

                HRESULT hr = S_OK;
                if (!m_staging)
                {
                    D3D11_TEXTURE2D_DESC desc{};
                    m_targetTexture->GetDesc(&desc);
                    desc.Usage = D3D11_USAGE_STAGING;
                    desc.BindFlags = 0;
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                    desc.MiscFlags = 0;
                    hr = m_device->CreateTexture2D(&desc, nullptr, &m_staging);
                    if (FAILED(hr))
                    {
                        IFC_RETURN(HandleFailure(hr));
                    }
                    ++m_churnCreated;
                }

                m_d3dContext->CopyResource(m_staging.Get(), m_targetTexture.Get());
                D3D11_MAPPED_SUBRESOURCE mapped{};
                hr = m_d3dContext->Map(m_staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
                if (FAILED(hr))
                {
                    IFC_RETURN(HandleFailure(hr));
                }
                const uint8_t* source = static_cast<const uint8_t*>(mapped.pData);
                for (uint32_t y = 0; y < m_height; ++y)
                {
                    memcpy(buffer + static_cast<size_t>(y) * stride, source + static_cast<size_t>(y) * mapped.RowPitch, static_cast<size_t>(rowBytes));
                }
                m_d3dContext->Unmap(m_staging.Get(), 0);
                return S_OK;
            }

            ResourceInventory GetResourceInventory() const noexcept override
            {
                ResourceInventory inventory;
                const uint64_t targetBytes = static_cast<uint64_t>(m_width) * m_height * 4;
                inventory.surfaceBytes = (m_targetTexture ? targetBytes : 0) + (m_staging ? targetBytes : 0) +
                    (m_scrollScratch ? targetBytes : 0);
                const uint32_t brushes = m_brush ? 1 : 0;
                const uint32_t bitmaps = static_cast<uint32_t>(m_bitmaps.size());
                inventory.cachedObjectCount = brushes + m_textFormatCount + m_damageGeometryObjects + m_clipGeometryCount +
                    m_pathGeometryCount + bitmaps;
                inventory.cacheBytes = brushes * BrushBytesEstimate + m_textFormatCount * TextFormatBytesEstimate +
                    (static_cast<uint64_t>(m_damageGeometryObjects) + m_clipGeometryCount + m_pathGeometryCount) * GeometryBytesEstimate +
                    m_bitmapBytes;
                inventory.uploadBytesThisFrame = m_uploadBytes;
                return inventory;
            }

            HRESULT SetPresentationSwapChain(_In_opt_ IDXGISwapChain1* swapChain) noexcept override
            {
                if (m_frameActive)
                {
                    return E_ILLEGAL_METHOD_CALL;
                }
                if (swapChain == nullptr)
                {
                    m_backBuffer.Reset();
                    m_swapChain.Reset();
                    return S_OK;
                }

                ComPtr<ID3D11Device> swapChainDevice;
                ComPtr<IUnknown> swapChainIdentity;
                ComPtr<IUnknown> painterIdentity;
                DXGI_SWAP_CHAIN_DESC1 desc{};
                if (FAILED(swapChain->GetDevice(IID_PPV_ARGS(&swapChainDevice))) ||
                    FAILED(swapChainDevice.As(&swapChainIdentity)) ||
                    FAILED(m_device.As(&painterIdentity)) ||
                    swapChainIdentity.Get() != painterIdentity.Get() ||
                    FAILED(swapChain->GetDesc1(&desc)) ||
                    desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM)
                {
                    return E_INVALIDARG;
                }
                IFC_RETURN(CheckDevice());

                m_backBuffer.Reset();
                m_swapChain = swapChain;
                if (desc.Width != m_width || desc.Height != m_height)
                {
                    const HRESULT hr = ResizeSwapChain();
                    if (FAILED(hr))
                    {
                        m_swapChain.Reset();
                        IFC_RETURN(HandleFailure(hr));
                    }
                }
                m_forceFullDamage = true;
                return S_OK;
            }

        private:
            HRESULT CreateTarget() noexcept
            {
                D3D11_TEXTURE2D_DESC desc{};
                desc.Width = m_width;
                desc.Height = m_height;
                desc.MipLevels = 1;
                desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                IFC_RETURN(m_device->CreateTexture2D(&desc, nullptr, &m_targetTexture));
                ++m_churnCreated;

                ComPtr<IDXGISurface> surface;
                IFC_RETURN(m_targetTexture.As(&surface));
                const D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
                    D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                    96.0f * m_dpiScale, 96.0f * m_dpiScale);
                IFC_RETURN(m_context->CreateBitmapFromDxgiSurface(surface.Get(), &properties, &m_targetBitmap));
                ++m_counters.targetBitmapsCreated;
                ++m_churnCreated;
                m_context->SetTarget(m_targetBitmap.Get());
                return S_OK;
            }

            void ReleaseTarget() noexcept
            {
                m_context->SetTarget(nullptr);
                m_churnDestroyed += (m_targetBitmap ? 1 : 0) + (m_targetTexture ? 1 : 0) + (m_staging ? 1 : 0) + (m_scrollScratch ? 1 : 0);
                m_targetBitmap.Reset();
                m_targetTexture.Reset();
                m_staging.Reset();
                m_scrollScratch.Reset();
            }

            HRESULT ResizeSwapChain() noexcept
            {
                m_backBuffer.Reset();
                DXGI_SWAP_CHAIN_DESC1 desc{};
                IFC_RETURN(m_swapChain->GetDesc1(&desc));
                IFC_RETURN(m_swapChain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, desc.Flags));
                return S_OK;
            }

            // Copies the damage from the target into the back buffer and presents it with matching dirty rects.
            HRESULT PresentDamage() noexcept
            {
                if (m_damageCount == 0 && m_frameScrollCount == 0)
                {
                    return S_OK;
                }
                if (!m_backBuffer)
                {
                    // Flip model: buffer 0 always names the current back buffer, so the reference stays valid across
                    // Present calls until ResizeBuffers or detach.
                    IFC_RETURN(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer)));
                }

                RECT dirty[MaxDamageRects + MaxScrollRects];
                uint32_t dirtyCount = 0;
                uint64_t copyBytes = 0;
                const auto copyRect = [&](const PixelRect& rect)
                {
                    const D3D11_BOX box{ rect.left, rect.top, 0, rect.right, rect.bottom, 1 };
                    m_d3dContext->CopySubresourceRegion(m_backBuffer.Get(), 0, rect.left, rect.top, 0, m_targetTexture.Get(), 0, &box);
                    dirty[dirtyCount++] = RECT{ static_cast<LONG>(rect.left), static_cast<LONG>(rect.top), static_cast<LONG>(rect.right), static_cast<LONG>(rect.bottom) };
                    copyBytes += rect.Area() * 4;
                };
                for (uint32_t i = 0; i < m_damageCount; ++i)
                {
                    copyRect(m_damage[i]);
                }
                // Areas moved by ScrollPixels are presented without being repainted (a full-damage frame covers them).
                if (!m_frameFullDamage)
                {
                    for (uint32_t i = 0; i < m_frameScrollCount; ++i)
                    {
                        copyRect(m_frameScrollRects[i]);
                    }
                }

                // Outside the dirty rects, flip-model Present1 carries the previous frame forward into the new buffer.
                DXGI_PRESENT_PARAMETERS parameters{};
                if (!m_frameFullDamage)
                {
                    parameters.DirtyRectsCount = dirtyCount;
                    parameters.pDirtyRects = dirty;
                }
                IFC_RETURN(m_swapChain->Present1(0, 0, &parameters));

                m_presentCopyBytes = copyBytes;
                m_uploadBytes += m_identity.isWarp ? copyBytes : 0;
                return S_OK;
            }

            void ComputeDamage(_In_reads_opt_(count) const RectF* rects, uint32_t count) noexcept
            {
                m_damageCount = 0;
                m_damageBounds = {};
                m_frameFullDamage = m_forceFullDamage || rects == nullptr || count == 0;
                if (m_frameFullDamage)
                {
                    m_damage[0] = PixelRect{ 0, 0, m_width, m_height };
                    m_damageCount = 1;
                }
                else
                {
                    for (uint32_t i = 0; i < count; ++i)
                    {
                        const PixelRect rect = ToPixelRect(rects[i], m_width, m_height);
                        if (rect.IsEmpty())
                        {
                            continue;
                        }
                        if (m_damageCount == MaxDamageRects)
                        {
                            // Too many rects for one mask: merge everything into the bounding box.
                            PixelRect bounds = m_damage[0];
                            for (uint32_t j = 1; j < m_damageCount; ++j)
                            {
                                bounds = Union(bounds, m_damage[j]);
                            }
                            for (uint32_t j = i; j < count; ++j)
                            {
                                const PixelRect more = ToPixelRect(rects[j], m_width, m_height);
                                if (!more.IsEmpty())
                                {
                                    bounds = Union(bounds, more);
                                }
                            }
                            m_damage[0] = bounds;
                            m_damageCount = 1;
                            break;
                        }
                        m_damage[m_damageCount++] = rect;
                    }
                }

                for (uint32_t i = 0; i < m_damageCount; ++i)
                {
                    m_damageBounds = i == 0 ? m_damage[0] : Union(m_damageBounds, m_damage[i]);
                    m_stats.damagedPixels += m_damage[i].Area();
                }
                m_stats.damageRectCount = m_damageCount;
            }

            static PixelRect Union(const PixelRect& a, const PixelRect& b) noexcept
            {
                return PixelRect{ (std::min)(a.left, b.left), (std::min)(a.top, b.top), (std::max)(a.right, b.right), (std::max)(a.bottom, b.bottom) };
            }

            D2D1_RECT_F ToDips(const PixelRect& rect) const noexcept
            {
                return D2D1::RectF(static_cast<float>(rect.left) / m_dpiScale, static_cast<float>(rect.top) / m_dpiScale,
                    static_cast<float>(rect.right) / m_dpiScale, static_cast<float>(rect.bottom) / m_dpiScale);
            }

            HRESULT EnsureDamageGeometry() noexcept
            {
                if (m_damageGeometry && m_damageGeometryRectCount == m_damageCount)
                {
                    bool same = true;
                    for (uint32_t i = 0; i < m_damageCount && same; ++i)
                    {
                        same = m_damageGeometryRects[i] == m_damage[i];
                    }
                    if (same)
                    {
                        return S_OK;
                    }
                }
                ResetDamageGeometry();

                ComPtr<ID2D1RectangleGeometry> rectangles[MaxDamageRects];
                ID2D1Geometry* geometries[MaxDamageRects]{};
                for (uint32_t i = 0; i < m_damageCount; ++i)
                {
                    IFC_RETURN(m_factory->CreateRectangleGeometry(ToDips(m_damage[i]), &rectangles[i]));
                    geometries[i] = rectangles[i].Get();
                    ++m_counters.geometriesCreated;
                    ++m_churnCreated;
                }
                IFC_RETURN(m_factory->CreateGeometryGroup(D2D1_FILL_MODE_WINDING, geometries, m_damageCount, &m_damageGeometry));
                ++m_counters.geometriesCreated;
                ++m_churnCreated;
                m_damageGeometryObjects = m_damageCount + 1;
                m_damageGeometryRectCount = m_damageCount;
                for (uint32_t i = 0; i < m_damageCount; ++i)
                {
                    m_damageGeometryRects[i] = m_damage[i];
                }
                return S_OK;
            }

            void ResetDamageGeometry() noexcept
            {
                m_churnDestroyed += m_damageGeometryObjects;
                m_damageGeometry.Reset();
                m_damageGeometryObjects = 0;
                m_damageGeometryRectCount = 0;
            }

            // Expects an identity transform.
            void PushDamageClip() noexcept
            {
                m_damageUsesLayer = m_damageCount > 1;
                if (m_damageUsesLayer)
                {
                    const D2D1_LAYER_PARAMETERS1 parameters = D2D1::LayerParameters1(D2D1::InfiniteRect(), m_damageGeometry.Get(),
                        D2D1_ANTIALIAS_MODE_ALIASED, D2D1::IdentityMatrix(), 1.0f, nullptr, D2D1_LAYER_OPTIONS1_NONE);
                    m_context->PushLayer(&parameters, nullptr);
                }
                else
                {
                    // m_damage[0] is empty when all damage fell outside the target: everything is clipped away.
                    const PixelRect rect = m_damageCount == 1 ? m_damage[0] : PixelRect{};
                    m_context->PushAxisAlignedClip(ToDips(rect), D2D1_ANTIALIAS_MODE_ALIASED);
                }
            }

            void PopDamageClip() noexcept
            {
                if (m_damageUsesLayer)
                {
                    m_context->PopLayer();
                }
                else
                {
                    m_context->PopAxisAlignedClip();
                }
            }

            void PopEntry(StackEntry expected) noexcept
            {
                if (!m_frameActive)
                {
                    return;
                }
                if (m_overflowDepth > 0)
                {
                    --m_overflowDepth;
                    return;
                }
                if (m_depth == 0 || m_stack[m_depth - 1] != expected)
                {
                    return; // unbalanced pop; ignored rather than corrupting D2D's stack
                }
                --m_depth;
                if (expected == StackEntry::Clip)
                {
                    m_context->PopAxisAlignedClip();
                }
                else
                {
                    m_context->PopLayer();
                }
            }

            void UnwindStack() noexcept
            {
                m_overflowDepth = 0;
                while (m_depth > 0)
                {
                    PopEntry(m_stack[m_depth - 1]);
                }
            }

            // Overdraw estimate: the primitive's transformed bounds, in pixels, clipped to the damage bounds.
            void CountPrimitive(const RectF& rect) noexcept
            {
                ++m_stats.primitiveCount;
                const Matrix3x2F& m = m_transform;
                const float xs[] = { rect.left, rect.right };
                const float ys[] = { rect.top, rect.bottom };
                double minX = INFINITY, minY = INFINITY, maxX = -INFINITY, maxY = -INFINITY;
                for (const float x : xs)
                {
                    for (const float y : ys)
                    {
                        const double px = (static_cast<double>(x) * m.m11 + static_cast<double>(y) * m.m21 + m.dx) * m_dpiScale;
                        const double py = (static_cast<double>(x) * m.m12 + static_cast<double>(y) * m.m22 + m.dy) * m_dpiScale;
                        minX = std::fmin(minX, px);
                        maxX = std::fmax(maxX, px);
                        minY = std::fmin(minY, py);
                        maxY = std::fmax(maxY, py);
                    }
                }
                minX = std::fmax(minX, m_damageBounds.left);
                minY = std::fmax(minY, m_damageBounds.top);
                maxX = std::fmin(maxX, m_damageBounds.right);
                maxY = std::fmin(maxY, m_damageBounds.bottom);
                if (maxX > minX && maxY > minY)
                {
                    m_stats.paintedPixels += static_cast<uint64_t>((maxX - minX) * (maxY - minY) + 0.5);
                }
            }

            ComPtr<IDWriteTextFormat> GetTextFormat(const TextStyle& style) noexcept
            {
                const wchar_t* family = style.fontFamily != nullptr && style.fontFamily[0] != L'\0' ? style.fontFamily : L"Segoe UI";
                if (!(style.fontSize > 0))
                {
                    return nullptr;
                }
                const uint32_t weight = style.fontWeight < 1 ? 1 : (style.fontWeight > 999 ? 999 : style.fontWeight);
                const bool cacheable = wcsnlen(family, MaxFamilyLength) < MaxFamilyLength;
                if (cacheable)
                {
                    for (uint32_t i = 0; i < m_textFormatCount; ++i)
                    {
                        const TextFormatEntry& entry = m_textFormats[i];
                        if (entry.size == style.fontSize && entry.weight == weight && wcscmp(entry.family, family) == 0)
                        {
                            return entry.format;
                        }
                    }
                }

                ComPtr<IDWriteTextFormat> format;
                if (FAILED(m_dwriteFactory->CreateTextFormat(family, nullptr, static_cast<DWRITE_FONT_WEIGHT>(weight),
                    DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, style.fontSize, L"en-us", &format)))
                {
                    return nullptr;
                }
                ++m_counters.textFormatsCreated;
                ++m_churnCreated;
                if (!cacheable)
                {
                    ++m_churnDestroyed; // used for this call only
                    return format;
                }

                uint32_t slot = m_textFormatCount;
                if (slot == MaxTextFormats)
                {
                    slot = m_nextTextFormatEviction;
                    m_nextTextFormatEviction = (m_nextTextFormatEviction + 1) % MaxTextFormats;
                    ++m_churnDestroyed;
                }
                else
                {
                    ++m_textFormatCount;
                }
                TextFormatEntry& entry = m_textFormats[slot];
                wcscpy_s(entry.family, family);
                entry.size = style.fontSize;
                entry.weight = weight;
                entry.format = format;
                return format;
            }

            // Rounded-clip masks, cached by rect and radii (LRU). Returns nullptr only if creation fails.
            ID2D1Geometry* GetClipGeometry(const RectF& rect, float radiusX, float radiusY) noexcept
            {
                const float rx = radiusX > 0 ? radiusX : 0.0f;
                const float ry = radiusY > 0 ? radiusY : 0.0f;
                ++m_cacheTick;
                for (uint32_t i = 0; i < m_clipGeometryCount; ++i)
                {
                    ClipGeometryEntry& entry = m_clipGeometries[i];
                    if (entry.rect.left == rect.left && entry.rect.top == rect.top && entry.rect.right == rect.right &&
                        entry.rect.bottom == rect.bottom && entry.radiusX == rx && entry.radiusY == ry)
                    {
                        entry.lastUse = m_cacheTick;
                        return entry.geometry.Get();
                    }
                }

                ComPtr<ID2D1RoundedRectangleGeometry> geometry;
                if (FAILED(m_factory->CreateRoundedRectangleGeometry(D2D1::RoundedRect(ToD2D(rect), rx, ry), &geometry)))
                {
                    return nullptr;
                }
                ++m_counters.geometriesCreated;
                ++m_churnCreated;

                uint32_t slot = m_clipGeometryCount;
                if (slot == MaxClipGeometries)
                {
                    slot = 0;
                    for (uint32_t i = 1; i < MaxClipGeometries; ++i)
                    {
                        slot = m_clipGeometries[i].lastUse < m_clipGeometries[slot].lastUse ? i : slot;
                    }
                    ++m_churnDestroyed; // D2D keeps it alive while a pushed layer still references it
                }
                else
                {
                    ++m_clipGeometryCount;
                }
                ClipGeometryEntry& entry = m_clipGeometries[slot];
                entry.rect = rect;
                entry.radiusX = rx;
                entry.radiusY = ry;
                entry.lastUse = m_cacheTick;
                entry.geometry = geometry;
                return entry.geometry.Get();
            }

            // Path geometries: cached by (cacheKey, fill mode) when cacheKey != 0 (LRU), built per call otherwise.
            ComPtr<ID2D1PathGeometry> GetPathGeometry(const PathData& path, _Out_ RectF* bounds) noexcept
            {
                *bounds = {};
                ++m_cacheTick;
                if (path.cacheKey != 0)
                {
                    for (uint32_t i = 0; i < m_pathGeometryCount; ++i)
                    {
                        PathGeometryEntry& entry = m_pathGeometries[i];
                        if (entry.key == path.cacheKey && entry.nonZeroFill == path.nonZeroFill)
                        {
                            entry.lastUse = m_cacheTick;
                            *bounds = entry.bounds;
                            return entry.geometry;
                        }
                    }
                }

                ComPtr<ID2D1PathGeometry> geometry;
                if (FAILED(BuildPathGeometry(path, &geometry, bounds)))
                {
                    return nullptr;
                }
                ++m_counters.geometriesCreated;
                ++m_counters.pathGeometriesCreated;
                ++m_churnCreated;
                if (path.cacheKey == 0)
                {
                    ++m_churnDestroyed; // used for this call only
                    return geometry;
                }

                uint32_t slot = m_pathGeometryCount;
                if (slot == MaxPathGeometries)
                {
                    slot = 0;
                    for (uint32_t i = 1; i < MaxPathGeometries; ++i)
                    {
                        slot = m_pathGeometries[i].lastUse < m_pathGeometries[slot].lastUse ? i : slot;
                    }
                    ++m_churnDestroyed;
                }
                else
                {
                    ++m_pathGeometryCount;
                }
                PathGeometryEntry& entry = m_pathGeometries[slot];
                entry.key = path.cacheKey;
                entry.nonZeroFill = path.nonZeroFill;
                entry.lastUse = m_cacheTick;
                entry.bounds = *bounds;
                entry.geometry = geometry;
                return geometry;
            }

            // MoveTo starts a figure (ending an open one); LineTo/CubicTo without a figure start one at the current
            // point; Close ends the figure closed and returns to its start. Fills treat open figures as closed.
            // bounds is the control-point bounding box (an overestimate for curves), used for overdraw accounting.
            HRESULT BuildPathGeometry(const PathData& path, _Outptr_ ID2D1PathGeometry** result, _Out_ RectF* bounds) noexcept
            {
                *result = nullptr;
                if (path.verbCount == 0 || path.verbs == nullptr || (path.pointCount > 0 && path.points == nullptr))
                {
                    return E_INVALIDARG;
                }
                uint32_t needed = 0;
                for (uint32_t i = 0; i < path.verbCount; ++i)
                {
                    switch (path.verbs[i])
                    {
                    case PathVerb::MoveTo:
                    case PathVerb::LineTo: needed += 1; break;
                    case PathVerb::CubicTo: needed += 3; break;
                    case PathVerb::Close: break;
                    default: return E_INVALIDARG;
                    }
                }
                if (needed == 0 || needed > path.pointCount)
                {
                    return E_INVALIDARG;
                }

                ComPtr<ID2D1PathGeometry> geometry;
                ComPtr<ID2D1GeometrySink> sink;
                IFC_RETURN(m_factory->CreatePathGeometry(&geometry));
                IFC_RETURN(geometry->Open(&sink));
                sink->SetFillMode(path.nonZeroFill ? D2D1_FILL_MODE_WINDING : D2D1_FILL_MODE_ALTERNATE);

                float minX = INFINITY, minY = INFINITY, maxX = -INFINITY, maxY = -INFINITY;
                uint32_t next = 0;
                const auto take = [&]()
                {
                    const PointF& p = path.points[next++];
                    minX = std::fmin(minX, p.x);
                    maxX = std::fmax(maxX, p.x);
                    minY = std::fmin(minY, p.y);
                    maxY = std::fmax(maxY, p.y);
                    return D2D1::Point2F(p.x, p.y);
                };
                bool open = false;
                D2D1_POINT_2F start{}, current{};
                const auto ensureFigure = [&]()
                {
                    if (!open)
                    {
                        sink->BeginFigure(current, D2D1_FIGURE_BEGIN_FILLED);
                        start = current;
                        open = true;
                    }
                };
                for (uint32_t i = 0; i < path.verbCount; ++i)
                {
                    switch (path.verbs[i])
                    {
                    case PathVerb::MoveTo:
                        if (open)
                        {
                            sink->EndFigure(D2D1_FIGURE_END_OPEN);
                            open = false;
                        }
                        current = take();
                        ensureFigure();
                        break;
                    case PathVerb::LineTo:
                    {
                        ensureFigure();
                        current = take();
                        sink->AddLine(current);
                        break;
                    }
                    case PathVerb::CubicTo:
                    {
                        ensureFigure();
                        const D2D1_POINT_2F c1 = take();
                        const D2D1_POINT_2F c2 = take();
                        current = take();
                        sink->AddBezier(D2D1::BezierSegment(c1, c2, current));
                        break;
                    }
                    case PathVerb::Close:
                        if (open)
                        {
                            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                            open = false;
                            current = start;
                        }
                        break;
                    }
                }
                if (open)
                {
                    sink->EndFigure(D2D1_FIGURE_END_OPEN);
                }
                IFC_RETURN(sink->Close());
                *bounds = RectF{ minX, minY, maxX, maxY };
                *result = geometry.Detach();
                return S_OK;
            }

            void AddPendingScrollRect(const PixelRect& rect) noexcept
            {
                if (m_pendingScrollCount == MaxScrollRects)
                {
                    PixelRect bounds = rect;
                    for (uint32_t i = 0; i < m_pendingScrollCount; ++i)
                    {
                        bounds = Union(bounds, m_pendingScrollRects[i]);
                    }
                    m_pendingScrollRects[0] = bounds;
                    m_pendingScrollCount = 1;
                    return;
                }
                m_pendingScrollRects[m_pendingScrollCount++] = rect;
            }

            HRESULT CheckDevice() noexcept
            {
                if (!m_lost)
                {
                    const HRESULT removed = m_device->GetDeviceRemovedReason();
                    if (FAILED(removed))
                    {
                        MarkDeviceLost(removed);
                    }
                }
                return m_lost ? m_lostHr : S_OK;
            }

            // Classifies a failed call: device loss makes the painter permanently failed; anything else is returned as is.
            HRESULT HandleFailure(HRESULT hr) noexcept
            {
                if (!m_lost && (IsDeviceLoss(hr) || FAILED(m_device->GetDeviceRemovedReason())))
                {
                    MarkDeviceLost(hr);
                }
                return m_lost ? m_lostHr : hr;
            }

            void MarkDeviceLost(HRESULT observed) noexcept
            {
                const HRESULT removedReason = m_device->GetDeviceRemovedReason();
                m_lost = true;
                m_lostHr = FAILED(removedReason) ? removedReason : (FAILED(observed) && observed != D2DERR_RECREATE_TARGET ? observed : DXGI_ERROR_DEVICE_REMOVED);
                TraceDeviceLost(m_identity.mode, observed, removedReason, m_frameNumber);
            }

            void EmitChurn() noexcept
            {
                if (m_churnCreated != 0 || m_churnDestroyed != 0)
                {
                    TraceResourceChurn(m_identity.mode, m_frameNumber, m_churnCreated, m_churnDestroyed);
                    m_churnCreated = 0;
                    m_churnDestroyed = 0;
                }
            }

            ComPtr<ID3D11Device> m_device;
            ComPtr<ID3D11DeviceContext> m_d3dContext;
            ComPtr<ID2D1Factory1> m_factory;
            ComPtr<ID2D1Device> m_d2dDevice;
            ComPtr<ID2D1DeviceContext> m_context;
            ComPtr<IDWriteFactory> m_dwriteFactory;
            ComPtr<ID2D1SolidColorBrush> m_brush;
            ComPtr<ID3D11Texture2D> m_targetTexture;
            ComPtr<ID2D1Bitmap1> m_targetBitmap;
            ComPtr<ID3D11Texture2D> m_staging;
            ComPtr<ID3D11Texture2D> m_scrollScratch;
            ComPtr<IDXGISwapChain1> m_swapChain;
            ComPtr<ID3D11Texture2D> m_backBuffer;

            DeviceIdentity m_identity;
            uint32_t m_width = 0;
            uint32_t m_height = 0;
            float m_dpiScale = 1.0f;

            GpuTimestampProfiler m_profiler;
            bool m_profileGpu = false;

            bool m_lost = false;
            HRESULT m_lostHr = S_OK;

            bool m_frameActive = false;
            bool m_forceFullDamage = true; // after creation, attach, and Resize
            bool m_frameFullDamage = false;
            uint32_t m_frameNumber = 0;
            FrameStats m_stats;
            Matrix3x2F m_transform;
            uint64_t m_uploadBytes = 0;
            uint64_t m_pendingUploadBytes = 0; // EnsureBitmap copies made outside a frame
            uint64_t m_presentCopyBytes = 0;

            PixelRect m_pendingScrollRects[MaxScrollRects];
            uint32_t m_pendingScrollCount = 0;
            PixelRect m_frameScrollRects[MaxScrollRects];
            uint32_t m_frameScrollCount = 0;

            uint64_t m_cacheTick = 0;
            ClipGeometryEntry m_clipGeometries[MaxClipGeometries];
            uint32_t m_clipGeometryCount = 0;
            PathGeometryEntry m_pathGeometries[MaxPathGeometries];
            uint32_t m_pathGeometryCount = 0;
            std::unordered_map<uint64_t, BitmapEntry> m_bitmaps;
            uint64_t m_bitmapBytes = 0;

            PixelRect m_damage[MaxDamageRects];
            uint32_t m_damageCount = 0;
            PixelRect m_damageBounds;
            bool m_damageUsesLayer = false;
            ComPtr<ID2D1GeometryGroup> m_damageGeometry;
            PixelRect m_damageGeometryRects[MaxDamageRects];
            uint32_t m_damageGeometryRectCount = 0;
            uint32_t m_damageGeometryObjects = 0;

            StackEntry m_stack[MaxStackDepth]{};
            uint32_t m_depth = 0;
            uint32_t m_overflowDepth = 0;

            TextFormatEntry m_textFormats[MaxTextFormats];
            uint32_t m_textFormatCount = 0;
            uint32_t m_nextTextFormatEviction = 0;

            D2DResourceCounters m_counters;
            uint32_t m_churnCreated = 0;
            uint32_t m_churnDestroyed = 0;
        };
    }

    HRESULT CreateD2DPainter(Mode mode, ID3D11Device* device, const PainterOptions& options, std::unique_ptr<ID2DImmediatePainter>& painter) noexcept
    {
        painter.reset();
        if ((mode != Mode::ImmediateD2DHardware && mode != Mode::ImmediateD2DSoftware) || device == nullptr ||
            options.width == 0 || options.height == 0 || !(options.dpiScale > 0))
        {
            return E_INVALIDARG;
        }

        std::unique_ptr<D2DPainter> created(new (std::nothrow) D2DPainter());
        if (!created)
        {
            return E_OUTOFMEMORY;
        }
        IFC_RETURN(created->Initialize(mode, device, options));
        painter = std::move(created);
        return S_OK;
    }

    HRESULT CreateD2DPainter(Mode mode, ID3D11Device* device, const PainterOptions& options, std::unique_ptr<IImmediatePainter>& painter) noexcept
    {
        std::unique_ptr<ID2DImmediatePainter> d2dPainter;
        const HRESULT hr = CreateD2DPainter(mode, device, options, d2dPainter);
        painter = std::move(d2dPainter);
        return hr;
    }
}
