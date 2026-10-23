// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "..\OnlineXamlProfilerTap\SnapshotWalker.h"
#include <winrt/base.h>
#include <algorithm>
#include <cstdio>
#include <map>
#include <stdexcept>

namespace
{
    using OnlineXaml::TreeKind;
    using namespace OnlineXamlProfiler;

    struct FakeNode
    {
        UINT64 Parent{};
        UINT64 Xaml{};
        UINT64 Comp{};
        std::vector<std::pair<UINT64, UINT32>> Children;
        HRESULT Status = S_OK;
    };

    struct FakeRuntime
    {
        std::vector<UINT64> Roots{100};
        std::map<std::pair<TreeKind, UINT64>, FakeNode> Nodes{
            {{TreeKind::Xaml, 100}, {0, 100, 22, {{101, 0}, {102, 3}}}},
            {{TreeKind::Xaml, 101}, {0, 101, 23, {}}},
            {{TreeKind::Xaml, 102}, {0, 102, 0, {}}},
            {{TreeKind::Composition, 20}, {0, 0, 20, {{21, 0}}}},
            {{TreeKind::Composition, 21}, {20, 0, 21, {{22, 0}}}},
            {{TreeKind::Composition, 22}, {21, 100, 22, {{23, 0}}}},
            {{TreeKind::Composition, 23}, {22, 101, 23, {}}}
        };
        std::atomic<bool> Cancelled{};
        UINT32 AbiVersion = OnlineXaml::TreeAccessAbiVersion;
        UINT32 CancelOnRead{};
        UINT32 ReadCount{};
        bool InScope = false;
    };

    FakeRuntime* g_runtime{};

    void Check(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    void CheckStatus(HRESULT actual, HRESULT expected, const char* message)
    {
        if (actual != expected)
        {
            char detail[512]{};
            sprintf_s(detail, "%s Expected 0x%08X, received 0x%08X.",
                message, static_cast<UINT32>(expected), static_cast<UINT32>(actual));
            throw std::runtime_error(detail);
        }
    }

    HRESULT WINAPI ReadNode(void* context, TreeKind kind, UINT64 id, const OnlineXaml::NodeReader* reader)
    {
        auto& runtime = *static_cast<FakeRuntime*>(context);
        if (!runtime.InScope) { return RO_E_CLOSED; }
        if (++runtime.ReadCount == runtime.CancelOnRead) { runtime.Cancelled = true; }
        const auto found = runtime.Nodes.find({kind, id});
        if (found == runtime.Nodes.end()) { return HRESULT_FROM_WIN32(ERROR_NOT_FOUND); }
        const auto& source = found->second;
        if (FAILED(source.Status)) { return source.Status; }
        OnlineXaml::NativeNode node;
        node.Tree = kind;
        node.Id = id;
        node.ParentId = source.Parent;
        node.XamlId = source.Xaml;
        node.CompId = source.Comp;
        node.Type = kind == TreeKind::Xaml ? L"Test.UIElement" : L"Test.HWCompNode";
        node.Name = L"Test node";
        node.ActualWidth = 120.5;
        node.ActualHeight = 48;
        node.IsVisible = TRUE;
        auto status = reader->Node(reader->Context, &node);
        if (FAILED(status)) { return status; }
        if (reader->Child)
        {
            for (const auto& [child, index] : source.Children)
            {
                status = reader->Child(reader->Context, child, index);
                if (FAILED(status)) { return status; }
            }
        }
        return S_OK;
    }

    HRESULT WINAPI WithTreeAccess(UINT32 version, void* context, OnlineXaml::AccessCallback callback)
    {
        if (version != OnlineXaml::TreeAccessAbiVersion) { return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH); }
        auto& runtime = *g_runtime;
        OnlineXaml::TreeAccess access;
        access.AbiVersion = runtime.AbiVersion;
        access.ThreadId = GetCurrentThreadId();
        access.RootCount = static_cast<UINT32>(runtime.Roots.size());
        access.Roots = runtime.Roots.data();
        access.Context = &runtime;
        access.ReadNode = ReadNode;
        runtime.InScope = true;
        const auto status = callback(context, &access);
        runtime.InScope = false;
        return status;
    }

    HRESULT Capture(FakeRuntime& runtime, ThreadCapture& capture, UINT32 maximumNodes = MaximumNodeCount)
    {
        g_runtime = &runtime;
        return CaptureSnapshot(WithTreeAccess, maximumNodes, runtime.Cancelled, capture);
    }

