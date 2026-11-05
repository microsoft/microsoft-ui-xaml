// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlProfilerTracing.h"
#include <atomic>
#include <microsoft.ui.composition.h>

// Keep the atomic's definition in one translation unit; PCHs can give std::atomic
// different alignment annotations that conflict during link-time code generation.
static std::atomic<bool> g_profilerEnabled{false};

namespace
{
    PCWSTR VisualTypeName(_In_ IUnknown* visual)
    {
        wrl::ComPtr<WUComp::ISpriteVisual> sprite;
        if (SUCCEEDED(visual->QueryInterface(IID_PPV_ARGS(&sprite))))
        {
            return L"SpriteVisual";
        }
        wrl::ComPtr<WUComp::ILayerVisual> layer;
        if (SUCCEEDED(visual->QueryInterface(IID_PPV_ARGS(&layer))))
        {
            return L"LayerVisual";
        }
        wrl::ComPtr<WUComp::IShapeVisual> shape;
        if (SUCCEEDED(visual->QueryInterface(IID_PPV_ARGS(&shape))))
        {
            return L"ShapeVisual";
        }
        wrl::ComPtr<WUComp::IRedirectVisual> redirect;
        if (SUCCEEDED(visual->QueryInterface(IID_PPV_ARGS(&redirect))))
        {
            return L"RedirectVisual";
        }
        // Check the derived visual interfaces before their container base.
        wrl::ComPtr<WUComp::IContainerVisual> container;
        if (SUCCEEDED(visual->QueryInterface(IID_PPV_ARGS(&container))))
        {
            return L"ContainerVisual";
        }
        return L"Visual";
    }

    // Normalize interface pointers to the IVisual identity used by visual-tree snapshots.
    uint64_t VisualId(_In_opt_ IUnknown* visual)
    {
        if (visual == nullptr)
        {
            return 0;
        }

        wrl::ComPtr<WUComp::IVisual> normalizedVisual;
        if (FAILED(visual->QueryInterface(IID_PPV_ARGS(&normalizedVisual))))
        {
            return 0;
        }
        return reinterpret_cast<uint64_t>(normalizedVisual.Get());
    }
}

void XamlProfilerTracing::EnableAllocationProfiling(bool enabled) noexcept
{
    g_profilerEnabled.store(enabled, std::memory_order_relaxed);
}

bool XamlProfilerTracing::IsAllocationProfilingEnabled() noexcept
{
    return g_profilerEnabled.load(std::memory_order_relaxed);
}

void XamlProfilerTracing::NotifyChildInserted(
    _In_opt_ IUnknown* parentVisual,
    _In_opt_ IUnknown* childVisual)
{
    if (!IsEnabled())
    {
        return;
    }

    const uint64_t parentVisualId = VisualId(parentVisual);
    const uint64_t childVisualId = VisualId(childVisual);
    if (parentVisualId == 0 || childVisualId == 0)
    {
        return;
    }

    WucVisualChildInserted(parentVisualId, childVisualId, VisualTypeName(childVisual));
}

void XamlProfilerTracing::NotifyChildRemoved(
    _In_opt_ IUnknown* parentVisual,
    _In_opt_ IUnknown* childVisual)
{
    if (!IsEnabled())
    {
        return;
    }

    const uint64_t parentVisualId = VisualId(parentVisual);
    const uint64_t childVisualId = VisualId(childVisual);
    if (parentVisualId == 0 || childVisualId == 0)
    {
        return;
    }

    WucVisualChildRemoved(parentVisualId, childVisualId);
}

void XamlProfilerTracing::NotifyChildrenCleared(_In_opt_ IUnknown* parentVisual)
{
    if (!IsEnabled())
    {
        return;
    }

    const uint64_t parentVisualId = VisualId(parentVisual);
    if (parentVisualId == 0)
    {
        return;
    }

    WucVisualChildrenCleared(parentVisualId);
}

void XamlProfilerTracing::NotifyRootSet(
    _In_opt_ IUnknown* visual,
    uint64_t targetId)
{
    if (!IsEnabled())
    {
        return;
    }

    const uint64_t visualId = VisualId(visual);
    if (visualId == 0)
    {
        return;
    }

    WucVisualRootSet(visualId, targetId, VisualTypeName(visual));
}

void XamlProfilerTracing::NotifyRootCleared(uint64_t targetId)
{
    if (IsEnabled())
    {
        WucVisualRootCleared(targetId);
    }
}
