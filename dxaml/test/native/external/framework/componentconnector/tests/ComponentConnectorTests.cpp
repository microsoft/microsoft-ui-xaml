// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ComponentConnectorTests.h"
#include "CustomPages.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>

using namespace std;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace ::Tests::Native::External::Framework::ComponentConnector;

#include "CustomPages.h"

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Framework {

        bool ComponentConnectorTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            featureEnforceXbfV2Stream.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
            return true;
        }

        bool ComponentConnectorTests::ClassCleanup()
        {
            return true;
        }
         
        bool ComponentConnectorTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }
        
        bool ComponentConnectorTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        template <typename T>
        static void ValidateConnectOnIComponentConnectorGetsCalledScenario(bool realize, bool nullConnector)
        {
            TestCleanupWrapper cleanup;

            T^ page = nullptr;

            RunOnUIThread([&]()
            {
                page = ref new T();

                if (nullConnector)
                {
                    page->GetHelper()->SetReturnNullConnector();
                }

                Application::LoadComponent(
                    page,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/componentconnector/PageWithICC.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                TestServices::WindowHelper->WindowContent = page;

                VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(0), 5U);
                VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(1), (!nullConnector) ? 5U : 0U);
                VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(2), 0U);
                VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(3), 0U);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(0), 11U);
                VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(1), (!nullConnector) ? 5U : 0U);
                VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(2), (!nullConnector) ? 4U : 0U);
                VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(3), (!nullConnector) ? 2U : 0U);

                if (realize)
                {
                    page->GetHelper()->Realize(0);

                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(0), 13U);
                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(1), (!nullConnector) ? 7U : 0U);
                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(2), (!nullConnector) ? 4U : 0U);
                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(3), (!nullConnector) ? 2U : 0U);

                    page->GetHelper()->Realize(1);

                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(0), 15U);
                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(1), (!nullConnector) ? 7U : 0U);
                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(2), (!nullConnector) ? 6U : 0U);
                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(3), (!nullConnector) ? 2U : 0U);

                    page->GetHelper()->Realize(2);

                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(0), 16U);
                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(1), (!nullConnector) ? 7U : 0U);
                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(2), (!nullConnector) ? 6U : 0U);
                    VERIFY_ARE_EQUAL(page->GetHelper()->GetCallCount(3), (!nullConnector) ? 3U : 0U);
                }
            });
        }

        void ComponentConnectorTests::ValidateConnectOnIComponentConnectorGetsCalled()
        {
            ValidateConnectOnIComponentConnectorGetsCalledScenario<PageWithICC>(true, false);
            ValidateConnectOnIComponentConnectorGetsCalledScenario<PageWithICC>(true, true);
        }

        void ComponentConnectorTests::ValidateConnectOnIComponentConnectorGetsCalledForDeferredElements()
        {
            ValidateConnectOnIComponentConnectorGetsCalledScenario<PageWithICC>(true, false);
            ValidateConnectOnIComponentConnectorGetsCalledScenario<PageWithICC>(false, true);
        }
    }
} } } }
