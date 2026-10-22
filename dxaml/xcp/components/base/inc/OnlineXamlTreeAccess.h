// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <unknwn.h>
#include <cstdint>

namespace OnlineXaml
{
    inline constexpr UINT32 TreeAccessAbiVersion = 1;

    enum class TreeKind : UINT32
    {
        Xaml = 1,
        Composition = 2,
        Visual = 3
    };

    struct NativeNode
    {
        UINT32 Size = sizeof(NativeNode);
        TreeKind Tree = TreeKind::Xaml;
        UINT64 Id = 0;
        UINT64 ParentId = 0;
        UINT64 XamlId = 0;
        UINT64 CompId = 0;
        PCWSTR Type = nullptr;
        PCWSTR Name = nullptr;
        double ActualWidth = 0;
        double ActualHeight = 0;
        BOOL IsVisible = FALSE;
        IUnknown* Visual = nullptr;
        IUnknown* MainTreeVisual = nullptr;
    };

    // Metadata and COM pointers are borrowed only during Node. Copy values and
    // AddRef any visuals needed for the rest of the synchronous access scope.
    struct NodeReader
    {
        UINT32 Size = sizeof(NodeReader);
        void* Context = nullptr;
        HRESULT(WINAPI* Node)(void* context, const NativeNode* node) = nullptr;
        // Optional: enumerate only this node's immediate children, in order.
        HRESULT(WINAPI* Child)(void* context, UINT64 id, UINT32 index) = nullptr;
    };

    struct TreeAccess
    {
        UINT32 Size = sizeof(TreeAccess);
        UINT32 AbiVersion = TreeAccessAbiVersion;
        DWORD ThreadId = 0;
        UINT32 RootCount = 0;
        const UINT64* Roots = nullptr;
        void* Context = nullptr;
        HRESULT(WINAPI* ReadNode)(
            void* context, TreeKind tree, UINT64 id, const NodeReader* reader) = nullptr;
    };

    // The callback, adapter, native IDs, and acquired COM references must remain
    // on this UI thread and must not outlive the callback. Do not pump messages,
    // mutate live objects, or let exceptions cross this private ABI.
    using AccessCallback = HRESULT(WINAPI*)(void* context, const TreeAccess* access);
    using WithTreeAccess = HRESULT(WINAPI*)(UINT32 abiVersion, void* context, AccessCallback callback);
}
