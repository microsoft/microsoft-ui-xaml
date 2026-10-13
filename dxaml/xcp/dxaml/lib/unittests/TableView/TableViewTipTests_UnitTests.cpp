// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <ole2.h>
#include <string>
#include <TableViewTipTests.h>
#include <WexTestClass.h>
#include "TableViewTipTestHelpers.h"

using namespace TableViewTelemetry;

class TableViewTipTests_UnitTests
{
public:
    TEST_CLASS(TableViewTipTests_UnitTests);

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

#if TABLEVIEW_TIP_REAL_LIBRARY
    TEST_METHOD(InitializationRequiresPositiveCompletion)
    {
        if (!TableViewTipTestHelpers::RequireNativeAbi()) { return; }
        auto success = tip::start<TableViewInitializationTest>();
        VERIFY_IS_TRUE(static_cast<bool>(success));
        VERIFY_IS_TRUE(success.test_id() != GUID{});
        success.set_flag(TIP_reason(TableViewTipReason::scenario_completed));
        success.complete();
        VERIFY_IS_TRUE(success->state().completionKind == TestCompletionKind::Success);
        VERIFY_ARE_EQUAL(static_cast<unsigned short>(TableViewTipReason::scenario_completed), success->state().reason);

        auto incomplete = tip::start<TableViewInitializationTest>();
        VERIFY_IS_TRUE(static_cast<bool>(incomplete));
        incomplete.complete();
        VERIFY_IS_TRUE(incomplete->state().completionKind == TestCompletionKind::Failure);
        VERIFY_ARE_EQUAL(static_cast<unsigned short>(TableViewTipReason::scenario_not_completed), incomplete->state().reason);
    }

    TEST_METHOD(FailureFlagsAreAuthoritative)
    {
        if (!TableViewTipTestHelpers::RequireNativeAbi()) { return; }
        const auto failures = {
            TableViewTipReason::template_failure, TableViewTipReason::source_failure,
            TableViewTipReason::shaping_failure, TableViewTipReason::realization_failure,
            TableViewTipReason::selection_failure, TableViewTipReason::editing_failure,
            TableViewTipReason::scroll_failure, TableViewTipReason::internal_failure };
        for (auto reason : failures)
        {
            auto test = tip::start<TableViewOperationTest>();
            VERIFY_IS_TRUE(static_cast<bool>(test));
            test.set_flag(TIP_reason(TableViewTipReason::scenario_completed));
            test.set_flag(static_cast<unsigned short>(reason), "fixed_failure");
            test.complete();
            VERIFY_IS_TRUE(test->state().completionKind == TestCompletionKind::Failure);
            VERIFY_ARE_EQUAL(static_cast<unsigned short>(reason), test->state().reason);
        }
    }

    TEST_METHOD(IgnoreDoesNotRequireSuccessFlags)
    {
        if (!TableViewTipTestHelpers::RequireNativeAbi()) { return; }
        const auto ignores = {
            TableViewTipReason::cancelled, TableViewTipReason::vetoed,
            TableViewTipReason::validation_rejected, TableViewTipReason::superseded,
            TableViewTipReason::unloaded, TableViewTipReason::retemplated, TableViewTipReason::stale,
            TableViewTipReason::deferred_or_unproven, TableViewTipReason::application_fault,
            TableViewTipReason::observation_limit, TableViewTipReason::unattributed_fault };
        for (auto reason : ignores)
        {
            auto test = tip::start<TableViewOperationTest>();
            VERIFY_IS_TRUE(static_cast<bool>(test));
            test.complete_and_ignore(static_cast<unsigned short>(reason), "expected_ignore");
            VERIFY_IS_TRUE(test->state().completionKind == TestCompletionKind::Ignored);
            VERIFY_ARE_EQUAL(static_cast<unsigned short>(reason), test->state().reason);
        }
    }

    TEST_METHOD(ConcurrentInstancesAndRepeatedCompletion)
    {
        if (!TableViewTipTestHelpers::RequireNativeAbi()) { return; }
        auto first = tip::start<TableViewOperationTest>();
        auto second = tip::start<TableViewOperationTest>();
        VERIFY_IS_TRUE(static_cast<bool>(first) && static_cast<bool>(second));
        VERIFY_IS_TRUE(first.test_id() != second.test_id());
        first.set_flag(TIP_reason(TableViewTipReason::scenario_completed));
        first.complete();
        first.complete_and_fail(TIP_reason(TableViewTipReason::internal_failure));
        VERIFY_IS_TRUE(first->state().completionKind == TestCompletionKind::Success);
        VERIFY_IS_TRUE(static_cast<bool>(second));
        second.complete_and_ignore(TIP_reason(TableViewTipReason::cancelled));
    }

