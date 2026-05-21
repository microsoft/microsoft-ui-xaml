// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>

#include "WindowHelper.h"
#include "Utilities.h"
#include "ErrorHandlingHelper.h"
#include <string>

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ErrorHandling;

namespace Private { namespace Infrastructure {

HRESULT ErrorHandlingHelper::RuntimeClassInitialize()
{
    COM_START
    {
        auto testHooks = WindowHelper::GetTestHooks();
        m_spStackLogger = std::make_unique<LoggingHelper>(testHooks.Get());

        // We enable this only on the UI thread- TAEF itself is good about capturing unhandled
        // exceptions from the test execution threads themselves.
        RunOnUIThread([&]() {
            m_spStackLogger->SetUnhandledExceptionFilter();
        });
    }
    COM_END
}

HRESULT ErrorHandlingHelper::get_PrintStacksOnJupiterFailure(BOOLEAN* pEnable)
{
    *pEnable = m_spStackLogger->GetPrintStacksOnJupiterFailure();
    return S_OK;
}

HRESULT ErrorHandlingHelper::put_PrintStacksOnJupiterFailure(BOOLEAN enable)
{
    m_spStackLogger->SetPrintStacksOnJupiterFailure(!!enable);
    return S_OK;
}

HRESULT ErrorHandlingHelper::IgnoreLeaksForTest()
{
    m_spStackLogger->SetIgnoreLeaksForTest(true);
    return S_OK;
}

void ErrorHandlingHelper::TrackLeaksForTest()
{
    wrl::ComPtr<test_infra::ITestServicesStatics> testServicesStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
        &testServicesStatics
    ));

    wrl::ComPtr<test_infra::IErrorHandlingHelper> errorHelper;
    LogThrow_IfFailed(testServicesStatics->get_ErrorHandlingHelper(&errorHelper));
    if (errorHelper)
    {
        static_cast<ErrorHandlingHelper*>(errorHelper.Get())->m_spStackLogger->SetIgnoreLeaksForTest(false);
    }
}
void ErrorHandlingHelper::PerformLeakDetection()
{
    auto testHooks = WindowHelper::GetTestHooks();

    unsigned int leakThreshold = UINT_MAX;

    WEX::Common::String value;
    if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"LeakThreshold", value)))
    {
        // If a value for leak threshold was passed in, then use that.
        leakThreshold = std::stoi(value.GetBuffer());
    }

    LOG_OUTPUT(L"Checking for leaks.");
    RunOnUIThread([&] {
        testHooks->PostTestCheckForLeaks(leakThreshold);
    });
}

bool ErrorHandlingHelper::ShouldIgnoreLeaks()
{
    wrl::ComPtr<test_infra::ITestServicesStatics> testServicesStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
        &testServicesStatics
    ));

    wrl::ComPtr<test_infra::IErrorHandlingHelper> errorHelper;
    LogThrow_IfFailed(testServicesStatics->get_ErrorHandlingHelper(&errorHelper));
    if (errorHelper)
    {
        return static_cast<ErrorHandlingHelper*>(errorHelper.Get())->m_spStackLogger->GetIgnoreLeaksForTest();
    }

    return false;
}

} }