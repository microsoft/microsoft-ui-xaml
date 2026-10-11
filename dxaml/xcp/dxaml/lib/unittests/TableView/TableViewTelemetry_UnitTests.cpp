// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <TableViewTelemetry.h>
#include <WexTestClass.h>
#include <limits>

TRACELOGGING_DEFINE_PROVIDER(g_hTelemetryProvider, "Microsoft.UI.Xaml.Controls.Tabular",
    (0x80a0b321, 0x304c, 0x576e, 0xc0, 0xf8, 0xc5, 0x93, 0xc3, 0xe7, 0xcf, 0x86));
TRACELOGGING_DEFINE_PROVIDER(g_hLoggingProvider, "Microsoft.UI.Xaml.Controls.Tabular.Debug",
    (0x03dc0588, 0xa8a2, 0x5562, 0xb2, 0x09, 0x03, 0x91, 0x8b, 0xcc, 0x67, 0x92));
GUID g_TelemetryProviderActivityId{};
const char* gFileVersion = "TableViewPolicyTests";

using namespace TableViewTelemetry;

class TableViewTelemetry_UnitTests
{
public:
    TEST_CLASS(TableViewTelemetry_UnitTests);

    TEST_CLASS_SETUP(ClassSetup)
    {
#if TABLEVIEW_TIP_REAL_LIBRARY
        TestControlReporting(TestReportingRequest::Suspend);
#endif
        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
#if TABLEVIEW_TIP_REAL_LIBRARY
        TestControlReporting(TestReportingRequest::Resume);
#endif
        return true;
    }

    TEST_METHOD(ReadinessIsInitialOnly)
    {
        State state;
        BeginInitial(state, Origin::Loaded);
#if TABLEVIEW_TIP_REAL_LIBRARY
        VERIFY_IS_TRUE(state.initialTipPending && IsInitializing(state));
#endif
        CompleteInitial(state, Result::Success);
        auto const generation = BeginOperation(state, Operation::Filter);
        VERIFY_IS_FALSE(IsInitializing(state));
        IgnoreOperation(state, generation, IgnoreReason::Cancelled);
    }

    TEST_METHOD(InitializationOutcomeDoesNotDependOnNativeAdmission)
    {
        State state;
        BeginInitial(state, Origin::Loaded);
        VERIFY_IS_TRUE(state.initial == InitialState::Started);
        CompleteInitial(state, Result::Success);
        VERIFY_IS_TRUE(state.initial == InitialState::Finished);
        VERIFY_IS_TRUE(state.initialOutcome == InitialOutcome::Arranged);
        IgnoreInitial(state, IgnoreReason::Unloaded);
        BeginInitial(state, Origin::Template);
        VERIFY_IS_TRUE(state.initialOutcome == InitialOutcome::Arranged);
    }

#ifdef KEYWORD_TABLEVIEW
    TEST_METHOD(LoadedSessionsKeepSuccessfulOnlyCounts)
    {
        State state;
        BeginSession(state);
        VERIFY_IS_TRUE(state.session.active);
        auto const epoch = state.session.generation;
        auto const success = BeginOperation(state, Operation::Selection);
        VERIFY_IS_TRUE(IsOperationStarted(state));
        CompleteOperation(state, success, Result::Success);
        CompleteOperation(state, success, Result::Success);
        BeginSession(state);
        VERIFY_ARE_EQUAL(epoch, state.session.generation);
        VERIFY_ARE_EQUAL(1u, state.session.counts[8]);
        auto const ignored = BeginOperation(state, Operation::Selection);
        IgnoreOperation(state, ignored, IgnoreReason::Vetoed);
        auto const incidental = BeginOperation(state, Operation::Layout);
        VERIFY_IS_FALSE(IsOperationStarted(state));
        CompleteOperation(state, incidental, Result::Success);
        VERIFY_ARE_EQUAL(1u, state.session.counts[8]);
        VERIFY_ARE_EQUAL(0u, state.session.counts[4]);
        RecordRetemplate(state);
        VERIFY_ARE_EQUAL(1u, state.session.retemplateCount);
        EndSession(state);
        EndSession(state);
        VERIFY_IS_FALSE(state.session.active);
        BeginSession(state);
        VERIFY_IS_TRUE(state.session.generation > epoch);
        VERIFY_ARE_EQUAL(0u, state.session.counts[8]);
        VERIFY_ARE_EQUAL(0u, state.session.retemplateCount);
        AbandonSession(state);
        VERIFY_IS_FALSE(state.session.active);
    }

