// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "UtilitiesRoutineHelper.h"
#include <Shlwapi.h>
#include <PsApi.h>

#include <wil\result.h>
#include <Handle.h>
#include <TestEvent.h>
#include <Timeouts.h>

using namespace WEX::Common;
using namespace WEX::TestExecution;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        bool UtilitiesRoutineHelper::s_isBVT = false;

        bool UtilitiesRoutineHelper::IsOneCore()
        {
            ULONG platformId=0;

            RtlGetDeviceFamilyInfoEnum(nullptr, &platformId, nullptr);

            // We test for known non-OneCore based devices here.
            // We are betting on more OneCore then non-OneCore devices will be added
            if (platformId==DEVICEFAMILYINFOENUM_WINDOWS_8X ||
                platformId==DEVICEFAMILYINFOENUM_DESKTOP ||
                platformId==DEVICEFAMILYINFOENUM_SERVER)
            {
                return false;
            }

            return true;
        }

        bool UtilitiesRoutineHelper::IsDesktop()
        {
            ULONG platformId=0;

            RtlGetDeviceFamilyInfoEnum(nullptr, &platformId, nullptr);

            // We test for known Desktop based devices here.
            if (platformId==DEVICEFAMILYINFOENUM_DESKTOP ||
                platformId==DEVICEFAMILYINFOENUM_SERVER)
            {
                return true;
            }

            return false;
        }

        bool UtilitiesRoutineHelper::IsXBox()
        {
            ULONG platformId = 0;
            RtlGetDeviceFamilyInfoEnum(NULL, &platformId, NULL);

            return (platformId == DEVICEFAMILYINFOENUM_XBOX || platformId == DEVICEFAMILYINFOENUM_XBOXSRA || platformId == DEVICEFAMILYINFOENUM_XBOXERA);
        }

        // Returns True if the reg key with the provided path and name already existed, False if this call created it.
        bool UtilitiesRoutineHelper::SetRegKey(const wchar_t* path, const wchar_t* name, DWORD value, BOOLEAN currentUser )
        {
            bool nameExisted = true;
            HKEY hkey = nullptr;
            HKEY handle = currentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

            if (RegOpenKeyEx(handle, path, 0, KEY_READ | KEY_WRITE, &hkey) != ERROR_SUCCESS)
            {
                LogThrow_IfFailed(RegCreateKeyEx(handle, path, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkey, nullptr));
                nameExisted = false;
            }

            if (nameExisted)
            {
                DWORD data = 0;
                DWORD dwSize = sizeof(DWORD);
                if (RegQueryValueEx(hkey, name, 0, nullptr, reinterpret_cast<LPBYTE>(&data), &dwSize) != ERROR_SUCCESS)
                {
                    nameExisted = false;
                }
            }

            LogThrow_IfFailed(RegSetValueEx(hkey, name, 0, REG_DWORD, reinterpret_cast<LPBYTE>(&value), sizeof(DWORD)));
            LogThrow_IfFailed(RegCloseKey(hkey));

            return nameExisted;
        }

        void UtilitiesRoutineHelper::DeleteRegKey(const wchar_t* path, const wchar_t* name, BOOLEAN currentUser)
        {
            HKEY handle = currentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
            HKEY hkey = nullptr;
            LogThrow_IfFailed(RegOpenKeyEx(handle, path, 0, KEY_WRITE, &hkey));
            LogThrow_IfFailed(RegDeleteValue(hkey, name));
            LogThrow_IfFailed(RegCloseKey(hkey));
        }

        void UtilitiesRoutineHelper::EnableChangingTimeZone(bool enable)
        {
            // Note: The current implementation of SetTimeZone invokes a separate tool, which makes this function
            // unnecessary. However, if SetTimeZone gets updated to change the timezone from within the process, this
            // function will be necessary again.
            HANDLE hToken;
            TOKEN_PRIVILEGES tkp;
            WEX::Common::Throw::IfFalse(!!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken), E_FAIL);
            WEX::Common::Throw::IfFalse(!!LookupPrivilegeValue(NULL, SE_TIME_ZONE_NAME, &tkp.Privileges[0].Luid), E_FAIL);
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

            if (ERROR_SUCCESS != AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)nullptr, nullptr))
            {
                auto lastError = GetLastError();
                if (lastError != 0)
                {
                    LOG_OUTPUT(L"AdjustTokenPrivileges failed (%d)", GetLastError());
                    WEX::Common::Throw::IfFalse(false, E_FAIL);
                }
            }
        }

        void UtilitiesRoutineHelper::SetTimeZone(const WCHAR* timezoneId)
        {
            LOG_OUTPUT(L"Setting timezone to %s", timezoneId);

            std::wstring command = std::wstring(L"tzutil /s \"") + std::wstring(timezoneId) + std::wstring(L"\"");

            DWORD exitCode = 0;
            RunCommandLine(command.c_str(), &exitCode);
            FAIL_FAST_IF(exitCode != 0);
        }

        void UtilitiesRoutineHelper::RunCommandLine(const WCHAR* commandLine, DWORD* pExitCode)
        {
            // commandline parameter to CreateProcess is non-const, so copy
            // filename into a temp buffer first
            const size_t bufferSize = wcslen(commandLine) + 1;
            WCHAR* commandLineCopy = new WCHAR[bufferSize];
            auto deleteCommandLineCopy = wil::scope_exit([&]() {
                    delete[] commandLineCopy;
            });
            LogThrow_IfFailed(
                StringCchCopy(commandLineCopy, bufferSize, commandLine));

            STARTUPINFO startup = {};
            startup.cb = sizeof(startup);
            PROCESS_INFORMATION processInfo = {};

#pragma warning(suppress:6335)  // Process handle processInfo.hThread closed by wil::scope_exit below.
            LogThrow_LastErrorIfFalse(
                ::CreateProcess(
                    nullptr,        // application name
                    commandLineCopy,// command line
                    nullptr,        // process attributes
                    nullptr,        // thread attributes
                    FALSE,          // inherit handles?
                    0,              // creation flags
                    nullptr,        // environmant
                    nullptr,        // current directory
                    &startup,       // startup info
                    &processInfo)); // process info

            auto closeHandles = wil::scope_exit([&]() {
                ::CloseHandle(processInfo.hProcess);
                ::CloseHandle(processInfo.hThread);
            });

            LogThrow_IfFailed(
                ::WaitForSingleObject(processInfo.hProcess, INFINITE));

            LogThrow_LastErrorIfFalse(
                ::GetExitCodeProcess(processInfo.hProcess, pExitCode));
        }

        void UtilitiesRoutineHelper::RunAsBVT()
        {
            s_isBVT = true;
        }

        void UtilitiesRoutineHelper::CheckForBVTMode()
        {
            if (IsBVT()) return;

            String value;
            if (SUCCEEDED(RuntimeParameters::TryGetValue(L"RunAsBVT", value)))
            {
                if (!value.IsEmpty())
                {
                    LOG_OUTPUT(L"Activating BVT mode in server process");
                    UtilitiesRoutineHelper::RunAsBVT();
                }
            }
        }

        VS_FIXEDFILEINFO* GetFixedFileInfo(LPVOID lpData)
        {
            VS_FIXEDFILEINFO* pvsFixedFileInfo = nullptr;
            UINT dumsize = 0;
            ::VerQueryValue(lpData, L"\\", (void**)&pvsFixedFileInfo, &dumsize);
            return pvsFixedFileInfo;
        }

        struct Version
        {
            WORD wMSMajor;
            WORD wMSMinor;
            WORD wLSMajor;
            WORD wLSMinor;
        };

        Version GetFileVersionInfo(const wchar_t* pszFileName)
        {
            DWORD dwHandle = 0;
            DWORD dwLen = GetFileVersionInfoSize(pszFileName, &dwHandle);
            const size_t VERINFO_MAX_SIZE = 0x1000;
            LPVOID lpFileVerInfo[VERINFO_MAX_SIZE];
            WEX::Common::Throw::LastErrorIf(dwLen <= 0 || dwLen > VERINFO_MAX_SIZE, L"GetFileVersionInfo, invalid dwLen");
            WEX::Common::Throw::LastErrorIf(!::GetFileVersionInfo(pszFileName, 0, dwLen, lpFileVerInfo), L"::GetFileVersionInfo");

            VS_FIXEDFILEINFO* pFixedFileInfo = GetFixedFileInfo(lpFileVerInfo);
            WEX::Common::Throw::LastErrorIf(pFixedFileInfo == nullptr);

            Version ver = {};
            ver.wMSMajor = (WORD) ((pFixedFileInfo->dwFileVersionMS & 0xffff0000) >> 16);
            ver.wMSMinor = (WORD) (pFixedFileInfo->dwFileVersionMS & 0x0000ffff);
            ver.wLSMajor = (WORD) ((pFixedFileInfo->dwFileVersionLS & 0xffff0000) >> 16);
            ver.wLSMinor = (WORD) (pFixedFileInfo->dwFileVersionLS & 0x0000ffff);
            LOG_OUTPUT(L"Version:%d.%d.%d.%d", ver.wMSMajor, ver.wMSMinor, ver.wLSMajor, ver.wLSMinor);

            return ver;
        }

        DWORD UtilitiesRoutineHelper::GetProcessId(const WCHAR* appExe)
        {
            DWORD processId = 0;
            DWORD aProcesses[1024];
            DWORD cbNeeded = 0;
            if (!::EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded ) )
            {
                WEX::Common::Throw::LastError(L"EnumProcesses() failed");
            }

            // Calculate how many process identifiers were returned.
            const DWORD cProcesses = cbNeeded / sizeof(DWORD);

            for (uint32_t i = 0; i < cProcesses; i++ )
            {
                if (aProcesses[i] == 0)
                {
                    continue;
                }

                Handle hProcess(::OpenProcess(
                    PROCESS_QUERY_INFORMATION   | // need to enum modules
                    PROCESS_VM_READ             | // allow memory reading
                    PROCESS_TERMINATE           | // allow terminating
                    SYNCHRONIZE,                  // allow waiting on handle
                    FALSE,                        // don't inherit handle
                    aProcesses[i]));              // PID

                if (!hProcess.IsValid())
                {
                    continue;
                }

                HMODULE hMod = nullptr;
                cbNeeded = 0;
                if (::EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
                {
                    wchar_t processName[MAX_PATH] = L"<unknown>";
                    ::GetModuleBaseNameW(hProcess, hMod, processName, _countof(processName));
                    if (0 == _wcsicmp(processName, appExe))
                    {
                        processId = aProcesses[i];
                        break;
                    }
                }
            }
            return processId;
        }

        void UtilitiesRoutineHelper::TerminateProcess(const DWORD processId)
        {
            Handle hProcess(::OpenProcess(
                PROCESS_QUERY_INFORMATION   | // need to enum modules
                PROCESS_VM_READ             | // allow memory reading
                PROCESS_TERMINATE           | // allow terminating
                SYNCHRONIZE,                  // allow waiting on handle
                FALSE,                        // don't inherit handle
                processId));                  // PID

            if (!hProcess.IsValid())
            {
                WEX::Common::Throw::LastError(L"Cannot OpenProcess");
            }

            LOG_OUTPUT(L"Terminating PID %lu", processId);

            if (!::TerminateProcess(hProcess, 1))
            {
                WEX::Common::Throw::LastError(L"Failed to terminate process");
            }
            LOG_OUTPUT(L"Called TerminateProcess. Waiting on the handle...");
            DWORD terminateWFSO = ::WaitForSingleObject(hProcess, static_cast<DWORD>(Event::GetDefaultTimeout().count()));
            if (WAIT_OBJECT_0 == terminateWFSO)
            {
               LOG_OUTPUT(L"Terminated process successfully");
            }
            else
            {
                WEX::Logging::Log::Error(L"Timeout waiting for process handle to terminate.");
            }
        }

        HRESULT UtilitiesRoutineHelper::TerminateProcesses(const WCHAR* appExe)
        {
            DWORD aProcesses[1024];
            DWORD cbNeeded = 0;
            if (!::EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded ) )
            {
                WEX::Logging::Log::Error(L"EnumProcesses() failed");
                return E_FAIL;
            }

            // Calculate how many process identifiers were returned.
            const DWORD cProcesses = cbNeeded / sizeof(DWORD);

            for (uint32_t i = 0; i < cProcesses; i++ )
            {
                if (aProcesses[i] == 0)
                {
                    continue;
                }

                Handle hProcess(::OpenProcess(
                    PROCESS_QUERY_INFORMATION   | // need to enum modules
                    PROCESS_VM_READ             | // allow memory reading
                    PROCESS_TERMINATE           | // allow terminating
                    SYNCHRONIZE,                  // allow waiting on handle
                    FALSE,                        // don't inherit handle
                    aProcesses[i]));              // PID

                if (!hProcess.IsValid())
                {
                    continue;
                }

                HMODULE hMod = nullptr;
                cbNeeded = 0;
                if (::EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
                {
                    wchar_t processName[MAX_PATH] = L"<unknown>";
                    GetModuleBaseNameW(hProcess, hMod, processName, _countof(processName));
                    if (0 == _wcsicmp(processName, appExe))
                    {
                       LOG_OUTPUT(L"Terminating PID %lu", aProcesses[i]);

                       if (!::TerminateProcess(hProcess, 1))
                       {
                           WEX::Logging::Log::Error(L"Failed to terminate process");
                           return E_FAIL;
                       }
                       LOG_OUTPUT(L"Called TerminateProcess. Waiting on the handle...");
                       DWORD terminateWFSO = WaitForSingleObject(hProcess, static_cast<DWORD>(Event::GetDefaultTimeout().count()));
                       if (WAIT_OBJECT_0 == terminateWFSO)
                       {
                           LOG_OUTPUT(L"Terminated process successfully");
                       }
                       else
                       {
                          WEX::Logging::Log::Error(L"Timeout waiting for process handle to terminate.");
                       }
                    }
                }
            }
            return S_OK;
        }

        HRESULT UtilitiesRoutineHelper::CheckAndKillServerManager()
        {
            LOG_OUTPUT(L"UtilitiesRoutineHelper::CheckAndKillServerManager");

            if (IsBVT())
            {
                const auto maxKills = 10;
                for(auto kills=0;kills<maxKills;kills++)
                {
                    DWORD processId = GetProcessId(L"ServerManager.exe");
                    if (processId != 0)
                    {
                        WEX::Logging::Log::Warning(L"Found an active process instance of ServerManager.exe");
                        LOG_OUTPUT(L"ServerManager.exe, killing PID: %lu", processId);
                        TerminateProcess(processId);
                        LOG_OUTPUT(L"ServerManager.exe killed");
                    }
                    else
                    {
                        break;
                    }
                }
                DWORD processId = GetProcessId(L"ServerManager.exe");
                if (processId != 0)
                {
                   WEX::Logging::Log::Error(L"Giving up trying to kill ServerManager.exe");
                }
            }

            return S_OK;
        }

    }
} } } }
