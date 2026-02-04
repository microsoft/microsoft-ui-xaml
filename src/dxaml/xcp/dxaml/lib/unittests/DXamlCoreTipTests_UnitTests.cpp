// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <ole2.h>
#define wil_details_GetKernelBaseModuleHandle() GetModuleHandleW(L"kernelbase.dll")
#include <tip.h>
#include <tip/tip_test_helper.h>
#include <DXamlCoreTipTests.h>
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
