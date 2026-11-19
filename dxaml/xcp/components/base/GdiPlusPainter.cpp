// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>
#include <GdiPlusPainter.h>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <vector>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

// GDI+ headers call min/max unqualified; precomp.h undefines the macros.
namespace Gdiplus
{
    using std::min;
    using std::max;
}
#include <objidl.h>
#include <gdiplus.h>

// gdiplus.dll is delay-loaded by Microsoft.ui.xaml.dll (dxaml\xcp\dxaml\dllsrv\winrt\sources.props), so processes that
// never create a GDI+ painter never load it.
#pragma comment(lib, "gdiplus.lib")

// GDI+ immediate painter (CPU, antialiased, PARGB). Owned by the immediate-rendering-gdiplus branch.
//
// Contract for the implementation (tests: unittests\GdiPlusPainterTests.cpp):
// - Runtime: every painter holds a GdiPlus::RuntimeReference. GdiplusStartup/GdiplusShutdown are refcounted across
//   painters; destroying the last painter shuts GDI+ down, and a later painter starts it again.
// - Target: a Gdiplus::Bitmap in PixelFormat32bppPARGB that wraps a buffer owned by the painter. ReadPixels returns its
//   bytes unchanged (already premultiplied BGRA), so a 50% alpha fill on a transparent clear reads back with color
//   channels scaled by alpha.
// - Quality: SmoothingModeAntiAlias for geometry; PixelOffsetModeHalf so pixel centers are at +0.5 like D2D and
//   integer-aligned rectangles have crisp edges. Text is drawn with GDI+ (Graphics::DrawString) using
//   TextRenderingHintAntiAlias, i.e. grayscale antialiasing, never ClearType, because the target has per-pixel alpha
//   (ClearType's per-channel coverage cannot be stored in it).
// - Damage and clips: the Graphics clip is the union over damage rects of (damage rect intersected with the current
//   axis-aligned clip), in device pixels. Clear uses Graphics::Clear, which replaces pixels inside the clip.
// - Opacity groups: children are drawn into a pooled full-size PARGB layer, then blended once with DrawImage and an
//   ImageAttributes color matrix that scales alpha.
// - Resources: brushes, pens, fonts, font families, the string format, layers, and the image attributes are cached; an
//   identical second frame creates none (GetLastFrameResourceChurn reports 0 created / 0 destroyed). Caches are
//   bounded, so steady-state frames do not grow ResourceInventory::cachedObjectCount.
// - Presentation and upload/copy accounting: GDI+ cannot draw to a presentable surface while preserving alpha, so each
//   frame paints into the CPU bitmap. With a composition swap chain attached (on any D3D11 device), EndFrame copies
//   ONLY the damaged region(s) into the back buffer with UpdateSubresource and calls Present1 with them as dirty rects;
//   uploadBytesThisFrame is that copy, DamageCopyBytes(width, height, damage) == FrameStats::damagedPixels * 4. The
//   first frame after attach or Resize is fully damaged and copies the whole bitmap. Without a swap chain, frames are
//   offscreen only and report 0, like D2D-SW.
namespace RenderingBackend
{
    namespace GdiPlus
    {
        namespace
        {
            std::mutex s_runtimeLock;
            uint32_t s_runtimeReferences = 0;
            ULONG_PTR s_runtimeToken = 0;

            HRESULT HResultFromStatus(Gdiplus::Status status) noexcept
            {
                switch (status)
                {
                    case Gdiplus::Ok: return S_OK;
                    case Gdiplus::InvalidParameter: return E_INVALIDARG;
                    case Gdiplus::OutOfMemory: return E_OUTOFMEMORY;
                    case Gdiplus::NotImplemented: return E_NOTIMPL;
                    case Gdiplus::Win32Error: return HRESULT_FROM_WIN32(GetLastError());
                    default: return E_FAIL;
                }
            }

            uint32_t ClampEdge(float value, uint32_t limit, bool roundUp) noexcept
            {
                const float rounded = roundUp ? std::ceil(value) : std::floor(value);
                if (!(rounded > 0))
                {
                    return 0;
                }
                return rounded >= static_cast<float>(limit) ? limit : static_cast<uint32_t>(rounded);
            }
        }

        HRESULT RuntimeReference::Acquire() noexcept
        {
            if (m_held)
            {
                return S_OK;
            }
            std::lock_guard<std::mutex> lock(s_runtimeLock);
            if (s_runtimeReferences == 0)
            {
                Gdiplus::GdiplusStartupInput input;
                ULONG_PTR token = 0;
                const HRESULT hr = HResultFromStatus(Gdiplus::GdiplusStartup(&token, &input, nullptr));
                if (FAILED(hr))
                {
                    return hr;
                }
                s_runtimeToken = token;
            }
            ++s_runtimeReferences;
            m_held = true;
            return S_OK;
        }

        void RuntimeReference::Release() noexcept
        {
            if (!m_held)
            {
                return;
            }
            m_held = false;
            std::lock_guard<std::mutex> lock(s_runtimeLock);
            if (--s_runtimeReferences == 0)
            {
                Gdiplus::GdiplusShutdown(s_runtimeToken);
                s_runtimeToken = 0;
            }
        }

        uint32_t GetRuntimeReferenceCount() noexcept
        {
            std::lock_guard<std::mutex> lock(s_runtimeLock);
            return s_runtimeReferences;
        }

        bool IsRuntimeStarted() noexcept
        {
            std::lock_guard<std::mutex> lock(s_runtimeLock);
            return s_runtimeToken != 0;
        }

