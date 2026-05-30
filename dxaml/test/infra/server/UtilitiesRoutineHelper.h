// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#if defined(BUILDING_PRIVATEINFRASERVER_DLL)
#define PI_API __declspec(dllexport)
#else
#define PI_API __declspec(dllimport)
#endif

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {
        class PI_API UtilitiesRoutineHelper
        {
        public:
            static bool IsOneCore();
            static bool IsXBox();
            static bool IsDesktop();

            static bool SetRegKey(const wchar_t* path, const wchar_t* name, DWORD value, BOOLEAN currentUser = false);
            static void DeleteRegKey(const wchar_t* path, const wchar_t* name, BOOLEAN currentUser = false);

            static void EnableChangingTimeZone(bool enable);
            static void SetTimeZone(const WCHAR* timezoneId);
            static void RunCommandLine(const WCHAR* commandLine, DWORD* pExitCode);
            static DWORD GetProcessId(const WCHAR* appExe);
            static void TerminateProcess(const DWORD processId);
            static HRESULT TerminateProcesses(const WCHAR* appExe);

            static bool IsBVT()
            {
                return s_isBVT;
            }

            static void RunAsBVT();

            static void CheckForBVTMode();

            static HRESULT CheckAndKillServerManager();

        private:
            static bool s_isBVT;
        };
    }
} } } }
