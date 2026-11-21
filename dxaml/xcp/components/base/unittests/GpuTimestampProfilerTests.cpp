// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GpuTimestampProfilerTests.h"
#include <GpuTimestampProfiler.h>
#include <ImmediatePainter.h>
#include <d3d11.h>
#include <wrl/client.h>

namespace RenderingBackendTests
{
    using namespace RenderingBackend;
    using namespace WEX::Common;
    using namespace WEX::Logging;
    using Microsoft::WRL::ComPtr;

    namespace
    {
        struct WarpTarget
        {
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;
            ComPtr<ID3D11RenderTargetView> view;
        };

        WarpTarget CreateWarpTarget()
        {
            WarpTarget target;
            DeviceIdentity identity{};
            VERIFY_SUCCEEDED(CreateD3DDeviceForMode(Mode::ImmediateD2DSoftware, &target.device, &identity));
            target.device->GetImmediateContext(&target.context);

            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = 256;
            desc.Height = 256;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET;
            ComPtr<ID3D11Texture2D> texture;
            VERIFY_SUCCEEDED(target.device->CreateTexture2D(&desc, nullptr, &texture));
            VERIFY_SUCCEEDED(target.device->CreateRenderTargetView(texture.Get(), nullptr, &target.view));
            return target;
        }

        void Paint(GpuTimestampProfiler& profiler, WarpTarget& target, uint32_t frameNumber)
        {
            const float color[4] = { 0.2f, 0.4f, 0.6f, 1.0f };
            profiler.BeginPaint(target.context.Get(), frameNumber);
            target.context->ClearRenderTargetView(target.view.Get(), color);
            profiler.EndPaint(target.context.Get());
        }

        // Test-only wait: submits the work, then polls the non-blocking Collect until nothing is pending.
        uint32_t CollectAll(GpuTimestampProfiler& profiler, ID3D11DeviceContext* context, GpuTimestampProfiler::Result* results,
            uint32_t capacity)
        {
            context->Flush();
            uint32_t valid = 0;
            const ULONGLONG deadline = GetTickCount64() + 10000;
            while (profiler.PendingCount() > 0 && GetTickCount64() < deadline)
            {
                const uint32_t got = profiler.Collect(context, results + valid, capacity - valid);
                valid += got;
                if (profiler.PendingCount() > 0)
                {
                    Sleep(1);
                }
            }
            VERIFY_ARE_EQUAL(0u, profiler.PendingCount(), L"timestamp results became available within 10 s");
            return valid;
        }
    }

    void GpuTimestampProfilerTests::TicksConvertToMicroseconds()
    {
        uint64_t us = 7;
        VERIFY_IS_TRUE(GpuTicksToMicroseconds(1000, 2000, 1000000, false, us));
        VERIFY_ARE_EQUAL(1000ull, us);
        VERIFY_IS_TRUE(GpuTicksToMicroseconds(5, 5, 1000000000, false, us));
        VERIFY_ARE_EQUAL(0ull, us);
        // 2.5 ms at 10 MHz (a common QPC-like GPU frequency).
        VERIFY_IS_TRUE(GpuTicksToMicroseconds(0, 25000, 10000000, false, us));
        VERIFY_ARE_EQUAL(2500ull, us);
        // Sub-microsecond intervals truncate to zero, which is still a valid result.
        VERIFY_IS_TRUE(GpuTicksToMicroseconds(0, 999, 1000000000, false, us));
        VERIFY_ARE_EQUAL(0ull, us);
        // Ten hours at 10 GHz: ticks * 1e6 would overflow 64 bits.
        const uint64_t frequency = 10000000000ull;
        VERIFY_IS_TRUE(GpuTicksToMicroseconds(0, 36000ull * frequency + frequency / 2, frequency, false, us));
        VERIFY_ARE_EQUAL(36000ull * 1000000ull + 500000ull, us);
    }

    void GpuTimestampProfilerTests::UnusableIntervalsAreRejected()
    {
        uint64_t us = 7;
        VERIFY_IS_FALSE(GpuTicksToMicroseconds(1000, 2000, 1000000, true, us));
        VERIFY_ARE_EQUAL(0ull, us);
        VERIFY_IS_FALSE(GpuTicksToMicroseconds(1000, 2000, 0, false, us));
        VERIFY_IS_FALSE(GpuTicksToMicroseconds(2000, 1000, 1000000, false, us));
    }

