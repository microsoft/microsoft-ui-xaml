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
#include <algorithm>
#include <cmath>

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
};

ImmediateRenderer::ImmediateRenderer(Mode mode) noexcept
    : m_mode(mode)
{
}

ImmediateRenderer::~ImmediateRenderer()
{
    ReleaseResources();
}

void ImmediateRenderer::ReleaseResources() noexcept
{
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
    }
}

namespace
{
    struct Emitter
    {
        ImmediateRenderer::DisplayList& list;
        const CUIElement* element;
        Matrix3x2F transform;
        const CMILMatrix& world;
        RectF clip;
        float scale;
        RectF elementBounds{};
        Fnv signature;
        bool any = false;

        void Push(ImmediateRenderer::Command command, const RectF& localInkBounds)
        {
            command.transform = transform;
            command.clip = clip;
            RectF device = IntersectRect(TransformRectBounds(world, localInkBounds), clip);
            if (IsEmptyRect(device))
            {
                return;
            }
            device = { std::floor(device.left * scale) - 1, std::floor(device.top * scale) - 1,
                       std::ceil(device.right * scale) + 1, std::ceil(device.bottom * scale) + 1 };
            command.deviceBounds = device;
            elementBounds = UnionRect(elementBounds, device);

            signature.AddValue(command.type);
            signature.AddValue(command.transform);
            signature.AddValue(command.clip);
            signature.AddValue(command.rect);
            signature.AddValue(command.color);
            signature.AddValue(command.radiusX);
            signature.AddValue(command.radiusY);
            signature.AddValue(command.thickness);
            signature.AddValue(command.fontSize);
            signature.AddValue(command.fontWeight);
            if (command.type == ImmediateRenderer::CommandType::Text)
            {
                const auto& text = list.strings[command.textIndex];
                const auto& family = list.strings[command.textIndex + 1];
                signature.Add(text.data(), text.size() * sizeof(wchar_t));
                signature.Add(family.data(), family.size() * sizeof(wchar_t));
            }
            list.commands.push_back(command);
            any = true;
        }

        void Fill(const RectF& rect, float rx, float ry, const ColorF& color)
        {
            if (IsEmptyRect(rect)) return;
            ImmediateRenderer::Command c;
            c.type = (rx > 0 || ry > 0) ? ImmediateRenderer::CommandType::FillRounded : ImmediateRenderer::CommandType::FillRect;
            c.rect = rect;
            c.radiusX = rx;
            c.radiusY = ry;
            c.color = color;
            Push(c, rect);
        }

        void Stroke(const RectF& rect, float rx, float ry, float thickness, const ColorF& color)
        {
            if (thickness <= 0 || rect.right < rect.left || rect.bottom < rect.top) return;
            ImmediateRenderer::Command c;
            c.type = ImmediateRenderer::CommandType::StrokeRounded;
            c.rect = rect;
            c.radiusX = rx;
            c.radiusY = ry;
            c.thickness = thickness;
            c.color = color;
            const float h = thickness / 2;
            Push(c, { rect.left - h, rect.top - h, rect.right + h, rect.bottom + h });
        }

