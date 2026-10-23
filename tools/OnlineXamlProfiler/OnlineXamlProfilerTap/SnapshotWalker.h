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
    };

    struct ThreadCapture
    {
        DWORD ThreadId{};
        INT64 StartQpc{};
        INT64 EndQpc{};
        INT64 QpcFrequency{};
        std::vector<NodeRecord> Nodes;
    };

    HRESULT CaptureSnapshot(
        OnlineXaml::WithTreeAccess withTreeAccess, UINT32 maximumNodes,
        const std::atomic<bool>& cancelled, ThreadCapture& capture) noexcept;
}