        uint64_t DamagedPixelCount(uint32_t width, uint32_t height, const RectF* damageRects, uint32_t count) noexcept
        {
            if (damageRects == nullptr || count == 0)
            {
                return static_cast<uint64_t>(width) * height;
            }
            uint64_t total = 0;
            for (uint32_t index = 0; index < count; ++index)
            {
                const RectF& rect = damageRects[index];
                const uint32_t left = ClampEdge(rect.left, width, false);
                const uint32_t top = ClampEdge(rect.top, height, false);
                const uint32_t right = ClampEdge(rect.right, width, true);
                const uint32_t bottom = ClampEdge(rect.bottom, height, true);
                if (right > left && bottom > top)
                {
                    total += static_cast<uint64_t>(right - left) * (bottom - top);
                }
            }
            return total;
        }

        uint64_t DamageCopyBytes(uint32_t width, uint32_t height, const RectF* damageRects, uint32_t count) noexcept
        {
            constexpr uint64_t c_bytesPerPixel = 4; // PARGB
            return DamagedPixelCount(width, height, damageRects, count) * c_bytesPerPixel;
        }

        namespace
        {
            struct PixelRect
            {
                int32_t left = 0, top = 0, right = 0, bottom = 0;

                bool IsEmpty() const noexcept { return right <= left || bottom <= top; }
                uint64_t Area() const noexcept { return IsEmpty() ? 0 : static_cast<uint64_t>(right - left) * static_cast<uint64_t>(bottom - top); }
            };

            PixelRect Intersect(const PixelRect& a, const PixelRect& b) noexcept
            {
                return { std::max(a.left, b.left), std::max(a.top, b.top), std::min(a.right, b.right), std::min(a.bottom, b.bottom) };
            }

            int32_t ToPixel(float value) noexcept
            {
                constexpr float c_limit = 1 << 24;
                if (!(value > -c_limit))
                {
                    return -(1 << 24);
                }
                return value < c_limit ? static_cast<int32_t>(value) : (1 << 24);
            }

            BYTE ToByte(float value) noexcept
            {
                if (!(value > 0))
                {
                    return 0;
                }
                return value >= 1 ? 255 : static_cast<BYTE>(std::lround(value * 255));
            }

            Gdiplus::Color ToGdiColor(const ColorF& color) noexcept
            {
                return Gdiplus::Color(ToByte(color.a), ToByte(color.r), ToByte(color.g), ToByte(color.b));
            }