    TEST_METHOD(SessionEpochRejectsLateOperationOwners)
    {
        State owner;
        OperationState action{ owner };
        BeginSession(owner);
        auto const old = BeginOperation(action, Operation::CommitEdit);
        EndSession(owner);
        BeginSession(owner);
        auto const current = BeginOperation(owner, Operation::Selection);
        CompleteOperation(action, old, Result::Success);
        VERIFY_ARE_EQUAL(0u, owner.session.counts[10]);
        VERIFY_IS_TRUE(IsOperationStarted(owner));
        CompleteOperation(owner, current, Result::Success);
        VERIFY_ARE_EQUAL(1u, owner.session.counts[8]);
        OperationState lateFailure{ owner };
        auto const failureGeneration = BeginOperation(lateFailure, Operation::CommitEdit);
#if TABLEVIEW_TIP_REAL_LIBRARY
        auto observedFailure = lateFailure.operationTest;
#endif
        EndSession(owner);
        BeginSession(owner);
        FailOperation(lateFailure, Operation::CommitEdit, failureGeneration, Stage::Editing, E_FAIL);
#if TABLEVIEW_TIP_REAL_LIBRARY
        VERIFY_IS_TRUE(observedFailure->state().completionKind == TestCompletionKind::Ignored);
        VERIFY_ARE_EQUAL(static_cast<unsigned short>(IgnoreReason::Stale), observedFailure->state().reason);
#endif
        VERIFY_ARE_EQUAL(0u, owner.session.counts[10]);
        AbandonSession(owner);
    }

    TEST_METHOD(CountersSaturateWithoutWrapping)
    {
        State state;
        BeginSession(state);
        state.session.counts[8] = (std::numeric_limits<uint32_t>::max)();
        state.session.appFaultCount = (std::numeric_limits<uint32_t>::max)();
        auto const success = BeginOperation(state, Operation::Selection);
        CompleteOperation(state, success, Result::Success);
        auto const app = BeginOperation(state, Operation::CommitEdit);
        IgnoreOperation(state, app, IgnoreReason::ApplicationFault, E_INVALIDARG, AppFaultKind::Write);
        VERIFY_ARE_EQUAL((std::numeric_limits<uint32_t>::max)(), state.session.counts[8]);
        VERIFY_ARE_EQUAL((std::numeric_limits<uint32_t>::max)(), state.session.appFaultCount);
        VERIFY_ARE_EQUAL((1u << 8) | (1u << 13), state.session.overflowMask);
        AbandonSession(state);
    }

    TEST_METHOD(AppAndDeferredSamplesDoNotBecomeSuccess)
    {
        State state;
        BeginSession(state);
        for (auto reason : { IgnoreReason::DeferredOrUnproven, IgnoreReason::ApplicationFault,
            IgnoreReason::ObservationLimit, IgnoreReason::UnattributedFault })
        {
            auto const generation = BeginOperation(state, Operation::Sort);
#if TABLEVIEW_TIP_REAL_LIBRARY
            auto observed = state.operationTest;
            VERIFY_IS_TRUE(static_cast<bool>(observed));
#endif
            IgnoreOperation(state, generation, reason, E_INVALIDARG,
                reason == IgnoreReason::ApplicationFault ? AppFaultKind::Binding : AppFaultKind::None);
#if TABLEVIEW_TIP_REAL_LIBRARY
            VERIFY_IS_TRUE(observed->state().completionKind == TestCompletionKind::Ignored);
            VERIFY_ARE_EQUAL(static_cast<unsigned short>(reason), observed->state().reason);
            VERIFY_IS_TRUE(observed->diagnosticHResultAvailable);
            VERIFY_ARE_EQUAL(E_INVALIDARG, observed->diagnosticHResult);
            VERIFY_IS_FALSE(observed->failureHResultAvailable);
#endif
            VERIFY_IS_FALSE(IsOperationStarted(state));
        }
        VERIFY_ARE_EQUAL(0u, state.session.counts[2]);
        VERIFY_ARE_EQUAL(1u, state.session.appFaultCount);
        VERIFY_ARE_EQUAL(1u, state.session.deferredCount);
        VERIFY_ARE_EQUAL(1u, state.session.unattributedFaultCount);
        AbandonSession(state);
    }

