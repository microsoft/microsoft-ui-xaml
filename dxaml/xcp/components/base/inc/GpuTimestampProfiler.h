// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>

// GPU time per immediate paint (test plan W3) from D3D11 timestamp + timestamp-disjoint queries. Non-blocking: queries
// are issued around a paint and read back a frame or two later with D3D11_ASYNC_GETDATA_DONOTFLUSH; Present (or an
// explicit Flush) submits them. A small ring of query sets is reused; when the ring is full the oldest unread interval
// is dropped rather than stalling the render thread.
namespace RenderingBackend
{
    // Converts a timestamp interval to microseconds. False when the interval is unusable: the disjoint flag is set
    // (frequency changed, e.g. power state transition), the frequency is zero, or end precedes begin.
    bool GpuTicksToMicroseconds(uint64_t beginTicks, uint64_t endTicks, uint64_t frequency, bool disjoint,
        _Out_ uint64_t& microseconds) noexcept;

    class GpuTimestampProfiler
    {
    public:
        static constexpr uint32_t SlotCount = 4;

        struct Result
        {
            uint32_t frameNumber = 0;
            uint64_t gpuMicroseconds = 0;
        };

        struct Counters
        {
            uint64_t issued = 0;    // intervals closed with EndPaint
            uint64_t completed = 0; // valid intervals read back (each emits GpuPaintTime)
            uint64_t disjoint = 0;  // intervals read back but unusable (see GpuTicksToMicroseconds)
            uint64_t dropped = 0;   // intervals overwritten before read back, or opened without a matching EndPaint
        };

        HRESULT Initialize(_In_ ID3D11Device* device) noexcept;
        bool IsInitialized() const noexcept { return m_initialized; }

        void BeginPaint(_In_ ID3D11DeviceContext* context, uint32_t frameNumber) noexcept;
        void EndPaint(_In_ ID3D11DeviceContext* context) noexcept;

        // Never flushes or waits. Reads intervals back in issue order, stopping at the first one the GPU has not finished.
        // Emits RenderingBackend::TraceGpuPaintTime for each valid interval. When results is non-null, also copies valid
        // intervals into it and stops once capacity are copied (the rest stay pending). Returns the number of valid intervals.
        uint32_t Collect(_In_ ID3D11DeviceContext* context, _Out_writes_opt_(capacity) Result* results, uint32_t capacity) noexcept;

        uint32_t PendingCount() const noexcept { return m_pendingCount; }
        const Counters& GetCounters() const noexcept { return m_counters; }

    private:
        enum class SlotState : uint8_t { Free, Open, Pending };

        struct Slot
        {
            Microsoft::WRL::ComPtr<ID3D11Query> disjoint;
            Microsoft::WRL::ComPtr<ID3D11Query> begin;
            Microsoft::WRL::ComPtr<ID3D11Query> end;
            uint64_t sequence = 0;
            uint32_t frameNumber = 0;
            SlotState state = SlotState::Free;
        };

        Slot m_slots[SlotCount];
        uint64_t m_sequence = 0;     // issue order; Collect reads Pending slots oldest first
        uint32_t m_next = 0;         // slot the next BeginPaint uses
        uint32_t m_pendingCount = 0;
        int32_t m_open = -1;         // slot between BeginPaint and EndPaint
        bool m_initialized = false;
        Counters m_counters;
    };
}
