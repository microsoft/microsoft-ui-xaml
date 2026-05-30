// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Platform;
using namespace Concurrency;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Quality {
                    class VsmInstrumentationTests : public WEX::TestClass<VsmInstrumentationTests>
                    {
                        public:
                            BEGIN_TEST_CLASS(VsmInstrumentationTests)
                                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                            END_TEST_CLASS()

                            TEST_CLASS_SETUP(ClassSetup)

                            TEST_METHOD_CLEANUP(TestCleanup)

                            BEGIN_TEST_METHOD(VsmAggregationVerification)
                                TEST_METHOD_PROPERTY(L"RunAs", L"UAP")
                                TEST_METHOD_PROPERTY(L"Ignore", L"True")
                                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can verify traces for VSM Instrumentation.")
                                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                            END_TEST_METHOD()

                            void SetupButtonAndTextBox();
                            void RunButtonClickScenario(int iterations);

                        private:
                            Button^ btn = nullptr;
                    };
                }
            }
        }
    }
}
