// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>
#include <GdiPlusPainter.h>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <list>
#include <map>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dwrite.h>
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
// - Extended primitives:
//   - Rounded clips share one stack with axis-aligned clips and opacity layers (pops that do not match the top entry are
//     ignored). GDI+ region clips are aliased, so a rounded clip draws its children into a pooled layer (clipped to the
//     clip's device bounds), multiplies the layer by an antialiased coverage mask (the rounded rect filled into a pooled
//     mask surface), and blends it once on pop. Square, pixel-aligned clips under an axis-aligned transform skip the
//     layer and behave like PushAxisAlignedClip.
//   - Paths are realized as GraphicsPath (alternate or winding fill). Paths with a cacheKey are kept in a bounded LRU;
//     uncached paths reuse one scratch path. Strokes use the cached pens (flat caps, miter joins).
//   - Glyph runs stay GDI+ text: the IDWriteFontFace is mapped once to its GDI family (IDWriteGdiInterop::
//     ConvertFontFaceToLOGFONT), a Gdiplus::Font is cached per (face, em size) in a bounded LRU, and the glyph indices
//     are drawn with Graphics::DrawDriverString at explicit per-glyph origins (no cmap lookup), computed from DirectWrite
//     advances and offsets (right-to-left runs advance leftwards from the origin).
//   - Bitmaps are painter-owned PARGB copies keyed by (key, generation), drawn with bilinear filtering, edge clamping
//     (WrapModeTileFlipXY), and an alpha color matrix for opacity. Copies count in uploadBytesThisFrame; storage counts
//     in cacheBytes (width * height * 4 per bitmap).
//   - ScrollPixels memmoves rows of the PARGB target. The moved area is added (without double counting overlap with the
//     damage) to the next frame's back-buffer copy and Present1 dirty rects.
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

            PixelRect Offset(const PixelRect& rect, int32_t dx, int32_t dy) noexcept
            {
                return { rect.left + dx, rect.top + dy, rect.right + dx, rect.bottom + dy };
            }

            // Appends rect minus every rect already in list, so the list stays disjoint and its areas can be summed.
            void AddDisjoint(std::vector<PixelRect>& list, const PixelRect& rect)
            {
                if (rect.IsEmpty())
                {
                    return;
                }
                std::vector<PixelRect> pieces{ rect };
                std::vector<PixelRect> remaining;
                for (const PixelRect& existing : list)
                {
                    remaining.clear();
                    for (const PixelRect& piece : pieces)
                    {
                        const PixelRect overlap = Intersect(piece, existing);
                        if (overlap.IsEmpty())
                        {
                            remaining.push_back(piece);
                            continue;
                        }
                        const PixelRect parts[] =
                        {
                            { piece.left, piece.top, piece.right, overlap.top },
                            { piece.left, overlap.bottom, piece.right, piece.bottom },
                            { piece.left, overlap.top, overlap.left, overlap.bottom },
                            { overlap.right, overlap.top, piece.right, overlap.bottom },
                        };
                        for (const PixelRect& part : parts)
                        {
                            if (!part.IsEmpty())
                            {
                                remaining.push_back(part);
                            }
                        }
                    }
                    pieces.swap(remaining);
                }
                list.insert(list.end(), pieces.begin(), pieces.end());
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
            constexpr size_t c_maxCachedPaths = 256;
            constexpr uint64_t c_pathPointBytes = sizeof(Gdiplus::PointF) + 1; // point + type byte
            constexpr size_t c_maxFontFaces = 64;
            constexpr size_t c_maxGlyphFonts = 64;
            constexpr uint64_t c_fontFaceBytes = sizeof(LOGFONTW) + 64;
            constexpr uint64_t c_dwriteInteropBytes = 256;

            // One stack for clips and layers: pops must nest.
            enum class StackEntry : uint8_t
            {
                AxisAlignedClip,
                OpacityLayer,
                RoundedClip,          // layer + antialiased mask
                RoundedClipAsAxisClip // square and pixel-aligned: no layer needed
            };

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
                // Rounded clip mask: the clip in DIPs under the transform current at push.
                bool mask = false;
                RectF maskRect{};
                float radiusX = 0, radiusY = 0;
                Matrix3x2F maskTransform{};
            };

            struct CachedPath
            {
                std::unique_ptr<Gdiplus::GraphicsPath> path;
                RectF bounds{};
                uint64_t bytes = 0;
                std::list<uint64_t>::iterator lru;
            };

            struct FontFaceEntry
            {
                Microsoft::WRL::ComPtr<IDWriteFontFace> face; // keeps the pointer key from being reused
                Gdiplus::FontFamily* family = nullptr;        // owned by m_fontFamilies; nullptr if GDI+ cannot use it
                INT style = Gdiplus::FontStyleRegular;
            };

            using GlyphFontKey = std::pair<IDWriteFontFace*, uint32_t>; // face, em size bits

            struct GlyphFontEntry
            {
                std::unique_ptr<Gdiplus::Font> font;
                std::list<GlyphFontKey>::iterator lru;
            };

            struct BitmapEntry
            {
                uint64_t generation = 0;
                uint32_t width = 0, height = 0;
                std::vector<uint8_t> pixels;
                std::unique_ptr<Gdiplus::Bitmap> bitmap;
            };

            uint32_t FloatBits(float value) noexcept
            {
                uint32_t bits = 0;
                static_assert(sizeof(bits) == sizeof(value));
                memcpy(&bits, &value, sizeof(value));
                return bits;
            }

            // Graphics before its bitmap, bitmap before the buffer it wraps.
            void ReleaseSurface(Surface& surface) noexcept
            {
                surface.graphics.reset();
                surface.bitmap.reset();
                surface.pixels.clear();
                surface.pixels.shrink_to_fit();
            }

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
                    if (m_mask.bitmap)
                    {
                        m_frameDestroyed += 2;
                    }
                    ReleaseSurface(m_mask);
                    m_scrolledAreas.clear();
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
                    m_uploadBytes = m_pendingUploadBytes;
                    m_pendingUploadBytes = 0;
                    m_frameCreated = 0;
                    m_frameDestroyed = 0;
                    m_presentFull = m_forceFullFrame;
                    m_forceFullFrame = false;

                    const PixelRect full{ 0, 0, static_cast<int32_t>(m_width), static_cast<int32_t>(m_height) };
                    m_damage.clear();
                    m_clipStack.clear();
                    m_layerStack.clear();
                    m_stack.clear();
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
                    m_stack.push_back(StackEntry::AxisAlignedClip);
                    ApplyState(Current());
                }

                void PopAxisAlignedClip() noexcept override
                {
                    if (!m_inFrame || m_stack.empty() || m_stack.back() != StackEntry::AxisAlignedClip)
                    {
                        return;
                    }
                    m_stack.pop_back();
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
                    m_stack.push_back(StackEntry::OpacityLayer);
                    PushLayer(record);
                }

                void PopOpacityLayer() noexcept override
                {
                    if (!m_inFrame || m_stack.empty() || m_stack.back() != StackEntry::OpacityLayer)
                    {
                        return;
                    }
                    m_stack.pop_back();
                    PopLayer();
                }

                void PushRoundedRectangleClip(const RectF& clip, float radiusX, float radiusY) noexcept override
                {
                    if (!m_inFrame)
                    {
                        return;
                    }
                    const PixelRect bounds = TransformBounds(clip, 0, false);
                    m_clipStack.push_back(Intersect(CurrentClip(), bounds));
                    if (!(radiusX > 0 && radiusY > 0) && IsPixelAlignedRect(clip))
                    {
                        m_stack.push_back(StackEntry::RoundedClipAsAxisClip);
                        ApplyState(Current());
                        return;
                    }
                    m_stack.push_back(StackEntry::RoundedClip);
                    LayerRecord record{};
                    record.mask = true;
                    record.maskRect = clip;
                    record.radiusX = radiusX;
                    record.radiusY = radiusY;
                    record.maskTransform = m_userTransform;
                    PushLayer(record);
                }

                void PopRoundedRectangleClip() noexcept override
                {
                    if (!m_inFrame || m_stack.empty())
                    {
                        return;
                    }
                    const StackEntry entry = m_stack.back();
                    if (entry != StackEntry::RoundedClip && entry != StackEntry::RoundedClipAsAxisClip)
                    {
                        return;
                    }
                    m_stack.pop_back();
                    m_clipStack.pop_back();
                    if (entry == StackEntry::RoundedClip)
                    {
                        PopLayer();
                    }
                    else
                    {
                        ApplyState(Current());
                    }
                }

                void FillPath(const PathData& data, const ColorF& color) noexcept override
                {
                    if (!BeginPrimitive())
                    {
                        return;
                    }
                    RectF bounds{};
                    Gdiplus::GraphicsPath* path = GetPath(data, bounds);
                    Gdiplus::SolidBrush* brush = path ? GetBrush(color) : nullptr;
                    if (!brush)
                    {
                        return;
                    }
                    Record(Current().FillPath(brush, path));
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(bounds, 0));
                }

                void StrokePath(const PathData& data, float thickness, const ColorF& color) noexcept override
                {
                    if (!BeginPrimitive() || !(thickness > 0))
                    {
                        return;
                    }
                    RectF bounds{};
                    Gdiplus::GraphicsPath* path = GetPath(data, bounds);
                    Gdiplus::Pen* pen = path ? GetPen(color, thickness) : nullptr;
                    if (!pen)
                    {
                        return;
                    }
                    Record(Current().DrawPath(pen, path));
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(bounds, thickness / 2));
                }

                void DrawGlyphRun(const GlyphRunDesc& run, const ColorF& color) noexcept override
                {
                    if (run.fontFace == nullptr || run.glyphIndices == nullptr || run.glyphAdvances == nullptr || run.glyphCount == 0 ||
                        !(run.fontEmSize > 0) || !std::isfinite(run.fontEmSize))
                    {
                        return;
                    }
                    if (!BeginPrimitive())
                    {
                        return;
                    }
                    Gdiplus::Font* font = GetGlyphFont(run.fontFace, run.fontEmSize);
                    Gdiplus::SolidBrush* brush = font ? GetBrush(color) : nullptr;
                    if (!brush)
                    {
                        return;
                    }

                    // Explicit glyph origins from DirectWrite's advances and offsets. Right-to-left runs advance leftwards
                    // from the origin, and a positive advanceOffset moves a glyph in the run's direction (left for RTL),
                    // matching DWRITE_GLYPH_OFFSET.
                    const uint32_t count = std::min<uint32_t>(run.glyphCount, INT_MAX);
                    const bool rightToLeft = (run.bidiLevel & 1) != 0;
                    m_glyphPositions.resize(count);
                    float pen = 0;
                    float minX = INFINITY, maxX = -INFINITY, minY = INFINITY, maxY = -INFINITY;
                    for (uint32_t index = 0; index < count; ++index)
                    {
                        const float advance = run.glyphAdvances[index];
                        const GlyphOffset offset = run.glyphOffsets ? run.glyphOffsets[index] : GlyphOffset{};
                        float x;
                        if (rightToLeft)
                        {
                            pen += advance;
                            x = run.baselineOrigin.x - pen - offset.advanceOffset;
                        }
                        else
                        {
                            x = run.baselineOrigin.x + pen + offset.advanceOffset;
                            pen += advance;
                        }
                        const float y = run.baselineOrigin.y - offset.ascenderOffset;
                        m_glyphPositions[index] = Gdiplus::PointF(x, y);
                        minX = std::min({ minX, x, x + advance });
                        maxX = std::max({ maxX, x, x + advance });
                        minY = std::min(minY, y);
                        maxY = std::max(maxY, y);
                    }
                    Record(Current().DrawDriverString(run.glyphIndices, static_cast<INT>(count), font, brush, m_glyphPositions.data(), 0, nullptr));

                    // Ink estimate: ascent above and descent below the baselines, plus some side bearing overhang.
                    const float em = run.fontEmSize;
                    const RectF ink{ minX - em * 0.25f, minY - em * 1.25f, maxX + em * 0.25f, maxY + em * 0.5f };
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(ink, 0));
                }

                HRESULT EnsureBitmap(const BitmapSource& source) noexcept override
                {
                    if (source.pixels == nullptr || source.width == 0 || source.height == 0 || source.width > c_maxDimension ||
                        source.height > c_maxDimension || source.stride < source.width * 4)
                    {
                        return E_INVALIDARG;
                    }
                    const auto found = m_bitmaps.find(source.key);
                    if (found != m_bitmaps.end() && found->second->generation == source.generation)
                    {
                        return S_OK;
                    }

                    BitmapEntry* entry = nullptr;
                    if (found != m_bitmaps.end() && found->second->width == source.width && found->second->height == source.height)
                    {
                        // Same size: refresh the pixels in place; the Gdiplus::Bitmap wraps this buffer.
                        entry = found->second.get();
                    }
                    else
                    {
                        auto created = std::unique_ptr<BitmapEntry>(new (std::nothrow) BitmapEntry());
                        if (!created)
                        {
                            return E_OUTOFMEMORY;
                        }
                        created->pixels.resize(static_cast<size_t>(source.width) * source.height * 4);
                        created->width = source.width;
                        created->height = source.height;
                        created->bitmap.reset(new Gdiplus::Bitmap(static_cast<INT>(source.width), static_cast<INT>(source.height),
                            static_cast<INT>(source.width * 4), PixelFormat32bppPARGB, created->pixels.data()));
                        IFC_RETURN(CheckCreated(created->bitmap));
                        if (found != m_bitmaps.end())
                        {
                            found->second = std::move(created);
                            entry = found->second.get();
                            ++m_frameDestroyed;
                        }
                        else
                        {
                            entry = m_bitmaps.emplace(source.key, std::move(created)).first->second.get();
                        }
                        ++m_frameCreated;
                    }

                    const size_t rowBytes = static_cast<size_t>(source.width) * 4;
                    for (uint32_t y = 0; y < source.height; ++y)
                    {
                        memcpy(entry->pixels.data() + y * rowBytes, source.pixels + static_cast<size_t>(y) * source.stride, rowBytes);
                    }
                    entry->generation = source.generation;
                    (m_inFrame ? m_uploadBytes : m_pendingUploadBytes) += static_cast<uint64_t>(rowBytes) * source.height;
                    return S_OK;
                }

                void DrawBitmap(uint64_t key, const RectF& destination, const RectF* sourceRect, float opacity) noexcept override
                {
                    const auto found = m_bitmaps.find(key);
                    if (found == m_bitmaps.end() || !BeginPrimitive() || !(opacity > 0))
                    {
                        return;
                    }
                    const BitmapEntry& entry = *found->second;
                    const RectF source = sourceRect ? *sourceRect : RectF{ 0, 0, static_cast<float>(entry.width), static_cast<float>(entry.height) };
                    Gdiplus::ImageAttributes* attributes = GetBitmapAttributes();
                    if (!attributes)
                    {
                        return;
                    }
                    if (opacity < 1)
                    {
                        Gdiplus::ColorMatrix matrix =
                        { {
                            { 1, 0, 0, 0, 0 },
                            { 0, 1, 0, 0, 0 },
                            { 0, 0, 1, 0, 0 },
                            { 0, 0, 0, opacity, 0 },
                            { 0, 0, 0, 0, 1 },
                        } };
                        Record(attributes->SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap));
                    }
                    else
                    {
                        Record(attributes->ClearColorMatrix(Gdiplus::ColorAdjustTypeBitmap));
                    }
                    Gdiplus::Graphics& graphics = Current();
                    Record(graphics.SetInterpolationMode(Gdiplus::InterpolationModeBilinear));
                    Record(graphics.DrawImage(entry.bitmap.get(), ToGdiRect(destination), source.left, source.top, source.right - source.left,
                        source.bottom - source.top, Gdiplus::UnitPixel, attributes));
                    Record(graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor));
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(destination, 0));
                }

                void ReleaseBitmap(uint64_t key) noexcept override
                {
                    if (m_bitmaps.erase(key) > 0)
                    {
                        ++m_frameDestroyed;
                    }
                }

                HRESULT ScrollPixels(const RectF& rect, int32_t dx, int32_t dy) noexcept override
                {
                    if (m_inFrame)
                    {
                        return E_ILLEGAL_METHOD_CALL;
                    }
                    if (!std::isfinite(rect.left) || !std::isfinite(rect.top) || !std::isfinite(rect.right) || !std::isfinite(rect.bottom))
                    {
                        return E_INVALIDARG;
                    }
                    const PixelRect area = Intersect(FullTarget(), { ToPixel(std::floor(rect.left + 0.5f)), ToPixel(std::floor(rect.top + 0.5f)),
                        ToPixel(std::floor(rect.right + 0.5f)), ToPixel(std::floor(rect.bottom + 0.5f)) });
                    const int32_t limit = static_cast<int32_t>(c_maxDimension);
                    if (area.IsEmpty() || (dx == 0 && dy == 0) || dx <= -limit || dx >= limit || dy <= -limit || dy >= limit)
                    {
                        return S_OK;
                    }
                    const PixelRect source = Intersect(area, Offset(area, -dx, -dy));
                    if (source.IsEmpty())
                    {
                        return S_OK;
                    }
                    const PixelRect destination = Offset(source, dx, dy);
                    m_target.graphics->Flush(Gdiplus::FlushIntentionSync);

                    const size_t stride = static_cast<size_t>(m_width) * 4;
                    const size_t rowBytes = static_cast<size_t>(source.right - source.left) * 4;
                    uint8_t* pixels = m_target.pixels.data();
                    const auto moveRow = [&](int32_t y)
                    {
                        memmove(pixels + static_cast<size_t>(y + dy) * stride + static_cast<size_t>(destination.left) * 4,
                            pixels + static_cast<size_t>(y) * stride + static_cast<size_t>(source.left) * 4, rowBytes);
                    };
                    if (dy > 0)
                    {
                        for (int32_t y = source.bottom - 1; y >= source.top; --y)
                        {
                            moveRow(y);
                        }
                    }
                    else
                    {
                        for (int32_t y = source.top; y < source.bottom; ++y)
                        {
                            moveRow(y);
                        }
                    }
                    AddDisjoint(m_scrolledAreas, destination);
                    return S_OK;
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
                    while (!m_stack.empty())
                    {
                        switch (m_stack.back())
                        {
                            case StackEntry::AxisAlignedClip: PopAxisAlignedClip(); break;
                            case StackEntry::OpacityLayer: PopOpacityLayer(); break;
                            default: PopRoundedRectangleClip(); break;
                        }
                    }
                    m_clipStack.clear();
                    m_layerStack.clear();
                    m_target.graphics->Flush(Gdiplus::FlushIntentionSync);
                    m_inFrame = false;

                    HRESULT hr = m_frameHr;
                    if (SUCCEEDED(hr) && m_swapChain)
                    {
                        hr = Present();
                    }
                    m_scrolledAreas.clear();
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
                    inventory.surfaceBytes = surfaceBytes * (1 + m_layers.size() + (m_mask.bitmap ? 1 : 0));
                    const uint32_t families = static_cast<uint32_t>(m_fontFamilies.size());
                    const uint32_t singletons = (m_stringFormat ? 1 : 0) + (m_imageAttributes ? 1 : 0) + (m_path ? 1 : 0) + (m_transform ? 1 : 0) +
                        (m_bitmapAttributes ? 1 : 0) + (m_gdiInterop ? 1 : 0);
                    inventory.cachedObjectCount = static_cast<uint32_t>(m_brushes.size() + m_pens.size() + m_fonts.size() + m_paths.size() +
                        m_fontFaces.size() + m_glyphFonts.size() + m_bitmaps.size()) + families + singletons;
                    uint64_t bitmapBytes = 0;
                    for (const auto& bitmap : m_bitmaps)
                    {
                        bitmapBytes += static_cast<uint64_t>(bitmap.second->width) * bitmap.second->height * 4;
                    }
                    inventory.cacheBytes = m_brushes.size() * c_brushBytes + m_pens.size() * c_penBytes + m_fonts.size() * c_fontBytes +
                        families * c_fontFamilyBytes + (m_stringFormat ? c_stringFormatBytes : 0) + (m_imageAttributes ? c_imageAttributesBytes : 0) +
                        (m_path ? c_pathBytes : 0) + (m_transform ? c_matrixBytes : 0) + m_pathCacheBytes + m_fontFaces.size() * c_fontFaceBytes +
                        m_glyphFonts.size() * c_fontBytes + bitmapBytes + (m_bitmapAttributes ? c_imageAttributesBytes : 0) +
                        (m_gdiInterop ? c_dwriteInteropBytes : 0);
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
                    if (!m_presentFull)
                    {
                        // Damage rects are disjoint by contract; scrolled areas are added minus their overlap with them.
                        m_presentRects.assign(m_damage.begin(), m_damage.end());
                        for (const PixelRect& scrolled : m_scrolledAreas)
                        {
                            AddDisjoint(m_presentRects, scrolled);
                        }
                        if (m_presentRects.empty())
                        {
                            return S_OK;
                        }
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
                        m_dirtyRects.resize(m_presentRects.size());
                        for (size_t index = 0; index < m_presentRects.size(); ++index)
                        {
                            const PixelRect& rect = m_presentRects[index];
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
                    m_uploadBytes += copied;
                    IFC_RETURN(m_swapChain->Present1(0, 0, &parameters));
                    return S_OK;
                }

                // Children of an opacity group or rounded clip draw into a pooled full-size layer (one per nesting depth),
                // cleared inside its bounds (current clip intersected with the damage bounds).
                void PushLayer(LayerRecord record) noexcept
                {
                    record.bounds = Intersect(CurrentClip(), m_damageBounds);
                    const size_t depth = m_layerStack.size();
                    if (SUCCEEDED(m_frameHr) && depth == m_layers.size())
                    {
                        auto layer = std::unique_ptr<Surface>(new (std::nothrow) Surface());
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
                    else
                    {
                        ApplyState(Current());
                    }
                }

                // Masks (rounded clip) and blends the top layer into its parent once, clipped by the parent's state.
                void PopLayer() noexcept
                {
                    if (m_layerStack.empty())
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
                    if (record.mask && !ApplyRoundedMask(layer, record))
                    {
                        return;
                    }
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

                // Antialiased rounded clip: fills the clip shape (under the transform current at push) into the mask
                // surface and scales every premultiplied layer pixel in the bounds by the mask's coverage.
                bool ApplyRoundedMask(Surface& layer, const LayerRecord& record) noexcept
                {
                    if (!m_mask.bitmap)
                    {
                        const HRESULT hr = CreateSurface(m_width, m_height, m_mask);
                        if (FAILED(hr))
                        {
                            ReleaseSurface(m_mask);
                            Fail(hr);
                            return false;
                        }
                    }
                    Gdiplus::SolidBrush* brush = GetBrush({ 1, 1, 1, 1 });
                    if (!brush)
                    {
                        return false;
                    }
                    const PixelRect& b = record.bounds;
                    Gdiplus::Graphics& mask = *m_mask.graphics;
                    Record(mask.ResetTransform());
                    Record(mask.SetClip(Gdiplus::Rect(b.left, b.top, b.right - b.left, b.bottom - b.top), Gdiplus::CombineModeReplace));
                    Record(mask.Clear(Gdiplus::Color(0, 0, 0, 0)));

                    // Borrow m_transform for the push-time transform, then restore the current one.
                    const Matrix3x2F& t = record.maskTransform;
                    const float s = m_dpiScale;
                    m_transform->SetElements(t.m11 * s, t.m12 * s, t.m21 * s, t.m22 * s, t.dx * s, t.dy * s);
                    Record(mask.SetTransform(m_transform.get()));
                    UpdateDeviceTransform();
                    if (BuildRoundedPath(record.maskRect, record.radiusX, record.radiusY))
                    {
                        Record(mask.FillPath(brush, m_path.get()));
                    }
                    else
                    {
                        Record(mask.FillRectangle(brush, ToGdiRect(record.maskRect)));
                    }
                    mask.Flush(Gdiplus::FlushIntentionSync);
                    if (FAILED(m_frameHr))
                    {
                        return false;
                    }

                    const size_t stride = static_cast<size_t>(m_width) * 4;
                    for (int32_t y = b.top; y < b.bottom; ++y)
                    {
                        uint8_t* destination = layer.pixels.data() + y * stride + static_cast<size_t>(b.left) * 4;
                        const uint8_t* coverage = m_mask.pixels.data() + y * stride + static_cast<size_t>(b.left) * 4;
                        for (int32_t x = b.left; x < b.right; ++x, destination += 4, coverage += 4)
                        {
                            const uint32_t alpha = coverage[3];
                            if (alpha == 255)
                            {
                                continue;
                            }
                            for (int channel = 0; channel < 4; ++channel)
                            {
                                destination[channel] = static_cast<uint8_t>((destination[channel] * alpha + 127) / 255);
                            }
                        }
                    }
                    return true;
                }

                // True when the transform is axis-aligned and rect's device edges are on pixel boundaries, so an
                // aliased clip is exact.
                bool IsPixelAlignedRect(const RectF& rect) const noexcept
                {
                    const Matrix3x2F& t = m_userTransform;
                    if (t.m12 != 0 || t.m21 != 0)
                    {
                        return false;
                    }
                    const float s = m_dpiScale;
                    const float edges[] = { (rect.left * t.m11 + t.dx) * s, (rect.right * t.m11 + t.dx) * s, (rect.top * t.m22 + t.dy) * s, (rect.bottom * t.m22 + t.dy) * s };
                    for (float edge : edges)
                    {
                        if (!std::isfinite(edge) || std::fabs(edge - std::round(edge)) > 1.0f / 256)
                        {
                            return false;
                        }
                    }
                    return true;
                }

                // Builds the GraphicsPath for data; false (draw nothing) for malformed data.
                bool BuildPath(const PathData& data, Gdiplus::GraphicsPath& path, RectF& bounds) noexcept
                {
                    if ((data.verbCount > 0 && data.verbs == nullptr) || (data.pointCount > 0 && data.points == nullptr))
                    {
                        return false;
                    }
                    Record(path.Reset());
                    Record(path.SetFillMode(data.nonZeroFill ? Gdiplus::FillModeWinding : Gdiplus::FillModeAlternate));
                    bounds = { INFINITY, INFINITY, -INFINITY, -INFINITY };
                    uint32_t next = 0;
                    const auto take = [&](Gdiplus::PointF& point) noexcept
                    {
                        if (next >= data.pointCount)
                        {
                            return false;
                        }
                        const PointF& p = data.points[next++];
                        if (!std::isfinite(p.x) || !std::isfinite(p.y))
                        {
                            return false;
                        }
                        bounds = { std::min(bounds.left, p.x), std::min(bounds.top, p.y), std::max(bounds.right, p.x), std::max(bounds.bottom, p.y) };
                        point = Gdiplus::PointF(p.x, p.y);
                        return true;
                    };

                    Gdiplus::PointF current, start, c1, c2, end;
                    bool hasCurrent = false;
                    for (uint32_t index = 0; index < data.verbCount; ++index)
                    {
                        switch (data.verbs[index])
                        {
                            case PathVerb::MoveTo:
                                if (!take(end)) return false;
                                Record(path.StartFigure());
                                current = start = end;
                                hasCurrent = true;
                                break;
                            case PathVerb::LineTo:
                                if (!take(end)) return false;
                                if (!hasCurrent)
                                {
                                    current = start = end;
                                    hasCurrent = true;
                                    break;
                                }
                                Record(path.AddLine(current, end));
                                current = end;
                                break;
                            case PathVerb::CubicTo:
                                if (!take(c1) || !take(c2) || !take(end)) return false;
                                if (!hasCurrent)
                                {
                                    current = start = c1;
                                    hasCurrent = true;
                                }
                                Record(path.AddBezier(current, c1, c2, end));
                                current = end;
                                break;
                            case PathVerb::Close:
                                if (hasCurrent)
                                {
                                    Record(path.CloseFigure());
                                    current = start;
                                }
                                break;
                            default:
                                return false;
                        }
                    }
                    if (!(bounds.right >= bounds.left))
                    {
                        bounds = {};
                    }
                    return SUCCEEDED(m_frameHr);
                }

                // Paths with a cacheKey are realized once and kept in a bounded LRU; others reuse the scratch path.
                Gdiplus::GraphicsPath* GetPath(const PathData& data, RectF& bounds) noexcept
                {
                    if (data.cacheKey == 0)
                    {
                        return BuildPath(data, *m_path, bounds) ? m_path.get() : nullptr;
                    }
                    const auto found = m_paths.find(data.cacheKey);
                    if (found != m_paths.end())
                    {
                        m_pathLru.splice(m_pathLru.begin(), m_pathLru, found->second.lru);
                        bounds = found->second.bounds;
                        return found->second.path.get();
                    }

                    std::unique_ptr<Gdiplus::GraphicsPath> path(new Gdiplus::GraphicsPath());
                    const HRESULT hr = CheckCreated(path);
                    if (FAILED(hr))
                    {
                        Fail(hr);
                        return nullptr;
                    }
                    ++m_frameCreated;
                    if (!BuildPath(data, *path, bounds))
                    {
                        ++m_frameDestroyed;
                        return nullptr;
                    }
                    while (m_paths.size() >= c_maxCachedPaths)
                    {
                        const auto victim = m_paths.find(m_pathLru.back());
                        m_pathCacheBytes -= victim->second.bytes;
                        m_paths.erase(victim);
                        m_pathLru.pop_back();
                        ++m_frameDestroyed;
                    }
                    m_pathLru.push_front(data.cacheKey);
                    CachedPath& entry = m_paths[data.cacheKey];
                    entry.path = std::move(path);
                    entry.bounds = bounds;
                    entry.bytes = c_pathBytes + static_cast<uint64_t>(entry.path->GetPointCount()) * c_pathPointBytes;
                    entry.lru = m_pathLru.begin();
                    m_pathCacheBytes += entry.bytes;
                    return entry.path.get();
                }

                // Maps a DirectWrite font face to its GDI family and style once (negative results are cached too).
                const FontFaceEntry* GetFontFace(IDWriteFontFace* face) noexcept
                {
                    const auto found = m_fontFaces.find(face);
                    if (found != m_fontFaces.end())
                    {
                        return &found->second;
                    }
                    if (!m_gdiInterop)
                    {
                        Microsoft::WRL::ComPtr<IDWriteFactory> factory;
                        HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf()));
                        if (SUCCEEDED(hr))
                        {
                            hr = factory->GetGdiInterop(&m_gdiInterop);
                        }
                        if (FAILED(hr))
                        {
                            Fail(hr);
                            return nullptr;
                        }
                        ++m_frameCreated;
                    }
                    if (m_fontFaces.size() >= c_maxFontFaces)
                    {
                        // Glyph fonts are keyed by face: drop them together.
                        m_frameDestroyed += static_cast<uint32_t>(m_fontFaces.size() + m_glyphFonts.size());
                        m_glyphFonts.clear();
                        m_glyphFontLru.clear();
                        m_fontFaces.clear();
                    }

                    FontFaceEntry entry{};
                    entry.face = face;
                    LOGFONTW logFont{};
                    if (SUCCEEDED(m_gdiInterop->ConvertFontFaceToLOGFONT(face, &logFont)))
                    {
                        // The GDI family name already selects weight-specific faces (for example "Segoe UI Semibold");
                        // GDI+ styles only add bold and italic.
                        entry.family = GetFontFamily(logFont.lfFaceName);
                        entry.style = (logFont.lfWeight >= FW_BOLD ? Gdiplus::FontStyleBold : 0) | (logFont.lfItalic ? Gdiplus::FontStyleItalic : 0);
                    }
                    const FontFaceEntry* result = &m_fontFaces.emplace(face, std::move(entry)).first->second;
                    ++m_frameCreated;
                    return result;
                }

                Gdiplus::Font* GetGlyphFont(IDWriteFontFace* face, float emSize) noexcept
                {
                    const GlyphFontKey key{ face, FloatBits(emSize) };
                    const auto found = m_glyphFonts.find(key);
                    if (found != m_glyphFonts.end())
                    {
                        m_glyphFontLru.splice(m_glyphFontLru.begin(), m_glyphFontLru, found->second.lru);
                        return found->second.font.get();
                    }
                    const FontFaceEntry* entry = GetFontFace(face);
                    if (!entry || !entry->family)
                    {
                        // Not a font GDI+ can use (for example CFF outlines or a private collection): nothing is drawn.
                        return nullptr;
                    }
                    // UnitPixel: the em size is in world units (DIPs), so the DPI scale in the world transform applies.
                    std::unique_ptr<Gdiplus::Font> font(new Gdiplus::Font(entry->family, emSize, entry->style, Gdiplus::UnitPixel));
                    if (font && font->GetLastStatus() != Gdiplus::Ok && entry->style != Gdiplus::FontStyleRegular)
                    {
                        font.reset(new Gdiplus::Font(entry->family, emSize, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel));
                    }
                    if (FAILED(CheckCreated(font)))
                    {
                        return nullptr;
                    }
                    while (m_glyphFonts.size() >= c_maxGlyphFonts)
                    {
                        m_glyphFonts.erase(m_glyphFontLru.back());
                        m_glyphFontLru.pop_back();
                        ++m_frameDestroyed;
                    }
                    m_glyphFontLru.push_front(key);
                    GlyphFontEntry& glyphFont = m_glyphFonts[key];
                    glyphFont.font = std::move(font);
                    glyphFont.lru = m_glyphFontLru.begin();
                    ++m_frameCreated;
                    return glyphFont.font.get();
                }

                // Bilinear sampling clamps at the source rect's edges instead of fading to transparent.
                Gdiplus::ImageAttributes* GetBitmapAttributes() noexcept
                {
                    if (!m_bitmapAttributes)
                    {
                        std::unique_ptr<Gdiplus::ImageAttributes> attributes(new Gdiplus::ImageAttributes());
                        HRESULT hr = CheckCreated(attributes);
                        if (SUCCEEDED(hr))
                        {
                            hr = HResultFromStatus(attributes->SetWrapMode(Gdiplus::WrapModeTileFlipXY));
                        }
                        if (FAILED(hr))
                        {
                            Fail(hr);
                            return nullptr;
                        }
                        ++m_frameCreated;
                        m_bitmapAttributes = std::move(attributes);
                    }
                    return m_bitmapAttributes.get();
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

                // Extended primitives.
                Surface m_mask; // rounded-clip coverage, created on first use
                std::unique_ptr<Gdiplus::ImageAttributes> m_bitmapAttributes;
                std::unordered_map<uint64_t, CachedPath> m_paths;
                std::list<uint64_t> m_pathLru; // most recently used first
                uint64_t m_pathCacheBytes = 0;
                Microsoft::WRL::ComPtr<IDWriteGdiInterop> m_gdiInterop;
                std::unordered_map<IDWriteFontFace*, FontFaceEntry> m_fontFaces;
                std::map<GlyphFontKey, GlyphFontEntry> m_glyphFonts; // after m_fontFamilies: fonts are destroyed first
                std::list<GlyphFontKey> m_glyphFontLru;
                std::vector<Gdiplus::PointF> m_glyphPositions;
                std::unordered_map<uint64_t, std::unique_ptr<BitmapEntry>> m_bitmaps;
                std::vector<PixelRect> m_scrolledAreas; // moved by ScrollPixels since the last frame; disjoint
                std::vector<PixelRect> m_presentRects;
                uint64_t m_pendingUploadBytes = 0;      // EnsureBitmap copies made outside a frame

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
                std::vector<StackEntry> m_stack;
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