    const NodeRecord& Find(const ThreadCapture& capture, TreeKind kind, UINT64 id)
    {
        const auto node = std::find_if(capture.Nodes.begin(), capture.Nodes.end(),
            [&](const auto& value) { return value.Tree == kind && value.Id == id; });
        Check(node != capture.Nodes.end(), "Expected captured node is missing.");
        return *node;
    }

    void CapturesPrivateAncestryAndCopiesData()
    {
        FakeRuntime runtime;
        ThreadCapture capture;
        CheckStatus(Capture(runtime, capture), S_OK, "TAP walker failed on the fake native graph.");
        Check(!runtime.InScope && capture.Nodes.size() == 7, "Capture did not finish its native access scope.");
        Check(Find(capture, TreeKind::Composition, 20).ParentId == 0, "Internal composition root was lost.");
        Check(Find(capture, TreeKind::Composition, 22).ParentId == 21, "Private composition ancestry was flattened.");
        Check(Find(capture, TreeKind::Composition, 23).XamlId == 101, "Composition-to-XAML link was lost.");
        const auto& child = Find(capture, TreeKind::Xaml, 102);
        Check(child.ParentId == 100 && child.ChildIndex == 3, "Native child ordering was not preserved.");
        Check(child.Type == L"Test.UIElement" && child.Name == L"Test node", "Borrowed strings were not copied.");
        Check(child.Properties[0].Name == L"ActualWidth" && child.Properties[0].Value == L"120.5",
            "Scalar property formatting did not move into the TAP.");
        runtime.Nodes.clear();
        Check(child.Name == L"Test node", "Snapshot depends on the native graph after capture.");
    }

    void EnforcesBudgetAcrossTrees()
    {
        for (UINT32 maximumNodes : {3u, 5u})
        {
            FakeRuntime runtime;
            ThreadCapture capture;
            CheckStatus(Capture(runtime, capture, maximumNodes), HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW),
                "Node limit was not shared across the XAML and HWCompNode trees.");
            Check(capture.Nodes.size() == maximumNodes, "TAP exceeded its capture node limit.");
        }
    }

    void RejectsNativeParentCycles()
    {
        FakeRuntime runtime;
        runtime.Nodes.at({TreeKind::Composition, 20}).Parent = 23;
        ThreadCapture capture;
        CheckStatus(Capture(runtime, capture), HRESULT_FROM_WIN32(ERROR_CIRCULAR_DEPENDENCY),
            "A native parent cycle was not reported.");
    }

    void CancelsBeforeAndDuringTraversal()
    {
        FakeRuntime runtime;
        ThreadCapture capture;
        runtime.Cancelled = true;
        CheckStatus(Capture(runtime, capture), HRESULT_FROM_WIN32(ERROR_CANCELLED),
            "Pre-cancelled capture returned the wrong error.");
        Check(runtime.ReadCount == 0,
            "Cancelled capture touched live native nodes.");
        runtime.Cancelled = false;
        runtime.CancelOnRead = 2;
        CheckStatus(Capture(runtime, capture), HRESULT_FROM_WIN32(ERROR_CANCELLED),
            "Mid-traversal cancellation returned the wrong error.");
        Check(runtime.ReadCount == 2,
            "TAP did not stop traversing after cancellation.");
    }

    void ReportsNativeReadFailures()
    {
        FakeRuntime runtime;
        runtime.Nodes.at({TreeKind::Composition, 21}).Status = E_ACCESSDENIED;
        ThreadCapture capture;
        CheckStatus(Capture(runtime, capture), E_ACCESSDENIED, "Native read failure was silently replaced.");
    }

    void RejectsIncompatibleAdapter()
    {
        FakeRuntime runtime;
        runtime.AbiVersion++;
        ThreadCapture capture;
        CheckStatus(Capture(runtime, capture), HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH),
            "An incompatible native adapter returned the wrong error.");
        Check(runtime.ReadCount == 0,
            "TAP used an incompatible native adapter.");
    }
}

int main()
try
{
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    CapturesPrivateAncestryAndCopiesData();
    EnforcesBudgetAcrossTrees();
    RejectsNativeParentCycles();
    CancelsBeforeAndDuringTraversal();
    ReportsNativeReadFailures();
    RejectsIncompatibleAdapter();
    winrt::uninit_apartment();
    puts("Passed: 6 TAP walker tests (native ancestry, copied data, budget, cycles, cancellation, errors, and ABI).");
    return 0;
}
catch (const std::exception& exception)
{
    fprintf(stderr, "TAP walker test failed: %s\n", exception.what());
    return 1;
}
catch (...)
{
    fprintf(stderr, "TAP walker test failed: HRESULT 0x%08X\n", static_cast<UINT32>(winrt::to_hresult()));
    return 1;
}
