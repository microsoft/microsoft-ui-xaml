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

    TEST_METHOD(CompletesOnlyAfterTheSampledReleases)
    {
        DirectUI::ReleaseQueueCleanupTip sample(GetTickCount64(), 2);
        VERIFY_IS_TRUE(sample.test.test_id() != GUID{});
        VERIFY_IS_FALSE(sample.RecordRelease());
        VERIFY_IS_TRUE(static_cast<bool>(sample.test));
        VERIFY_IS_TRUE(sample.RecordRelease());
        VERIFY_IS_FALSE(static_cast<bool>(sample.test));
        VERIFY_ARE_EQUAL(TestCompletionKind::Success, sample.test.data()->state().completionKind);
    }

    TEST_METHOD(PostingOrStartingWithoutReleasesDoesNotSucceed)
    {
        DirectUI::ReleaseQueueCleanupTip sample(GetTickCount64(), 1);
        sample.test.complete();
        VERIFY_ARE_EQUAL(TestCompletionKind::Failure, sample.test.data()->state().completionKind);
        VERIFY_ARE_EQUAL(
            static_cast<unsigned short>(UIAffinityReleaseQueueCleanupTest::reason::incomplete_batch),
            sample.test.data()->state().reason);
    }

    TEST_METHOD(ShutdownIgnoresAnUnfinishedSample)
    {
        UIAffinityReleaseQueueCleanupTest result;
        {
            DirectUI::ReleaseQueueCleanupTip sample(GetTickCount64(), 2);
            result = sample.test;
            VERIFY_IS_FALSE(sample.RecordRelease());
        }
        VERIFY_ARE_EQUAL(TestCompletionKind::Ignored, result.data()->state().completionKind);
        VERIFY_ARE_EQUAL(
            static_cast<unsigned short>(UIAffinityReleaseQueueCleanupTest::reason::cancelled),
            result.data()->state().reason);
    }

    TEST_METHOD(CleanupErrorsFailWithTheOriginalHResult)
    {
        DirectUI::ReleaseQueueCleanupTip sample(GetTickCount64(), 2);
        VERIFY_IS_FALSE(sample.RecordRelease());
        sample.Fail(E_OUTOFMEMORY);
        VERIFY_ARE_EQUAL(TestCompletionKind::Failure, sample.test.data()->state().completionKind);
        VERIFY_ARE_EQUAL(E_OUTOFMEMORY, sample.test.data()->failureHResult);
    }

    TEST_METHOD(DelayDoesNotTurnSuccessfulCleanupIntoFailure)
    {
        DirectUI::ReleaseQueueCleanupTip sample(GetTickCount64() - 1000, 1);
        VERIFY_IS_TRUE(sample.RecordRelease());
        VERIFY_ARE_EQUAL(TestCompletionKind::Success, sample.test.data()->state().completionKind);
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(3), sample.test.data()->delayBucket);
    }

    TEST_METHOD(MetricsUseBoundedBuckets)
    {
        using DirectUI::ReleaseQueueCleanupTip;
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(0), ReleaseQueueCleanupTip::DelayBucket(15));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(1), ReleaseQueueCleanupTip::DelayBucket(16));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(1), ReleaseQueueCleanupTip::DelayBucket(99));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(2), ReleaseQueueCleanupTip::DelayBucket(100));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(2), ReleaseQueueCleanupTip::DelayBucket(999));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(3), ReleaseQueueCleanupTip::DelayBucket(1000));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(0), ReleaseQueueCleanupTip::CountBucket(1));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(1), ReleaseQueueCleanupTip::CountBucket(2));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(1), ReleaseQueueCleanupTip::CountBucket(16));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(2), ReleaseQueueCleanupTip::CountBucket(17));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(2), ReleaseQueueCleanupTip::CountBucket(256));
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(3), ReleaseQueueCleanupTip::CountBucket(257));
        VERIFY_ARE_EQUAL(64884230u, UIAffinityReleaseQueueCleanupTest::info().testCaseId);
    }

    TEST_METHOD(ReportsTheCompletedCountAndDelayBucket)
    {
        DirectUI::ReleaseQueueCleanupTip sample(GetTickCount64(), 17);
        for (size_t index = 0; index < 16; ++index)
        {
            VERIFY_IS_FALSE(sample.RecordRelease());
        }
        VERIFY_IS_TRUE(sample.RecordRelease());
        auto data = sample.test.data();
        VERIFY_ARE_EQUAL(static_cast<uint8_t>(2), data->releaseCountBucket);
        VERIFY_ARE_EQUAL(static_cast<unsigned int>(data->delayBucket), data->state().metricsBucket);
        VERIFY_ARE_EQUAL(static_cast<uint32_t>(WINUI_BUILD_VERSION), data->componentBuild);
    }

    TEST_METHOD(DestructionDoesNotOverwriteSuccess)
    {
        UIAffinityReleaseQueueCleanupTest result;
        {
            DirectUI::ReleaseQueueCleanupTip sample(GetTickCount64(), 1);
            result = sample.test;
            VERIFY_IS_TRUE(sample.RecordRelease());
            VERIFY_IS_FALSE(sample.RecordRelease());
        }
        VERIFY_ARE_EQUAL(TestCompletionKind::Success, result.data()->state().completionKind);
    }

private:
    tip::testhelpers::unique_tip_unittest_guard m_reportingGuard;
};
#endif
