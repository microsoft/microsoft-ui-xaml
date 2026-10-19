// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "OnlineXamlSnapshot.h"
#include "DXamlServices.h"
#include "hwcompnode.h"
#include "ContentRootCoordinator.h"
#include <unordered_set>

namespace
{
    template<typename T>
    UINT64 Address(T* value)
    {
        return static_cast<UINT64>(reinterpret_cast<uintptr_t>(value));
    }

    class SnapshotWalker
    {
    public:
        SnapshotWalker(const OnlineXaml::Sink& sink, UINT32 maximumNodes, OnlineXaml::CaptureInfo& info)
            : m_sink(sink), m_maximumNodes(maximumNodes), m_info(info)
        {
        }

        HRESULT Capture(CCoreServices& core)
        {
            std::vector<CUIElement*> roots;
            for (const auto& contentRoot : core.GetContentRootCoordinator()->GetContentRoots())
            {
                if (!contentRoot->IsShuttingDown())
                {
                    contentRoot->GetVisualTreeNoRef()->GetExistingRootsNoRef(roots);
                }
            }

            IFC_RETURN(CaptureXaml(roots));
            IFC_RETURN(CaptureComposition());
            return CaptureVisuals();
        }

    private:
        HRESULT AddNode(const OnlineXaml::Node& node)
        {
            if (m_info.NodeCount >= m_maximumNodes)
            {
                return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            }
            IFC_RETURN(m_sink.AddNode(m_sink.Context, &node));
            ++m_info.NodeCount;
            return S_OK;
        }

        HRESULT Property(OnlineXaml::TreeKind tree, UINT64 id, PCWSTR name, PCWSTR value, HRESULT status = S_OK)
        {
            return m_sink.AddProperty(m_sink.Context, tree, id, name, value, status);
        }

        HRESULT Number(OnlineXaml::TreeKind tree, UINT64 id, PCWSTR name, double value, HRESULT status = S_OK)
        {
            wchar_t text[64]{};
            if (SUCCEEDED(status))
            {
                swprintf_s(text, L"%.9g", value);
            }
            return Property(tree, id, name, text, status);
        }

        HRESULT CaptureXaml(const std::vector<CUIElement*>& roots)
        {
            struct Pending
            {
                CUIElement* Element;
                UINT64 Parent;
                UINT32 Index;
            };
            std::vector<Pending> pending;
            for (size_t index = roots.size(); index > 0; --index)
            {
                pending.push_back({roots[index - 1], 0, static_cast<UINT32>(index - 1)});
            }
            std::unordered_set<CUIElement*> visited;
            while (!pending.empty())
            {
                auto entry = pending.back();
                pending.pop_back();
                auto element = entry.Element;
                if (!element || !visited.insert(element).second)
                {
                    continue;
                }

                OnlineXaml::Node node;
                node.Tree = OnlineXaml::TreeKind::Xaml;
                node.Id = Address(element);
                node.ParentId = entry.Parent;
                node.ChildIndex = entry.Index;
                node.XamlId = node.Id;
                node.CompId = Address(element->GetCompositionPeer());
                auto type = element->GetClassInformation()->GetFullName();
                node.Type = type.GetBuffer();
                node.Name = element->m_strName.GetBuffer();
                IFC_RETURN(AddNode(node));
                IFC_RETURN(Number(node.Tree, node.Id, L"ActualWidth", element->GetActualWidth()));
                IFC_RETURN(Number(node.Tree, node.Id, L"ActualHeight", element->GetActualHeight()));
                IFC_RETURN(Property(node.Tree, node.Id, L"IsVisible", element->IsVisible() ? L"true" : L"false"));

                if (auto comp = element->GetCompositionPeer())
                {
                    m_compSeeds.push_back(comp);
                }
                if (auto children = element->GetChildren())
                {
                    const auto& collection = children->GetCollection();
                    for (size_t index = collection.size(); index > 0; --index)
                    {
                        auto child = do_pointer_cast<CUIElement>(collection[index - 1]);
                        if (child)
                        {
                            pending.push_back({child, node.Id, static_cast<UINT32>(index - 1)});
                        }
                    }
                }
            }
            return S_OK;
        }

