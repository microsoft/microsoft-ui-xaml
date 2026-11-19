// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "D2DSoftwarePainterTests.h"
#include <ImmediatePainter.h>
#include <D2DPainter.h>
#include <d3d11.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <algorithm>
#include <cstring>
#include <vector>

namespace RenderingBackendTests
{
    using namespace RenderingBackend;
    using namespace WEX::Common;
    using namespace WEX::Logging;
    using namespace WEX::TestExecution;
    using Microsoft::WRL::ComPtr;

    namespace
    {
        constexpr uint32_t c_size = 64;
        constexpr ColorF c_white{ 1, 1, 1, 1 };
        constexpr ColorF c_red{ 1, 0, 0, 1 };
        constexpr ColorF c_blue{ 0, 0, 1, 1 };

        // S2 (dashboard) scene shape: rounded tiles with borders, labels and values, a translucent group, a clipped
        // header, and a fractional translation so antialiasing and text rasterization are exercised.
        constexpr uint32_t c_sceneWidth = 256;
        constexpr uint32_t c_sceneHeight = 160;

        void PaintDashboardFrame(IImmediatePainter& p)
        {
            p.Clear({ 0.95f, 0.95f, 0.96f, 1 });
            TextStyle label{};
            label.fontSize = 11;
            TextStyle value{};
            value.fontSize = 18;
            value.fontWeight = 600;

            p.PushAxisAlignedClip({ 0, 0, 256, 20 });
            p.FillRectangle({ 0, 0, 256, 20 }, { 0.13f, 0.38f, 0.75f, 1 });
            p.DrawText(L"Dashboard", 9, label, { 6, 3, 300, 20 }, c_white);
            p.PopAxisAlignedClip();

            const wchar_t* values[] = { L"42.7%", L"1,024", L"OK", L"3.14", L"-8", L"99+" };
            for (uint32_t i = 0; i < 6; ++i)
            {
                Matrix3x2F at{};
                at.dx = 4.5f + (i % 3) * 84.0f;
                at.dy = 26.25f + (i / 3) * 66.0f;
                p.SetTransform(at);
                const bool translucent = i == 4;
                if (translucent)
                {
                    p.PushOpacityLayer(0.6f);
                }
                p.FillRoundedRectangle({ 0, 0, 78, 60 }, 6, 6, c_white);
                p.StrokeRoundedRectangle({ 0.5f, 0.5f, 77.5f, 59.5f }, 6, 6, 1, { 0.8f, 0.8f, 0.82f, 1 });
                p.FillRoundedRectangle({ 6, 46, 6 + 10.0f * (i + 1), 52 }, 3, 3, { 0.2f, 0.65f, 0.3f, 1 });
                p.DrawText(L"Metric", 6, label, { 6, 4, 72, 18 }, { 0.35f, 0.35f, 0.38f, 1 });
                p.DrawText(values[i], static_cast<uint32_t>(wcslen(values[i])), value, { 6, 18, 72, 44 }, { 0.1f, 0.1f, 0.12f, 1 });
                if (translucent)
                {
                    p.PopOpacityLayer();
                }
            }
            p.SetTransform({});
        }

        std::vector<uint8_t> ReadAll(IImmediatePainter& painter, uint32_t width, uint32_t height)
        {
            std::vector<uint8_t> bytes(static_cast<size_t>(width) * height * 4);
            VERIFY_SUCCEEDED(painter.ReadPixels(bytes.data(), width * 4, static_cast<uint32_t>(bytes.size())));
            return bytes;
        }

        // FNV-1a; logged so a golden hash can be recorded per WARP (d3d10warp.dll) version.
        uint64_t Hash(const std::vector<uint8_t>& bytes)
        {
            uint64_t hash = 14695981039346656037ull;
            for (const uint8_t b : bytes)
            {
                hash = (hash ^ b) * 1099511628211ull;
            }
            return hash;
        }

        template <typename PaintFn>
        FrameStats Paint(IImmediatePainter& painter, PaintFn&& paint, const RectF* damage = nullptr, uint32_t damageCount = 0)
        {
            VERIFY_SUCCEEDED(painter.BeginFrame(damage, damageCount));
            paint(painter);
            FrameStats stats{};
            VERIFY_SUCCEEDED(painter.EndFrame(&stats));
            return stats;
        }

        void PaintScene(IImmediatePainter& p)
        {
            Paint(p, [](IImmediatePainter& painter) { PaintDashboardFrame(painter); });
        }

        void PaintSolid(IImmediatePainter& p, const ColorF& color, const RectF* damage = nullptr, uint32_t damageCount = 0)
        {
            Paint(p, [&](IImmediatePainter& painter) { painter.Clear(color); }, damage, damageCount);
        }

        ComPtr<ID3D11Device> CreateWarpDevice(_Out_opt_ DeviceIdentity* identityOut = nullptr)
        {
            ComPtr<ID3D11Device> device;
            DeviceIdentity identity{};
            VERIFY_SUCCEEDED(CreateD3DDeviceForMode(Mode::ImmediateD2DSoftware, &device, &identity));
            VERIFY_IS_TRUE(identity.isWarp);
            if (identityOut)
            {
                *identityOut = identity;
            }
            return device;
        }

