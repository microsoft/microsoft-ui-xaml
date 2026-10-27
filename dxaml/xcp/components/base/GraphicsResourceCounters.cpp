// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <GraphicsResourceCounters.h>
#include <atomic>
#include <iterator>

namespace GraphicsResourceCounters
{
    namespace
    {
        constexpr const wchar_t* ResourceNames[] =
        {
            L"Composition.ContainerVisual",
            L"Composition.SpriteVisual",
            L"Composition.LayerVisual",
            L"Composition.ShapeVisual",
            L"Composition.VisualSurface",
            L"Composition.ColorBrush",
            L"Composition.SurfaceBrush",
            L"Composition.MaskBrush",
            L"Composition.NineGridBrush",
            L"Composition.LinearGradientBrush",
            L"Composition.EffectBrush",
            L"Composition.Surface",
            L"Composition.VirtualSurface",
            L"D2D.SolidColorBrush",
            L"D2D.BitmapBrush",
            L"D2D.LinearGradientBrush",
            L"D2D.RadialGradientBrush",
            L"WinUI.DCompSurfaceWrapper",
            L"WinUI.D2DSolidColorBrushWrapper",
            L"WinUI.D2DBitmapBrushWrapper",
            L"WinUI.D2DLinearGradientBrushWrapper",
            L"WinUI.D2DRadialGradientBrushWrapper",
            L"WinUI.D2DStrokeStyleWrapper",
            L"WinUI.D2DGeometryWrapper",
            L"WinUI.D2DBitmapWrapper"
        };
        static_assert(std::size(ResourceNames) == ResourceKindCount);

        constexpr bool HasLifetime(uint32_t index) noexcept
        {
            return index >= static_cast<uint32_t>(ResourceKind::DCompSurfaceWrapper);
        }

#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
        struct Counters
        {
            std::atomic<uint64_t> created{0};
            std::atomic<uint64_t> destroyed{0};
            std::atomic<uint64_t> live{0};
        };

        Counters g_counters[ResourceKindCount];
#endif
    }

#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
    void RecordCreated(ResourceKind kind) noexcept
    {
        const auto index = static_cast<uint32_t>(kind);
        auto& counters = g_counters[index];
        counters.created.fetch_add(1, std::memory_order_relaxed);
        if (HasLifetime(index))
        {
            counters.live.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void RecordDestroyed(ResourceKind kind) noexcept
    {
        const auto index = static_cast<uint32_t>(kind);
        ASSERT(HasLifetime(index));
        auto& counters = g_counters[index];
        counters.destroyed.fetch_add(1, std::memory_order_relaxed);
        ASSERT(counters.live.load(std::memory_order_relaxed) != 0);
        counters.live.fetch_sub(1, std::memory_order_relaxed);
    }
#endif

    bool TryGetSnapshot(uint32_t index, Snapshot& snapshot) noexcept
    {
        snapshot = {};
        if (index >= ResourceKindCount)
        {
            return false;
        }

        snapshot.name = ResourceNames[index];
        snapshot.hasLifetime = HasLifetime(index);
#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
        snapshot.enabled = true;
        const auto& counters = g_counters[index];
        snapshot.created = counters.created.load(std::memory_order_relaxed);
        snapshot.destroyed = counters.destroyed.load(std::memory_order_relaxed);
        snapshot.live = counters.live.load(std::memory_order_relaxed);
#endif
        return true;
    }
}
