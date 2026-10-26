// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "OnlineXamlTreeAccess.h"
#include "DXamlServices.h"
#include "hwcompnode.h"
#include "ContentRootCoordinator.h"
#include "XcpAllocation.h"
#ifdef XAMLPROFILER_ENABLED
#include "OnlineXamlObservationCache.h"
#include "XamlProfilerTracing.h"
#endif

namespace
{
#ifdef XAMLPROFILER_ENABLED
    OnlineXaml::SurfaceObservationCache<>& ObservationCache() noexcept
    {
        static OnlineXaml::SurfaceObservationCache<> cache;
        return cache;
    }
#endif

    HRESULT WINAPI ReadMemory(void* context, OnlineXaml::NativeMemory* memory)
    {
        if (!memory || memory->Size != sizeof(*memory)) { return E_INVALIDARG; }
        if (DirectUI::DXamlServices::GetSafeHandle() != context ||
            DirectUI::DXamlServices::IsDXamlCoreShuttingDown()) { return RPC_E_WRONG_THREAD; }
        *memory = {};
        memory->HeapId = XcpAllocation::GetHeapHandle();
        memory->IsPrivateHeap = XcpAllocation::IsUsingPrivateHeap();
        if (XcpAllocation::IsOutstandingAllocationTrackingAvailable())
        {
            memory->HeapStatus = S_OK;
            memory->OutstandingBytes = XcpAllocation::GetOutstandingAllocationSize();
            memory->OutstandingCount = XcpAllocation::GetOutstandingAllocationCount();
        }
#ifdef XAMLPROFILER_ENABLED
        ObservationCache().ReadMemory(*memory);
#endif
        return S_OK;
    }

    HRESULT WINAPI ReadSurface(void* context, UINT64 id, OnlineXaml::NativeSurface* surface)
    {
        if (!id || !surface || surface->Size != sizeof(*surface)) { return E_INVALIDARG; }
        if (DirectUI::DXamlServices::GetSafeHandle() != context ||
            DirectUI::DXamlServices::IsDXamlCoreShuttingDown()) { return RPC_E_WRONG_THREAD; }
        *surface = {};
#ifdef XAMLPROFILER_ENABLED
        return ObservationCache().Read(id, *surface);
#else
        return E_NOTIMPL;
#endif
    }

    template<typename T>
    UINT64 Address(T* value)
    {
        return static_cast<UINT64>(reinterpret_cast<uintptr_t>(value));
    }

