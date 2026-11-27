// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlProfilerTracing.h"
#include <atomic>
#include <memory>
#include <microsoft.ui.composition.h>
#include <unordered_map>

// Keep the atomic's definition in one translation unit; PCHs can give std::atomic
// different alignment annotations that conflict during link-time code generation.
static std::atomic<bool> g_profilerEnabled{false};

namespace
{
    struct StoryboardState
    {
        uint64_t id;
        uint64_t run = 0;
        uint64_t lastObservation = 0;
        uint32_t lastState = UINT32_MAX;
    };

    using StoryboardStates = std::unordered_map<const void*, StoryboardState>;
    wil::srwlock g_storyboardLock;
    std::atomic<bool> g_hasStoryboardStates{false};
    uint64_t g_nextStoryboardId = 0;
    // No static destructor: storyboard teardown can occur during module shutdown.
    // The last tracked storyboard releases the table.
    StoryboardStates* g_storyboardStates = nullptr;

    // The caller holds g_storyboardLock exclusively and has checked enablement.
    StoryboardState& GetOrCreateStoryboardState(const void* storyboard)
    {
        if (g_storyboardStates == nullptr)
        {
            auto states = std::make_unique<StoryboardStates>();
            states->emplace(storyboard, StoryboardState{++g_nextStoryboardId});
            g_storyboardStates = states.release();
        }
        auto [entry, inserted] = g_storyboardStates->try_emplace(storyboard, StoryboardState{});
        if (inserted)
        {
            entry->second.id = ++g_nextStoryboardId;
        }
        g_hasStoryboardStates.store(true, std::memory_order_release);
        return entry->second;
    }

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

void XamlProfilerTracing::SetProfilerEnabled(bool enabled) noexcept
{
    g_profilerEnabled.store(enabled, std::memory_order_relaxed);
}

bool XamlProfilerTracing::IsProfilerEnabled() noexcept
{
    return g_profilerEnabled.load(std::memory_order_relaxed);
}

void XamlProfilerTracing::BeginStoryboard(_In_ const void* storyboard)
{
    if (!IsProfilerEnabled() && !g_hasStoryboardStates.load(std::memory_order_acquire))
    {
        return;
    }

    auto lock = g_storyboardLock.lock_exclusive();
    StoryboardState* state = nullptr;
    if (IsProfilerEnabled())
    {
        state = &GetOrCreateStoryboardState(storyboard);
    }
    else if (g_storyboardStates != nullptr)
    {
        const auto entry = g_storyboardStates->find(storyboard);
        if (entry != g_storyboardStates->end())
        {
            state = &entry->second;
        }
    }
    if (state != nullptr)
    {
        // Keep tracked runs distinct across provider disable/re-enable without enrolling new objects.
        ++state->run;
        state->lastState = UINT32_MAX;
    }
}

bool XamlProfilerTracing::TryGetStoryboardState(
    _In_ const void* storyboard, uint32_t stateKey, bool observation, bool destroyed,
    _Out_ uint64_t& storyboardId, _Out_ uint64_t& runId)
{
    storyboardId = runId = 0;
    if (!IsProfilerEnabled())
    {
        return false;
    }

    auto lock = g_storyboardLock.lock_exclusive();
    if (!IsProfilerEnabled())
    {
        return false;
    }

    StoryboardState* state = nullptr;
    if (destroyed)
    {
        if (g_storyboardStates != nullptr)
        {
            const auto entry = g_storyboardStates->find(storyboard);
            if (entry != g_storyboardStates->end())
            {
                state = &entry->second;
            }
        }
        if (state == nullptr)
        {
            return false;
        }
    }
    else
    {
        state = &GetOrCreateStoryboardState(storyboard);
    }

    const auto now = GetTickCount64();
    if (observation && stateKey == state->lastState && now - state->lastObservation < 1000)
    {
        return false;
    }
    state->lastState = stateKey;
    state->lastObservation = now;
    storyboardId = state->id;
    runId = state->run;
    return true;
}

void XamlProfilerTracing::ForgetStoryboard(_In_ const void* storyboard)
{
    if (!g_hasStoryboardStates.load(std::memory_order_acquire))
    {
        return;
    }

    auto lock = g_storyboardLock.lock_exclusive();
    if (g_storyboardStates != nullptr)
    {
        g_storyboardStates->erase(storyboard);
        if (g_storyboardStates->empty())
        {
            delete g_storyboardStates;
            g_storyboardStates = nullptr;
            g_hasStoryboardStates.store(false, std::memory_order_release);
        }
    }
}

void XamlProfilerTracing::NotifyChildInserted(
    _In_opt_ IUnknown* parentVisual,
    _In_opt_ IUnknown* childVisual)
{
    if (!IsProfilerEnabled())
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
    if (!IsProfilerEnabled())
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
    if (!IsProfilerEnabled())
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
    if (!IsProfilerEnabled())
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
    if (IsProfilerEnabled())
    {
        WucVisualRootCleared(targetId);
    }
}
