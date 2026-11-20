// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImmediateRenderer.h"
#include <GdiPlusPainter.h>
#include <RenderingResourceDiagnostics.h>
#include <XamlProfilerTracing.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <RootScale.h>
#include "XamlIslandRoot.h"
#include "TextBlock.h"
#include "SolidColorBrush.h"
#include "shape.h"
#include "rectangle.h"
#include "RectangleGeometry.h"
#include "Transform.h"
#include "RichTextBlock.h"
#include "ListViewBaseItemChrome.h"
#include <ImmediateTextSink.h>
#include <PixelFormat.h>
#include <XamlCompositionBrush.h>
#include <FxCallbacks.h>
#include <dwrite.h>
#include <algorithm>
#include <cmath>
#include "ImmediateRenderer.Independent.h"

using namespace RenderingBackend;
using Microsoft::WRL::ComPtr;

namespace
{
    constexpr uint32_t c_maxDamageRects = 32;
    constexpr uint32_t c_maxRawDamageRects = 256;

    bool IsEmptyRect(const RectF& r) noexcept
    {
        return !(r.right > r.left && r.bottom > r.top);
    }

    RectF IntersectRect(const RectF& a, const RectF& b) noexcept
    {
        return { std::max(a.left, b.left), std::max(a.top, b.top), std::min(a.right, b.right), std::min(a.bottom, b.bottom) };
    }

    RectF UnionRect(const RectF& a, const RectF& b) noexcept
    {
        if (IsEmptyRect(a)) return b;
        if (IsEmptyRect(b)) return a;
        return { std::min(a.left, b.left), std::min(a.top, b.top), std::max(a.right, b.right), std::max(a.bottom, b.bottom) };
    }

    bool RectsIntersect(const RectF& a, const RectF& b) noexcept
    {
        return a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;
    }

    bool RectsEqual(const RectF& a, const RectF& b) noexcept
    {
        return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
    }

    RectF TransformRectBounds(const CMILMatrix& m, const RectF& r) noexcept
    {
        XPOINTF points[4] = { { r.left, r.top }, { r.right, r.top }, { r.left, r.bottom }, { r.right, r.bottom } };
        m.Transform(points, points, 4);
        RectF out{ points[0].x, points[0].y, points[0].x, points[0].y };
        for (const auto& p : points)
        {
            out.left = std::min(out.left, p.x);
            out.top = std::min(out.top, p.y);
            out.right = std::max(out.right, p.x);
            out.bottom = std::max(out.bottom, p.y);
        }
        return out;
    }

    RectF FromXRect(const XRECTF& r) noexcept
    {
        return { r.X, r.Y, r.X + r.Width, r.Y + r.Height };
    }

    Matrix3x2F ToMatrix(const CMILMatrix& m) noexcept
    {
        return { m.GetM11(), m.GetM12(), m.GetM21(), m.GetM22(), m.GetDx(), m.GetDy() };
    }

    // Prototype diagnostics: WINUI_IMMEDIATE_CAPTURE=<path.bmp> writes the most recently presented swap-chain buffer
    // after every paint. Screen capture (BitBlt/Graphics.Capture) cannot see app content in a disconnected session.
    const std::wstring& GetCapturePath()
    {
        static const std::wstring s_path = []
        {
            wchar_t buffer[MAX_PATH] = {};
            const DWORD length = GetEnvironmentVariableW(L"WINUI_IMMEDIATE_CAPTURE", buffer, ARRAYSIZE(buffer));
            return (length > 0 && length < ARRAYSIZE(buffer)) ? std::wstring(buffer, length) : std::wstring();
        }();
        return s_path;
    }

    // Painters copy only the frame's damage (plus scrolled areas) into the flip swap chain's back buffer and present
    // with dirty rects; DXGI carries the previous frame forward outside them (verified with mid-scroll captures of both
    // painters). WINUI_IMMEDIATE_PRESENT_UNION=1 also repaints the previous frame's damage, for presenters that don't.
    bool UnionPresentedDamage()
    {
        static const bool s_union = GetEnvironmentVariableW(L"WINUI_IMMEDIATE_PRESENT_UNION", nullptr, 0) != 0;
        return s_union;
    }

    // Diagnostic: WINUI_IMMEDIATE_FULL_WALK=1 walks the whole tree every frame (no subtree reuse).
    bool DisableIncrementalWalk()
    {
        static const bool s_fullWalk = GetEnvironmentVariableW(L"WINUI_IMMEDIATE_FULL_WALK", nullptr, 0) != 0;
        return s_fullWalk;
    }

    // Prototype diagnostics: WINUI_IMMEDIATE_STATS=<path.txt> appends one line per painted island frame.
    FILE* GetStatsFile()
    {
        static FILE* s_file = []() -> FILE*
        {
            wchar_t buffer[MAX_PATH] = {};
            const DWORD length = GetEnvironmentVariableW(L"WINUI_IMMEDIATE_STATS", buffer, ARRAYSIZE(buffer));
            FILE* file = nullptr;
            if (length > 0 && length < ARRAYSIZE(buffer) && _wfopen_s(&file, buffer, L"w") == 0)
            {
                fprintf(file, "frame island elements visited painted culled commands blits blitdx blitdy damagerects damagepx full walkus paintus reused\n");
                return file;
            }
            return nullptr;
        }();
        return s_file;
    }

    int64_t QpcMicroseconds() noexcept
    {
        static const int64_t s_frequency = []
        {
            LARGE_INTEGER f{};
            QueryPerformanceFrequency(&f);
            return f.QuadPart;
        }();
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        return now.QuadPart * 1000000 / s_frequency;
    }

    HRESULT CapturePresentedBuffer(_In_ IDXGISwapChain1* swapChain, _In_z_ const wchar_t* path)
    {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        IFC_RETURN(swapChain->GetDesc1(&desc));
        // Flip model: buffer 0 is the next back buffer; the last buffer is the one just presented.
        ComPtr<ID3D11Texture2D> presented;
        IFC_RETURN(swapChain->GetBuffer(desc.BufferCount - 1, IID_PPV_ARGS(&presented)));
        ComPtr<ID3D11Device> device;
        presented->GetDevice(&device);
        D3D11_TEXTURE2D_DESC textureDesc{};
        presented->GetDesc(&textureDesc);
        textureDesc.Usage = D3D11_USAGE_STAGING;
        textureDesc.BindFlags = 0;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        textureDesc.MiscFlags = 0;
        ComPtr<ID3D11Texture2D> staging;
        IFC_RETURN(device->CreateTexture2D(&textureDesc, nullptr, &staging));
        ComPtr<ID3D11DeviceContext> context;
        device->GetImmediateContext(&context);
        context->CopyResource(staging.Get(), presented.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        IFC_RETURN(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped));
        auto unmap = wil::scope_exit([&] { context->Unmap(staging.Get(), 0); });

        const uint32_t rowBytes = textureDesc.Width * 4;
        BITMAPFILEHEADER fileHeader{};
        BITMAPINFOHEADER infoHeader{};
        infoHeader.biSize = sizeof(infoHeader);
        infoHeader.biWidth = static_cast<LONG>(textureDesc.Width);
        infoHeader.biHeight = -static_cast<LONG>(textureDesc.Height);
        infoHeader.biPlanes = 1;
        infoHeader.biBitCount = 32;
        infoHeader.biCompression = BI_RGB;
        fileHeader.bfType = 0x4D42;
        fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(infoHeader);
        fileHeader.bfSize = fileHeader.bfOffBits + rowBytes * textureDesc.Height;

        wil::unique_hfile file(CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!file)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        DWORD written = 0;
        WriteFile(file.get(), &fileHeader, sizeof(fileHeader), &written, nullptr);
        WriteFile(file.get(), &infoHeader, sizeof(infoHeader), &written, nullptr);
        for (uint32_t y = 0; y < textureDesc.Height; ++y)
        {
            WriteFile(file.get(), static_cast<const uint8_t*>(mapped.pData) + y * mapped.RowPitch, rowBytes, &written, nullptr);
        }
        return S_OK;
    }

    ColorF ToColor(uint32_t argb, float opacity) noexcept
    {
        ColorF c;
        c.a = ((argb >> 24) & 0xff) / 255.0f * opacity;
        c.r = ((argb >> 16) & 0xff) / 255.0f;
        c.g = ((argb >> 8) & 0xff) / 255.0f;
        c.b = (argb & 0xff) / 255.0f;
        return c;
    }

    enum class BrushResult { None, Color, Unsupported };

    // Solid colors are exact; gradients are approximated by the average of their stops (the target scenes only use
    // them for subtle control borders). Anything else is unsupported.
    BrushResult ResolveBrush(_In_opt_ CBrush* brush, float opacity, _Out_ ColorF* color) noexcept
    {
        *color = {};
        if (brush == nullptr)
        {
            return BrushResult::None;
        }
        const float brushOpacity = brush->m_eOpacity * opacity;
        if (auto solid = do_pointer_cast<CSolidColorBrush>(brush))
        {
            *color = ToColor(solid->m_rgb, brushOpacity);
            return color->a > 0 ? BrushResult::Color : BrushResult::None;
        }
        if (auto composition = do_pointer_cast<CXamlCompositionBrush>(brush))
        {
            // AcrylicBrush and other XamlCompositionBrushBase effects: paint the fallback color (the brush's own
            // "effects unavailable" rendering); callers report it through CompositionBrushDetail.
            *color = ToColor(composition->m_fallbackColor, brushOpacity);
            return color->a > 0 ? BrushResult::Color : BrushResult::None;
        }
        if (auto gradient = do_pointer_cast<CGradientBrush>(brush))
        {
            float a = 0, r = 0, g = 0, b = 0;
            uint32_t count = 0;
            if (gradient->m_pStops != nullptr)
            {
                for (CDependencyObject* item : *gradient->m_pStops)
                {
                    if (auto stop = do_pointer_cast<CGradientStop>(item))
                    {
                        const ColorF c = ToColor(stop->m_rgb, 1.0f);
                        a += c.a; r += c.r; g += c.g; b += c.b;
                        ++count;
                    }
                }
            }
            if (count == 0)
            {
                return BrushResult::None;
            }
            color->a = a / count * brushOpacity;
            color->r = r / count;
            color->g = g / count;
            color->b = b / count;
            return color->a > 0 ? BrushResult::Color : BrushResult::None;
        }
        return BrushResult::Unsupported;
    }

    // Unsupported-content detail for a brush painted with its fallback color, or nullptr.
    const wchar_t* CompositionBrushDetail(_In_opt_ CBrush* brush) noexcept
    {
        if (do_pointer_cast<CXamlCompositionBrush>(brush) == nullptr)
        {
            return nullptr;
        }
        const CClassInfo* info = brush->GetClassInformation();
        if (info != nullptr)
        {
            const xstring_ptr name = info->GetFullName();
            if (!name.IsNullOrEmpty() && wcsstr(name.GetBuffer(), L"AcrylicBrush") != nullptr)
            {
                return L"AcrylicBrush:fallback";
            }
        }
        // MUXC's AcrylicBrush is a custom type: the core class info only knows its XamlCompositionBrushBase base.
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_acrylicBrush, L"Microsoft.UI.Xaml.Media.AcrylicBrush");
        if (brush->HasManagedPeer()
            && SUCCEEDED(FxCallbacks::FrameworkCallbacks_CheckPeerType(brush, c_acrylicBrush, FALSE)))
        {
            return L"AcrylicBrush:fallback";
        }
        return L"XamlCompositionBrush:fallback";
    }

    struct Fnv
    {
        uint64_t hash = 14695981039346656037ull;

        void Add(const void* data, size_t size) noexcept
        {
            auto bytes = static_cast<const uint8_t*>(data);
            for (size_t i = 0; i < size; ++i)
            {
                hash ^= bytes[i];
                hash *= 1099511628211ull;
            }
        }

        template <typename T> void AddValue(const T& value) noexcept { Add(&value, sizeof(value)); }
    };

    // Maps XAML family names ("XamlAutoFontFamily", "A, B", "ms-appx:///font.ttf#Name") to one installed family name.
    std::wstring ResolveFontFamily(_In_opt_ CFontFamily* family)
    {
        std::wstring name;
        xstring_ptr source;
        if (family != nullptr && SUCCEEDED(family->get_Source(&source)) && !source.IsNullOrEmpty())
        {
            name.assign(source.GetBuffer(), source.GetCount());
        }
        const auto comma = name.find(L',');
        if (comma != std::wstring::npos)
        {
            name.resize(comma);
        }
        const auto hash = name.find(L'#');
        if (hash != std::wstring::npos)
        {
            name = name.substr(hash + 1);
        }
        const auto first = name.find_first_not_of(L" \t");
        const auto last = name.find_last_not_of(L" \t");
        name = first == std::wstring::npos ? std::wstring() : name.substr(first, last - first + 1);
        if (name.empty() || _wcsicmp(name.c_str(), L"XamlAutoFontFamily") == 0)
        {
            name = L"Segoe UI";
        }
        return name;
    }

    // Coalesces damage: overlapping rects merge, then the smallest-growth pairs merge until at most maxCount remain.
    void CoalesceDamage(_Inout_ std::vector<RectF>& rects, uint32_t maxCount)
    {
        bool merged = true;
        while (merged)
        {
            merged = false;
            for (size_t i = 0; i < rects.size() && !merged; ++i)
            {
                for (size_t j = i + 1; j < rects.size(); ++j)
                {
                    if (RectsIntersect(rects[i], rects[j]))
                    {
                        rects[i] = UnionRect(rects[i], rects[j]);
                        rects.erase(rects.begin() + j);
                        merged = true;
                        break;
                    }
                }
            }
        }

        auto area = [](const RectF& r) { return static_cast<double>(r.right - r.left) * (r.bottom - r.top); };
        while (rects.size() > maxCount)
        {
            size_t bestI = 0, bestJ = 1;
            double bestGrowth = DBL_MAX;
            for (size_t i = 0; i < rects.size(); ++i)
            {
                for (size_t j = i + 1; j < rects.size(); ++j)
                {
                    const double growth = area(UnionRect(rects[i], rects[j])) - area(rects[i]) - area(rects[j]);
                    if (growth < bestGrowth)
                    {
                        bestGrowth = growth;
                        bestI = i;
                        bestJ = j;
                    }
                }
            }
            rects[bestI] = UnionRect(rects[bestI], rects[bestJ]);
            rects.erase(rects.begin() + bestJ);
            // A merge can create new overlaps; keep the set disjoint so painters can sum damaged pixels.
            CoalesceDamage(rects, UINT32_MAX);
        }
    }

    bool IsDeviceLost(HRESULT hr) noexcept
    {
        return hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET || hr == static_cast<HRESULT>(0x8899000CL) /* D2DERR_RECREATE_TARGET */;
    }
}



