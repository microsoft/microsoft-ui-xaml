// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>
#include <GdiPlusPainter.h>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <limits>
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
#include <array>
#include <tuple>
#include <d2d1.h>
#include <dwrite_3.h>
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
// - S11-S23 primitives:
//   - Gradients: Gdiplus::LinearGradientBrush wraps instead of clamping, so the brush axis is extended to cover the
//     shape's projection (quantized to 1/8 of the gradient length so small motion reuses the brush) and padded with the
//     end colors; SetInterpolationColors carries the stops. GDI+ interpolates straight colors, so segments whose alphas
//     differ are subdivided to approximate premultiplied interpolation like D2D. Brushes are cached in a bounded LRU.
//   - Shadows: the offset shape's exact coverage is rasterized into an 8-bit mask (shape + 3 sigma, physical pixels,
//     subpixel phase quantized to 1/8 px) and blurred with three box passes per axis; masks are cached in a bounded LRU
//     keyed by (size, radii, sigma, phase) and count in cacheBytes. Compositing colors the mask, knocks out the shape's
//     own coverage, and draws it with DrawImage (1:1 under axis-aligned transforms, bilinear otherwise).
//   - BlurBackdrop blurs the current surface's pixels in place (three box passes per axis, edge clamp) inside the
//     rounded rect's antialiased coverage, clipped to the current clip and damage.
//   - Glyph runs: color fonts (IDWriteFontFace2::IsColorFont) are split into palette layers with
//     IDWriteFactory2::TranslateColorGlyphRun. Faces GDI+ cannot draw faithfully (non-TrueType outlines, variable fonts,
//     fonts outside the system collection, failed GDI mapping) are drawn from DirectWrite outlines
//     (GetGlyphRunOutline into a GraphicsPath, cached per (face, glyph, em size) in a bounded LRU). clearType runs use
//     TextRenderingHintClearTypeGridFit when every pixel under the run is opaque, grayscale otherwise; outline-drawn
//     glyphs are always grayscale.
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
            constexpr size_t c_maxGradientBrushes = 32;
            constexpr uint64_t c_gradientBrushBytes = 256;
            constexpr uint64_t c_gradientStopBytes = sizeof(Gdiplus::ARGB) + sizeof(Gdiplus::REAL);
            constexpr size_t c_maxShadowMasks = 32;
            constexpr uint64_t c_maxShadowMaskBytes = 4 << 20;
            constexpr uint64_t c_maxShadowPixels = 16 << 20;
            constexpr uint64_t c_releaseScratchBytes = 4 << 20; // larger scratch surfaces are freed at EndFrame
            constexpr float c_maxSigma = 128;                   // physical pixels
            constexpr size_t c_maxGlyphOutlines = 2048;
            constexpr uint64_t c_maxGlyphOutlineBytes = 2 << 20;
            constexpr uint64_t c_glyphOutlineBytes = 128;

            // Gradient fills use 8x8 supersampling (SmoothingModeAntiAlias8x8, GDI+ 1.1): the default 8x4 grid
            // rounds the thin coverage of a curved edge over a gradient to near 0 or 1.
            class FineAntialiasing
            {
            public:
                explicit FineAntialiasing(Gdiplus::Graphics& graphics) noexcept
                    : m_graphics(graphics), m_previous(graphics.GetSmoothingMode())
                {
                    m_graphics.SetSmoothingMode(static_cast<Gdiplus::SmoothingMode>(Gdiplus::SmoothingModeAntiAlias + 1));
                }
                ~FineAntialiasing()
                {
                    m_graphics.SetSmoothingMode(m_previous);
                }
                FineAntialiasing(const FineAntialiasing&) = delete;
                FineAntialiasing& operator=(const FineAntialiasing&) = delete;

            private:
                Gdiplus::Graphics& m_graphics;
                Gdiplus::SmoothingMode m_previous;
            };

            // Radii of three box filters approximating a gaussian of standard deviation sigma.
            void BoxRadii(float sigma, int32_t (&radii)[3]) noexcept
            {
                radii[0] = radii[1] = radii[2] = 0;
                if (!(sigma > 0))
                {
                    return;
                }
                const float variance12 = 12 * sigma * sigma;
                int32_t lower = static_cast<int32_t>(std::floor(std::sqrt(variance12 / 3 + 1)));
                if (lower % 2 == 0)
                {
                    --lower;
                }
                lower = std::max(lower, 1);
                const int32_t upper = lower + 2;
                const float lowerF = static_cast<float>(lower);
                const int32_t lowerCount = static_cast<int32_t>(std::lround((variance12 - 3 * lowerF * lowerF - 12 * lowerF - 9) / (-4 * lowerF - 4)));
                for (int32_t index = 0; index < 3; ++index)
                {
                    radii[index] = ((index < lowerCount ? lower : upper) - 1) / 2;
                }
            }

            // One box-filter pass over `lines` lines of `length` samples (`channels` interleaved bytes each). Samples are
            // sampleStep bytes apart and lines lineStep bytes apart, so the same code runs rows and columns. Edges clamp.
            void BoxBlurPass(uint8_t* data, int32_t length, int32_t lines, ptrdiff_t sampleStep, ptrdiff_t lineStep, int32_t channels,
                int32_t radius, std::vector<uint32_t>& line)
            {
                if (radius <= 0 || length <= 0)
                {
                    return;
                }
                const uint32_t window = static_cast<uint32_t>(2 * radius + 1);
                line.resize(static_cast<size_t>(length) * channels);
                for (int32_t lineIndex = 0; lineIndex < lines; ++lineIndex)
                {
                    uint8_t* base = data + lineIndex * lineStep;
                    for (int32_t i = 0; i < length; ++i)
                    {
                        for (int32_t c = 0; c < channels; ++c)
                        {
                            line[static_cast<size_t>(i) * channels + c] = base[i * sampleStep + c];
                        }
                    }
                    for (int32_t c = 0; c < channels; ++c)
                    {
                        const auto at = [&](int32_t i) { return line[static_cast<size_t>(std::clamp(i, 0, length - 1)) * channels + c]; };
                        uint32_t sum = 0;
                        for (int32_t k = -radius; k <= radius; ++k)
                        {
                            sum += at(k);
                        }
                        for (int32_t i = 0; i < length; ++i)
                        {
                            base[i * sampleStep + c] = static_cast<uint8_t>((sum + window / 2) / window);
                            sum = sum + at(i + radius + 1) - at(i - radius);
                        }
                    }
                }
            }

            // Three box passes per axis over a width x height image with `channels` bytes per pixel and rowStride bytes per row.
            void GaussianBlur(uint8_t* data, int32_t width, int32_t height, ptrdiff_t rowStride, int32_t channels, float sigmaX, float sigmaY,
                std::vector<uint32_t>& line)
            {
                int32_t radii[3];
                BoxRadii(sigmaX, radii);
                for (int32_t radius : radii)
                {
                    BoxBlurPass(data, width, height, channels, rowStride, channels, radius, line);
                }
                BoxRadii(sigmaY, radii);
                for (int32_t radius : radii)
                {
                    BoxBlurPass(data, height, width, rowStride, channels, channels, radius, line);
                }
            }

            // Collects a DirectWrite glyph outline (DIPs, y down, origin on the baseline) into a GraphicsPath. Lives on the
            // stack for one GetGlyphRunOutline call, so reference counting is nominal.
            class GraphicsPathSink final : public IDWriteGeometrySink
            {
            public:
                explicit GraphicsPathSink(Gdiplus::GraphicsPath& path) noexcept : m_path(path) {}

                IFACEMETHODIMP QueryInterface(REFIID iid, void** object) noexcept override
                {
                    if (object == nullptr)
                    {
                        return E_POINTER;
                    }
                    if (iid == __uuidof(IUnknown) || iid == __uuidof(ID2D1SimplifiedGeometrySink))
                    {
                        *object = static_cast<IDWriteGeometrySink*>(this);
                        return S_OK;
                    }
                    *object = nullptr;
                    return E_NOINTERFACE;
                }
                IFACEMETHODIMP_(ULONG) AddRef() noexcept override { return 1; }
                IFACEMETHODIMP_(ULONG) Release() noexcept override { return 1; }

                IFACEMETHODIMP_(void) SetFillMode(D2D1_FILL_MODE fillMode) noexcept override
                {
                    Check(m_path.SetFillMode(fillMode == D2D1_FILL_MODE_WINDING ? Gdiplus::FillModeWinding : Gdiplus::FillModeAlternate));
                }
                IFACEMETHODIMP_(void) SetSegmentFlags(D2D1_PATH_SEGMENT) noexcept override {}
                IFACEMETHODIMP_(void) BeginFigure(D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN) noexcept override
                {
                    Check(m_path.StartFigure());
                    m_current = Gdiplus::PointF(startPoint.x, startPoint.y);
                }
                IFACEMETHODIMP_(void) AddLines(const D2D1_POINT_2F* points, UINT32 count) noexcept override
                {
                    for (UINT32 index = 0; index < count; ++index)
                    {
                        const Gdiplus::PointF end(points[index].x, points[index].y);
                        Check(m_path.AddLine(m_current, end));
                        m_current = end;
                    }
                }
                IFACEMETHODIMP_(void) AddBeziers(const D2D1_BEZIER_SEGMENT* beziers, UINT32 count) noexcept override
                {
                    for (UINT32 index = 0; index < count; ++index)
                    {
                        const D2D1_BEZIER_SEGMENT& b = beziers[index];
                        const Gdiplus::PointF end(b.point3.x, b.point3.y);
                        Check(m_path.AddBezier(m_current, Gdiplus::PointF(b.point1.x, b.point1.y), Gdiplus::PointF(b.point2.x, b.point2.y), end));
                        m_current = end;
                    }
                }
                IFACEMETHODIMP_(void) EndFigure(D2D1_FIGURE_END figureEnd) noexcept override
                {
                    if (figureEnd == D2D1_FIGURE_END_CLOSED)
                    {
                        Check(m_path.CloseFigure());
                    }
                }
                IFACEMETHODIMP Close() noexcept override { return m_ok ? S_OK : E_FAIL; }

            private:
                void Check(Gdiplus::Status status) noexcept { m_ok = m_ok && status == Gdiplus::Ok; }

                Gdiplus::GraphicsPath& m_path;
                Gdiplus::PointF m_current;
                bool m_ok = true;
            };

            // Maps DIPs to the pixel grid shadows and backdrop blurs are computed on: device pixels under an axis-aligned
            // transform (X = ax * x + bx), otherwise a local grid scaled like the transform (bx = by = 0) that is then
            // drawn through the world transform.
            struct MaskSpace
            {
                bool axisAligned = false;
                float ax = 1, ay = 1, bx = 0, by = 0;

                RectF Map(const RectF& rect) const noexcept
                {
                    return { ax * rect.left + bx, ay * rect.top + by, ax * rect.right + bx, ay * rect.bottom + by };
                }
            };

            using ShadowKey = std::array<uint32_t, 7>; // width, height, radiusX, radiusY, sigmaX, sigmaY bits; phase

            struct ShadowMask
            {
                uint32_t width = 0, height = 0;
                std::vector<uint8_t> alpha;
                std::list<ShadowKey>::iterator lru;
            };

            struct GradientBrushEntry
            {
                std::unique_ptr<Gdiplus::LinearGradientBrush> brush;
                uint64_t bytes = 0;
                std::list<std::vector<uint32_t>>::iterator lru;
            };

            using GlyphOutlineKey = std::tuple<IDWriteFontFace*, uint16_t, uint32_t>; // face, glyph, em size bits

            struct GlyphOutlineEntry
            {
                Microsoft::WRL::ComPtr<IDWriteFontFace> face; // keeps the pointer key from being reused
                std::unique_ptr<Gdiplus::GraphicsPath> path;  // empty for blank glyphs
                uint64_t bytes = 0;
                std::list<GlyphOutlineKey>::iterator lru;
            };

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
                bool isColor = false;                         // COLR/CPAL layers
                bool outline = false;                         // draw from DirectWrite outlines instead of GDI+ text
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
                // A Bitmap over the pixel-aligned sub-region last drawn with a source rect, so bilinear sampling clamps
                // at the source rect's edges instead of bleeding in neighboring pixels.
                std::unique_ptr<Gdiplus::Bitmap> view;
                PixelRect viewRect{};
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
                    const FontFaceEntry* entry = GetFontFace(run.fontFace);
                    if (!entry)
                    {
                        return;
                    }
                    const bool outline = entry->outline;
                    const bool isColor = entry->isColor;
                    float minX, maxX, minY, maxY;
                    LayoutGlyphs(run, minX, maxX, minY, maxY);

                    bool clearType = false;
                    if (run.clearType)
                    {
                        DWRITE_FONT_METRICS metrics{};
                        run.fontFace->GetMetrics(&metrics);
                        const float scale = metrics.designUnitsPerEm ? run.fontEmSize / metrics.designUnitsPerEm : 0;
                        clearType = IsOpaqueUnder(DeviceBounds({ minX, minY - metrics.ascent * scale, maxX, maxY + metrics.descent * scale }, 0));
                    }
                    if (!(run.useColorFont && isColor && DrawColorLayers(run, color, clearType)))
                    {
                        DrawGlyphs(run, color, outline, clearType);
                    }

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
                    BitmapEntry& entry = *found->second;
                    RectF source = sourceRect ? *sourceRect : RectF{ 0, 0, static_cast<float>(entry.width), static_cast<float>(entry.height) };
                    if (!(source.right > source.left) || !(source.bottom > source.top))
                    {
                        return;
                    }
                    Gdiplus::Bitmap* image = entry.bitmap.get();
                    const PixelRect whole{ 0, 0, static_cast<int32_t>(entry.width), static_cast<int32_t>(entry.height) };
                    const PixelRect region = Intersect(whole, { ToPixel(std::floor(source.left)), ToPixel(std::floor(source.top)),
                        ToPixel(std::ceil(source.right)), ToPixel(std::ceil(source.bottom)) });
                    if (region.IsEmpty())
                    {
                        return;
                    }
                    if (region.left != 0 || region.top != 0 || region.right != whole.right || region.bottom != whole.bottom)
                    {
                        if (!entry.view || memcmp(&entry.viewRect, &region, sizeof(region)) != 0)
                        {
                            if (entry.view)
                            {
                                entry.view.reset();
                                ++m_frameDestroyed;
                            }
                            entry.view.reset(new Gdiplus::Bitmap(region.right - region.left, region.bottom - region.top,
                                static_cast<INT>(entry.width * 4), PixelFormat32bppPARGB,
                                entry.pixels.data() + (static_cast<size_t>(region.top) * entry.width + region.left) * 4));
                            if (FAILED(CheckCreated(entry.view)))
                            {
                                return;
                            }
                            entry.viewRect = region;
                            ++m_frameCreated;
                        }
                        image = entry.view.get();
                        source = { source.left - region.left, source.top - region.top, source.right - region.left, source.bottom - region.top };
                    }
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
                    Record(graphics.DrawImage(image, ToGdiRect(destination), source.left, source.top, source.right - source.left,
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

                void FillRoundedRectangleGradient(const RectF& rect, float radiusX, float radiusY, const LinearGradientDesc& gradient) noexcept override
                {
                    if (!BeginPrimitive())
                    {
                        return;
                    }
                    Gdiplus::Brush* brush = GetGradientBrush(gradient, rect);
                    if (!brush)
                    {
                        return;
                    }
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(rect, 0));
                    MaskSpace space;
                    if (radiusX > 0 && radiusY > 0 && GetMaskSpace(space) && space.axisAligned &&
                        FillRoundedGradientExact(rect, radiusX, radiusY, gradient, *brush, space))
                    {
                        return;
                    }
                    FineAntialiasing fine(Current());
                    if (BuildRoundedPath(rect, radiusX, radiusY))
                    {
                        Record(Current().FillPath(brush, m_path.get()));
                    }
                    else
                    {
                        Record(Current().FillRectangle(brush, ToGdiRect(rect)));
                    }
                }

                void FillPathGradient(const PathData& data, const LinearGradientDesc& gradient) noexcept override
                {
                    if (!BeginPrimitive())
                    {
                        return;
                    }
                    RectF bounds{};
                    Gdiplus::GraphicsPath* path = GetPath(data, bounds);
                    Gdiplus::Brush* brush = path ? GetGradientBrush(gradient, bounds) : nullptr;
                    if (!brush)
                    {
                        return;
                    }
                    FineAntialiasing fine(Current());
                    Record(Current().FillPath(brush, path));
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(bounds, 0));
                }

                void DrawRoundedRectangleShadow(const RectF& rect, float radiusX, float radiusY, float blurRadius, const PointF& offset, const ColorF& color) noexcept override
                {
                    if (!BeginPrimitive() || !(color.a > 0) || !std::isfinite(offset.x) || !std::isfinite(offset.y))
                    {
                        return;
                    }
                    MaskSpace space;
                    if (!GetMaskSpace(space))
                    {
                        return;
                    }
                    const RectF shape = space.Map({ rect.left + offset.x, rect.top + offset.y, rect.right + offset.x, rect.bottom + offset.y });
                    const float width = shape.right - shape.left;
                    const float height = shape.bottom - shape.top;
                    if (!(width > 0) || !(height > 0) || !std::isfinite(width) || !std::isfinite(height) || !std::isfinite(shape.left) || !std::isfinite(shape.top))
                    {
                        return;
                    }
                    const bool rounded = radiusX > 0 && radiusY > 0;
                    const float rx = rounded ? std::min(radiusX * space.ax, width / 2) : 0;
                    const float ry = rounded ? std::min(radiusY * space.ay, height / 2) : 0;
                    const float blur = (blurRadius > 0 && std::isfinite(blurRadius)) ? blurRadius : 0;
                    const float sigmaX = std::min(blur / 2 * space.ax, c_maxSigma);
                    const float sigmaY = std::min(blur / 2 * space.ay, c_maxSigma);
                    const int32_t padX = static_cast<int32_t>(std::ceil(3 * sigmaX)) + 1;
                    const int32_t padY = static_cast<int32_t>(std::ceil(3 * sigmaY)) + 1;

                    // Subpixel phase quantized to 1/8 px, so moving shadows reuse masks.
                    const auto phase = [](float edge, int32_t& origin) noexcept
                    {
                        const float floored = std::floor(edge);
                        const uint32_t eighths = static_cast<uint32_t>(std::lround((edge - floored) * 8));
                        origin = ToPixel(floored) + (eighths == 8 ? 1 : 0);
                        return eighths & 7;
                    };
                    int32_t shapeX = 0, shapeY = 0;
                    const uint32_t phaseX = phase(shape.left, shapeX);
                    const uint32_t phaseY = phase(shape.top, shapeY);
                    const double maskWidth = std::ceil(phaseX / 8.0 + width) + 2.0 * padX;
                    const double maskHeight = std::ceil(phaseY / 8.0 + height) + 2.0 * padY;
                    if (maskWidth * maskHeight > static_cast<double>(c_maxShadowPixels))
                    {
                        return;
                    }
                    const uint32_t w = static_cast<uint32_t>(maskWidth);
                    const uint32_t h = static_cast<uint32_t>(maskHeight);
                    const RectF maskShape{ padX + phaseX / 8.0f, padY + phaseY / 8.0f, padX + phaseX / 8.0f + width, padY + phaseY / 8.0f + height };
                    const ShadowKey key{ FloatBits(width), FloatBits(height), FloatBits(rx), FloatBits(ry), FloatBits(sigmaX), FloatBits(sigmaY), (phaseX << 3) | phaseY };
                    const ShadowMask* mask = GetShadowMask(key, w, h, maskShape, rx, ry, sigmaX, sigmaY);
                    if (!mask || !EnsureShadowScratch(w + 1, h + 1))
                    {
                        return;
                    }

                    // Color the mask and knock out the shape itself (without the offset), both in mask pixels.
                    const int32_t originX = shapeX - padX;
                    const int32_t originY = shapeY - padY;
                    const RectF knockDevice = space.Map(rect);
                    const RectF knock{ knockDevice.left - originX, knockDevice.top - originY, knockDevice.right - originX, knockDevice.bottom - originY };
                    const float kw = knock.right - knock.left, kh = knock.bottom - knock.top;
                    const float krx = rounded ? std::min(radiusX * space.ax, kw / 2) : 0;
                    const float kry = rounded ? std::min(radiusY * space.ay, kh / 2) : 0;
                    const bool knockout = kw > 0 && kh > 0;
                    const float alpha = std::min(color.a, 1.0f);
                    uint32_t palette[256];
                    for (uint32_t coverage = 0; coverage < 256; ++coverage)
                    {
                        const float a = alpha * coverage / 255;
                        palette[coverage] = (static_cast<uint32_t>(ToByte(a)) << 24) | (static_cast<uint32_t>(ToByte(color.r * a)) << 16) |
                            (static_cast<uint32_t>(ToByte(color.g * a)) << 8) | ToByte(color.b * a);
                    }
                    const uint32_t stride = m_shadowScratchWidth;
                    uint32_t* pixels = reinterpret_cast<uint32_t*>(m_shadowScratch.pixels.data());
                    for (uint32_t y = 0; y < h; ++y)
                    {
                        const uint8_t* source = mask->alpha.data() + static_cast<size_t>(y) * w;
                        uint32_t* destination = pixels + static_cast<size_t>(y) * stride;
                        for (uint32_t x = 0; x < w; ++x)
                        {
                            uint32_t coverage = source[x];
                            if (coverage != 0 && knockout)
                            {
                                const uint32_t knocked = RoundedRectCoverage(knock, krx, kry, static_cast<int32_t>(x), static_cast<int32_t>(y));
                                coverage = (coverage * (255 - knocked) + 127) / 255;
                            }
                            destination[x] = palette[coverage];
                        }
                        destination[w] = 0;
                    }
                    memset(pixels + static_cast<size_t>(h) * stride, 0, static_cast<size_t>(w + 1) * 4);

                    Gdiplus::Graphics& graphics = Current();
                    if (space.axisAligned)
                    {
                        // Identity transform, integer rects, nearest-neighbor sampling, and half pixel offset: a 1:1 copy.
                        Record(graphics.ResetTransform());
                        Record(graphics.DrawImage(m_shadowScratch.bitmap.get(), Gdiplus::Rect(originX, originY, static_cast<INT>(w), static_cast<INT>(h)),
                            0, 0, static_cast<INT>(w), static_cast<INT>(h), Gdiplus::UnitPixel, nullptr));
                        Record(graphics.SetTransform(m_transform.get()));
                        m_stats.paintedPixels += VisibleArea({ originX, originY, originX + static_cast<int32_t>(w), originY + static_cast<int32_t>(h) });
                    }
                    else
                    {
                        const RectF destination{ originX / space.ax, originY / space.ay, (originX + static_cast<float>(w)) / space.ax, (originY + static_cast<float>(h)) / space.ay };
                        Record(graphics.SetInterpolationMode(Gdiplus::InterpolationModeBilinear));
                        Record(graphics.DrawImage(m_shadowScratch.bitmap.get(), ToGdiRect(destination), 0, 0, static_cast<Gdiplus::REAL>(w),
                            static_cast<Gdiplus::REAL>(h), Gdiplus::UnitPixel));
                        Record(graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor));
                        m_stats.paintedPixels += VisibleArea(DeviceBounds(destination, 0));
                    }
                }

                void BlurBackdrop(const RectF& rect, float radiusX, float radiusY, float blurRadius) noexcept override
                {
                    if (!m_inFrame || !(blurRadius > 0) || !std::isfinite(blurRadius))
                    {
                        return;
                    }
                    if (!BeginPrimitive())
                    {
                        return;
                    }
                    MaskSpace space;
                    if (!GetMaskSpace(space))
                    {
                        return;
                    }
                    const PixelRect region = Intersect(Intersect(DeviceBounds(rect, 0), CurrentClip()), m_damageBounds);
                    if (region.IsEmpty())
                    {
                        return;
                    }
                    const float sigmaX = std::min(blurRadius / 2 * space.ax, c_maxSigma);
                    const float sigmaY = std::min(blurRadius / 2 * space.ay, c_maxSigma);
                    const int32_t padX = static_cast<int32_t>(std::ceil(3 * sigmaX)) + 1;
                    const int32_t padY = static_cast<int32_t>(std::ceil(3 * sigmaY)) + 1;
                    const PixelRect source = Intersect({ region.left - padX, region.top - padY, region.right + padX, region.bottom + padY }, FullTarget());
                    const int32_t sw = source.right - source.left;
                    const int32_t sh = source.bottom - source.top;

                    Surface& surface = CurrentSurface();
                    surface.graphics->Flush(Gdiplus::FlushIntentionSync);
                    const size_t stride = static_cast<size_t>(m_width) * 4;
                    const size_t blurStride = static_cast<size_t>(sw) * 4;
                    m_blurPixels.resize(blurStride * sh);
                    for (int32_t y = 0; y < sh; ++y)
                    {
                        memcpy(m_blurPixels.data() + y * blurStride, surface.pixels.data() + (source.top + y) * stride + static_cast<size_t>(source.left) * 4, blurStride);
                    }
                    GaussianBlur(m_blurPixels.data(), sw, sh, static_cast<ptrdiff_t>(blurStride), 4, sigmaX, sigmaY, m_blurLine);

                    // Antialiased coverage of the rounded rect: exact under axis-aligned transforms, pixel-center tests
                    // through the inverse transform otherwise.
                    const RectF device = space.Map(rect);
                    const bool rounded = radiusX > 0 && radiusY > 0;
                    const float drx = rounded ? std::min(radiusX * space.ax, (device.right - device.left) / 2) : 0;
                    const float dry = rounded ? std::min(radiusY * space.ay, (device.bottom - device.top) / 2) : 0;
                    const Matrix3x2F& t = m_userTransform;
                    const float determinant = t.m11 * t.m22 - t.m21 * t.m12;
                    if (!space.axisAligned && !(std::fabs(determinant) > 0))
                    {
                        return;
                    }
                    const float lrx = rounded ? std::min(radiusX, (rect.right - rect.left) / 2) : 0;
                    const float lry = rounded ? std::min(radiusY, (rect.bottom - rect.top) / 2) : 0;
                    const auto coverageAt = [&](int32_t x, int32_t y) noexcept -> uint32_t
                    {
                        if (space.axisAligned)
                        {
                            return RoundedRectCoverage(device, drx, dry, x, y);
                        }
                        const float u = (x + 0.5f) / m_dpiScale - t.dx;
                        const float v = (y + 0.5f) / m_dpiScale - t.dy;
                        const float lx = (u * t.m22 - v * t.m21) / determinant;
                        const float ly = (v * t.m11 - u * t.m12) / determinant;
                        if (!(lx >= rect.left && lx < rect.right && ly >= rect.top && ly < rect.bottom))
                        {
                            return 0;
                        }
                        if (lrx > 0 && lry > 0)
                        {
                            const float nx = (lx - std::clamp(lx, rect.left + lrx, rect.right - lrx)) / lrx;
                            const float ny = (ly - std::clamp(ly, rect.top + lry, rect.bottom - lry)) / lry;
                            return nx * nx + ny * ny <= 1 ? 255 : 0;
                        }
                        return 255;
                    };

                    for (const PixelRect& damage : m_damage)
                    {
                        const PixelRect area = Intersect(damage, region);
                        for (int32_t y = area.top; y < area.bottom; ++y)
                        {
                            uint8_t* destination = surface.pixels.data() + y * stride + static_cast<size_t>(area.left) * 4;
                            const uint8_t* blurred = m_blurPixels.data() + (y - source.top) * blurStride + static_cast<size_t>(area.left - source.left) * 4;
                            for (int32_t x = area.left; x < area.right; ++x, destination += 4, blurred += 4)
                            {
                                const uint32_t coverage = coverageAt(x, y);
                                if (coverage == 0)
                                {
                                    continue;
                                }
                                for (int channel = 0; channel < 4; ++channel)
                                {
                                    destination[channel] = static_cast<uint8_t>((destination[channel] * (255 - coverage) + blurred[channel] * coverage + 127) / 255);
                                }
                            }
                        }
                    }
                    m_stats.paintedPixels += VisibleArea(region);
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
                    ReleaseLargeScratch();

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
                        inventory.cachedObjectCount += bitmap.second->view ? 1 : 0;
                    }
                    inventory.cacheBytes = m_brushes.size() * c_brushBytes + m_pens.size() * c_penBytes + m_fonts.size() * c_fontBytes +
                        families * c_fontFamilyBytes + (m_stringFormat ? c_stringFormatBytes : 0) + (m_imageAttributes ? c_imageAttributesBytes : 0) +
                        (m_path ? c_pathBytes : 0) + (m_transform ? c_matrixBytes : 0) + m_pathCacheBytes + m_fontFaces.size() * c_fontFaceBytes +
                        m_glyphFonts.size() * c_fontBytes + bitmapBytes + (m_bitmapAttributes ? c_imageAttributesBytes : 0) +
                        (m_gdiInterop ? c_dwriteInteropBytes : 0);
                    // S11-S23 caches: gradient brushes, shadow masks, the shadow compositing scratch, glyph outlines.
                    inventory.cachedObjectCount += static_cast<uint32_t>(m_gradientBrushes.size() + m_shadowMasks.size() + m_glyphOutlines.size()) +
                        (m_shadowScratch.bitmap ? 1 : 0);
                    inventory.cacheBytes += m_gradientBrushBytes + m_shadowMaskBytes + m_glyphOutlineBytes +
                        m_shadowScratch.pixels.size() + m_blurPixels.capacity() + m_blurLine.capacity() * sizeof(uint32_t);
                    // ---- shapes: styled pen, outline/flatten/dash paths, scratch mask Graphics and matrix, ramp ----
                    const uint32_t shapeObjects = (m_stylePen ? 1 : 0) + (m_outlinePath ? 6 : 0) + (m_scratchMatrix ? 1 : 0) +
                        (m_shadowScratch.graphics ? 1 : 0);
                    inventory.cachedObjectCount += shapeObjects;
                    inventory.cacheBytes += (m_stylePen ? c_penBytes : 0) + (m_outlinePath ? 6 * c_pathBytes : 0) + (m_scratchMatrix ? c_matrixBytes : 0) +
                        m_ramp.capacity() * sizeof(uint32_t) + (m_flatPoints.capacity() + m_figurePoints.capacity() + m_dashPoints.capacity()) * sizeof(Gdiplus::PointF);
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

                // ---- shapes (F1 shapes and strokes, F2 brushes) ----
                // Solid brushes draw directly. Gradient brushes (linear and radial, every spread and interpolation mode,
                // any brush transform) are "mask and shade": the primitive is drawn in white into the scratch surface
                // (device pixels of its bounds under the clip and damage), and each covered pixel is replaced by the
                // gradient evaluated at its center (1024-entry ramp) times its coverage, then composited 1:1.
                // Dashed strokes are dashed here (flattened outline walked by the dash pattern) so every dash cap,
                // including square, and zero-length dashes match D2D; undashed strokes use one reusable styled pen.

                void FillRoundedRectangleBrush(const RectF& rect, float radiusX, float radiusY, const BrushDesc& brush) noexcept override
                {
                    if (brush.kind == BrushKind::Solid)
                    {
                        FillRoundedRectangle(rect, radiusX, radiusY, SolidColor(brush));
                        return;
                    }
                    if (!BeginPrimitive() || !PrepareRamp(brush))
                    {
                        return;
                    }
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(rect, 0));
                    ShadeBrush(DeviceBounds(rect, 1), brush, [&](Gdiplus::Graphics& graphics, const Gdiplus::Color& white)
                    {
                        Gdiplus::SolidBrush* mask = GetBrush(c_maskWhite);
                        (void)white;
                        if (!mask)
                        {
                            return;
                        }
                        FineAntialiasing fine(graphics);
                        if (BuildRoundedPath(rect, radiusX, radiusY))
                        {
                            Record(graphics.FillPath(mask, m_path.get()));
                        }
                        else
                        {
                            Record(graphics.FillRectangle(mask, ToGdiRect(rect)));
                        }
                    });
                }

                void FillPathBrush(const PathData& data, const BrushDesc& brush) noexcept override
                {
                    if (brush.kind == BrushKind::Solid)
                    {
                        FillPath(data, SolidColor(brush));
                        return;
                    }
                    if (!BeginPrimitive() || !PrepareRamp(brush))
                    {
                        return;
                    }
                    RectF bounds{};
                    Gdiplus::GraphicsPath* path = GetPath(data, bounds);
                    if (!path)
                    {
                        return;
                    }
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(bounds, 0));
                    ShadeBrush(DeviceBounds(bounds, 1), brush, [&](Gdiplus::Graphics& graphics, const Gdiplus::Color&)
                    {
                        if (Gdiplus::SolidBrush* mask = GetBrush(c_maskWhite))
                        {
                            FineAntialiasing fine(graphics);
                            Record(graphics.FillPath(mask, path));
                        }
                    });
                }

                void StrokeRoundedRectangleBrush(const RectF& rect, float radiusX, float radiusY, float thickness, const BrushDesc& brush,
                    const StrokeStyleDesc* style) noexcept override
                {
                    if (!BeginPrimitive() || !(thickness > 0) || !EnsureShapePaths())
                    {
                        return;
                    }
                    BuildOutline(rect, radiusX, radiusY, *m_outlinePath);
                    StrokeWithBrush(*m_outlinePath, rect, thickness, brush, style);
                }

                void StrokePathBrush(const PathData& data, float thickness, const BrushDesc& brush, const StrokeStyleDesc* style) noexcept override
                {
                    if (!BeginPrimitive() || !(thickness > 0) || !EnsureShapePaths())
                    {
                        return;
                    }
                    RectF bounds{};
                    Gdiplus::GraphicsPath* path = GetPath(data, bounds);
                    if (path)
                    {
                        StrokeWithBrush(*path, bounds, thickness, brush, style);
                    }
                }

                void DrawGlyphRunBrush(const GlyphRunDesc& run, const BrushDesc& brush) noexcept override
                {
                    GlyphRunDesc plain = run;
                    plain.useColorFont = false;
                    plain.clearType = false;
                    if (brush.kind == BrushKind::Solid)
                    {
                        DrawGlyphRun(plain, SolidColor(brush));
                        return;
                    }
                    if (run.fontFace == nullptr || run.glyphIndices == nullptr || run.glyphAdvances == nullptr || run.glyphCount == 0 ||
                        !(run.fontEmSize > 0) || !std::isfinite(run.fontEmSize))
                    {
                        return;
                    }
                    if (!BeginPrimitive() || !PrepareRamp(brush))
                    {
                        return;
                    }
                    const FontFaceEntry* entry = GetFontFace(run.fontFace);
                    if (!entry)
                    {
                        return;
                    }
                    const bool outline = entry->outline;
                    float minX, maxX, minY, maxY;
                    LayoutGlyphs(run, minX, maxX, minY, maxY);
                    const float em = run.fontEmSize;
                    const RectF ink{ minX - em * 0.5f, minY - em * 1.5f, maxX + em * 0.5f, maxY + em * 0.75f };
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(ink, 0));
                    ShadeBrush(DeviceBounds(ink, 1), brush, [&](Gdiplus::Graphics& graphics, const Gdiplus::Color&)
                    {
                        // DrawGlyphs draws on Current() under m_transform: point both at the scratch for the mask.
                        m_redirect = &graphics;
                        Gdiplus::REAL elements[6];
                        m_scratchMatrix->GetElements(elements);
                        m_transform->SetElements(elements[0], elements[1], elements[2], elements[3], elements[4], elements[5]);
                        DrawGlyphs(plain, c_maskWhite, outline, false);
                        m_redirect = nullptr;
                        UpdateDeviceTransform();
                    });
                }

            private:
                static constexpr ColorF c_maskWhite{ 1, 1, 1, 1 };
                static constexpr uint32_t c_rampSize = 1024;
                static constexpr size_t c_maxDashes = 1 << 17; // dashes per stroke; finer patterns draw solid

                static ColorF SolidColor(const BrushDesc& brush) noexcept
                {
                    ColorF color = brush.color;
                    color.a *= brush.opacity >= 0 ? std::min(brush.opacity, 1.0f) : 0.0f;
                    return color;
                }

                static Gdiplus::LineCap ToGdiCap(LineCap cap) noexcept
                {
                    switch (cap)
                    {
                        case LineCap::Square: return Gdiplus::LineCapSquare;
                        case LineCap::Round: return Gdiplus::LineCapRound;
                        case LineCap::Triangle: return Gdiplus::LineCapTriangle;
                        default: return Gdiplus::LineCapFlat;
                    }
                }

                static bool IsDashed(const StrokeStyleDesc* style) noexcept
                {
                    if (style == nullptr || style->dashes == nullptr || style->dashCount == 0 || !std::isfinite(style->dashOffset))
                    {
                        return false;
                    }
                    float total = 0;
                    for (uint32_t i = 0; i < style->dashCount; ++i)
                    {
                        if (!std::isfinite(style->dashes[i]) || style->dashes[i] < 0)
                        {
                            return false;
                        }
                        total += style->dashes[i];
                    }
                    return total > 0 && std::isfinite(total);
                }

                // How far (DIPs) the stroke can reach beyond the outline: miter tips, square and triangle caps.
                static float StrokeReach(float thickness, const StrokeStyleDesc* style) noexcept
                {
                    const float miter = style ? (std::isfinite(style->miterLimit) ? std::max(style->miterLimit, 1.0f) : 10.0f) : 10.0f;
                    const bool mitered = style == nullptr || style->lineJoin == LineJoin::Miter;
                    return thickness / 2 * std::max(mitered ? miter : 1.0f, 1.5f);
                }

                bool EnsureShapePaths() noexcept
                {
                    const auto create = [&](std::unique_ptr<Gdiplus::GraphicsPath>& path)
                    {
                        if (!path)
                        {
                            path.reset(new Gdiplus::GraphicsPath());
                            const HRESULT hr = CheckCreated(path);
                            if (FAILED(hr))
                            {
                                path.reset();
                                Fail(hr);
                                return false;
                            }
                            ++m_frameCreated;
                        }
                        return true;
                    };
                    bool ok = create(m_outlinePath) && create(m_flatPath);
                    for (auto& group : m_dashGroups)
                    {
                        ok = ok && create(group);
                    }
                    if (ok && !m_stylePen)
                    {
                        m_stylePen.reset(new Gdiplus::Pen(Gdiplus::Color(), 1.0f));
                        const HRESULT hr = CheckCreated(m_stylePen);
                        if (FAILED(hr))
                        {
                            m_stylePen.reset();
                            Fail(hr);
                            return false;
                        }
                        ++m_frameCreated;
                    }
                    return ok;
                }

                // Rect / rounded rect / ellipse outline, starting and running like D2D's geometries (for dashes).
                void BuildOutline(const RectF& rect, float radiusX, float radiusY, Gdiplus::GraphicsPath& path) noexcept
                {
                    Record(path.Reset());
                    const float width = rect.right - rect.left, height = rect.bottom - rect.top;
                    if (!(width >= 0) || !(height >= 0))
                    {
                        return;
                    }
                    if (radiusX > 0 && radiusY > 0 && radiusX * 2 >= width && radiusY * 2 >= height)
                    {
                        // AddEllipse starts at the rightmost point and runs clockwise (y down), like D2D's ellipse.
                        Record(path.AddEllipse(ToGdiRect(rect)));
                        return;
                    }
                    if (!(radiusX > 0 && radiusY > 0))
                    {
                        Record(path.AddRectangle(ToGdiRect(rect)));
                        return;
                    }
                    const float rx = std::min(radiusX, width / 2), ry = std::min(radiusY, height / 2);
                    const float dx = 2 * rx, dy = 2 * ry;
                    // D2D's rounded rectangle starts where the top-left arc starts, (left, top + ry), and runs clockwise.
                    Record(path.StartFigure());
                    Record(path.AddArc(rect.left, rect.top, dx, dy, 180, 90));
                    Record(path.AddLine(rect.left + rx, rect.top, rect.right - rx, rect.top));
                    Record(path.AddArc(rect.right - dx, rect.top, dx, dy, 270, 90));
                    Record(path.AddLine(rect.right, rect.top + ry, rect.right, rect.bottom - ry));
                    Record(path.AddArc(rect.right - dx, rect.bottom - dy, dx, dy, 0, 90));
                    Record(path.AddLine(rect.right - rx, rect.bottom, rect.left + rx, rect.bottom));
                    Record(path.AddArc(rect.left, rect.bottom - dy, dx, dy, 90, 90));
                    Record(path.AddLine(rect.left, rect.bottom - ry, rect.left, rect.top + ry));
                    Record(path.CloseFigure());
                }

                void StrokeWithBrush(Gdiplus::GraphicsPath& path, const RectF& bounds, float thickness, const BrushDesc& brush, const StrokeStyleDesc* style) noexcept
                {
                    m_stats.paintedPixels += VisibleArea(DeviceBounds(bounds, thickness / 2));
                    if (brush.kind == BrushKind::Solid)
                    {
                        DrawStyled(Current(), path, thickness, ToGdiColor(SolidColor(brush)), style);
                        return;
                    }
                    if (!PrepareRamp(brush))
                    {
                        return;
                    }
                    ShadeBrush(DeviceBounds(bounds, StrokeReach(thickness, style) + 1), brush, [&](Gdiplus::Graphics& graphics, const Gdiplus::Color& white)
                    {
                        DrawStyled(graphics, path, thickness, white, style);
                    });
                }

                void DrawStyled(Gdiplus::Graphics& graphics, Gdiplus::GraphicsPath& path, float thickness, const Gdiplus::Color& color, const StrokeStyleDesc* style) noexcept
                {
                    Gdiplus::Pen& pen = *m_stylePen;
                    Record(pen.SetColor(color));
                    Record(pen.SetWidth(thickness));
                    Record(pen.SetDashStyle(Gdiplus::DashStyleSolid));
                    // D2D (and Composition) clip a miter join at miterLimit half-widths from the vertex. GDI+ bevels a
                    // miter past its limit, which it measures against the full width (so limit / 2, at least 1); the
                    // clipped part is filled by FillMiterJoins. Below a limit of 2 the pen bevels and FillMiterJoins
                    // adds every miter.
                    const bool miterJoin = style == nullptr || style->lineJoin == LineJoin::Miter;
                    const float miterLimit = style ? (std::isfinite(style->miterLimit) ? std::max(style->miterLimit, 1.0f) : 10.0f) : 10.0f;
                    const bool penBevels = miterJoin && miterLimit < 2;
                    Record(pen.SetLineJoin(!miterJoin ? (style->lineJoin == LineJoin::Round ? Gdiplus::LineJoinRound : Gdiplus::LineJoinBevel) :
                        penBevels ? Gdiplus::LineJoinBevel : Gdiplus::LineJoinMiter));
                    Record(pen.SetMiterLimit(std::max(miterLimit / 2, 1.0f)));
                    const LineCap startCap = style ? style->startCap : LineCap::Flat;
                    const LineCap endCap = style ? style->endCap : LineCap::Flat;
                    if (!IsDashed(style) || !BuildDashes(path, *style))
                    {
                        Record(pen.SetStartCap(ToGdiCap(startCap)));
                        Record(pen.SetEndCap(ToGdiCap(endCap)));
                        Record(graphics.DrawPath(&pen, &path));
                        if (miterJoin)
                        {
                            FillMiterJoins(graphics, path, thickness, miterLimit, penBevels, color);
                        }
                        return;
                    }
                    // Groups: bit 0 = the dash starts the open figure (start cap), bit 1 = it ends it (end cap).
                    for (uint32_t group = 0; group < 4; ++group)
                    {
                        Gdiplus::GraphicsPath& dashes = *m_dashGroups[group];
                        if (dashes.GetPointCount() == 0)
                        {
                            continue;
                        }
                        Record(pen.SetStartCap(ToGdiCap((group & 1) ? startCap : style->dashCap)));
                        Record(pen.SetEndCap(ToGdiCap((group & 2) ? endCap : style->dashCap)));
                        Record(graphics.DrawPath(&pen, &dashes));
                        if (miterJoin)
                        {
                            FillMiterJoins(graphics, dashes, thickness, miterLimit, penBevels, color);
                        }
                    }
                }

                // Fills the miter joins GDI+ bevelled: the miter clipped at `limit` half-widths from the vertex (D2D
                // D2D1_LINE_JOIN_MITER), or the whole miter when it is within the limit and the pen bevels. Uses the
                // flattening scratch (m_flatPath / m_flatPoints), so it must not run while BuildDashes' output is read.
                void FillMiterJoins(Gdiplus::Graphics& graphics, Gdiplus::GraphicsPath& path, float thickness, float limit, bool penBevels, const Gdiplus::Color& color) noexcept
                {
                    Gdiplus::GraphicsPath& flat = *m_flatPath;
                    Record(flat.Reset());
                    Record(flat.AddPath(&path, FALSE));
                    Gdiplus::REAL elements[6];
                    m_transform->GetElements(elements);
                    const float scale = std::sqrt(std::abs(elements[0] * elements[3] - elements[1] * elements[2]));
                    Record(flat.Flatten(nullptr, 0.1f / std::max(scale, 1e-3f)));
                    const INT count = flat.GetPointCount();
                    if (count <= 0 || FAILED(m_frameHr))
                    {
                        return;
                    }
                    m_flatPoints.resize(static_cast<size_t>(count));
                    m_flatTypes.resize(static_cast<size_t>(count));
                    Record(flat.GetPathPoints(m_flatPoints.data(), count));
                    Record(flat.GetPathTypes(m_flatTypes.data(), count));
                    if (FAILED(m_frameHr))
                    {
                        return;
                    }
                    Gdiplus::SolidBrush fill(color);
                    // Opaque: include the vertex so the polygon overlaps the stroke body (no antialiasing seam).
                    // Translucent: abut the bevel edge instead of blending twice.
                    const bool overlap = color.GetA() == 255;
                    const float h = thickness / 2;
                    const auto same = [](const Gdiplus::PointF& a, const Gdiplus::PointF& b)
                    {
                        return std::abs(a.X - b.X) + std::abs(a.Y - b.Y) <= 1e-5f;
                    };
                    INT start = 0;
                    while (start < count)
                    {
                        INT end = start + 1;
                        while (end < count && (m_flatTypes[end] & Gdiplus::PathPointTypePathTypeMask) != Gdiplus::PathPointTypeStart)
                        {
                            ++end;
                        }
                        const bool closed = (m_flatTypes[end - 1] & Gdiplus::PathPointTypeCloseSubpath) != 0;
                        m_joinFigure.clear();
                        for (INT i = start; i < end; ++i)
                        {
                            if (m_joinFigure.empty() || !same(m_joinFigure.back(), m_flatPoints[i]))
                            {
                                m_joinFigure.push_back(m_flatPoints[i]);
                            }
                        }
                        if (closed && m_joinFigure.size() > 2 && same(m_joinFigure.front(), m_joinFigure.back()))
                        {
                            m_joinFigure.pop_back();
                        }
                        start = end;
                        const size_t n = m_joinFigure.size();
                        if (n < 3)
                        {
                            continue;
                        }
                        for (size_t i = closed ? 0 : 1; i < (closed ? n : n - 1); ++i)
                        {
                            const Gdiplus::PointF& prev = m_joinFigure[(i + n - 1) % n];
                            const Gdiplus::PointF& v = m_joinFigure[i];
                            const Gdiplus::PointF& next = m_joinFigure[(i + 1) % n];
                            float d1x = v.X - prev.X, d1y = v.Y - prev.Y, d2x = next.X - v.X, d2y = next.Y - v.Y;
                            const float l1 = std::sqrt(d1x * d1x + d1y * d1y), l2 = std::sqrt(d2x * d2x + d2y * d2y);
                            d1x /= l1; d1y /= l1; d2x /= l2; d2y /= l2;
                            const float cross = d1x * d2y - d1y * d2x;
                            if (std::abs(cross) < 1e-6f && d1x * d2x + d1y * d2y > 0)
                            {
                                continue;
                            }
                            // Outer normals: away from the turn.
                            const float s = cross > 0 ? -1.0f : 1.0f;
                            const float o1x = -s * d1y, o1y = s * d1x, o2x = -s * d2y, o2y = s * d2x;
                            float mx = o1x + o2x, my = o1y + o2y;
                            const float ml = std::sqrt(mx * mx + my * my);
                            if (ml > 1e-6f)
                            {
                                mx /= ml; my /= ml;
                            }
                            else
                            {
                                mx = d1x; my = d1y;
                            }
                            const float cosHalf = o1x * mx + o1y * my;
                            const float ratio = cosHalf > 1e-6f ? 1 / cosHalf : std::numeric_limits<float>::max();
                            if (!(penBevels ? ratio > 1.0001f : ratio > limit))
                            {
                                continue;
                            }
                            const Gdiplus::PointF p1(v.X + o1x * h, v.Y + o1y * h), p2(v.X + o2x * h, v.Y + o2y * h);
                            Gdiplus::PointF polygon[5];
                            INT points = 0;
                            if (overlap)
                            {
                                polygon[points++] = v;
                            }
                            polygon[points++] = p1;
                            if (ratio <= limit)
                            {
                                polygon[points++] = Gdiplus::PointF(v.X + mx * h * ratio, v.Y + my * h * ratio);
                            }
                            else
                            {
                                const float along1 = d1x * mx + d1y * my, along2 = -(d2x * mx + d2y * my);
                                if (along1 < 1e-6f || along2 < 1e-6f)
                                {
                                    continue;
                                }
                                const float t1 = (limit - cosHalf) * h / along1, t2 = (limit - cosHalf) * h / along2;
                                polygon[points++] = Gdiplus::PointF(p1.X + d1x * t1, p1.Y + d1y * t1);
                                polygon[points++] = Gdiplus::PointF(p2.X - d2x * t2, p2.Y - d2y * t2);
                            }
                            polygon[points++] = p2;
                            Record(graphics.FillPolygon(&fill, polygon, points));
                        }
                    }
                }

                // Splits the flattened outline into dashes (m_dashGroups). The pattern restarts at every figure at
                // dashOffset; an odd-length pattern is repeated (SVG/D2D). false: draw undashed.
                bool BuildDashes(Gdiplus::GraphicsPath& source, const StrokeStyleDesc& style) noexcept
                {
                    for (auto& group : m_dashGroups)
                    {
                        Record(group->Reset());
                    }
                    Gdiplus::GraphicsPath& flat = *m_flatPath;
                    Record(flat.Reset());
                    Record(flat.AddPath(&source, FALSE));
                    Gdiplus::REAL elements[6];
                    m_transform->GetElements(elements);
                    const float scale = std::sqrt(std::abs(elements[0] * elements[3] - elements[1] * elements[2]));
                    Record(flat.Flatten(nullptr, 0.1f / std::max(scale, 1e-3f)));
                    const INT count = flat.GetPointCount();
                    if (count <= 0 || FAILED(m_frameHr))
                    {
                        return count == 0;
                    }
                    m_flatPoints.resize(static_cast<size_t>(count));
                    m_flatTypes.resize(static_cast<size_t>(count));
                    Record(flat.GetPathPoints(m_flatPoints.data(), count));
                    Record(flat.GetPathTypes(m_flatTypes.data(), count));
                    if (FAILED(m_frameHr))
                    {
                        return false;
                    }

                    m_dashPattern.assign(style.dashes, style.dashes + style.dashCount);
                    if (m_dashPattern.size() % 2 != 0)
                    {
                        m_dashPattern.insert(m_dashPattern.end(), style.dashes, style.dashes + style.dashCount);
                    }
                    float period = 0;
                    for (float length : m_dashPattern)
                    {
                        period += length;
                    }
                    // Too many dashes for the outline (or a pattern far below a device pixel): solid is indistinguishable.
                    const float zeroLength = 0.01f / std::max(scale, 1e-3f);

                    size_t produced = 0;
                    INT figureStart = 0;
                    while (figureStart < count)
                    {
                        INT figureEnd = figureStart + 1;
                        while (figureEnd < count && (m_flatTypes[figureEnd] & Gdiplus::PathPointTypePathTypeMask) != Gdiplus::PathPointTypeStart)
                        {
                            ++figureEnd;
                        }
                        const bool closed = (m_flatTypes[figureEnd - 1] & Gdiplus::PathPointTypeCloseSubpath) != 0;
                        m_figurePoints.assign(m_flatPoints.begin() + figureStart, m_flatPoints.begin() + figureEnd);
                        if (closed)
                        {
                            m_figurePoints.push_back(m_flatPoints[figureStart]);
                        }
                        figureStart = figureEnd;
                        if (m_figurePoints.size() < 2)
                        {
                            continue;
                        }

                        // Phase: position dashOffset into the pattern.
                        float phase = std::fmod(style.dashOffset, period);
                        if (phase < 0)
                        {
                            phase += period;
                        }
                        size_t index = 0;
                        for (size_t guard = 0; phase > 0 && phase >= m_dashPattern[index] && guard < 2 * m_dashPattern.size(); ++guard)
                        {
                            phase -= m_dashPattern[index];
                            index = (index + 1) % m_dashPattern.size();
                        }
                        float remaining = std::max(m_dashPattern[index] - phase, 0.0f);
                        bool on = index % 2 == 0;
                        bool dashAtStart = on;
                        m_dashPoints.clear();
                        if (on)
                        {
                            m_dashPoints.push_back(m_figurePoints[0]);
                        }

                        const auto emit = [&](bool atStart, bool atEnd, const Gdiplus::PointF& direction)
                        {
                            if (m_dashPoints.empty())
                            {
                                return;
                            }
                            if (m_dashPoints.size() == 1)
                            {
                                // Zero-length dash: a tiny segment along the outline so caps still draw (like D2D).
                                const Gdiplus::PointF p = m_dashPoints[0];
                                m_dashPoints.push_back(Gdiplus::PointF(p.X + direction.X * zeroLength, p.Y + direction.Y * zeroLength));
                            }
                            const uint32_t group = (closed ? 0u : (atStart ? 1u : 0u) | (atEnd ? 2u : 0u));
                            Gdiplus::GraphicsPath& target = *m_dashGroups[group];
                            Record(target.StartFigure());
                            Record(target.AddLines(m_dashPoints.data(), static_cast<INT>(m_dashPoints.size())));
                            m_dashPoints.clear();
                            ++produced;
                        };

                        Gdiplus::PointF direction(1, 0);
                        for (size_t segment = 0; segment + 1 < m_figurePoints.size() && produced < c_maxDashes; ++segment)
                        {
                            const Gdiplus::PointF a = m_figurePoints[segment], b = m_figurePoints[segment + 1];
                            const float length = std::hypot(b.X - a.X, b.Y - a.Y);
                            if (!(length > 0))
                            {
                                continue;
                            }
                            direction = Gdiplus::PointF((b.X - a.X) / length, (b.Y - a.Y) / length);
                            float position = 0;
                            while (produced < c_maxDashes)
                            {
                                const float step = std::min(remaining, length - position);
                                position += step;
                                remaining -= step;
                                const Gdiplus::PointF point(a.X + direction.X * position, a.Y + direction.Y * position);
                                const auto addPoint = [&](const Gdiplus::PointF& p)
                                {
                                    if (m_dashPoints.empty() || m_dashPoints.back().X != p.X || m_dashPoints.back().Y != p.Y)
                                    {
                                        m_dashPoints.push_back(p);
                                    }
                                };
                                if (remaining > 0)
                                {
                                    if (on)
                                    {
                                        addPoint(point); // segment end inside a dash: a join
                                    }
                                    break;
                                }
                                // Pattern element boundary at point.
                                if (on)
                                {
                                    addPoint(point);
                                    emit(dashAtStart, false, direction);
                                    dashAtStart = false;
                                }
                                index = (index + 1) % m_dashPattern.size();
                                remaining = m_dashPattern[index];
                                on = index % 2 == 0;
                                if (on)
                                {
                                    addPoint(point);
                                }
                                if (position >= length && remaining > 0)
                                {
                                    break;
                                }
                            }
                        }
                        if (on)
                        {
                            emit(dashAtStart, true, direction);
                        }
                    }
                    if (produced >= c_maxDashes)
                    {
                        return false;
                    }
                    return SUCCEEDED(m_frameHr);
                }

                bool EnsureScratchGraphics() noexcept
                {
                    if (!m_scratchMatrix)
                    {
                        m_scratchMatrix.reset(new Gdiplus::Matrix());
                        const HRESULT hr = CheckCreated(m_scratchMatrix);
                        if (FAILED(hr))
                        {
                            m_scratchMatrix.reset();
                            Fail(hr);
                            return false;
                        }
                        ++m_frameCreated;
                    }
                    if (m_shadowScratch.graphics)
                    {
                        return true;
                    }
                    m_shadowScratch.graphics.reset(new Gdiplus::Graphics(m_shadowScratch.bitmap.get()));
                    HRESULT hr = CheckCreated(m_shadowScratch.graphics);
                    if (SUCCEEDED(hr))
                    {
                        Gdiplus::Graphics& graphics = *m_shadowScratch.graphics;
                        hr = HResultFromStatus(graphics.SetPageUnit(Gdiplus::UnitPixel));
                        hr = SUCCEEDED(hr) ? HResultFromStatus(graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias)) : hr;
                        hr = SUCCEEDED(hr) ? HResultFromStatus(graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf)) : hr;
                        hr = SUCCEEDED(hr) ? HResultFromStatus(graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias)) : hr;
                        hr = SUCCEEDED(hr) ? HResultFromStatus(graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver)) : hr;
                    }
                    if (FAILED(hr))
                    {
                        m_shadowScratch.graphics.reset();
                        Fail(hr);
                        return false;
                    }
                    ++m_frameCreated;
                    return true;
                }

                // Builds m_ramp: c_rampSize premultiplied BGRA samples of the brush's stops over [0, 1], interpolated
                // with straight alpha (as D2D and Composition do) in sRGB or linear light, scaled by the brush opacity. false: draw nothing.
                bool PrepareRamp(const BrushDesc& brush) noexcept
                {
                    if (brush.stops == nullptr || brush.stopCount == 0 || !(brush.opacity > 0))
                    {
                        return false;
                    }
                    m_sortedStops.assign(brush.stops, brush.stops + brush.stopCount);
                    for (GradientStop& stop : m_sortedStops)
                    {
                        stop.offset = std::isfinite(stop.offset) ? std::clamp(stop.offset, 0.0f, 1.0f) : 0.0f;
                        stop.color.a = std::clamp(stop.color.a, 0.0f, 1.0f);
                    }
                    std::stable_sort(m_sortedStops.begin(), m_sortedStops.end(), [](const GradientStop& a, const GradientStop& b) { return a.offset < b.offset; });
                    const bool linear = brush.interpolation == GradientInterpolation::Linear;
                    const auto toLinear = [](float c) { return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f); };
                    const auto toSRgb = [](float c) { return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1 / 2.4f) - 0.055f; };
                    const float opacity = std::min(brush.opacity, 1.0f);
                    m_ramp.resize(c_rampSize);
                    size_t next = 0;
                    for (uint32_t i = 0; i < c_rampSize; ++i)
                    {
                        const float t = static_cast<float>(i) / (c_rampSize - 1);
                        while (next < m_sortedStops.size() && m_sortedStops[next].offset <= t)
                        {
                            ++next;
                        }
                        const GradientStop& s0 = m_sortedStops[next == 0 ? 0 : next - 1];
                        const GradientStop& s1 = m_sortedStops[next == m_sortedStops.size() ? m_sortedStops.size() - 1 : next];
                        const float span = s1.offset - s0.offset;
                        const float f = span > 0 ? std::clamp((t - s0.offset) / span, 0.0f, 1.0f) : (next == 0 ? 1.0f : 0.0f);
                        const float a = s0.color.a + (s1.color.a - s0.color.a) * f;
                        const auto channel = [&](float c0, float c1)
                        {
                            if (!linear)
                            {
                                return (c0 + (c1 - c0) * f) * a;
                            }
                            const float l = toLinear(c0) + (toLinear(c1) - toLinear(c0)) * f;
                            return toSRgb(std::clamp(l, 0.0f, 1.0f)) * a;
                        };
                        m_ramp[i] = (static_cast<uint32_t>(ToByte(a * opacity)) << 24) | (static_cast<uint32_t>(ToByte(channel(s0.color.r, s1.color.r) * opacity)) << 16) |
                            (static_cast<uint32_t>(ToByte(channel(s0.color.g, s1.color.g) * opacity)) << 8) | ToByte(channel(s0.color.b, s1.color.b) * opacity);
                    }
                    return true;
                }

                // Draws the primitive's coverage mask with drawMask (white, onto the scratch under the device transform
                // shifted to the area), shades it with the brush (m_ramp from PrepareRamp) and composites it 1:1.
                template <typename DrawMask>
                void ShadeBrush(const PixelRect& bounds, const BrushDesc& brush, DrawMask&& drawMask) noexcept
                {
                    const PixelRect area = Intersect(Intersect(bounds, CurrentClip()), m_damageBounds);
                    if (area.IsEmpty() || m_ramp.size() != c_rampSize)
                    {
                        return;
                    }
                    const uint32_t width = static_cast<uint32_t>(area.right - area.left), height = static_cast<uint32_t>(area.bottom - area.top);
                    if (static_cast<uint64_t>(width) * height > c_maxShadowPixels || !EnsureShadowScratch(width, height) || !EnsureScratchGraphics())
                    {
                        return;
                    }
                    const uint32_t stride = m_shadowScratchWidth;
                    uint32_t* pixels = reinterpret_cast<uint32_t*>(m_shadowScratch.pixels.data());
                    for (uint32_t y = 0; y < height; ++y)
                    {
                        memset(pixels + static_cast<size_t>(y) * stride, 0, static_cast<size_t>(width) * 4);
                    }
                    Gdiplus::Graphics& scratch = *m_shadowScratch.graphics;
                    Record(scratch.ResetTransform());
                    Record(scratch.SetClip(Gdiplus::Rect(0, 0, static_cast<INT>(width), static_cast<INT>(height)), Gdiplus::CombineModeReplace));
                    Gdiplus::REAL e[6];
                    m_transform->GetElements(e);
                    Record(m_scratchMatrix->SetElements(e[0], e[1], e[2], e[3], e[4] - area.left, e[5] - area.top));
                    Record(scratch.SetTransform(m_scratchMatrix.get()));
                    drawMask(scratch, Gdiplus::Color(255, 255, 255, 255));
                    scratch.Flush(Gdiplus::FlushIntentionSync);
                    if (FAILED(m_frameHr))
                    {
                        return;
                    }

                    // Device pixel -> brush space: inverse of (brush transform * user transform * DPI scale).
                    const Matrix3x2F& b = brush.transform;
                    const Matrix3x2F& u = m_userTransform;
                    const float s = m_dpiScale;
                    const float m11 = (b.m11 * u.m11 + b.m12 * u.m21) * s, m12 = (b.m11 * u.m12 + b.m12 * u.m22) * s;
                    const float m21 = (b.m21 * u.m11 + b.m22 * u.m21) * s, m22 = (b.m21 * u.m12 + b.m22 * u.m22) * s;
                    const float dx = (b.dx * u.m11 + b.dy * u.m21 + u.dx) * s, dy = (b.dx * u.m12 + b.dy * u.m22 + u.dy) * s;
                    const float det = m11 * m22 - m12 * m21;
                    if (!(std::abs(det) > 1e-12f) || !std::isfinite(det))
                    {
                        return;
                    }
                    const float i11 = m22 / det, i12 = -m12 / det, i21 = -m21 / det, i22 = m11 / det;
                    const float idx = -(dx * i11 + dy * i21), idy = -(dx * i12 + dy * i22);

                    // t(p): linear projection, or the focal radial solution on the unit circle (offset 0 at the origin,
                    // 1 on the ellipse): q = f + t (u - f) with |u| = 1.
                    const bool radial = brush.kind == BrushKind::Radial;
                    float ax = 0, ay = 0, a0 = 0;
                    float cx = 0, cy = 0, rx = 1, ry = 1, fx = 0, fy = 0, fa = 1;
                    bool degenerate = false;
                    if (!radial)
                    {
                        const float ddx = brush.end.x - brush.start.x, ddy = brush.end.y - brush.start.y;
                        const float length2 = ddx * ddx + ddy * ddy;
                        degenerate = !(length2 > 1e-12f) || !std::isfinite(length2);
                        if (!degenerate)
                        {
                            ax = ddx / length2;
                            ay = ddy / length2;
                            a0 = -(brush.start.x * ax + brush.start.y * ay);
                        }
                    }
                    else
                    {
                        rx = brush.radiusX;
                        ry = brush.radiusY;
                        degenerate = !(rx > 0) || !(ry > 0);
                        cx = brush.center.x;
                        cy = brush.center.y;
                        if (!degenerate)
                        {
                            fx = (brush.origin.x - cx) / rx;
                            fy = (brush.origin.y - cy) / ry;
                            const float f2 = fx * fx + fy * fy;
                            constexpr float c_maxFocal = 0.999f;
                            if (!(f2 < c_maxFocal * c_maxFocal))
                            {
                                const float k = std::isfinite(f2) && f2 > 0 ? c_maxFocal / std::sqrt(f2) : 0.0f;
                                fx *= k;
                                fy *= k;
                            }
                            fa = 1 - (fx * fx + fy * fy);
                        }
                    }
                    const GradientSpread spread = brush.spread;
                    const auto index = [&](float t) -> uint32_t
                    {
                        if (spread == GradientSpread::Repeat)
                        {
                            t -= std::floor(t);
                        }
                        else if (spread == GradientSpread::Reflect)
                        {
                            t = std::fmod(std::abs(t), 2.0f);
                            t = t > 1 ? 2 - t : t;
                        }
                        t = std::isfinite(t) ? std::clamp(t, 0.0f, 1.0f) : 1.0f;
                        return static_cast<uint32_t>(t * (c_rampSize - 1) + 0.5f);
                    };

                    // Half the pixel footprint in t for a repeating linear gradient (0: no seam filtering).
                    float seamHalf = 0;
                    if (!radial && !degenerate && spread == GradientSpread::Repeat)
                    {
                        seamHalf = (std::abs(i11 * ax + i12 * ay) + std::abs(i21 * ax + i22 * ay)) / 2;
                        seamHalf = std::isfinite(seamHalf) ? std::min(seamHalf, 0.5f) : 0.0f;
                    }

                    for (uint32_t y = 0; y < height; ++y)
                    {
                        uint32_t* row = pixels + static_cast<size_t>(y) * stride;
                        const float py = area.top + y + 0.5f;
                        for (uint32_t x = 0; x < width; ++x)
                        {
                            const uint32_t coverage = row[x] >> 24;
                            if (coverage == 0)
                            {
                                continue;
                            }
                            const float px = area.left + x + 0.5f;
                            const float bx = px * i11 + py * i21 + idx;
                            const float by = px * i12 + py * i22 + idy;
                            float t = 1;
                            if (!degenerate)
                            {
                                if (!radial)
                                {
                                    t = bx * ax + by * ay + a0;
                                }
                                else
                                {
                                    const float qx = (bx - cx) / rx - fx, qy = (by - cy) / ry - fy;
                                    const float fd = fx * qx + fy * qy;
                                    const float dd = qx * qx + qy * qy;
                                    t = (fd + std::sqrt(std::max(fd * fd + fa * dd, 0.0f))) / fa;
                                }
                            }
                            uint32_t color = m_ramp[index(t)];
                            if (seamHalf > 0)
                            {
                                // Repeat wraps from the last stop to the first: area-weight the pixel across the seam
                                // like D2D's filtered sampling instead of a hard step.
                                const float f = t - std::floor(t);
                                float weight = 0;
                                uint32_t other = 0;
                                if (f < seamHalf)
                                {
                                    weight = (seamHalf - f) / (2 * seamHalf);
                                    other = m_ramp.back();
                                }
                                else if (f > 1 - seamHalf)
                                {
                                    weight = (f + seamHalf - 1) / (2 * seamHalf);
                                    other = m_ramp.front();
                                }
                                if (weight > 0)
                                {
                                    const uint32_t w = static_cast<uint32_t>(std::clamp(weight, 0.0f, 1.0f) * 256);
                                    const auto mixChannel = [&](uint32_t shift)
                                    {
                                        return ((((color >> shift) & 0xFF) * (256 - w) + ((other >> shift) & 0xFF) * w + 128) >> 8) << shift;
                                    };
                                    color = mixChannel(24) | mixChannel(16) | mixChannel(8) | mixChannel(0);
                                }
                            }
                            if (coverage == 255)
                            {
                                row[x] = color;
                                continue;
                            }
                            const auto scaleChannel = [&](uint32_t shift) { return (((color >> shift) & 0xFF) * coverage + 127) / 255 << shift; };
                            row[x] = scaleChannel(24) | scaleChannel(16) | scaleChannel(8) | scaleChannel(0);
                        }
                    }

                    Gdiplus::Graphics& graphics = Current();
                    Record(graphics.ResetTransform());
                    Record(graphics.DrawImage(m_shadowScratch.bitmap.get(), Gdiplus::Rect(area.left, area.top, static_cast<INT>(width), static_cast<INT>(height)),
                        0, 0, static_cast<INT>(width), static_cast<INT>(height), Gdiplus::UnitPixel, nullptr));
                    graphics.Flush(Gdiplus::FlushIntentionSync);
                    Record(graphics.SetTransform(m_transform.get()));
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

                // Antialiased rounded clip: scales every premultiplied layer pixel in the bounds by the clip shape's
                // coverage. Under an axis-aligned transform (the common case) coverage is computed exactly; otherwise the
                // shape is filled (under the transform current at push) into the mask surface with GDI+ antialiasing,
                // which underestimates coverage on shallow arcs.
                bool ApplyRoundedMask(Surface& layer, const LayerRecord& record) noexcept
                {
                    const PixelRect& b = record.bounds;
                    const size_t stride = static_cast<size_t>(m_width) * 4;
                    auto scaleRows = [&](auto&& coverageAt)
                    {
                        for (int32_t y = b.top; y < b.bottom; ++y)
                        {
                            uint8_t* destination = layer.pixels.data() + y * stride + static_cast<size_t>(b.left) * 4;
                            for (int32_t x = b.left; x < b.right; ++x, destination += 4)
                            {
                                const uint32_t alpha = coverageAt(x, y);
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
                    };

                    const Matrix3x2F& t = record.maskTransform;
                    const float s = m_dpiScale;
                    if (t.m12 == 0 && t.m21 == 0)
                    {
                        const float x0 = (t.m11 * record.maskRect.left + t.dx) * s;
                        const float x1 = (t.m11 * record.maskRect.right + t.dx) * s;
                        const float y0 = (t.m22 * record.maskRect.top + t.dy) * s;
                        const float y1 = (t.m22 * record.maskRect.bottom + t.dy) * s;
                        const RectF device{ std::min(x0, x1), std::min(y0, y1), std::max(x0, x1), std::max(y0, y1) };
                        const float rx = std::min(std::max(record.radiusX, 0.0f) * std::abs(t.m11 * s), (device.right - device.left) / 2);
                        const float ry = std::min(std::max(record.radiusY, 0.0f) * std::abs(t.m22 * s), (device.bottom - device.top) / 2);
                        scaleRows([&](int32_t x, int32_t y) { return RoundedRectCoverage(device, rx, ry, x, y); });
                        return true;
                    }

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
                    Gdiplus::Graphics& mask = *m_mask.graphics;
                    Record(mask.ResetTransform());
                    Record(mask.SetClip(Gdiplus::Rect(b.left, b.top, b.right - b.left, b.bottom - b.top), Gdiplus::CombineModeReplace));
                    Record(mask.Clear(Gdiplus::Color(0, 0, 0, 0)));

                    // Borrow m_transform for the push-time transform, then restore the current one.
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
                    scaleRows([&](int32_t x, int32_t y) { return static_cast<uint32_t>(m_mask.pixels[y * stride + static_cast<size_t>(x) * 4 + 3]); });
                    return true;
                }

                // Coverage (0-255) of pixel (x, y) by a device-space rounded rect with elliptical corners: exact along
                // the straight edges, 16x16 supersampled where the pixel touches a corner box.
                static uint32_t RoundedRectCoverage(const RectF& rect, float rx, float ry, int32_t x, int32_t y) noexcept
                {
                    const float px = static_cast<float>(x), py = static_cast<float>(y);
                    const float cx = std::min(px + 1, rect.right) - std::max(px, rect.left);
                    const float cy = std::min(py + 1, rect.bottom) - std::max(py, rect.top);
                    if (!(cx > 0) || !(cy > 0))
                    {
                        return 0;
                    }
                    const bool nearCornerX = px < rect.left + rx || px + 1 > rect.right - rx;
                    const bool nearCornerY = py < rect.top + ry || py + 1 > rect.bottom - ry;
                    if (!(rx > 0) || !(ry > 0) || !nearCornerX || !nearCornerY)
                    {
                        return static_cast<uint32_t>(std::min(cx, 1.0f) * std::min(cy, 1.0f) * 255 + 0.5f);
                    }
                    constexpr int c_samples = 16;
                    uint32_t inside = 0;
                    for (int j = 0; j < c_samples; ++j)
                    {
                        const float sy = py + (j + 0.5f) / c_samples;
                        if (sy < rect.top || sy >= rect.bottom)
                        {
                            continue;
                        }
                        const float qy = std::clamp(sy, rect.top + ry, rect.bottom - ry);
                        const float ny = (sy - qy) / ry;
                        for (int i = 0; i < c_samples; ++i)
                        {
                            const float sx = px + (i + 0.5f) / c_samples;
                            if (sx < rect.left || sx >= rect.right)
                            {
                                continue;
                            }
                            const float qx = std::clamp(sx, rect.left + rx, rect.right - rx);
                            const float nx = (sx - qx) / rx;
                            inside += nx * nx + ny * ny <= 1 ? 1 : 0;
                        }
                    }
                    return (inside * 255 + c_samples * c_samples / 2) / (c_samples * c_samples);
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
                            hr = factory->GetSystemFontCollection(&m_systemFonts, FALSE);
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = factory->GetGdiInterop(&m_gdiInterop);
                        }
                        if (FAILED(hr))
                        {
                            m_systemFonts.Reset();
                            Fail(hr);
                            return nullptr;
                        }
                        // Optional (Windows 8.1+): color glyph layers.
                        static_cast<void>(factory.As(&m_dwriteFactory2));
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
                    Microsoft::WRL::ComPtr<IDWriteFontFace2> face2;
                    entry.isColor = SUCCEEDED(face->QueryInterface(IID_PPV_ARGS(&face2))) && face2->IsColorFont();

                    // GDI+ text only draws installed TrueType-outline fonts at their default instance; anything else is
                    // drawn from DirectWrite outlines.
                    const DWRITE_FONT_FACE_TYPE type = face->GetType();
                    bool outline = type != DWRITE_FONT_FACE_TYPE_TRUETYPE && type != DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION;
                    if (!outline)
                    {
                        // Collections report OPENTYPE_COLLECTION even for PostScript (CFF/CFF2) outlines.
                        for (const UINT32 tag : { DWRITE_MAKE_OPENTYPE_TAG('C', 'F', 'F', ' '), DWRITE_MAKE_OPENTYPE_TAG('C', 'F', 'F', '2') })
                        {
                            const void* data = nullptr;
                            UINT32 size = 0;
                            void* context = nullptr;
                            BOOL exists = FALSE;
                            if (SUCCEEDED(face->TryGetFontTable(tag, &data, &size, &context, &exists)) && exists)
                            {
                                face->ReleaseFontTable(context);
                                outline = true;
                            }
                        }
                    }
                    Microsoft::WRL::ComPtr<IDWriteFontFace5> face5;
                    if (!outline && SUCCEEDED(face->QueryInterface(IID_PPV_ARGS(&face5))) && face5->HasVariations())
                    {
                        outline = true;
                    }
                    Microsoft::WRL::ComPtr<IDWriteFont> systemFont;
                    if (!outline && FAILED(m_systemFonts->GetFontFromFontFace(face, &systemFont)))
                    {
                        outline = true; // private or in-memory font: GDI cannot map it by name
                    }
                    LOGFONTW logFont{};
                    if (!outline && SUCCEEDED(m_gdiInterop->ConvertFontFaceToLOGFONT(face, &logFont)))
                    {
                        // The GDI family name already selects weight-specific faces (for example "Segoe UI Semibold");
                        // GDI+ styles only add bold and italic.
                        entry.family = GetFontFamily(logFont.lfFaceName);
                        entry.style = (logFont.lfWeight >= FW_BOLD ? Gdiplus::FontStyleBold : 0) | (logFont.lfItalic ? Gdiplus::FontStyleItalic : 0);
                    }
                    entry.outline = outline || !entry.family;
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
                        // Not a font GDI+ can use (for example CFF outlines or a private collection): callers draw outlines.
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

                // Explicit glyph origins (m_glyphPositions) from DirectWrite's advances and offsets, and the extent of the
                // pen positions and baselines. Right-to-left runs advance leftwards from the origin, and a positive
                // advanceOffset moves a glyph in the run's direction (left for RTL), matching DWRITE_GLYPH_OFFSET.
                void LayoutGlyphs(const GlyphRunDesc& run, float& minX, float& maxX, float& minY, float& maxY) noexcept
                {
                    const uint32_t count = std::min<uint32_t>(run.glyphCount, INT_MAX);
                    const bool rightToLeft = (run.bidiLevel & 1) != 0;
                    m_glyphPositions.resize(count);
                    float pen = 0;
                    minX = INFINITY, maxX = -INFINITY, minY = INFINITY, maxY = -INFINITY;
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
                }

                // One monochrome run: GDI+ text (DrawDriverString with glyph indices) for fonts GDI maps faithfully,
                // otherwise cached DirectWrite outlines filled as paths.
                void DrawGlyphs(const GlyphRunDesc& run, const ColorF& color, bool outline, bool clearType) noexcept
                {
                    if (run.glyphIndices == nullptr || run.glyphAdvances == nullptr || run.glyphCount == 0)
                    {
                        return;
                    }
                    Gdiplus::SolidBrush* brush = GetBrush(color);
                    if (!brush)
                    {
                        return;
                    }
                    float minX, maxX, minY, maxY;
                    LayoutGlyphs(run, minX, maxX, minY, maxY);
                    const uint32_t count = static_cast<uint32_t>(m_glyphPositions.size());
                    Gdiplus::Graphics& graphics = Current();
                    if (!outline)
                    {
                        if (Gdiplus::Font* font = GetGlyphFont(run.fontFace, run.fontEmSize))
                        {
                            if (clearType)
                            {
                                Record(graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit));
                            }
                            Record(graphics.DrawDriverString(run.glyphIndices, static_cast<INT>(count), font, brush, m_glyphPositions.data(), 0, nullptr));
                            if (clearType)
                            {
                                Record(graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias));
                            }
                            return;
                        }
                    }
                    for (uint32_t index = 0; index < count; ++index)
                    {
                        Gdiplus::GraphicsPath* path = GetGlyphOutline(run.fontFace, run.glyphIndices[index], run.fontEmSize);
                        if (!path)
                        {
                            continue;
                        }
                        Record(graphics.TranslateTransform(m_glyphPositions[index].X, m_glyphPositions[index].Y, Gdiplus::MatrixOrderPrepend));
                        Record(graphics.FillPath(brush, path));
                        Record(graphics.SetTransform(m_transform.get()));
                    }
                }

                // Color font layers (COLR/CPAL); false when the run has no color glyphs (draw it monochrome).
                bool DrawColorLayers(const GlyphRunDesc& run, const ColorF& color, bool clearType) noexcept
                {
                    if (!m_dwriteFactory2)
                    {
                        return false;
                    }
                    static_assert(sizeof(GlyphOffset) == sizeof(DWRITE_GLYPH_OFFSET));
                    DWRITE_GLYPH_RUN glyphRun{};
                    glyphRun.fontFace = run.fontFace;
                    glyphRun.fontEmSize = run.fontEmSize;
                    glyphRun.glyphCount = run.glyphCount;
                    glyphRun.glyphIndices = run.glyphIndices;
                    glyphRun.glyphAdvances = run.glyphAdvances;
                    glyphRun.glyphOffsets = reinterpret_cast<const DWRITE_GLYPH_OFFSET*>(run.glyphOffsets);
                    glyphRun.bidiLevel = run.bidiLevel;
                    Microsoft::WRL::ComPtr<IDWriteColorGlyphRunEnumerator> layers;
                    if (FAILED(m_dwriteFactory2->TranslateColorGlyphRun(run.baselineOrigin.x, run.baselineOrigin.y, &glyphRun, nullptr,
                        DWRITE_MEASURING_MODE_NATURAL, nullptr, 0, &layers)))
                    {
                        return false; // DWRITE_E_NOCOLOR
                    }
                    BOOL hasRun = FALSE;
                    while (SUCCEEDED(layers->MoveNext(&hasRun)) && hasRun)
                    {
                        const DWRITE_COLOR_GLYPH_RUN* layer = nullptr;
                        if (FAILED(layers->GetCurrentRun(&layer)) || layer == nullptr || layer->glyphRun.fontFace == nullptr)
                        {
                            continue;
                        }
                        GlyphRunDesc layerRun{};
                        layerRun.fontFace = layer->glyphRun.fontFace;
                        layerRun.fontEmSize = layer->glyphRun.fontEmSize;
                        layerRun.glyphIndices = layer->glyphRun.glyphIndices;
                        layerRun.glyphAdvances = layer->glyphRun.glyphAdvances;
                        layerRun.glyphOffsets = reinterpret_cast<const GlyphOffset*>(layer->glyphRun.glyphOffsets);
                        layerRun.glyphCount = layer->glyphRun.glyphCount;
                        layerRun.bidiLevel = layer->glyphRun.bidiLevel;
                        layerRun.baselineOrigin = { layer->baselineOriginX, layer->baselineOriginY };
                        const ColorF layerColor = layer->paletteIndex == 0xFFFF ? color :
                            ColorF{ layer->runColor.r, layer->runColor.g, layer->runColor.b, layer->runColor.a };
                        const FontFaceEntry* entry = GetFontFace(layerRun.fontFace);
                        if (entry)
                        {
                            DrawGlyphs(layerRun, layerColor, entry->outline, clearType);
                        }
                    }
                    return true;
                }

                // Outline of one glyph at the origin (DIPs, y down), cached per (face, glyph, em size); nullptr for blank
                // glyphs and failures.
                Gdiplus::GraphicsPath* GetGlyphOutline(IDWriteFontFace* face, uint16_t glyph, float emSize) noexcept
                {
                    const GlyphOutlineKey key{ face, glyph, FloatBits(emSize) };
                    const auto found = m_glyphOutlines.find(key);
                    if (found != m_glyphOutlines.end())
                    {
                        m_glyphOutlineLru.splice(m_glyphOutlineLru.begin(), m_glyphOutlineLru, found->second.lru);
                        return found->second.path->GetPointCount() > 0 ? found->second.path.get() : nullptr;
                    }
                    std::unique_ptr<Gdiplus::GraphicsPath> path(new Gdiplus::GraphicsPath(Gdiplus::FillModeWinding));
                    if (FAILED(CheckCreated(path)))
                    {
                        return nullptr;
                    }
                    GraphicsPathSink sink(*path);
                    if (FAILED(face->GetGlyphRunOutline(emSize, &glyph, nullptr, nullptr, 1, FALSE, FALSE, &sink)) || FAILED(sink.Close()))
                    {
                        return nullptr;
                    }
                    const uint64_t bytes = c_glyphOutlineBytes + static_cast<uint64_t>(path->GetPointCount()) * c_pathPointBytes;
                    while (!m_glyphOutlines.empty() && (m_glyphOutlines.size() >= c_maxGlyphOutlines || m_glyphOutlineBytes + bytes > c_maxGlyphOutlineBytes))
                    {
                        const auto victim = m_glyphOutlines.find(m_glyphOutlineLru.back());
                        m_glyphOutlineBytes -= victim->second.bytes;
                        m_glyphOutlines.erase(victim);
                        m_glyphOutlineLru.pop_back();
                        ++m_frameDestroyed;
                    }
                    m_glyphOutlineLru.push_front(key);
                    GlyphOutlineEntry& entry = m_glyphOutlines[key];
                    entry.face = face;
                    entry.path = std::move(path);
                    entry.bytes = bytes;
                    entry.lru = m_glyphOutlineLru.begin();
                    m_glyphOutlineBytes += bytes;
                    ++m_frameCreated;
                    return entry.path->GetPointCount() > 0 ? entry.path.get() : nullptr;
                }

                // True when every pixel of bounds (within the current clip) is opaque on the current surface, so ClearType's
                // per-channel coverage can be blended without an alpha channel.
                bool IsOpaqueUnder(const PixelRect& bounds) noexcept
                {
                    const PixelRect area = Intersect(Intersect(bounds, CurrentClip()), FullTarget());
                    if (area.IsEmpty())
                    {
                        return false;
                    }
                    Surface& surface = CurrentSurface();
                    surface.graphics->Flush(Gdiplus::FlushIntentionSync);
                    const size_t stride = static_cast<size_t>(m_width) * 4;
                    for (int32_t y = area.top; y < area.bottom; ++y)
                    {
                        const uint8_t* row = surface.pixels.data() + y * stride + 3;
                        for (int32_t x = area.left; x < area.right; ++x)
                        {
                            if (row[static_cast<size_t>(x) * 4] != 255)
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                }

                bool GetMaskSpace(MaskSpace& space) const noexcept
                {
                    const Matrix3x2F& t = m_userTransform;
                    const float s = m_dpiScale;
                    space = {};
                    if (t.m12 == 0 && t.m21 == 0 && t.m11 > 0 && t.m22 > 0)
                    {
                        space.axisAligned = true;
                        space.ax = t.m11 * s;
                        space.ay = t.m22 * s;
                        space.bx = t.dx * s;
                        space.by = t.dy * s;
                    }
                    else
                    {
                        space.ax = std::hypot(t.m11, t.m12) * s;
                        space.ay = std::hypot(t.m21, t.m22) * s;
                    }
                    return space.ax > 0 && space.ay > 0 && std::isfinite(space.ax) && std::isfinite(space.ay) && std::isfinite(space.bx) && std::isfinite(space.by);
                }

                // Blurred coverage of a rounded rect (shape in mask pixels), cached in a bounded LRU.
                const ShadowMask* GetShadowMask(const ShadowKey& key, uint32_t width, uint32_t height, const RectF& shape, float rx, float ry,
                    float sigmaX, float sigmaY) noexcept
                {
                    const auto found = m_shadowMasks.find(key);
                    if (found != m_shadowMasks.end())
                    {
                        m_shadowLru.splice(m_shadowLru.begin(), m_shadowLru, found->second.lru);
                        return &found->second;
                    }
                    ShadowMask mask{};
                    mask.width = width;
                    mask.height = height;
                    mask.alpha.assign(static_cast<size_t>(width) * height, 0);
                    const int32_t top = static_cast<int32_t>(std::floor(shape.top));
                    const int32_t bottom = std::min(static_cast<int32_t>(std::ceil(shape.bottom)), static_cast<int32_t>(height));
                    const int32_t left = static_cast<int32_t>(std::floor(shape.left));
                    const int32_t right = std::min(static_cast<int32_t>(std::ceil(shape.right)), static_cast<int32_t>(width));
                    for (int32_t y = std::max(top, 0); y < bottom; ++y)
                    {
                        uint8_t* row = mask.alpha.data() + static_cast<size_t>(y) * width;
                        for (int32_t x = std::max(left, 0); x < right; ++x)
                        {
                            row[x] = static_cast<uint8_t>(RoundedRectCoverage(shape, rx, ry, x, y));
                        }
                    }
                    GaussianBlur(mask.alpha.data(), static_cast<int32_t>(width), static_cast<int32_t>(height), static_cast<ptrdiff_t>(width), 1,
                        sigmaX, sigmaY, m_blurLine);

                    const uint64_t bytes = mask.alpha.size();
                    while (!m_shadowMasks.empty() && (m_shadowMasks.size() >= c_maxShadowMasks || m_shadowMaskBytes + bytes > c_maxShadowMaskBytes))
                    {
                        const auto victim = m_shadowMasks.find(m_shadowLru.back());
                        m_shadowMaskBytes -= victim->second.alpha.size();
                        m_shadowMasks.erase(victim);
                        m_shadowLru.pop_back();
                        ++m_frameDestroyed;
                    }
                    m_shadowLru.push_front(key);
                    ShadowMask& entry = m_shadowMasks[key];
                    entry = std::move(mask);
                    entry.lru = m_shadowLru.begin();
                    m_shadowMaskBytes += bytes;
                    ++m_frameCreated;
                    return &entry;
                }

                // PARGB scratch the colored shadow is composited from; grows to the largest shadow drawn.
                bool EnsureShadowScratch(uint32_t width, uint32_t height) noexcept
                {
                    if (m_shadowScratch.bitmap && width <= m_shadowScratchWidth && height <= m_shadowScratchHeight)
                    {
                        return true;
                    }
                    const uint32_t newWidth = std::max(width, m_shadowScratchWidth);
                    const uint32_t newHeight = std::max(height, m_shadowScratchHeight);
                    if (m_shadowScratch.bitmap)
                    {
                        ++m_frameDestroyed;
                    }
                    ReleaseSurface(m_shadowScratch);
                    m_shadowScratchWidth = m_shadowScratchHeight = 0;
                    m_shadowScratch.pixels.assign(static_cast<size_t>(newWidth) * newHeight * 4, 0);
                    m_shadowScratch.bitmap.reset(new Gdiplus::Bitmap(static_cast<INT>(newWidth), static_cast<INT>(newHeight), static_cast<INT>(newWidth * 4),
                        PixelFormat32bppPARGB, m_shadowScratch.pixels.data()));
                    const HRESULT hr = CheckCreated(m_shadowScratch.bitmap);
                    if (FAILED(hr))
                    {
                        ReleaseSurface(m_shadowScratch);
                        Fail(hr);
                        return false;
                    }
                    ++m_frameCreated;
                    m_shadowScratchWidth = newWidth;
                    m_shadowScratchHeight = newHeight;
                    return true;
                }

                // Scratch buffers sized by an unusually large shadow or blur are not kept between frames.
                void ReleaseLargeScratch() noexcept
                {
                    if (m_shadowScratch.pixels.size() > c_releaseScratchBytes)
                    {
                        ReleaseSurface(m_shadowScratch);
                        m_shadowScratchWidth = m_shadowScratchHeight = 0;
                        ++m_frameDestroyed;
                    }
                    if (m_blurPixels.capacity() > c_releaseScratchBytes)
                    {
                        m_blurPixels.clear();
                        m_blurPixels.shrink_to_fit();
                    }
                }

                // Linear gradient brush that behaves like clamp mode over bounds (DIPs): the brush axis is extended to
                // cover bounds' projection and padded with the end colors, so GDI+'s wrapping never shows inside the
                // shape. nullptr draws nothing; degenerate gradients use a solid brush.
                Gdiplus::Brush* GetGradientBrush(const LinearGradientDesc& gradient, const RectF& bounds) noexcept
                {
                    if (gradient.stops == nullptr || gradient.stopCount == 0 || !(gradient.opacity > 0))
                    {
                        return nullptr;
                    }
                    const float opacity = std::min(gradient.opacity, 1.0f);
                    m_sortedStops.assign(gradient.stops, gradient.stops + gradient.stopCount);
                    for (GradientStop& stop : m_sortedStops)
                    {
                        stop.offset = std::isfinite(stop.offset) ? std::clamp(stop.offset, 0.0f, 1.0f) : 0.0f;
                    }
                    std::stable_sort(m_sortedStops.begin(), m_sortedStops.end(), [](const GradientStop& a, const GradientStop& b) { return a.offset < b.offset; });

                    const PointF start = gradient.start;
                    const float dx = gradient.end.x - start.x;
                    const float dy = gradient.end.y - start.y;
                    const float length2 = dx * dx + dy * dy;
                    if (m_sortedStops.size() == 1 || !(length2 > 1e-12f) || !std::isfinite(length2) || !std::isfinite(start.x) || !std::isfinite(start.y))
                    {
                        ColorF solid = m_sortedStops.back().color;
                        solid.a *= opacity;
                        return GetBrush(solid);
                    }

                    // Extend the axis over the shape's projection (plus 2 DIPs), quantized to 1/8 of the gradient length.
                    float tMin = 0, tMax = 1;
                    if (std::isfinite(bounds.left) && std::isfinite(bounds.top) && std::isfinite(bounds.right) && std::isfinite(bounds.bottom))
                    {
                        const float margin = 2 / std::sqrt(length2);
                        for (float x : { bounds.left, bounds.right })
                        {
                            for (float y : { bounds.top, bounds.bottom })
                            {
                                const float t = ((x - start.x) * dx + (y - start.y) * dy) / length2;
                                tMin = std::min(tMin, t - margin);
                                tMax = std::max(tMax, t + margin);
                            }
                        }
                    }
                    tMin = std::max(std::floor(tMin * 8) / 8, -4096.0f);
                    tMax = std::min(std::ceil(tMax * 8) / 8, 4096.0f);
                    if (tMin >= 0)
                    {
                        tMin = -0.125f;
                    }
                    if (tMax <= 1)
                    {
                        tMax = 1.125f;
                    }
                    const float span = tMax - tMin;

                    m_gradientColors.clear();
                    m_gradientPositions.clear();
                    const auto add = [&](float position, const ColorF& color)
                    {
                        ColorF scaled = color;
                        scaled.a = std::clamp(scaled.a, 0.0f, 1.0f) * opacity;
                        if (!m_gradientPositions.empty())
                        {
                            // GDI+ wants increasing positions; hard stops become (nearly) coincident ones.
                            position = std::max(position, m_gradientPositions.back() + 1e-6f);
                        }
                        m_gradientPositions.push_back(position);
                        m_gradientColors.push_back(ToGdiColor(scaled));
                    };
                    add(0, m_sortedStops.front().color);
                    for (size_t index = 0; index < m_sortedStops.size(); ++index)
                    {
                        const GradientStop& stop = m_sortedStops[index];
                        if (index > 0)
                        {
                            // GDI+ interpolates straight colors; approximate premultiplied interpolation where alpha changes.
                            const GradientStop& previous = m_sortedStops[index - 1];
                            const float a0 = std::clamp(previous.color.a, 0.0f, 1.0f), a1 = std::clamp(stop.color.a, 0.0f, 1.0f);
                            if (a0 != a1 && stop.offset > previous.offset)
                            {
                                constexpr int c_steps = 8;
                                for (int step = 1; step < c_steps; ++step)
                                {
                                    const float f = static_cast<float>(step) / c_steps;
                                    const float a = a0 + (a1 - a0) * f;
                                    const auto channel = [&](float c0, float c1) { return a > 0 ? (c0 * a0 + (c1 * a1 - c0 * a0) * f) / a : 0.0f; };
                                    const ColorF mixed{ channel(previous.color.r, stop.color.r), channel(previous.color.g, stop.color.g),
                                        channel(previous.color.b, stop.color.b), a };
                                    add((previous.offset + (stop.offset - previous.offset) * f - tMin) / span, mixed);
                                }
                            }
                        }
                        add((stop.offset - tMin) / span, stop.color);
                    }
                    add(1, m_sortedStops.back().color);
                    m_gradientPositions.back() = 1;
                    for (size_t index = m_gradientPositions.size() - 1; index-- > 0;)
                    {
                        m_gradientPositions[index] = std::min(m_gradientPositions[index], m_gradientPositions[index + 1] - 1e-6f);
                    }

                    const Gdiplus::PointF p0(start.x + dx * tMin, start.y + dy * tMin);
                    const Gdiplus::PointF p1(start.x + dx * tMax, start.y + dy * tMax);
                    std::vector<uint32_t>& key = m_gradientKey;
                    key.assign({ FloatBits(p0.X), FloatBits(p0.Y), FloatBits(p1.X), FloatBits(p1.Y) });
                    for (size_t index = 0; index < m_gradientColors.size(); ++index)
                    {
                        key.push_back(m_gradientColors[index].GetValue());
                        key.push_back(FloatBits(m_gradientPositions[index]));
                    }
                    const auto found = m_gradientBrushes.find(key);
                    if (found != m_gradientBrushes.end())
                    {
                        m_gradientLru.splice(m_gradientLru.begin(), m_gradientLru, found->second.lru);
                        return found->second.brush.get();
                    }

                    std::unique_ptr<Gdiplus::LinearGradientBrush> brush(new Gdiplus::LinearGradientBrush(p0, p1, m_gradientColors.front(), m_gradientColors.back()));
                    HRESULT hr = CheckCreated(brush);
                    if (SUCCEEDED(hr))
                    {
                        hr = HResultFromStatus(brush->SetInterpolationColors(m_gradientColors.data(), m_gradientPositions.data(), static_cast<INT>(m_gradientColors.size())));
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = HResultFromStatus(brush->SetWrapMode(Gdiplus::WrapModeTileFlipX));
                    }
                    if (FAILED(hr))
                    {
                        Fail(hr);
                        return nullptr;
                    }
                    while (m_gradientBrushes.size() >= c_maxGradientBrushes)
                    {
                        const auto victim = m_gradientBrushes.find(m_gradientLru.back());
                        m_gradientBrushBytes -= victim->second.bytes;
                        m_gradientBrushes.erase(victim);
                        m_gradientLru.pop_back();
                        ++m_frameDestroyed;
                    }
                    m_gradientLru.push_front(key);
                    GradientBrushEntry& entry = m_gradientBrushes[key];
                    entry.brush = std::move(brush);
                    entry.bytes = c_gradientBrushBytes + m_gradientColors.size() * c_gradientStopBytes;
                    entry.lru = m_gradientLru.begin();
                    m_gradientBrushBytes += entry.bytes;
                    ++m_frameCreated;
                    return entry.brush.get();
                }

                // Premultiplied BGRA of the gradient (m_sortedStops, as left by GetGradientBrush) at axis parameter t,
                // interpolated in premultiplied space and clamped at the ends, scaled by opacity * coverage / 255.
                uint32_t GradientPixel(float t, float opacity, uint32_t coverage) const noexcept
                {
                    const auto& stops = m_sortedStops;
                    size_t index = 0;
                    while (index < stops.size() && stops[index].offset <= t)
                    {
                        ++index;
                    }
                    const GradientStop& s0 = stops[index == 0 ? 0 : index - 1];
                    const GradientStop& s1 = stops[index == stops.size() ? stops.size() - 1 : index];
                    const float span = s1.offset - s0.offset;
                    const float f = span > 0 ? std::clamp((t - s0.offset) / span, 0.0f, 1.0f) : (index == 0 ? 1.0f : 0.0f);
                    const float a0 = std::clamp(s0.color.a, 0.0f, 1.0f), a1 = std::clamp(s1.color.a, 0.0f, 1.0f);
                    const float scale = opacity * coverage / 255;
                    const auto channel = [&](float c0, float c1) { return (c0 * a0 + (c1 * a1 - c0 * a0) * f) * scale; };
                    return (static_cast<uint32_t>(ToByte((a0 + (a1 - a0) * f) * scale)) << 24) |
                        (static_cast<uint32_t>(ToByte(channel(s0.color.r, s1.color.r))) << 16) |
                        (static_cast<uint32_t>(ToByte(channel(s0.color.g, s1.color.g))) << 8) | ToByte(channel(s0.color.b, s1.color.b));
                }

                // Rounded-rect gradient fill under an axis-aligned transform with exact corner coverage (GDI+ antialiasing
                // underestimates curved edges): the straight-edged cross goes through the gradient brush, with its inner
                // edges on device pixel boundaries; the four corner blocks are computed in software and copied 1:1.
                bool FillRoundedGradientExact(const RectF& rect, float radiusX, float radiusY, const LinearGradientDesc& gradient,
                    Gdiplus::Brush& brush, const MaskSpace& space) noexcept
                {
                    constexpr int64_t c_maxCornerPixels = 1 << 18;
                    const RectF d = space.Map(rect);
                    const float width = d.right - d.left, height = d.bottom - d.top;
                    if (!(width > 0) || !(height > 0) || !std::isfinite(width) || !std::isfinite(height) || !std::isfinite(d.left) || !std::isfinite(d.top) ||
                        std::abs(d.left) > 1e6f || std::abs(d.top) > 1e6f || std::abs(d.right) > 1e6f || std::abs(d.bottom) > 1e6f || m_sortedStops.empty())
                    {
                        return false;
                    }
                    const float rx = std::min(radiusX * space.ax, width / 2);
                    const float ry = std::min(radiusY * space.ay, height / 2);
                    const int32_t l = static_cast<int32_t>(std::floor(d.left)), t = static_cast<int32_t>(std::floor(d.top));
                    const int32_t r = static_cast<int32_t>(std::ceil(d.right)), b = static_cast<int32_t>(std::ceil(d.bottom));
                    int32_t xl = static_cast<int32_t>(std::ceil(d.left + rx)), xr = static_cast<int32_t>(std::floor(d.right - rx));
                    int32_t yt = static_cast<int32_t>(std::ceil(d.top + ry)), yb = static_cast<int32_t>(std::floor(d.bottom - ry));
                    if (xl >= xr)
                    {
                        xl = xr = r;
                    }
                    if (yt >= yb)
                    {
                        yt = yb = b;
                    }
                    const int32_t cornerWidth = std::max(xl - l, r - xr), cornerHeight = std::max(yt - t, b - yb);
                    if (static_cast<int64_t>(cornerWidth) * cornerHeight > c_maxCornerPixels ||
                        !EnsureShadowScratch(static_cast<uint32_t>(cornerWidth) + 1, static_cast<uint32_t>(cornerHeight) + 1))
                    {
                        return false;
                    }

                    Gdiplus::Graphics& graphics = Current();
                    const auto toDipX = [&](float x) { return (x - space.bx) / space.ax; };
                    const auto toDipY = [&](float y) { return (y - space.by) / space.ay; };
                    Gdiplus::GraphicsPath& path = *m_path;
                    Record(path.Reset());
                    const auto addRect = [&](float x0, float y0, float x1, float y1)
                    {
                        x0 = std::max(x0, d.left);
                        y0 = std::max(y0, d.top);
                        x1 = std::min(x1, d.right);
                        y1 = std::min(y1, d.bottom);
                        if (x1 > x0 && y1 > y0)
                        {
                            Record(path.AddRectangle(Gdiplus::RectF(toDipX(x0), toDipY(y0), toDipX(x1) - toDipX(x0), toDipY(y1) - toDipY(y0))));
                        }
                    };
                    addRect(d.left, static_cast<float>(yt), d.right, static_cast<float>(yb));
                    addRect(static_cast<float>(xl), d.top, static_cast<float>(xr), static_cast<float>(yt));
                    addRect(static_cast<float>(xl), static_cast<float>(yb), static_cast<float>(xr), d.bottom);
                    if (path.GetPointCount() > 0)
                    {
                        Record(graphics.FillPath(&brush, &path));
                    }

                    const float dx = gradient.end.x - gradient.start.x, dy = gradient.end.y - gradient.start.y;
                    const float length2 = dx * dx + dy * dy;
                    const bool degenerate = m_sortedStops.size() == 1 || !(length2 > 1e-12f) || !std::isfinite(length2);
                    const float opacity = std::min(gradient.opacity, 1.0f);
                    const uint32_t stride = m_shadowScratchWidth;
                    uint32_t* pixels = reinterpret_cast<uint32_t*>(m_shadowScratch.pixels.data());
                    const auto corner = [&](int32_t x0, int32_t y0, int32_t x1, int32_t y1)
                    {
                        if (x1 <= x0 || y1 <= y0)
                        {
                            return;
                        }
                        const uint32_t w = static_cast<uint32_t>(x1 - x0), h = static_cast<uint32_t>(y1 - y0);
                        for (uint32_t y = 0; y < h; ++y)
                        {
                            uint32_t* row = pixels + static_cast<size_t>(y) * stride;
                            const float py = toDipY(y0 + y + 0.5f) - gradient.start.y;
                            for (uint32_t x = 0; x < w; ++x)
                            {
                                const uint32_t coverage = RoundedRectCoverage(d, rx, ry, x0 + static_cast<int32_t>(x), y0 + static_cast<int32_t>(y));
                                if (coverage == 0)
                                {
                                    row[x] = 0;
                                    continue;
                                }
                                const float px = toDipX(x0 + x + 0.5f) - gradient.start.x;
                                const float along = degenerate ? 1.0f : (px * dx + py * dy) / length2;
                                row[x] = GradientPixel(along, opacity, coverage);
                            }
                            row[w] = 0;
                        }
                        memset(pixels + static_cast<size_t>(h) * stride, 0, static_cast<size_t>(w + 1) * 4);
                        Record(graphics.DrawImage(m_shadowScratch.bitmap.get(), Gdiplus::Rect(x0, y0, static_cast<INT>(w), static_cast<INT>(h)),
                            0, 0, static_cast<INT>(w), static_cast<INT>(h), Gdiplus::UnitPixel, nullptr));
                        graphics.Flush(Gdiplus::FlushIntentionSync);
                    };
                    Record(graphics.ResetTransform());
                    corner(l, t, xl, yt);
                    corner(xr, t, r, yt);
                    corner(l, yb, xl, b);
                    corner(xr, yb, r, b);
                    Record(graphics.SetTransform(m_transform.get()));
                    return true;
                }

                Surface& CurrentSurface() noexcept
                {
                    const size_t depth = m_layerStack.size();
                    if (depth > 0 && m_layerStack.back().active)
                    {
                        return *m_layers[depth - 1];
                    }
                    return m_target;
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
                    if (m_redirect)
                    {
                        return *m_redirect;
                    }
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

                // S11-S23 primitives.
                Microsoft::WRL::ComPtr<IDWriteFactory2> m_dwriteFactory2; // color glyph layers; null before Windows 8.1
                Microsoft::WRL::ComPtr<IDWriteFontCollection> m_systemFonts;
                std::map<GlyphOutlineKey, GlyphOutlineEntry> m_glyphOutlines;
                std::list<GlyphOutlineKey> m_glyphOutlineLru;
                uint64_t m_glyphOutlineBytes = 0;
                std::vector<GradientStop> m_sortedStops;
                std::vector<Gdiplus::Color> m_gradientColors;
                std::vector<Gdiplus::REAL> m_gradientPositions;
                std::vector<uint32_t> m_gradientKey;
                std::map<std::vector<uint32_t>, GradientBrushEntry> m_gradientBrushes;
                std::list<std::vector<uint32_t>> m_gradientLru;
                uint64_t m_gradientBrushBytes = 0;
                std::map<ShadowKey, ShadowMask> m_shadowMasks;
                std::list<ShadowKey> m_shadowLru;
                uint64_t m_shadowMaskBytes = 0;
                Surface m_shadowScratch; // bitmap only (no Graphics)
                uint32_t m_shadowScratchWidth = 0, m_shadowScratchHeight = 0;
                std::vector<uint8_t> m_blurPixels;
                std::vector<uint32_t> m_blurLine;

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

                // ---- shapes ----
                Gdiplus::Graphics* m_redirect = nullptr;              // Current() while drawing a gradient glyph mask
                std::unique_ptr<Gdiplus::Matrix> m_scratchMatrix;     // device transform shifted to the mask area
                std::unique_ptr<Gdiplus::Pen> m_stylePen;             // reconfigured per styled stroke
                std::unique_ptr<Gdiplus::GraphicsPath> m_outlinePath; // rect / rounded rect / ellipse stroke outline
                std::unique_ptr<Gdiplus::GraphicsPath> m_flatPath;    // flattened copy walked by the dasher
                std::unique_ptr<Gdiplus::GraphicsPath> m_dashGroups[4];
                std::vector<Gdiplus::PointF> m_flatPoints;
                std::vector<BYTE> m_flatTypes;
                std::vector<Gdiplus::PointF> m_joinFigure;
                std::vector<Gdiplus::PointF> m_figurePoints;
                std::vector<Gdiplus::PointF> m_dashPoints;
                std::vector<float> m_dashPattern;
                std::vector<uint32_t> m_ramp;
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
