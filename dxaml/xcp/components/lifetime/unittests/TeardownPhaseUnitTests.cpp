// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TeardownPhaseUnitTests.h"

#include <TeardownPhase.h>

using namespace WEX::TestExecution;
using namespace DirectUI;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Lifetime {

    void TeardownPhaseUnitTests::ForwardAdvancementIsLegal()
    {
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::Live, TeardownPhase::DetachingManaged));
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::DetachingManaged, TeardownPhase::TearingDownNative));
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::TearingDownNative, TeardownPhase::Dead));
    }

    void TeardownPhaseUnitTests::ForwardSkipsAreLegal()
    {
        // A scoped native teardown can jump straight from Live to TearingDownNative without a managed-detach step.
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::Live, TeardownPhase::TearingDownNative));
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::Live, TeardownPhase::Dead));
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::DetachingManaged, TeardownPhase::Dead));
    }

    void TeardownPhaseUnitTests::SelfTransitionsAreLegal()
    {
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::Live, TeardownPhase::Live));
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::DetachingManaged, TeardownPhase::DetachingManaged));
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::TearingDownNative, TeardownPhase::TearingDownNative));
        // Dead -> Dead is idempotent and therefore legal even though Dead is terminal.
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::Dead, TeardownPhase::Dead));
    }

    void TeardownPhaseUnitTests::ResetToLiveIsLegal()
    {
        // A visual tree can be reset and rebuilt while the core lives, so any non-terminal phase may return to Live.
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::DetachingManaged, TeardownPhase::Live));
        VERIFY_IS_TRUE(IsLegalTeardownPhaseTransition(TeardownPhase::TearingDownNative, TeardownPhase::Live));
    }

    void TeardownPhaseUnitTests::BackwardTransitionsAreIllegal()
    {
        VERIFY_IS_FALSE(IsLegalTeardownPhaseTransition(TeardownPhase::TearingDownNative, TeardownPhase::DetachingManaged));
        VERIFY_IS_FALSE(IsLegalTeardownPhaseTransition(TeardownPhase::Dead, TeardownPhase::TearingDownNative));
    }

    void TeardownPhaseUnitTests::DeadIsTerminal()
    {
        // Dead has no legal outgoing edge except the idempotent self-edge.
        VERIFY_IS_FALSE(IsLegalTeardownPhaseTransition(TeardownPhase::Dead, TeardownPhase::Live));
        VERIFY_IS_FALSE(IsLegalTeardownPhaseTransition(TeardownPhase::Dead, TeardownPhase::DetachingManaged));
        VERIFY_IS_FALSE(IsLegalTeardownPhaseTransition(TeardownPhase::Dead, TeardownPhase::TearingDownNative));
    }

    void TeardownPhaseUnitTests::TearingDownOrDeadPredicate()
    {
        // The reentrancy gate fires only once native teardown has begun.
        VERIFY_IS_FALSE(IsTearingDownOrDead(TeardownPhase::Live));
        VERIFY_IS_FALSE(IsTearingDownOrDead(TeardownPhase::DetachingManaged));
        VERIFY_IS_TRUE(IsTearingDownOrDead(TeardownPhase::TearingDownNative));
        VERIFY_IS_TRUE(IsTearingDownOrDead(TeardownPhase::Dead));
    }

} } } } }
