// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <ImmediatePainter.h>

// GDI+-specific seams for the immediate-rendering prototype (see GdiPlusPainter.cpp). The shared painter contract is
// IImmediatePainter in ImmediatePainter.h; this header only adds what the GDI+ backend needs beyond it.
namespace RenderingBackend::GdiPlus
{
    // Process-wide GDI+ runtime shared by every GDI+ painter: GdiplusStartup on the first held reference,
    // GdiplusShutdown when the last one is released. A painter holds one reference for its lifetime.
    class RuntimeReference
    {
    public:
        RuntimeReference() noexcept = default;
        ~RuntimeReference() noexcept { Release(); }
        RuntimeReference(const RuntimeReference&) = delete;
        RuntimeReference& operator=(const RuntimeReference&) = delete;

        // Idempotent: a reference that is already held is not counted twice.
        HRESULT Acquire() noexcept;
        void Release() noexcept;
        bool IsHeld() const noexcept { return m_held; }

    private:
        bool m_held = false;
    };

    uint32_t GetRuntimeReferenceCount() noexcept;
    bool IsRuntimeStarted() noexcept;

    // Pixels covered by the damage rects (physical pixels), each rounded outward and clipped to the target. Rects are
    // summed, so callers pass coalesced, non-overlapping damage. nullptr / count == 0 means the whole target.
    uint64_t DamagedPixelCount(uint32_t width, uint32_t height, _In_reads_opt_(count) const RectF* damageRects, uint32_t count) noexcept;

    // Bytes copied from the PARGB bitmap to the presentable surface for one frame: DamagedPixelCount * 4. This is what
    // the GDI+ painter reports as ResourceInventory::uploadBytesThisFrame.
    uint64_t DamageCopyBytes(uint32_t width, uint32_t height, _In_reads_opt_(count) const RectF* damageRects, uint32_t count) noexcept;

    // GDI+ objects (brushes, pens, fonts, string formats, bitmaps, paths) and DirectWrite objects created and destroyed
    // between the last BeginFrame and EndFrame (test plan W6). The caller emits them with TraceResourceChurn.
    struct ResourceChurn
    {
        uint32_t created = 0;
        uint32_t destroyed = 0;
    };

    // E_INVALIDARG when painter is not a GDI+ painter.
    HRESULT GetLastFrameResourceChurn(const IImmediatePainter& painter, _Out_ ResourceChurn* churn) noexcept;
}
