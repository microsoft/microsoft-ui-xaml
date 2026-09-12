// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Lifetime {
    // Pillar D - unit tests for the teardown-epoch ordering contract (IsLegalTeardownPhaseTransition) and the
    // TearingDown/Dead predicate used by the reentrancy gate. These exercise the pure logic in TeardownPhase.h;
    // the runtime choke point (CCoreServices::SetTeardownPhase) asserts on this same contract.
    class TeardownPhaseUnitTests
    {
    public:
        BEGIN_TEST_CLASS(TeardownPhaseUnitTests)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_METHOD(ForwardAdvancementIsLegal)
        TEST_METHOD(ForwardSkipsAreLegal)
        TEST_METHOD(SelfTransitionsAreLegal)
        TEST_METHOD(ResetToLiveIsLegal)
        TEST_METHOD(BackwardTransitionsAreIllegal)
        TEST_METHOD(DeadIsTerminal)
        TEST_METHOD(TearingDownOrDeadPredicate)
    };

} } } } }
