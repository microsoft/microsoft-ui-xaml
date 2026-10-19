// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <cstdint>

namespace OnlineXaml
{
    inline constexpr UINT32 SnapshotAbiVersion = 1;
    inline constexpr UINT32 MaximumNodeCount = 100000;

    enum class TreeKind : UINT32
    {
        Xaml = 1,
        Composition = 2,
        Visual = 3
    };

    struct Node
    {
        UINT32 Size = sizeof(Node);
        TreeKind Tree = TreeKind::Xaml;
        UINT64 Id = 0;
        UINT64 ParentId = 0;
        UINT32 ChildIndex = 0;
        UINT64 XamlId = 0;
        UINT64 CompId = 0;
        UINT64 VisualId = 0;
        PCWSTR Type = nullptr;
        PCWSTR Name = nullptr;
    };

    struct CaptureInfo
    {
        UINT32 Size = sizeof(CaptureInfo);
        DWORD ThreadId = 0;
        INT64 StartQpc = 0;
        INT64 EndQpc = 0;
        INT64 QpcFrequency = 0;
        UINT32 NodeCount = 0;
    };

    // Strings and node pointers are borrowed only for the duration of each callback.
    // The sink must copy values, must not pump messages, and must not inspect live objects.
    struct Sink
    {
        UINT32 Size = sizeof(Sink);
        void* Context = nullptr;
        HRESULT(WINAPI* AddNode)(void* context, const Node* node) = nullptr;
        HRESULT(WINAPI* AddProperty)(
            void* context, TreeKind tree, UINT64 id, PCWSTR name, PCWSTR value, HRESULT status) = nullptr;
    };

    using CaptureSnapshot = HRESULT(WINAPI*)(
        UINT32 abiVersion, UINT32 maximumNodes, const Sink* sink, CaptureInfo* info);
}
