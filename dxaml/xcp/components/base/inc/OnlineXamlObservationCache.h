// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "OnlineXamlTreeAccess.h"
#include <array>

namespace OnlineXaml
{
    // Fixed storage has no allocation-failure or static-destruction dependency.
    // Keys exist only while the DCompSurface owner is alive; no COM references.
    template<size_t Capacity = 8192>
    class SurfaceObservationCache
    {
        static_assert(Capacity > 0 && Capacity <= UINT32_MAX);

    public:
        void Register(const NativeSurface& surface) noexcept
        {
            if (!surface.CompositionSurfaceId || !surface.SurfaceId) { return; }
            AcquireSRWLockExclusive(&m_lock);
            auto slot = Find(surface.CompositionSurfaceId, true);
            if (slot == Capacity)
            {
                ++m_dropped;
            }
            else
            {
                auto& entry = m_entries[slot];
                if (entry.Occupancy != State::Live) { ++m_count; }
                entry.Occupancy = State::Live;
                entry.Value = surface;
                entry.Value.Generation = ++m_generation;
                entry.Value.Texture = {};
            }
            ReleaseSRWLockExclusive(&m_lock);
        }

        void Remove(UINT64 key, UINT64 owner) noexcept
        {
            AcquireSRWLockExclusive(&m_lock);
            const auto slot = Find(key, false);
            if (slot != Capacity && m_entries[slot].Value.SurfaceId == owner)
            {
                m_entries[slot] = {};
                m_entries[slot].Occupancy = State::Deleted;
                --m_count;
            }
            ReleaseSRWLockExclusive(&m_lock);
        }

        void Observe(UINT64 key, UINT64 owner, const NativeTexture& texture) noexcept
        {
            AcquireSRWLockExclusive(&m_lock);
            const auto slot = Find(key, false);
            if (slot != Capacity && m_entries[slot].Value.SurfaceId == owner)
            {
                m_entries[slot].Value.Texture = texture;
            }
            ReleaseSRWLockExclusive(&m_lock);
        }

        HRESULT Read(UINT64 key, NativeSurface& surface) noexcept
        {
            AcquireSRWLockShared(&m_lock);
            const auto slot = Find(key, false);
            if (slot != Capacity) { surface = m_entries[slot].Value; }
            ReleaseSRWLockShared(&m_lock);
            return slot == Capacity ? HRESULT_FROM_WIN32(ERROR_NOT_FOUND) : S_OK;
        }

        void ReadMemory(NativeMemory& memory) noexcept
        {
            AcquireSRWLockShared(&m_lock);
            memory.CacheStatus = m_dropped ? HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) : S_OK;
            memory.CacheCapacity = static_cast<UINT32>(Capacity);
            memory.CachedSurfaceCount = m_count;
            memory.DroppedRegistrations = m_dropped;
            memory.CacheBytes = sizeof(*this);
            ReleaseSRWLockShared(&m_lock);
        }

    private:
        enum class State : BYTE { Empty, Live, Deleted };
        struct Entry { NativeSurface Value; State Occupancy = State::Empty; };

        size_t Find(UINT64 key, bool inserting) const noexcept
        {
            auto deleted = Capacity;
            const auto hash = (key >> 4) * 11400714819323198485ull;
            for (size_t probe = 0; probe < Capacity; ++probe)
            {
                const auto slot = static_cast<size_t>((hash + probe) % Capacity);
                const auto& entry = m_entries[slot];
                if (entry.Occupancy == State::Live && entry.Value.CompositionSurfaceId == key) { return slot; }
                if (entry.Occupancy == State::Deleted && deleted == Capacity) { deleted = slot; }
                if (entry.Occupancy == State::Empty) { return inserting ? (deleted == Capacity ? slot : deleted) : Capacity; }
            }
            return inserting ? deleted : Capacity;
        }

        SRWLOCK m_lock = SRWLOCK_INIT;
        std::array<Entry, Capacity> m_entries{};
        UINT64 m_generation = 0;
        UINT64 m_dropped = 0;
        UINT32 m_count = 0;
    };
}