        HRESULT CaptureComposition()
        {
            struct Pending
            {
                HWCompNode* Comp;
                UINT64 Parent;
                UINT32 Index;
            };
            std::vector<Pending> pending;
            std::unordered_set<HWCompNode*> roots;
            for (auto seed : m_compSeeds)
            {
                HWCompNode* root = seed;
                std::unordered_set<HWCompNode*> ancestors;
                while (auto parent = do_pointer_cast<HWCompNode>(root->GetParentInternal(false /* publicOnly */)))
                {
                    if (!ancestors.insert(root).second || ancestors.size() > m_maximumNodes)
                    {
                        return HRESULT_FROM_WIN32(ERROR_CIRCULAR_DEPENDENCY);
                    }
                    root = parent;
                }
                if (roots.insert(root).second)
                {
                    pending.push_back({root, 0, static_cast<UINT32>(roots.size() - 1)});
                }
            }

            std::unordered_set<HWCompNode*> visited;
            while (!pending.empty())
            {
                auto entry = pending.back();
                pending.pop_back();
                auto comp = entry.Comp;
                if (!visited.insert(comp).second)
                {
                    continue;
                }
                auto tree = do_pointer_cast<HWCompTreeNode>(comp);
                OnlineXaml::Node node;
                node.Tree = OnlineXaml::TreeKind::Composition;
                node.Id = Address(comp);
                node.ParentId = entry.Parent;
                node.ChildIndex = entry.Index;
                node.CompId = node.Id;
                auto type = comp->GetClassInformation()->GetFullName();
                node.Type = type.GetBuffer();
                if (tree)
                {
                    node.XamlId = Address(tree->GetUIElementPeer());
                    auto visual = tree->GetWUCVisual();
                    node.VisualId = Address(visual.get());
                    if (visual)
                    {
                        m_visualSeeds.push_back(visual);
                        m_visualOwners[visual.get()] = node.Id;
                    }
                    auto placeholder = tree->GetWUCVisualInMainTree();
                    if (placeholder && placeholder.get() != visual.get())
                    {
                        m_visualSeeds.push_back(placeholder);
                        m_visualOwners[placeholder.get()] = node.Id;
                    }
                }
                m_compOwners[node.Id] = node.XamlId;
                IFC_RETURN(AddNode(node));
                if (tree)
                {
                    std::vector<HWCompNode*> children(tree->GetChildrenBegin(), tree->GetChildrenEnd());
                    for (size_t index = children.size(); index > 0; --index)
                    {
                        pending.push_back({children[index - 1], node.Id, static_cast<UINT32>(index - 1)});
                    }
                }
            }
            return S_OK;
        }

        HRESULT VisualProperties(WUComp::IVisual* visual, UINT64 id)
        {
            constexpr auto kind = OnlineXaml::TreeKind::Visual;
            wfn::Vector2 size{};
            auto status = visual->get_Size(&size);
            IFC_RETURN(Number(kind, id, L"Width", size.X, status));
            IFC_RETURN(Number(kind, id, L"Height", size.Y, status));
            wfn::Vector3 offset{};
            status = visual->get_Offset(&offset);
            IFC_RETURN(Number(kind, id, L"OffsetX", offset.X, status));
            IFC_RETURN(Number(kind, id, L"OffsetY", offset.Y, status));
            IFC_RETURN(Number(kind, id, L"OffsetZ", offset.Z, status));
            float opacity{};
            status = visual->get_Opacity(&opacity);
            IFC_RETURN(Number(kind, id, L"Opacity", opacity, status));
            boolean visible{};
            status = visual->get_IsVisible(&visible);
            IFC_RETURN(Property(kind, id, L"IsVisible", visible ? L"true" : L"false", status));
            wfn::Vector3 scale{};
            status = visual->get_Scale(&scale);
            IFC_RETURN(Number(kind, id, L"ScaleX", scale.X, status));
            IFC_RETURN(Number(kind, id, L"ScaleY", scale.Y, status));
            IFC_RETURN(Number(kind, id, L"ScaleZ", scale.Z, status));
            float rotation{};
            status = visual->get_RotationAngleInDegrees(&rotation);
            return Number(kind, id, L"RotationAngleInDegrees", rotation, status);
        }

