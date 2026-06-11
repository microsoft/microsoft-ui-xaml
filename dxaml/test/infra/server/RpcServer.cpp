// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RpcServer.h"

#include "Sddl.h"

#include <RpcHandle.h>
#include "UtilitiesRoutineHelper.h"
#include <ShellScalingApi.h>
#include "PredictableDManipEnablerServer.h"

#include <vector>
#include <memory>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;

static std::unique_ptr<PredictableDManipEnablerServer> spPredictableDManipEnabler;

HRESULT RpcServerStart()
{
    UtilitiesRoutineHelper::CheckForBVTMode();

    COM_START_GROUP(L"RpcServerStart")
    {
        spPredictableDManipEnabler.reset(new PredictableDManipEnablerServer());

        if (UtilitiesRoutineHelper::IsDesktop())
        {
            bool shouldSkipMinimize = false;
            String value;
            if (SUCCEEDED(RuntimeParameters::TryGetValue(L"SkipConsoleWindowMinimize", value)))
            {
                if (!value.IsEmpty())
                {
                    shouldSkipMinimize = true;
                }
            }

            if(!shouldSkipMinimize)
            {
                // With MDA there exists a race condition in win32k where the console window and MDA window will
                // fight to end up on top. We call ShowWindow SW_SHOWMINNOACTIVE here to make the console window not active.
                const auto consoleWindow = GetConsoleWindow();
                // It's possible to load the test infrastructure client in normal apps as well as TAEF processes
                // and indeed our performance tests do this. In that case we don't have a Console window HWND and
                // need to perform no action here. Also, TAEF tests that use TE.ProcessHost.UAP.exe will not have
                // a console window.
                if (consoleWindow != NULL)
                {
                    LOG_OUTPUT(L"Minimizing console window");
                    Throw::LastErrorIfFalse(!!::ShowWindow(consoleWindow, SW_SHOWMINNOACTIVE), L"Failed to call ShowWindow on the console window.");
                }
            }

            PROCESS_DPI_AWARENESS dpiAwareness = {};
            LogThrow_IfFailed(::GetProcessDpiAwareness(nullptr, &dpiAwareness));
            LOG_OUTPUT(L"Server: Current DPI awareness is %d", static_cast<int>(dpiAwareness));
            if (dpiAwareness != PROCESS_PER_MONITOR_DPI_AWARE)
            {
                LOG_OUTPUT(L"Server: Setting DPI awareness to PROCESS_PER_MONITOR_DPI_AWARE");
                HRESULT hr = ::SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
                // SetProcessDpiAwareness will return an E_ACCESSDENIED if we set dpi awareness more then once per process
                Throw::IfFalse(hr == S_OK || hr == E_ACCESSDENIED, hr);
            }

            LOG_OUTPUT(L"Server: Setting thread DPI awareness context to DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2");
            DPI_AWARENESS_CONTEXT oldDpiAwarenessContext = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

            // DPI awareness contexts contain informational flags and can't be bitwise compared.
            if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            {
                LOG_OUTPUT(L"Server: Old DPI awareness was DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2");
            }
            else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
            {
                LOG_OUTPUT(L"Server: Old DPI awareness was DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE");
            }
            else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
            {
                LOG_OUTPUT(L"Server: Old DPI awareness was DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED");
            }
            else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
            {
                LOG_OUTPUT(L"Server: Old DPI awareness was DPI_AWARENESS_CONTEXT_SYSTEM_AWARE");
            }
            else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE))
            {
                LOG_OUTPUT(L"Server: Old DPI awareness was DPI_AWARENESS_CONTEXT_UNAWARE");
            }
        }

        LogThrow_IfFailed(UtilitiesRoutineHelper::CheckAndKillServerManager());

        // This security descriptor allows for anonymous access from
        // app containers, which is above and beyond what a default
        // security descriptor would allow.
        PSECURITY_DESCRIPTOR pSecurityDescriptor;
        Throw::LastErrorIf(!ConvertStringSecurityDescriptorToSecurityDescriptor(
            L"D:(A;;GRGWGX;;;WD)(A;;GRGWGX;;;AC)(A;;GRGWGX;;;AN)",
            SDDL_REVISION_1,
            &pSecurityDescriptor,
            NULL));

        RPC_STATUS status = RPC_S_OK;
        status = RpcServerUseProtseqEp(
            // ncalrpc is a local binary RPC format.
            static_cast<RPC_WSTR>(const_cast<WCHAR*>(L"ncalrpc")),
            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
            // RpcInterface is our named RPC endpoint.
            static_cast<RPC_WSTR>(const_cast<WCHAR*>(L"RpcInterface")),
            pSecurityDescriptor);

        // We could get RPC_S_DUPLICATE_ENDPOINT if the service has already been
        // started/stopped in the same process; don't treat it as an error.
        Throw::IfFalse(status == RPC_S_OK || status == RPC_S_DUPLICATE_ENDPOINT, HRESULT_FROM_WIN32(status));

        status = RpcServerRegisterIf3(
            // Interface to register.
            RpcInterface_v1_0_s_ifspec,
            // Use the MIDL generated entry-point vector.
            NULL,
            // Use the MIDL generated entry-point vector.
            NULL,
            // No authentication is required, this is local. The
            // server will start listening immediately.
            RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH | RPC_IF_AUTOLISTEN,
            // Allow the default max number of calls.
            RPC_C_LISTEN_MAX_CALLS_DEFAULT,
            // No size restrictions
            static_cast<unsigned int>(-1),
            // No security callback function
            NULL,
            pSecurityDescriptor);
        Throw::IfFalse(status == RPC_S_OK, HRESULT_FROM_WIN32(status));
    }
    COM_END
}

HRESULT RpcServerStop()
{
    COM_START_GROUP(L"RpcServerStop")
    {
        RPC_STATUS status = RpcServerUnregisterIf(
            // Interface to unregister.
            RpcInterface_v1_0_s_ifspec,
            // Use the MIDL generated entry-point vector.
            NULL,
            // Wait for current calls to complete.
            TRUE);

        spPredictableDManipEnabler.reset();

        Throw::IfFalse(status == RPC_S_OK, HRESULT_FROM_WIN32(status));
    }
    COM_END
}

void __RPC_USER MIDL_user_free(_Pre_maybenull_ _Post_invalid_ void* p)
{
    delete [] (BYTE*)p;
}

_Must_inspect_result_
_Ret_maybenull_ _Post_writable_byte_size_(size)
void* __RPC_USER MIDL_user_allocate(_In_ size_t size)
{
    return (void *)new BYTE[size];
}