            Gdiplus::RectF ToGdiRect(const RectF& rect) noexcept
            {
                return Gdiplus::RectF(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
            }

            template <typename T>
            HRESULT CheckCreated(const std::unique_ptr<T>& object) noexcept
            {
                return object ? HResultFromStatus(object->GetLastStatus()) : E_OUTOFMEMORY;
            }

            // Bounded caches: an animation that keeps producing new colors or sizes evicts (and reports churn) instead of
            // growing without limit. Byte costs are estimates of GDI+'s native object sizes for ResourceInventory.
            constexpr size_t c_maxBrushes = 256;
            constexpr size_t c_maxPens = 128;
            constexpr size_t c_maxFonts = 32;
            constexpr uint64_t c_brushBytes = 64;
            constexpr uint64_t c_penBytes = 160;
            constexpr uint64_t c_fontBytes = 1024;
            constexpr uint64_t c_fontFamilyBytes = 512;
            constexpr uint64_t c_stringFormatBytes = 256;
            constexpr uint64_t c_imageAttributesBytes = 512;
            constexpr uint64_t c_pathBytes = 512;
            constexpr uint64_t c_matrixBytes = 64;
            constexpr uint32_t c_maxDimension = 1 << 15;

            // A PARGB bitmap over a painter-owned buffer, with its Graphics. Used for the target and opacity layers.
            struct Surface
            {
                std::vector<uint8_t> pixels;
                std::unique_ptr<Gdiplus::Bitmap> bitmap;
                std::unique_ptr<Gdiplus::Graphics> graphics;
            };

            struct LayerRecord
            {
                float opacity = 1;
                PixelRect bounds;
                bool active = false;
            };

            struct FontKey
            {
                std::wstring family;
                float size = 0;
                INT style = Gdiplus::FontStyleRegular;

                bool operator<(const FontKey& other) const noexcept
                {
                    if (size != other.size) return size < other.size;
                    if (style != other.style) return style < other.style;
                    return family < other.family;
                }
            };

            class GdiPlusPainter final : public IImmediatePainter
            {
            public:
                HRESULT Initialize(const PainterOptions& options) noexcept
                {
                    IFC_RETURN(m_runtime.Acquire());
                    m_transform.reset(new Gdiplus::Matrix());
                    IFC_RETURN(CheckCreated(m_transform));
                    m_path.reset(new Gdiplus::GraphicsPath());
                    IFC_RETURN(CheckCreated(m_path));
                    IFC_RETURN(Resize(options.width, options.height, options.dpiScale));
                    return S_OK;
                }

                ResourceChurn LastFrameChurn() const noexcept { return m_lastChurn; }

                DeviceIdentity GetDeviceIdentity() const noexcept override
                {
                    DeviceIdentity identity{};
                    identity.mode = Mode::ImmediateGdiPlus;
                    return identity;
                }

                HRESULT Resize(uint32_t width, uint32_t height, float dpiScale) noexcept override
                {
                    if (m_inFrame)
                    {
                        return E_UNEXPECTED;
                    }
                    if (width == 0 || height == 0 || width > c_maxDimension || height > c_maxDimension || !(dpiScale > 0) || !std::isfinite(dpiScale))
                    {
                        return E_INVALIDARG;
                    }

                    Surface target;
                    IFC_RETURN(CreateSurface(width, height, target));
                    if (m_swapChain)
                    {
                        DXGI_SWAP_CHAIN_DESC1 desc{};
                        IFC_RETURN(m_swapChain->GetDesc1(&desc));
                        IFC_RETURN(m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, desc.Flags));
                    }

                    // Layers are sized to the target; drop them and let the next frame recreate what it needs.
                    m_frameDestroyed += 2 * static_cast<uint32_t>(m_layers.size());
                    m_layers.clear();
                    if (m_target.bitmap)
                    {
                        m_frameDestroyed += 2;
                    }
                    m_target = std::move(target);
                    m_width = width;
                    m_height = height;
                    m_dpiScale = dpiScale;
                    m_forceFullFrame = true;
                    return S_OK;
                }

                HRESULT BeginFrame(const RectF* damageRects, uint32_t damageRectCount) noexcept override
                {
                    if (m_inFrame)
                    {
                        return E_UNEXPECTED;
                    }
                    m_frameHr = S_OK;
                    m_stats = {};
                    m_uploadBytes = 0;
                    m_frameCreated = 0;
                    m_frameDestroyed = 0;
                    m_presentFull = m_forceFullFrame;
                    m_forceFullFrame = false;

                    const PixelRect full{ 0, 0, static_cast<int32_t>(m_width), static_cast<int32_t>(m_height) };
                    m_damage.clear();
                    m_clipStack.clear();
                    m_layerStack.clear();
                    if (m_presentFull || damageRects == nullptr || damageRectCount == 0)
                    {
                        m_damage.push_back(full);
                    }
                    else
                    {
                        for (uint32_t index = 0; index < damageRectCount; ++index)
                        {
                            const RectF& rect = damageRects[index];
                            const PixelRect pixels
                            {
                                static_cast<int32_t>(ClampEdge(rect.left, m_width, false)),
                                static_cast<int32_t>(ClampEdge(rect.top, m_height, false)),
                                static_cast<int32_t>(ClampEdge(rect.right, m_width, true)),
                                static_cast<int32_t>(ClampEdge(rect.bottom, m_height, true)),
                            };
                            if (!pixels.IsEmpty())
                            {
                                m_damage.push_back(pixels);
                            }
                        }
                    }

                    m_damageBounds = {};
                    for (const PixelRect& rect : m_damage)
                    {
                        m_stats.damagedPixels += rect.Area();
                        m_damageBounds = m_damageBounds.IsEmpty() ? rect : PixelRect{ std::min(m_damageBounds.left, rect.left),
                            std::min(m_damageBounds.top, rect.top), std::max(m_damageBounds.right, rect.right), std::max(m_damageBounds.bottom, rect.bottom) };
                    }
                    m_stats.damageRectCount = static_cast<uint32_t>(m_damage.size());

                    m_userTransform = {};
                    UpdateDeviceTransform();
                    m_inFrame = true;
                    ApplyState(*m_target.graphics);
                    return S_OK;
                }

                void Clear(const ColorF& color) noexcept override
                {
                    if (!CanDraw())
                    {
                        return;
                    }
                    Record(Current().Clear(ToGdiColor(color)));
                    m_stats.paintedPixels += VisibleArea(FullTarget());
                }

                void FillRectangle(const RectF& rect, const ColorF& color) noexcept override
                {
                    if (!BeginPrimitive())
                    {
                        return;
                    }
                    if (Gdiplus::SolidBrush* brush = GetBrush(color))
                    {
                        Record(Current().FillRectangle(brush, ToGdiRect(rect)));
                        m_stats.paintedPixels += VisibleArea(DeviceBounds(rect, 0));
                    }
                }

                void FillRoundedRectangle(const RectF& rect, float radiusX, float radiusY, const ColorF& color) noexcept override
                {
                    if (!BeginPrimitive())
                    {
                        return;
                    }
                    Gdiplus::SolidBrush* brush = GetBrush(color);
                    if (!brush)
                    {
                        return;
                    }
                    if (BuildRoundedPath(rect, radiusX, radiusY))
                    {
                        Record(Current().FillPath(brush, m_path.get()));
                    }
                    else
                    {
                        Record(Current().FillRectangle(brush, ToGdiRect(rect)));
                    }
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(rect, 0));
                }

                void StrokeRoundedRectangle(const RectF& rect, float radiusX, float radiusY, float thickness, const ColorF& color) noexcept override
                {
                    if (!BeginPrimitive() || !(thickness > 0))
                    {
                        return;
                    }
                    Gdiplus::Pen* pen = GetPen(color, thickness);
                    if (!pen)
                    {
                        return;
                    }
                    if (BuildRoundedPath(rect, radiusX, radiusY))
                    {
                        Record(Current().DrawPath(pen, m_path.get()));
                    }
                    else
                    {
                        Record(Current().DrawRectangle(pen, ToGdiRect(rect)));
                    }
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(rect, thickness / 2));
                }

