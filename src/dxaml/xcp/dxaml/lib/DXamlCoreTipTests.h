// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowsAppSdk-ProductInfo.h"

#define wil_details_GetKernelBaseModuleHandle() GetModuleHandleW(L"kernelbase.dll")
#include <tip/tip.h>
// From tip/tson_string.h's try/catch: C++ exception handler used, but unwind semantics are not enabled.
#pragma warning (disable: 4530)
#include <tip/tson_string.h>
#include <memory>

// Stringify helpers for compile-time version string
#define MUX_VER_WSTR2(x) L#x
#define MUX_VER_WSTR(x) MUX_VER_WSTR2(x)

class TipTestHelper
{
public:

    static std::wstring GetMuxVersion()
    {
        // Compile-time version for TIP test logging.
        return MUX_VER_WSTR(WINUI_RELEASE_MAJOR) L"."
               MUX_VER_WSTR(WINDOWSAPPSDK_RELEASE_MAJOR) L"."
               MUX_VER_WSTR(WINDOWSAPPSDK_RELEASE_MINOR) L"."
               MUX_VER_WSTR(WINUI_BUILD_VERSION);
    }
};

TIP_declare_test(DXamlInitializeCoreTest, 42637712)
{
    TIP_set_process_scope();

    enum class reason
    {
        packaged_process,
        init_type_uwp,
        init_type_main_view,
        init_type_islands_only,
        disabled_tsf3,
        initialized_dispatcher,
        created_dispatcher_xcpwindow,
        created_uwp_window,
        failed_dxamlcore_init
    };

    // Test Properties
    std::wstring muxVersion{};

    TIP_require_any_flag_set(reason::init_type_uwp, reason::init_type_main_view, reason::init_type_islands_only);
    TIP_require_all_flags_set(reason::initialized_dispatcher, reason::created_uwp_window, reason::created_dispatcher_xcpwindow);
    TIP_require_flag_clear(reason::failed_dxamlcore_init);

    // Routine to evaluate success/failure
    void evaluate()
    {
        TIP_fail_if_complete_not_called();
        TIP_succeed();
    }

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(TIP_value(muxVersion));
    }
};

#undef wil_details_GetKernelBaseModuleHandle
