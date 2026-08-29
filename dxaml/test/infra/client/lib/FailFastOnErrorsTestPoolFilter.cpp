// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FailFastOnErrorsTestPoolFilter.h"
#include "XamlTailored.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace WEX::Common;
using namespace WEX::Logging;

namespace Private { namespace Infrastructure {

bool FailFastOnErrorsTestPoolFilter::IsDirty()
{
    BOOLEAN failFastOnErrors = FALSE;

    RunOnUIThread([&]() {
        wrl::ComPtr<xaml::IApplicationStatics> appStatics;
        FAIL_FAST_IF_FAILED(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Application).Get(), &appStatics));

        wrl::ComPtr<xaml::IApplication> app;
        auto hr = appStatics->get_Current(&app);

        // The test may have called ShutdownXaml, in which case we will no longer have an Application
        // so we need to handle that case.
        if(SUCCEEDED(hr) && app)
        {
            wrl::ComPtr<xaml::IDebugSettings> debugSettings;
            FAIL_FAST_IF_FAILED(app->get_DebugSettings(&debugSettings));

            debugSettings->get_FailFastOnErrors(&failFastOnErrors);
        }
    });

    // If FailFastOnErrors is still enabled, other tests may unnecessarily crash.
    if (failFastOnErrors)
    {
        Log::Error(L"FailFastOnErrors mode is still active after the test.");
    }
    return !!failFastOnErrors;
}

} }