    HRESULT WINAPI ReadNode(
        void* context, OnlineXaml::TreeKind kind, UINT64 id, const OnlineXaml::NodeReader* reader)
    {
        if (!context || !id || !reader || reader->Size != sizeof(*reader) || !reader->Node)
        {
            return E_INVALIDARG;
        }
        if (DirectUI::DXamlServices::GetSafeHandle() != context ||
            DirectUI::DXamlServices::IsDXamlCoreShuttingDown())
        {
            return RPC_E_WRONG_THREAD;
        }

        OnlineXaml::NativeNode node;
        node.Tree = kind;
        node.Id = id;
        if (kind == OnlineXaml::TreeKind::Xaml)
        {
            auto element = reinterpret_cast<CUIElement*>(static_cast<uintptr_t>(id));
            auto type = element->GetClassInformation()->GetFullName();
            node.Type = type.GetBuffer();
            node.Name = element->m_strName.GetBuffer();
            node.XamlId = id;
            node.CompId = Address(element->GetCompositionPeer());
            node.ActualWidth = element->GetActualWidth();
            node.ActualHeight = element->GetActualHeight();
            node.IsVisible = element->IsVisible();
#ifdef XAMLPROFILER_ENABLED
            node.CoreAllocationBytes = XamlProfilerGetCoreSize(element);
            node.CoreAllocationStatus = node.CoreAllocationBytes ? S_OK : E_NOTIMPL;
#endif
            IFC_RETURN(reader->Node(reader->Context, &node));
            if (reader->Child)
            {
                if (auto children = element->GetChildren())
                {
                    const auto& collection = children->GetCollection();
                    if (collection.size() > UINT32_MAX)
                    {
                        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                    }
                    for (size_t index = 0; index < collection.size(); ++index)
                    {
                        if (auto child = do_pointer_cast<CUIElement>(collection[index]))
                        {
                            IFC_RETURN(reader->Child(reader->Context, Address(child), static_cast<UINT32>(index)));
                        }
                    }
                }
            }
        }
        else if (kind == OnlineXaml::TreeKind::Composition)
        {
            auto comp = reinterpret_cast<HWCompNode*>(static_cast<uintptr_t>(id));
            auto tree = do_pointer_cast<HWCompTreeNode>(comp);
            auto type = comp->GetClassInformation()->GetFullName();
            node.Type = type.GetBuffer();
            node.CompId = id;
            node.ParentId = Address(do_pointer_cast<HWCompNode>(comp->GetParentInternal(false /* publicOnly */)));
            xref_ptr<WUComp::IVisual> visual;
            xref_ptr<WUComp::IVisual> mainTreeVisual;
            if (tree)
            {
                node.XamlId = Address(tree->GetUIElementPeer());
                visual = tree->GetWUCVisual();
                mainTreeVisual = tree->GetWUCVisualInMainTree();
                node.Visual = visual.get();
                node.MainTreeVisual = mainTreeVisual.get();
            }
            IFC_RETURN(reader->Node(reader->Context, &node));
            if (tree && reader->Child)
            {
                UINT32 index = 0;
                for (auto child = tree->GetChildrenBegin(); child != tree->GetChildrenEnd(); ++child)
                {
                    if (index == UINT32_MAX)
                    {
                        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                    }
                    IFC_RETURN(reader->Child(reader->Context, Address(*child), index++));
                }
            }
        }
        else
        {
            return E_INVALIDARG;
        }
        return S_OK;
    }
}

extern "C" HRESULT WINAPI OnlineXamlProfilerWithTreeAccess(
    UINT32 abiVersion, void* context, OnlineXaml::AccessCallback callback) noexcept
{
    if (abiVersion != OnlineXaml::TreeAccessAbiVersion)
    {
        return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    }
    if (!callback)
    {
        return E_INVALIDARG;
    }
    auto core = DirectUI::DXamlServices::GetSafeHandle();
    if (!core || DirectUI::DXamlServices::IsDXamlCoreShuttingDown())
    {
        return RPC_E_WRONG_THREAD;
    }

    std::vector<CUIElement*> roots;
    for (const auto& contentRoot : core->GetContentRootCoordinator()->GetContentRoots())
    {
        if (!contentRoot->IsShuttingDown())
        {
            contentRoot->GetVisualTreeNoRef()->GetExistingRootsNoRef(roots);
        }
    }
    if (roots.size() > UINT32_MAX)
    {
        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }
    std::vector<UINT64> ids;
    ids.reserve(roots.size());
    for (auto root : roots) { ids.push_back(Address(root)); }
    OnlineXaml::TreeAccess access;
    access.ThreadId = GetCurrentThreadId();
    access.RootCount = static_cast<UINT32>(ids.size());
    access.Roots = ids.data();
    access.Context = core;
    access.ReadNode = ReadNode;
    access.ReadMemory = ReadMemory;
    access.ReadSurface = ReadSurface;
    return callback(context, &access);
}

#ifdef XAMLPROFILER_ENABLED
void OnlineXaml::RegisterSurfaceObservation(const NativeSurface& surface) noexcept
{
    ObservationCache().Register(surface);
}

void OnlineXaml::RemoveSurfaceObservation(UINT64 id, UINT64 owner) noexcept
{
    ObservationCache().Remove(id, owner);
}

void OnlineXaml::ObserveSurfaceTexture(UINT64 id, UINT64 owner, const NativeTexture& texture) noexcept
{
    ObservationCache().Observe(id, owner, texture);
}
#endif
