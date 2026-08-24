//  Copyright (c) Microsoft Corporation.  All rights reserved.

#include "LaunchTools.h"
#include <shobjidl_core.h>
#include <WexTestClass.h>

using namespace WEX::Common;

namespace Microsoft::UI::Xaml::Tests::Common::LaunchTools
{
    DWORD LaunchAppX(
        const wchar_t* packageFamilyNameAndAppId)
    {
        return LaunchAppX(packageFamilyNameAndAppId, nullptr);
    }

    DWORD LaunchAppX(
        const wchar_t* fullPackageName,
        const wchar_t* arguments)
    {
        // Send a keypress from our process. The act of sending or receiving input gives you
        // foreground rights, so this is necessary to ensure that we have such rights, which
        // are needed to call CoAllowSetForegroundWindow.
        INPUT ip = { };
        ip.type = INPUT_KEYBOARD;
        ip.ki.wVk = 0x41;       // Keycode for 'a'

        Throw::LastErrorIfFalse(
            !!::SendInput(
                1 /* number of INPUT structs in array */,
                &ip,
                sizeof(ip)));

        ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release

        Throw::LastErrorIfFalse(
            !!::SendInput(
                1 /* number of INPUT structs in array */,
                &ip,
                sizeof(ip)));

        Microsoft::WRL::ComPtr<IApplicationActivationManager> applicationManager;

        // It is important to use LOCAL_SERVER context when running as an admin, this connects us to
        // a version of the AppXActivationManager with the lower privledges needed to start the app.
        Throw::IfFailed(
            CoCreateInstance(
                CLSID_ApplicationActivationManager,
                nullptr,
                CLSCTX_LOCAL_SERVER,
                IID_PPV_ARGS(&applicationManager)));

        Throw::IfFailed(
            CoAllowSetForegroundWindow(
                applicationManager.Get(),
                nullptr));

        DWORD processIdentifier;
        Throw::IfFailed(applicationManager->ActivateApplication(fullPackageName, arguments, AO_NONE, &processIdentifier));
        return processIdentifier;
    }

    static void TerminateRunningProcess(HANDLE handle)
    {
        DWORD exitCode = 0;

        Throw::IfFalse(
            !!::GetExitCodeProcess(
                handle,
                &exitCode),
            E_FAIL);

        Throw::If(exitCode != STILL_ACTIVE, E_FAIL);

        Throw::IfFalse(
            !!::TerminateProcess(
                handle,
                0),
            E_FAIL);
    }

    void TerminateProcessByPid(
        DWORD pid)
    {
        wil::unique_handle processHandle(::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid));
        TerminateRunningProcess(processHandle.get());
    }

    void TerminateProcessByHandle(wil::unique_handle handle)
    {
         TerminateRunningProcess(handle.get());
    }

    wil::unique_handle LaunchCmdLineToolAndReturn(
        const wchar_t* cmdLine)
    {
        STARTUPINFO startupInfo = { };
        PROCESS_INFORMATION processInformation = { };

        Throw::LastErrorIf(
            ::CreateProcess(
                nullptr,                                                    // lpApplicationName
                reinterpret_cast<LPWSTR>(const_cast<wchar_t*>(cmdLine)),    // lpCommandLine
                nullptr,                                                    // lpProcessAttributes
                nullptr,                                                    // lpThreadAttributes
                TRUE,                                                       // bInheritHandles
                0,                                                          // dwCreationFlags
                nullptr,                                                    // lpEnvironment
                nullptr,                                                    // lpCurrentDirectory
                &startupInfo,
                &processInformation) == 0,
            L"CreateProcess failed");

        // Wrap process handle, so we close it.
        wil::unique_handle processHandle(processInformation.hProcess);

        return processHandle;
    }

    DWORD LaunchCmdLineTool(
        const wchar_t* cmdLine,
        int waitMs)
    {
        wil::unique_handle processHandle = LaunchCmdLineToolAndReturn(cmdLine);

        ::WaitForSingleObject(
            processHandle.get(),
            waitMs);

        DWORD exitCode = -1;

        if (waitMs != INFINITE)
        {
            TerminateRunningProcess(processHandle.get());
        }

        Throw::IfFalse(
            !!::GetExitCodeProcess(
                processHandle.get(),
                &exitCode),
            E_FAIL);

        return exitCode;
    }
}