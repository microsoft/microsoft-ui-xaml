// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LoadLibraryAbs.h"
#include <windows.h>
#include <string>
#include <minerror.h>
#include <shlwapi.h>
#include <strsafe.h>

static const std::wstring& GetMuxPath()
{
    static const std::wstring muxPath = [] {
        WCHAR muxPathBuffer[MAX_PATH];
        GetModuleFileName(GetModuleHandle(L"Microsoft.UI.Xaml.dll"), muxPathBuffer, MAX_PATH);
        FAIL_FAST_ASSERT(PathRemoveFileSpecW(muxPathBuffer) != 0);
        std::wstring result(muxPathBuffer);
        if (result.back() != L'\\')
        {
            result += L"\\";
        }
        return result;
    }();

    return muxPath;
}

std::wstring GetMuxAbsPath(_In_ LPCWSTR lpLibFileName)
{
    FAIL_FAST_ASSERT(lpLibFileName != nullptr && L'\0' != lpLibFileName[0]);
    auto muxPath = GetMuxPath();
    FAIL_FAST_ASSERT(!muxPath.empty());
    
    if (PathIsRelativeW(lpLibFileName))
    {
        return muxPath + lpLibFileName;
    }
    else
    {
        // Absolute path, so ensure it's inside muxPath
        FAIL_FAST_ASSERT(_wcsnicmp(lpLibFileName, muxPath.c_str(), muxPath.size()) == 0);
        return lpLibFileName;
    }
}

HMODULE WINAPI LoadLibraryExWAbs(
    _In_ LPCWSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
    )
{
    // Ensure caller's flags are compatible with LOAD_WITH_ALTERED_SEARCH_PATH
    FAIL_FAST_ASSERT((dwFlags & (LOAD_LIBRARY_SEARCH_APPLICATION_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS|LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32|LOAD_LIBRARY_SEARCH_USER_DIRS)) == 0);
    return LoadLibraryExW(GetMuxAbsPath(lpLibFileName).c_str(), hFile, dwFlags | LOAD_WITH_ALTERED_SEARCH_PATH);
}

BOOL WINAPI GetModuleHandleExWAbs(
    _In_ DWORD dwFlags,
    _In_opt_ LPCWSTR lpModuleName,
    _Out_ HMODULE* phModule
    )
{
    return GetModuleHandleExW(dwFlags, GetMuxAbsPath(lpModuleName).c_str(), phModule);
}