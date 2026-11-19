// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <GpuTimestampProfiler.h>
#include <RenderingBackend.h>

namespace RenderingBackend
{
    bool GpuTicksToMicroseconds(uint64_t beginTicks, uint64_t endTicks, uint64_t frequency, bool disjoint,
        _Out_ uint64_t& microseconds) noexcept
    {
        microseconds = 0;
        if (disjoint || frequency == 0 || endTicks < beginTicks)
        {
            return false;
        }
        const uint64_t ticks = endTicks - beginTicks;
        // Split to avoid overflowing ticks * 1e6 for long intervals on high-frequency counters.
        microseconds = (ticks / frequency) * 1000000ull + ((ticks % frequency) * 1000000ull) / frequency;
        return true;
    }

    HRESULT GpuTimestampProfiler::Initialize(_In_ ID3D11Device* device) noexcept
    {
        m_initialized = false;
        m_next = 0;
        m_pendingCount = 0;
        m_open = -1;
        m_counters = {};

        const D3D11_QUERY_DESC disjointDesc{ D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
        const D3D11_QUERY_DESC timestampDesc{ D3D11_QUERY_TIMESTAMP, 0 };
        for (auto& slot : m_slots)
        {
            slot = Slot{};
            HRESULT hr = device->CreateQuery(&disjointDesc, &slot.disjoint);
            if (SUCCEEDED(hr)) hr = device->CreateQuery(&timestampDesc, &slot.begin);
            if (SUCCEEDED(hr)) hr = device->CreateQuery(&timestampDesc, &slot.end);
            if (FAILED(hr))
            {
                for (auto& reset : m_slots) reset = Slot{};
                return hr;
            }
        }
        m_initialized = true;
        return S_OK;
    }

    void GpuTimestampProfiler::BeginPaint(_In_ ID3D11DeviceContext* context, uint32_t frameNumber) noexcept
    {
        if (!m_initialized)
        {
            return;
        }
        if (m_open >= 0)
        {
            // BeginPaint without EndPaint: close the bracket and discard it.
            Slot& abandoned = m_slots[m_open];
            context->End(abandoned.disjoint.Get());
            abandoned.state = SlotState::Free;
            m_open = -1;
            ++m_counters.dropped;
        }

        Slot& slot = m_slots[m_next];
        if (slot.state == SlotState::Pending)
        {
            // Ring full: the GPU (or readback) is more than SlotCount paints behind. Drop the oldest, never wait.
            --m_pendingCount;
            ++m_counters.dropped;
        }
        context->Begin(slot.disjoint.Get());
        context->End(slot.begin.Get());
        slot.frameNumber = frameNumber;
        slot.sequence = ++m_sequence;
        slot.state = SlotState::Open;
        m_open = static_cast<int32_t>(m_next);
        m_next = (m_next + 1) % SlotCount;
    }

    void GpuTimestampProfiler::EndPaint(_In_ ID3D11DeviceContext* context) noexcept
    {
        if (!m_initialized || m_open < 0)
        {
            return;
        }
        Slot& slot = m_slots[m_open];
        context->End(slot.end.Get());
        context->End(slot.disjoint.Get());
        slot.state = SlotState::Pending;
        m_open = -1;
        ++m_pendingCount;
        ++m_counters.issued;
    }

    uint32_t GpuTimestampProfiler::Collect(_In_ ID3D11DeviceContext* context, _Out_writes_opt_(capacity) Result* results,
        uint32_t capacity) noexcept
    {
        uint32_t valid = 0;
        while (m_initialized && m_pendingCount > 0 && (results == nullptr || valid < capacity))
        {
            Slot* oldest = nullptr;
            for (auto& slot : m_slots)
            {
                if (slot.state == SlotState::Pending && (oldest == nullptr || slot.sequence < oldest->sequence))
                {
                    oldest = &slot;
                }
            }

            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
            HRESULT hr = context->GetData(oldest->disjoint.Get(), &disjoint, sizeof(disjoint), D3D11_ASYNC_GETDATA_DONOTFLUSH);
            if (hr == S_FALSE)
            {
                break;
            }
            uint64_t begin = 0;
            uint64_t end = 0;
            if (SUCCEEDED(hr))
            {
                hr = context->GetData(oldest->begin.Get(), &begin, sizeof(begin), D3D11_ASYNC_GETDATA_DONOTFLUSH);
                if (hr == S_OK) hr = context->GetData(oldest->end.Get(), &end, sizeof(end), D3D11_ASYNC_GETDATA_DONOTFLUSH);
                if (hr == S_FALSE)
                {
                    break;
                }
            }

            uint64_t microseconds = 0;
            if (SUCCEEDED(hr) && GpuTicksToMicroseconds(begin, end, disjoint.Frequency, disjoint.Disjoint != FALSE, microseconds))
            {
                TraceGpuPaintTime(oldest->frameNumber, microseconds);
                if (results != nullptr)
                {
                    results[valid] = Result{ oldest->frameNumber, microseconds };
                }
                ++valid;
                ++m_counters.completed;
            }
            else
            {
                ++m_counters.disjoint;
            }
            oldest->state = SlotState::Free;
            --m_pendingCount;
        }
        return valid;
    }
}