        HRESULT CaptureVisuals()
        {
            struct Pending
            {
                xref_ptr<WUComp::IVisual> Visual;
                UINT64 Parent;
                UINT32 Index;
                UINT64 Owner;
            };
            std::vector<Pending> pending;
            std::unordered_set<WUComp::IVisual*> roots;
            for (auto visual : m_visualSeeds)
            {
                std::unordered_set<WUComp::IVisual*> ancestors;
                while (true)
                {
                    if (!ancestors.insert(visual.get()).second || ancestors.size() > m_maximumNodes)
                    {
                        return HRESULT_FROM_WIN32(ERROR_CIRCULAR_DEPENDENCY);
                    }
                    xref_ptr<WUComp::IContainerVisual> parent;
                    IFC_RETURN(visual->get_Parent(parent.ReleaseAndGetAddressOf()));
                    if (!parent)
                    {
                        break;
                    }
                    IFC_RETURN(parent->QueryInterface(IID_PPV_ARGS(visual.ReleaseAndGetAddressOf())));
                }
                if (roots.insert(visual.get()).second)
                {
                    pending.push_back({visual, 0, static_cast<UINT32>(roots.size() - 1), 0});
                }
            }

            std::unordered_set<WUComp::IVisual*> visited;
            while (!pending.empty())
            {
                auto entry = pending.back();
                pending.pop_back();
                auto visual = entry.Visual.get();
                if (!visited.insert(visual).second)
                {
                    continue;
                }
                auto owner = m_visualOwners.find(visual);
                if (owner != m_visualOwners.end())
                {
                    entry.Owner = owner->second;
                }
                OnlineXaml::Node node;
                node.Tree = OnlineXaml::TreeKind::Visual;
                node.Id = Address(visual);
                node.ParentId = entry.Parent;
                node.ChildIndex = entry.Index;
                node.CompId = entry.Owner;
                node.XamlId = entry.Owner ? m_compOwners.at(entry.Owner) : 0;
                node.VisualId = node.Id;
                wrl::Wrappers::HString type;
                IFC_RETURN(visual->GetRuntimeClassName(type.GetAddressOf()));
                node.Type = type.GetRawBuffer(nullptr);
                IFC_RETURN(AddNode(node));
                IFC_RETURN(VisualProperties(visual, node.Id));
                IFC_RETURN(Property(node.Tree, node.Id, L"CompLink",
                    owner != m_visualOwners.end() ? L"Direct" : (entry.Owner ? L"NearestAncestor" : L"None")));

                xref_ptr<WUComp::IContainerVisual> container;
                auto status = visual->QueryInterface(IID_PPV_ARGS(container.ReleaseAndGetAddressOf()));
                if (status == E_NOINTERFACE)
                {
                    continue;
                }
                IFC_RETURN(status);
                xref_ptr<WUComp::IVisualCollection> children;
                IFC_RETURN(container->get_Children(children.ReleaseAndGetAddressOf()));
                xref_ptr<wfc::IIterable<WUComp::Visual*>> iterable;
                IFC_RETURN(children->QueryInterface(IID_PPV_ARGS(iterable.ReleaseAndGetAddressOf())));
                xref_ptr<wfc::IIterator<WUComp::Visual*>> iterator;
                IFC_RETURN(iterable->First(iterator.ReleaseAndGetAddressOf()));
                boolean hasCurrent{};
                IFC_RETURN(iterator->get_HasCurrent(&hasCurrent));
                std::vector<xref_ptr<WUComp::IVisual>> orderedChildren;
                while (hasCurrent)
                {
                    if (orderedChildren.size() >= m_maximumNodes)
                    {
                        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                    }
                    xref_ptr<WUComp::IVisual> child;
                    IFC_RETURN(iterator->get_Current(child.ReleaseAndGetAddressOf()));
                    orderedChildren.push_back(std::move(child));
                    IFC_RETURN(iterator->MoveNext(&hasCurrent));
                }
                for (size_t index = orderedChildren.size(); index > 0; --index)
                {
                    pending.push_back({orderedChildren[index - 1], node.Id,
                        static_cast<UINT32>(index - 1), entry.Owner});
                }
            }
            return S_OK;
        }

        const OnlineXaml::Sink& m_sink;
        UINT32 m_maximumNodes;
        OnlineXaml::CaptureInfo& m_info;
        std::vector<HWCompTreeNode*> m_compSeeds;
        std::vector<xref_ptr<WUComp::IVisual>> m_visualSeeds;
        std::unordered_map<WUComp::IVisual*, UINT64> m_visualOwners;
        std::unordered_map<UINT64, UINT64> m_compOwners;
    };
}

extern "C" HRESULT WINAPI OnlineXamlProfilerCaptureSnapshot(
    UINT32 abiVersion, UINT32 maximumNodes, const OnlineXaml::Sink* sink, OnlineXaml::CaptureInfo* info) noexcept
{
    if (abiVersion != OnlineXaml::SnapshotAbiVersion)
    {
        return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    }
    if (!sink || sink->Size != sizeof(*sink) || !sink->AddNode || !sink->AddProperty ||
        !info || info->Size != sizeof(*info) || !maximumNodes || maximumNodes > OnlineXaml::MaximumNodeCount)
    {
        return E_INVALIDARG;
    }
    auto core = DirectUI::DXamlServices::GetSafeHandle();
    if (!core || DirectUI::DXamlServices::IsDXamlCoreShuttingDown())
    {
        return RPC_E_WRONG_THREAD;
    }

    *info = {};
    info->ThreadId = GetCurrentThreadId();
    LARGE_INTEGER time{};
    QueryPerformanceFrequency(&time);
    info->QpcFrequency = time.QuadPart;
    QueryPerformanceCounter(&time);
    info->StartQpc = time.QuadPart;
    SnapshotWalker walker(*sink, maximumNodes, *info);
    auto status = walker.Capture(*core);
    QueryPerformanceCounter(&time);
    info->EndQpc = time.QuadPart;
    return status;
}
