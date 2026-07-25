//  Copyright (c) Microsoft Corporation.  All rights reserved.

#include "AppPerfTests.h"
#include "LaunchTools.h"
#include <Windows.h>
#include <wil/resource.h>

using namespace Microsoft::UI::Xaml::Tests::Common::LaunchTools;
using namespace WEX::Common;

#define LOG_OUTPUT(fmt, ...) WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))

namespace Microsoft::UI::Xaml::Tests::AppPerf
{
    class ExecutionSettings
    {
        int m_launchCount                   = 0;
        int m_appActiveTimeMilliseconds     = 0;
        int m_preRunTimeMilliseconds        = 0;
        int m_postRunTimeMilliseconds       = 0;
        String m_binaryDirPath;
        String m_appToRun;
        String m_appArgs;
        bool m_runAsCommandLine             = false;
        String m_sweepModules;
        String m_sweepOutput;
        String m_sweepOutputDir;

    public:
        int GetLaunchCount() const
        {
            return m_launchCount;
        }

        int GetAppActiveTimeMilliseconds() const
        {
            return m_appActiveTimeMilliseconds;
        }

        int GetPreRunTimeMilliseconds() const
        {
            return m_preRunTimeMilliseconds;
        }

        int GetPostRunTimeMilliseconds() const
        {
            return m_postRunTimeMilliseconds;
        }
        
        const String& GetBinaryDirPath() const
        {
            return m_binaryDirPath;
        }

        const String& GetAppToRun() const
        {
            return m_appToRun;
        }

        const String& GetAppArgs() const
        {
            return m_appArgs;
        }

        bool RunAsCommandLine() const
        {
            return m_runAsCommandLine;
        }

        bool ShouldSweep() const
        {
            return !m_sweepModules.IsEmpty();
        }

        const String& GetSweepModules() const
        {
            return m_sweepModules;
        }

        const String& GetSweepOutput() const
        {
            return m_sweepOutput;
        }

        const String& GetSweepOutputDir() const
        {
            return m_sweepOutputDir;
        }

