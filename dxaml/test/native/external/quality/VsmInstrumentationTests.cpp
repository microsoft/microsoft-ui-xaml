// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "VsmInstrumentationTests.h"

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Quality {
                    bool VsmInstrumentationTests::ClassSetup()
                    {
                        CommonTestSetupHelper::CommonTestClassSetup();

                        return true;
                    }

                    bool VsmInstrumentationTests::TestCleanup()
                    {
                        TestServices::WindowHelper->VerifyTestCleanup();
                
                        return true;
                    }

                    void VsmInstrumentationTests::VsmAggregationVerification()
                    {
                        TestCleanupWrapper cleanup;

                        // start the trace consumer before the tailored process starts. 
                        // normally you can launch call this method from your TestSetup which is 
                        // called from within the context of the tailored process
                        TraceConsumer::Start();

                        SetupButtonAndTextBox();
                        RunButtonClickScenario(10);

                        // verify traces
                        TraceConsumer::Stop();

                        // TODO: We cannot currently verify event traces by name till support has FIed to our branch. So we just do a rough count now for verification
                        // The expected number of events after aggregation for VSM is 1. However, if we count the Launch event, the total event count is 2
                        TraceConsumer::VerifyEventTraced("Vsm_Event", 1);
                    }

                    void VsmInstrumentationTests::RunButtonClickScenario(int iterations)
                    {
                        for (int i = 0; i < iterations; ++i)
                        {
                            LOG_OUTPUT(L"Clicking Button.");
                            TestServices::InputHelper->LeftMouseClick(btn);
                        }
                    }

                    void VsmInstrumentationTests::SetupButtonAndTextBox()
                    {
                        RunOnUIThread([&]()
                        {
                            StackPanel^ mainStackPanel = ref new StackPanel();

                            btn = ref new Button();
                            btn->Width = 150;
                            btn->Height = 50;
                            btn->Content = "Button";
                            btn->HorizontalAlignment = HorizontalAlignment::Center;

                            mainStackPanel->Children->Append(btn);
                            TestServices::WindowHelper->WindowContent = mainStackPanel;
                        });

                        TestServices::WindowHelper->WaitForIdle();
                    }
                }
            }
        }
    }
}