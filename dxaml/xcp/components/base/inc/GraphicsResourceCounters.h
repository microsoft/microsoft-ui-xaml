// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

namespace GraphicsResourceCounters
{
    enum class ResourceKind : uint32_t
    {
        ContainerVisual,
        SpriteVisual,
        LayerVisual,
        ShapeVisual,
        VisualSurface,
        ColorBrush,
        SurfaceBrush,
        MaskBrush,
        NineGridBrush,
        LinearGradientBrush,
        EffectBrush,
        CompositionSurface,
        VirtualCompositionSurface,
        D2DSolidColorBrush,
        D2DBitmapBrush,
        D2DLinearGradientBrush,
        D2DRadialGradientBrush,
        DCompSurfaceWrapper,
        D2DSolidColorBrushWrapper,
        D2DBitmapBrushWrapper,
        D2DLinearGradientBrushWrapper,
        D2DRadialGradientBrushWrapper,
        D2DStrokeStyleWrapper,
        D2DGeometryWrapper,
        D2DBitmapWrapper,
        Count
    };

    constexpr uint32_t ResourceKindCount = static_cast<uint32_t>(ResourceKind::Count);

    struct Snapshot
    {
        const wchar_t* name;
        bool enabled;
        bool hasLifetime;
        uint64_t created;
        uint64_t destroyed;
        uint64_t live;
    };

    // Independent atomic observations, not a transactionally consistent state cut.
    bool TryGetSnapshot(uint32_t index, Snapshot& snapshot) noexcept;

#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
    void RecordCreated(ResourceKind kind) noexcept;
    void RecordDestroyed(ResourceKind kind) noexcept;
#else
    inline void RecordCreated(ResourceKind) noexcept
    {
    }

    inline void RecordDestroyed(ResourceKind) noexcept
    {
    }
#endif

    template<typename TResult>
    inline TResult RecordCreation(ResourceKind kind, TResult result) noexcept
    {
#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
        if (result >= 0)
        {
            RecordCreated(kind);
        }
#else
        (void)kind;
#endif
        return result;
    }
}