        static ExecutionSettings ReadWithDefaults()
        {
            ExecutionSettings result;

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"LaunchCount", result.m_launchCount)))
            {
                result.m_launchCount = 1;
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"AppActiveTime", result.m_appActiveTimeMilliseconds)))
            {
                result.m_appActiveTimeMilliseconds = 3000;
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"PreRunTime", result.m_preRunTimeMilliseconds)))
            {
                result.m_preRunTimeMilliseconds = 1000;
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"PostRunTime", result.m_postRunTimeMilliseconds)))
            {
                result.m_postRunTimeMilliseconds = 1000;
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"AppToRun", result.m_appToRun)))
            {
                Throw::Exception(E_INVALIDARG);
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"AppArgs", result.m_appArgs)))
            {
                result.m_appArgs.Empty();
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"RunAsCommandLine", result.m_runAsCommandLine)))
            {
                result.m_runAsCommandLine = false;
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"SweepModules", result.m_sweepModules)))
            {
                result.m_sweepModules.Empty();
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"SweepOutput", result.m_sweepOutput)))
            {
                result.m_sweepOutput.Format(
                    L"%s.pgc",
                    static_cast<const wchar_t*>(result.m_appToRun));
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"SweepOutputDir", result.m_sweepOutputDir)))
            {
                result.m_sweepOutputDir.Format(
                    L".\\");
            }

            if (FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"BinaryDirPath", result.m_binaryDirPath)))
            {
                result.m_binaryDirPath.Format(L".\\");
            }

            return result;
        }
    };

    ExecutionSettings g_executionSettings;

    static void Wait(DWORD milliseconds)
    {
        LOG_OUTPUT(L"*** Waiting %d ms...", milliseconds);
        Sleep(milliseconds);
    }

    static void Sweep(const wchar_t* instrumentedModule, const wchar_t* pgcFile)
    {
        WEX::Common::String outputDir;
        outputDir.Format(
            L"%s\\%s",
            static_cast<const wchar_t*>(g_executionSettings.GetSweepOutputDir()),
            instrumentedModule);

        BOOL createDirResult = ::CreateDirectoryW(static_cast<const wchar_t*>(outputDir), nullptr);
        if (!createDirResult)
        {
            LOG_OUTPUT(L"*** CreateDirectoryW returns false, gle=0x%x", ::GetLastError());
        }

        Throw::IfFalse(createDirResult, E_FAIL);

        WEX::Common::String pgcName;
        pgcName.Format(
            L"%s\\%s",
            static_cast<const wchar_t*>(outputDir),
            pgcFile);

        WEX::Common::String cmdLine;
        cmdLine.Format(
            L".\\pgosweep.exe %s %s",
            instrumentedModule,
            static_cast<const wchar_t*>(pgcName));

        LOG_OUTPUT(L"*** Starting %s", static_cast<const wchar_t*>(cmdLine));
        DWORD exitCode = LaunchCmdLineTool(static_cast<const wchar_t*>(cmdLine), -1);
        if (exitCode != 0)
        {
            LOG_OUTPUT(L"*** Sweep failed with exit code, 0x%x", exitCode);
            Throw::IfFalse(exitCode == 0, E_FAIL);
        }
    }

    static void SweepIfNeeded()
    {
        if (g_executionSettings.ShouldSweep())
        {
            static constexpr wchar_t k_delims[] = L"!";

            // Make copy since we'll be modifying it.
            String moduleNames = g_executionSettings.GetSweepModules();

            wchar_t* moduleName = wcstok(const_cast<wchar_t*>(static_cast<const wchar_t*>(moduleNames)), k_delims);
            while (moduleName != nullptr)
            {
                Sweep(
                    moduleName,
                    g_executionSettings.GetSweepOutput());

                moduleName = wcstok(nullptr, k_delims);
            }
        }
    }

    static void AppLaunchAndTerminate(const wchar_t* appId, const wchar_t* args)
    {
        int pid = LaunchAppX(appId, args);

        auto scopeGuard = wil::scope_exit([pid]
        {
            LOG_OUTPUT(L"*** Terminating app");
            TerminateProcessByPid(pid);
            Wait(g_executionSettings.GetPostRunTimeMilliseconds());
        });

        LOG_OUTPUT(L"*** App launched");
        Wait(g_executionSettings.GetAppActiveTimeMilliseconds());

        SweepIfNeeded();
    }

    static void RunApp(const wchar_t* appId, const wchar_t* args)
    {
        LOG_OUTPUT(L"*** App Id      : %s", appId);
        LOG_OUTPUT(L"*** App args    : %s", args);
        LOG_OUTPUT(L"*** Iterations  : %d", g_executionSettings.GetLaunchCount());

        Wait(g_executionSettings.GetPreRunTimeMilliseconds());

        for (int i = 1; i <= g_executionSettings.GetLaunchCount(); ++i)
        {
            LOG_OUTPUT(L"*** Iteration %d", i);
            AppLaunchAndTerminate(appId, args);
        }
    }

    static void CmdLineLaunchAndTerminate(const wchar_t* cmd, const wchar_t* args)
    {
        WEX::Common::String cmdLine;

        cmdLine.Format(
            L"%s %s",
            cmd,
            args);

        LOG_OUTPUT(L"*** Command line launched");

        wil::unique_handle processHandle = LaunchCmdLineToolAndReturn(cmdLine);
        LOG_OUTPUT(L"*** App launched");
        Wait(g_executionSettings.GetAppActiveTimeMilliseconds());
        
        SweepIfNeeded();

        ::WaitForSingleObject(
            processHandle.get(), g_executionSettings.GetAppActiveTimeMilliseconds());
        LOG_OUTPUT(L"*** Terminating command line app");
        TerminateProcessByHandle(std::move(processHandle));
        Wait(g_executionSettings.GetPostRunTimeMilliseconds());
    }

    static void RunCmdLine(const wchar_t* appId, const wchar_t* args)
    {
        LOG_OUTPUT(L"*** Command                : %s", appId);
        LOG_OUTPUT(L"*** Args                   : %s", args);
        LOG_OUTPUT(L"*** Launch iteration count : %d", g_executionSettings.GetLaunchCount());

        Wait(g_executionSettings.GetPreRunTimeMilliseconds());

        for (int i = 1; i <= g_executionSettings.GetLaunchCount(); ++i)
        {
            LOG_OUTPUT(L"*** Iteration %d", i);
            CmdLineLaunchAndTerminate(appId, args);
        }
    }

    bool AppPerfTests::ClassSetup()
    {
        Throw::IfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
        g_executionSettings = ExecutionSettings::ReadWithDefaults();
        return true;
    }

    void AppPerfTests::RunAppFromParameters()
    {
        if (g_executionSettings.RunAsCommandLine())
        {
            auto& app = g_executionSettings.GetAppToRun();
            String appfullpath = g_executionSettings.GetBinaryDirPath() + app;
            RunCmdLine(
                static_cast<const wchar_t*>(appfullpath),
                static_cast<const wchar_t*>(g_executionSettings.GetAppArgs()));
        }
        else
        {
            RunApp(
                static_cast<const wchar_t*>(g_executionSettings.GetAppToRun()),
                static_cast<const wchar_t*>(g_executionSettings.GetAppArgs()));
        }
    }
}
