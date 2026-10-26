// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "SnapshotWalker.h"
#include <wil/cppwinrt.h>
#include <wil/result.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
    using OnlineXaml::TreeKind;
    using namespace OnlineXamlProfiler;
    using winrt::Microsoft::UI::Composition::Visual;
    using winrt::Microsoft::UI::Composition::ContainerVisual;

    UINT64 VisualId(const Visual& visual)
    {
        return static_cast<UINT64>(reinterpret_cast<uintptr_t>(winrt::get_abi(visual)));
    }

    void CheckCancelled(const std::atomic<bool>& cancelled)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_CANCELLED), cancelled.load());
    }

    struct NativeRecord
    {
        OnlineXaml::NativeNode Values;
        std::wstring Type;
        std::wstring Name;
        Visual BackingVisual{nullptr};
        Visual MainTreeVisual{nullptr};
        std::vector<std::pair<UINT64, UINT32>> Children;
        const std::atomic<bool>& Cancelled;
        UINT32 MaximumChildren;
        bool Copied = false;
    };

    HRESULT WINAPI CopyNode(void* context, const OnlineXaml::NativeNode* node) noexcept
    try
    {
        auto& record = *static_cast<NativeRecord*>(context);
        CheckCancelled(record.Cancelled);
        THROW_HR_IF(E_INVALIDARG, !node || node->Size != sizeof(*node) || !node->Id || !node->Type || record.Copied);
        record.Values = *node;
        record.Values.Type = record.Values.Name = nullptr;
        record.Values.Visual = record.Values.MainTreeVisual = nullptr;
        record.Type = node->Type;
        record.Name = node->Name ? node->Name : L"";
        if (node->Visual)
        {
            THROW_IF_FAILED(node->Visual->QueryInterface(winrt::guid_of<Visual>(), winrt::put_abi(record.BackingVisual)));
        }
        if (node->MainTreeVisual)
        {
            THROW_IF_FAILED(node->MainTreeVisual->QueryInterface(winrt::guid_of<Visual>(), winrt::put_abi(record.MainTreeVisual)));
        }
        record.Copied = true;
        return S_OK;
    }
    catch (...)
    {
        return winrt::to_hresult();
    }

    HRESULT WINAPI CopyChild(void* context, UINT64 id, UINT32 index) noexcept
    try
    {
        auto& record = *static_cast<NativeRecord*>(context);
        CheckCancelled(record.Cancelled);
        THROW_HR_IF(E_INVALIDARG, !record.Copied || !id ||
            (!record.Children.empty() && index <= record.Children.back().second));
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), record.Children.size() >= record.MaximumChildren);
        record.Children.emplace_back(id, index);
        return S_OK;
    }
    catch (...)
    {
        return winrt::to_hresult();
    }

    template<typename T, typename Getter>
    HRESULT ReadProperty(T& value, Getter getter) noexcept
    try
    {
        value = getter();
        return S_OK;
    }
    catch (...)
    {
        return winrt::to_hresult();
    }

    class SnapshotWalker
    {
    public:
        SnapshotWalker(const OnlineXaml::TreeAccess& access, UINT32 maximumNodes,
            const std::atomic<bool>& cancelled, ThreadCapture& capture)
            : m_access(access), m_maximumNodes(maximumNodes), m_cancelled(cancelled), m_capture(capture)
        {
        }

        void Capture()
        {
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH),
                m_access.AbiVersion != OnlineXaml::TreeAccessAbiVersion);
            THROW_HR_IF(E_INVALIDARG, m_access.Size != sizeof(m_access) || !m_access.ReadNode ||
                (m_access.RootCount && !m_access.Roots));
            THROW_HR_IF(RPC_E_WRONG_THREAD, m_access.ThreadId != GetCurrentThreadId());
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), m_access.RootCount > m_maximumNodes);
            CaptureXaml();
            CaptureComposition();
            CaptureVisuals();
        }

    private:
        NativeRecord ReadNative(TreeKind kind, UINT64 id, bool children = true)
        {
            CheckCancelled(m_cancelled);
            NativeRecord record{{}, {}, {}, nullptr, nullptr, {}, m_cancelled, m_maximumNodes};
            OnlineXaml::NodeReader reader;
            reader.Context = &record;
            reader.Node = CopyNode;
            reader.Child = children ? CopyChild : nullptr;
            THROW_IF_FAILED(m_access.ReadNode(m_access.Context, kind, id, &reader));
            THROW_HR_IF(E_INVALIDARG, !record.Copied || record.Values.Id != id || record.Values.Tree != kind);
            return record;
        }

        NodeRecord& AddNode(TreeKind kind, UINT64 id, UINT64 parent, UINT32 index)
        {
            CheckCancelled(m_cancelled);
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), m_capture.Nodes.size() >= m_maximumNodes);
            auto& node = m_capture.Nodes.emplace_back();
            node.Tree = kind;
            node.Id = id;
            node.ParentId = parent;
            node.ChildIndex = index;
            return node;
        }

        static void Property(NodeRecord& node, PCWSTR name, PCWSTR value, HRESULT status = S_OK)
        {
            node.Properties.push_back({name, value, status});
        }

        static void Number(NodeRecord& node, PCWSTR name, double value, HRESULT status = S_OK)
        {
            wchar_t text[64]{};
            if (SUCCEEDED(status)) { swprintf_s(text, L"%.9g", value); }
            Property(node, name, text, status);
        }

        void CaptureXaml()
        {
            struct Pending { UINT64 Id; UINT64 Parent; UINT32 Index; };
            std::vector<Pending> pending;
            for (UINT32 index = m_access.RootCount; index > 0; --index)
            {
                pending.push_back({m_access.Roots[index - 1], 0, index - 1});
            }
            std::unordered_set<UINT64> visited;
            while (!pending.empty())
            {
                auto entry = pending.back();
                pending.pop_back();
                if (!visited.insert(entry.Id).second) { continue; }
                auto captured = ReadNative(TreeKind::Xaml, entry.Id);
                auto& node = AddNode(TreeKind::Xaml, entry.Id, entry.Parent, entry.Index);
                node.Type = std::move(captured.Type);
                node.Name = std::move(captured.Name);
                node.XamlId = node.Id;
                node.CompId = captured.Values.CompId;
                Number(node, L"ActualWidth", captured.Values.ActualWidth);
                Number(node, L"ActualHeight", captured.Values.ActualHeight);
                Property(node, L"IsVisible", captured.Values.IsVisible ? L"true" : L"false");
                if (node.CompId) { m_compSeeds.push_back(node.CompId); }
                for (auto child = captured.Children.rbegin(); child != captured.Children.rend(); ++child)
                {
                    pending.push_back({child->first, node.Id, child->second});
                }
            }
        }

        void CaptureComposition()
        {
            struct Pending { UINT64 Id; UINT64 Parent; UINT32 Index; };
            std::vector<Pending> pending;
            std::unordered_set<UINT64> roots;
            for (auto seed : m_compSeeds)
            {
                auto root = seed;
                std::unordered_set<UINT64> ancestors;
                while (true)
                {
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_CIRCULAR_DEPENDENCY), !ancestors.insert(root).second);
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), ancestors.size() > m_maximumNodes);
                    auto captured = ReadNative(TreeKind::Composition, root, false);
                    if (!captured.Values.ParentId) { break; }
                    root = captured.Values.ParentId;
                }
                if (roots.insert(root).second)
                {
                    pending.push_back({root, 0, static_cast<UINT32>(roots.size() - 1)});
                }
            }
            std::unordered_set<UINT64> visited;
            while (!pending.empty())
            {
                auto entry = pending.back();
                pending.pop_back();
                if (!visited.insert(entry.Id).second) { continue; }
                auto captured = ReadNative(TreeKind::Composition, entry.Id);
                auto& node = AddNode(TreeKind::Composition, entry.Id, entry.Parent, entry.Index);
                node.Type = std::move(captured.Type);
                node.CompId = node.Id;
                node.XamlId = captured.Values.XamlId;
                node.VisualId = VisualId(captured.BackingVisual);
                m_compOwners[node.Id] = node.XamlId;
                for (auto visual : {captured.BackingVisual, captured.MainTreeVisual})
                {
                    if (visual && m_visualOwners.emplace(VisualId(visual), node.Id).second)
                    {
                        m_visualSeeds.push_back(visual);
                    }
                }
                for (auto child = captured.Children.rbegin(); child != captured.Children.rend(); ++child)
                {
                    pending.push_back({child->first, node.Id, child->second});
                }
            }
        }

        static void VisualProperties(const Visual& visual, NodeRecord& node)
        {
            winrt::Windows::Foundation::Numerics::float2 size{};
            auto status = ReadProperty(size, [&] { return visual.Size(); });
            Number(node, L"Width", size.x, status);
            Number(node, L"Height", size.y, status);
            winrt::Windows::Foundation::Numerics::float3 offset{};
            status = ReadProperty(offset, [&] { return visual.Offset(); });
            Number(node, L"OffsetX", offset.x, status);
            Number(node, L"OffsetY", offset.y, status);
            Number(node, L"OffsetZ", offset.z, status);
            float opacity{};
            status = ReadProperty(opacity, [&] { return visual.Opacity(); });
            Number(node, L"Opacity", opacity, status);
            bool visible{};
            status = ReadProperty(visible, [&] { return visual.IsVisible(); });
            Property(node, L"IsVisible", visible ? L"true" : L"false", status);
            winrt::Windows::Foundation::Numerics::float3 scale{};
            status = ReadProperty(scale, [&] { return visual.Scale(); });
            Number(node, L"ScaleX", scale.x, status);
            Number(node, L"ScaleY", scale.y, status);
            Number(node, L"ScaleZ", scale.z, status);
            float rotation{};
            status = ReadProperty(rotation, [&] { return visual.RotationAngleInDegrees(); });
            Number(node, L"RotationAngleInDegrees", rotation, status);
        }

        void CaptureVisuals()
        {
            struct Pending { Visual Value; UINT64 Parent; UINT32 Index; UINT64 Owner; };
            std::vector<Pending> pending;
            std::unordered_set<UINT64> roots;
            for (auto visual : m_visualSeeds)
            {
                std::unordered_set<UINT64> ancestors;
                while (true)
                {
                    CheckCancelled(m_cancelled);
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_CIRCULAR_DEPENDENCY), !ancestors.insert(VisualId(visual)).second);
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), ancestors.size() > m_maximumNodes);
                    auto parent = visual.Parent();
                    if (!parent) { break; }
                    visual = parent;
                }
                if (roots.insert(VisualId(visual)).second)
                {
                    pending.push_back({visual, 0, static_cast<UINT32>(roots.size() - 1), 0});
                }
            }
            std::unordered_set<UINT64> visited;
            while (!pending.empty())
            {
                auto entry = std::move(pending.back());
                pending.pop_back();
                auto id = VisualId(entry.Value);
                if (!visited.insert(id).second) { continue; }
                auto owner = m_visualOwners.find(id);
                if (owner != m_visualOwners.end()) { entry.Owner = owner->second; }
                auto& node = AddNode(TreeKind::Visual, id, entry.Parent, entry.Index);
                node.Type = winrt::get_class_name(entry.Value);
                node.VisualId = id;
                node.CompId = entry.Owner;
                node.XamlId = entry.Owner ? m_compOwners.at(entry.Owner) : 0;
                VisualProperties(entry.Value, node);
                Property(node, L"CompLink",
                    owner != m_visualOwners.end() ? L"Direct" : (entry.Owner ? L"NearestAncestor" : L"None"));
                ContainerVisual container{nullptr};
                const auto status = entry.Value.as<IUnknown>()->QueryInterface(
                    winrt::guid_of<ContainerVisual>(), winrt::put_abi(container));
                if (status == E_NOINTERFACE) { continue; }
                THROW_IF_FAILED(status);
                std::vector<Visual> children;
                for (auto child : container.Children())
                {
                    CheckCancelled(m_cancelled);
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), children.size() >= m_maximumNodes);
                    children.push_back(child);
                }
                for (size_t index = children.size(); index > 0; --index)
                {
                    pending.push_back({children[index - 1], id, static_cast<UINT32>(index - 1), entry.Owner});
                }
            }
        }

        const OnlineXaml::TreeAccess& m_access;
        UINT32 m_maximumNodes;
        const std::atomic<bool>& m_cancelled;
        ThreadCapture& m_capture;
        std::vector<UINT64> m_compSeeds;
        std::vector<Visual> m_visualSeeds;
        std::unordered_map<UINT64, UINT64> m_visualOwners;
        std::unordered_map<UINT64, UINT64> m_compOwners;
    };

    struct WalkContext
    {
        UINT32 MaximumNodes;
        const std::atomic<bool>& Cancelled;
        ThreadCapture& Capture;
    };

    HRESULT WINAPI Walk(void* context, const OnlineXaml::TreeAccess* access) noexcept
    try
    {
        THROW_HR_IF_NULL(E_INVALIDARG, access);
        auto& walk = *static_cast<WalkContext*>(context);
        SnapshotWalker(*access, walk.MaximumNodes, walk.Cancelled, walk.Capture).Capture();
        return S_OK;
    }
    catch (...)
    {
        return winrt::to_hresult();
    }
}

HRESULT OnlineXamlProfiler::CaptureSnapshot(
    OnlineXaml::WithTreeAccess withTreeAccess, UINT32 maximumNodes,
    const std::atomic<bool>& cancelled, ThreadCapture& capture) noexcept
try
{
    THROW_HR_IF(E_INVALIDARG, !withTreeAccess || !maximumNodes || maximumNodes > MaximumNodeCount);
    capture = {};
    capture.ThreadId = GetCurrentThreadId();
    LARGE_INTEGER time{};
    QueryPerformanceFrequency(&time);
    capture.QpcFrequency = time.QuadPart;
    QueryPerformanceCounter(&time);
    capture.StartQpc = time.QuadPart;
    CheckCancelled(cancelled);
    WalkContext context{maximumNodes, cancelled, capture};
    const auto status = withTreeAccess(OnlineXaml::TreeAccessAbiVersion, &context, Walk);
    QueryPerformanceCounter(&time);
    capture.EndQpc = time.QuadPart;
    return status;
}
catch (...)
{
    return winrt::to_hresult();
}
