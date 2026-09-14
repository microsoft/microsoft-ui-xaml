// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Hosting.h"
#include "HostingModeOverride.h"

#include <Activation.h>
#include <wil\Result.h>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::WRL;
using namespace ::Private::Infrastructure;

HRESULT Hosting::GetHostingMode(HostingMode* hostingMode)
{
    *hostingMode = HostingMode::UAP;

    std::wstring declaredMode;
    RETURN_IF_FAILED_MSG(GetDeclaredHostingMode(declaredMode), "Invalid class-declared hosting mode.");
    String value(declaredMode.c_str());
    if (value.IsEmpty())
    {
        // UAP host startup and clients outside the native test framework can run
        // before ClassSetup has declared a mode.
        RuntimeParameters::TryGetValue(L"HostingMode", value);
    }

    if (!value.IsEmpty())
    {
        if (value.CompareNoCase(L"UAP") == 0)
        {
            *hostingMode = HostingMode::UAP;
        }
        else if(value.CompareNoCase(L"WPF") == 0)
        {
            *hostingMode = HostingMode::WPF;
        }
        else if(value.CompareNoCase(L"WinForms") == 0)
        {
            *hostingMode = HostingMode::WinForms;
        }
        else if(value.CompareNoCase(L"Win32Explicit") == 0)
        {
            *hostingMode = HostingMode::Win32Explicit;
        }
        else
        {
            RETURN_HR_MSG(E_INVALIDARG, "Unrecognized hosting mode: %ls", static_cast<const wchar_t*>(value));
        }
    }

    return S_OK;
}