        void Text(std::wstring text, std::wstring family, float fontSize, uint32_t weight, const RectF& layout, const RectF& ink, const ColorF& color)
        {
            if (text.empty() || fontSize <= 0) return;
            ImmediateRenderer::Command c;
            c.type = ImmediateRenderer::CommandType::Text;
            c.rect = layout;
            c.fontSize = fontSize;
            c.fontWeight = weight;
            c.color = color;
            c.textIndex = static_cast<uint32_t>(list.strings.size());
            list.strings.push_back(std::move(text));
            list.strings.push_back(std::move(family));
            Push(c, ink);
        }
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
}

void ImmediateRenderer::EmitContent(
    _In_ CUIElement* element,
    const CMILMatrix& world,
    const RectF& clip,
    float opacity,
    float scale,
    _Inout_ DisplayList& list)
{
    Emitter emitter{ list, element, ToMatrix(world), world, clip, scale };
    const float width = element->GetActualWidth();
    const float height = element->GetActualHeight();
    const RectF bounds{ 0, 0, width, height };

    const KnownTypeIndex type = element->GetTypeIndex();

    if (element->OfTypeByIndex<KnownTypeIndex::Panel>()
        || element->OfTypeByIndex<KnownTypeIndex::Border>()
        || (element->OfTypeByIndex<KnownTypeIndex::ContentPresenter>()
            && !element->OfTypeByIndex<KnownTypeIndex::ListViewBaseItemPresenter>()))
    {
        auto fe = static_cast<CFrameworkElement*>(element);
        const xref_ptr<CBrush> background = fe->GetBackgroundBrush();
        const xref_ptr<CBrush> border = fe->GetBorderBrush();
        const XTHICKNESS thickness = fe->GetBorderThickness();
        const float radius = MaxOf(fe->GetCornerRadius());
        const float t = MaxOf(thickness);

        ColorF backgroundColor;
        const BrushResult backgroundResult = ResolveBrush(background.get(), opacity, &backgroundColor);
        ColorF borderColor;
        const BrushResult borderResult = t > 0 ? ResolveBrush(border.get(), opacity, &borderColor) : BrushResult::None;
        if (backgroundResult == BrushResult::Unsupported || borderResult == BrushResult::Unsupported)
        {
            ReportUnsupported(element, L"Brush");
        }

        if (backgroundResult == BrushResult::Color)
        {
            RectF fill = bounds;
            float fillRadius = radius;
            if (fe->GetBackgroundSizing() == DirectUI::BackgroundSizing::InnerBorderEdge && borderResult == BrushResult::Color)
            {
                fill = { thickness.left, thickness.top, width - thickness.right, height - thickness.bottom };
                fillRadius = std::max(0.0f, radius - t / 2);
            }
            emitter.Fill(fill, fillRadius, fillRadius, backgroundColor);
        }

        if (borderResult == BrushResult::Color)
        {
            if (!IsUniform(thickness) && radius == 0)
            {
                emitter.Fill({ 0, 0, width, thickness.top }, 0, 0, borderColor);
                emitter.Fill({ 0, height - thickness.bottom, width, height }, 0, 0, borderColor);
                emitter.Fill({ 0, thickness.top, thickness.left, height - thickness.bottom }, 0, 0, borderColor);
                emitter.Fill({ width - thickness.right, thickness.top, width, height - thickness.bottom }, 0, 0, borderColor);
            }
            else
            {
                const float h = t / 2;
                const float r = std::max(0.0f, radius - h);
                emitter.Stroke({ h, h, width - h, height - h }, r, r, t, borderColor);
            }
        }
    }
    else if (type == KnownTypeIndex::Rectangle || type == KnownTypeIndex::Ellipse)
    {
        auto shape = static_cast<CShape*>(element);
        const xref_ptr<CBrush> stroke = shape->GetStroke();
        const float t = stroke ? shape->GetStrokeThickness() : 0.0f;
        ColorF fillColor;
        const BrushResult fillResult = ResolveBrush(shape->m_pFill, opacity, &fillColor);
        ColorF strokeColor;
        const BrushResult strokeResult = t > 0 ? ResolveBrush(stroke.get(), opacity, &strokeColor) : BrushResult::None;
        if (fillResult == BrushResult::Unsupported || strokeResult == BrushResult::Unsupported)
        {
            ReportUnsupported(element, L"Brush");
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
            emitter.Fill(geometry, rx, ry, fillColor);
        }
        if (strokeResult == BrushResult::Color)
        {
            emitter.Stroke(geometry, rx, ry, t, strokeColor);
        }
    }
    else if (type == KnownTypeIndex::TextBlock || type == KnownTypeIndex::TextBoxView)
    {
        std::wstring text;
        bool wrap = false;
        XTHICKNESS padding{};
        if (type == KnownTypeIndex::TextBlock)
        {
            auto textBlock = static_cast<CTextBlock*>(element);
            const xstring_ptr value = textBlock->GetText();
            if (!value.IsNullOrEmpty())
            {
                text.assign(value.GetBuffer(), value.GetCount());
            }
            else if (CInlineCollection* inlines = textBlock->GetInlineCollection())
            {
                XUINT32 count = 0;
                const WCHAR* chars = nullptr;
                bool takeOwnership = false;
                if (SUCCEEDED(inlines->GetText(true, &count, &chars, &takeOwnership)) && chars != nullptr)
                {
                    text.assign(chars, count);
                    if (takeOwnership)
                    {
                        delete[] chars;
                    }
                }
            }
            padding = textBlock->GetPadding();
            CValue wrapping;
            if (SUCCEEDED(element->GetValueByIndex(KnownPropertyIndex::TextBlock_TextWrapping, &wrapping)))
            {
                wrap = wrapping.AsEnum() != static_cast<uint32_t>(DirectUI::TextWrapping::NoWrap);
            }
        }
        else
        {
            // TextBoxView draws its owner's text; find the TextBox in the template's ancestor chain.
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
        }

        const TextFormatting* formatting = nullptr;
        if (!text.empty() && SUCCEEDED(element->GetTextFormatting(&formatting)) && formatting != nullptr)
        {
            ColorF color;
            const BrushResult result = ResolveBrush(formatting->m_pForeground, opacity, &color);
            if (result == BrushResult::Unsupported)
            {
                ReportUnsupported(element, L"Foreground brush");
            }
            else if (result == BrushResult::Color)
            {
                const float fontSize = formatting->m_eFontSize;
                RectF layout{ padding.left, padding.top, width - padding.right, height - padding.bottom };
                // Layout came from XAML's text stack; a different shaper may measure a few pixels wider. Widen
                // single-line layouts so they never wrap, and damage a slightly inflated ink rect.
                if (!wrap)
                {
                    layout.right += fontSize * 2;
                }
                else
                {
                    // Headers are often arranged at exactly their measured width; a little slack keeps one-line
                    // wrapping text from breaking its last word.
                    layout.right += fontSize / 2;
                }
                const float overhang = std::max(2.0f, fontSize / 4);
                const RectF ink{ layout.left - overhang, layout.top - overhang, layout.right + overhang, layout.bottom + overhang };
                emitter.Text(std::move(text), ResolveFontFamily(formatting->m_pFontFamily), fontSize,
                    static_cast<uint32_t>(formatting->m_nFontWeight), layout, ink, color);
            }
        }
    }
    else if (element->OfTypeByIndex<KnownTypeIndex::Shape>())
    {
        ReportUnsupported(element, L"Shape geometry");
    }
    else if (type == KnownTypeIndex::Image
        || type == KnownTypeIndex::RichTextBlock
        || type == KnownTypeIndex::RichTextBlockOverflow
        || type == KnownTypeIndex::Glyphs
        || type == KnownTypeIndex::SwapChainPanel
        || type == KnownTypeIndex::MediaPlayerPresenter
        || element->OfTypeByIndex<KnownTypeIndex::ListViewBaseItemPresenter>()
        || element->OfTypeByIndex<KnownTypeIndex::CalendarViewBaseItem>())
    {
        ReportUnsupported(element, L"Element content");
    }

    if (emitter.any)
    {
        ++m_elementsRendered;
        list.records[element] = { emitter.elementBounds, emitter.signature.hash };
    }
}

void ImmediateRenderer::Walk(
    _In_ CUIElement* element,
    const CMILMatrix& parentWorld,
    const RectF& parentClip,
    float parentOpacity,
    float scale,
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
            CUIElementCollection* children = element->GetChildren();
            if (children != nullptr && children->GetCount() > 0)
            {
                ReportUnsupported(element, L"Open popups");
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

    const float opacity = parentOpacity * element->GetOpacityLocal();
    if (!(opacity > 0))
    {
        return;
    }

    RectF clip = parentClip;
    const bool hasLayoutClip = element->HasLayoutClip();
    const bool layoutClipIsAncestor = element->ShouldApplyLayoutClipAsAncestorClip();
    if (hasLayoutClip && layoutClipIsAncestor)
    {
        clip = IntersectRect(clip, TransformRectBounds(parentWorld, FromXRect(element->LayoutClipGeometry->m_rc)));
    }

    CMILMatrix world = parentWorld;
    {
        CMILMatrix local;
        if (!element->GetLocalTransform(TransformRetrievalOptions::None, &local))
        {
            world = local;
            world.Append(parentWorld);
        }
    }

    if (element->HasActiveProjection())
    {
        ReportUnsupported(element, L"Projection");
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
            CMILMatrix clipWorld = world;
            if (rectangleClip->m_pTransform != nullptr)
            {
                CMILMatrix clipTransform(true);
                rectangleClip->m_pTransform->GetTransform(&clipTransform);
                clipWorld = clipTransform;
                clipWorld.Append(world);
            }
            clip = IntersectRect(clip, TransformRectBounds(clipWorld, FromXRect(rectangleClip->m_rc)));
        }
        else
        {
            ReportUnsupported(element, L"Non-rectangular clip");
        }
    }

    if (hasLayoutClip && !layoutClipIsAncestor)
    {
        clip = IntersectRect(clip, TransformRectBounds(world, FromXRect(element->LayoutClipGeometry->m_rc)));
    }

    if (IsEmptyRect(clip))
    {
        return;
    }

    EmitContent(element, world, clip, opacity, scale, list);

    CUIElement** children = nullptr;
    XUINT32 childCount = 0;
    element->GetChildrenInRenderOrder(&children, &childCount);
    for (XUINT32 i = 0; i < childCount; ++i)
    {
        if (children[i] != nullptr)
        {
            Walk(children[i], world, clip, opacity, scale, list);
        }
    }
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
            ReleaseResources();
        }
        return hr;
    }

    // Pass 1: walk the element tree into a display list (DIPs, island space).
    DisplayList list;
    list.records.reserve(state.records.size());
    const RectF islandClip{ 0, 0, width / scale, height / scale };
    Walk(island, CMILMatrix(true), islandClip, 1.0f, scale, list);

    // Damage: elements whose drawing changed contribute their old and new bounds.
    std::vector<RectF> damage;
    bool fullDamage = state.fullDamage;
    if (!fullDamage)
    {
        auto add = [&](const RectF& r)
        {
            if (damage.size() < c_maxRawDamageRects)
            {
                damage.push_back(r);
            }
            else
            {
                fullDamage = true;
            }
        };
        for (const auto& entry : list.records)
        {
            auto old = state.records.find(entry.first);
            if (old == state.records.end())
            {
                add(entry.second.bounds);
            }
            else if (old->second.signature != entry.second.signature || !RectsEqual(old->second.bounds, entry.second.bounds))
            {
                add(old->second.bounds);
                add(entry.second.bounds);
            }
        }
        for (const auto& entry : state.records)
        {
            if (list.records.find(entry.first) == list.records.end())
            {
                add(entry.second.bounds);
            }
        }
    }

    const RectF surface{ 0, 0, static_cast<float>(width), static_cast<float>(height) };
    if (!fullDamage)
    {
        for (auto& r : damage)
        {
            r = IntersectRect(r, surface);
        }
        damage.erase(std::remove_if(damage.begin(), damage.end(), [](const RectF& r) { return IsEmptyRect(r); }), damage.end());
        if (damage.empty())
        {
            state.records = std::move(list.records);
            return S_OK;
        }
        CoalesceDamage(damage, c_maxDamageRects);
    }

    // Pass 2: replay commands that intersect the damage.
    auto paint = [&](IImmediatePainter& painter)
    {
        painter.Clear({ 0, 0, 0, 0 });
        const Matrix3x2F identity;
        Matrix3x2F current = identity;
        bool clipPushed = false;
        RectF currentClip{};
        for (const Command& c : list.commands)
        {
            if (!fullDamage && std::none_of(damage.begin(), damage.end(), [&](const RectF& d) { return RectsIntersect(d, c.deviceBounds); }))
            {
                continue;
            }
            const bool needsClip = !(c.clip.left <= islandClip.left && c.clip.top <= islandClip.top
                && c.clip.right >= islandClip.right && c.clip.bottom >= islandClip.bottom);
            if (clipPushed && (!needsClip || !RectsEqual(currentClip, c.clip)))
            {
                painter.PopAxisAlignedClip();
                clipPushed = false;
            }
            if (needsClip && !clipPushed)
            {
                // Clip rects are in island space; push them untransformed.
                if (memcmp(&current, &identity, sizeof(identity)) != 0)
                {
                    painter.SetTransform(identity);
                    current = identity;
                }
                painter.PushAxisAlignedClip(c.clip);
                clipPushed = true;
                currentClip = c.clip;
            }
            if (memcmp(&current, &c.transform, sizeof(c.transform)) != 0)
            {
                painter.SetTransform(c.transform);
                current = c.transform;
            }
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
                    const std::wstring& text = list.strings[c.textIndex];
                    TextStyle style;
                    style.fontFamily = list.strings[c.textIndex + 1].c_str();
                    style.fontSize = c.fontSize;
                    style.fontWeight = c.fontWeight;
                    painter.DrawText(text.c_str(), static_cast<uint32_t>(text.size()), style, c.rect, c.color);
                    break;
                }
            }
        }
        if (clipPushed)
        {
            painter.PopAxisAlignedClip();
        }
    };

    hr = PaintTracedFrame(*state.painter, frameNumber, reinterpret_cast<uint64_t>(island),
        fullDamage ? nullptr : damage.data(), fullDamage ? 0 : static_cast<uint32_t>(damage.size()), paint);

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
            ReleaseResources();
        }
        else
        {
            state.fullDamage = true;
            state.records.clear();
        }
        return hr;
    }

    state.fullDamage = false;
    state.records = std::move(list.records);

    if (!GetCapturePath().empty())
    {
        LOG_IF_FAILED(CapturePresentedBuffer(state.swapChain.Get(), GetCapturePath().c_str()));
    }
    return S_OK;
}
