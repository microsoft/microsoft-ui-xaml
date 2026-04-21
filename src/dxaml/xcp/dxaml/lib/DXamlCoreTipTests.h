// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.


#define wil_details_GetKernelBaseModuleHandle() GetModuleHandleW(L"kernelbase.dll")
#include <tip/tip.h>
// From tip/tson_string.h's try/catch: C++ exception handler used, but unwind semantics are not enabled.
#pragma warning (disable: 4530)
#include <tip/tson_string.h>
#include <memory>

class TipTestHelper
{
public:

    static std::wstring GetMuxVersion() 
    {
        // Empty wstring for failures
        std::wstring emptyVersion = L"";

        // Get Microsoft.UI.Xaml.dll version    
        WCHAR fullModulePath[MAX_PATH];

        // Get the handle to the current Microsoft.UI.Xaml.dll module
        HMODULE thisModule;
        if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"", &thisModule))
        {
            return emptyVersion;
        }

        // Get the full path of the current Microsoft.UI.Xaml.dll module
        if (!GetModuleFileName(thisModule, fullModulePath, sizeof(fullModulePath)))
        {
            return emptyVersion;
        }

        WCHAR muxVersion[200];
        const DWORD fileVersionSize = GetFileVersionInfoSizeW(fullModulePath, nullptr);

        if (fileVersionSize > 0)
        {
            auto dataBlock = std::make_unique<WCHAR[]>(fileVersionSize);

            // Store file-version info in dataBlock
            if (GetFileVersionInfoW(fullModulePath, 0, fileVersionSize, dataBlock.get()))
            {
                VS_FIXEDFILEINFO* pFileInfo = nullptr;
                UINT fileInfoLen = 0;
                // Store specific version info into pFileInfo.
                BOOL retVal = VerQueryValueA(dataBlock.get(), "\\", reinterpret_cast<LPVOID*>(&pFileInfo), &fileInfoLen);

                if (retVal != FALSE && pFileInfo != nullptr)
                {
                    swprintf_s(muxVersion, ARRAYSIZE(muxVersion), L"%u.%u.%u.%u", HIWORD(pFileInfo->dwFileVersionMS), LOWORD(pFileInfo->dwFileVersionMS), HIWORD(pFileInfo->dwFileVersionLS), LOWORD(pFileInfo->dwFileVersionLS));
                    return muxVersion;
                }
            }
        }
        // Empty wstring on failure
        return emptyVersion;
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
