// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include "WinUIProfilerProtocol.h"

namespace WinUIProfiler
{
    bool EnterInstrumentation() noexcept;
    void LeaveInstrumentation() noexcept;

    class InstrumentationScope final
    {
    public:
        InstrumentationScope() noexcept : m_owned(EnterInstrumentation()) {}
        ~InstrumentationScope() noexcept { if (m_owned) LeaveInstrumentation(); }
        InstrumentationScope(const InstrumentationScope&) = delete;
        InstrumentationScope& operator=(const InstrumentationScope&) = delete;
        bool Detach() noexcept
        {
            const bool owned = m_owned;
            m_owned = false;
            return owned;
        }
    private:
        bool m_owned;
    };

    HRESULT GetProtocolInfo(uint32_t version, uint32_t size, ProtocolInfo* info) noexcept;
    HRESULT GetSequenceCut(uint32_t version, uint16_t eventId, uint32_t size, SequenceCut* cut) noexcept;
    HRESULT RequestCapture(uint32_t version, uint32_t kind, GUID captureId) noexcept;

    // A bounded, allocation-free reservation window. Completion is not delivery.
    class SequenceDomain final
    {
    public:
        static constexpr uint32_t Window = 256;
        HRESULT Reserve(uint64_t* sequence) noexcept;
        HRESULT Settle(uint64_t sequence) noexcept;
        void Snapshot(uint64_t* reserved, uint64_t* settled) noexcept;
    private:
        SRWLOCK m_lock = SRWLOCK_INIT;
        uint64_t m_reserved = 0;
        uint64_t m_settled = 0;
        bool m_completed[Window]{};
    };
}
