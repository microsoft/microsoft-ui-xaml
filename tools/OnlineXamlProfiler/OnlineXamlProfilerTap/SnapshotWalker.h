// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "OnlineXamlTreeAccess.h"
#include <atomic>
#include <string>
#include <vector>

namespace OnlineXamlProfiler
{
    inline constexpr UINT32 MaximumNodeCount = 100000;
    inline constexpr UINT32 MaximumSurfaceReferenceCount = 100000;

    struct PropertyRecord
    {
        std::wstring Name;
        std::wstring Value;
        HRESULT Status{};
    };

    struct NodeRecord
    {
        OnlineXaml::TreeKind Tree{};
        UINT64 Id{};
        UINT64 ParentId{};
        UINT32 ChildIndex{};
        UINT64 XamlId{};
        UINT64 CompId{};
        UINT64 VisualId{};
        std::wstring Type;
        std::wstring Name;
        std::vector<PropertyRecord> Properties;
        HRESULT CoreAllocationStatus = E_NOTIMPL;
        UINT64 CoreAllocationBytes = 0;
    };

    struct SurfaceReference
    {
        UINT64 VisualId = 0;
        std::wstring Role;
        UINT64 CompositionSurfaceId = 0;
        HRESULT Status = E_NOTIMPL;
        OnlineXaml::NativeSurface Surface;
    };

    struct ThreadCapture
    {
        DWORD ThreadId{};
        INT64 StartQpc{};
        INT64 EndQpc{};
        INT64 QpcFrequency{};
        std::vector<NodeRecord> Nodes;
        HRESULT MemoryStatus = E_NOTIMPL;
        OnlineXaml::NativeMemory Memory;
        std::vector<SurfaceReference> Surfaces;
    };

    HRESULT CaptureSnapshot(
        OnlineXaml::WithTreeAccess withTreeAccess, UINT32 maximumNodes,
        const std::atomic<bool>& cancelled, ThreadCapture& capture) noexcept;
}
