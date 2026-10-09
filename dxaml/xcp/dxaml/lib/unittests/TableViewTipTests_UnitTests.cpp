// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <ole2.h>
#include <TableViewTipTests.h>
#include <WexTestClass.h>

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
        const auto ignores = {
            TableViewTipReason::cancelled, TableViewTipReason::vetoed,
            TableViewTipReason::validation_rejected, TableViewTipReason::superseded,
            TableViewTipReason::unloaded, TableViewTipReason::retemplated, TableViewTipReason::stale };
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
        VERIFY_ARE_EQUAL(static_cast<unsigned char>(1), initial.version);
        VERIFY_ARE_EQUAL(static_cast<unsigned char>(1), operation.version);
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
            dimensions.content = nonzero ? Content::GroupHeaders : Content::Empty;
            dimensions.rowCountBucket = nonzero ? 3u : 0u;
            dimensions.columnCountBucket = nonzero ? 4u : 0u;
            dimensions.configurationAvailable = nonzero;
            dimensions.rowCountAvailable = nonzero;
            auto const expected = dimensions;

            tson::write_buffer buffer;
            tson::output_archive output(buffer, 1);
            output(TIP_value(dimensions));
            VERIFY_SUCCEEDED(output.finish());

            dimensions = {};
            dimensions.rowCountBucket = 4;
            dimensions.columnCountBucket = 4;
            dimensions.configurationAvailable = true;
            dimensions.rowCountAvailable = true;
            tson::read_buffer inputBuffer(buffer.data(), buffer.size());
            tson::input_archive input(inputBuffer, 1);
            input(TIP_value(dimensions));
            VERIFY_SUCCEEDED(input.finish());
            VERIFY_IS_TRUE(dimensions.origin == expected.origin && dimensions.content == expected.content);
            VERIFY_ARE_EQUAL(expected.rowCountBucket, dimensions.rowCountBucket);
            VERIFY_ARE_EQUAL(expected.columnCountBucket, dimensions.columnCountBucket);
            VERIFY_ARE_EQUAL(expected.configurationAvailable, dimensions.configurationAvailable);
            VERIFY_ARE_EQUAL(expected.rowCountAvailable, dimensions.rowCountAvailable);
            VERIFY_ARE_EQUAL(expected.componentMajor, dimensions.componentMajor);
            VERIFY_ARE_EQUAL(expected.productMajor, dimensions.productMajor);
            VERIFY_ARE_EQUAL(expected.productMinor, dimensions.productMinor);
            VERIFY_ARE_EQUAL(expected.componentBuild, dimensions.componentBuild);

            tson::read_buffer wrongVersionBuffer(buffer.data(), buffer.size());
            tson::input_archive wrongVersion(wrongVersionBuffer, 2);
            VERIFY_FAILED(wrongVersion.finish());
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
            Operation::GroupConfiguration, Operation::GroupExpansion, Operation::Selection,
            Operation::BeginEdit, Operation::CommitEdit, Operation::CancelEdit, Operation::Scroll })
        {
            VERIFY_IS_TRUE(IsMeasuredOperation(operation));
        }
        VERIFY_IS_FALSE(IsMeasuredOperation(Operation::InitialLayout));
        VERIFY_IS_FALSE(IsMeasuredOperation(Operation::HeaderRefresh));
        VERIFY_IS_FALSE(IsMeasuredOperation(Operation::Layout));
    }
};
