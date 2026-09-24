//  Copyright (c) Microsoft Corporation.  All rights reserved.

#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <wil/resource.h>

namespace Microsoft::UI::Xaml::Tests::Common::LaunchTools
{
    DWORD LaunchAppX(
        const wchar_t* packageFamilyNameAndAppId);

    // Arguments can be passed to Modern apps and are useful for measuring perf of different pages with one app.
    // They are available in the app via App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^ e), e->Arguments.
    DWORD LaunchAppX(
        const wchar_t* packageFamilyNameAndAppId,
        const wchar_t* arguments);

    void TerminateProcessByPid(
        DWORD pid);

    void TerminateProcessByHandle(wil::unique_handle handle);

    wil::unique_handle LaunchCmdLineToolAndReturn(
        const wchar_t* cmdLine);
    
    DWORD LaunchCmdLineTool(
        const wchar_t* cmdLine,
        int waitMs);

}
