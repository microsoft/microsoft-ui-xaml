// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <RpcServer.h>
#include <ResourcesPriHelper.h>
#include <WexTestClass.h>
#include <RuntimeParameters.h>

#include <crtdbg.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <roapi.h>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

#include <Versioning.h>

#define LOG_OUTPUT(fmt, ...) WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))

// These methods will be called from the out-of-proc loaded instance of
// this DLL. They will register our RPC server.

BEGIN_MODULE()
    MODULE_PROPERTY(L"RunFixtureAs:Module", L"ElevatedUserOrSystem")
    MODULE_PROPERTY(L"EnsureLoggedOnUser:UserCount", L"1")
    MODULE_PROPERTY(L"DeploymentItem", L"..\\EtwProcessor.dll")
    MODULE_PROPERTY(L"ThreadingModel[@HostingMode='WPF']", L"STA")
    MODULE_PROPERTY(L"UAP:Host[@HostingMode='Win32Explicit']", L"PackagedCwa")
    MODULE_PROPERTY(L"UAP:Host[@HostingMode='WPF']", L"PackagedCwa")
    MODULE_PROPERTY(L"UAP:AppXManifest[@HostingMode='Win32Explicit']", APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL)
    MODULE_PROPERTY(L"UAP:AppXManifest[@HostingMode='WPF']", APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL)
    MODULE_PROPERTY(L"UAP:AppXManifest[default]", APPXMANIFEST_WINDOWS_VERSION_CURRENT)
    MODULE_PROPERTY(L"UAP:WaitForXamlWindowActivation", L"false")
    MODULE_PROPERTY(L"Hosting:Mode", L"WPF")
    MODULE_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS4)
    MODULE_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
END_MODULE()

MODULE_SETUP(ModuleSetup);
MODULE_CLEANUP(ModuleCleanup);

// Nice helper functions to split string
// https://stackoverflow.com/questions/236129/most-elegant-way-to-split-a-string
namespace StringHelpers
{
    template<typename Out>
    void Split(const std::string &s, char delim, Out result) {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }

    std::vector<std::string> Split(const std::string &s, char delim) {
        std::vector<std::string> elems;
        Split(s, delim, std::back_inserter(elems));
        return elems;
    }
}

// In debug builds, suppress CRT dialog boxes that hang automated test
// runs, and crash immediately on CRT asserts/errors.  TAEF spawns
// multiple Te.ProcessHost.exe processes; ModuleSetup only runs in one
// of them.  A static constructor runs in ALL processes that load this
// DLL, so the suppression is always active.
#ifdef _DEBUG
namespace {
    int __cdecl CrashOnCrtFailure(int reportType, wchar_t* filename, int linenumber, wchar_t*, wchar_t* message)
    {
        if (reportType == _CRT_ASSERT || reportType == _CRT_ERROR)
        {
            wchar_t buf[512];
            swprintf_s(buf, L"*** CRT %s: %s [%s:%d]\n",
                reportType == _CRT_ASSERT ? L"ASSERT" : L"ERROR",
                message ? message : L"(no message)",
                filename ? filename : L"(unknown)", linenumber);
            OutputDebugStringW(buf);
            fwprintf(stderr, L"%s", buf);

            // Break into debugger if attached, then failfast so WER
            // collects a crash dump for the test harness.
            if (IsDebuggerPresent()) __debugbreak();
            __fastfail(FAST_FAIL_FATAL_APP_EXIT);
        }
        return -1;  // _CRT_WARN: suppress dialog, keep running
    }

    void ApplyCrtSuppression()
    {
        _set_error_mode(_OUT_TO_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_DEBUG);
        _CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, CrashOnCrtFailure);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    }

    struct CrtSuppressor {
        CrtSuppressor() { ApplyCrtSuppression(); }
    } g_crtSuppressor;
}
#endif

bool ModuleSetup()
{
#ifdef _DEBUG
    // Re-apply: other DLLs loaded since the static constructor may
    // have overridden our CRT report settings.
    ApplyCrtSuppression();
#endif

    // SwitcherMode test parameter — when /p:SwitcherMode=true is passed on the te.exe
    // command line, enable the lifted system-composition switcher process-wide before
    // any Compositor object is created. Must run before RpcServerStart so any
    // Compositor created on the test server side picks up the switched backend.
    //
    // This is the single opt-in point used by the entire 1,416-test switcher-coverage
    // run defined in docs/SwitcherTestCoverageScope.md. Mirrors the per-class
    // CompositionEngine::TrySetProcessEngine(System) call already present in
    // SwitcherTests::ClassSetup; centralizing it here lets any test in any DLL that
    // links this shared infra opt in via the same flag.
    //
    // Hard-fail policy: if the caller explicitly requested SwitcherMode=true but
    // switcher cannot engage (API missing, OS feature flag off, etc.), fail the
    // whole module loudly rather than silently running tests in baseline mode and
    // producing a false-positive "switcher coverage" green run.
    WEX::Common::String switcherModeParam;
    if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"SwitcherMode", switcherModeParam))
        && (switcherModeParam.CompareNoCase(L"true") == 0 || switcherModeParam == L"1"))
    {
        try
        {
            // UAP/packaged hosting may enter ModuleSetup before COM/WinRT is initialized on this
            // thread, making the WinRT CompositionEngine activation fail with CO_E_NOTINITIALIZED
            // (0x800401F0). Ensure an MTA apartment; leave it up for the host (WPF already inits).
            ::RoInitialize(RO_INIT_MULTITHREADED);

            bool ok = Microsoft::UI::Composition::CompositionEngine::TrySetProcessEngine(
                Microsoft::UI::Composition::CompositionEngineType::System);

            if (!ok)
            {
                LOG_OUTPUT(L"FAIL: SwitcherMode=true requested but TrySetProcessEngine(System) did not engage (ok=%d)",
                    static_cast<int>(ok));
                return false;
            }
            LOG_OUTPUT(L"SwitcherMode=true: switcher engaged process-wide via TrySetProcessEngine(System)");
        }
        catch (Platform::Exception^ ex)
        {
            LOG_OUTPUT(L"FAIL: SwitcherMode=true requested but CompositionEngine API not available (hr=0x%08x)",
                ex->HResult);
            return false;
        }
    }

    VERIFY_SUCCEEDED(ConfigureResourcesPri(false /* configureManaged */));
    VERIFY_SUCCEEDED(RpcServerStart());
    return true;
}

bool ModuleCleanup()
{
    VERIFY_SUCCEEDED(RpcServerStop());
    return true;
}
