// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <ole2.h>
#include <string>
#define wil_details_GetKernelBaseModuleHandle() GetModuleHandleW(L"kernelbase.dll")
#include <tip.h>
#include <tip/tip_test_helper.h>
#include <DXamlCoreTipTests.h>
#include <UIAffinityReleaseQueueTipTests.h>
#include <WexTestClass.h>

class DXamlCoreTipTests_UnitTests
{
public:
    TEST_CLASS(DXamlCoreTipTests_UnitTests);

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_testReportingGuard = tip::testhelpers::silence_test_results();
        return true;
    }

    TEST_METHOD(Test_SuccessScenario_DXamlInitializeCoreTest)
    {
        auto testVerifier = tip::testhelpers::test_verifier();

        auto test = tip::start_and_watch_errors<DXamlInitializeCoreTest>();

        test.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::init_type_islands_only));
        test.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::initialized_dispatcher));
        test.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::created_dispatcher_xcpwindow));
        test.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::created_uwp_window));

        test.complete();

        VERIFY_IS_TRUE(testVerifier.verify_success<DXamlInitializeCoreTest>());
    }


    TEST_METHOD(Test_FailureScenario_initFailure_DXamlInitializeCoreTest)
    {
        auto testVerifier = tip::testhelpers::test_verifier();

        auto test = tip::start_and_watch_errors<DXamlInitializeCoreTest>();

        test.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::failed_dxamlcore_init));

        test.complete();

        VERIFY_IS_TRUE(testVerifier.verify_failure<DXamlInitializeCoreTest>());
    }

    private:
        tip::testhelpers::unique_tip_unittest_guard m_testReportingGuard;
};

#if defined(MUX_TIP_REAL)
class UIAffinityReleaseQueueTipTests_UnitTests
{
public:
    TEST_CLASS(UIAffinityReleaseQueueTipTests_UnitTests);

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_reportingGuard = tip::testhelpers::silence_test_results();
        return true;
    }

    TEST_METHOD(CompletesWhenTheQueueDrains)
    {
        const GUID id = tip::start<UIAffinityReleaseQueueCleanupTest>().test_id();
        VERIFY_IS_TRUE(id != GUID{});
        auto result = DirectUI::CompleteReleaseQueueCleanupTip(id, GetTickCount64(), 3, S_OK);
        VERIFY_ARE_EQUAL(TestCompletionKind::Success, result.data()->state().completionKind);
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(1), result.data()->releaseCountBucket);
        VERIFY_IS_FALSE(static_cast<bool>(tip::open<UIAffinityReleaseQueueCleanupTest>(id)));
    }

    TEST_METHOD(SampleOutlivesItsStartHandle)
    {
        const GUID id = tip::start<UIAffinityReleaseQueueCleanupTest>().test_id();
        auto running = tip::open<UIAffinityReleaseQueueCleanupTest>(id);
        VERIFY_IS_TRUE(static_cast<bool>(running));
        running.complete();
        VERIFY_ARE_EQUAL(TestCompletionKind::Failure, running.data()->state().completionKind);
        VERIFY_ARE_EQUAL(
            static_cast<unsigned short>(UIAffinityReleaseQueueCleanupTest::reason::incomplete_batch),
            running.data()->state().reason);
    }

    TEST_METHOD(CleanupErrorsFailWithTheOriginalHResult)
    {
        const GUID id = tip::start<UIAffinityReleaseQueueCleanupTest>().test_id();
        auto result = DirectUI::CompleteReleaseQueueCleanupTip(id, GetTickCount64(), 1, E_OUTOFMEMORY);
        VERIFY_ARE_EQUAL(TestCompletionKind::Failure, result.data()->state().completionKind);
        VERIFY_ARE_EQUAL(E_OUTOFMEMORY, result.data()->failureHResult);
    }

    TEST_METHOD(DelayDoesNotTurnSuccessfulCleanupIntoFailure)
    {
        const GUID id = tip::start<UIAffinityReleaseQueueCleanupTest>().test_id();
        auto result = DirectUI::CompleteReleaseQueueCleanupTip(id, GetTickCount64() - 1000, 1, S_OK);
        VERIFY_ARE_EQUAL(TestCompletionKind::Success, result.data()->state().completionKind);
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(3), result.data()->delayBucket);
    }
    TEST_METHOD(MetricsUseBoundedBuckets)
    {
        using Data = UIAffinityReleaseQueueCleanupTest::tip_test_data;
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(0), Data::DelayBucket(15));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(1), Data::DelayBucket(16));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(1), Data::DelayBucket(99));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(2), Data::DelayBucket(100));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(2), Data::DelayBucket(999));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(3), Data::DelayBucket(1000));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(0), Data::CountBucket(1));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(1), Data::CountBucket(2));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(1), Data::CountBucket(16));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(2), Data::CountBucket(17));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(2), Data::CountBucket(256));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(3), Data::CountBucket(257));
        VERIFY_ARE_EQUAL(64884230u, UIAffinityReleaseQueueCleanupTest::info().testCaseId);
    }
private:
    tip::testhelpers::unique_tip_unittest_guard m_reportingGuard;
};
#endif