        std::unique_ptr<ID2DImmediatePainter> CreateWarpPainter(ID3D11Device* device, uint32_t width = c_size, uint32_t height = c_size)
        {
            std::unique_ptr<ID2DImmediatePainter> painter;
            VERIFY_SUCCEEDED(CreateD2DPainter(Mode::ImmediateD2DSoftware, device, PainterOptions{ width, height, 1.0f }, painter),
                L"CreateD2DPainter(d2d-sw) on a WARP device");
            VERIFY_IS_NOT_NULL(painter.get());
            return painter;
        }

        ComPtr<IDXGISwapChain1> CreateCompositionSwapChain(ID3D11Device* device, uint32_t width, uint32_t height)
        {
            ComPtr<IDXGIDevice> dxgiDevice;
            ComPtr<IDXGIAdapter> adapter;
            ComPtr<IDXGIFactory2> factory;
            VERIFY_SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
            VERIFY_SUCCEEDED(dxgiDevice->GetAdapter(&adapter));
            VERIFY_SUCCEEDED(adapter->GetParent(IID_PPV_ARGS(&factory)));

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
            ComPtr<IDXGISwapChain1> swapChain;
            VERIFY_SUCCEEDED(factory->CreateSwapChainForComposition(device, &desc, nullptr, &swapChain));
            return swapChain;
        }

        ULONG RefCount(IUnknown* object)
        {
            object->AddRef();
            return object->Release();
        }

        // A D3D11 device on WARP whose removal can be forced: D3D11On12 over a D3D12 WARP device, removed with
        // ID3D12Device5::RemoveDevice. Returns false (and logs Skipped) when D3D12 WARP is not available.
        struct RemovableWarpDevice
        {
            ComPtr<ID3D12Device> d3d12;
            ComPtr<ID3D12CommandQueue> queue;
            ComPtr<ID3D11Device> d3d11;
            ComPtr<ID3D11DeviceContext> context;