namespace ImmediateRenderingDetail
{
    enum class CommandType : uint8_t
    {
        Nop, FillRect, FillRounded, StrokeRounded, Text, GlyphRun, Bitmap, FillPath, StrokePath,
        PushLayer, PopLayer, PushRoundedClip, PopRoundedClip,
    };

    struct Command
    {
        CommandType type = CommandType::FillRect;
        Matrix3x2F transform;  // DIPs, element -> island
        RectF clip;            // DIPs, island space; always set (unclipped = island bounds)
        RectF rect;            // DIPs, element space
        RectF deviceBounds;    // physical pixels, island space (clipped); for pushes, the union of the group
        ColorF color;
        float radiusX = 0;
        float radiusY = 0;
        float thickness = 0;
        float fontSize = 0;
        float opacity = 1;
        uint32_t fontWeight = 400;
        uint32_t index = 0;    // Text: strings (text, family); GlyphRun: glyphRuns; Push*: index of the matching pop
        uint64_t key = 0;      // Bitmap
        RectF sourceRect;      // Bitmap, pixels; empty = whole bitmap
        const CUIElement* owner = nullptr;
        uint32_t foldedFrom = UINT32_MAX; // index of the opacity group folded into this command (subtree reuse)
        bool pinned = false;   // PushLayer of an independent layer: never folded (the render thread re-evaluates it)
    };

    struct GlyphRunData
    {
        ComPtr<IDWriteFontFace> face;
        float emSize = 0;
        uint32_t bidiLevel = 0;
        std::vector<uint16_t> indices;
        std::vector<float> advances;
        std::vector<GlyphOffset> offsets;
    };

    // Per-element drawing summary used for damage. Content and group (opacity layer / rounded clip around the children)
    // are diffed separately so an unchanged group does not damage everything under it.
    struct ElementRecord
    {
        bool hasContent = false;
        RectF bounds;            // physical pixels, island space, clipped; union of the element's own commands
        RectF relBounds;         // physical pixels relative to the innermost scroller's content origin; not clipped by viewports
        uint64_t signature = 14695981039346656037ull;
        const CUIElement* scroller = nullptr; // innermost ScrollContentPresenter
        RectF cover;             // physical pixels: the element's drawing is uniform inside this rect (blit reuse)

        bool hasGroup = false;
        RectF groupBounds;       // physical pixels, union of the group's commands
        uint64_t groupSignature = 0;
        RectF roundedClip;       // physical pixels; non-empty when the group is a rounded clip
        float roundedRadius = 0; // physical pixels
    };

    struct ScrollerInfo
    {
        const CUIElement* presenter = nullptr;
        const CUIElement* parent = nullptr;
        RectF viewport;          // physical pixels, island space
        float offsetX = 0;       // physical pixels: content origin in island space
        float offsetY = 0;
        bool hasContent = false;
    };

    struct PathGeometryData
    {
        std::vector<PathVerb> verbs;
        std::vector<PointF> points;
        bool nonZero = false;
    };

    // Output range of one walked subtree (children included), recorded post-order so a subtree's descendant spans are
    // [spanBegin, own index). The parent context it was walked in is the reuse key.
    struct SubtreeSpan
    {
        const CUIElement* element = nullptr;
        Matrix3x2F world;
        RectF clip;
        RectF contentClip;
        float scale = 1;
        float offsetX = 0;
        float offsetY = 0;
        const CUIElement* scroller = nullptr;
        uint32_t cmdBegin = 0, cmdEnd = 0;
        uint32_t strBegin = 0, strEnd = 0;
        uint32_t glyphBegin = 0, glyphEnd = 0;
        uint32_t pathBegin = 0, pathEnd = 0;
        uint32_t bitmapBegin = 0, bitmapEnd = 0;
        uint32_t recordBegin = 0, recordEnd = 0;
        uint32_t spanBegin = 0;
    };

    struct DisplayList
    {
        std::vector<Command> commands;
        std::vector<std::wstring> strings;
        std::vector<GlyphRunData> glyphRuns;
        std::vector<PathGeometryData> paths; // FillPath/StrokePath: index
        std::vector<xref_ptr<IPALSurface>> bitmaps; // Bitmap commands: decoded software surfaces (key = pointer)
        std::unordered_map<const CUIElement*, ElementRecord> records;
        std::vector<const CUIElement*> recordOrder; // records in creation order (subtree spans index this)
        std::vector<ScrollerInfo> scrollers;
        std::vector<SubtreeSpan> spans;
        std::unordered_map<const CUIElement*, uint32_t> spanIndex;
        uint32_t uncacheable = 0; // bumped by content that depends on more than the parent context (popups, scrollers)
        std::vector<IndependentLayer> layers; // innermost first
        int64_t snapshotQpc = 0;  // QPC (microseconds) when the walk sampled the animation clocks

        ElementRecord& Record(_In_ const CUIElement* element)
        {
            auto result = records.try_emplace(element);
            if (result.second)
            {
                recordOrder.push_back(element);
            }
            return result.first->second;
        }
    };

    struct WalkContext
    {
        CMILMatrix world{ true };
        RectF clip;              // DIPs, island space
        RectF contentClip;       // DIPs, island space; clip accumulated inside the innermost scroller's content
        float scale = 1;
        int scroller = -1;       // index into DisplayList::scrollers
        float offsetX = 0;       // DIPs: innermost scroller content origin
        float offsetY = 0;
        bool scrollerContent = false;
    };
}

using namespace ImmediateRenderingDetail;

struct ImmediateRenderer::IslandState
{
    std::unique_ptr<IImmediatePainter> painter;
    ComPtr<IDXGISwapChain1> swapChain;
    ComPtr<WUComp::ISpriteVisual> sprite;
    ComPtr<WUComp::ICompositionSurfaceBrush> surfaceBrush;
    uint32_t width = 0;
    uint32_t height = 0;
    float scale = 0;
    bool fullDamage = true;
    std::unordered_map<const CUIElement*, ElementRecord> records;
    std::unordered_map<const CUIElement*, ScrollerInfo> scrollers;
    // Damage of the last presented frame, repainted again when WINUI_IMMEDIATE_PRESENT_UNION=1 (diagnostic: flip-model
    // Present1 dirty rects already carry the rest of the previous buffer forward, so this is off by default).
    std::vector<RectF> presentedDamage;
    bool presentedFull = true;
    // Decoded image surfaces uploaded to the painter (EnsureBitmap). The reference keeps the key (surface pointer) from
    // being reused while the painter holds the bitmap; entries unused for c_bitmapIdleFrames are released.
    struct BitmapEntry
    {
        xref_ptr<IPALSurface> surface;
        uint64_t lastUsed = 0;
    };
    std::unordered_map<uint64_t, BitmapEntry> bitmaps;
    uint64_t frames = 0;
    DisplayList previousWalk;
    IndependentIsland independent;
};

ImmediateRenderer::ImmediateRenderer(Mode mode) noexcept
    : m_mode(mode)
    , m_independent(std::make_unique<IndependentState>())
{
}

ImmediateRenderer::~ImmediateRenderer()
{
    StopRenderThread();
    ReleaseResources();
}

void ImmediateRenderer::ReleaseResources() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_independent->mutex);
    for (auto& entry : m_islands)
    {
        if (entry.second->painter)
        {
            IGNOREHR(entry.second->painter->SetPresentationSwapChain(nullptr));
        }
    }
    m_islands.clear();
    m_device.Reset();
    m_deviceTraced = false;
}