                void DrawText(const wchar_t* text, uint32_t length, const TextStyle& style, const RectF& layoutRect, const ColorF& color) noexcept override
                {
                    if (!BeginPrimitive() || text == nullptr || length == 0 || !(style.fontSize > 0))
                    {
                        return;
                    }
                    Gdiplus::Font* font = GetFont(style);
                    Gdiplus::StringFormat* format = GetStringFormat();
                    Gdiplus::SolidBrush* brush = GetBrush(color);
                    if (!font || !format || !brush)
                    {
                        return;
                    }
                    const INT count = static_cast<INT>(std::min<uint32_t>(length, INT_MAX));
                    Record(Current().DrawString(text, count, font, ToGdiRect(layoutRect), format, brush));
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(layoutRect, 0));
                }

                void PushAxisAlignedClip(const RectF& clip) noexcept override
                {
                    if (!m_inFrame)
                    {
                        return;
                    }
                    // Aliased clip, like D2D1_ANTIALIAS_MODE_ALIASED: the device-space bounds snapped to pixel edges.
                    const PixelRect snapped = TransformBounds(clip, 0, true);
                    m_clipStack.push_back(Intersect(CurrentClip(), snapped));
                    ApplyState(Current());
                }

                void PopAxisAlignedClip() noexcept override
                {
                    if (!m_inFrame || m_clipStack.empty())
                    {
                        return;
                    }
                    m_clipStack.pop_back();
                    ApplyState(Current());
                }

                void SetTransform(const Matrix3x2F& transform) noexcept override
                {
                    m_userTransform = transform;
                    UpdateDeviceTransform();
                    if (m_inFrame)
                    {
                        Record(Current().SetTransform(m_transform.get()));
                    }
                }

                void PushOpacityLayer(float opacity) noexcept override
                {
                    if (!m_inFrame)
                    {
                        return;
                    }
                    LayerRecord record{};
                    record.opacity = opacity;
                    record.bounds = Intersect(CurrentClip(), m_damageBounds);
                    const size_t depth = m_layerStack.size();
                    if (SUCCEEDED(m_frameHr) && depth == m_layers.size())
                    {
                        auto layer = std::make_unique<Surface>();
                        HRESULT hr = layer ? CreateSurface(m_width, m_height, *layer) : E_OUTOFMEMORY;
                        if (SUCCEEDED(hr))
                        {
                            m_layers.push_back(std::move(layer));
                        }
                        Fail(hr);
                    }
                    record.active = SUCCEEDED(m_frameHr) && depth < m_layers.size();
                    m_layerStack.push_back(record);
                    if (record.active)
                    {
                        Gdiplus::Graphics& layer = *m_layers[depth]->graphics;
                        ApplyState(layer);
                        Record(layer.Clear(Gdiplus::Color(0, 0, 0, 0)));
                    }
                }

                // Extended primitives: not implemented yet (immediate-rendering extended scenes).
                void PushRoundedRectangleClip(const RectF&, float, float) noexcept override {}
                void PopRoundedRectangleClip() noexcept override {}
                void FillPath(const PathData&, const ColorF&) noexcept override {}
                void StrokePath(const PathData&, float, const ColorF&) noexcept override {}
                void DrawGlyphRun(const GlyphRunDesc&, const ColorF&) noexcept override {}
                HRESULT EnsureBitmap(const BitmapSource&) noexcept override { return E_NOTIMPL; }
                void DrawBitmap(uint64_t, const RectF&, const RectF*, float) noexcept override {}
                void ReleaseBitmap(uint64_t) noexcept override {}
                HRESULT ScrollPixels(const RectF&, int32_t, int32_t) noexcept override { return E_NOTIMPL; }

                void PopOpacityLayer() noexcept override
                {
                    if (!m_inFrame || m_layerStack.empty())
                    {
                        return;
                    }
                    const LayerRecord record = m_layerStack.back();
                    const size_t depth = m_layerStack.size() - 1;
                    m_layerStack.pop_back();
                    Gdiplus::Graphics& parent = Current();
                    ApplyState(parent);
                    if (!record.active || FAILED(m_frameHr) || record.bounds.IsEmpty() || !(record.opacity > 0))
                    {
                        return;
                    }

                    Surface& layer = *m_layers[depth];
                    layer.graphics->Flush(Gdiplus::FlushIntentionSync);
                    Gdiplus::ImageAttributes* attributes = nullptr;
                    if (record.opacity < 1)
                    {
                        attributes = GetImageAttributes();
                        if (!attributes)
                        {
                            return;
                        }
                        // GDI+ applies color matrices to straight (unpremultiplied) color: scale alpha only.
                        Gdiplus::ColorMatrix matrix =
                        { {
                            { 1, 0, 0, 0, 0 },
                            { 0, 1, 0, 0, 0 },
                            { 0, 0, 1, 0, 0 },
                            { 0, 0, 0, record.opacity, 0 },
                            { 0, 0, 0, 0, 1 },
                        } };
                        Record(attributes->SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap));
                    }

