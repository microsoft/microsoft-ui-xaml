// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>

using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Test {

    class InfrastructureLeakDetectionTests : public WEX::TestClass<InfrastructureLeakDetectionTests>
    {
    public:
        BEGIN_TEST_CLASS(InfrastructureLeakDetectionTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"478e7719-b8f0-434b-b29a-e7911a52ae17;e63becda-2a21-4186-9066-a5c3d52f5603")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ValidateWpfExpectedLeakDetection)
            TEST_METHOD_PROPERTY(L"Description", L"Detects a deliberately retained native brush, then verifies a clean shutdown.")
        END_TEST_METHOD()
    };

    bool InfrastructureLeakDetectionTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool InfrastructureLeakDetectionTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void InfrastructureLeakDetectionTests::ValidateWpfExpectedLeakDetection()
    {
        for (int interval = 0; interval < 2; ++interval)
        {
            const bool expectLeaks = interval == 0;
            std::weak_ptr<int> retainedLifetime;
            auto releaseThreadId = std::make_shared<DWORD>(0);
            DWORD retiringThreadId = 0;

            {
                auto helper = TestServices::WindowHelper;
                helper->InitializeXaml();
                auto shutdown = wil::scope_exit([&]() {
                    helper->WaitForIdle();
                    helper->ResetWindowContentAndWaitForIdle();
                    helper->ShutdownXaml();
                });

                TestServices::EnableLeakDetection(expectLeaks);
                TestServices::EnableLeakDetection(expectLeaks);

                LOG_OUTPUT(L"WPF leak-detection interval %d: expectLeaks=%s.",
                    interval, expectLeaks ? L"true" : L"false");

                RunOnUIThread([&]() {
                    retiringThreadId = GetCurrentThreadId();
                    if (expectLeaks)
                    {
                        auto lifetime = std::shared_ptr<int>(new int(0), [releaseThreadId](int* value) {
                            *releaseThreadId = GetCurrentThreadId();
                            delete value;
                        });
                        retainedLifetime = lifetime;
                        auto brush = ref new SolidColorBrush();
                        helper->SetPostTickCallback(ref new PostTickCallback([brush, lifetime]() {
                            (void)brush->Opacity;
                        }));
                    }
                });
            }

            VERIFY_IS_TRUE(retainedLifetime.expired());
            if (expectLeaks)
            {
                VERIFY_ARE_EQUAL(retiringThreadId, *releaseThreadId);
            }
            TestServices::WindowHelper->VerifyTestCleanup();
        }
    }

} } } } }
