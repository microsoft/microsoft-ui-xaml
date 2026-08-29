// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Hosting.h"

#include <Activation.h>
#include <wil\Result.h>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::WRL;
using namespace ::Private::Infrastructure;

HRESULT Hosting::GetHostingMode(HostingMode* hostingMode)
{
   *hostingMode = HostingMode::UAP;

    String value;
    if (SUCCEEDED(RuntimeParameters::TryGetValue(L"HostingMode", value)))
    {
        if (value.IsEmpty() || value.CompareNoCase(L"UAP") == 0)
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
    }

    return S_OK;
}