            bool Create()
            {
                ComPtr<IDXGIFactory4> factory;
                ComPtr<IDXGIAdapter> warp;
                VERIFY_SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
                VERIFY_SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
                const HRESULT hr = D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12));
                if (FAILED(hr))
                {
                    Log::Result(TestResults::Skipped, String().Format(L"D3D12 WARP unavailable (0x%08X); device removal cannot be simulated.", hr));
                    return false;
                }
                D3D12_COMMAND_QUEUE_DESC queueDesc{};
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                VERIFY_SUCCEEDED(d3d12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)));
                IUnknown* queues[] = { queue.Get() };
                VERIFY_SUCCEEDED(D3D11On12CreateDevice(d3d12.Get(), D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, queues, 1, 0,
                    &d3d11, &context, nullptr));
                return true;
            }

            void Remove()
            {
                ComPtr<ID3D12Device5> device5;
                VERIFY_SUCCEEDED(d3d12.As(&device5), L"ID3D12Device5 (RemoveDevice) is required to simulate removal");
                device5->RemoveDevice();
                context->Flush();
            }
        };

        uint32_t PixelAt(const std::vector<uint8_t>& bytes, uint32_t width, uint32_t x, uint32_t y)
        {
            uint32_t value;
            memcpy(&value, &bytes[(static_cast<size_t>(y) * width + x) * 4], sizeof(value));
            return value; // 0xAARRGGBB
        }

        ComPtr<IDWriteFontFace> CreateSegoeFontFace()
        {
            ComPtr<IDWriteFactory> factory;
            ComPtr<IDWriteFontCollection> collection;
            ComPtr<IDWriteFontFamily> family;
            ComPtr<IDWriteFont> font;
            ComPtr<IDWriteFontFace> face;
            UINT32 index = 0;
            BOOL exists = FALSE;
            VERIFY_SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf())));
            VERIFY_SUCCEEDED(factory->GetSystemFontCollection(&collection));
            VERIFY_SUCCEEDED(collection->FindFamilyName(L"Segoe UI", &index, &exists));
            VERIFY_IS_TRUE(!!exists);
            VERIFY_SUCCEEDED(collection->GetFontFamily(index, &family));
            VERIFY_SUCCEEDED(family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font));
            VERIFY_SUCCEEDED(font->CreateFontFace(&face));
            return face;
        }
    }

    void D2DSoftwarePainterTests::RejectsInvalidArguments()
    {
        auto device = CreateWarpDevice();
        std::unique_ptr<ID2DImmediatePainter> painter;
        VERIFY_ARE_EQUAL(E_INVALIDARG, CreateD2DPainter(Mode::ImmediateD2DSoftware, nullptr, PainterOptions{ 8, 8, 1.0f }, painter));
        VERIFY_ARE_EQUAL(E_INVALIDARG, CreateD2DPainter(Mode::ImmediateGdiPlus, device.Get(), PainterOptions{ 8, 8, 1.0f }, painter));
        VERIFY_ARE_EQUAL(E_INVALIDARG, CreateD2DPainter(Mode::Composition, device.Get(), PainterOptions{ 8, 8, 1.0f }, painter));
        VERIFY_ARE_EQUAL(E_INVALIDARG, CreateD2DPainter(Mode::ImmediateD2DSoftware, device.Get(), PainterOptions{ 0, 8, 1.0f }, painter));
        VERIFY_ARE_EQUAL(E_INVALIDARG, CreateD2DPainter(Mode::ImmediateD2DSoftware, device.Get(), PainterOptions{ 8, 8, 0.0f }, painter));
        VERIFY_IS_NULL(painter.get());
    }

    void D2DSoftwarePainterTests::WarpSupportsCompositionSwapChain()
    {
        auto device = CreateWarpDevice();
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        ComPtr<ID3D11Texture2D> backBuffer;
        VERIFY_SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
        DXGI_PRESENT_PARAMETERS parameters{};
        VERIFY_SUCCEEDED(swapChain->Present1(0, 0, &parameters));
        RECT dirty{ 0, 0, 16, 16 };
        parameters.DirtyRectsCount = 1;
        parameters.pDirtyRects = &dirty;
        VERIFY_SUCCEEDED(swapChain->Present1(0, 0, &parameters), L"dirty-rect present (how partial damage is presented)");
    }

    void D2DSoftwarePainterTests::DeviceRemovalCanBeSimulatedOnWarp()
    {
        RemovableWarpDevice device;
        if (!device.Create()) return;
        VERIFY_ARE_EQUAL(S_OK, device.d3d11->GetDeviceRemovedReason());
        device.Remove();
        const HRESULT reason = device.d3d11->GetDeviceRemovedReason();
        Log::Comment(String().Format(L"removed reason 0x%08X", reason));
        VERIFY_IS_TRUE(FAILED(reason), L"D3D11On12 device observes the D3D12 removal");
    }

    void D2DSoftwarePainterTests::TwoWarpDevicesProduceIdenticalPixels()
    {
        auto deviceA = CreateWarpDevice();
        auto deviceB = CreateWarpDevice();
        VERIFY_IS_TRUE(deviceA.Get() != deviceB.Get());
        auto painterA = CreateWarpPainter(deviceA.Get(), c_sceneWidth, c_sceneHeight);
        auto painterB = CreateWarpPainter(deviceB.Get(), c_sceneWidth, c_sceneHeight);
        PaintScene(*painterA);
        PaintScene(*painterB);
        const auto a = ReadAll(*painterA, c_sceneWidth, c_sceneHeight);
        const auto b = ReadAll(*painterB, c_sceneWidth, c_sceneHeight);

        size_t mismatches = 0;
        size_t first = a.size();
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (a[i] != b[i])
            {
                first = mismatches++ == 0 ? i : first;
            }
        }
        Log::Comment(String().Format(L"hash A %016llX, hash B %016llX, mismatched bytes %zu (first at byte %zu)", Hash(a), Hash(b), mismatches, first));

        // Guard against a vacuous pass: the frame has text and antialiased edges, so many distinct colors.
        std::vector<uint32_t> colors;
        for (size_t i = 0; i < a.size() && colors.size() < 64; i += 4)
        {
            uint32_t color;
            memcpy(&color, &a[i], sizeof(color));
            if (std::find(colors.begin(), colors.end(), color) == colors.end()) colors.push_back(color);
        }
        VERIFY_IS_TRUE(colors.size() >= 16, L"scene rendered with antialiasing and text");
        VERIFY_IS_TRUE(mismatches == 0, L"WARP output must be byte-identical across devices");
    }

    void D2DSoftwarePainterTests::RepeatedFrameReusesResources()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get(), c_sceneWidth, c_sceneHeight);
        PaintScene(*painter);
        PaintScene(*painter);
        const auto counters = painter->GetResourceCounters();
        const auto inventory = painter->GetResourceInventory();
        Log::Comment(String().Format(L"after warm-up: brushes %u textFormats %u geometries %u layers %u bitmaps %u; cache %llu bytes, %u objects",
            counters.solidBrushesCreated, counters.textFormatsCreated, counters.geometriesCreated, counters.layersCreated,
            counters.targetBitmapsCreated, inventory.cacheBytes, inventory.cachedObjectCount));
        VERIFY_IS_TRUE(counters.solidBrushesCreated >= 1);
        VERIFY_IS_TRUE(counters.textFormatsCreated >= 1, L"two text styles were drawn");
        VERIFY_IS_TRUE(inventory.cachedObjectCount > 0);
        VERIFY_IS_TRUE(inventory.cacheBytes > 0, L"text formats and glyph runs are cached");
        VERIFY_IS_TRUE(inventory.cacheBytes <= 8ull * 1024 * 1024, L"cache is bounded for a 6-tile scene");

        for (int i = 0; i < 3; ++i)
        {
            PaintScene(*painter);
        }
        const auto after = painter->GetResourceCounters();
        const auto afterInventory = painter->GetResourceInventory();
        VERIFY_ARE_EQUAL(counters.solidBrushesCreated, after.solidBrushesCreated, L"no new brushes");
        VERIFY_ARE_EQUAL(counters.textFormatsCreated, after.textFormatsCreated, L"no new text formats");
        VERIFY_ARE_EQUAL(counters.geometriesCreated, after.geometriesCreated, L"no new geometries");
        VERIFY_ARE_EQUAL(counters.layersCreated, after.layersCreated, L"no new layers");
        VERIFY_ARE_EQUAL(counters.targetBitmapsCreated, after.targetBitmapsCreated, L"no new target bitmaps");
        VERIFY_ARE_EQUAL(inventory.cachedObjectCount, afterInventory.cachedObjectCount);
        VERIFY_ARE_EQUAL(inventory.cacheBytes, afterInventory.cacheBytes);
    }

    void D2DSoftwarePainterTests::UsesCallerDeviceAndReportsItsIdentity()
    {
        DeviceIdentity expected{};
        auto device = CreateWarpDevice(&expected);
        const ULONG before = RefCount(device.Get());
        {
            auto painter = CreateWarpPainter(device.Get());
            const auto identity = painter->GetDeviceIdentity();
            VERIFY_IS_TRUE(identity.mode == Mode::ImmediateD2DSoftware);
            VERIFY_IS_TRUE(identity.isWarp);
            VERIFY_ARE_EQUAL(expected.adapterLuid, identity.adapterLuid);
            VERIFY_ARE_EQUAL(expected.featureLevel, identity.featureLevel);

            ComPtr<ID3D11Device> used;
            VERIFY_SUCCEEDED(painter->GetD3DDevice(&used));
            VERIFY_IS_TRUE(device.Get() == used.Get(), L"painter draws with the caller's device, not its own");
            used.Reset();
            VERIFY_IS_TRUE(RefCount(device.Get()) > before, L"painter keeps the device alive");
        }
        VERIFY_ARE_EQUAL(before, RefCount(device.Get()), L"painter releases the device");
    }

    void D2DSoftwarePainterTests::OffscreenFrameReportsNoUpload()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        PaintSolid(*painter, c_red);
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame, L"full frame, no swap chain");

        const RectF damage{ 0, 0, 32, 32 };
        PaintSolid(*painter, c_blue, &damage, 1);
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame, L"partial frame, no swap chain");
        ReadAll(*painter, c_size, c_size);
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame, L"ReadPixels is not counted");
    }

    void D2DSoftwarePainterTests::PresentedFrameUploadsDamagedPixels()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));

        auto stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_red); });
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(c_size) * c_size * 4, painter->GetResourceInventory().uploadBytesThisFrame, L"first frame copies the whole target");
        VERIFY_ARE_EQUAL(stats.damagedPixels * 4, painter->GetResourceInventory().uploadBytesThisFrame);

        const RectF damage[] = { { 0, 0, 16, 64 }, { 40, 8, 64, 24 } };
        stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_blue); }, damage, 2);
        const uint64_t expected = (16ull * 64 + 24ull * 16) * 4;
        Log::Comment(String().Format(L"damaged %llu px, upload %llu bytes", stats.damagedPixels, painter->GetResourceInventory().uploadBytesThisFrame));
        VERIFY_ARE_EQUAL(2u, stats.damageRectCount);
        VERIFY_ARE_EQUAL(expected, painter->GetResourceInventory().uploadBytesThisFrame, L"only damaged pixels are copied");
        VERIFY_ARE_EQUAL(stats.damagedPixels * 4, painter->GetResourceInventory().uploadBytesThisFrame);

        ReadAll(*painter, c_size, c_size);
        VERIFY_ARE_EQUAL(expected, painter->GetResourceInventory().uploadBytesThisFrame, L"ReadPixels is not counted");

        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(nullptr));
        PaintSolid(*painter, c_white, damage, 1);
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame, L"detached: offscreen-only again");
    }

    void D2DSoftwarePainterTests::PresentedUploadIsClippedToTarget()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));
        PaintSolid(*painter, c_red);

        const RectF damage{ 48, 48, 80, 80 };
        PaintSolid(*painter, c_blue, &damage, 1);
        VERIFY_ARE_EQUAL(16ull * 16 * 4, painter->GetResourceInventory().uploadBytesThisFrame, L"damage outside the target is not copied");
    }

    void D2DSoftwarePainterTests::ReportsFailureAfterDeviceRemoval()
    {
        RemovableWarpDevice device;
        if (!device.Create()) return;
        auto painter = CreateWarpPainter(device.d3d11.Get());
        const auto identity = painter->GetDeviceIdentity();
        PaintSolid(*painter, c_red);

        device.Remove();

        HRESULT hr = painter->BeginFrame(nullptr, 0);
        if (SUCCEEDED(hr))
        {
            PaintDashboardFrame(*painter);
            FrameStats stats{};
            hr = painter->EndFrame(&stats);
        }
        Log::Comment(String().Format(L"frame after removal: 0x%08X", hr));
        VERIFY_IS_TRUE(FAILED(hr), L"a frame on a removed device reports failure");

        std::vector<uint8_t> bytes(static_cast<size_t>(c_size) * c_size * 4);
        VERIFY_IS_TRUE(FAILED(painter->ReadPixels(bytes.data(), c_size * 4, static_cast<uint32_t>(bytes.size()))));
        VERIFY_IS_TRUE(FAILED(painter->Resize(c_size * 2, c_size, 1.0f)));
        VERIFY_IS_TRUE(FAILED(painter->BeginFrame(nullptr, 0)), L"stays failed; recovery needs a new device");

        const auto after = painter->GetDeviceIdentity();
        VERIFY_ARE_EQUAL(identity.adapterLuid, after.adapterLuid);
        painter->GetResourceInventory();
        painter.reset();
    }

    void D2DSoftwarePainterTests::MultiRectDamageReplacesOnlyDamagedPixels()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        PaintSolid(*painter, c_red);

        const RectF damage[] = { { 0, 0, 16, 16 }, { 32, 32, 48, 48 } };
        const auto paintDamage = [&]()
        {
            return Paint(*painter, [](IImmediatePainter& p)
            {
                p.Clear({ 0, 0, 0, 0 });
                p.FillRectangle({ 8, 8, 40, 40 }, c_blue);
            }, damage, 2);
        };
        const auto stats = paintDamage();
        VERIFY_ARE_EQUAL(2u, stats.damageRectCount);
        VERIFY_ARE_EQUAL(2ull * 16 * 16, stats.damagedPixels);

        const auto bytes = ReadAll(*painter, c_size, c_size);
        const auto pixel = [&](uint32_t x, uint32_t y)
        {
            uint32_t value;
            memcpy(&value, &bytes[(static_cast<size_t>(y) * c_size + x) * 4], sizeof(value));
            return value; // 0xAARRGGBB
        };
        VERIFY_ARE_EQUAL(0x00000000u, pixel(2, 2), L"transparent Clear replaces inside the first rect");
        VERIFY_ARE_EQUAL(0xFF0000FFu, pixel(12, 12), L"fill drawn inside the first rect");
        VERIFY_ARE_EQUAL(0xFF0000FFu, pixel(36, 36), L"fill drawn inside the second rect");
        VERIFY_ARE_EQUAL(0x00000000u, pixel(44, 44), L"transparent Clear replaces inside the second rect");
        VERIFY_ARE_EQUAL(0xFFFF0000u, pixel(24, 24), L"between the rects keeps the previous frame");
        VERIFY_ARE_EQUAL(0xFFFF0000u, pixel(60, 4), L"outside the damage keeps the previous frame");

        const auto counters = painter->GetResourceCounters();
        paintDamage();
        const auto after = painter->GetResourceCounters();
        VERIFY_ARE_EQUAL(counters.geometriesCreated, after.geometriesCreated, L"same damage reuses the mask geometry");
    }

    void D2DSoftwarePainterTests::RejectsSwapChainOnAnotherDevice()
    {
        auto device = CreateWarpDevice();
        auto otherDevice = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        auto foreign = CreateCompositionSwapChain(otherDevice.Get(), c_size, c_size);
        VERIFY_ARE_EQUAL(E_INVALIDARG, painter->SetPresentationSwapChain(foreign.Get()));

        PaintSolid(*painter, c_red);
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame, L"rejected swap chain is not attached");
    }

    void D2DSoftwarePainterTests::ResizeResizesAttachedSwapChain()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));
        PaintSolid(*painter, c_red);

        VERIFY_SUCCEEDED(painter->Resize(32, 48, 1.0f));
        DXGI_SWAP_CHAIN_DESC1 desc{};
        VERIFY_SUCCEEDED(swapChain->GetDesc1(&desc));
        VERIFY_ARE_EQUAL(32u, desc.Width);
        VERIFY_ARE_EQUAL(48u, desc.Height);

        const RectF damage{ 0, 0, 8, 8 };
        const auto stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_blue); }, &damage, 1);
        VERIFY_ARE_EQUAL(32ull * 48, stats.damagedPixels, L"the frame after Resize is fully damaged");
        VERIFY_ARE_EQUAL(32ull * 48 * 4, painter->GetResourceInventory().uploadBytesThisFrame);
        PaintSolid(*painter, c_red, &damage, 1);
        VERIFY_ARE_EQUAL(8ull * 8 * 4, painter->GetResourceInventory().uploadBytesThisFrame);
    }

    void D2DSoftwarePainterTests::RoundedClipUsesCachedLayerMask()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        const auto paint = [&]()
        {
            return Paint(*painter, [](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.PushAxisAlignedClip({ 0, 0, 48, 64 });
                p.PushRoundedRectangleClip({ 8, 8, 56, 56 }, 16, 16);
                p.PushOpacityLayer(1.0f);
                p.FillRectangle({ 0, 0, 64, 64 }, c_red);
                p.PopAxisAlignedClip(); // mismatched: ignored, the opacity layer is on top
                p.PopOpacityLayer();
                p.PopRoundedRectangleClip();
                p.PopAxisAlignedClip();
                p.FillRectangle({ 60, 60, 64, 64 }, c_blue); // after all pops: unclipped
            });
        };
        const auto stats = paint();
        VERIFY_ARE_EQUAL(2u, stats.primitiveCount, L"clips and layers are not primitives");

        const auto bytes = ReadAll(*painter, c_size, c_size);
        VERIFY_ARE_EQUAL(0xFFFF0000u, PixelAt(bytes, c_size, 32, 32), L"inside the rounded clip");
        VERIFY_ARE_EQUAL(0xFFFF0000u, PixelAt(bytes, c_size, 32, 8), L"straight top edge is pixel-aligned");
        VERIFY_ARE_EQUAL(0xFFFFFFFFu, PixelAt(bytes, c_size, 9, 9), L"rounded corner is cut");
        VERIFY_ARE_EQUAL(0xFFFFFFFFu, PixelAt(bytes, c_size, 4, 32), L"outside the rounded clip");
        VERIFY_ARE_EQUAL(0xFFFFFFFFu, PixelAt(bytes, c_size, 52, 32), L"outside the enclosing axis-aligned clip");
        VERIFY_ARE_EQUAL(0xFF0000FFu, PixelAt(bytes, c_size, 62, 62), L"stack fully unwound");
        bool antialiased = false;
        for (uint32_t i = 8; i < 24 && !antialiased; ++i)
        {
            const uint32_t value = PixelAt(bytes, c_size, i, i);
            antialiased = value != 0xFFFFFFFFu && value != 0xFFFF0000u;
        }
        VERIFY_IS_TRUE(antialiased, L"the corner arc is antialiased");

        const auto counters = painter->GetResourceCounters();
        VERIFY_ARE_EQUAL(0u, counters.layersCreated, L"layerless");
        paint();
        const auto after = painter->GetResourceCounters();
        VERIFY_ARE_EQUAL(counters.geometriesCreated, after.geometriesCreated, L"the mask geometry is cached");
        VERIFY_ARE_EQUAL(0u, after.layersCreated);
    }

    void D2DSoftwarePainterTests::ExtendedFrameReusesResources()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        auto fontFace = CreateSegoeFontFace();
        const UINT32 codePoints[] = { 'H', 'i' };
        uint16_t glyphs[2]{};
        VERIFY_SUCCEEDED(fontFace->GetGlyphIndices(codePoints, 2, glyphs));
        const float advances[] = { 9, 4 };

        std::vector<uint8_t> pixels(8 * 8 * 4, 0xFF);
        BitmapSource image{};
        image.key = 42;
        image.generation = 1;
        image.width = 8;
        image.height = 8;
        image.stride = 8 * 4;
        image.pixels = pixels.data();

        const PathVerb verbs[] = { PathVerb::MoveTo, PathVerb::LineTo, PathVerb::CubicTo, PathVerb::Close };
        const PointF points[] = { { 4, 4 }, { 30, 4 }, { 30, 20 }, { 10, 30 }, { 4, 20 } };
        PathData path{};
        path.verbs = verbs;
        path.verbCount = 4;
        path.points = points;
        path.pointCount = 5;
        path.cacheKey = 7;

        const auto paint = [&]()
        {
            VERIFY_SUCCEEDED(painter->EnsureBitmap(image));
            return Paint(*painter, [&](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.PushRoundedRectangleClip({ 2, 2, 62, 62 }, 6, 6);
                p.FillPath(path, c_red);
                p.StrokePath(path, 2, c_blue);
                GlyphRunDesc run{};
                run.fontFace = fontFace.Get();
                run.fontEmSize = 14;
                run.glyphIndices = glyphs;
                run.glyphAdvances = advances;
                run.glyphCount = 2;
                run.baselineOrigin = { 34, 50 };
                p.DrawGlyphRun(run, { 0, 0, 0, 1 });
                p.DrawBitmap(42, { 40, 4, 56, 20 }, nullptr, 0.5f);
                p.PopRoundedRectangleClip();
            });
        };

        auto stats = paint();
        VERIFY_ARE_EQUAL(4u, stats.primitiveCount, L"fill path, stroke path, glyph run, bitmap");
        paint();
        const auto counters = painter->GetResourceCounters();
        const auto inventory = painter->GetResourceInventory();
        Log::Comment(String().Format(L"after warm-up: geometries %u paths %u bitmaps %u uploads %u; cache %llu bytes, %u objects",
            counters.geometriesCreated, counters.pathGeometriesCreated, counters.bitmapsCreated, counters.bitmapUploads,
            inventory.cacheBytes, inventory.cachedObjectCount));
        VERIFY_ARE_EQUAL(1u, counters.pathGeometriesCreated, L"fill and stroke share the cached path");
        VERIFY_ARE_EQUAL(1u, counters.bitmapsCreated);
        VERIFY_ARE_EQUAL(1u, counters.bitmapUploads);
        VERIFY_IS_TRUE(inventory.cacheBytes >= 8ull * 8 * 4, L"bitmap storage counts in cacheBytes");

        for (int i = 0; i < 3; ++i)
        {
            paint();
        }
        const auto after = painter->GetResourceCounters();
        const auto afterInventory = painter->GetResourceInventory();
        VERIFY_ARE_EQUAL(counters.solidBrushesCreated, after.solidBrushesCreated);
        VERIFY_ARE_EQUAL(counters.geometriesCreated, after.geometriesCreated);
        VERIFY_ARE_EQUAL(counters.pathGeometriesCreated, after.pathGeometriesCreated);
        VERIFY_ARE_EQUAL(counters.layersCreated, after.layersCreated);
        VERIFY_ARE_EQUAL(counters.bitmapsCreated, after.bitmapsCreated);
        VERIFY_ARE_EQUAL(counters.bitmapUploads, after.bitmapUploads);
        VERIFY_ARE_EQUAL(inventory.cachedObjectCount, afterInventory.cachedObjectCount);
        VERIFY_ARE_EQUAL(inventory.cacheBytes, afterInventory.cacheBytes);
        VERIFY_ARE_EQUAL(0ull, afterInventory.uploadBytesThisFrame, L"unchanged bitmap is not copied again");

        const auto bytes = ReadAll(*painter, c_size, c_size);
        VERIFY_ARE_EQUAL(0xFFFF0000u, PixelAt(bytes, c_size, 16, 14), L"path interior is filled");

        // cacheKey 0 builds a geometry per call and keeps none.
        path.cacheKey = 0;
        stats = Paint(*painter, [&](IImmediatePainter& p) { p.FillPath(path, c_red); p.FillPath(path, c_red); });
        VERIFY_ARE_EQUAL(2u, stats.primitiveCount);
        VERIFY_ARE_EQUAL(after.pathGeometriesCreated + 2, painter->GetResourceCounters().pathGeometriesCreated);
        VERIFY_ARE_EQUAL(afterInventory.cachedObjectCount, painter->GetResourceInventory().cachedObjectCount);
    }

    void D2DSoftwarePainterTests::BitmapUploadAccounting()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        std::vector<uint32_t> blue(16 * 8, 0xFF0000FFu);
        BitmapSource image{};
        image.key = 1;
        image.generation = 1;
        image.width = 16;
        image.height = 8;
        image.stride = 16 * 4;
        image.pixels = reinterpret_cast<const uint8_t*>(blue.data());
        const uint64_t imageBytes = 16ull * 8 * 4;

        VERIFY_ARE_EQUAL(E_INVALIDARG, painter->EnsureBitmap(BitmapSource{}));
        const uint64_t emptyCache = painter->GetResourceInventory().cacheBytes;

        // Between frames: counted in the next frame (offscreen, so nothing else is counted).
        VERIFY_SUCCEEDED(painter->EnsureBitmap(image));
        VERIFY_ARE_EQUAL(emptyCache + imageBytes, painter->GetResourceInventory().cacheBytes);
        auto stats = Paint(*painter, [](IImmediatePainter& p)
        {
            const RectF rightHalf{ 8, 0, 16, 8 };
            p.Clear(c_white);
            p.DrawBitmap(1, { 8, 8, 40, 24 }, nullptr, 1.0f);      // scaled 2x
            p.DrawBitmap(1, { 8, 32, 16, 40 }, &rightHalf, 1.0f);  // right half, 1:1
        });
        VERIFY_ARE_EQUAL(2u, stats.primitiveCount);
        VERIFY_ARE_EQUAL(imageBytes, painter->GetResourceInventory().uploadBytesThisFrame);
        auto bytes = ReadAll(*painter, c_size, c_size);
        VERIFY_ARE_EQUAL(0xFF0000FFu, PixelAt(bytes, c_size, 24, 16));
        VERIFY_ARE_EQUAL(0xFF0000FFu, PixelAt(bytes, c_size, 12, 36));
        VERIFY_ARE_EQUAL(0xFFFFFFFFu, PixelAt(bytes, c_size, 44, 16));

        // Same (key, generation): no copy.
        VERIFY_SUCCEEDED(painter->EnsureBitmap(image));
        PaintSolid(*painter, c_white);
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame);

        // New generation, same size: refreshed in place, counted in the frame it happens in.
        std::vector<uint32_t> red(16 * 8, 0xFFFF0000u);
        image.generation = 2;
        image.pixels = reinterpret_cast<const uint8_t*>(red.data());
        const auto before = painter->GetResourceCounters();
        Paint(*painter, [&](IImmediatePainter& p)
        {
            VERIFY_SUCCEEDED(p.EnsureBitmap(image));
            p.DrawBitmap(1, { 8, 8, 24, 16 }, nullptr, 1.0f);
        });
        VERIFY_ARE_EQUAL(imageBytes, painter->GetResourceInventory().uploadBytesThisFrame);
        VERIFY_ARE_EQUAL(before.bitmapsCreated, painter->GetResourceCounters().bitmapsCreated, L"same size: CopyFromMemory");
        VERIFY_ARE_EQUAL(before.bitmapUploads + 1, painter->GetResourceCounters().bitmapUploads);
        bytes = ReadAll(*painter, c_size, c_size);
        VERIFY_ARE_EQUAL(0xFFFF0000u, PixelAt(bytes, c_size, 12, 12), L"refreshed pixels are drawn");

        // New size: a new bitmap replaces the old one.
        image.generation = 3;
        image.width = 8;
        VERIFY_SUCCEEDED(painter->EnsureBitmap(image));
        VERIFY_ARE_EQUAL(before.bitmapsCreated + 1, painter->GetResourceCounters().bitmapsCreated);
        VERIFY_ARE_EQUAL(emptyCache + imageBytes / 2, painter->GetResourceInventory().cacheBytes);

        // Presented on WARP: the bitmap copy adds to the damage copy.
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));
        PaintSolid(*painter, c_white);
        image.generation = 4;
        VERIFY_SUCCEEDED(painter->EnsureBitmap(image));
        const RectF damage{ 0, 0, 16, 16 };
        PaintSolid(*painter, c_white, &damage, 1);
        VERIFY_ARE_EQUAL(16ull * 16 * 4, painter->GetPresentCopyBytesThisFrame());
        VERIFY_ARE_EQUAL(16ull * 16 * 4 + imageBytes / 2, painter->GetResourceInventory().uploadBytesThisFrame);

        // Release: storage is freed and drawing the key is a no-op.
        painter->ReleaseBitmap(1);
        painter->ReleaseBitmap(1);
        VERIFY_ARE_EQUAL(emptyCache, painter->GetResourceInventory().cacheBytes);
        stats = Paint(*painter, [](IImmediatePainter& p) { p.DrawBitmap(1, { 0, 0, 64, 64 }, nullptr, 1.0f); });
        VERIFY_ARE_EQUAL(0u, stats.primitiveCount);
    }

    void D2DSoftwarePainterTests::ScrollPixelsPresentsMovedArea()
    {
        auto device = CreateWarpDevice();
        auto painter = CreateWarpPainter(device.Get());
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            for (uint32_t row = 0; row < 8; ++row)
            {
                p.FillRectangle({ 0, row * 8.0f, 64, row * 8.0f + 8 }, row % 2 ? c_blue : c_red);
            }
        });

        VERIFY_SUCCEEDED(painter->BeginFrame(nullptr, 0));
        VERIFY_ARE_EQUAL(E_ILLEGAL_METHOD_CALL, painter->ScrollPixels({ 0, 0, 32, 64 }, 0, -16));
        FrameStats stats{};
        VERIFY_SUCCEEDED(painter->EndFrame(&stats));

        // Scroll the left half up by 12 (overlapping source and destination); repaint only the exposed strip.
        VERIFY_SUCCEEDED(painter->ScrollPixels({ 0, 0, 32, 64 }, 0, -12));
        const RectF exposed{ 0, 52, 32, 64 };
        stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); }, &exposed, 1);
        VERIFY_ARE_EQUAL(32ull * 12, stats.damagedPixels, L"the moved area is not damage");
        const uint64_t presented = (32ull * 52 + 32ull * 12) * 4;
        VERIFY_ARE_EQUAL(presented, painter->GetPresentCopyBytesThisFrame(), L"moved area is presented with the frame");
        VERIFY_ARE_EQUAL(presented, painter->GetResourceInventory().uploadBytesThisFrame);

        const auto bytes = ReadAll(*painter, c_size, c_size);
        for (uint32_t y = 0; y < 52; ++y)
        {
            const uint32_t expected = ((y + 12) / 8) % 2 ? 0xFF0000FFu : 0xFFFF0000u;
            if (PixelAt(bytes, c_size, 4, y) != expected)
            {
                VERIFY_ARE_EQUAL(expected, PixelAt(bytes, c_size, 4, y), String().Format(L"moved row %u", y));
            }
        }
        VERIFY_ARE_EQUAL(0xFFFFFFFFu, PixelAt(bytes, c_size, 4, 58), L"exposed strip repainted");
        VERIFY_ARE_EQUAL(0xFFFF0000u, PixelAt(bytes, c_size, 40, 4), L"outside the scroll rect is unchanged");
        VERIFY_ARE_EQUAL(0xFF0000FFu, PixelAt(bytes, c_size, 40, 60));

        stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); }, &exposed, 1);
        VERIFY_ARE_EQUAL(32ull * 12 * 4, painter->GetResourceInventory().uploadBytesThisFrame, L"moved area presented once");

        // Scrolling everything out of the rect, and offscreen scrolling, copy nothing to present.
        VERIFY_SUCCEEDED(painter->ScrollPixels({ 0, 0, 32, 64 }, 40, 0));
        PaintSolid(*painter, c_white, &exposed, 1);
        VERIFY_ARE_EQUAL(32ull * 12 * 4, painter->GetResourceInventory().uploadBytesThisFrame);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(nullptr));
        VERIFY_SUCCEEDED(painter->ScrollPixels({ 0, 0, 64, 64 }, 4, 4));
        PaintSolid(*painter, c_white, &exposed, 1);
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame);
        VERIFY_ARE_EQUAL(1u, painter->GetResourceCounters().scratchSurfacesCreated, L"scratch texture is reused");
    }
}