                    // Identity transform, integer rects, nearest-neighbor sampling, and half pixel offset: a 1:1 copy.
                    const PixelRect& b = record.bounds;
                    const INT width = b.right - b.left;
                    const INT height = b.bottom - b.top;
                    Record(parent.ResetTransform());
                    Record(parent.DrawImage(layer.bitmap.get(), Gdiplus::Rect(b.left, b.top, width, height), b.left, b.top, width, height,
                        Gdiplus::UnitPixel, attributes));
                    Record(parent.SetTransform(m_transform.get()));
                    m_stats.paintedPixels += VisibleArea(b);
                }

                HRESULT EndFrame(FrameStats* stats) noexcept override
                {
                    if (stats)
                    {
                        *stats = {};
                    }
                    if (!m_inFrame)
                    {
                        return E_UNEXPECTED;
                    }
                    while (!m_layerStack.empty())
                    {
                        PopOpacityLayer();
                    }
                    m_clipStack.clear();
                    m_target.graphics->Flush(Gdiplus::FlushIntentionSync);
                    m_inFrame = false;

                    HRESULT hr = m_frameHr;
                    if (SUCCEEDED(hr) && m_swapChain)
                    {
                        hr = Present();
                    }
                    if (FAILED(hr))
                    {
                        m_forceFullFrame = true;
                    }
                    m_lastChurn = { m_frameCreated, m_frameDestroyed };
                    if (stats)
                    {
                        *stats = m_stats;
                    }
                    return hr;
                }

                HRESULT ReadPixels(uint8_t* buffer, uint32_t stride, uint32_t bufferSize) noexcept override
                {
                    const uint64_t rowBytes = static_cast<uint64_t>(m_width) * 4;
                    if (buffer == nullptr || stride < rowBytes || bufferSize < static_cast<uint64_t>(stride) * (m_height - 1) + rowBytes)
                    {
                        return E_INVALIDARG;
                    }
                    m_target.graphics->Flush(Gdiplus::FlushIntentionSync);
                    const uint8_t* source = m_target.pixels.data();
                    for (uint32_t y = 0; y < m_height; ++y)
                    {
                        memcpy(buffer + static_cast<size_t>(y) * stride, source + static_cast<size_t>(y) * rowBytes, static_cast<size_t>(rowBytes));
                    }
                    return S_OK;
                }

                ResourceInventory GetResourceInventory() const noexcept override
                {
                    ResourceInventory inventory{};
                    const uint64_t surfaceBytes = static_cast<uint64_t>(m_width) * m_height * 4;
                    inventory.surfaceBytes = surfaceBytes * (1 + m_layers.size());
                    const uint32_t families = static_cast<uint32_t>(m_fontFamilies.size());
                    const uint32_t singletons = (m_stringFormat ? 1 : 0) + (m_imageAttributes ? 1 : 0) + (m_path ? 1 : 0) + (m_transform ? 1 : 0);
                    inventory.cachedObjectCount = static_cast<uint32_t>(m_brushes.size() + m_pens.size() + m_fonts.size()) + families + singletons;
                    inventory.cacheBytes = m_brushes.size() * c_brushBytes + m_pens.size() * c_penBytes + m_fonts.size() * c_fontBytes +
                        families * c_fontFamilyBytes + (m_stringFormat ? c_stringFormatBytes : 0) + (m_imageAttributes ? c_imageAttributesBytes : 0) +
                        (m_path ? c_pathBytes : 0) + (m_transform ? c_matrixBytes : 0);
                    inventory.uploadBytesThisFrame = m_uploadBytes;
                    return inventory;
                }

                HRESULT SetPresentationSwapChain(IDXGISwapChain1* swapChain) noexcept override
                {
                    if (m_inFrame)
                    {
                        return E_UNEXPECTED;
                    }
                    if (swapChain == nullptr)
                    {
                        m_swapChain.Reset();
                        m_context.Reset();
                        return S_OK;
                    }

                    DXGI_SWAP_CHAIN_DESC1 desc{};
                    IFC_RETURN(swapChain->GetDesc1(&desc));
                    if (desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM)
                    {
                        return E_INVALIDARG;
                    }
                    // Any D3D11 device: the copy is a CPU upload (UpdateSubresource), so no device sharing is needed.
                    Microsoft::WRL::ComPtr<ID3D11Device> device;
                    IFC_RETURN(swapChain->GetDevice(IID_PPV_ARGS(&device)));
                    if (desc.Width != m_width || desc.Height != m_height)
                    {
                        IFC_RETURN(swapChain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, desc.Flags));
                    }
                    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
                    device->GetImmediateContext(&context);
                    m_swapChain = swapChain;
                    m_context = std::move(context);
                    m_forceFullFrame = true;
                    return S_OK;
                }

            private:
                HRESULT CreateSurface(uint32_t width, uint32_t height, Surface& surface) noexcept
                {
                    surface.pixels.assign(static_cast<size_t>(width) * height * 4, 0);
                    surface.bitmap.reset(new Gdiplus::Bitmap(static_cast<INT>(width), static_cast<INT>(height), static_cast<INT>(width * 4),
                        PixelFormat32bppPARGB, surface.pixels.data()));
                    IFC_RETURN(CheckCreated(surface.bitmap));
                    ++m_frameCreated;
                    surface.graphics.reset(new Gdiplus::Graphics(surface.bitmap.get()));
                    IFC_RETURN(CheckCreated(surface.graphics));
                    ++m_frameCreated;

                    Gdiplus::Graphics& graphics = *surface.graphics;
                    IFC_RETURN(HResultFromStatus(graphics.SetPageUnit(Gdiplus::UnitPixel)));
                    IFC_RETURN(HResultFromStatus(graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias)));
                    IFC_RETURN(HResultFromStatus(graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf)));
                    IFC_RETURN(HResultFromStatus(graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias)));
                    IFC_RETURN(HResultFromStatus(graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver)));
                    IFC_RETURN(HResultFromStatus(graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor)));
                    return S_OK;
                }

                HRESULT Present() noexcept
                {
                    if (m_damage.empty())
                    {
                        return S_OK;
                    }
                    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
                    IFC_RETURN(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

                    const uint32_t stride = m_width * 4;
                    const uint8_t* pixels = m_target.pixels.data();
                    DXGI_PRESENT_PARAMETERS parameters{};
                    uint64_t copied = 0;
                    if (m_presentFull)
                    {
                        m_context->UpdateSubresource(backBuffer.Get(), 0, nullptr, pixels, stride, 0);
                        copied = static_cast<uint64_t>(m_width) * m_height * 4;
                    }
                    else
                    {
                        m_dirtyRects.resize(m_damage.size());
                        for (size_t index = 0; index < m_damage.size(); ++index)
                        {
                            const PixelRect& rect = m_damage[index];
                            const D3D11_BOX box{ static_cast<UINT>(rect.left), static_cast<UINT>(rect.top), 0,
                                static_cast<UINT>(rect.right), static_cast<UINT>(rect.bottom), 1 };
                            m_context->UpdateSubresource(backBuffer.Get(), 0, &box,
                                pixels + static_cast<size_t>(rect.top) * stride + static_cast<size_t>(rect.left) * 4, stride, 0);
                            m_dirtyRects[index] = { rect.left, rect.top, rect.right, rect.bottom };
                            copied += rect.Area() * 4;
                        }
                        parameters.DirtyRectsCount = static_cast<UINT>(m_dirtyRects.size());
                        parameters.pDirtyRects = m_dirtyRects.data();
                    }
                    backBuffer.Reset();
                    m_uploadBytes = copied;
                    IFC_RETURN(m_swapChain->Present1(0, 0, &parameters));
                    return S_OK;
                }

                Gdiplus::Graphics& Current() noexcept
                {
                    const size_t depth = m_layerStack.size();
                    if (depth > 0 && m_layerStack.back().active)
                    {
                        return *m_layers[depth - 1]->graphics;
                    }
                    // An inactive layer (allocation failed) draws nothing; CanDraw is false once m_frameHr has failed.
                    return *m_target.graphics;
                }

                PixelRect FullTarget() const noexcept
                {
                    return { 0, 0, static_cast<int32_t>(m_width), static_cast<int32_t>(m_height) };
                }

                PixelRect CurrentClip() const noexcept
                {
                    return m_clipStack.empty() ? FullTarget() : m_clipStack.back();
                }

                // Pixels of bounds inside (damage intersected with the current clip).
                uint64_t VisibleArea(const PixelRect& bounds) const noexcept
                {
                    const PixelRect clipped = Intersect(bounds, CurrentClip());
                    uint64_t area = 0;
                    for (const PixelRect& rect : m_damage)
                    {
                        area += Intersect(rect, clipped).Area();
                    }
                    return area;
                }

                void UpdateDeviceTransform() noexcept
                {
                    const Matrix3x2F& t = m_userTransform;
                    const float s = m_dpiScale;
                    m_transform->SetElements(t.m11 * s, t.m12 * s, t.m21 * s, t.m22 * s, t.dx * s, t.dy * s);
                }

                PixelRect TransformBounds(const RectF& rect, float inflate, bool snapToNearest) const noexcept
                {
                    const Matrix3x2F& t = m_userTransform;
                    const float s = m_dpiScale;
                    const float xs[] = { rect.left - inflate, rect.right + inflate };
                    const float ys[] = { rect.top - inflate, rect.bottom + inflate };
                    float minX = INFINITY, minY = INFINITY, maxX = -INFINITY, maxY = -INFINITY;
                    for (float x : xs)
                    {
                        for (float y : ys)
                        {
                            const float dx = (x * t.m11 + y * t.m21 + t.dx) * s;
                            const float dy = (x * t.m12 + y * t.m22 + t.dy) * s;
                            minX = std::min(minX, dx);
                            maxX = std::max(maxX, dx);
                            minY = std::min(minY, dy);
                            maxY = std::max(maxY, dy);
                        }
                    }
                    if (!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(maxX) || !std::isfinite(maxY))
                    {
                        return {};
                    }
                    if (snapToNearest)
                    {
                        return { ToPixel(std::floor(minX + 0.5f)), ToPixel(std::floor(minY + 0.5f)), ToPixel(std::floor(maxX + 0.5f)), ToPixel(std::floor(maxY + 0.5f)) };
                    }
                    return { ToPixel(std::floor(minX)), ToPixel(std::floor(minY)), ToPixel(std::ceil(maxX)), ToPixel(std::ceil(maxY)) };
                }

                PixelRect DeviceBounds(const RectF& rect, float inflate) const noexcept
                {
                    return TransformBounds(rect, inflate, false);
                }

                // Clip = union over damage rects of (damage rect intersected with the current axis-aligned clip), set in
                // device space; then the DIP-to-pixel transform is restored.
                void ApplyState(Gdiplus::Graphics& graphics) noexcept
                {
                    Record(graphics.ResetTransform());
                    const PixelRect clip = CurrentClip();
                    bool any = false;
                    for (const PixelRect& damage : m_damage)
                    {
                        const PixelRect visible = Intersect(damage, clip);
                        if (visible.IsEmpty())
                        {
                            continue;
                        }
                        Record(graphics.SetClip(Gdiplus::Rect(visible.left, visible.top, visible.right - visible.left, visible.bottom - visible.top),
                            any ? Gdiplus::CombineModeUnion : Gdiplus::CombineModeReplace));
                        any = true;
                    }
                    m_clipIsEmpty = !any;
                    if (!any)
                    {
                        // Nothing is visible; drawing is skipped (CanDraw), so a rejected empty clip is not a failure.
                        graphics.SetClip(Gdiplus::Rect(0, 0, 0, 0), Gdiplus::CombineModeReplace);
                    }
                    Record(graphics.SetTransform(m_transform.get()));
                }

                bool CanDraw() const noexcept
                {
                    return m_inFrame && SUCCEEDED(m_frameHr) && !m_clipIsEmpty;
                }

                bool BeginPrimitive() noexcept
                {
                    if (!m_inFrame)
                    {
                        return false;
                    }
                    ++m_stats.primitiveCount;
                    return CanDraw();
                }

                void Fail(HRESULT hr) noexcept
                {
                    if (FAILED(hr) && SUCCEEDED(m_frameHr))
                    {
                        m_frameHr = hr;
                    }
                }

                void Record(Gdiplus::Status status) noexcept
                {
                    if (status != Gdiplus::Ok)
                    {
                        Fail(HResultFromStatus(status));
                    }
                }

                // Returns false when the radii are zero (callers draw a plain rectangle) or the rect is empty.
                bool BuildRoundedPath(const RectF& rect, float radiusX, float radiusY) noexcept
                {
                    const float width = rect.right - rect.left;
                    const float height = rect.bottom - rect.top;
                    if (!(radiusX > 0) || !(radiusY > 0) || !(width > 0) || !(height > 0))
                    {
                        return false;
                    }
                    const float dx = 2 * std::min(radiusX, width / 2);
                    const float dy = 2 * std::min(radiusY, height / 2);
                    Gdiplus::GraphicsPath& path = *m_path;
                    Record(path.Reset());
                    Record(path.AddArc(rect.left, rect.top, dx, dy, 180, 90));
                    Record(path.AddArc(rect.right - dx, rect.top, dx, dy, 270, 90));
                    Record(path.AddArc(rect.right - dx, rect.bottom - dy, dx, dy, 0, 90));
                    Record(path.AddArc(rect.left, rect.bottom - dy, dx, dy, 90, 90));
                    Record(path.CloseFigure());
                    return true;
                }

                template <typename Map>
                void EvictIfFull(Map& map, size_t limit) noexcept
                {
                    if (map.size() >= limit)
                    {
                        m_frameDestroyed += static_cast<uint32_t>(map.size());
                        map.clear();
                    }
                }

                Gdiplus::SolidBrush* GetBrush(const ColorF& color) noexcept
                {
                    const Gdiplus::Color gdiColor = ToGdiColor(color);
                    const uint32_t key = gdiColor.GetValue();
                    const auto found = m_brushes.find(key);
                    if (found != m_brushes.end())
                    {
                        return found->second.get();
                    }
                    EvictIfFull(m_brushes, c_maxBrushes);
                    std::unique_ptr<Gdiplus::SolidBrush> brush(new Gdiplus::SolidBrush(gdiColor));
                    return Insert(m_brushes, key, std::move(brush));
                }

                Gdiplus::Pen* GetPen(const ColorF& color, float thickness) noexcept
                {
                    const Gdiplus::Color gdiColor = ToGdiColor(color);
                    uint32_t thicknessBits = 0;
                    static_assert(sizeof(thicknessBits) == sizeof(thickness));
                    memcpy(&thicknessBits, &thickness, sizeof(thickness));
                    const uint64_t key = (static_cast<uint64_t>(gdiColor.GetValue()) << 32) | thicknessBits;
                    const auto found = m_pens.find(key);
                    if (found != m_pens.end())
                    {
                        return found->second.get();
                    }
                    EvictIfFull(m_pens, c_maxPens);
                    std::unique_ptr<Gdiplus::Pen> pen(new Gdiplus::Pen(gdiColor, thickness));
                    return Insert(m_pens, key, std::move(pen));
                }

                Gdiplus::FontFamily* GetFontFamily(const wchar_t* name) noexcept
                {
                    const auto found = m_fontFamilies.find(name);
                    if (found != m_fontFamilies.end())
                    {
                        return found->second.get();
                    }
                    std::unique_ptr<Gdiplus::FontFamily> family(new Gdiplus::FontFamily(name));
                    if (FAILED(CheckCreated(family)))
                    {
                        // Font not installed: not a frame failure; callers fall back to another family.
                        return nullptr;
                    }
                    ++m_frameCreated;
                    return m_fontFamilies.emplace(name, std::move(family)).first->second.get();
                }

                Gdiplus::Font* GetFont(const TextStyle& style) noexcept
                {
                    FontKey key{};
                    key.family = (style.fontFamily && *style.fontFamily) ? style.fontFamily : L"Segoe UI";
                    key.size = style.fontSize;
                    key.style = style.fontWeight >= 600 ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular;
                    const auto found = m_fonts.find(key);
                    if (found != m_fonts.end())
                    {
                        return found->second.get();
                    }
                    Gdiplus::FontFamily* family = GetFontFamily(key.family.c_str());
                    if (!family)
                    {
                        family = GetFontFamily(L"Segoe UI");
                    }
                    if (!family)
                    {
                        family = GetFontFamily(L"Arial");
                    }
                    if (!family)
                    {
                        return nullptr;
                    }
                    EvictIfFull(m_fonts, c_maxFonts);
                    // UnitPixel: the size is in world units (DIPs), so the DPI scale in the world transform applies.
                    std::unique_ptr<Gdiplus::Font> font(new Gdiplus::Font(family, key.size, key.style, Gdiplus::UnitPixel));
                    const HRESULT hr = CheckCreated(font);
                    if (FAILED(hr))
                    {
                        Fail(hr);
                        return nullptr;
                    }
                    ++m_frameCreated;
                    return m_fonts.emplace(std::move(key), std::move(font)).first->second.get();
                }

                Gdiplus::StringFormat* GetStringFormat() noexcept
                {
                    if (!m_stringFormat)
                    {
                        // Typographic (no GDI+ 1/6 em padding, closer to DirectWrite layout), wrapping, unclipped like
                        // D2D DrawText, and partial last lines are drawn.
                        std::unique_ptr<Gdiplus::StringFormat> format(new Gdiplus::StringFormat(Gdiplus::StringFormat::GenericTypographic()));
                        HRESULT hr = CheckCreated(format);
                        if (SUCCEEDED(hr))
                        {
                            const INT flags = (format->GetFormatFlags() | Gdiplus::StringFormatFlagsNoClip) & ~Gdiplus::StringFormatFlagsLineLimit;
                            hr = HResultFromStatus(format->SetFormatFlags(flags));
                        }
                        if (FAILED(hr))
                        {
                            Fail(hr);
                            return nullptr;
                        }
                        ++m_frameCreated;
                        m_stringFormat = std::move(format);
                    }
                    return m_stringFormat.get();
                }

                Gdiplus::ImageAttributes* GetImageAttributes() noexcept
                {
                    if (!m_imageAttributes)
                    {
                        std::unique_ptr<Gdiplus::ImageAttributes> attributes(new Gdiplus::ImageAttributes());
                        const HRESULT hr = CheckCreated(attributes);
                        if (FAILED(hr))
                        {
                            Fail(hr);
                            return nullptr;
                        }
                        ++m_frameCreated;
                        m_imageAttributes = std::move(attributes);
                    }
                    return m_imageAttributes.get();
                }

                template <typename Map, typename Key, typename T>
                T* Insert(Map& map, const Key& key, std::unique_ptr<T> object) noexcept
                {
                    const HRESULT hr = CheckCreated(object);
                    if (FAILED(hr))
                    {
                        Fail(hr);
                        return nullptr;
                    }
                    T* raw = object.get();
                    map.emplace(key, std::move(object));
                    ++m_frameCreated;
                    return raw;
                }

                // Declared first so it is destroyed last: every GDI+ object below must be gone before GdiplusShutdown.
                RuntimeReference m_runtime;

                uint32_t m_width = 0;
                uint32_t m_height = 0;
                float m_dpiScale = 1;

                Surface m_target;
                std::vector<std::unique_ptr<Surface>> m_layers; // pooled by nesting depth
                std::unique_ptr<Gdiplus::Matrix> m_transform;   // user transform * DPI scale
                std::unique_ptr<Gdiplus::GraphicsPath> m_path;  // rebuilt per rounded rect; never reallocated
                std::unique_ptr<Gdiplus::StringFormat> m_stringFormat;
                std::unique_ptr<Gdiplus::ImageAttributes> m_imageAttributes;
                std::unordered_map<uint32_t, std::unique_ptr<Gdiplus::SolidBrush>> m_brushes;
                std::unordered_map<uint64_t, std::unique_ptr<Gdiplus::Pen>> m_pens;
                std::map<std::wstring, std::unique_ptr<Gdiplus::FontFamily>> m_fontFamilies;
                std::map<FontKey, std::unique_ptr<Gdiplus::Font>> m_fonts; // after m_fontFamilies: fonts are destroyed first

                Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
                Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
                std::vector<RECT> m_dirtyRects;

                // Frame state.
                bool m_inFrame = false;
                bool m_clipIsEmpty = false;
                bool m_forceFullFrame = true;
                bool m_presentFull = false;
                HRESULT m_frameHr = S_OK;
                Matrix3x2F m_userTransform{};
                std::vector<PixelRect> m_damage;
                PixelRect m_damageBounds{};
                std::vector<PixelRect> m_clipStack;
                std::vector<LayerRecord> m_layerStack;
                FrameStats m_stats{};
                uint64_t m_uploadBytes = 0;
                uint32_t m_frameCreated = 0;
                uint32_t m_frameDestroyed = 0;
                ResourceChurn m_lastChurn{};
            };
        }

        HRESULT GetLastFrameResourceChurn(const IImmediatePainter& painter, ResourceChurn* churn) noexcept
        {
            *churn = {};
            if (painter.GetDeviceIdentity().mode != Mode::ImmediateGdiPlus)
            {
                return E_INVALIDARG;
            }
            *churn = static_cast<const GdiPlusPainter&>(painter).LastFrameChurn();
            return S_OK;
        }
    }

    HRESULT CreateGdiPlusPainter(const PainterOptions& options, std::unique_ptr<IImmediatePainter>& painter) noexcept
    {
        painter.reset();
        if (options.width == 0 || options.height == 0 || !(options.dpiScale > 0))
        {
            return E_INVALIDARG;
        }
        std::unique_ptr<GdiPlus::GdiPlusPainter> gdiPlusPainter(new (std::nothrow) GdiPlus::GdiPlusPainter());
        if (!gdiPlusPainter)
        {
            return E_OUTOFMEMORY;
        }
        IFC_RETURN(gdiPlusPainter->Initialize(options));
        painter = std::move(gdiPlusPainter);
        return S_OK;
    }
}
