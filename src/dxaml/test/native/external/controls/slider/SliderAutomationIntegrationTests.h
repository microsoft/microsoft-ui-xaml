// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Slider {

    class SliderAutomationIntegrationTests : public WEX::TestClass<SliderAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(SliderAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyDefaultAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that Slider has reasonable default AutomationProperties.Name property if not explicitly set.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyValuePropertyChangedEventIsRaised)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the RangeValue Value PropertyChangedEvent occurs when Slider.Value is changed.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyValuePropertyChangedEventIsRaisedMultipleTimes)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the RangeValue Value PropertyChangedEvent occurs as many times  Slider.Value is changed.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyValuePropertyChangedEventCoalescingonMTC)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that when in MTC, Slider.Value is changed multiple times in quick sucession, only one PropertyChangedEvent is raised for RangeValue pattern")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

    private:
        void SetupAutomateSlider(xaml_controls::Slider^* slider);
    };

} } } } } }
