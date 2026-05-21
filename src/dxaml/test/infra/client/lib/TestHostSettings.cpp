// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TestHostSettings.h"
#include "WindowHelper.h"
#include "HostingDispatcher.h"

#include <IXamlTestHooks-win.h>

using namespace WEX::Common;
using namespace WEX::TestExecution;

namespace Private { namespace Infrastructure {

wrl::ComPtr<test_infra::Hosting::IWin32HostFactory> TestHostSettingsStatics::s_spWin32HostFactory;

HRESULT TestHostSettingsStatics::put_Win32HostFactory(_In_ test_infra::Hosting::IWin32HostFactory* hostFactory)
{
    s_spWin32HostFactory = hostFactory;
    return S_OK;
}

HRESULT TestHostSettingsStatics::get_Win32HostFactory(_Out_ test_infra::Hosting::IWin32HostFactory** hostFactory)
{
    return s_spWin32HostFactory.CopyTo(hostFactory);
}

} }
