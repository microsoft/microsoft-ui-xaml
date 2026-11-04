// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RenderingResourceDiagnosticsTests.h"
#include <RenderingResourceDiagnostics.h>
#include <XamlProfilerTracing.h>
#include <array>
#include <thread>

namespace RenderingResourceDiagnosticsTests
{
    using namespace RenderingResourceDiagnostics;

    bool CounterTests::Setup()
    {
        (void)XamlProfilerLogging::Provider();
        m_wasProfilingEnabled = XamlProfilerTracing::IsProfilerEnabled();
        XamlProfilerTracing::SetProfilerEnabled(true);
        return true;
    }

    bool CounterTests::Cleanup()
    {
        XamlProfilerTracing::SetProfilerEnabled(m_wasProfilingEnabled);
        return true;
    }

    Snapshot Read(ResourceKind kind)
    {
        Snapshot snapshot{};
        VERIFY_IS_TRUE(TryGetSnapshot(static_cast<uint32_t>(kind), snapshot));
        return snapshot;
    }

    void CounterTests::RuntimeEnablement()
    {
        const auto before = Read(ResourceKind::SpriteVisual);
        XamlProfilerTracing::SetProfilerEnabled(false);
        VERIFY_IS_FALSE(IsEnabled());
        VERIFY_IS_FALSE(LogResourceCreated(ResourceKind::SpriteVisual));
        const auto disabled = Read(ResourceKind::SpriteVisual);
        VERIFY_IS_FALSE(disabled.enabled);
        VERIFY_ARE_EQUAL(uint64_t{0}, disabled.created);
        VERIFY_ARE_EQUAL(uint64_t{0}, disabled.destroyed);
        VERIFY_ARE_EQUAL(uint64_t{0}, disabled.live);

        XamlProfilerTracing::SetProfilerEnabled(true);
        VERIFY_IS_TRUE(IsEnabled());
        VERIFY_ARE_EQUAL(before.created, Read(ResourceKind::SpriteVisual).created);
        VERIFY_IS_TRUE(LogResourceCreated(ResourceKind::SpriteVisual));
        VERIFY_ARE_EQUAL(before.created + 1, Read(ResourceKind::SpriteVisual).created);
    }

    void CounterTests::LifetimeAcrossRuntimeChanges()
    {
        const auto before = Read(ResourceKind::D2DStrokeStyleWrapper);
        XamlProfilerTracing::SetProfilerEnabled(false);
        VERIFY_IS_FALSE(LogResourceCreated(ResourceKind::D2DStrokeStyleWrapper));
        XamlProfilerTracing::SetProfilerEnabled(true);
        VERIFY_ARE_EQUAL(before.live, Read(ResourceKind::D2DStrokeStyleWrapper).live);
        VERIFY_IS_TRUE(LogResourceCreated(ResourceKind::D2DStrokeStyleWrapper));

        XamlProfilerTracing::SetProfilerEnabled(false);
        LogResourceDestroyed(ResourceKind::D2DStrokeStyleWrapper);
        VERIFY_IS_FALSE(LogResourceCreated(ResourceKind::D2DStrokeStyleWrapper));
        XamlProfilerTracing::SetProfilerEnabled(true);
        const auto after = Read(ResourceKind::D2DStrokeStyleWrapper);
        VERIFY_ARE_EQUAL(before.created + 1, after.created);
        VERIFY_ARE_EQUAL(before.destroyed + 1, after.destroyed);
        VERIFY_ARE_EQUAL(before.live, after.live);

        VERIFY_IS_TRUE(LogResourceCreated(ResourceKind::D2DStrokeStyleWrapper));
        XamlProfilerTracing::SetProfilerEnabled(false);
        XamlProfilerTracing::SetProfilerEnabled(true);
        LogResourceDestroyed(ResourceKind::D2DStrokeStyleWrapper);
        const auto reenabled = Read(ResourceKind::D2DStrokeStyleWrapper);
        VERIFY_ARE_EQUAL(before.created + 2, reenabled.created);
        VERIFY_ARE_EQUAL(before.destroyed + 2, reenabled.destroyed);
        VERIFY_ARE_EQUAL(before.live, reenabled.live);
    }

    void CounterTests::WrapperLifetime()
    {
        const auto before = Read(ResourceKind::DCompSurfaceWrapper);
        LogResourceCreated(ResourceKind::DCompSurfaceWrapper);
        LogResourceCreated(ResourceKind::DCompSurfaceWrapper);
        const auto during = Read(ResourceKind::DCompSurfaceWrapper);
        VERIFY_ARE_EQUAL(before.live + 2, during.live);
        LogResourceDestroyed(ResourceKind::DCompSurfaceWrapper);
        LogResourceDestroyed(ResourceKind::DCompSurfaceWrapper);
        const auto after = Read(ResourceKind::DCompSurfaceWrapper);
        VERIFY_ARE_EQUAL(before.live, after.live);
        VERIFY_ARE_EQUAL(before.created + 2, after.created);
        VERIFY_ARE_EQUAL(before.destroyed + 2, after.destroyed);
    }

    void CounterTests::IndependentCategories()
    {
        const auto before = Read(ResourceKind::ContainerVisual);
        LogResourceCreated(ResourceKind::ColorBrush);
        VERIFY_ARE_EQUAL(before.created, Read(ResourceKind::ContainerVisual).created);
        const auto creationOnly = Read(ResourceKind::ColorBrush);
        VERIFY_IS_FALSE(creationOnly.hasLifetime);
        VERIFY_ARE_EQUAL(uint64_t{0}, creationOnly.live);
        VERIFY_ARE_EQUAL(uint64_t{0}, creationOnly.destroyed);
    }

    void CountOnWorker()
    {
        for (uint32_t index = 0; index < 1000; ++index)
        {
            LogResourceCreated(ResourceKind::D2DGeometryWrapper);
            LogResourceDestroyed(ResourceKind::D2DGeometryWrapper);
        }
    }

    void CounterTests::ConcurrentUpdates()
    {
        const auto before = Read(ResourceKind::D2DGeometryWrapper);
        std::array<std::thread, 4> workers{
            std::thread(CountOnWorker),
            std::thread(CountOnWorker),
            std::thread(CountOnWorker),
            std::thread(CountOnWorker)};
        for (auto& worker : workers)
        {
            worker.join();
        }
        const auto after = Read(ResourceKind::D2DGeometryWrapper);
        VERIFY_ARE_EQUAL(before.created + 4000, after.created);
        VERIFY_ARE_EQUAL(before.destroyed + 4000, after.destroyed);
        VERIFY_ARE_EQUAL(before.live, after.live);
    }

    void CounterTests::SnapshotMetadata()
    {
        for (uint32_t index = 0; index < ResourceKindCount; ++index)
        {
            Snapshot snapshot{};
            VERIFY_IS_TRUE(TryGetSnapshot(index, snapshot));
            VERIFY_IS_TRUE(snapshot.enabled);
            VERIFY_IS_NOT_NULL(snapshot.name);
            VERIFY_IS_TRUE(wcslen(snapshot.name) < 64);
        }
        Snapshot invalid{};
        VERIFY_IS_FALSE(TryGetSnapshot(ResourceKindCount, invalid));
        VERIFY_IS_FALSE(TryGetSnapshot(UINT32_MAX, invalid));
    }
}
