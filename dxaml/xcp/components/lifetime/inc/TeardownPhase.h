// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

namespace DirectUI
{
    // Pillar D - explicit teardown epoch.
    //
    // Historically "how far along is this tree/core in its teardown?" was implicit and scattered across several
    // independent booleans on CCoreServices (m_bIsShuttingDown, m_isTearingDownIsland, m_bIsDestroyingCoreServices).
    // Reentrancy-during-teardown defects (failure mode P5) and shutdown-ordering defects (P6) are ultimately places
    // where those bits disagree, or where a callback runs against half-destroyed invariants. TeardownPhase makes the
    // teardown epoch a single, explicit, ordered token so that "are we tearing down?" has one authoritative answer.
    enum class TeardownPhase : std::uint8_t
    {
        Live,               // Normal operation; the tree is live and fully usable.
        DetachingManaged,   // Managed peers are being detached from their native objects.
        TearingDownNative,  // Native tree/core objects are being torn down.
        Dead                // Terminal; the core has been destroyed. All further access must no-op.
    };

    // Ordering contract for the teardown epoch. Legal edges:
    //  - self edges (idempotent re-announce of the current phase),
    //  - forward advancement Live -> DetachingManaged -> TearingDownNative -> Dead (forward skips are allowed; a
    //    scoped visual-tree reset can go straight to TearingDownNative without a distinct managed-detach step),
    //  - a reset back to Live from any non-terminal phase (a visual tree can be reset and then rebuilt while the
    //    core object itself lives on - e.g. re-navigation / island reconnect),
    //  - Dead is terminal: once the core is destroyed there is no legal outgoing edge.
    //
    // This is the assertion the single SetTeardownPhase choke point validates (in debug); it is intentionally
    // permissive about forward progress but strict about the two hard invariants: you never leave Dead, and you
    // never move "backwards" through a teardown except by an explicit full reset to Live.
    constexpr bool IsLegalTeardownPhaseTransition(TeardownPhase from, TeardownPhase to)
    {
        if (from == to)
        {
            return true; // idempotent re-announce
        }

        if (from == TeardownPhase::Dead)
        {
            return false; // terminal - no outgoing edges
        }

        if (to == TeardownPhase::Live)
        {
            return true; // full reset to begin a fresh tree lifecycle
        }

        // Otherwise only forward advancement toward Dead is legal.
        return to > from;
    }

    // True once the native tree/core is being torn down (or is already dead). This is the point past which a
    // reentrant callback can no longer safely touch tree/core invariants.
    constexpr bool IsTearingDownOrDead(TeardownPhase phase)
    {
        return phase >= TeardownPhase::TearingDownNative;
    }

#if DBG
    inline const wchar_t* TeardownPhaseToString(TeardownPhase phase)
    {
        switch (phase)
        {
            case TeardownPhase::Live:              return L"Live";
            case TeardownPhase::DetachingManaged:  return L"DetachingManaged";
            case TeardownPhase::TearingDownNative: return L"TearingDownNative";
            case TeardownPhase::Dead:              return L"Dead";
        }
        return L"<invalid>";
    }
#endif
}
