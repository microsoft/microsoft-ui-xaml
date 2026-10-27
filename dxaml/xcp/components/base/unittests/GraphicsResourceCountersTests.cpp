// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GraphicsResourceCountersTests.h"
#include <GraphicsResourceCounters.h>
#include <array>
#include <thread>

namespace GraphicsResourceCountersTests
{
    using namespace GraphicsResourceCounters;

#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
    constexpr uint64_t ExpectedIncrement = 1;
#else
    constexpr uint64_t ExpectedIncrement = 0;
#endif

    Snapshot Read(ResourceKind kind)
    {
        Snapshot snapshot{};
        VERIFY_IS_TRUE(TryGetSnapshot(static_cast<uint32_t>(kind), snapshot));
        return snapshot;
    }

    void CounterTests::FactoryResults()
    {
        const auto before = Read(ResourceKind::SpriteVisual);
        VERIFY_ARE_EQUAL(E_OUTOFMEMORY, RecordCreation(ResourceKind::SpriteVisual, E_OUTOFMEMORY));
        VERIFY_ARE_EQUAL(before.created, Read(ResourceKind::SpriteVisual).created);
        VERIFY_ARE_EQUAL(S_OK, RecordCreation(ResourceKind::SpriteVisual, S_OK));
        VERIFY_ARE_EQUAL(before.created + ExpectedIncrement, Read(ResourceKind::SpriteVisual).created);
    }

    void CounterTests::WrapperLifetime()
    {
        const auto before = Read(ResourceKind::DCompSurfaceWrapper);
        RecordCreated(ResourceKind::DCompSurfaceWrapper);
        RecordCreated(ResourceKind::DCompSurfaceWrapper);
        const auto during = Read(ResourceKind::DCompSurfaceWrapper);
        VERIFY_ARE_EQUAL(before.live + 2 * ExpectedIncrement, during.live);
        RecordDestroyed(ResourceKind::DCompSurfaceWrapper);
        RecordDestroyed(ResourceKind::DCompSurfaceWrapper);
        const auto after = Read(ResourceKind::DCompSurfaceWrapper);
        VERIFY_ARE_EQUAL(before.live, after.live);
        VERIFY_ARE_EQUAL(before.created + 2 * ExpectedIncrement, after.created);
        VERIFY_ARE_EQUAL(before.destroyed + 2 * ExpectedIncrement, after.destroyed);
    }

    void CounterTests::IndependentCategories()
    {
        const auto before = Read(ResourceKind::ContainerVisual);
        RecordCreated(ResourceKind::ColorBrush);
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
            RecordCreated(ResourceKind::D2DGeometryWrapper);
            RecordDestroyed(ResourceKind::D2DGeometryWrapper);
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
        VERIFY_ARE_EQUAL(before.created + 4000 * ExpectedIncrement, after.created);
        VERIFY_ARE_EQUAL(before.destroyed + 4000 * ExpectedIncrement, after.destroyed);
        VERIFY_ARE_EQUAL(before.live, after.live);
    }

    void CounterTests::SnapshotMetadata()
    {
        for (uint32_t index = 0; index < ResourceKindCount; ++index)
        {
            Snapshot snapshot{};
            VERIFY_IS_TRUE(TryGetSnapshot(index, snapshot));
            VERIFY_ARE_EQUAL(ExpectedIncrement != 0, snapshot.enabled);
            VERIFY_IS_NOT_NULL(snapshot.name);
            VERIFY_IS_TRUE(wcslen(snapshot.name) < 64);
        }
        Snapshot invalid{};
        VERIFY_IS_FALSE(TryGetSnapshot(ResourceKindCount, invalid));
        VERIFY_IS_FALSE(TryGetSnapshot(UINT32_MAX, invalid));
    }
}
