// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GraphicsResourceDiagnosticsTests.h"
#include <RenderingResourceDiagnostics.h>
#include <ResourceDiagnostics.h>
#include <XamlProfilerTracing.h>

namespace GraphicsResourceDiagnosticsTests
{
    using RenderingResourceDiagnostics::ResourceKind;

    bool DiagnosticsTests::Setup()
    {
        (void)XamlProfilerLogging::Provider();
        m_wasProfilingEnabled = XamlProfilerTracing::IsAllocationProfilingEnabled();
        XamlProfilerTracing::EnableAllocationProfiling(true);
        return true;
    }

    bool DiagnosticsTests::Cleanup()
    {
        XamlProfilerTracing::EnableAllocationProfiling(m_wasProfilingEnabled);
        return true;
    }

    void DiagnosticsTests::ResourceSnapshotContract()
    {
        VERIFY_ARE_EQUAL(E_POINTER, ResourceDiagnostics::GetResourceSnapShot(nullptr));

        XamlResourceSnapshot snapshot{};
        VERIFY_SUCCEEDED(ResourceDiagnostics::GetResourceSnapShot(&snapshot));
        for (UINT32 index = 0; index < RenderingResourceDiagnostics::ResourceKindCount; ++index)
        {
            RenderingResourceDiagnostics::Snapshot expected{};
            VERIFY_IS_TRUE(RenderingResourceDiagnostics::TryGetSnapshot(index, expected));
            const auto& resource = snapshot.Resources[index];
            VERIFY_ARE_EQUAL(expected.created, resource.TotalCreated);
            VERIFY_ARE_EQUAL(expected.destroyed, resource.TotalDestroyed);
            VERIFY_ARE_EQUAL(expected.live, resource.LiveCount);
        }
        for (UINT32 index = RenderingResourceDiagnostics::ResourceKindCount; index < XamlResourceSnapshotGraphicsCapacity; ++index)
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
        RenderingResourceDiagnostics::LogResourceCreated(ResourceKind::SpriteVisual);
        RenderingResourceDiagnostics::LogResourceCreated(ResourceKind::DCompSurfaceWrapper);
        XamlResourceSnapshot during{};
        const auto duringResult = ResourceDiagnostics::GetResourceSnapShot(&during);
        RenderingResourceDiagnostics::LogResourceDestroyed(ResourceKind::DCompSurfaceWrapper);
        XamlResourceSnapshot after{};
        const auto afterResult = ResourceDiagnostics::GetResourceSnapShot(&after);

        VERIFY_SUCCEEDED(beforeResult);
        VERIFY_SUCCEEDED(duringResult);
        VERIFY_SUCCEEDED(afterResult);
        constexpr UINT64 expectedIncrement = 1;
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

    void DiagnosticsTests::RuntimeDisabledSnapshot()
    {
        XamlResourceSnapshot before{};
        VERIFY_SUCCEEDED(ResourceDiagnostics::GetResourceSnapShot(&before));
        RenderingResourceDiagnostics::LogResourceCreated(ResourceKind::SpriteVisual);
        XamlProfilerTracing::EnableAllocationProfiling(false);
        VERIFY_IS_FALSE(RenderingResourceDiagnostics::LogResourceCreated(ResourceKind::SpriteVisual));

        XamlResourceSnapshot disabled{};
        VERIFY_SUCCEEDED(ResourceDiagnostics::GetResourceSnapShot(&disabled));
        for (const auto& resource : disabled.Resources)
        {
            VERIFY_ARE_EQUAL(UINT64{0}, resource.TotalCreated);
            VERIFY_ARE_EQUAL(UINT64{0}, resource.TotalDestroyed);
            VERIFY_ARE_EQUAL(UINT64{0}, resource.LiveCount);
        }

        XamlProfilerTracing::EnableAllocationProfiling(true);
        XamlResourceSnapshot reenabled{};
        VERIFY_SUCCEEDED(ResourceDiagnostics::GetResourceSnapShot(&reenabled));
        const auto visualIndex = static_cast<UINT32>(ResourceKind::SpriteVisual);
        VERIFY_ARE_EQUAL(before.Resources[visualIndex].TotalCreated + 1, reenabled.Resources[visualIndex].TotalCreated);
    }
}