void ImmediateRenderer::PruneIslands(_In_ DCompTreeHost* dcompTreeHost)
{
    auto& renderData = dcompTreeHost->GetXamlIslandRenderData();
    std::lock_guard<std::recursive_mutex> lock(m_independent->mutex);
    for (auto it = m_islands.begin(); it != m_islands.end();)
    {
        if (renderData.find(it->first) == renderData.end())
        {
            if (it->second->painter)
            {
                IGNOREHR(it->second->painter->SetPresentationSwapChain(nullptr));
            }
            it = m_islands.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

_Check_return_ HRESULT ImmediateRenderer::EnsureDevice()
{
    if (m_device)
    {
        return S_OK;
    }

    if (m_mode == Mode::ImmediateD2DHardware || m_mode == Mode::ImmediateD2DSoftware)
    {
        // CreateD3DDeviceForMode emits DeviceInfo for the mode.
        DeviceIdentity identity{};
        IFC_RETURN(CreateD3DDeviceForMode(m_mode, m_device.ReleaseAndGetAddressOf(), &identity));
        m_deviceTraced = true;
        return S_OK;
    }

    // GDI+ paints on the CPU and only needs a device that owns the presentation swap chain. A dedicated device keeps
    // the painter off WinUI's shared, lock-guarded CD3D11Device.
    constexpr D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, m_device.ReleaseAndGetAddressOf(), nullptr, nullptr);
    if (FAILED(hr))
    {
        IFC_RETURN(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, m_device.ReleaseAndGetAddressOf(), nullptr, nullptr));
    }
    return S_OK;
}

_Check_return_ HRESULT ImmediateRenderer::EnsureIslandResources(
    _In_ DCompTreeHost* dcompTreeHost,
    _In_ CXamlIslandRoot* island,
    _Inout_ IslandState& state,
    uint32_t width,
    uint32_t height,
    float scale)
{
    IFC_RETURN(EnsureDevice());

    if (!state.painter)
    {
        PainterOptions options;
        options.width = width;
        options.height = height;
        options.dpiScale = scale;
        if (m_mode == Mode::ImmediateGdiPlus)
        {
            IFC_RETURN(CreateGdiPlusPainter(options, state.painter));
        }
        else
        {
            IFC_RETURN(CreateD2DPainter(m_mode, m_device.Get(), options, state.painter));
        }

        if (!m_deviceTraced)
        {
            const DeviceIdentity identity = state.painter->GetDeviceIdentity();
            TraceDeviceInfo(identity.mode, identity.adapterLuid, identity.isWarp, identity.featureLevel);
            m_deviceTraced = true;
        }

        ComPtr<IDXGIDevice> dxgiDevice;
        ComPtr<IDXGIAdapter> adapter;
        ComPtr<IDXGIFactory2> factory;
        IFC_RETURN(m_device.As(&dxgiDevice));
        IFC_RETURN(dxgiDevice->GetAdapter(&adapter));
        IFC_RETURN(adapter->GetParent(IID_PPV_ARGS(&factory)));

        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = width;
        desc.Height = height;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
        IFC_RETURN(factory->CreateSwapChainForComposition(m_device.Get(), &desc, nullptr, state.swapChain.ReleaseAndGetAddressOf()));
        IFC_RETURN(state.painter->SetPresentationSwapChain(state.swapChain.Get()));

        state.width = width;
        state.height = height;
        state.scale = scale;
        state.fullDamage = true;
        state.records.clear();
        state.scrollers.clear();
    }
    else if (state.width != width || state.height != height || state.scale != scale)
    {
        IFC_RETURN(state.painter->Resize(width, height, scale));
        state.width = width;
        state.height = height;
        state.scale = scale;
        state.fullDamage = true;
    }

    WUComp::ICompositor* compositor = dcompTreeHost->GetCompositor();
    if (!state.sprite)
    {
        ComPtr<WUComp::ICompositionSurface> surface;
        IFC_RETURN(dcompTreeHost->CreateCompositionSurfaceForSwapChain(state.swapChain.Get(), &surface));
        IFC_RETURN(compositor->CreateSurfaceBrush(&state.surfaceBrush));
        IFC_RETURN(state.surfaceBrush->put_Surface(surface.Get()));
        IFC_RETURN(compositor->CreateSpriteVisual(&state.sprite));
        if (XamlProfilerTracing::IsProfilerEnabled())
        {
            RenderingResourceDiagnostics::LogResourceCreated(RenderingResourceDiagnostics::ResourceKind::SurfaceBrush);
            RenderingResourceDiagnostics::LogResourceCreated(RenderingResourceDiagnostics::ResourceKind::SpriteVisual);
        }
        ComPtr<WUComp::ICompositionBrush> brush;
        IFC_RETURN(state.surfaceBrush.As(&brush));
        IFC_RETURN(state.sprite->put_Brush(brush.Get()));
    }

    ComPtr<WUComp::IVisual> visual;
    IFC_RETURN(state.sprite.As(&visual));
    IFC_RETURN(visual->put_Size({ static_cast<float>(width), static_cast<float>(height) }));

    // Host the sprite as the island's root, the same way DCompTreeHost::ConnectXamlIslandTargetRoots hosts the island's
    // comp node visual in Composition mode (which never happens here: no comp nodes are created).
    auto& renderDataMap = dcompTreeHost->GetXamlIslandRenderData();
    auto found = renderDataMap.find(island);
    if (found != renderDataMap.end() && island->GetContentRequested())
    {
        auto& renderData = found->second;
        if (renderData.m_islandRootVisual.Get() != visual.Get() && renderData.m_disconnectedRootVisual == nullptr)
        {
            ComPtr<ixp::IContentIslandExperimental> contentIslandExperimental;
            IFC_RETURN(island->GetContentIsland()->QueryInterface(IID_PPV_ARGS(&contentIslandExperimental)));
            IFC_RETURN(contentIslandExperimental->put_Root(visual.Get()));
            island->SetRootVisual(visual.Get());
            renderData.m_islandRootVisual = visual;
            renderData.contentConnected = true;
            if (XamlProfilerTracing::IsEnabled())
            {
                XamlProfilerTracing::NotifyRootSet(visual.Get(), reinterpret_cast<uint64_t>(island));
            }
        }
    }

    return S_OK;
}



void ImmediateRenderer::ReportUnsupported(_In_ CUIElement* element, _In_z_ const wchar_t* detail)
{
    const xstring_ptr className = element->GetClassName();
    std::wstring key(className.IsNullOrEmpty() ? L"" : className.GetBuffer());
    key += L'|';
    key += detail;
    if (m_reportedUnsupported.insert(key).second)
    {
        TraceUnsupportedContent(m_mode, className.IsNullOrEmpty() ? L"" : className.GetBuffer(), detail);
        if (FILE* statsFile = GetStatsFile())
        {
            fwprintf(statsFile, L"# unsupported %s %s\n", className.IsNullOrEmpty() ? L"" : className.GetBuffer(), detail);
        }
    }
}

namespace
{
    const RectF c_unbounded{ -1e7f, -1e7f, 1e7f, 1e7f };

    // DIPs -> physical pixels, rounded outward with a 1px antialiasing margin.
    RectF ToDevice(const RectF& r, float scale) noexcept
    {
        return { std::floor(r.left * scale) - 1, std::floor(r.top * scale) - 1, std::ceil(r.right * scale) + 1, std::ceil(r.bottom * scale) + 1 };
    }

    RectF ScaleRect(const RectF& r, float scale) noexcept
    {
        return { r.left * scale, r.top * scale, r.right * scale, r.bottom * scale };
    }

    RectF OffsetRect(const RectF& r, float dx, float dy) noexcept
    {
        return { r.left + dx, r.top + dy, r.right + dx, r.bottom + dy };
    }

    bool RectsNear(const RectF& a, const RectF& b) noexcept
    {
        constexpr float e = 0.01f;
        return std::fabs(a.left - b.left) < e && std::fabs(a.top - b.top) < e && std::fabs(a.right - b.right) < e && std::fabs(a.bottom - b.bottom) < e;
    }

    bool Contains(const RectF& outer, const RectF& inner) noexcept
    {
        return outer.left <= inner.left && outer.top <= inner.top && outer.right >= inner.right && outer.bottom >= inner.bottom;
    }

    int64_t Quantize(float v) noexcept
    {
        return std::llround(static_cast<double>(v) * 256.0);
    }

    bool IsDrawing(CommandType type) noexcept
    {
        switch (type)
        {
            case CommandType::Nop:
            case CommandType::PushLayer:
            case CommandType::PopLayer:
            case CommandType::PushRoundedClip:
            case CommandType::PopRoundedClip:
                return false;
            default:
                return true;
        }
    }

    bool IsPush(CommandType type) noexcept
    {
        return type == CommandType::PushLayer || type == CommandType::PushRoundedClip;
    }

    bool IsPop(CommandType type) noexcept
    {
        return type == CommandType::PopLayer || type == CommandType::PopRoundedClip;
    }

    // Hashes a transform relative to the innermost scroller's content origin, so content that only scrolled keeps its
    // signature (and can be reused by a ScrollPixels blit).
    void HashRelative(Fnv& fnv, const CMILMatrix& m, const WalkContext& ctx) noexcept
    {
        const int64_t values[] = {
            Quantize(m.GetM11()), Quantize(m.GetM12()), Quantize(m.GetM21()), Quantize(m.GetM22()),
            Quantize(m.GetDx() - ctx.offsetX), Quantize(m.GetDy() - ctx.offsetY),
            Quantize(ctx.contentClip.left - ctx.offsetX), Quantize(ctx.contentClip.top - ctx.offsetY),
            Quantize(ctx.contentClip.right - ctx.offsetX), Quantize(ctx.contentClip.bottom - ctx.offsetY),
        };
        fnv.Add(values, sizeof(values));
    }

    // Flattens a XAML geometry into painter path verbs: quadratics are elevated to cubics and arcs are split into
    // cubic segments of at most 90 degrees. Hollow figures are stroked but not filled.
    constexpr double c_pi = 3.14159265358979323846;

    class PathFlattener final : public CXcpObjectBase<IPALGeometrySink>
    {
    public:
        PathGeometryData fill;
        PathGeometryData stroke;

        void BeginFigure(const XPOINTF& startPoint, bool fIsHollow) override
        {
            m_hollow = fIsHollow;
            m_current = startPoint;
            AddVerb(PathVerb::MoveTo, &startPoint, 1);
        }

        void EndFigure(bool fIsClosed) override
        {
            if (fIsClosed)
            {
                AddVerb(PathVerb::Close, nullptr, 0);
            }
            else if (!m_hollow)
            {
                // Fills close open figures implicitly; strokes must not.
                fill.verbs.push_back(PathVerb::Close);
            }
        }

        void AddArc(const XPOINTF& point, const XSIZEF& size, XFLOAT rotationAngle, bool fIsClockwise, bool fIsLargeArc) override
        {
            AppendArc(m_current, point, std::fabs(size.width), std::fabs(size.height), rotationAngle, fIsLargeArc, fIsClockwise);
        }

        void AddBezier(const XPOINTF& c1, const XPOINTF& c2, const XPOINTF& end) override
        {
            const XPOINTF pts[] = { c1, c2, end };
            AddVerb(PathVerb::CubicTo, pts, 3);
        }

        void AddLine(const XPOINTF& point) override
        {
            AddVerb(PathVerb::LineTo, &point, 1);
        }

        void AddQuadraticBezier(const XPOINTF& control, const XPOINTF& end) override
        {
            const XPOINTF p0 = m_current;
            const XPOINTF pts[] = {
                { p0.x + 2.0f / 3.0f * (control.x - p0.x), p0.y + 2.0f / 3.0f * (control.y - p0.y) },
                { end.x + 2.0f / 3.0f * (control.x - end.x), end.y + 2.0f / 3.0f * (control.y - end.y) },
                end,
            };
            AddVerb(PathVerb::CubicTo, pts, 3);
        }

        void AddLines(_In_reads_(uiCount) const XPOINTF* pPoints, XUINT32 uiCount) override
        {
            for (XUINT32 i = 0; i < uiCount; ++i) AddLine(pPoints[i]);
        }

        void AddBeziers(_In_reads_(uiCount) const XPOINTF* pPoints, XUINT32 uiCount) override
        {
            for (XUINT32 i = 0; i + 2 < uiCount; i += 3) AddBezier(pPoints[i], pPoints[i + 1], pPoints[i + 2]);
        }

        void AddQuadraticBeziers(_In_reads_(uiCount) const XPOINTF* pPoints, XUINT32 uiCount) override
        {
            for (XUINT32 i = 0; i + 1 < uiCount; i += 2) AddQuadraticBezier(pPoints[i], pPoints[i + 1]);
        }

        void SetFillMode(GeometryFillMode fillMode) override
        {
            fill.nonZero = stroke.nonZero = fillMode == GeometryFillMode::Winding;
        }

        _Check_return_ HRESULT Close() override { return S_OK; }

    private:
        void AddVerb(PathVerb verb, _In_reads_opt_(count) const XPOINTF* pts, uint32_t count)
        {
            stroke.verbs.push_back(verb);
            if (!m_hollow) fill.verbs.push_back(verb);
            for (uint32_t i = 0; i < count; ++i)
            {
                const PointF p{ pts[i].x, pts[i].y };
                stroke.points.push_back(p);
                if (!m_hollow) fill.points.push_back(p);
            }
            if (count > 0) m_current = pts[count - 1];
        }

        // SVG endpoint-to-center arc conversion (SVG 1.1 F.6.5), then cubic approximation per <= 90 degree segment.
        void AppendArc(const XPOINTF& p0, const XPOINTF& p1, float rx, float ry, float angleDegrees, bool largeArc, bool clockwise)
        {
            if ((p0.x == p1.x && p0.y == p1.y))
            {
                return;
            }
            if (rx < 1e-6f || ry < 1e-6f)
            {
                AddLine(p1);
                return;
            }
            const double phi = angleDegrees * c_pi / 180.0;
            const double cosPhi = std::cos(phi), sinPhi = std::sin(phi);
            const double dx = (p0.x - p1.x) / 2.0, dy = (p0.y - p1.y) / 2.0;
            const double x1 = cosPhi * dx + sinPhi * dy;
            const double y1 = -sinPhi * dx + cosPhi * dy;
            double rxd = rx, ryd = ry;
            const double lambda = (x1 * x1) / (rxd * rxd) + (y1 * y1) / (ryd * ryd);
            if (lambda > 1)
            {
                rxd *= std::sqrt(lambda);
                ryd *= std::sqrt(lambda);
            }
            const double num = rxd * rxd * ryd * ryd - rxd * rxd * y1 * y1 - ryd * ryd * x1 * x1;
            const double den = rxd * rxd * y1 * y1 + ryd * ryd * x1 * x1;
            double coef = den > 0 ? std::sqrt(std::max(0.0, num / den)) : 0;
            if (largeArc == clockwise) coef = -coef;
            const double cx1 = coef * rxd * y1 / ryd;
            const double cy1 = -coef * ryd * x1 / rxd;
            const double cx = cosPhi * cx1 - sinPhi * cy1 + (p0.x + p1.x) / 2.0;
            const double cy = sinPhi * cx1 + cosPhi * cy1 + (p0.y + p1.y) / 2.0;
            auto angle = [](double ux, double uy, double vx, double vy) {
                return std::atan2(ux * vy - uy * vx, ux * vx + uy * vy);
            };
            const double theta1 = angle(1, 0, (x1 - cx1) / rxd, (y1 - cy1) / ryd);
            double delta = angle((x1 - cx1) / rxd, (y1 - cy1) / ryd, (-x1 - cx1) / rxd, (-y1 - cy1) / ryd);
            if (clockwise && delta < 0) delta += 2 * c_pi;
            else if (!clockwise && delta > 0) delta -= 2 * c_pi;

            const int segments = std::max(1, static_cast<int>(std::ceil(std::fabs(delta) / (c_pi / 2) - 1e-6)));
            const double step = delta / segments;
            const double k = 4.0 / 3.0 * std::tan(step / 4);
            auto at = [&](double t, double ddx, double ddy) {
                // Point on the unit circle (plus derivative offset), mapped to the ellipse.
                const double ux = std::cos(t) + ddx, uy = std::sin(t) + ddy;
                return XPOINTF{ static_cast<float>(cx + rxd * cosPhi * ux - ryd * sinPhi * uy),
                                static_cast<float>(cy + rxd * sinPhi * ux + ryd * cosPhi * uy) };
            };
            double t = theta1;
            for (int i = 0; i < segments; ++i)
            {
                const double t2 = t + step;
                XPOINTF pts[] = {
                    at(t, -k * std::sin(t), k * std::cos(t)),
                    at(t2, k * std::sin(t2), -k * std::cos(t2)),
                    i == segments - 1 ? p1 : at(t2, 0, 0),
                };
                AddVerb(PathVerb::CubicTo, pts, 3);
                t = t2;
            }
        }

        XPOINTF m_current{};
        bool m_hollow = false;
    };

    struct Emitter
    {
        DisplayList& list;
        const CUIElement* element;
        const WalkContext& ctx;
        ElementRecord* record = nullptr;
        bool any = false;

        ElementRecord& Record()
        {
            if (record == nullptr)
            {
                record = &list.Record(element);
                if (!record->hasContent)
                {
                    record->hasContent = true;
                    record->bounds = {};
                    record->relBounds = {};
                    record->cover = c_unbounded;
                    record->scroller = ctx.scroller >= 0 ? list.scrollers[ctx.scroller].presenter : nullptr;
                }
            }
            return *record;
        }

        // coverLocal: element-space rect inside which this command draws uniformly (nullptr: nowhere).
        void Push(Command command, const RectF& localInk, const CMILMatrix& m, const RectF& clip, _In_opt_ const RectF* coverLocal)
        {
            ElementRecord& r = Record();
            command.transform = ToMatrix(m);
            command.clip = clip;
            command.owner = element;

            Fnv fnv;
            fnv.hash = r.signature;
            fnv.AddValue(command.type);
            HashRelative(fnv, m, ctx);
            fnv.AddValue(command.rect);
            fnv.AddValue(command.color);
            fnv.AddValue(command.radiusX);
            fnv.AddValue(command.radiusY);
            fnv.AddValue(command.thickness);
            fnv.AddValue(command.fontSize);
            fnv.AddValue(command.fontWeight);
            fnv.AddValue(command.opacity);
            fnv.AddValue(command.key);
            fnv.AddValue(command.sourceRect);
            if (command.type == CommandType::Text)
            {
                const auto& text = list.strings[command.index];
                const auto& family = list.strings[command.index + 1];
                fnv.Add(text.data(), text.size() * sizeof(wchar_t));
                fnv.Add(family.data(), family.size() * sizeof(wchar_t));
            }
            else if (command.type == CommandType::GlyphRun)
            {
                const GlyphRunData& run = list.glyphRuns[command.index];
                fnv.AddValue(run.face.Get());
                fnv.AddValue(run.emSize);
                fnv.AddValue(run.bidiLevel);
                fnv.Add(run.indices.data(), run.indices.size() * sizeof(uint16_t));
                fnv.Add(run.advances.data(), run.advances.size() * sizeof(float));
                fnv.Add(run.offsets.data(), run.offsets.size() * sizeof(GlyphOffset));
                // Glyph-run clips (trimming) are in glyph space; hash them relative to the content origin.
                const RectF relClip = OffsetRect(clip, -ctx.offsetX, -ctx.offsetY);
                const int64_t q[] = { Quantize(relClip.left), Quantize(relClip.right) };
                fnv.Add(q, sizeof(q));
            }
            else if (command.type == CommandType::FillPath || command.type == CommandType::StrokePath)
            {
                const PathGeometryData& path = list.paths[command.index];
                fnv.Add(path.verbs.data(), path.verbs.size() * sizeof(PathVerb));
                fnv.Add(path.points.data(), path.points.size() * sizeof(PointF));
                fnv.AddValue(path.nonZero);
            }
            r.signature = fnv.hash;

            const RectF ink = TransformRectBounds(m, localInk);
            const RectF rel = IntersectRect(ink, ctx.contentClip);
            if (!IsEmptyRect(rel))
            {
                r.relBounds = UnionRect(r.relBounds, ToDevice(OffsetRect(rel, -ctx.offsetX, -ctx.offsetY), ctx.scale));
            }

            const RectF device = IntersectRect(ink, clip);
            if (IsEmptyRect(device))
            {
                return;
            }
            command.deviceBounds = ToDevice(device, ctx.scale);
            r.bounds = UnionRect(r.bounds, command.deviceBounds);

            if (coverLocal != nullptr && m.IsScaleOrTranslationOnly() && !IsEmptyRect(*coverLocal))
            {
                const RectF c = ScaleRect(IntersectRect(TransformRectBounds(m, *coverLocal), clip), ctx.scale);
                r.cover = IntersectRect(r.cover, { std::ceil(c.left), std::ceil(c.top), std::floor(c.right), std::floor(c.bottom) });
            }
            else
            {
                r.cover = {};
            }

            list.commands.push_back(command);
            any = true;
        }

        void Fill(const RectF& rect, float rx, float ry, const ColorF& color, const CMILMatrix& m)
        {
            if (IsEmptyRect(rect)) return;
            Command c;
            c.type = (rx > 0 || ry > 0) ? CommandType::FillRounded : CommandType::FillRect;
            c.rect = rect;
            c.radiusX = rx;
            c.radiusY = ry;
            c.color = color;
            const RectF interior{ rect.left + rx, rect.top + ry, rect.right - rx, rect.bottom - ry };
            Push(c, rect, m, ctx.clip, &interior);
        }

        void Stroke(const RectF& rect, float rx, float ry, float thickness, const ColorF& color, const CMILMatrix& m)
        {
            if (thickness <= 0 || rect.right < rect.left || rect.bottom < rect.top) return;
            Command c;
            c.type = CommandType::StrokeRounded;
            c.rect = rect;
            c.radiusX = rx;
            c.radiusY = ry;
            c.thickness = thickness;
            c.color = color;
            const float h = thickness / 2;
            const float r = std::max(rx, ry);
            const RectF hole{ rect.left + h + r, rect.top + h + r, rect.right - h - r, rect.bottom - h - r };
            Push(c, { rect.left - h, rect.top - h, rect.right + h, rect.bottom + h }, m, ctx.clip, &hole);
        }

        // thickness == 0: fill; otherwise stroke. localInk: element-space bounds of the drawing.
        void Path(PathGeometryData data, float thickness, const ColorF& color, const CMILMatrix& m, const RectF& localInk)
        {
            if (data.verbs.empty() || IsEmptyRect(localInk)) return;
            Command c;
            c.type = thickness > 0 ? CommandType::StrokePath : CommandType::FillPath;
            c.thickness = thickness;
            c.color = color;
            c.index = static_cast<uint32_t>(list.paths.size());
            list.paths.push_back(std::move(data));
            Push(c, localInk, m, ctx.clip, nullptr);
        }

        void Text(std::wstring text, std::wstring family, float fontSize, uint32_t weight, const RectF& layout, const RectF& ink, const ColorF& color)
        {
            if (text.empty() || fontSize <= 0) return;
            Command c;
            c.type = CommandType::Text;
            c.rect = layout;
            c.fontSize = fontSize;
            c.fontWeight = weight;
            c.color = color;
            c.index = static_cast<uint32_t>(list.strings.size());
            list.strings.push_back(std::move(text));
            list.strings.push_back(std::move(family));
            Push(c, ink, ctx.world, ctx.clip, nullptr);
        }

        void Glyphs(const DWRITE_GLYPH_RUN& run, const CMILMatrix& m, const ColorF& color, float clipLeft, float clipRight)
        {
            if (run.glyphCount == 0 || run.fontFace == nullptr || run.glyphIndices == nullptr || !(run.fontEmSize > 0))
            {
                return;
            }
            GlyphRunData data;
            data.face = run.fontFace;
            data.emSize = run.fontEmSize;
            data.bidiLevel = run.bidiLevel;
            data.indices.assign(run.glyphIndices, run.glyphIndices + run.glyphCount);
            float width = 0;
            if (run.glyphAdvances != nullptr)
            {
                data.advances.assign(run.glyphAdvances, run.glyphAdvances + run.glyphCount);
            }
            else
            {
                // Painters require advances; take the design advances.
                data.advances.resize(run.glyphCount);
                std::vector<DWRITE_GLYPH_METRICS> metrics(run.glyphCount);
                DWRITE_FONT_METRICS fontMetrics{};
                run.fontFace->GetMetrics(&fontMetrics);
                if (SUCCEEDED(run.fontFace->GetDesignGlyphMetrics(run.glyphIndices, run.glyphCount, metrics.data(), run.isSideways)))
                {
                    for (uint32_t i = 0; i < run.glyphCount; ++i)
                    {
                        data.advances[i] = metrics[i].advanceWidth * run.fontEmSize / fontMetrics.designUnitsPerEm;
                    }
                }
            }
            for (float a : data.advances)
            {
                width += a;
            }
            if (run.glyphOffsets != nullptr)
            {
                data.offsets.resize(run.glyphCount);
                for (uint32_t i = 0; i < run.glyphCount; ++i)
                {
                    data.offsets[i] = { run.glyphOffsets[i].advanceOffset, run.glyphOffsets[i].ascenderOffset };
                }
            }

            const float em = run.fontEmSize;
            RectF ink = (run.bidiLevel & 1) ? RectF{ -width - em / 2, -em * 1.3f, em / 2, em * 0.6f }
                                            : RectF{ -em / 2, -em * 1.3f, width + em / 2, em * 0.6f };
            RectF clip = ctx.clip;
            if (clipLeft > -1e30f || clipRight < 1e30f)
            {
                const RectF glyphClip{ std::max(clipLeft, ink.left), ink.top - em, std::min(clipRight, ink.right), ink.bottom + em };
                if (IsEmptyRect(glyphClip))
                {
                    return;
                }
                clip = IntersectRect(clip, TransformRectBounds(m, glyphClip));
                ink = IntersectRect(ink, glyphClip);
            }

            Command c;
            c.type = CommandType::GlyphRun;
            c.color = color;
            c.fontSize = em;
            c.index = static_cast<uint32_t>(list.glyphRuns.size());
            list.glyphRuns.push_back(std::move(data));
            Push(c, ink, m, clip, nullptr);
        }

        // Draws the source rect (bitmap pixels) of a decoded surface into dest (element space). rx/ry > 0 clips the
        // image to a rounded rect (ImageBrush on an Ellipse or rounded Border).
        void Bitmap(_In_ IPALSurface* surface, const RectF& dest, const RectF& source, float opacity, const CMILMatrix& m,
            const RectF& clipRect, float rx, float ry)
        {
            if (IsEmptyRect(dest) || !(opacity > 0))
            {
                return;
            }
            size_t pushIndex = SIZE_MAX;
            if (rx > 0 || ry > 0)
            {
                pushIndex = list.commands.size();
                Command push;
                push.type = CommandType::PushRoundedClip;
                push.transform = ToMatrix(m);
                push.rect = clipRect;
                push.radiusX = rx;
                push.radiusY = ry;
                push.owner = element;
                list.commands.push_back(push);
            }
            Command c;
            c.type = CommandType::Bitmap;
            c.rect = dest;
            c.sourceRect = source;
            c.opacity = opacity;
            c.radiusX = rx;
            c.radiusY = ry;
            c.key = reinterpret_cast<uint64_t>(surface);
            c.index = static_cast<uint32_t>(list.bitmaps.size());
            list.bitmaps.emplace_back(surface);
            const size_t before = list.commands.size();
            Push(c, dest, m, ctx.clip, nullptr);
            if (pushIndex != SIZE_MAX)
            {
                if (list.commands.size() == before)
                {
                    list.commands.pop_back();
                    return;
                }
                Command& push = list.commands[pushIndex];
                push.deviceBounds = list.commands.back().deviceBounds;
                push.index = static_cast<uint32_t>(list.commands.size());
                Command pop;
                pop.type = CommandType::PopRoundedClip;
                pop.deviceBounds = push.deviceBounds;
                pop.owner = element;
                list.commands.push_back(pop);
            }
        }
    };

    // Receives the glyph runs, decorations and highlight rects XAML's text stack produced for a text element.
    class TextEmitter final : public ImmediateTextSink
    {
    public:
        TextEmitter(Emitter& emitter, const CMILMatrix& world) : m_emitter(emitter), m_world(world) {}

        void HighlightRect(const XRECTF& rect, const CMILMatrix& transform, _In_opt_ CBrush* brush) override
        {
            ColorF color;
            if (Resolve(brush, &color))
            {
                CMILMatrix m = transform;
                m.Append(m_world);
                m_emitter.Fill(FromXRect(rect), 0, 0, color, m);
            }
        }

        void GlyphRun(const DWRITE_GLYPH_RUN& run, const CMILMatrix& transform, _In_opt_ CBrush* brush, float clipLeft, float clipRight) override
        {
            ColorF color;
            if (Resolve(brush, &color))
            {
                CMILMatrix m = transform;
                m.Append(m_world);
                m_emitter.Glyphs(run, m, color, clipLeft, clipRight);
            }
        }

        void Line(const XRECTF_RB& rect, const CMILMatrix& transform, _In_opt_ CBrush* brush) override
        {
            ColorF color;
            if (Resolve(brush, &color))
            {
                CMILMatrix m = transform;
                m.Append(m_world);
                m_emitter.Fill({ rect.left, rect.top, rect.right, rect.bottom }, 0, 0, color, m);
            }
        }

        bool unsupportedBrush = false;

    private:
        bool Resolve(_In_opt_ CBrush* brush, _Out_ ColorF* color)
        {
            const BrushResult result = ResolveBrush(brush, 1.0f, color);
            if (result == BrushResult::Unsupported)
            {
                unsupportedBrush = true;
            }
            return result == BrushResult::Color;
        }

        Emitter& m_emitter;
        const CMILMatrix& m_world;
    };

    float MaxOf(const XTHICKNESS& t) noexcept
    {
        return std::max(std::max(t.left, t.right), std::max(t.top, t.bottom));
    }

    float MaxOf(const XCORNERRADIUS& r) noexcept
    {
        return std::max(std::max(r.topLeft, r.topRight), std::max(r.bottomLeft, r.bottomRight));
    }

    bool IsUniform(const XTHICKNESS& t) noexcept
    {
        return t.left == t.right && t.left == t.top && t.left == t.bottom;
    }

    bool IsBorderLike(_In_ CUIElement* element) noexcept
    {
        return element->OfTypeByIndex<KnownTypeIndex::Panel>()
            || element->OfTypeByIndex<KnownTypeIndex::Border>()
            || (element->OfTypeByIndex<KnownTypeIndex::ContentPresenter>()
                && !element->OfTypeByIndex<KnownTypeIndex::ListViewBaseItemPresenter>());
    }

    // Paints an ImageBrush (Image element content or a Fill/Background) over bounds (element space) from its decoded
    // software surface; immediate modes force software decodes (CImageSource::PrepareDecodeParams). Requests a
    // decode at the rendered size like HWWalk::RequestImageDecode. Returns an unsupported-content detail or nullptr.
    const wchar_t* EmitImageBrush(
        Emitter& emitter,
        _In_ CImageBrush* brush,
        const RectF& bounds,
        float rx,
        float ry,
        const CMILMatrix& world,
        float scale,
        bool nineGrid)
    {
        if (!(brush->m_eOpacity > 0) || IsEmptyRect(bounds) || brush->m_pImageSource == nullptr)
        {
            return nullptr;
        }

        XRECTF xbounds{ bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top };
        if (nineGrid || brush->ShouldRequestDecode())
        {
            XRECTF decodeBounds = xbounds;
            if (nineGrid)
            {
                brush->GetNaturalBounds(&decodeBounds);
            }
            else if (SUCCEEDED(brush->AdjustDecodeRectForStretch(&decodeBounds)))
            {
                const RectF device = ScaleRect(TransformRectBounds(world, FromXRect(decodeBounds)), scale);
                decodeBounds = { 0, 0, device.right - device.left, device.bottom - device.top };
            }
            IGNOREHR(brush->QueueRequestDecode(XcpRound(decodeBounds.Width), XcpRound(decodeBounds.Height), true));
        }

        IPALSurface* surface = brush->GetSoftwareSurface();
        if (surface == nullptr)
        {
            // Still loading/decoding, or decoded to a hardware surface (SVG, SurfaceImageSource, ...).
            return brush->m_pImageSource->HasHardwareSurfaces() ? L"Image without software surface" : nullptr;
        }
        if (surface->GetPixelFormat() != pixelColor32bpp_A8R8G8B8 || surface->GetWidth() == 0 || surface->GetHeight() == 0)
        {
            return L"Image pixel format";
        }

        XRECTF natural{};
        brush->GetNaturalBounds(&natural);
        if (!(natural.Width > 0 && natural.Height > 0))
        {
            return nullptr;
        }
        CMILMatrix identity(true);
        CMILMatrix brushToElement(true);
        if (FAILED(brush->ComputeDeviceToSource(&identity, &xbounds, &brushToElement)))
        {
            return L"ImageBrush transform";
        }
        if (!brushToElement.IsScaleOrTranslationOnly())
        {
            return L"ImageBrush transform";
        }

        const RectF dest = TransformRectBounds(brushToElement, { 0, 0, natural.Width, natural.Height });
        const RectF visible = IntersectRect(dest, bounds);
        if (IsEmptyRect(visible) || IsEmptyRect(dest))
        {
            return nullptr;
        }
        const float sx = surface->GetWidth() / (dest.right - dest.left);
        const float sy = surface->GetHeight() / (dest.bottom - dest.top);
        const RectF source{ (visible.left - dest.left) * sx, (visible.top - dest.top) * sy,
            (visible.right - dest.left) * sx, (visible.bottom - dest.top) * sy };
        emitter.Bitmap(surface, visible, source, std::min(1.0f, brush->m_eOpacity), world, bounds, rx, ry);
        return nineGrid ? L"Image NineGrid" : nullptr;
    }

    // Closes a group opened at pushIndex: drops empty groups, folds a single-primitive opacity group into the
    // primitive (exact), otherwise emits the pop. Records the group for damage.
    void CloseGroup(
        _Inout_ DisplayList& list,
        size_t pushIndex,
        _In_ const CUIElement* element,
        const WalkContext& ctx,
        const CMILMatrix& world)
    {
        Command& push = list.commands[pushIndex];
        RectF bounds{};
        uint32_t draws = 0;
        size_t single = SIZE_MAX;
        bool nested = false;
        for (size_t i = pushIndex + 1; i < list.commands.size(); ++i)
        {
            const Command& c = list.commands[i];
            if (c.type == CommandType::Nop)
            {
                continue;
            }
            if (IsPush(c.type) || IsPop(c.type))
            {
                nested = true;
                bounds = UnionRect(bounds, c.deviceBounds);
                continue;
            }
            ++draws;
            single = i;
            bounds = UnionRect(bounds, c.deviceBounds);
        }

        const bool isLayer = push.type == CommandType::PushLayer;
        Fnv fnv;
        ElementRecord& record = list.Record(element);
        if (!record.hasGroup)
        {
            record.hasGroup = true;
            record.groupBounds = {};
            record.groupSignature = fnv.hash;
        }
        fnv.hash = record.groupSignature;
        fnv.AddValue(push.type);
        fnv.AddValue(push.opacity);
        fnv.AddValue(push.rect);
        fnv.AddValue(push.radiusX);
        HashRelative(fnv, world, ctx);
        record.groupSignature = fnv.hash;
        record.groupBounds = UnionRect(record.groupBounds, bounds);

        if (draws == 0 && !nested)
        {
            push.type = CommandType::Nop;
            return;
        }
        if (isLayer && draws == 1 && !nested && !push.pinned)
        {
            Command& c = list.commands[single];
            c.color.a *= push.opacity;
            c.opacity *= push.opacity;
            c.foldedFrom = static_cast<uint32_t>(pushIndex);
            push.type = CommandType::Nop;
            return;
        }

        if (!isLayer)
        {
            const RectF clipRect = ScaleRect(TransformRectBounds(world, push.rect), ctx.scale);
            record.roundedClip = clipRect;
            record.roundedRadius = push.radiusX * ctx.scale * std::max(std::fabs(world.GetM11()), std::fabs(world.GetM22()));
        }

        push.index = static_cast<uint32_t>(list.commands.size());
        push.deviceBounds = bounds;
        Command pop;
        pop.type = isLayer ? CommandType::PopLayer : CommandType::PopRoundedClip;
        pop.deviceBounds = bounds;
        pop.owner = element;
        list.commands.push_back(pop);
    }
}

void ImmediateRenderer::EmitContent(
    _In_ CUIElement* element,
    const WalkContext& ctx,
    bool deferBorder,
    _Inout_ DisplayList& list)
{
    Emitter emitter{ list, element, ctx };
    const CMILMatrix& world = ctx.world;
    const float width = element->GetActualWidth();
    const float height = element->GetActualHeight();
    const KnownTypeIndex type = element->GetTypeIndex();

    if (IsBorderLike(element))
    {
        auto fe = static_cast<CFrameworkElement*>(element);
        const xref_ptr<CBrush> background = fe->GetBackgroundBrush();
        const xref_ptr<CBrush> border = fe->GetBorderBrush();
        const XTHICKNESS thickness = fe->GetBorderThickness();
        const float radius = MaxOf(fe->GetCornerRadius());
        const float t = MaxOf(thickness);

        ColorF backgroundColor;
        BrushResult backgroundResult = ResolveBrush(background.get(), 1.0f, &backgroundColor);
        ColorF borderColor;
        const BrushResult borderResult = t > 0 ? ResolveBrush(border.get(), 1.0f, &borderColor) : BrushResult::None;
        if (auto imageBrush = do_pointer_cast<CImageBrush>(background.get()))
        {
            if (const wchar_t* detail = EmitImageBrush(emitter, imageBrush, { 0, 0, width, height }, radius, radius, world, ctx.scale, false))
            {
                ReportUnsupported(element, detail);
            }
            backgroundResult = BrushResult::None;
        }
        if (backgroundResult == BrushResult::Unsupported || borderResult == BrushResult::Unsupported)
        {
            ReportUnsupported(element, L"Brush");
        }
        if (const wchar_t* detail = CompositionBrushDetail(background.get()))
        {
            ReportUnsupported(element, detail);
        }

        if (backgroundResult == BrushResult::Color)
        {
            RectF fill{ 0, 0, width, height };
            float fillRadius = radius;
            if (fe->GetBackgroundSizing() == DirectUI::BackgroundSizing::InnerBorderEdge && borderResult == BrushResult::Color)
            {
                fill = { thickness.left, thickness.top, width - thickness.right, height - thickness.bottom };
                fillRadius = std::max(0.0f, radius - t / 2);
            }
            emitter.Fill(fill, fillRadius, fillRadius, backgroundColor, world);
        }

        if (!deferBorder)
        {
            EmitBorder(element, ctx, list);
        }
    }
    else if (type == KnownTypeIndex::Rectangle || type == KnownTypeIndex::Ellipse)
    {
        auto shape = static_cast<CShape*>(element);
        const xref_ptr<CBrush> stroke = shape->GetStroke();
        const float t = stroke ? shape->GetStrokeThickness() : 0.0f;
        ColorF fillColor;
        BrushResult fillResult = ResolveBrush(shape->m_pFill, 1.0f, &fillColor);
        if (fillResult == BrushResult::Color && element->GetCompositeMode() == DirectUI::ElementCompositeMode::DestInvert)
        {
            // The TextBox caret inverts what is under it. Painters have no invert blend, so draw it in the text
            // foreground (which contrasts with the background the same way) or, elsewhere, in the inverted color.
            ColorF foreground;
            const TextFormatting* formatting = nullptr;
            CUIElement* parentElement = element->GetUIElementAdjustedParentInternal(false);
            if (parentElement != nullptr && parentElement->GetTypeIndex() == KnownTypeIndex::TextBoxView
                && SUCCEEDED(parentElement->GetTextFormatting(&formatting)) && formatting != nullptr
                && ResolveBrush(formatting->m_pForeground, 1.0f, &foreground) == BrushResult::Color)
            {
                foreground.a *= fillColor.a;
                fillColor = foreground;
            }
            else
            {
                fillColor = { 1 - fillColor.r, 1 - fillColor.g, 1 - fillColor.b, fillColor.a };
            }
            ReportUnsupported(element, L"CompositeMode DestInvert:approximated");
        }
        ColorF strokeColor;
        const BrushResult strokeResult = t > 0 ? ResolveBrush(stroke.get(), 1.0f, &strokeColor) : BrushResult::None;
        CImageBrush* fillImage = do_pointer_cast<CImageBrush>(shape->m_pFill);
        if (fillImage != nullptr)
        {
            fillResult = BrushResult::None;
        }
        if (fillResult == BrushResult::Unsupported || strokeResult == BrushResult::Unsupported)
        {
            ReportUnsupported(element, L"Brush");
        }
        if (const wchar_t* detail = CompositionBrushDetail(shape->m_pFill))
        {
            ReportUnsupported(element, detail);
        }

        const float h = strokeResult == BrushResult::Color ? t / 2 : 0.0f;
        const RectF geometry{ h, h, width - h, height - h };
        float rx = 0, ry = 0;
        if (type == KnownTypeIndex::Ellipse)
        {
            rx = (geometry.right - geometry.left) / 2;
            ry = (geometry.bottom - geometry.top) / 2;
        }
        else
        {
            auto rectangle = static_cast<CRectangle*>(element);
            rx = std::min(rectangle->m_eRadiusX, (geometry.right - geometry.left) / 2);
            ry = std::min(rectangle->m_eRadiusY, (geometry.bottom - geometry.top) / 2);
        }
        if (fillResult == BrushResult::Color)
        {
            emitter.Fill(geometry, rx, ry, fillColor, world);
        }
        else if (fillImage != nullptr)
        {
            if (const wchar_t* detail = EmitImageBrush(emitter, fillImage, geometry, rx, ry, world, ctx.scale, false))
            {
                ReportUnsupported(element, detail);
            }
        }
        if (strokeResult == BrushResult::Color)
        {
            emitter.Stroke(geometry, rx, ry, t, strokeColor, world);
        }
    }
    else if (type == KnownTypeIndex::TextBlock || type == KnownTypeIndex::RichTextBlock)
    {
        // Replay XAML's own DirectWrite layout (glyph runs, underlines, selection/highlight rects).
        TextEmitter sink(emitter, world);
        HRESULT hr = type == KnownTypeIndex::TextBlock
            ? static_cast<CTextBlock*>(element)->VisitImmediate(sink)
            : static_cast<CRichTextBlock*>(element)->VisitImmediate(sink);
        if (FAILED(hr))
        {
            ReportUnsupported(element, L"Text layout unavailable");
        }
        if (sink.unsupportedBrush)
        {
            ReportUnsupported(element, L"Foreground brush");
        }
    }
    else if (type == KnownTypeIndex::TextBoxView)
    {
        // TextBoxView draws its owner's text (RichEdit layout, not exposed as glyph runs); re-lay it out with DrawText.
        std::wstring text;
        CDependencyObject* ancestor = element->GetParentInternal(false);
        for (int depth = 0; ancestor != nullptr && depth < 16; ++depth)
        {
            if (ancestor->OfTypeByIndex<KnownTypeIndex::TextBox>())
            {
                CValue value;
                if (SUCCEEDED(ancestor->GetValueByIndex(KnownPropertyIndex::TextBox_Text, &value)))
                {
                    const xstring_ptr str = value.AsString();
                    if (!str.IsNullOrEmpty())
                    {
                        text.assign(str.GetBuffer(), str.GetCount());
                    }
                }
                break;
            }
            if (ancestor->OfTypeByIndex<KnownTypeIndex::PasswordBox>() || ancestor->OfTypeByIndex<KnownTypeIndex::RichEditBox>())
            {
                ReportUnsupported(element, L"PasswordBox/RichEditBox text");
                break;
            }
            ancestor = ancestor->GetParentInternal(false);
        }

        const TextFormatting* formatting = nullptr;
        if (!text.empty() && SUCCEEDED(element->GetTextFormatting(&formatting)) && formatting != nullptr)
        {
            ColorF color;
            const BrushResult result = ResolveBrush(formatting->m_pForeground, 1.0f, &color);
            if (result == BrushResult::Unsupported)
            {
                ReportUnsupported(element, L"Foreground brush");
            }
            else if (result == BrushResult::Color)
            {
                const float fontSize = formatting->m_eFontSize;
                // A different shaper may measure a few pixels wider than RichEdit; widen so single lines never wrap.
                RectF layout{ 0, 0, width + fontSize * 2, height };
                const float overhang = std::max(2.0f, fontSize / 4);
                const RectF ink{ layout.left - overhang, layout.top - overhang, layout.right + overhang, layout.bottom + overhang };
                emitter.Text(std::move(text), ResolveFontFamily(formatting->m_pFontFamily), fontSize,
                    static_cast<uint32_t>(formatting->m_nFontWeight), layout, ink, color);
            }
        }
    }
    else if (element->OfTypeByIndex<KnownTypeIndex::ListViewBaseItemPresenter>())
    {
        // Rounded chrome draws its backplate, borders and selection indicator as child Borders (walked normally);
        // the legacy chrome draws them itself.
        if (!CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeEnabled(element->GetContext()))
        {
            ReportUnsupported(element, L"Legacy ListViewItem chrome");
        }
    }
    else if (element->OfTypeByIndex<KnownTypeIndex::Shape>())
    {
        // Path, Line, Polyline, Polygon: flatten the render geometry and draw it under the shape's stretch transform.
        auto shape = static_cast<CShape*>(element);
        if (shape->m_pRender == nullptr)
        {
            IGNOREHR(shape->UpdateRenderGeometry());
        }
        if (shape->m_pRender == nullptr)
        {
            return;
        }
        xref_ptr<PathFlattener> flattener;
        flattener.attach(new PathFlattener());
        if (FAILED(shape->m_pRender->VisitSink(flattener.get())) || flattener->stroke.verbs.empty())
        {
            return;
        }
        CMILMatrix stretch(true);
        if (shape->m_Stretch != DirectUI::Stretch::None)
        {
            CValue result;
            if (SUCCEEDED(CShape::GetShapeGeometryTransform(shape, 0, nullptr, nullptr, &result)))
            {
                CDependencyObject* matrixObject = result.AsObject();
                if (auto transform = do_pointer_cast<CTransform>(matrixObject))
                {
                    transform->GetTransform(&stretch);
                }
                ReleaseInterface(matrixObject);
            }
        }
        // The stretch applies to the geometry, not to the pen: transform the points, keep the stroke thickness.
        RectF b{ FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };
        for (PathGeometryData* data : { &flattener->fill, &flattener->stroke })
        {
            for (PointF& p : data->points)
            {
                XPOINTF xp;
                stretch.Transform(XPOINTF{ p.x, p.y }, xp);
                p = { xp.x, xp.y };
                b = { std::min(b.left, p.x), std::min(b.top, p.y), std::max(b.right, p.x), std::max(b.bottom, p.y) };
            }
        }

        ColorF fillColor;
        const BrushResult fillResult = ResolveBrush(shape->m_pFill, 1.0f, &fillColor);
        const float t = shape->GetStrokeThickness();
        auto stroke = shape->GetStroke();
        ColorF strokeColor;
        const BrushResult strokeResult = t > 0 ? ResolveBrush(stroke.get(), 1.0f, &strokeColor) : BrushResult::None;
        if (fillResult == BrushResult::Unsupported || strokeResult == BrushResult::Unsupported)
        {
            ReportUnsupported(element, L"Brush");
        }
        if (strokeResult == BrushResult::Color && shape->HasStrokeDashArray())
        {
            ReportUnsupported(element, L"Shape stroke dashes");
        }
        if (fillResult == BrushResult::Color && !flattener->fill.verbs.empty())
        {
            emitter.Path(std::move(flattener->fill), 0, fillColor, world, b);
        }
        if (strokeResult == BrushResult::Color)
        {
            // Conservative ink for miter joins (the painter strokes with miter joins).
            const float pad = 2 * t;
            emitter.Path(std::move(flattener->stroke), t, strokeColor, world, { b.left - pad, b.top - pad, b.right + pad, b.bottom + pad });
        }
    }
    else if (type == KnownTypeIndex::Image)
    {
        auto image = static_cast<CImageBase*>(element);
        if (CImageBrush* brush = image->GetBackgroundBrush())
        {
            if (const wchar_t* detail = EmitImageBrush(emitter, brush, { 0, 0, width, height }, 0, 0, world, ctx.scale, image->HasNinegrid()))
            {
                ReportUnsupported(element, detail);
            }
        }
    }
    else if (type == KnownTypeIndex::RichTextBlockOverflow
        || type == KnownTypeIndex::Glyphs
        || type == KnownTypeIndex::SwapChainPanel
        || type == KnownTypeIndex::MediaPlayerPresenter
        || element->OfTypeByIndex<KnownTypeIndex::CalendarViewBaseItem>())
    {
        ReportUnsupported(element, L"Element content");
    }
}

void ImmediateRenderer::EmitBorder(
    _In_ CUIElement* element,
    const WalkContext& ctx,
    _Inout_ DisplayList& list)
{
    if (!IsBorderLike(element))
    {
        return;
    }
    Emitter emitter{ list, element, ctx };
    auto fe = static_cast<CFrameworkElement*>(element);
    const float width = element->GetActualWidth();
    const float height = element->GetActualHeight();
    const XTHICKNESS thickness = fe->GetBorderThickness();
    const float t = MaxOf(thickness);
    if (!(t > 0))
    {
        return;
    }
    const xref_ptr<CBrush> border = fe->GetBorderBrush();
    ColorF borderColor;
    if (ResolveBrush(border.get(), 1.0f, &borderColor) != BrushResult::Color)
    {
        return;
    }
    const float radius = MaxOf(fe->GetCornerRadius());
    if (!IsUniform(thickness) && radius == 0)
    {
        emitter.Fill({ 0, 0, width, thickness.top }, 0, 0, borderColor, ctx.world);
        emitter.Fill({ 0, height - thickness.bottom, width, height }, 0, 0, borderColor, ctx.world);
        emitter.Fill({ 0, thickness.top, thickness.left, height - thickness.bottom }, 0, 0, borderColor, ctx.world);
        emitter.Fill({ width - thickness.right, thickness.top, width, height - thickness.bottom }, 0, 0, borderColor, ctx.world);
    }
    else
    {
        const float h = t / 2;
        const float r = std::max(0.0f, radius - h);
        emitter.Stroke({ h, h, width - h, height - h }, r, r, t, borderColor, ctx.world);
    }
}

void ImmediateRenderer::Walk(
    _In_ CUIElement* element,
    const WalkContext& parent,
    _Inout_ DisplayList& list)
{
    ++m_elementsVisited;

    if (!element->IsVisible() || element->NWSkipRendering())
    {
        return;
    }

    const KnownTypeIndex type = element->GetTypeIndex();
    switch (type)
    {
        case KnownTypeIndex::PopupRoot:
        {
            // Open popups paint on top of the island content, oldest first (HWWalk::PopupRootRenderChildren). A popup's
            // child is parented to the PopupRoot but positioned by the popup's own ancestor chain.
            auto popupRoot = static_cast<CPopupRoot*>(element);
            ++list.uncacheable;
            const std::vector<CPopup*> popups = popupRoot->GetOpenPopupList(true /* includeUnloadingPopups */);
            WalkContext popupCtx = parent;
            popupCtx.scroller = -1;
            popupCtx.scrollerContent = false;
            popupCtx.offsetX = popupCtx.offsetY = 0;
            popupCtx.contentClip = parent.clip;
            for (auto it = popups.rbegin(); it != popups.rend(); ++it)
            {
                CPopup* popup = *it;
                if (popup->IsWindowed())
                {
                    // Windowed popups own an HWND/ContentIsland; immediate modes paint them into the island's swap
                    // chain instead, so content outside the window bounds is clipped.
                    ReportUnsupported(popup, L"Windowed popup:painted in island");
                }
                if (popup->m_overlayElement != nullptr)
                {
                    Walk(popup->m_overlayElement, popupCtx, list);
                }
                CMILMatrix popupWorld(true);
                for (CUIElement* e = popup; e != nullptr && e->GetTypeIndex() != KnownTypeIndex::XamlIslandRoot
                    && e->GetTypeIndex() != KnownTypeIndex::PopupRoot; e = e->GetUIElementAdjustedParentInternal(false))
                {
                    CMILMatrix local;
                    (void)e->GetLocalTransform(TransformRetrievalOptions::None, &local);
                    popupWorld.Append(local);
                }
                popupWorld.Append(parent.world);
                WalkContext childCtx = popupCtx;
                // Walk() appends the child's own local transform (its arrange offset under the PopupRoot) to this.
                childCtx.world = popupWorld;
                if (popup->m_unloadingChild)
                {
                    Walk(popup->m_unloadingChild.get(), childCtx, list);
                }
                if (popup->m_pChild != nullptr)
                {
                    Walk(popup->m_pChild, childCtx, list);
                }
            }
            return;
        }
        case KnownTypeIndex::RenderTargetBitmapRoot:
        case KnownTypeIndex::XamlIslandRootCollection:
        case KnownTypeIndex::ConnectedAnimationRoot:
        case KnownTypeIndex::LayoutTransitionElement:
        case KnownTypeIndex::TransitionRoot:
        case KnownTypeIndex::FullWindowMediaRoot:
            return;
        default:
            break;
    }

    // Incremental walk: a subtree with no render-dirty flags (cleaned after every immediate frame) walked in the same
    // parent context produces the same output as last frame. Direct scroller content is always walked (it records the
    // scroller's offset). Hand-off visual clip changes (Composition InsetClip, mirrored by DCompPropertyChangedListener)
    // only dirty bounds, so subtrees with hand-off visuals are always walked.
    const bool usesHandOffVisual = element->IsUsingHandOffVisual();
    if (usesHandOffVisual)
    {
        ++list.uncacheable;
    }
    else if (m_previousWalk != nullptr && !parent.scrollerContent && !element->NWNeedsRendering() && ReuseSubtree(element, parent, list))
    {
        return;
    }
    const uint32_t cmdBegin = static_cast<uint32_t>(list.commands.size());
    const uint32_t strBegin = static_cast<uint32_t>(list.strings.size());
    const uint32_t glyphBegin = static_cast<uint32_t>(list.glyphRuns.size());
    const uint32_t pathBegin = static_cast<uint32_t>(list.paths.size());
    const uint32_t bitmapBegin = static_cast<uint32_t>(list.bitmaps.size());
    const uint32_t recordBegin = static_cast<uint32_t>(list.recordOrder.size());
    const uint32_t spanBegin = static_cast<uint32_t>(list.spans.size());
    const uint32_t uncacheableBefore = list.uncacheable;

    const float localOpacity = element->GetOpacityLocal();
    if (!(localOpacity > 0))
    {
        return;
    }
    IndependentProbe probe;
    ProbeIndependent(element, probe);
    if (probe.active)
    {
        ++list.uncacheable;
    }

    WalkContext ctx = parent;
    ctx.scrollerContent = false;
    auto clipTo = [&](const RectF& r)
    {
        ctx.clip = IntersectRect(ctx.clip, r);
        ctx.contentClip = IntersectRect(ctx.contentClip, r);
    };

    const bool hasLayoutClip = element->HasLayoutClip();
    const bool layoutClipIsAncestor = element->ShouldApplyLayoutClipAsAncestorClip();
    if (hasLayoutClip && layoutClipIsAncestor)
    {
        clipTo(TransformRectBounds(parent.world, FromXRect(element->LayoutClipGeometry->m_rc)));
    }
    probe.outerClip = ctx.clip;

    {
        CMILMatrix local;
        if (!element->GetLocalTransform(TransformRetrievalOptions::None, &local))
        {
            ctx.world = local;
            ctx.world.Append(parent.world);
        }
    }

    if (parent.scrollerContent && parent.scroller >= 0)
    {
        // Scrolled content: snap its origin to device pixels (like Composition's DManip snapping) so scroll deltas are
        // whole pixels and can be blitted; clips inside the content are tracked relative to its origin.
        if (ctx.world.IsScaleOrTranslationOnly())
        {
            ctx.world.SetDx(std::round(ctx.world.GetDx() * ctx.scale) / ctx.scale);
            ctx.world.SetDy(std::round(ctx.world.GetDy() * ctx.scale) / ctx.scale);
        }
        ScrollerInfo& scroller = list.scrollers[parent.scroller];
        if (!scroller.hasContent)
        {
            scroller.hasContent = true;
            scroller.viewport = ScaleRect(ctx.clip, ctx.scale);
            scroller.offsetX = ctx.world.GetDx() * ctx.scale;
            scroller.offsetY = ctx.world.GetDy() * ctx.scale;
        }
        ctx.offsetX = ctx.world.GetDx();
        ctx.offsetY = ctx.world.GetDy();
        ctx.contentClip = c_unbounded;
    }

    if (element->HasActiveProjection())
    {
        ReportUnsupported(element, L"Projection");
    }

    // ThemeShadow is a Composition projected/drop shadow; immediate modes do not draw shadows.
    if (element->IsShadowCaster())
    {
        ReportUnsupported(element, L"ThemeShadow");
    }

    // Composition content parented to the element (ElementCompositionPreview.SetElementChildVisual, e.g. the
    // AnimatedIcon used by CheckBox/RadioButton glyphs) has no XAML primitives to paint.
    if (element->IsUsingHandInVisual())
    {
        ReportUnsupported(element, L"Hand-in Composition visual");
    }

    // Clip rects are tracked as world-space bounding boxes; exact for scale/translate transforms (the target scenes).
    if (const xref_ptr<CGeometry> elementClip = element->GetClip())
    {
        if (auto rectangleClip = do_pointer_cast<CRectangleGeometry>(elementClip.get()))
        {
            CMILMatrix clipWorld = ctx.world;
            if (rectangleClip->m_pTransform != nullptr)
            {
                CMILMatrix clipTransform(true);
                rectangleClip->m_pTransform->GetTransform(&clipTransform);
                clipWorld = clipTransform;
                clipWorld.Append(ctx.world);
            }
            clipTo(TransformRectBounds(clipWorld, FromXRect(rectangleClip->m_rc)));
        }
        else
        {
            ReportUnsupported(element, L"Non-rectangular clip");
        }
    }

    // A Composition clip on the element's hand-off visual (ElementCompositionPreview.GetElementVisual; Expander sets an
    // InsetClip on its content clip border). Insets apply to the element's render size: in immediate modes no comp
    // node keeps the hand-off visual's Size in sync.
    if (usesHandOffVisual)
    {
        ComPtr<WUComp::IVisual> handOffVisual;
        element->GetStoredHandOffVisual(handOffVisual.ReleaseAndGetAddressOf());
        ComPtr<WUComp::ICompositionClip> handOffClip;
        if (handOffVisual != nullptr && SUCCEEDED(handOffVisual->get_Clip(&handOffClip)) && handOffClip != nullptr)
        {
            ComPtr<WUComp::IInsetClip> insetClip;
            float left = 0, top = 0, right = 0, bottom = 0;
            if (SUCCEEDED(handOffClip.As(&insetClip)) && SUCCEEDED(insetClip->get_LeftInset(&left))
                && SUCCEEDED(insetClip->get_TopInset(&top)) && SUCCEEDED(insetClip->get_RightInset(&right))
                && SUCCEEDED(insetClip->get_BottomInset(&bottom)))
            {
                const RectF local{ left, top, element->GetActualWidth() - right, element->GetActualHeight() - bottom };
                clipTo(TransformRectBounds(ctx.world, local));
            }
            else
            {
                ReportUnsupported(element, L"Hand-off visual clip");
            }
        }
    }

    if (hasLayoutClip && !layoutClipIsAncestor)
    {
        clipTo(TransformRectBounds(ctx.world, FromXRect(element->LayoutClipGeometry->m_rc)));
    }

    if (IsEmptyRect(ctx.clip))
    {
        return;
    }

    // Culling: skip subtrees entirely outside the clip (scrolled-out realized items, cache buffers). Inner bounds are
    // element-space bounds of the element and its subtree; inflate for antialiasing and glyph overhang.
    {
        XRECTF_RB inner{};
        if (SUCCEEDED(element->GetInnerBounds(&inner)) && inner.right >= inner.left && inner.bottom >= inner.top)
        {
            constexpr float c_cullMargin = 8.0f;
            const RectF local{ inner.left - c_cullMargin, inner.top - c_cullMargin, inner.right + c_cullMargin, inner.bottom + c_cullMargin };
            if (!probe.active && !RectsIntersect(TransformRectBounds(ctx.world, local), ctx.clip))
            {
                ++m_elementsCulled;
                return;
            }
        }
    }

    // Group opacity: the element and its subtree are composited together, then blended once.
    size_t layerIndex = SIZE_MAX;
    if (localOpacity < 1.0f || probe.active)
    {
        layerIndex = list.commands.size();
        Command push;
        push.type = CommandType::PushLayer;
        push.opacity = localOpacity;
        push.owner = element;
        push.pinned = probe.active;
        list.commands.push_back(push);
    }

    // CornerRadius on a Border/Panel/ContentPresenter with children clips the children to the rounded outer edge and
    // draws the border over them (CFrameworkElement::RequiresCompNodeForRoundedCorners in Composition mode).
    const bool roundedClip = element->RequiresCompNodeForRoundedCorners() && IsBorderLike(element);

    EmitContent(element, ctx, roundedClip, list);

    WalkContext childCtx = ctx;
    size_t clipIndex = SIZE_MAX;
    if (roundedClip)
    {
        const float width = element->GetActualWidth();
        const float height = element->GetActualHeight();
        const float radius = std::min(MaxOf(static_cast<CFrameworkElement*>(element)->GetCornerRadius()), std::min(width, height) / 2);
        Command push;
        push.type = CommandType::PushRoundedClip;
        push.transform = ToMatrix(ctx.world);
        push.rect = { 0, 0, width, height };
        push.radiusX = radius;
        push.radiusY = radius;
        push.owner = element;
        clipIndex = list.commands.size();
        list.commands.push_back(push);
        const RectF bounds = TransformRectBounds(ctx.world, push.rect);
        childCtx.clip = IntersectRect(childCtx.clip, bounds);
        childCtx.contentClip = IntersectRect(childCtx.contentClip, bounds);
    }

    if (type == KnownTypeIndex::ScrollContentPresenter)
    {
        ScrollerInfo scroller;
        scroller.presenter = element;
        scroller.parent = ctx.scroller >= 0 ? list.scrollers[ctx.scroller].presenter : nullptr;
        childCtx.scroller = static_cast<int>(list.scrollers.size());
        childCtx.scrollerContent = true;
        list.scrollers.push_back(scroller);
        ++list.uncacheable;
    }

    XUINT32 childCount = 0;
    if (!IsEmptyRect(childCtx.clip))
    {
        CUIElement** children = nullptr;
        element->GetChildrenInRenderOrder(&children, &childCount);
        for (XUINT32 i = 0; i < childCount; ++i)
        {
            if (children[i] != nullptr)
            {
                Walk(children[i], childCtx, list);
            }
        }
    }

    if (clipIndex != SIZE_MAX)
    {
        CloseGroup(list, clipIndex, element, ctx, ctx.world);
        EmitBorder(element, ctx, list);
    }
    if (layerIndex != SIZE_MAX)
    {
        if (probe.active)
        {
            FinishIndependentLayer(element, layerIndex, probe, ctx, list);
        }
        CloseGroup(list, layerIndex, element, ctx, ctx.world);
        if (probe.active && list.commands[layerIndex].type == CommandType::PushLayer)
        {
            probe.layer.pushIndex = static_cast<uint32_t>(layerIndex);
            list.layers.push_back(std::move(probe.layer));
        }
    }

    // Leaves are cheap to re-emit; record spans for subtrees only.
    if (childCount > 0 && !parent.scrollerContent && list.uncacheable == uncacheableBefore)
    {
        SubtreeSpan span;
        span.element = element;
        span.world = ToMatrix(parent.world);
        span.clip = parent.clip;
        span.contentClip = parent.contentClip;
        span.scale = parent.scale;
        span.offsetX = parent.offsetX;
        span.offsetY = parent.offsetY;
        span.scroller = parent.scroller >= 0 ? list.scrollers[parent.scroller].presenter : nullptr;
        span.cmdBegin = cmdBegin;
        span.cmdEnd = static_cast<uint32_t>(list.commands.size());
        span.strBegin = strBegin;
        span.strEnd = static_cast<uint32_t>(list.strings.size());
        span.glyphBegin = glyphBegin;
        span.glyphEnd = static_cast<uint32_t>(list.glyphRuns.size());
        span.pathBegin = pathBegin;
        span.pathEnd = static_cast<uint32_t>(list.paths.size());
        span.bitmapBegin = bitmapBegin;
        span.bitmapEnd = static_cast<uint32_t>(list.bitmaps.size());
        span.recordBegin = recordBegin;
        span.recordEnd = static_cast<uint32_t>(list.recordOrder.size());
        span.spanBegin = spanBegin;
        list.spanIndex[element] = static_cast<uint32_t>(list.spans.size());
        list.spans.push_back(span);
    }
}

bool ImmediateRenderer::ReuseSubtree(
    _In_ CUIElement* element,
    const WalkContext& parent,
    _Inout_ DisplayList& list)
{
    DisplayList& previous = *m_previousWalk;
    const auto found = previous.spanIndex.find(element);
    if (found == previous.spanIndex.end())
    {
        return false;
    }
    const uint32_t spanIndex = found->second;
    const SubtreeSpan& span = previous.spans[spanIndex];
    const CUIElement* scroller = parent.scroller >= 0 ? list.scrollers[parent.scroller].presenter : nullptr;
    const Matrix3x2F world = ToMatrix(parent.world);
    if (span.scroller != scroller || span.scale != parent.scale || span.offsetX != parent.offsetX || span.offsetY != parent.offsetY
        || memcmp(&span.world, &world, sizeof(world)) != 0 || memcmp(&span.clip, &parent.clip, sizeof(RectF)) != 0
        || memcmp(&span.contentClip, &parent.contentClip, sizeof(RectF)) != 0)
    {
        return false;
    }
    // A single-primitive opacity group of an ancestor folded its opacity into one of these commands.
    for (uint32_t i = span.cmdBegin; i < span.cmdEnd; ++i)
    {
        const uint32_t folded = previous.commands[i].foldedFrom;
        if (folded != UINT32_MAX && folded < span.cmdBegin)
        {
            return false;
        }
    }

    const int64_t cmdDelta = static_cast<int64_t>(list.commands.size()) - span.cmdBegin;
    const int64_t strDelta = static_cast<int64_t>(list.strings.size()) - span.strBegin;
    const int64_t glyphDelta = static_cast<int64_t>(list.glyphRuns.size()) - span.glyphBegin;
    const int64_t pathDelta = static_cast<int64_t>(list.paths.size()) - span.pathBegin;
    const int64_t bitmapDelta = static_cast<int64_t>(list.bitmaps.size()) - span.bitmapBegin;
    const int64_t recordDelta = static_cast<int64_t>(list.recordOrder.size()) - span.recordBegin;
    const int64_t spanDelta = static_cast<int64_t>(list.spans.size()) - span.spanBegin;
    auto shift = [](uint32_t& value, int64_t delta) { value = static_cast<uint32_t>(value + delta); };

    // Each element is walked once per frame, so the previous frame's data can be moved rather than copied.
    for (uint32_t i = span.strBegin; i < span.strEnd; ++i)
    {
        list.strings.push_back(std::move(previous.strings[i]));
    }
    for (uint32_t i = span.glyphBegin; i < span.glyphEnd; ++i)
    {
        list.glyphRuns.push_back(std::move(previous.glyphRuns[i]));
    }
    for (uint32_t i = span.pathBegin; i < span.pathEnd; ++i)
    {
        list.paths.push_back(std::move(previous.paths[i]));
    }
    for (uint32_t i = span.bitmapBegin; i < span.bitmapEnd; ++i)
    {
        list.bitmaps.push_back(std::move(previous.bitmaps[i]));
    }
    for (uint32_t i = span.cmdBegin; i < span.cmdEnd; ++i)
    {
        Command c = previous.commands[i];
        switch (c.type)
        {
            case CommandType::Text: shift(c.index, strDelta); break;
            case CommandType::GlyphRun: shift(c.index, glyphDelta); break;
            case CommandType::FillPath:
            case CommandType::StrokePath: shift(c.index, pathDelta); break;
            case CommandType::Bitmap: shift(c.index, bitmapDelta); break;
            case CommandType::PushLayer:
            case CommandType::PushRoundedClip: shift(c.index, cmdDelta); break;
            default: break;
        }
        if (c.foldedFrom != UINT32_MAX)
        {
            shift(c.foldedFrom, cmdDelta);
        }
        list.commands.push_back(c);
    }
    for (uint32_t i = span.recordBegin; i < span.recordEnd; ++i)
    {
        const CUIElement* owner = previous.recordOrder[i];
        auto record = previous.records.find(owner);
        if (record != previous.records.end())
        {
            list.records.emplace(owner, std::move(record->second));
            list.recordOrder.push_back(owner);
        }
    }
    for (uint32_t i = span.spanBegin; i <= spanIndex; ++i)
    {
        SubtreeSpan s = previous.spans[i];
        shift(s.cmdBegin, cmdDelta); shift(s.cmdEnd, cmdDelta);
        shift(s.strBegin, strDelta); shift(s.strEnd, strDelta);
        shift(s.glyphBegin, glyphDelta); shift(s.glyphEnd, glyphDelta);
        shift(s.pathBegin, pathDelta); shift(s.pathEnd, pathDelta);
        shift(s.bitmapBegin, bitmapDelta); shift(s.bitmapEnd, bitmapDelta);
        shift(s.recordBegin, recordDelta); shift(s.recordEnd, recordDelta);
        shift(s.spanBegin, spanDelta);
        list.spanIndex[s.element] = static_cast<uint32_t>(list.spans.size());
        list.spans.push_back(s);
    }
    ++m_subtreesReused;
    return true;
}

// Replays a display list's commands that intersect the damage. skipGroups: skip push/pop groups whose recorded bounds miss
// the damage (the render thread moves layers outside their recorded group bounds, so it replays every group).
static void ReplayCommands(
    IImmediatePainter& painter,
    const std::vector<Command>& commands,
    const DisplayList& list,
    const RectF& islandClip,
    bool fullDamage,
    const std::vector<RectF>& damage,
    bool skipGroups,
    _Inout_ uint32_t& elementsRendered)
{
    auto intersects = [&](const RectF& bounds)
    {
        return fullDamage || std::any_of(damage.begin(), damage.end(), [&](const RectF& d) { return RectsIntersect(d, bounds); });
    };
    std::vector<bool> layerPushed;
    painter.Clear({ 0, 0, 0, 0 });
    const Matrix3x2F identity;
    Matrix3x2F current = identity;
    bool clipPushed = false;
    RectF currentClip{};
    const CUIElement* lastOwner = nullptr;
    auto setTransform = [&](const Matrix3x2F& m)
    {
        if (memcmp(&current, &m, sizeof(m)) != 0)
        {
            painter.SetTransform(m);
            current = m;
        }
    };
    auto popClip = [&]
    {
        if (clipPushed)
        {
            painter.PopAxisAlignedClip();
            clipPushed = false;
        }
    };
    for (size_t i = 0; i < commands.size(); ++i)
    {
        const Command& c = commands[i];
        if (c.type == CommandType::Nop)
        {
            continue;
        }
        if (IsPush(c.type))
        {
            if (skipGroups && !intersects(c.deviceBounds))
            {
                i = c.index; // skip the whole group, including its pop
                continue;
            }
            popClip();
            if (c.type == CommandType::PushLayer)
            {
                // Pinned (independent) layers can reach opacity 1: no layer needed.
                layerPushed.push_back(c.opacity < 1.0f);
                if (c.opacity < 1.0f)
                {
                    painter.PushOpacityLayer(c.opacity);
                }
            }
            else
            {
                setTransform(c.transform);
                painter.PushRoundedRectangleClip(c.rect, c.radiusX, c.radiusY);
            }
            continue;
        }
        if (IsPop(c.type))
        {
            popClip();
            if (c.type == CommandType::PopLayer)
            {
                if (!layerPushed.empty() && layerPushed.back())
                {
                    painter.PopOpacityLayer();
                }
                if (!layerPushed.empty())
                {
                    layerPushed.pop_back();
                }
            }
            else
            {
                painter.PopRoundedRectangleClip();
            }
            continue;
        }
        if (!intersects(c.deviceBounds))
        {
            continue;
        }
        if (c.owner != lastOwner)
        {
            lastOwner = c.owner;
            ++elementsRendered;
        }
        const bool needsClip = !(c.clip.left <= islandClip.left && c.clip.top <= islandClip.top
            && c.clip.right >= islandClip.right && c.clip.bottom >= islandClip.bottom);
        if (clipPushed && (!needsClip || !RectsEqual(currentClip, c.clip)))
        {
            popClip();
        }
        if (needsClip && !clipPushed)
        {
            // Clip rects are in island space; push them untransformed.
            setTransform(identity);
            painter.PushAxisAlignedClip(c.clip);
            clipPushed = true;
            currentClip = c.clip;
        }
        setTransform(c.transform);
        switch (c.type)
        {
            case CommandType::FillRect:
                painter.FillRectangle(c.rect, c.color);
                break;
            case CommandType::FillRounded:
                painter.FillRoundedRectangle(c.rect, c.radiusX, c.radiusY, c.color);
                break;
            case CommandType::StrokeRounded:
                painter.StrokeRoundedRectangle(c.rect, c.radiusX, c.radiusY, c.thickness, c.color);
                break;
            case CommandType::Text:
            {
                const std::wstring& text = list.strings[c.index];
                TextStyle style;
                style.fontFamily = list.strings[c.index + 1].c_str();
                style.fontSize = c.fontSize;
                style.fontWeight = c.fontWeight;
                painter.DrawText(text.c_str(), static_cast<uint32_t>(text.size()), style, c.rect, c.color);
                break;
            }
            case CommandType::GlyphRun:
            {
                const GlyphRunData& data = list.glyphRuns[c.index];
                GlyphRunDesc run;
                run.fontFace = data.face.Get();
                run.fontEmSize = data.emSize;
                run.glyphIndices = data.indices.data();
                run.glyphAdvances = data.advances.data();
                run.glyphOffsets = data.offsets.empty() ? nullptr : data.offsets.data();
                run.glyphCount = static_cast<uint32_t>(data.indices.size());
                run.bidiLevel = data.bidiLevel;
                painter.DrawGlyphRun(run, c.color);
                break;
            }
            case CommandType::Bitmap:
                painter.DrawBitmap(c.key, c.rect, IsEmptyRect(c.sourceRect) ? nullptr : &c.sourceRect, c.opacity);
                break;
            case CommandType::FillPath:
            case CommandType::StrokePath:
            {
                const PathGeometryData& data = list.paths[c.index];
                PathData path;
                path.verbs = data.verbs.data();
                path.verbCount = static_cast<uint32_t>(data.verbs.size());
                path.points = data.points.data();
                path.pointCount = static_cast<uint32_t>(data.points.size());
                path.nonZeroFill = data.nonZero;
                if (c.type == CommandType::FillPath)
                {
                    painter.FillPath(path, c.color);
                }
                else
                {
                    painter.StrokePath(path, c.thickness, c.color);
                }
                break;
            }
            default:
                break;
        }
    }
    popClip();
}

_Check_return_ HRESULT ImmediateRenderer::RenderIsland(
    _In_ DCompTreeHost* dcompTreeHost,
    _In_ CXamlIslandRoot* island,
    uint32_t frameNumber)
{
    const wf::Size size = island->GetSize();
    const uint32_t width = static_cast<uint32_t>(size.Width);
    const uint32_t height = static_cast<uint32_t>(size.Height);
    if (width == 0 || height == 0)
    {
        return S_OK;
    }
    float scale = RootScale::GetRasterizationScaleForElement(island);
    if (!(scale > 0))
    {
        scale = 1.0f;
    }

    std::lock_guard<std::recursive_mutex> independentLock(m_independent->mutex);
    if (FAILED(m_independent->renderThreadLoss))
    {
        OnDeviceLost(m_independent->renderThreadLoss, frameNumber);
        ReleaseResources();
    }
    CheckTargetChanges(island, frameNumber, width, height, scale);
    BuildAnimationIndex(island);

    auto& slot = m_islands[island];
    if (!slot)
    {
        slot = std::make_unique<IslandState>();
    }
    IslandState& state = *slot;

    HRESULT hr = EnsureIslandResources(dcompTreeHost, island, state, width, height, scale);
    if (FAILED(hr))
    {
        m_islands.erase(island);
        if (IsDeviceLost(hr))
        {
            OnDeviceLost(hr, frameNumber);
            ReleaseResources();
        }
        return hr;
    }
    state.independent.lastUiQpc = QpcMicroseconds();

    // Pass 1: walk the element tree into a display list (DIPs, island space).
    FILE* const statsFile = GetStatsFile();
    const int64_t walkStart = statsFile ? QpcMicroseconds() : 0;
    const uint32_t visitedBefore = m_elementsVisited;
    const uint32_t culledBefore = m_elementsCulled;
    const uint32_t renderedBefore = m_elementsRendered;
    const uint32_t reusedBefore = m_subtreesReused;
    DisplayList list;
    list.records.reserve(state.records.size());
    const RectF islandClip{ 0, 0, width / scale, height / scale };
    WalkContext root;
    root.clip = islandClip;
    root.contentClip = islandClip;
    root.scale = scale;
    {
        // The previous list is consumed (moved from) by subtree reuse; this frame's list replaces it afterwards.
        DisplayList previous = std::move(state.previousWalk);
        state.previousWalk = DisplayList();
        m_previousWalk = (state.fullDamage || DisableIncrementalWalk()) ? nullptr : &previous;
        auto clearPrevious = wil::scope_exit([&] { m_previousWalk = nullptr; });
        Walk(island, root, list);
        list.snapshotQpc = QpcMicroseconds();
    }

    const RectF surface{ 0, 0, static_cast<float>(width), static_cast<float>(height) };
    bool fullDamage = state.fullDamage;

    // Scroll blits: a scroller whose viewport is unchanged and whose content moved by whole pixels moves the
    // viewport's pixels with ScrollPixels; only the exposed strip and changed content are repainted.
    struct Blit
    {
        const CUIElement* presenter;
        RectF rect;       // integer physical pixels
        RectF viewport;   // exact physical pixels
        int32_t dx, dy;
    };
    std::vector<Blit> blits;
    std::unordered_map<const CUIElement*, const ScrollerInfo*> scrollers;
    std::unordered_map<const CUIElement*, std::pair<float, float>> deltas;
    for (const ScrollerInfo& s : list.scrollers)
    {
        scrollers[s.presenter] = &s;
    }
    if (!fullDamage)
    {
        for (const ScrollerInfo& s : list.scrollers)
        {
            auto prev = state.scrollers.find(s.presenter);
            if (!s.hasContent || prev == state.scrollers.end() || !prev->second.hasContent)
            {
                continue;
            }
            const float dx = s.offsetX - prev->second.offsetX;
            const float dy = s.offsetY - prev->second.offsetY;
            deltas[s.presenter] = { dx, dy };
            if (!RectsNear(s.viewport, prev->second.viewport))
            {
                continue;
            }
            const float rdx = std::round(dx);
            const float rdy = std::round(dy);
            if ((rdx == 0 && rdy == 0) || std::fabs(dx - rdx) > 0.01f || std::fabs(dy - rdy) > 0.01f)
            {
                continue;
            }
            const RectF v = IntersectRect({ std::ceil(s.viewport.left), std::ceil(s.viewport.top),
                std::floor(s.viewport.right), std::floor(s.viewport.bottom) }, surface);
            if (IsEmptyRect(v) || std::fabs(rdx) >= v.right - v.left || std::fabs(rdy) >= v.bottom - v.top)
            {
                continue;
            }
            if (std::any_of(blits.begin(), blits.end(), [&](const Blit& b) { return RectsIntersect(b.rect, v); }))
            {
                continue;
            }
            blits.push_back({ s.presenter, v, s.viewport, static_cast<int32_t>(rdx), static_cast<int32_t>(rdy) });
        }
    }

    auto findBlit = [&](const CUIElement* presenter) -> const Blit*
    {
        for (const Blit& b : blits)
        {
            if (b.presenter == presenter) return &b;
        }
        return nullptr;
    };

    // A record moved with a blit if its innermost scroller moved by exactly the delta of the nearest blitted scroller
    // in its scroller chain.
    auto movedWithBlit = [&](const ElementRecord& r) -> bool
    {
        if (blits.empty() || r.scroller == nullptr)
        {
            return false;
        }
        auto delta = deltas.find(r.scroller);
        if (delta == deltas.end())
        {
            return false;
        }
        for (const CUIElement* p = r.scroller; p != nullptr;)
        {
            if (const Blit* b = findBlit(p))
            {
                return std::fabs(delta->second.first - b->dx) < 0.01f && std::fabs(delta->second.second - b->dy) < 0.01f;
            }
            auto found = scrollers.find(p);
            p = found != scrollers.end() ? found->second->parent : nullptr;
        }
        return false;
    };

    // Damage: elements whose drawing changed contribute their old and new bounds.
    std::vector<RectF> damage;
    if (!fullDamage)
    {
        auto add = [&](const RectF& r)
        {
            if (IsEmptyRect(r))
            {
                return;
            }
            if (damage.size() < c_maxRawDamageRects)
            {
                damage.push_back(r);
            }
            else
            {
                fullDamage = true;
            }
        };
        // Old pixels also moved with every blit they overlapped.
        auto addOld = [&](const RectF& r)
        {
            add(r);
            for (const Blit& b : blits)
            {
                if (RectsIntersect(r, b.rect))
                {
                    add(IntersectRect(OffsetRect(r, static_cast<float>(b.dx), static_cast<float>(b.dy)), b.rect));
                }
            }
        };
        // Unchanged, unmoved drawing inside a blitted viewport was moved by the blit unless it is uniform there.
        auto addIfBlitted = [&](const ElementRecord& r)
        {
            for (const Blit& b : blits)
            {
                if (RectsIntersect(r.bounds, b.rect) && !Contains(r.cover, b.rect))
                {
                    add(IntersectRect(r.bounds, b.rect));
                    add(IntersectRect(OffsetRect(r.bounds, static_cast<float>(b.dx), static_cast<float>(b.dy)), b.rect));
                }
            }
        };

        for (const Blit& b : blits)
        {
            const RectF& v = b.rect;
            if (b.dx > 0) add({ v.left, v.top, v.left + b.dx, v.bottom });
            if (b.dx < 0) add({ v.right + b.dx, v.top, v.right, v.bottom });
            if (b.dy > 0) add({ v.left, v.top, v.right, v.top + b.dy });
            if (b.dy < 0) add({ v.left, v.bottom + b.dy, v.right, v.bottom });
            // Partial pixels between the exact viewport and the blitted (inner-rounded) rect.
            const RectF outer = IntersectRect({ std::floor(b.viewport.left), std::floor(b.viewport.top),
                std::ceil(b.viewport.right), std::ceil(b.viewport.bottom) }, surface);
            add({ outer.left, outer.top, outer.right, v.top });
            add({ outer.left, v.bottom, outer.right, outer.bottom });
            add({ outer.left, v.top, v.left, v.bottom });
            add({ v.right, v.top, outer.right, v.bottom });
        }

        for (const auto& entry : list.records)
        {
            const ElementRecord& now = entry.second;
            auto found = state.records.find(entry.first);
            const ElementRecord* old = found != state.records.end() ? &found->second : nullptr;

            if (now.hasContent || (old && old->hasContent))
            {
                if (!old || !old->hasContent)
                {
                    add(now.bounds);
                }
                else if (!now.hasContent)
                {
                    addOld(old->bounds);
                }
                else if (old->signature == now.signature && old->scroller == now.scroller && RectsEqual(old->relBounds, now.relBounds))
                {
                    if (movedWithBlit(now))
                    {
                        // Pixels already moved; newly exposed parts are in the exposed strips.
                    }
                    else if (!RectsEqual(old->bounds, now.bounds))
                    {
                        addOld(old->bounds);
                        add(now.bounds);
                    }
                    else
                    {
                        addIfBlitted(now);
                    }
                }
                else
                {
                    addOld(old->bounds);
                    add(now.bounds);
                }
            }

            if (now.hasGroup || (old && old->hasGroup))
            {
                const bool same = old && old->hasGroup && now.hasGroup && old->groupSignature == now.groupSignature;
                if (!same)
                {
                    if (old && old->hasGroup) addOld(old->groupBounds);
                    if (now.hasGroup) add(now.groupBounds);
                }
                else if (!IsEmptyRect(now.roundedClip))
                {
                    // Blitted pixels ignore the rounded corners' mask: repaint the corners inside blitted viewports.
                    for (const Blit& b : blits)
                    {
                        if (!RectsIntersect(now.roundedClip, b.rect))
                        {
                            continue;
                        }
                        const RectF& c = now.roundedClip;
                        const float r = std::ceil(now.roundedRadius) + 1;
                        const RectF corners[] = {
                            { c.left - 1, c.top - 1, c.left + r, c.top + r }, { c.right - r, c.top - 1, c.right + 1, c.top + r },
                            { c.left - 1, c.bottom - r, c.left + r, c.bottom + 1 }, { c.right - r, c.bottom - r, c.right + 1, c.bottom + 1 },
                        };
                        for (const RectF& corner : corners)
                        {
                            add(IntersectRect(corner, b.rect));
                            add(IntersectRect(OffsetRect(corner, static_cast<float>(b.dx), static_cast<float>(b.dy)), b.rect));
                        }
                    }
                }
            }
        }
        for (const auto& entry : state.records)
        {
            if (list.records.find(entry.first) == list.records.end())
            {
                if (entry.second.hasContent) addOld(entry.second.bounds);
                if (entry.second.hasGroup) addOld(entry.second.groupBounds);
            }
        }
    }

    AppendIndependentDamage(state, damage, !blits.empty(), fullDamage);
    bool ownFull = fullDamage;
    std::vector<RectF> ownDamage;
    if (!fullDamage)
    {
        for (auto& r : damage)
        {
            r = IntersectRect(r, surface);
        }
        damage.erase(std::remove_if(damage.begin(), damage.end(), [](const RectF& r) { return IsEmptyRect(r); }), damage.end());
        if (damage.empty() && blits.empty())
        {
            state.records = list.records;
            SaveScrollers(state, list);
            state.previousWalk = std::move(list);
            OnIslandPresented(state, frameNumber);
            return S_OK;
        }
        ownDamage = damage;
        for (const Blit& b : blits)
        {
            ownDamage.push_back(b.rect);
        }
        if (!UnionPresentedDamage())
        {
            CoalesceDamage(damage, c_maxDamageRects);
        }
        else if (state.presentedFull)
        {
            fullDamage = true;
        }
        else
        {
            damage.insert(damage.end(), state.presentedDamage.begin(), state.presentedDamage.end());
            CoalesceDamage(damage, c_maxDamageRects);
        }
    }

    if (!fullDamage)
    {
        for (const Blit& b : blits)
        {
            if (FAILED(state.painter->ScrollPixels(b.rect, b.dx, b.dy)))
            {
                fullDamage = true;
                ownFull = true;
                break;
            }
        }
    }

    // Pass 2: replay commands that intersect the damage.
    auto paint = [&](IImmediatePainter& painter)
    {
        ReplayCommands(painter, list.commands, list, islandClip, fullDamage, damage, true, m_elementsRendered);
    };

    // Upload decoded images the painter has not seen, and release ones no longer drawn.
    constexpr uint64_t c_bitmapIdleFrames = 120;
    ++state.frames;
    for (const auto& decoded : list.bitmaps)
    {
        const uint64_t key = reinterpret_cast<uint64_t>(decoded.get());
        auto found = state.bitmaps.find(key);
        if (found == state.bitmaps.end())
        {
            void* pixels = nullptr;
            XINT32 stride = 0;
            XUINT32 w = 0;
            XUINT32 h = 0;
            if (FAILED(decoded->Lock(&pixels, &stride, &w, &h)))
            {
                continue;
            }
            BitmapSource source;
            source.key = key;
            source.generation = 1;
            source.width = w;
            source.height = h;
            source.stride = static_cast<uint32_t>(stride);
            source.pixels = static_cast<const uint8_t*>(pixels);
            const HRESULT ensureHr = state.painter->EnsureBitmap(source);
            IGNOREHR(decoded->Unlock());
            if (FAILED(ensureHr))
            {
                continue;
            }
            found = state.bitmaps.emplace(key, IslandState::BitmapEntry{ decoded, 0 }).first;
        }
        found->second.lastUsed = state.frames;
    }
    for (auto it = state.bitmaps.begin(); it != state.bitmaps.end();)
    {
        if (state.frames - it->second.lastUsed > c_bitmapIdleFrames)
        {
            state.painter->ReleaseBitmap(it->first);
            it = state.bitmaps.erase(it);
        }
        else
        {
            ++it;
        }
    }

    const int64_t paintStart = statsFile ? QpcMicroseconds() : 0;
    hr = PaintTracedFrame(*state.painter, frameNumber, reinterpret_cast<uint64_t>(island),
        fullDamage ? nullptr : damage.data(), fullDamage ? 0 : static_cast<uint32_t>(damage.size()), paint);
    hr = ApplyDeviceLostSimulation(hr, frameNumber);
    if (statsFile)
    {
        const int64_t paintEnd = QpcMicroseconds();
        double damagePixels = 0;
        if (fullDamage)
        {
            damagePixels = static_cast<double>(width) * height;
        }
        else
        {
            for (const RectF& d : damage)
            {
                damagePixels += static_cast<double>(d.right - d.left) * (d.bottom - d.top);
            }
        }
        fprintf(statsFile, "%u %p %zu %u %u %u %zu %zu %d %d %zu %.0f %d %lld %lld %u\n", frameNumber, static_cast<void*>(island),
            list.records.size(), m_elementsVisited - visitedBefore, m_elementsRendered - renderedBefore,
            m_elementsCulled - culledBefore, list.commands.size(), blits.size(), blits.empty() ? 0 : blits[0].dx,
            blits.empty() ? 0 : blits[0].dy, fullDamage ? 0 : damage.size(), damagePixels, fullDamage ? 1 : 0,
            static_cast<long long>(paintStart - walkStart), static_cast<long long>(paintEnd - paintStart),
            m_subtreesReused - reusedBefore);
        fflush(statsFile);
    }

    if (m_mode == Mode::ImmediateGdiPlus)
    {
        GdiPlus::ResourceChurn churn;
        if (SUCCEEDED(GdiPlus::GetLastFrameResourceChurn(*state.painter, &churn)))
        {
            TraceResourceChurn(m_mode, frameNumber, churn.created, churn.destroyed);
        }
    }

    if (FAILED(hr))
    {
        if (IsDeviceLost(hr))
        {
            OnDeviceLost(hr, frameNumber);
            ReleaseResources();
        }
        else
        {
            state.fullDamage = true;
            state.records.clear();
            state.scrollers.clear();
        }
        return hr;
    }

    state.fullDamage = false;
    if (!ownFull)
    {
        CoalesceDamage(ownDamage, c_maxDamageRects);
    }
    state.presentedFull = ownFull;
    state.presentedDamage = ownFull ? std::vector<RectF>() : std::move(ownDamage);
    state.records = list.records;
    SaveScrollers(state, list);
    state.previousWalk = std::move(list);
    OnIslandPresented(state, frameNumber);

    if (!GetCapturePath().empty())
    {
        LOG_IF_FAILED(CapturePresentedBuffer(state.swapChain.Get(), GetCapturePath().c_str()));
    }
    return S_OK;
}

void ImmediateRenderer::SaveScrollers(_Inout_ IslandState& state, const DisplayList& list)
{
    state.scrollers.clear();
    for (const ScrollerInfo& s : list.scrollers)
    {
        state.scrollers[s.presenter] = s;
    }
}

#include "ImmediateRenderer.Independent.inl"