    void GpuTimestampProfilerTests::MeasuresClearOnWarp()
    {
        auto target = CreateWarpTarget();
        GpuTimestampProfiler profiler;
        VERIFY_SUCCEEDED(profiler.Initialize(target.device.Get()));
        VERIFY_IS_TRUE(profiler.IsInitialized());

        Paint(profiler, target, 7);
        VERIFY_ARE_EQUAL(1u, profiler.PendingCount());

        GpuTimestampProfiler::Result results[GpuTimestampProfiler::SlotCount]{};
        const uint32_t valid = CollectAll(profiler, target.context.Get(), results, ARRAYSIZE(results));
        const auto& counters = profiler.GetCounters();
        VERIFY_ARE_EQUAL(1ull, counters.issued);
        VERIFY_ARE_EQUAL(1ull, counters.completed + counters.disjoint);
        VERIFY_ARE_EQUAL(0ull, counters.dropped);
        if (valid == 0)
        {
            // A disjoint interval (frequency change) is legitimately discarded rather than reported.
            Log::Comment(L"interval was disjoint; discarded");
            VERIFY_ARE_EQUAL(1ull, counters.disjoint);
            return;
        }
        VERIFY_ARE_EQUAL(1u, valid);
        VERIFY_ARE_EQUAL(7u, results[0].frameNumber);
        VERIFY_IS_TRUE(results[0].gpuMicroseconds < 5000000ull, L"a 256x256 clear takes well under 5 s");
        Log::Comment(String().Format(L"WARP clear: %llu us", results[0].gpuMicroseconds));
    }

    void GpuTimestampProfilerTests::FullRingDropsOldestInsteadOfBlocking()
    {
        auto target = CreateWarpTarget();
        GpuTimestampProfiler profiler;
        VERIFY_SUCCEEDED(profiler.Initialize(target.device.Get()));

        constexpr uint32_t paints = GpuTimestampProfiler::SlotCount + 2;
        for (uint32_t frame = 1; frame <= paints; ++frame)
        {
            Paint(profiler, target, frame);
        }
        VERIFY_ARE_EQUAL(GpuTimestampProfiler::SlotCount, profiler.PendingCount());
        VERIFY_ARE_EQUAL(2ull, profiler.GetCounters().dropped);

        GpuTimestampProfiler::Result results[GpuTimestampProfiler::SlotCount]{};
        const uint32_t valid = CollectAll(profiler, target.context.Get(), results, ARRAYSIZE(results));
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(GpuTimestampProfiler::SlotCount),
            profiler.GetCounters().completed + profiler.GetCounters().disjoint);
        // Survivors are the newest intervals, read back in issue order.
        uint32_t previous = 2;
        for (uint32_t index = 0; index < valid; ++index)
        {
            VERIFY_IS_TRUE(results[index].frameNumber > previous);
            VERIFY_IS_TRUE(results[index].frameNumber <= paints);
            previous = results[index].frameNumber;
        }
    }

    void GpuTimestampProfilerTests::UnbalancedCallsAreTolerated()
    {
        auto target = CreateWarpTarget();
        GpuTimestampProfiler profiler;
        VERIFY_SUCCEEDED(profiler.Initialize(target.device.Get()));

        profiler.EndPaint(target.context.Get());
        VERIFY_ARE_EQUAL(0ull, profiler.GetCounters().issued);
        VERIFY_ARE_EQUAL(0u, profiler.PendingCount());

        profiler.BeginPaint(target.context.Get(), 1); // abandoned
        Paint(profiler, target, 2);
        VERIFY_ARE_EQUAL(1ull, profiler.GetCounters().dropped);
        VERIFY_ARE_EQUAL(1ull, profiler.GetCounters().issued);

        GpuTimestampProfiler::Result results[GpuTimestampProfiler::SlotCount]{};
        const uint32_t valid = CollectAll(profiler, target.context.Get(), results, ARRAYSIZE(results));
        if (valid == 1)
        {
            VERIFY_ARE_EQUAL(2u, results[0].frameNumber);
        }
        VERIFY_ARE_EQUAL(1ull, profiler.GetCounters().completed + profiler.GetCounters().disjoint);
    }

    void GpuTimestampProfilerTests::UninitializedProfilerIsInert()
    {
        auto target = CreateWarpTarget();
        GpuTimestampProfiler profiler;
        VERIFY_IS_FALSE(profiler.IsInitialized());
        Paint(profiler, target, 1);
        VERIFY_ARE_EQUAL(0u, profiler.PendingCount());
        VERIFY_ARE_EQUAL(0u, profiler.Collect(target.context.Get(), nullptr, 0));
        VERIFY_ARE_EQUAL(0ull, profiler.GetCounters().issued);
    }
}