    TEST_METHOD(VersionAndLifetimeAreBounded)
    {
        const auto initial = TableViewInitializationTest::info();
        const auto operation = TableViewOperationTest::info();
        VERIFY_ARE_EQUAL(64477764u, initial.testCaseId);
        VERIFY_ARE_EQUAL(64477774u, operation.testCaseId);
        VERIFY_ARE_EQUAL(static_cast<unsigned char>(4), initial.version);
        VERIFY_ARE_EQUAL(static_cast<unsigned char>(3), operation.version);
        VERIFY_IS_TRUE(initial.storage == TestStorage::Process && operation.storage == TestStorage::Process);
        constexpr auto forbidden = TestProperties::ExtendedLifetime | TestProperties::RuntimeExtended |
            TestProperties::RuntimeMax | TestProperties::PerfTrack | TestProperties::KeyMoment |
            TestProperties::MetricsBucketFullResolution;
        VERIFY_IS_TRUE((initial.properties & forbidden) == TestProperties::None);
        VERIFY_IS_TRUE((operation.properties & forbidden) == TestProperties::None);
    }

    TEST_METHOD(DimensionsRoundTripZeroAndNonzero)
    {
        for (const auto nonzero : { false, true })
        {
            TableViewTipDimensions dimensions;
            dimensions.origin = nonzero ? Origin::Loaded : Origin::Template;
            auto const expected = dimensions;

            tson::write_buffer buffer;
            tson::output_archive output(buffer, 3);
            output(TIP_value(dimensions));
            VERIFY_SUCCEEDED(output.finish());

            dimensions = {};
            tson::read_buffer inputBuffer(buffer.data(), buffer.size());
            tson::input_archive input(inputBuffer, 3);
            input(TIP_value(dimensions));
            VERIFY_SUCCEEDED(input.finish());
            VERIFY_IS_TRUE(dimensions.origin == expected.origin);
            VERIFY_ARE_EQUAL(expected.componentMajor, dimensions.componentMajor);
            VERIFY_ARE_EQUAL(expected.productMajor, dimensions.productMajor);
            VERIFY_ARE_EQUAL(expected.productMinor, dimensions.productMinor);
            VERIFY_ARE_EQUAL(expected.componentBuild, dimensions.componentBuild);

            tson::read_buffer wrongVersionBuffer(buffer.data(), buffer.size());
            tson::input_archive wrongVersion(wrongVersionBuffer, 2);
            VERIFY_FAILED(wrongVersion.finish());

            TableViewOperationTest::tip_test_data custom;
            custom.dimensions = expected;
            custom.operation = nonzero ? Operation::CommitEdit : Operation::InitialLayout;
            custom.diagnosticHResult = nonzero ? E_INVALIDARG : S_OK;
            custom.diagnosticHResultAvailable = nonzero;
            custom.appFaultKind = nonzero ? AppFaultKind::Binding : AppFaultKind::None;
            auto const expectedOperation = custom.operation;
            tson::write_buffer customBuffer;
            tson::output_archive customOutput(customBuffer, 3);
            custom.serialize(customOutput);
            VERIFY_SUCCEEDED(customOutput.finish());
            custom = {};
            tson::read_buffer customInputBuffer(customBuffer.data(), customBuffer.size());
            tson::input_archive customInput(customInputBuffer, 3);
            custom.serialize(customInput);
            VERIFY_SUCCEEDED(customInput.finish());
            VERIFY_IS_TRUE(custom.operation == expectedOperation);
            VERIFY_ARE_EQUAL(nonzero ? E_INVALIDARG : S_OK, custom.diagnosticHResult);
            VERIFY_ARE_EQUAL(nonzero, custom.diagnosticHResultAvailable);
            VERIFY_IS_TRUE(custom.appFaultKind == (nonzero ? AppFaultKind::Binding : AppFaultKind::None));
            VERIFY_IS_FALSE(custom.failureHResultAvailable);
        }
    }
#else
    TEST_METHOD(PublicStubDoesNotAdmitTests)
    {
        auto initial = tip::start<TableViewInitializationTest>();
        auto operation = tip::start<TableViewOperationTest>();
        VERIFY_IS_FALSE(static_cast<bool>(initial));
        VERIFY_IS_FALSE(static_cast<bool>(operation));
        VERIFY_IS_TRUE(initial.test_id() == GUID{} && operation.test_id() == GUID{});
    }
#endif

    TEST_METHOD(OperationKindsExcludeIncidentalLayout)
    {
        for (auto operation : { Operation::ReplaceSource, Operation::Sort, Operation::Filter,
            Operation::GroupConfiguration, Operation::Selection,
            Operation::BeginEdit, Operation::CommitEdit, Operation::CancelEdit })
        {
            VERIFY_IS_TRUE(IsMeasuredOperation(operation));
        }
        VERIFY_IS_FALSE(IsMeasuredOperation(Operation::InitialLayout));
        VERIFY_IS_FALSE(IsMeasuredOperation(Operation::HeaderRefresh));
        VERIFY_IS_FALSE(IsMeasuredOperation(Operation::Layout));
        VERIFY_IS_FALSE(IsMeasuredOperation(Operation::GroupExpansion));
        VERIFY_IS_FALSE(IsMeasuredOperation(Operation::Scroll));
    }
};
