// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GraphicsResourceDiagnosticsTests.h"
#include <GraphicsResourceCounters.h>
#include <ResourceDiagnostics.h>

namespace GraphicsResourceDiagnosticsTests
{
    using GraphicsResourceCounters::ResourceKind;

    void DiagnosticsTests::ResourceSnapshotContract()
    {
        VERIFY_ARE_EQUAL(E_POINTER, ResourceDiagnostics::GetResourceSnapShot(nullptr));

        XamlResourceSnapshot snapshot{};
        VERIFY_SUCCEEDED(ResourceDiagnostics::GetResourceSnapShot(&snapshot));
        for (UINT32 index = 0; index < GraphicsResourceCounters::ResourceKindCount; ++index)
        {
            GraphicsResourceCounters::Snapshot expected{};
            VERIFY_IS_TRUE(GraphicsResourceCounters::TryGetSnapshot(index, expected));
            const auto& resource = snapshot.Resources[index];
            VERIFY_ARE_EQUAL(expected.created, resource.TotalCreated);
            VERIFY_ARE_EQUAL(expected.destroyed, resource.TotalDestroyed);
            VERIFY_ARE_EQUAL(expected.live, resource.LiveCount);
        }
        for (UINT32 index = GraphicsResourceCounters::ResourceKindCount; index < XamlResourceSnapshotGraphicsCapacity; ++index)
        {
            VERIFY_ARE_EQUAL(UINT64{0}, snapshot.Resources[index].TotalCreated);
            VERIFY_ARE_EQUAL(UINT64{0}, snapshot.Resources[index].TotalDestroyed);
            VERIFY_ARE_EQUAL(UINT64{0}, snapshot.Resources[index].LiveCount);
        }
    }

    void DiagnosticsTests::ResourceSnapshotValues()
    {
        XamlResourceSnapshot before{};
        const auto beforeResult = ResourceDiagnostics::GetResourceSnapShot(&before);
        GraphicsResourceCounters::RecordCreated(ResourceKind::SpriteVisual);
        GraphicsResourceCounters::RecordCreated(ResourceKind::DCompSurfaceWrapper);
        XamlResourceSnapshot during{};
        const auto duringResult = ResourceDiagnostics::GetResourceSnapShot(&during);
        GraphicsResourceCounters::RecordDestroyed(ResourceKind::DCompSurfaceWrapper);
        XamlResourceSnapshot after{};
        const auto afterResult = ResourceDiagnostics::GetResourceSnapShot(&after);

        VERIFY_SUCCEEDED(beforeResult);
        VERIFY_SUCCEEDED(duringResult);
        VERIFY_SUCCEEDED(afterResult);
#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
        constexpr UINT64 expectedIncrement = 1;
#else
        constexpr UINT64 expectedIncrement = 0;
#endif
        const auto visualIndex = static_cast<UINT32>(ResourceKind::SpriteVisual);
        VERIFY_ARE_EQUAL(before.Resources[visualIndex].TotalCreated + expectedIncrement, during.Resources[visualIndex].TotalCreated);
        VERIFY_ARE_EQUAL(UINT64{0}, during.Resources[visualIndex].TotalDestroyed);
        VERIFY_ARE_EQUAL(UINT64{0}, during.Resources[visualIndex].LiveCount);

        const auto wrapperIndex = static_cast<UINT32>(ResourceKind::DCompSurfaceWrapper);
        VERIFY_ARE_EQUAL(before.Resources[wrapperIndex].TotalCreated + expectedIncrement, during.Resources[wrapperIndex].TotalCreated);
        VERIFY_ARE_EQUAL(before.Resources[wrapperIndex].LiveCount + expectedIncrement, during.Resources[wrapperIndex].LiveCount);
        VERIFY_ARE_EQUAL(before.Resources[wrapperIndex].TotalDestroyed + expectedIncrement, after.Resources[wrapperIndex].TotalDestroyed);
        VERIFY_ARE_EQUAL(before.Resources[wrapperIndex].LiveCount, after.Resources[wrapperIndex].LiveCount);
    }
}