    TEST_METHOD(StaleFaultDoesNotTouchNewerWork)
    {
        State state;
        BeginSession(state);
        auto const old = BeginOperation(state, Operation::Sort);
        auto const current = BeginOperation(state, Operation::Filter);
        IgnoreOperation(state, old, IgnoreReason::ApplicationFault, E_FAIL, AppFaultKind::Callback);
        FailOperation(state, Operation::Sort, old, Stage::Sort, E_FAIL);
        VERIFY_IS_TRUE(IsOperationStarted(state));
        VERIFY_ARE_EQUAL(0u, state.session.appFaultCount);
        CompleteOperation(state, current, Result::Success);
        VERIFY_ARE_EQUAL(0u, state.session.counts[2]);
        VERIFY_ARE_EQUAL(1u, state.session.counts[5]);
        AbandonSession(state);
    }

    TEST_METHOD(SemanticTerminalOwnershipDoesNotDependOnEmittedDiagnosticMask)
    {
        State state;
        BeginSession(state);
        auto const old = BeginOperation(state, Operation::Sort);
        VERIFY_IS_FALSE(IsOperationHandled(state, Operation::Sort, old));
        VERIFY_ARE_EQUAL(0u, state.reportedErrors);
        IgnoreOperation(state, old, IgnoreReason::ApplicationFault, E_FAIL, AppFaultKind::Callback);
        VERIFY_IS_TRUE(IsOperationHandled(state, Operation::Sort, old));
        VERIFY_IS_FALSE(IsOperationHandled(state, Operation::Filter, old));
        VERIFY_IS_FALSE(IsOperationHandled(state, Operation::Sort, 0));
        VERIFY_ARE_EQUAL(0u, state.reportedErrors);
        VERIFY_ARE_EQUAL(1u, state.session.appFaultCount);
        VERIFY_ARE_EQUAL(0u, state.session.unattributedFaultCount);
        auto const current = BeginOperation(state, Operation::Sort);
        VERIFY_IS_FALSE(IsOperationHandled(state, Operation::Sort, current));
        VERIFY_IS_FALSE(IsOperationHandled(state, Operation::Sort, old));
        IgnoreOperation(state, current, IgnoreReason::UnattributedFault, E_UNEXPECTED);
        VERIFY_IS_TRUE(IsOperationHandled(state, Operation::Sort, current));
        VERIFY_ARE_EQUAL(1u, state.session.unattributedFaultCount);
        AbandonSession(state);

#if !TABLEVIEW_TIP_REAL_LIBRARY
        State unadmitted;
        auto const diagnosticOnly = BeginOperation(unadmitted, Operation::Sort);
        VERIFY_IS_FALSE(IsOperationStarted(unadmitted));
        IgnoreOperation(unadmitted, diagnosticOnly, IgnoreReason::ApplicationFault, E_FAIL, AppFaultKind::Callback);
        VERIFY_IS_FALSE(IsOperationHandled(unadmitted, Operation::Sort, diagnosticOnly));
#endif
    }

    TEST_METHOD(ShapingChangedAloneIsNotPublicationProof)
    {
        ShapingObservation observation;
        observation.completed = true;
        VERIFY_IS_TRUE(observation.Outcome() == IgnoreReason::DeferredOrUnproven);
        observation.published = true;
        VERIFY_IS_FALSE(observation.Outcome().has_value());
        observation.error = E_FAIL;
        VERIFY_IS_TRUE(observation.Outcome() == IgnoreReason::UnattributedFault);
        observation.error.reset();
        observation.unproven = true;
        VERIFY_IS_TRUE(observation.Outcome() == IgnoreReason::DeferredOrUnproven);
        observation.appFaultKind = AppFaultKind::Callback;
        VERIFY_IS_TRUE(observation.Outcome() == IgnoreReason::ApplicationFault);
        observation = {};
        VERIFY_IS_TRUE(observation.Outcome() == IgnoreReason::UnattributedFault);
    }
#else
    TEST_METHOD(LoadedSessionContractIsRequired)
    {
        VERIFY_FAIL(L"Missing schema3 loaded-session and conservative facade contract.");
    }
#endif
};
